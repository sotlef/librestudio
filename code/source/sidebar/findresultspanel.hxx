/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/code/search/FindOptions.hpp>
#include <com/sun/star/code/search/XFindEnumeration.hpp>
#include <rtl/ref.hxx>
#include <sfx2/sidebar/PanelLayout.hxx>
#include <vcl/idle.hxx>
#include <vcl/weld/Container.hxx>
#include <vcl/weld/Notebook.hxx>

#include <memory>
#include <vector>

namespace weld { class ComboBox; class TextView; class Toolbar; class TreeView; class Widget; class Label; }

namespace code { class UiNotifier; }

namespace code::sidebar
{

class FindSearchListener;

// The "Find in Files" tab of the bottom dock, like Visual Studio's Find
// Results window: a toolbar strip (scope selector, copy, previous / next
// location, clear filters, refresh, keep results, list view, modify find)
// above the result table (Code / File / Line / Col) -- or, with List View
// pressed, a monospaced text of "<full file name>(<line>): <code>" lines --
// and a summary line below.  Only the window is here, the search itself is
// not here: it is the enumeration of theFinder (codeapi's search service) that
// the panel takes its results from, a few at a time while the main loop is
// idle, so a search of a big solution shows its first results at once and the
// window stays alive.  A search is shown when theFinder::startSearch() is called.
class FindResultsPanel final : public PanelLayout
{
public:
    struct Result
    {
        OUString msCode;
        OUString msFile; // the full absolute name
        sal_Int32 mnLine = 0;
        sal_Int32 mnColumn = 0;
    };

    explicit FindResultsPanel(weld::Widget* pParent);
    virtual ~FindResultsPanel() override;

private:
    /// One tab of the window: a search and everything that shows it.
    struct Tab
    {
        OUString msIdent;
        std::unique_ptr<weld::Builder> mxPageBuilder;
        std::unique_ptr<weld::Container> mxPageRoot;
        std::unique_ptr<weld::ComboBox> mxScope;
        std::unique_ptr<weld::ComboBox> mxSearch;
        std::unique_ptr<weld::Toolbar> mxToolbar;
        std::unique_ptr<weld::Widget> mxTableBox;
        std::unique_ptr<weld::TreeView> mxTable;
        std::unique_ptr<weld::Widget> mxListBox;
        std::unique_ptr<weld::TextView> mxList;
        std::unique_ptr<weld::Label> mxDescription;
        std::unique_ptr<weld::Label> mxSummary;

        std::vector<Result> maResults;
        // indexes into maResults of the rows shown (those matching the search box)
        std::vector<size_t> maShown;
        sal_Int32 mnFilesSearched = 0;

        css::uno::Reference<css::code::search::XFindEnumeration> mxEnumeration;
        /// what the search was started with: Modify Find and Refresh need it
        css::code::search::FindOptions maOptions;
        bool mbHasOptions = false;
    };

    /// takes up the search theFinder shows, if it is another one than the shown
    void Refresh();
    /// a tab for the next search: the first one whose Keep Results is not pressed, or a new one
    Tab& FreeTab();
    Tab& AddTab();
    void CloseTab(const OUString& rIdent);
    Tab* FindTab(const OUString& rIdent) const;
    Tab* CurrentTab() const;
    /// reads results for a little while; false when no search is going on
    bool Pump();
    bool PumpTab(Tab& rTab);
    void AddResults(Tab& rTab, std::vector<Result> aResults);
    void AppendRow(Tab& rTab, const Result& rResult);
    void ClearResults(Tab& rTab);

    void UpdateTable(Tab& rTab);
    void UpdateListText(Tab& rTab);
    void UpdateSummary(Tab& rTab);
    /// "Find all "x", Match case, All open documents, "*.cpp""
    static OUString Describe(const css::code::search::FindOptions& rOptions);
    bool IsShown(const Tab& rTab, const Result& rResult) const;
    void SelectRelative(Tab& rTab, int nDelta);

    DECL_LINK(ToolbarClickHdl, const OUString&, void);
    DECL_LINK(TabCloseHdl, const OUString&, void);
    DECL_LINK(PumpHdl, Timer*, void);
    DECL_LINK(SearchChangedHdl, weld::ComboBox&, void);
    DECL_LINK(ListSizeAllocHdl, const Size&, void);

    std::unique_ptr<weld::Notebook> mxTabs;
    std::vector<std::unique_ptr<Tab>> maTabs;
    int mnTabCounter = 0;
    /// the tab a Refresh asked its search for: the next search belongs to it
    Tab* mpRefreshTab = nullptr;

    Idle maPump;
    std::shared_ptr<::code::UiNotifier> mxNotifier;
    rtl::Reference<FindSearchListener> mxListener;
};

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
