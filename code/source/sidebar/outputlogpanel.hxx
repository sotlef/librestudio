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

namespace weld { class TextView; }

namespace code::sidebar
{

// Real sfx2::sidebar::PanelLayout replacement for the old, hand-docked
// OutputPanel (code/source/outputpanel.hxx, now retired) -- same fake
// build-log content, ported from a raw DockingWindow::Paint() to a real
// weld::TextView, the same way SolutionPanel ported the old
// SolutionExplorerPanel's raw ListBox to a weld::TreeView.
class OutputLogPanel final : public PanelLayout
{
public:
    explicit OutputLogPanel(weld::Widget* pParent);
    virtual ~OutputLogPanel() override;

private:
    std::unique_ptr<weld::TextView> mxLog;
};

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
