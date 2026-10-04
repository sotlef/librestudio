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

void FindResultsPanel::EnsurePage()
{
    if (mxTable)
        return;

    mxTabs->set_tabs_at_bottom(true);
    mxTabs->set_tabs_closable(true);
    mxTabs->connect_tab_close(LINK(this, FindResultsPanel, TabCloseHdl));
    mxTabs->append_page(u"find"_ustr, u"Find"_ustr);
    mxPageBuilder = Application::CreateBuilder(mxTabs->get_page(u"find"_ustr),
                                               u"code/ui/findresultspage.ui"_ustr);
    // welding the root is what puts the page into its tab
    mxPageRoot = mxPageBuilder->weld_container(u"FindResultsPage"_ustr);
    mxScope = mxPageBuilder->weld_combo_box(u"scope"_ustr);
    mxSearch = mxPageBuilder->weld_combo_box(u"search"_ustr);
    mxToolbar = mxPageBuilder->weld_toolbar(u"toolbar"_ustr);
    mxTableBox = mxPageBuilder->weld_widget(u"tablebox"_ustr);
    mxTable = mxPageBuilder->weld_tree_view(u"results"_ustr);
    mxListBox = mxPageBuilder->weld_widget(u"listbox"_ustr);
    mxList = mxPageBuilder->weld_text_view(u"list"_ustr);
    mxSummary = mxPageBuilder->weld_label(u"summary"_ustr);

    mxScope->append_text(u"All Files"_ustr);
    mxScope->append_text(u"Current Document"_ustr);
    mxScope->append_text(u"Changed Documents"_ustr);
    mxScope->append_text(u"Open Documents"_ustr);
    mxScope->set_active(0);

    mxSearch->set_entry_placeholder_text(u"Search Find Results"_ustr);
    mxSearch->connect_changed(LINK(this, FindResultsPanel, SearchChangedHdl));
    mxToolbar->connect_clicked(LINK(this, FindResultsPanel, ToolbarClickHdl));

    // Fixed widths for File / Line / Col, Code takes the rest -- the table
    // renders at all on the generic-vcl path (qt6) only once the widths are
    // set from here, see ErrorListPanel.
    mxTable->connect_size_allocate(LINK(this, FindResultsPanel, ListSizeAllocHdl));

    mxListBox->set_visible(false);
    UpdateSummary();

}

IMPL_LINK_NOARG(FindResultsPanel, TabCloseHdl, const OUString&, void) { ClosePage(); }

void FindResultsPanel::ClosePage()
{
    if (!mxTable)
        return;
    maPump.Stop();
    mxEnumeration.clear();
    mbHasOptions = false;
    mxScope.reset();
    mxSearch.reset();
    mxToolbar.reset();
    mxTableBox.reset();
    mxTable.reset();
    mxListBox.reset();
    mxList.reset();
    mxSummary.reset();
    mxPageRoot.reset();
    mxPageBuilder.reset();
    mxTabs->remove_page(u"find"_ustr);
    maResults.clear();
    maShown.clear();
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
    if (!xShown.is() || xShown == mxEnumeration)
        return;

    EnsurePage();
    ClearResults();
    mxEnumeration = xShown;
    mbHasOptions = mxListener->Last(maOptions);
    if (mbHasOptions)
        mxTabs->set_tab_label_text(u"find"_ustr, "Find \"" + maOptions.FindWhat + "\"");
    maPump.Start();
}

void FindResultsPanel::ClearResults()
{
    maPump.Stop();
    maResults.clear();
    maShown.clear();
    mnFilesSearched = 0;
    mxTable->clear();
    mxList->set_text(OUString());
    UpdateSummary();
}

IMPL_LINK_NOARG(FindResultsPanel, PumpHdl, Timer*, void)
{
    if (Pump())
        maPump.Start();
}

bool FindResultsPanel::Pump()
{
    if (!mxEnumeration.is())
        return false;

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
            if (!mxEnumeration->hasMoreResults())
            {
                bMore = false;
                break;
            }
            const css::code::search::FindResult aFound = mxEnumeration->nextResult();
            Result aResult;
            aResult.msCode = aFound.LineText.trim();
            aResult.msFile = aFound.Url;
            osl::FileBase::getSystemPathFromFileURL(aFound.Url, aResult.msFile);
            aResult.mnLine = aFound.Match.Start.Line + 1;
            aResult.mnColumn = aFound.Match.Start.Column + 1;
            aBatch.push_back(std::move(aResult));
        }
        mnFilesSearched = mxEnumeration->getFilesSearched();
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "the search failed: " << rException.Message);
        bMore = false;
    }

    AddResults(std::move(aBatch));
    return bMore;
}

void FindResultsPanel::AppendRow(const Result& rResult)
{
    const int nRow = mxTable->n_children();
    mxTable->append();
    mxTable->set_text(nRow, rResult.msCode, COL_CODE);
    mxTable->set_text(nRow, rResult.msFile, COL_FILE);
    mxTable->set_text(nRow, OUString::number(rResult.mnLine), COL_LINE);
    mxTable->set_text(nRow, OUString::number(rResult.mnColumn), COL_COLUMN);
}

void FindResultsPanel::AddResults(std::vector<Result> aResults)
{
    if (!aResults.empty())
    {
        mxTable->freeze();
        for (Result& rResult : aResults)
        {
            maResults.push_back(std::move(rResult));
            if (IsShown(maResults.back()))
            {
                maShown.push_back(maResults.size() - 1);
                AppendRow(maResults.back());
            }
        }
        mxTable->thaw();
        UpdateListText();
    }
    UpdateSummary();
}

bool FindResultsPanel::IsShown(const Result& rResult) const
{
    const OUString aNeedle = mxSearch->get_active_text().toAsciiLowerCase();
    if (aNeedle.isEmpty())
        return true;
    return rResult.msCode.toAsciiLowerCase().indexOf(aNeedle) >= 0
           || rResult.msFile.toAsciiLowerCase().indexOf(aNeedle) >= 0;
}

void FindResultsPanel::UpdateTable()
{
    maShown.clear();
    for (size_t i = 0; i < maResults.size(); ++i)
        if (IsShown(maResults[i]))
            maShown.push_back(i);

    mxTable->freeze();
    mxTable->clear();
    for (size_t nIndex : maShown)
        AppendRow(maResults[nIndex]);
    mxTable->thaw();
}

void FindResultsPanel::UpdateListText()
{
    // "<full file name>(<line>): <code>", one line per row of the table
    OUStringBuffer aText;
    for (size_t nIndex : maShown)
    {
        const Result& rResult = maResults[nIndex];
        aText.append(rResult.msFile + "(" + OUString::number(rResult.mnLine) + "): " + rResult.msCode
                     + "\n");
    }
    mxList->set_text(aText.makeStringAndClear());
}

void FindResultsPanel::UpdateSummary()
{
    std::set<OUString> aFiles;
    for (size_t nIndex : maShown)
        aFiles.insert(maResults[nIndex].msFile);
    mxSummary->set_label("Matching lines: " + OUString::number(maShown.size())
                         + "    Matching files: " + OUString::number(aFiles.size())
                         + "    Total files searched: " + OUString::number(mnFilesSearched));
}

void FindResultsPanel::SelectRelative(int nDelta)
{
    const int nRows = mxTable->n_children();
    if (nRows == 0)
        return;
    const int nCurrent = mxTable->get_selected_index();
    const int nNew = nCurrent < 0 ? (nDelta > 0 ? 0 : nRows - 1)
                                  : std::clamp(nCurrent + nDelta, 0, nRows - 1);
    mxTable->select(nNew);
    mxTable->scroll_to_row(nNew);
}

IMPL_LINK(FindResultsPanel, ToolbarClickHdl, const OUString&, rIdent, void)
{
    if (rIdent == "copy")
    {
        // what is shown: the selected row, else every row, as the list view's lines
        OUStringBuffer aText;
        const int nSelected = mxTable->get_selected_index();
        for (size_t i = 0; i < maShown.size(); ++i)
        {
            if (nSelected >= 0 && static_cast<int>(i) != nSelected)
                continue;
            const Result& rResult = maResults[maShown[i]];
            aText.append(rResult.msFile + "(" + OUString::number(rResult.mnLine) + "): "
                         + rResult.msCode + "\n");
        }
        vcl::unohelper::TextDataObject::CopyStringTo(aText.makeStringAndClear(),
                                                      mxTable->get_clipboard());
    }
    else if (rIdent == "refresh")
    {
        // the same search again; theFinder tells the panel of it
        if (mbHasOptions)
            try
            {
                css::code::search::theFinder::get(comphelper::getProcessComponentContext())
                    ->startSearch(maOptions);
            }
            catch (const uno::Exception& rException)
            {
                SAL_WARN("code", "cannot search again: " << rException.Message);
            }
    }
    else if (rIdent == "prev")
        SelectRelative(-1);
    else if (rIdent == "next")
        SelectRelative(1);
    else if (rIdent == "clearfilters")
    {
        mxScope->set_active(0);
        mxSearch->set_entry_text(u""_ustr);
    }
    else if (rIdent == "listview")
    {
        const bool bList = mxToolbar->get_item_active(u"listview"_ustr);
        mxTableBox->set_visible(!bList);
        mxListBox->set_visible(bList);
    }
    // keep / modify: not there yet
}

IMPL_LINK_NOARG(FindResultsPanel, SearchChangedHdl, weld::ComboBox&, void)
{
    UpdateTable();
    UpdateListText();
    UpdateSummary();
}

IMPL_LINK(FindResultsPanel, ListSizeAllocHdl, const Size&, rSize, void)
{
    static constexpr int nFile = 220, nLine = 50, nColumn = 50;
    const int nCode = std::max(150, static_cast<int>(rSize.Width()) - nFile - nLine - nColumn);
    mxTable->set_column_fixed_widths({ nCode, nFile, nLine, nColumn });
}

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
