/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "finddialog.hxx"

#include "workbench.hxx"
#include "services/common.hxx"

#include <com/sun/star/code/search/FindOptions.hpp>
#include <com/sun/star/code/document/XDocumentManager.hpp>
#include <com/sun/star/code/document/XTextDocument.hpp>
#include <com/sun/star/code/document/TextEdit.hpp>
#include <com/sun/star/code/document/theDocumentManager.hpp>
#include <com/sun/star/code/search/FindResult.hpp>
#include <com/sun/star/code/search/FindScope.hpp>
#include <com/sun/star/code/search/XFindEnumeration.hpp>
#include <com/sun/star/code/search/XFinder.hpp>
#include <com/sun/star/code/search/theFinder.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <comphelper/configuration.hxx>
#include <comphelper/processfactory.hxx>
#include <sal/log.hxx>
#include <officecfg/Office/Code.hxx>
#include <osl/file.hxx>
#include <osl/process.h>
#include <vcl/svapp.hxx>
#include <vcl/vclenum.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/Button.hxx>
#include <vcl/weld/CheckButton.hxx>
#include <vcl/weld/ComboBox.hxx>
#include <vcl/weld/RadioButton.hxx>
#include <vcl/weld/Container.hxx>
#include <vcl/weld/Dialog.hxx>
#include <vcl/weld/Label.hxx>
#include <vcl/weld/MessageDialog.hxx>
#include <vcl/weld/Notebook.hxx>

#include <com/sun/star/uno/Sequence.hxx>
#include <comphelper/sequence.hxx>

#include <unicode/regex.h>
#include <unicode/uchar.h>
#include <unicode/unistr.h>

#include <map>
#include <algorithm>
#include <vector>

namespace code
{
namespace
{
using Find = officecfg::Office::Code::Find;

/// the most entries a history keeps
constexpr size_t MaxHistory = 20;

/// what both tabs show alike, so it goes from one tab to the other
struct FindState
{
    OUString aFindWhat;
    OUString aReplaceWith;
    OUString aLookIn;
    OUString aFileTypes;
    bool bMatchCase = false;
    bool bWholeWord = false;
    bool bRegex = false;
    bool bPreserveCase = false;
    bool bExternal = false;
    bool bMisc = false;
    bool bSubfolders = true;
    sal_Int32 nSelection = 0;
};

/// a history of the configuration; none is stored until the first search
template <class T> css::uno::Sequence<OUString> Hist()
{
    return T::get().value_or(css::uno::Sequence<OUString>());
}

/// the scope that is one document: there is no file to skip in it
OUString CurrentDocument() { return u"Current document"_ustr; }

/// Preserve case: the replacement takes the case of what it replaces, all
/// upper, all lower or a capital first letter; a mixed case is left as it is.
OUString PreserveCase(const OUString& rFound, const OUString& rReplacement)
{
    bool bLetters = false, bUpper = true, bLower = true;
    for (sal_Int32 i = 0; i < rFound.getLength(); ++i)
    {
        const sal_Unicode c = rFound[i];
        if (!u_isalpha(c))
            continue;
        bLetters = true;
        if (u_isupper(c))
            bLower = false;
        else
            bUpper = false;
    }
    if (!bLetters || rReplacement.isEmpty())
        return rReplacement;
    if (bUpper && rFound.getLength() > 1)
        return rReplacement.toAsciiUpperCase();
    if (bLower)
        return rReplacement.toAsciiLowerCase();
    // a capital first letter, the rest lower
    if (u_isupper(rFound[0]) && rFound.getLength() > 1)
    {
        bool bRestLower = true;
        for (sal_Int32 i = 1; i < rFound.getLength(); ++i)
            if (u_isupper(rFound[i]))
                bRestLower = false;
        if (bRestLower)
            return OUString::Concat(rReplacement.copy(0, 1).toAsciiUpperCase())
                   + rReplacement.copy(1).toAsciiLowerCase();
    }
    if (bUpper)
        return rReplacement.toAsciiUpperCase();
    return rReplacement;
}

/// the directory the "Current directory" scope stands for
OUString CurrentDirectory()
{
    OUString aUrl, aPath;
    if (osl_getProcessWorkingDir(&aUrl.pData) == osl_Process_E_None
        && osl::FileBase::getSystemPathFromFileURL(aUrl, aPath) == osl::FileBase::E_None)
        return aPath;
    return OUString();
}

void FillHistory(weld::ComboBox& rBox, const css::uno::Sequence<OUString>& rHistory)
{
    for (const OUString& rEntry : rHistory)
        rBox.append_text(rEntry);
}

/// puts rText on the top of the list, the way the lists of the other IDEs do
std::vector<OUString> Remember(const css::uno::Sequence<OUString>& rOld, const OUString& rText)
{
    std::vector<OUString> aNew;
    if (!rText.isEmpty())
        aNew.push_back(rText);
    for (const OUString& rEntry : rOld)
        if (rEntry != rText && aNew.size() < MaxHistory)
            aNew.push_back(rEntry);
    return aNew;
}

/// One tab: the Find in Files and the Replace in Files tab are the same page,
/// the latter with the "Replace with" row and Replace All in place of Find All.
class FindPage
{
public:
    FindPage(weld::Container* pHost, bool bReplace, const OUString& rCurrentDir)
        : m_xBuilder(Application::CreateBuilder(pHost, u"code/ui/findpage.ui"_ustr))
        , m_xRoot(m_xBuilder->weld_container(u"FindPage"_ustr))
        , m_xReplaceLabel(m_xBuilder->weld_label(u"replacelabel"_ustr))
        , m_xReplaceWith(m_xBuilder->weld_combo_box(u"replacewith"_ustr))
        , m_xFindWhat(m_xBuilder->weld_combo_box(u"findwhat"_ustr))
        , m_xMatchCase(m_xBuilder->weld_check_button(u"matchcase"_ustr))
        , m_xWholeWord(m_xBuilder->weld_check_button(u"wholeword"_ustr))
        , m_xRegex(m_xBuilder->weld_check_button(u"useregex"_ustr))
        , m_xPreserveCase(m_xBuilder->weld_check_button(u"preservecase"_ustr))
        , m_xLookIn(m_xBuilder->weld_combo_box(u"lookin"_ustr))
        , m_xSubfolders(m_xBuilder->weld_check_button(u"includesubfolders"_ustr))
        , m_xSelectionBox(m_xBuilder->weld_widget(u"selectionbox"_ustr))
        , m_xEntireDocument(m_xBuilder->weld_radio_button(u"entiredocument"_ustr))
        , m_xCurrentSelection(m_xBuilder->weld_radio_button(u"currentselection"_ustr))
        , m_xCurrentBlock(m_xBuilder->weld_radio_button(u"currentblock"_ustr))
        , m_xExternal(m_xBuilder->weld_check_button(u"includeexternal"_ustr))
        , m_xMisc(m_xBuilder->weld_check_button(u"includemisc"_ustr))
        , m_xFileTypes(m_xBuilder->weld_combo_box(u"filetypes"_ustr))
        , m_xFindPrev(m_xBuilder->weld_button(u"findprev"_ustr))
        , m_xFindNext(m_xBuilder->weld_button(u"findnext"_ustr))
        , m_xSkipFile(m_xBuilder->weld_button(u"skipfile"_ustr))
        , m_xReplaceNext(m_xBuilder->weld_button(u"replacenext"_ustr))
        , m_xFindAll(m_xBuilder->weld_button(u"findall"_ustr))
    {
        m_xLookIn->append_text(u"Entire solution"_ustr);
        m_xLookIn->append_text(u"Current project"_ustr);
        m_xLookIn->append_text(u"Current directory (" + rCurrentDir + u")");
        m_xLookIn->append_text(u"All open documents"_ustr);
        m_xLookIn->append_text(CurrentDocument());
        m_xLookIn->connect_changed(LINK(this, FindPage, LookInHdl));

        FillHistory(*m_xFindWhat, Hist<Find::FindHistory>());
        FillHistory(*m_xReplaceWith, Hist<Find::ReplaceHistory>());
        FillHistory(*m_xFileTypes, Hist<Find::FileTypesHistory>());

        if (bReplace)
        {
            m_xReplaceLabel->show();
            m_xReplaceWith->show();
            m_xReplaceNext->show();
            m_xPreserveCase->show();
            // the same button, in the same place: Replace All in place of Find All
            m_xFindAll->set_label(u"Replace All"_ustr);
        }
    }

    void SetButtonHandler(const Link<weld::Button&, void>& rLink)
    {
        for (weld::Button* p : { m_xFindPrev.get(), m_xFindNext.get(), m_xSkipFile.get(),
                                 m_xReplaceNext.get(), m_xFindAll.get() })
            p->connect_clicked(rLink);
    }

    void Show(bool bShow)
    {
        if (bShow)
            m_xRoot->show();
        else
            m_xRoot->hide();
    }

    FindState Get() const
    {
        FindState aState;
        aState.aFindWhat = m_xFindWhat->get_active_text();
        aState.aReplaceWith = m_xReplaceWith->get_active_text();
        aState.aLookIn = m_xLookIn->get_active_text();
        aState.aFileTypes = m_xFileTypes->get_active_text();
        aState.bMatchCase = m_xMatchCase->get_active();
        aState.bWholeWord = m_xWholeWord->get_active();
        aState.bRegex = m_xRegex->get_active();
        aState.bPreserveCase = m_xPreserveCase->get_active();
        aState.bExternal = m_xExternal->get_active();
        aState.bMisc = m_xMisc->get_active();
        aState.bSubfolders = m_xSubfolders->get_active();
        aState.nSelection = m_xCurrentBlock->get_active() ? 2 : m_xCurrentSelection->get_active() ? 1 : 0;
        return aState;
    }

    void Set(const FindState& rState)
    {
        m_xFindWhat->set_entry_text(rState.aFindWhat);
        m_xReplaceWith->set_entry_text(rState.aReplaceWith);
        m_xLookIn->set_entry_text(rState.aLookIn);
        m_xFileTypes->set_entry_text(rState.aFileTypes);
        m_xMatchCase->set_active(rState.bMatchCase);
        m_xWholeWord->set_active(rState.bWholeWord);
        m_xRegex->set_active(rState.bRegex);
        m_xPreserveCase->set_active(rState.bPreserveCase);
        m_xExternal->set_active(rState.bExternal);
        m_xMisc->set_active(rState.bMisc);
        m_xSubfolders->set_active(rState.bSubfolders);
        (rState.nSelection == 2 ? m_xCurrentBlock : rState.nSelection == 1 ? m_xCurrentSelection
                                                                           : m_xEntireDocument)
            ->set_active(true);
        UpdateSkipFile();
    }

    /// the default of the scope is the first item, the entire solution
    void SetDefaultScope()
    {
        if (m_xLookIn->get_active_text().isEmpty())
            m_xLookIn->set_entry_text(m_xLookIn->get_text(0));
        UpdateSkipFile();
    }

    weld::Button* FindAllButton() { return m_xFindAll.get(); }
    weld::Button* ReplaceNextButton() { return m_xReplaceNext.get(); }
    weld::Button* FindNextButton() { return m_xFindNext.get(); }
    weld::Button* FindPrevButton() { return m_xFindPrev.get(); }
    weld::Button* SkipFileButton() { return m_xSkipFile.get(); }

    void FocusFindWhat() { m_xFindWhat->grab_focus(); }

private:
    DECL_LINK(LookInHdl, weld::ComboBox&, void);

    /// one document has no file to skip, no folders and no external items,
    /// but a part of it to search
    void UpdateSkipFile()
    {
        // the scopes are the items of the list, in this order
        const OUString aScope = m_xLookIn->get_active_text();
        const bool bDocument = aScope == CurrentDocument();
        const bool bSolution = aScope == m_xLookIn->get_text(0);
        const bool bProject = aScope == m_xLookIn->get_text(1);
        // the directory's item carries the path, which may be an earlier one
        const bool bDirectory = aScope.startsWith(u"Current directory");
        m_xSkipFile->set_sensitive(!bDocument);
        m_xSelectionBox->set_visible(bDocument);
        m_xSubfolders->set_visible(bDirectory);
        m_xExternal->set_visible(bSolution || bProject);
        m_xMisc->set_visible(bSolution);
    }

    std::unique_ptr<weld::Builder> m_xBuilder;
    std::unique_ptr<weld::Container> m_xRoot;
    std::unique_ptr<weld::Label> m_xReplaceLabel;
    std::unique_ptr<weld::ComboBox> m_xReplaceWith;
    std::unique_ptr<weld::ComboBox> m_xFindWhat;
    std::unique_ptr<weld::CheckButton> m_xMatchCase;
    std::unique_ptr<weld::CheckButton> m_xWholeWord;
    std::unique_ptr<weld::CheckButton> m_xRegex;
    std::unique_ptr<weld::CheckButton> m_xPreserveCase;
    std::unique_ptr<weld::ComboBox> m_xLookIn;
    std::unique_ptr<weld::CheckButton> m_xSubfolders;
    std::unique_ptr<weld::Widget> m_xSelectionBox;
    std::unique_ptr<weld::RadioButton> m_xEntireDocument;
    std::unique_ptr<weld::RadioButton> m_xCurrentSelection;
    std::unique_ptr<weld::RadioButton> m_xCurrentBlock;
    std::unique_ptr<weld::CheckButton> m_xExternal;
    std::unique_ptr<weld::CheckButton> m_xMisc;
    std::unique_ptr<weld::ComboBox> m_xFileTypes;
    std::unique_ptr<weld::Button> m_xFindPrev;
    std::unique_ptr<weld::Button> m_xFindNext;
    std::unique_ptr<weld::Button> m_xSkipFile;
    std::unique_ptr<weld::Button> m_xReplaceNext;
    std::unique_ptr<weld::Button> m_xFindAll;
};

IMPL_LINK_NOARG(FindPage, LookInHdl, weld::ComboBox&, void) { UpdateSkipFile(); }

class FindDialog
{
public:
    FindDialog(weld::Window* pParent, bool bReplaceTab,
               const css::code::search::FindOptions* pInitial)
        : m_xBuilder(Application::CreateBuilder(pParent, u"code/ui/finddialog.ui"_ustr))
        , m_xDialog(m_xBuilder->weld_dialog(u"FindDialog"_ustr))
        , m_xTabs(m_xBuilder->weld_notebook(u"tabs"_ustr))
        , m_pFindHost(m_xTabs->get_page(u"findtab"_ustr))
        , m_pReplaceHost(m_xTabs->get_page(u"replacetab"_ustr))
        , m_aFind(m_pFindHost, false, CurrentDirectory())
        , m_aReplace(m_pReplaceHost, true, CurrentDirectory())
    {
        FindState aState;
        aState.aFindWhat = OUString();
        aState.aLookIn = Find::LookIn::get();
        aState.aFileTypes = Find::FileTypes::get();
        aState.bMatchCase = Find::MatchCase::get();
        aState.bWholeWord = Find::MatchWholeWord::get();
        aState.bRegex = Find::UseRegex::get();
        aState.bPreserveCase = Find::PreserveCase::get();
        aState.bExternal = Find::IncludeExternal::get();
        aState.bMisc = Find::IncludeMisc::get();
        aState.bSubfolders = Find::IncludeSubfolders::get();
        aState.nSelection = Find::SelectionScope::get();
        // the latest search is what Find in Files opens with
        const css::uno::Sequence<OUString> aFinds = Hist<Find::FindHistory>();
        if (aFinds.hasElements())
            aState.aFindWhat = aFinds[0];
        const css::uno::Sequence<OUString> aReplaces = Hist<Find::ReplaceHistory>();
        if (aReplaces.hasElements())
            aState.aReplaceWith = aReplaces[0];
        if (pInitial)
            ApplyOptions(aState, *pInitial);
        m_aFind.Set(aState);
        m_aReplace.Set(aState);
        m_aFind.SetDefaultScope();
        m_aReplace.SetDefaultScope();

        m_xTabs->connect_enter_page(LINK(this, FindDialog, TabHdl));
        m_aFind.SetButtonHandler(LINK(this, FindDialog, ButtonHdl));
        m_aReplace.SetButtonHandler(LINK(this, FindDialog, ButtonHdl));

        m_nTab = bReplaceTab ? 1 : 0;
        m_xTabs->set_current_page(m_nTab);
        Active().FocusFindWhat();
    }

    ~FindDialog()
    {
        if (m_xEnum.is())
            m_xEnum->cancel();
    }

    enum class Result { None, FindAll, ReplaceAll };

    /// how the dialog was left; the options are then in rOptions (for Replace All
    /// with Replace and ReplaceWith set), rPreserveCase is the check box of Replace All
    Result Run(css::code::search::FindOptions& rOptions, bool& rPreserveCase)
    {
        m_xDialog->run();
        Store();
        rOptions = m_aOptions;
        rPreserveCase = m_bPreserveCase;
        return m_eResult;
    }

private:
    DECL_LINK(TabHdl, const OUString&, void);
    DECL_LINK(ButtonHdl, weld::Button&, void);

    /// what the page asks for, as the search service takes it
    static css::code::search::FindOptions OptionsOf(const FindState& rState)
    {
        using namespace css::code::search;
        FindOptions aOptions;
        aOptions.FindWhat = rState.aFindWhat;
        aOptions.MatchCase = rState.bMatchCase;
        aOptions.MatchWholeWord = rState.bWholeWord;
        aOptions.UseRegex = rState.bRegex;
        aOptions.IncludeSubfolders = rState.bSubfolders;
        aOptions.IncludeExternal = rState.bExternal;
        aOptions.IncludeMisc = rState.bMisc;
        aOptions.FileTypes = rState.aFileTypes;

        const OUString aScope = rState.aLookIn;
        // the scopes are the items of the list, in this order (see FindPage)
        if (aScope == u"Entire solution")
            aOptions.Scope = FindScope_ENTIRE_SOLUTION;
        else if (aScope == u"Current project")
            aOptions.Scope = FindScope_CURRENT_PROJECT;
        else if (aScope == u"All open documents")
            aOptions.Scope = FindScope_OPEN_DOCUMENTS;
        else if (aScope == CurrentDocument())
        {
            aOptions.Scope = FindScope_CURRENT_DOCUMENT;
            if (::code::Workbench* pWorkbench = ::code::Workbench::Instance())
                aOptions.DocumentUrl = pWorkbench->CurrentDocumentUrl();
            // one file is searched whatever its name is
            aOptions.FileTypes.clear();
        }
        else
        {
            aOptions.Scope = FindScope_DIRECTORY;
            // "Current directory (/path)", or a path that was typed
            OUString aPath = aScope;
            if (aScope.startsWith(u"Current directory (") && aScope.endsWith(u")"))
                aPath = aScope.copy(18, aScope.getLength() - 19);
            osl::FileBase::getFileURLFromSystemPath(aPath, aOptions.Directory);
        }
        return aOptions;
    }

    /// the page as it was when the search of rOptions was started
    static void ApplyOptions(FindState& rState, const css::code::search::FindOptions& rOptions)
    {
        using namespace css::code::search;
        rState.aFindWhat = rOptions.FindWhat;
        rState.bMatchCase = rOptions.MatchCase;
        rState.bWholeWord = rOptions.MatchWholeWord;
        rState.bRegex = rOptions.UseRegex;
        rState.bSubfolders = rOptions.IncludeSubfolders;
        rState.bExternal = rOptions.IncludeExternal;
        rState.bMisc = rOptions.IncludeMisc;
        rState.aFileTypes = rOptions.FileTypes;
        if (rOptions.Replace)
            rState.aReplaceWith = rOptions.ReplaceWith;
        switch (rOptions.Scope)
        {
            case FindScope_ENTIRE_SOLUTION:
                rState.aLookIn = u"Entire solution"_ustr;
                break;
            case FindScope_CURRENT_PROJECT:
                rState.aLookIn = u"Current project"_ustr;
                break;
            case FindScope_OPEN_DOCUMENTS:
                rState.aLookIn = u"All open documents"_ustr;
                break;
            case FindScope_CURRENT_DOCUMENT:
                rState.aLookIn = CurrentDocument();
                break;
            default:
            {
                OUString aPath;
                osl::FileBase::getSystemPathFromFileURL(rOptions.Directory, aPath);
                rState.aLookIn = u"Current directory (" + aPath + u")";
                break;
            }
        }
    }

    /// Find Next / Find Previous / Skip File: the results of the search are read
    /// as far as they are needed and the editor goes to the current one, which
    /// is the Find in Files list without the list.  nStep is 1, -1, or 0 for
    /// Skip File (the next result in another file).
    void Navigate(int nStep, bool bSkipFile)
    {
        css::code::search::FindOptions aOptions;
        try
        {
            aOptions = OptionsOf(Active().Get());
            const bool bChanged = !(aOptions == m_aNavOptions);
            if (!m_xEnum.is() || bChanged)
            {
                if (m_xEnum.is())
                    m_xEnum->cancel();
                m_aNavOptions = aOptions;
                m_aNavResults.clear();
                // the search starts again after a replace, at the same place
                if (bChanged)
                    m_nNavIndex = -1;
                m_xEnum = css::code::search::theFinder::get(comphelper::getProcessComponentContext())
                              ->find(aOptions);
            }
            // reads one more result; false when there are no more
            auto aRead = [this]
            {
                if (!m_xEnum.is() || !m_xEnum->hasMoreResults())
                    return false;
                m_aNavResults.push_back(m_xEnum->nextResult());
                return true;
            };
            const OUString aUrl
                = m_nNavIndex >= 0 ? m_aNavResults[m_nNavIndex].Url : OUString();
            if (nStep > 0)
            {
                sal_Int32 nNext = m_nNavIndex + 1;
                while (true)
                {
                    while (nNext >= static_cast<sal_Int32>(m_aNavResults.size()) && aRead())
                        ;
                    if (nNext >= static_cast<sal_Int32>(m_aNavResults.size()))
                    {
                        // the end: it goes on from the first one
                        if (m_aNavResults.empty() || nNext == 0)
                            break;
                        nNext = 0;
                        if (!bSkipFile)
                            break;
                        // skipping: the first result of another file than aUrl
                        while (nNext < static_cast<sal_Int32>(m_aNavResults.size())
                               && m_aNavResults[nNext].Url == aUrl)
                            ++nNext;
                        if (nNext >= static_cast<sal_Int32>(m_aNavResults.size()))
                            nNext = 0;
                        break;
                    }
                    if (!bSkipFile || aUrl.isEmpty() || m_aNavResults[nNext].Url != aUrl)
                        break;
                    ++nNext;
                }
                m_nNavIndex = m_aNavResults.empty() ? -1 : nNext;
            }
            else
            {
                if (m_nNavIndex <= 0)
                {
                    // before the first: it goes on from the last one
                    while (aRead())
                        ;
                    m_nNavIndex = static_cast<sal_Int32>(m_aNavResults.size()) - 1;
                }
                else
                    --m_nNavIndex;
            }
            if (m_nNavIndex < 0)
            {
                std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(
                    m_xDialog.get(), VclMessageType::Info, VclButtonsType::Ok,
                    u"No matches were found."_ustr));
                xBox->run();
                return;
            }
            const css::code::search::FindResult& rResult = m_aNavResults[m_nNavIndex];
            if (::code::Workbench* pWorkbench = ::code::Workbench::Instance())
                pWorkbench->revealRange(rResult.Url, rResult.Match, false);
        }
        catch (const css::lang::IllegalArgumentException& rException)
        {
            std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(
                m_xDialog.get(), VclMessageType::Warning, VclButtonsType::Ok, rException.Message));
            xBox->run();
        }
        catch (const css::uno::Exception& rException)
        {
            SAL_WARN("code", "cannot search: " << rException.Message);
        }
    }

    /// Replace Next: the current match is replaced, and the next one is shown.
    /// Without a current match it only goes to the first.
    void ReplaceNext()
    {
        const FindState aState = Active().Get();
        ::code::Workbench* pWorkbench = ::code::Workbench::Instance();
        if (m_nNavIndex < 0 || !pWorkbench || !m_xEnum.is()
            || !(OptionsOf(aState) == m_aNavOptions))
        {
            Navigate(1, false);
            return;
        }
        const css::code::search::FindResult aDone = m_aNavResults[m_nNavIndex];
        OUString aNew = aState.aReplaceWith;
        if (aState.bPreserveCase && aDone.Match.Start.Line == aDone.Match.End.Line
            && aDone.Match.End.Column <= aDone.LineText.getLength())
            aNew = PreserveCase(
                aDone.LineText.copy(aDone.Match.Start.Column,
                                    aDone.Match.End.Column - aDone.Match.Start.Column),
                aNew);
        pWorkbench->ReplaceRange(aDone.Url, aDone.Match, aNew);
        // the lines moved: the search is read again, from the match that was replaced
        m_xEnum->cancel();
        m_xEnum.clear();
        m_aNavResults.clear();
        --m_nNavIndex;
        // a replacement that holds the match itself is not found again
        const sal_Int32 nLine = aDone.Match.Start.Line;
        Navigate(1, false);
        if (m_nNavIndex >= 0 && m_aNavResults[m_nNavIndex].Url == aDone.Url
            && m_aNavResults[m_nNavIndex].Match.Start.Line == nLine
            && m_aNavResults[m_nNavIndex].Match.Start.Column == aDone.Match.Start.Column
            && !aState.aReplaceWith.isEmpty())
            Navigate(1, false);
    }

    FindPage& Active() { return m_nTab == 1 ? m_aReplace : m_aFind; }

    /// the options are one set for both tabs: the tab left hands them on
    void SwitchTo(int nTab)
    {
        if (nTab == m_nTab)
            return;
        const FindState aState = Active().Get();
        m_nTab = nTab;
        Active().Set(aState);
        Active().FocusFindWhat();
    }

    /// the entered texts go on the top of their histories, the options are kept
    void Store()
    {
        const FindState aState = Active().Get();
        auto xBatch = comphelper::ConfigurationChanges::create();
        Find::FindHistory::set(
            comphelper::containerToSequence(Remember(Hist<Find::FindHistory>(), aState.aFindWhat)),
            xBatch);
        if (m_nTab == 1)
            Find::ReplaceHistory::set(
                comphelper::containerToSequence(
                    Remember(Hist<Find::ReplaceHistory>(), aState.aReplaceWith)),
                xBatch);
        Find::FileTypesHistory::set(
            comphelper::containerToSequence(
                Remember(Hist<Find::FileTypesHistory>(), aState.aFileTypes)),
            xBatch);
        Find::LookIn::set(aState.aLookIn, xBatch);
        Find::FileTypes::set(aState.aFileTypes, xBatch);
        Find::MatchCase::set(aState.bMatchCase, xBatch);
        Find::MatchWholeWord::set(aState.bWholeWord, xBatch);
        Find::UseRegex::set(aState.bRegex, xBatch);
        Find::PreserveCase::set(aState.bPreserveCase, xBatch);
        Find::IncludeExternal::set(aState.bExternal, xBatch);
        Find::IncludeMisc::set(aState.bMisc, xBatch);
        Find::IncludeSubfolders::set(aState.bSubfolders, xBatch);
        Find::SelectionScope::set(aState.nSelection, xBatch);
        xBatch->commit();
    }

    std::unique_ptr<weld::Builder> m_xBuilder;
    std::unique_ptr<weld::Dialog> m_xDialog;
    std::unique_ptr<weld::Notebook> m_xTabs;
    // owned by the notebook
    weld::Container* m_pFindHost;
    weld::Container* m_pReplaceHost;
    FindPage m_aFind;
    FindPage m_aReplace;
    int m_nTab = 0;
    Result m_eResult = Result::None;
    bool m_bPreserveCase = false;
    // the search Find Next and the others walk through
    css::uno::Reference<css::code::search::XFindEnumeration> m_xEnum;
    css::code::search::FindOptions m_aNavOptions;
    std::vector<css::code::search::FindResult> m_aNavResults;
    sal_Int32 m_nNavIndex = -1;
    css::code::search::FindOptions m_aOptions;
};

IMPL_LINK(FindDialog, TabHdl, const OUString&, rIdent, void)
{
    SwitchTo(m_xTabs->get_page_index(rIdent));
}

IMPL_LINK(FindDialog, ButtonHdl, weld::Button&, rButton, void)
{
    // Find All of the Find tab: the search is started once the dialog is gone.
    if (m_nTab == 0 && &rButton == Active().FindAllButton())
    {
        m_aOptions = OptionsOf(Active().Get());
        m_eResult = Result::FindAll;
        m_xDialog->response(RET_OK);
        return;
    }
    if (m_nTab == 1 && &rButton == Active().FindAllButton())
    {
        const FindState aState = Active().Get();
        m_aOptions = OptionsOf(aState);
        m_aOptions.Replace = true;
        m_aOptions.ReplaceWith = aState.aReplaceWith;
        m_bPreserveCase = aState.bPreserveCase;
        m_eResult = Result::ReplaceAll;
        m_xDialog->response(RET_OK);
        return;
    }
    FindPage& rPage = Active();
    if (&rButton == rPage.ReplaceNextButton())
        ReplaceNext();
    else if (&rButton == rPage.FindNextButton())
        Navigate(1, false);
    else if (&rButton == rPage.FindPrevButton())
        Navigate(-1, false);
    else if (&rButton == rPage.SkipFileButton())
        Navigate(1, true);
    Store();
}

/// Replace All: every match of the search is replaced, in the open documents
/// and, opening them, in the others; the lines that were changed are shown as
/// the results of the search.
void ReplaceAll(const css::code::search::FindOptions& rOptions, bool bPreserveCase)
{
    using namespace css::code::search;
    namespace doc = css::code::document;
    const css::uno::Reference<XFinder> xFinder
        = theFinder::get(comphelper::getProcessComponentContext());
    const css::uno::Reference<doc::XDocumentManager> xDocuments
        = doc::theDocumentManager::get(comphelper::getProcessComponentContext());
    ::code::Workbench* pWorkbench = ::code::Workbench::Instance();

    // the lines the search finds, by file
    std::map<OUString, std::vector<sal_Int32>> aLines;
    const css::uno::Reference<XFindEnumeration> xFound = xFinder->find(rOptions);
    while (xFound->hasMoreResults())
    {
        const FindResult aFound = xFound->nextResult();
        aLines[aFound.Url].push_back(aFound.Match.Start.Line);
    }
    const sal_Int32 nFilesSearched = xFound->getFilesSearched();

    UErrorCode eError = U_ZERO_ERROR;
    const OUString aPattern = codeservices::FindPattern(rOptions);
    icu::RegexMatcher aMatcher(
        icu::UnicodeString(reinterpret_cast<const char16_t*>(aPattern.getStr()),
                           aPattern.getLength()),
        rOptions.MatchCase ? 0 : UREGEX_CASE_INSENSITIVE, eError);
    if (U_FAILURE(eError) || !pWorkbench)
        return;

    std::vector<FindResult> aChanged;
    for (const auto& [rUrl, rFileLines] : aLines)
    {
        css::uno::Reference<doc::XTextDocument> xText;
        try
        {
            xText.set(xDocuments->openDocument(rUrl), css::uno::UNO_QUERY);
        }
        catch (const css::uno::Exception&)
        {
        }
        if (!xText.is())
            continue;

        struct Edit
        {
            css::code::core::Range aRange;
            OUString aNew;
        };
        // from the end of the file, so that what was not changed yet keeps its place
        for (auto it = rFileLines.rbegin(); it != rFileLines.rend(); ++it)
        {
            const sal_Int32 nLine = *it;
            OUString aLine;
            try
            {
                aLine = xText->getLine(nLine);
            }
            catch (const css::uno::Exception&)
            {
                continue;
            }
            const icu::UnicodeString aUnicode(reinterpret_cast<const char16_t*>(aLine.getStr()),
                                              aLine.getLength());
            aMatcher.reset(aUnicode);
            std::vector<Edit> aEdits;
            icu::UnicodeString aNewLine;
            sal_Int32 nPreviousEnd = 0;
            sal_Int32 nFirstStart = -1, nFirstEnd = 0;
            eError = U_ZERO_ERROR;
            while (aMatcher.find(eError) && U_SUCCESS(eError))
            {
                const sal_Int32 nStart = aMatcher.start(eError), nEnd = aMatcher.end(eError);
                OUString aReplacement = rOptions.ReplaceWith;
                if (bPreserveCase)
                    aReplacement = PreserveCase(aLine.copy(nStart, nEnd - nStart), aReplacement);
                if (!rOptions.UseRegex)
                {
                    // taken literally
                    OUStringBuffer aEscaped;
                    for (sal_Int32 i = 0; i < aReplacement.getLength(); ++i)
                    {
                        if (aReplacement[i] == '$' || aReplacement[i] == '\\')
                            aEscaped.append('\\');
                        aEscaped.append(aReplacement[i]);
                    }
                    aReplacement = aEscaped.makeStringAndClear();
                }
                const sal_Int32 nNewStart = aNewLine.length() + (nStart - nPreviousEnd);
                aMatcher.appendReplacement(
                    aNewLine,
                    icu::UnicodeString(reinterpret_cast<const char16_t*>(aReplacement.getStr()),
                                       aReplacement.getLength()),
                    eError);
                nPreviousEnd = nEnd;
                Edit aEdit;
                aEdit.aRange.Start.Line = aEdit.aRange.End.Line = nLine;
                aEdit.aRange.Start.Column = nStart;
                aEdit.aRange.End.Column = nEnd;
                icu::UnicodeString aPart;
                aNewLine.extractBetween(nNewStart, aNewLine.length(), aPart);
                aEdit.aNew = OUString(reinterpret_cast<const sal_Unicode*>(aPart.getBuffer()),
                                      aPart.length());
                aEdits.push_back(std::move(aEdit));
                if (nFirstStart < 0)
                {
                    nFirstStart = nNewStart;
                    nFirstEnd = aNewLine.length();
                }
                // an empty match would not go on
                if (nEnd == nStart && nEnd >= aLine.getLength())
                    break;
            }
            if (aEdits.empty())
                continue;
            aMatcher.appendTail(aNewLine);

            for (auto e = aEdits.rbegin(); e != aEdits.rend(); ++e)
                pWorkbench->ReplaceRange(rUrl, e->aRange, e->aNew);

            FindResult aResult;
            aResult.Url = rUrl;
            aResult.LineText = OUString(reinterpret_cast<const sal_Unicode*>(aNewLine.getBuffer()),
                                        aNewLine.length());
            aResult.Match.Start.Line = aResult.Match.End.Line = nLine;
            aResult.Match.Start.Column = nFirstStart;
            aResult.Match.End.Column = nFirstEnd;
            aChanged.push_back(std::move(aResult));
        }
    }
    // in the order of the files and of the lines
    std::reverse(aChanged.begin(), aChanged.end());
    std::stable_sort(aChanged.begin(), aChanged.end(),
                     [](const FindResult& a, const FindResult& b) { return a.Url < b.Url; });
    xFinder->showResults(rOptions, comphelper::containerToSequence(aChanged), nFilesSearched);
}
}

void RunFindDialog(weld::Window* pParent, bool bReplaceTab,
                   const std::function<void()>& rShowResults,
                   const css::code::search::FindOptions* pInitial)
{
    css::code::search::FindOptions aOptions;
    bool bPreserveCase = false;
    FindDialog::Result eResult;
    {
        FindDialog aDialog(pParent, bReplaceTab, pInitial);
        eResult = aDialog.Run(aOptions, bPreserveCase);
        if (eResult == FindDialog::Result::None)
            return;
    }

    if (rShowResults)
        rShowResults();
    try
    {
        if (eResult == FindDialog::Result::ReplaceAll)
        {
            ReplaceAll(aOptions, bPreserveCase);
            return;
        }
        css::code::search::theFinder::get(comphelper::getProcessComponentContext())
            ->startSearch(aOptions);
    }
    catch (const css::lang::IllegalArgumentException& rException)
    {
        std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(
            pParent, VclMessageType::Warning, VclButtonsType::Ok, rException.Message));
        xBox->run();
    }
    catch (const css::uno::Exception& rException)
    {
        SAL_WARN("code", "cannot search: " << rException.Message);
    }
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
