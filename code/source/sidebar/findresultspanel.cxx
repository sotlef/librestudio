/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "findresultspanel.hxx"

#include <rtl/ustrbuf.hxx>
#include <vcl/unohelp2.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/ComboBox.hxx>
#include <vcl/weld/TextView.hxx>
#include <vcl/weld/Toolbar.hxx>
#include <vcl/weld/TreeView.hxx>
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

FindResultsPanel::FindResultsPanel(weld::Widget* pParent)
    : PanelLayout(pParent, u"FindResultsPanel"_ustr, u"code/ui/findresultspanel.ui"_ustr)
    , mxScope(m_xBuilder->weld_combo_box(u"scope"_ustr))
    , mxSearch(m_xBuilder->weld_combo_box(u"search"_ustr))
    , mxToolbar(m_xBuilder->weld_toolbar(u"toolbar"_ustr))
    , mxTableBox(m_xBuilder->weld_widget(u"tablebox"_ustr))
    , mxTable(m_xBuilder->weld_tree_view(u"results"_ustr))
    , mxListBox(m_xBuilder->weld_widget(u"listbox"_ustr))
    , mxList(m_xBuilder->weld_text_view(u"list"_ustr))
    , mxSummary(m_xBuilder->weld_label(u"summary"_ustr))
{
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

FindResultsPanel::~FindResultsPanel() = default;

void FindResultsPanel::SetResults(std::vector<Result> aResults, sal_Int32 nFilesSearched)
{
    maResults = std::move(aResults);
    mnFilesSearched = nFilesSearched;
    UpdateTable();
    UpdateListText();
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
    for (size_t row = 0; row < maShown.size(); ++row)
    {
        const Result& rResult = maResults[maShown[row]];
        mxTable->append();
        mxTable->set_text(row, rResult.msCode, COL_CODE);
        mxTable->set_text(row, rResult.msFile, COL_FILE);
        mxTable->set_text(row, OUString::number(rResult.mnLine), COL_LINE);
        mxTable->set_text(row, OUString::number(rResult.mnColumn), COL_COLUMN);
    }
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
    // refresh / keep / modify: need the search itself
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
