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
    /// takes up the search theFinder shows, if it is another one than the shown
    void Refresh();
    /// the tab is made when the first search comes, not before
    void EnsurePage();
    void ClosePage();
    /// reads results for a little while; false when the search is over
    bool Pump();
    void AddResults(std::vector<Result> aResults);
    void AppendRow(const Result& rResult);
    void ClearResults();

    void UpdateTable();
    void UpdateListText();
    void UpdateSummary();
    bool IsShown(const Result& rResult) const;
    void SelectRelative(int nDelta);

    DECL_LINK(ToolbarClickHdl, const OUString&, void);
    DECL_LINK(TabCloseHdl, const OUString&, void);
    DECL_LINK(PumpHdl, Timer*, void);
    DECL_LINK(SearchChangedHdl, weld::ComboBox&, void);
    DECL_LINK(ListSizeAllocHdl, const Size&, void);

    std::unique_ptr<weld::Notebook> mxTabs;
    std::unique_ptr<weld::Builder> mxPageBuilder;
    std::unique_ptr<weld::Container> mxPageRoot;
    std::unique_ptr<weld::ComboBox> mxScope;
    std::unique_ptr<weld::ComboBox> mxSearch;
    std::unique_ptr<weld::Toolbar> mxToolbar;
    std::unique_ptr<weld::Widget> mxTableBox;
    std::unique_ptr<weld::TreeView> mxTable;
    std::unique_ptr<weld::Widget> mxListBox;
    std::unique_ptr<weld::TextView> mxList;
    std::unique_ptr<weld::Label> mxSummary;

    std::vector<Result> maResults;
    // indexes into maResults of the rows shown (those matching the search box)
    std::vector<size_t> maShown;
    sal_Int32 mnFilesSearched = 0;

    css::uno::Reference<css::code::search::XFindEnumeration> mxEnumeration;
    css::code::search::FindOptions maOptions;
    bool mbHasOptions = false;
    Idle maPump;
    std::shared_ptr<::code::UiNotifier> mxNotifier;
    rtl::Reference<FindSearchListener> mxListener;
};

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
