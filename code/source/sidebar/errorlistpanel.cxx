/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "errorlistpanel.hxx"

#include "../uibridge.hxx"

#include <com/sun/star/code/core/Severity.hpp>
#include <com/sun/star/code/document/DocumentState.hpp>
#include <com/sun/star/code/document/XDocument.hpp>
#include <com/sun/star/code/document/XDocumentManager.hpp>
#include <com/sun/star/code/document/theDocumentManager.hpp>
#include <com/sun/star/code/project/XProject.hpp>
#include <com/sun/star/code/project/XProjectManager.hpp>
#include <com/sun/star/code/project/theProjectManager.hpp>
#include <com/sun/star/code/task/XTaskHub.hpp>
#include <com/sun/star/code/task/theTaskHub.hpp>
#include <comphelper/processfactory.hxx>
#include <sal/log.hxx>
#include <tools/urlobj.hxx>

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

using namespace css;

namespace code::sidebar
{

namespace
{
// Where a task came from, as the "provider" filter sees it: what the build
// printed, or what an analysis of the open files produced.
bool IsBuildTask(const css::code::task::Task& rTask) { return rTask.Category == "build"; }

// The file's name for the File column; the whole URL is what the row opens.
OUString FileNameOf(const OUString& rUrl)
{
    return rUrl.isEmpty() ? OUString() : INetURLObject(rUrl).getName(INetURLObject::LAST_SEGMENT, true,
                                                                     INetURLObject::DecodeMechanism::WithCharset);
}

// "/home/me/proj/src/a.cpp" -> "proj" is not recoverable from a Task, which
// only carries the project's URL; its last segment is what is shown.
OUString ProjectNameOf(const OUString& rUrl)
{
    return rUrl.isEmpty() ? OUString() : INetURLObject(rUrl).getName(INetURLObject::LAST_SEGMENT, true,
                                                                     INetURLObject::DecodeMechanism::WithCharset);
}
}

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
    // Scope: which tasks belong to what the user is looking at.  "Entire
    // Solution" is every task; the narrower ones filter by the task's file or
    // project (see IsShown()).
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

    mxList->connect_item_activated(LINK(this, ErrorListPanel, RowActivatedHdl));

    // Tasks are added by the output filters and diagnostics on whatever
    // thread produced them; the table is only touched here, on the main
    // thread, once per burst.
    mxNotifier = std::make_shared<::code::UiNotifier>([this] { Refresh(); });
    mxListener = new ::code::ServiceListener(mxNotifier);
    try
    {
        css::code::task::theTaskHub::get(comphelper::getProcessComponentContext())
            ->addTaskListener(mxListener);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "Error List cannot reach the task hub: " << rException.Message);
    }

    Refresh();
}

ErrorListPanel::~ErrorListPanel()
{
    mxNotifier->Detach();
    try
    {
        css::code::task::theTaskHub::get(comphelper::getProcessComponentContext())
            ->removeTaskListener(mxListener);
    }
    catch (const uno::Exception&)
    {
    }
}

void ErrorListPanel::Refresh()
{
    maEntries.clear();
    try
    {
        const uno::Sequence<css::code::task::Task> aTasks
            = css::code::task::theTaskHub::get(comphelper::getProcessComponentContext())->getTasks();
        for (const auto& rTask : aTasks)
        {
            ErrorEntry aEntry;
            switch (rTask.Level)
            {
                case css::code::core::Severity_FATAL:
                case css::code::core::Severity_ERROR:
                    aEntry.meSeverity = Severity::Error;
                    break;
                case css::code::core::Severity_WARNING:
                    aEntry.meSeverity = Severity::Warning;
                    break;
                default: // INFO, HINT
                    aEntry.meSeverity = Severity::Message;
                    break;
            }
            aEntry.mbIntelliSense = !IsBuildTask(rTask);
            aEntry.msIdentifier = rTask.Identifier;
            aEntry.msCode = rTask.Code;
            aEntry.msDescription = rTask.Description;
            aEntry.msProject = ProjectNameOf(rTask.Project);
            aEntry.msUrl = rTask.Url;
            aEntry.msProjectUrl = rTask.Project;
            aEntry.msFile = FileNameOf(rTask.Url);
            // Positions are 0-based in the API, 1-based on screen.
            aEntry.msLine = rTask.Url.isEmpty() ? OUString() : OUString::number(rTask.Location.Line + 1);
            maEntries.push_back(std::move(aEntry));
        }
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot read the task list: " << rException.Message);
    }

    UpdateFilterButtonLabels();
    UpdateList();
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

bool ErrorListPanel::IsInScope(const ErrorEntry& rEntry) const
{
    const int nScope = mxScope->get_active();
    if (nScope <= 0) // Entire Solution (or nothing selected)
        return true;
    try
    {
        const uno::Reference<uno::XComponentContext> xContext = comphelper::getProcessComponentContext();
        switch (nScope)
        {
            case 1: // Current Project
            {
                auto xActive = css::code::project::theProjectManager::get(xContext)->getActiveProject();
                // A task that names no project cannot be placed in one: shown
                // only when there is no active project to compare with.
                return xActive.is() ? rEntry.msProjectUrl == xActive->getUrl() : true;
            }
            // The services have no notion of a "current" document (that is the
            // editor tab strip's), so Current Document shows the same as Open Documents.
            case 2: // Current Document
            case 3: // Changed Documents
            case 4: // Open Documents
            {
                auto xManager = css::code::document::theDocumentManager::get(xContext);
                for (const auto& xDocument : xManager->getOpenDocuments())
                {
                    if (xDocument->getUrl() != rEntry.msUrl)
                        continue;
                    return nScope != 3
                           || xDocument->getState() == css::code::document::DocumentState_MODIFIED;
                }
                return false;
            }
        }
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot apply the scope filter: " << rException.Message);
    }
    return true;
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

    if (!IsInScope(rEntry))
        return false;

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
    maRowEntries.clear();
    int nRow = 0;
    for (size_t nEntry = 0; nEntry < maEntries.size(); ++nEntry)
    {
        const ErrorEntry& rEntry = maEntries[nEntry];
        if (!IsShown(rEntry, aNeedle))
            continue;
        maRowEntries.push_back(nEntry);

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

IMPL_LINK_NOARG(ErrorListPanel, RowActivatedHdl, const weld::TreeIter&, bool)
{
    // Double click / Enter: hand the task to the hub, whose handlers decide what
    // "activate" means (the editor-open handler jumps to the file and line).
    const int nRow = mxList->get_selected_index();
    if (nRow < 0 || static_cast<size_t>(nRow) >= maRowEntries.size())
        return true;
    try
    {
        css::code::task::theTaskHub::get(comphelper::getProcessComponentContext())
            ->handleTask(maEntries[maRowEntries[nRow]].msIdentifier);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot activate the task: " << rException.Message);
    }
    return true;
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
