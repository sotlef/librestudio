/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sfx2/sidebar/PanelLayout.hxx>

#include <memory>
#include <vector>

namespace weld { class ComboBox; class TextView; class Toolbar; class TreeView; class Widget; class Label; }

namespace code::sidebar
{

// The "Find in Files" tab of the bottom dock, like Visual Studio's Find
// Results window: a toolbar strip (scope selector, copy, previous / next
// location, clear filters, refresh, keep results, list view, modify find)
// above the result table (Code / File / Line / Col) -- or, with List View
// pressed, a monospaced text of "<full file name>(<line>): <code>" lines --
// and a summary line below.  Only the window is here, the search itself is
// not: the table stays empty until something fills it with SetResults().
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

    void SetResults(std::vector<Result> aResults, sal_Int32 nFilesSearched);

private:
    void UpdateTable();
    void UpdateListText();
    void UpdateSummary();
    bool IsShown(const Result& rResult) const;
    void SelectRelative(int nDelta);

    DECL_LINK(ToolbarClickHdl, const OUString&, void);
    DECL_LINK(SearchChangedHdl, weld::ComboBox&, void);
    DECL_LINK(ListSizeAllocHdl, const Size&, void);

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
};

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
