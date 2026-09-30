/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/ui/XSidebar.hpp>
#include <sfx2/sidebar/PanelLayout.hxx>

namespace weld { class TreeView; }

namespace code::sidebar
{

// Real sfx2::sidebar::PanelLayout replacement for the old, hand-docked
// SolutionExplorerPanel (code/source/solutionexplorer.hxx, now retired) --
// same fake demo tree content, ported from a raw VCL ListBox to a
// weld::TreeView, the same widget kind every other sidebar panel in this
// tree (e.g. svx::sidebar::EmptyPanel) is built from.
class SolutionPanel final : public PanelLayout
{
public:
    SolutionPanel(weld::Widget* pParent,
        css::uno::Reference<css::ui::XSidebar> xSidebar);
    virtual ~SolutionPanel() override;

private:
    std::unique_ptr<weld::TreeView> mxTree;
    css::uno::Reference<css::ui::XSidebar> mxSidebar;
};

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
