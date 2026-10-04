/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "findresultspanel.hxx"

#include "../uibridge.hxx"

#include <com/sun/star/code/search/FindResult.hpp>
#include <com/sun/star/code/search/XFindListener.hpp>
#include <com/sun/star/code/search/theFinder.hpp>
#include <com/sun/star/container/NoSuchElementException.hpp>
#include <comphelper/processfactory.hxx>
#include <cppuhelper/implbase.hxx>
#include <osl/file.hxx>
#include "../../inc/codeids.hxx"
#include <sfx2/dispatch.hxx>
#include <svl/poolitem.hxx>
#include <svl/itemset.hxx>
#include <sfx2/frame.hxx>
#include <sfx2/viewfrm.hxx>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>
#include <tools/time.hxx>
#include <mutex>
#include <vcl/unohelp2.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/ComboBox.hxx>
#include <vcl/weld/TextView.hxx>
#include <vcl/weld/Toolbar.hxx>
#include <vcl/weld/TreeView.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/Notebook.hxx>
#include <vcl/weld/Label.hxx>
#include <vcl/weld/Widget.hxx>

#include <algorithm>
#include <set>

using namespace css;

namespace
{
// weld::TreeView column indexes, matching findresultspanel.ui
constexpr int COL_CODE = 0;
constexpr int COL_FILE = 1;
constexpr int COL_LINE = 2;
constexpr int COL_COLUMN = 3;
}

namespace code::sidebar
{

/// theFinder says a search was started; the panel, on the main thread, takes it up
class FindSearchListener final : public cppu::WeakImplHelper<css::code::search::XFindListener>
{
public:
    explicit FindSearchListener(std::shared_ptr<::code::UiNotifier> xNotifier)
        : m_xNotifier(std::move(xNotifier))
    {
    }

    void SAL_CALL searchStarted(const css::code::search::FindOptions& rOptions,
                                const uno::Reference<css::code::search::XFindEnumeration>&) override
    {
        {
            std::scoped_lock aGuard(m_aMutex);
            m_aLast = rOptions;
            m_bHasLast = true;
        }
        m_xNotifier->Notify();
    }
    void SAL_CALL disposing(const css::lang::EventObject&) override {}

    bool Last(css::code::search::FindOptions& rOptions)
    {
        std::scoped_lock aGuard(m_aMutex);
        rOptions = m_aLast;
        return m_bHasLast;
    }

private:
    std::shared_ptr<::code::UiNotifier> m_xNotifier;
    std::mutex m_aMutex;
    css::code::search::FindOptions m_aLast;
    bool m_bHasLast = false;
};

FindResultsPanel::FindResultsPanel(weld::Widget* pParent)
    : PanelLayout(pParent, u"FindResultsPanel"_ustr, u"code/ui/findresultspanel.ui"_ustr)
    , mxTabs(m_xBuilder->weld_notebook(u"FindResultsPanel"_ustr))
    , maPump("code::FindResultsPanel pump")
{
    maPump.SetPriority(TaskPriority::LOWEST);
    maPump.SetInvokeHandler(LINK(this, FindResultsPanel, PumpHdl));

    mxNotifier = std::make_shared<::code::UiNotifier>([this] { Refresh(); });
    mxListener = new FindSearchListener(mxNotifier);
    try
    {
        css::code::search::theFinder::get(comphelper::getProcessComponentContext())
            ->addFindListener(mxListener);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "Find in Files cannot reach the finder: " << rException.Message);
    }
    // the search may have been started before this panel was made
    Refresh();
}

FindResultsPanel::~FindResultsPanel()
{
    maPump.Stop();
    mxNotifier->Detach();
    try
    {
        css::code::search::theFinder::get(comphelper::getProcessComponentContext())
            ->removeFindListener(mxListener);
    }
    catch (const uno::Exception&)
    {
    }
}

FindResultsPanel::Tab& FindResultsPanel::AddTab()
{
    if (maTabs.empty())
    {
        mxTabs->set_tabs_at_bottom(true);
        mxTabs->set_tabs_closable(true);
        mxTabs->connect_tab_close(LINK(this, FindResultsPanel, TabCloseHdl));
    }

    auto xTab = std::make_unique<Tab>();
    Tab& t = *xTab;
    t.msIdent = "find" + OUString::number(++mnTabCounter);
    mxTabs->append_page(t.msIdent, u"Find"_ustr);
    t.mxPageBuilder = Application::CreateBuilder(mxTabs->get_page(t.msIdent),
                                                 u"code/ui/findresultspage.ui"_ustr);
    // welding the root is what puts the page into its tab
    t.mxPageRoot = t.mxPageBuilder->weld_container(u"FindResultsPage"_ustr);
    t.mxScope = t.mxPageBuilder->weld_combo_box(u"scope"_ustr);
    t.mxSearch = t.mxPageBuilder->weld_combo_box(u"search"_ustr);
    t.mxToolbar = t.mxPageBuilder->weld_toolbar(u"toolbar"_ustr);
    t.mxTableBox = t.mxPageBuilder->weld_widget(u"tablebox"_ustr);
    t.mxTable = t.mxPageBuilder->weld_tree_view(u"results"_ustr);
    t.mxListBox = t.mxPageBuilder->weld_widget(u"listbox"_ustr);
    t.mxList = t.mxPageBuilder->weld_text_view(u"list"_ustr);
    t.mxDescription = t.mxPageBuilder->weld_label(u"description"_ustr);
    t.mxSummary = t.mxPageBuilder->weld_label(u"summary"_ustr);

    t.mxScope->append_text(u"All Files"_ustr);
    t.mxScope->append_text(u"Current Document"_ustr);
    t.mxScope->append_text(u"Changed Documents"_ustr);
    t.mxScope->append_text(u"Open Documents"_ustr);
    t.mxScope->set_active(0);

    t.mxSearch->set_entry_placeholder_text(u"Search Find Results"_ustr);
    t.mxSearch->connect_changed(LINK(this, FindResultsPanel, SearchChangedHdl));
    t.mxToolbar->connect_clicked(LINK(this, FindResultsPanel, ToolbarClickHdl));

    // Fixed widths for File / Line / Col, Code takes the rest -- the table
    // renders at all on the generic-vcl path (qt6) only once the widths are
    // set from here, see ErrorListPanel.
    t.mxTable->connect_size_allocate(LINK(this, FindResultsPanel, ListSizeAllocHdl));

    t.mxListBox->set_visible(false);
    UpdateSummary(t);

    maTabs.push_back(std::move(xTab));
    return t;
}

IMPL_LINK(FindResultsPanel, TabCloseHdl, const OUString&, rIdent, void) { CloseTab(rIdent); }

FindResultsPanel::Tab* FindResultsPanel::FindTab(const OUString& rIdent) const
{
    for (const auto& rTab : maTabs)
        if (rTab->msIdent == rIdent)
            return rTab.get();
    return nullptr;
}

FindResultsPanel::Tab* FindResultsPanel::CurrentTab() const
{
    return FindTab(mxTabs->get_current_page_ident());
}

void FindResultsPanel::CloseTab(const OUString& rIdent)
{
    auto it = std::find_if(maTabs.begin(), maTabs.end(),
                           [&](const auto& rTab) { return rTab->msIdent == rIdent; });
    if (it == maTabs.end())
        return;
    if (mpRefreshTab == it->get())
        mpRefreshTab = nullptr;
    // the widgets go before the page they stand in
    std::unique_ptr<Tab> xTab = std::move(*it);
    maTabs.erase(it);
    xTab.reset();
    mxTabs->remove_page(rIdent);
}

FindResultsPanel::Tab& FindResultsPanel::FreeTab()
{
    // a search that was asked for by a tab's Refresh stays in it
    if (mpRefreshTab)
    {
        Tab* pTab = mpRefreshTab;
        mpRefreshTab = nullptr;
        return *pTab;
    }
    // the first tab whose Keep Results is not pressed
    for (const auto& rTab : maTabs)
        if (!rTab->mxToolbar->get_item_active(u"keep"_ustr))
            return *rTab;
    return AddTab();
}

void FindResultsPanel::Refresh()
{
    uno::Reference<css::code::search::XFindEnumeration> xShown;
    try
    {
        xShown = css::code::search::theFinder::get(comphelper::getProcessComponentContext())
                     ->getShownSearch();
    }
    catch (const uno::Exception&)
    {
    }
    if (!xShown.is())
        return;
    for (const auto& rTab : maTabs)
        if (rTab->mxEnumeration == xShown)
            return;

    Tab& t = FreeTab();
    ClearResults(t);
    t.mxEnumeration = xShown;
    t.mbHasOptions = mxListener->Last(t.maOptions);
    if (t.mbHasOptions)
        mxTabs->set_tab_label_text(t.msIdent, "Find \"" + t.maOptions.FindWhat + "\"");
    t.mxDescription->set_label(t.mbHasOptions ? Describe(t.maOptions) : OUString());
    t.mxDescription->set_tooltip_text(t.mxDescription->get_label());
    mxTabs->set_current_page(t.msIdent);
    maPump.Start();
}

void FindResultsPanel::ClearResults(Tab& t)
{
    t.mxEnumeration.clear();
    t.maResults.clear();
    t.maShown.clear();
    t.mnFilesSearched = 0;
    t.mxTable->clear();
    t.mxList->set_text(OUString());
    UpdateSummary(t);
}

IMPL_LINK_NOARG(FindResultsPanel, PumpHdl, Timer*, void)
{
    if (Pump())
        maPump.Start();
}

bool FindResultsPanel::Pump()
{
    bool bAnyMore = false;
    for (const auto& rTab : maTabs)
        if (rTab->mxEnumeration.is() && PumpTab(*rTab))
            bAnyMore = true;
    return bAnyMore;
}

bool FindResultsPanel::PumpTab(Tab& t)
{
    // As many results as fit into a few milliseconds: the window stays alive
    // while a search of a big solution goes on, and the first results are
    // there at once.
    constexpr sal_uInt64 nBudgetMs = 30;
    const sal_uInt64 nStart = tools::Time::GetSystemTicks();
    std::vector<Result> aBatch;
    bool bMore = true;
    try
    {
        while (tools::Time::GetSystemTicks() - nStart < nBudgetMs)
        {
            if (!t.mxEnumeration->hasMoreResults())
            {
                bMore = false;
                break;
            }
            const css::code::search::FindResult aFound = t.mxEnumeration->nextResult();
            Result aResult;
            aResult.msCode = aFound.LineText.trim();
            aResult.msFile = aFound.Url;
            osl::FileBase::getSystemPathFromFileURL(aFound.Url, aResult.msFile);
            aResult.mnLine = aFound.Match.Start.Line + 1;
            aResult.mnColumn = aFound.Match.Start.Column + 1;
            aBatch.push_back(std::move(aResult));
        }
        t.mnFilesSearched = t.mxEnumeration->getFilesSearched();
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "the search failed: " << rException.Message);
        bMore = false;
    }

    AddResults(t, std::move(aBatch));
    return bMore;
}

void FindResultsPanel::AppendRow(Tab& t, const Result& rResult)
{
    const int nRow = t.mxTable->n_children();
    t.mxTable->append();
    t.mxTable->set_text(nRow, rResult.msCode, COL_CODE);
    t.mxTable->set_text(nRow, rResult.msFile, COL_FILE);
    t.mxTable->set_text(nRow, OUString::number(rResult.mnLine), COL_LINE);
    t.mxTable->set_text(nRow, OUString::number(rResult.mnColumn), COL_COLUMN);
}

void FindResultsPanel::AddResults(Tab& t, std::vector<Result> aResults)
{
    if (!aResults.empty())
    {
        t.mxTable->freeze();
        for (Result& rResult : aResults)
        {
            t.maResults.push_back(std::move(rResult));
            if (IsShown(t, t.maResults.back()))
            {
                t.maShown.push_back(t.maResults.size() - 1);
                AppendRow(t, t.maResults.back());
            }
        }
        t.mxTable->thaw();
        UpdateListText(t);
    }
    UpdateSummary(t);
}

bool FindResultsPanel::IsShown(const Tab& t, const Result& rResult) const
{
    const OUString aNeedle = t.mxSearch->get_active_text().toAsciiLowerCase();
    if (aNeedle.isEmpty())
        return true;
    return rResult.msCode.toAsciiLowerCase().indexOf(aNeedle) >= 0
           || rResult.msFile.toAsciiLowerCase().indexOf(aNeedle) >= 0;
}

void FindResultsPanel::UpdateTable(Tab& t)
{
    t.maShown.clear();
    for (size_t i = 0; i < t.maResults.size(); ++i)
        if (IsShown(t, t.maResults[i]))
            t.maShown.push_back(i);

    t.mxTable->freeze();
    t.mxTable->clear();
    for (size_t nIndex : t.maShown)
        AppendRow(t, t.maResults[nIndex]);
    t.mxTable->thaw();
}

void FindResultsPanel::UpdateListText(Tab& t)
{
    // "<full file name>(<line>): <code>", one line per row of the table
    OUStringBuffer aText;
    for (size_t nIndex : t.maShown)
    {
        const Result& rResult = t.maResults[nIndex];
        aText.append(rResult.msFile + "(" + OUString::number(rResult.mnLine) + "): " + rResult.msCode
                     + "\n");
    }
    t.mxList->set_text(aText.makeStringAndClear());
}

OUString FindResultsPanel::Describe(const css::code::search::FindOptions& rOptions)
{
    using namespace css::code::search;
    OUString aText = "Find all \"" + rOptions.FindWhat + "\"";
    if (rOptions.MatchCase)
        aText += ", Match case";
    if (rOptions.MatchWholeWord)
        aText += ", Match whole word";
    if (rOptions.UseRegex)
        aText += ", Use regular expressions";
    switch (rOptions.Scope)
    {
        case FindScope_ENTIRE_SOLUTION:
            aText += ", Entire solution";
            break;
        case FindScope_CURRENT_PROJECT:
            aText += ", Current project";
            break;
        case FindScope_OPEN_DOCUMENTS:
            aText += ", All open documents";
            break;
        case FindScope_CURRENT_DOCUMENT:
            aText += ", Current document";
            break;
        default:
        {
            OUString aPath;
            osl::FileBase::getSystemPathFromFileURL(rOptions.Directory, aPath);
            aText += ", \"" + aPath + "\"";
            if (rOptions.IncludeSubfolders)
                aText += ", Include subfolders";
            break;
        }
    }
    if (!rOptions.FileTypes.isEmpty() && rOptions.FileTypes != "*.*")
        aText += ", \"" + rOptions.FileTypes + "\"";
    return aText;
}

void FindResultsPanel::UpdateSummary(Tab& t)
{
    std::set<OUString> aFiles;
    for (size_t nIndex : t.maShown)
        aFiles.insert(t.maResults[nIndex].msFile);
    t.mxSummary->set_label("Matching lines: " + OUString::number(t.maShown.size())
                         + "    Matching files: " + OUString::number(aFiles.size())
                         + "    Total files searched: " + OUString::number(t.mnFilesSearched));
}

void FindResultsPanel::SelectRelative(Tab& t, int nDelta)
{
    const int nRows = t.mxTable->n_children();
    if (nRows == 0)
        return;
    const int nCurrent = t.mxTable->get_selected_index();
    const int nNew = nCurrent < 0 ? (nDelta > 0 ? 0 : nRows - 1)
                                  : std::clamp(nCurrent + nDelta, 0, nRows - 1);
    t.mxTable->select(nNew);
    t.mxTable->scroll_to_row(nNew);
}

IMPL_LINK(FindResultsPanel, ToolbarClickHdl, const OUString&, rIdent, void)
{
    // the buttons of a tab can only be pressed while it is the current one
    Tab* pTab = CurrentTab();
    if (!pTab)
        return;
    Tab& t = *pTab;
    if (rIdent == "copy")
    {
        // what is shown: the selected row, else every row, as the list view's lines
        OUStringBuffer aText;
        const int nSelected = t.mxTable->get_selected_index();
        for (size_t i = 0; i < t.maShown.size(); ++i)
        {
            if (nSelected >= 0 && static_cast<int>(i) != nSelected)
                continue;
            const Result& rResult = t.maResults[t.maShown[i]];
            aText.append(rResult.msFile + "(" + OUString::number(rResult.mnLine) + "): "
                         + rResult.msCode + "\n");
        }
        vcl::unohelper::TextDataObject::CopyStringTo(aText.makeStringAndClear(),
                                                      t.mxTable->get_clipboard());
    }
    else if (rIdent == "refresh")
    {
        // the same search again; theFinder tells the panel of it
        if (t.mbHasOptions)
            try
            {
                mpRefreshTab = &t;
                css::code::search::theFinder::get(comphelper::getProcessComponentContext())
                    ->startSearch(t.maOptions);
            }
            catch (const uno::Exception& rException)
            {
                mpRefreshTab = nullptr;
                SAL_WARN("code", "cannot search again: " << rException.Message);
            }
    }
    else if (rIdent == "prev")
        SelectRelative(t, -1);
    else if (rIdent == "next")
        SelectRelative(t, 1);
    else if (rIdent == "clearfilters")
    {
        t.mxScope->set_active(0);
        t.mxSearch->set_entry_text(u""_ustr);
    }
    else if (rIdent == "listview")
    {
        const bool bList = t.mxToolbar->get_item_active(u"listview"_ustr);
        t.mxTableBox->set_visible(!bList);
        t.mxListBox->set_visible(bList);
    }
    else if (rIdent == "modify")
    {
        // the dialog of Find All as it was left; the options are the tab's own
        if (t.mbHasOptions)
        {
            // the dialog is run by the main window's shell (the options go as an Any)
            if (SfxViewFrame* pFrame = SfxViewFrame::Current())
            {
                const SfxUnoAnyItem aOptions(SID_CODE_ARG_FINDOPTIONS, uno::Any(t.maOptions));
                pFrame->GetDispatcher()->ExecuteList(SID_CODE_MODIFYFIND,
                                                     SfxCallMode::ASYNCHRON, { &aOptions });
            }
        }
    }
}

IMPL_LINK_NOARG(FindResultsPanel, SearchChangedHdl, weld::ComboBox&, void)
{
    Tab* pTab = CurrentTab();
    if (!pTab)
        return;
    Tab& t = *pTab;
    UpdateTable(t);
    UpdateListText(t);
    UpdateSummary(t);
}

IMPL_LINK(FindResultsPanel, ListSizeAllocHdl, const Size&, rSize, void)
{
    static constexpr int nFile = 220, nLine = 50, nColumn = 50;
    const int nCode = std::max(150, static_cast<int>(rSize.Width()) - nFile - nLine - nColumn);
    for (const auto& rTab : maTabs)
        rTab->mxTable->set_column_fixed_widths({ nCode, nFile, nLine, nColumn });
}

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
