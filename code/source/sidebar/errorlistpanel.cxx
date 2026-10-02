/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "errorlistpanel.hxx"

#include <algorithm>

#include <vcl/weld/Builder.hxx>
#include <vcl/weld/Button.hxx>
#include <vcl/weld/ComboBox.hxx>
#include <vcl/weld/ToggleButton.hxx>
#include <vcl/weld/TreeView.hxx>

namespace
{
// weld::TreeView column indexes, matching errorlistpanel.ui's renderer
// order: blank / severity icon / Code / Description / Project / File /
// Line / Details / blank / blank
constexpr int COL_SEVERITY = 1;
constexpr int COL_CODE = 2;
constexpr int COL_DESCRIPTION = 3;
constexpr int COL_PROJECT = 4;
constexpr int COL_FILE = 5;
constexpr int COL_LINE = 6;
}

namespace code::sidebar
{

ErrorListPanel::ErrorListPanel(weld::Widget* pParent)
    : PanelLayout(pParent, u"ErrorListPanel"_ustr, u"code/ui/errorlistpanel.ui"_ustr)
    , mxScope(m_xBuilder->weld_combo_box(u"scope"_ustr))
    , mxErrors(m_xBuilder->weld_toggle_button(u"errors"_ustr))
    , mxWarnings(m_xBuilder->weld_toggle_button(u"warnings"_ustr))
    , mxMessages(m_xBuilder->weld_toggle_button(u"messages"_ustr))
    , mxClearFilters(m_xBuilder->weld_button(u"clearfilters"_ustr))
    , mxProvider(m_xBuilder->weld_combo_box(u"provider"_ustr))
    , mxSearch(m_xBuilder->weld_combo_box(u"search"_ustr))
    , mxList(m_xBuilder->weld_tree_view(u"errorlist"_ustr))
{
    // The scope filter needs a real solution/project/document model to
    // mean anything; until one exists the selector is just selectable --
    // an honest no-op, like Output's "Show output from:" scopes.
    mxScope->append_text(u"Entire Solution"_ustr);
    mxScope->append_text(u"Current Project"_ustr);
    mxScope->append_text(u"Current Document"_ustr);
    mxScope->append_text(u"Changed Documents"_ustr);
    mxScope->append_text(u"Open Documents"_ustr);
    mxScope->set_active(0);

    mxProvider->append_text(u"Build + IntelliSense"_ustr);
    mxProvider->append_text(u"Build Only"_ustr);
    mxProvider->append_text(u"IntelliSense Only"_ustr);
    mxProvider->set_active(0);
    mxProvider->connect_changed(LINK(this, ErrorListPanel, FilterChangedHdl));

    mxSearch->set_entry_placeholder_text(u"Search Error List"_ustr);
    mxSearch->connect_changed(LINK(this, ErrorListPanel, FilterChangedHdl));

    mxErrors->connect_toggled(LINK(this, ErrorListPanel, FilterToggledHdl));
    mxWarnings->connect_toggled(LINK(this, ErrorListPanel, FilterToggledHdl));
    mxMessages->connect_toggled(LINK(this, ErrorListPanel, FilterToggledHdl));
    mxClearFilters->connect_clicked(LINK(this, ErrorListPanel, ClearFiltersHdl));

    // Column widths are distributed from C++ on every resize (Description
    // absorbs the remaining width, VS-style) -- this is also what makes the
    // table render at all on the generic-vcl path (qt6): VclBuilder inserts
    // header items with a canned width of 100 and never calls
    // SvTabListBox::SetTabs(), and without tab positions the rows paint
    // empty until something (a header drag, or this
    // set_column_fixed_widths()) computes them.
    mxList->connect_size_allocate(LINK(this, ErrorListPanel, ListSizeAllocHdl));

    // Canned demo rows, same honesty rules as OutputLogPanel's fake build
    // log: no real build/IntelliSense sink exists yet, but everything
    // downstream of this vector -- the severity/provider/search filtering
    // and the live counts on the toggle buttons -- is real.
    maEntries = {
        { Severity::Error, true, u"E0020"_ustr,
          u"identifier \"undeclared_var\" is undefined"_ustr, u"DemoProject"_ustr,
          u"main.cpp"_ustr, u"31"_ustr },
        { Severity::Warning, false, u"C4101"_ustr,
          u"'tmp': unreferenced local variable"_ustr, u"DemoProject"_ustr, u"utils.h"_ustr,
          u"12"_ustr },
        { Severity::Warning, false, u"C4244"_ustr,
          u"'initializing': conversion from 'double' to 'int', possible loss of data"_ustr,
          u"DemoProject"_ustr, u"main.cpp"_ustr, u"24"_ustr },
        { Severity::Message, true, u""_ustr,
          u"The variable 'result' is assigned but its value is never used"_ustr,
          u"DemoProject"_ustr, u"main.cpp"_ustr, u"18"_ustr },
    };

    UpdateFilterButtonLabels();
    UpdateList();
}

ErrorListPanel::~ErrorListPanel()
{
}

void ErrorListPanel::UpdateFilterButtonLabels()
{
    // Totals over the whole model, not the filtered view -- VS keeps the
    // real counts on the buttons even while a filter hides the rows.
    int nErrors = 0, nWarnings = 0, nMessages = 0;
    for (const auto& rEntry : maEntries)
    {
        switch (rEntry.meSeverity)
        {
            case Severity::Error: ++nErrors; break;
            case Severity::Warning: ++nWarnings; break;
            case Severity::Message: ++nMessages; break;
        }
    }
    mxErrors->set_label(OUString::number(nErrors)
                        + (nErrors == 1 ? u" Error"_ustr : u" Errors"_ustr));
    mxWarnings->set_label(OUString::number(nWarnings)
                          + (nWarnings == 1 ? u" Warning"_ustr : u" Warnings"_ustr));
    mxMessages->set_label(OUString::number(nMessages)
                          + (nMessages == 1 ? u" Message"_ustr : u" Messages"_ustr));
}

bool ErrorListPanel::IsShown(const ErrorEntry& rEntry, const OUString& rNeedle) const
{
    switch (rEntry.meSeverity)
    {
        case Severity::Error:
            if (!mxErrors->get_active())
                return false;
            break;
        case Severity::Warning:
            if (!mxWarnings->get_active())
                return false;
            break;
        case Severity::Message:
            if (!mxMessages->get_active())
                return false;
            break;
    }

    const int nProvider = mxProvider->get_active();
    if (nProvider == 1 && rEntry.mbIntelliSense) // Build Only
        return false;
    if (nProvider == 2 && !rEntry.mbIntelliSense) // IntelliSense Only
        return false;

    if (!rNeedle.isEmpty()
        && rEntry.msDescription.toAsciiLowerCase().indexOf(rNeedle) == -1
        && rEntry.msCode.toAsciiLowerCase().indexOf(rNeedle) == -1
        && rEntry.msProject.toAsciiLowerCase().indexOf(rNeedle) == -1
        && rEntry.msFile.toAsciiLowerCase().indexOf(rNeedle) == -1)
        return false;

    return true;
}

void ErrorListPanel::UpdateList()
{
    const OUString aNeedle = mxSearch->get_active_text().toAsciiLowerCase();

    mxList->freeze();
    mxList->clear();
    int nRow = 0;
    for (const auto& rEntry : maEntries)
    {
        if (!IsShown(rEntry, aNeedle))
            continue;

        mxList->append();
        OUString sIcon;
        switch (rEntry.meSeverity)
        {
            case Severity::Error: sIcon = u"cmd/sc_errorcellstyles.png"_ustr; break;
            case Severity::Warning: sIcon = u"cmd/sc_warningcellstyles.png"_ustr; break;
            case Severity::Message: sIcon = u"svtools/res/info_small.png"_ustr; break;
        }
        mxList->set_image(nRow, sIcon, COL_SEVERITY);
        mxList->set_text(nRow, rEntry.msCode, COL_CODE);
        mxList->set_text(nRow, rEntry.msDescription, COL_DESCRIPTION);
        mxList->set_text(nRow, rEntry.msProject, COL_PROJECT);
        mxList->set_text(nRow, rEntry.msFile, COL_FILE);
        mxList->set_text(nRow, rEntry.msLine, COL_LINE);
        ++nRow;
    }
    mxList->thaw();
}

IMPL_LINK(ErrorListPanel, ListSizeAllocHdl, const Size&, rSize, void)
{
    // Fixed, content-sized widths for everything but Description, which
    // takes whatever remains of the view -- so the trailing columns end up
    // at the panel's right edge, like Visual Studio.
    static constexpr int nBlank1 = 24, nSeverity = 24, nCode = 70, nProject = 110, nFile = 110,
                         nLine = 50, nDetails = 70, nBlank2 = 24, nBlank3 = 24;
    const int nFixed
        = nBlank1 + nSeverity + nCode + nProject + nFile + nLine + nDetails + nBlank2 + nBlank3;
    const int nDescription = std::max(150, static_cast<int>(rSize.Width()) - nFixed);
    mxList->set_column_fixed_widths({ nBlank1, nSeverity, nCode, nDescription, nProject, nFile,
                                      nLine, nDetails, nBlank2, nBlank3 });
}

IMPL_LINK_NOARG(ErrorListPanel, FilterToggledHdl, weld::Toggleable&, void)
{
    UpdateList();
}

IMPL_LINK_NOARG(ErrorListPanel, FilterChangedHdl, weld::ComboBox&, void)
{
    UpdateList();
}

IMPL_LINK_NOARG(ErrorListPanel, ClearFiltersHdl, weld::Button&, void)
{
    mxErrors->set_active(true);
    mxWarnings->set_active(true);
    mxMessages->set_active(true);
    mxScope->set_active(0);
    mxProvider->set_active(0);
    mxSearch->set_entry_text(u""_ustr);
    UpdateList();
}

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
