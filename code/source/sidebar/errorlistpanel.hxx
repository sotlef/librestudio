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

#include <tools/gen.hxx>

#include <vector>

namespace weld { class Button; class ComboBox; class Toggleable; class ToggleButton; class TreeView; }

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

    // One row of the (demo) error table.
    struct ErrorEntry
    {
        Severity meSeverity;
        bool mbIntelliSense; // provider: IntelliSense vs. Build
        OUString msCode;
        OUString msDescription;
        OUString msProject;
        OUString msFile;
        OUString msLine;
    };

    void UpdateList();
    void UpdateFilterButtonLabels();
    bool IsShown(const ErrorEntry& rEntry, const OUString& rNeedle) const;

    DECL_LINK(FilterToggledHdl, weld::Toggleable&, void);
    DECL_LINK(FilterChangedHdl, weld::ComboBox&, void);
    DECL_LINK(ClearFiltersHdl, weld::Button&, void);
    DECL_LINK(ListSizeAllocHdl, const Size&, void);

    std::unique_ptr<weld::ComboBox> mxScope;
    std::unique_ptr<weld::ToggleButton> mxErrors;
    std::unique_ptr<weld::ToggleButton> mxWarnings;
    std::unique_ptr<weld::ToggleButton> mxMessages;
    std::unique_ptr<weld::Button> mxClearFilters;
    std::unique_ptr<weld::ComboBox> mxProvider;
    std::unique_ptr<weld::ComboBox> mxSearch;
    std::unique_ptr<weld::TreeView> mxList;

    std::vector<ErrorEntry> maEntries;
};

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
