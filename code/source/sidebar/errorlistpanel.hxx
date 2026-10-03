/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/code/task/Task.hpp>
#include <com/sun/star/code/task/XTaskListener.hpp>
#include <sfx2/sidebar/PanelLayout.hxx>

#include <tools/gen.hxx>

#include <memory>
#include <vector>

namespace code { class UiNotifier; }

namespace weld { class TreeIter; class Button; class ComboBox; class Toggleable; class ToggleButton; class TreeView; }

namespace code::sidebar
{

// Visual-Studio-style Error List: the second tab of the bottom Output dock
// (ErrorListDeck, Sidebar.xcu). Composite like OutputLogPanel: a toolbar
// strip above the table -- scope selector, severity filter toggles with
// live counts, Clear All Filters, provider selector, search box -- then a
// multi-column error table.
class ErrorListPanel final : public PanelLayout
{
public:
    explicit ErrorListPanel(weld::Widget* pParent);
    virtual ~ErrorListPanel() override;

private:
    enum class Severity
    {
        Error,
        Warning,
        Message
    };

    // One row of the table: a Task of theTaskHub, prepared for display.
    struct ErrorEntry
    {
        Severity meSeverity;
        bool mbIntelliSense; // provider: IntelliSense (diagnostics) vs. Build
        OUString msIdentifier; // the task's, to hand back to XTaskHub::handleTask()
        OUString msCode;
        OUString msDescription;
        OUString msProject;
        OUString msFile;
        OUString msLine;
        OUString msUrl;
        OUString msProjectUrl;
    };

    // Re-reads the tasks from the hub, then redraws; runs on the main thread.
    void Refresh();
    void UpdateList();
    void UpdateFilterButtonLabels();
    bool IsShown(const ErrorEntry& rEntry, const OUString& rNeedle) const;
    bool IsInScope(const ErrorEntry& rEntry) const;

    DECL_LINK(FilterToggledHdl, weld::Toggleable&, void);
    DECL_LINK(FilterChangedHdl, weld::ComboBox&, void);
    DECL_LINK(ClearFiltersHdl, weld::Button&, void);
    DECL_LINK(ListSizeAllocHdl, const Size&, void);
    DECL_LINK(RowActivatedHdl, const weld::TreeIter&, bool);

    std::unique_ptr<weld::ComboBox> mxScope;
    std::unique_ptr<weld::ToggleButton> mxErrors;
    std::unique_ptr<weld::ToggleButton> mxWarnings;
    std::unique_ptr<weld::ToggleButton> mxMessages;
    std::unique_ptr<weld::Button> mxClearFilters;
    std::unique_ptr<weld::ComboBox> mxProvider;
    std::unique_ptr<weld::ComboBox> mxSearch;
    std::unique_ptr<weld::TreeView> mxList;

    std::vector<ErrorEntry> maEntries;
    // the entry each table row stands for, in row order (rows are filtered)
    std::vector<size_t> maRowEntries;

    std::shared_ptr<::code::UiNotifier> mxNotifier;
    css::uno::Reference<css::code::task::XTaskListener> mxListener;
};

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
