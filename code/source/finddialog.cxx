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

#include <com/sun/star/code/search/FindOptions.hpp>
#include <com/sun/star/code/search/FindScope.hpp>
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

    /// true if the dialog was left with Find All; the options are then in rOptions
    bool Run(css::code::search::FindOptions& rOptions)
    {
        m_xDialog->run();
        Store();
        if (!m_bFindAll)
            return false;
        rOptions = m_aOptions;
        return true;
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
    bool m_bFindAll = false;
    css::code::search::FindOptions m_aOptions;
};

IMPL_LINK(FindDialog, TabHdl, const OUString&, rIdent, void)
{
    SwitchTo(m_xTabs->get_page_index(rIdent));
}

IMPL_LINK(FindDialog, ButtonHdl, weld::Button&, rButton, void)
{
    // Find All of the Find tab: the search is started once the dialog is gone.
    // The other buttons need the editor (Find Next, Replace) and are not there yet.
    if (m_nTab == 0 && &rButton == Active().FindAllButton())
    {
        m_aOptions = OptionsOf(Active().Get());
        m_bFindAll = true;
        m_xDialog->response(RET_OK);
        return;
    }
    Store();
}
}

void RunFindDialog(weld::Window* pParent, bool bReplaceTab,
                   const std::function<void()>& rShowResults,
                   const css::code::search::FindOptions* pInitial)
{
    css::code::search::FindOptions aOptions;
    {
        FindDialog aDialog(pParent, bReplaceTab, pInitial);
        if (!aDialog.Run(aOptions))
            return;
    }

    if (rShowResults)
        rShowResults();
    try
    {
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
