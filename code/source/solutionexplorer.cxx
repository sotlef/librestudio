/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "solutionexplorer.hxx"

#include <vcl/toolkit/lstbox.hxx>

SolutionExplorerPanel::SolutionExplorerPanel(vcl::Window* pParent)
    : DockingWindow(pParent, WB_BORDER)
{
    SetText(u"Solution Explorer"_ustr);

    // A flat ListBox with hand-indented fake entries, not a real tree widget:
    // this is the deliberate v1 shortcut. Swap for weld::TreeView (needs a
    // .ui file) plus a real project model when this stops being a prop.
    mxTree = VclPtr<ListBox>::Create(this, WB_BORDER);
    mxTree->InsertEntry(u"Solution 'DemoProject' (1 project)"_ustr);
    mxTree->InsertEntry(u"    DemoProject"_ustr);
    mxTree->InsertEntry(u"        Source Files"_ustr);
    mxTree->InsertEntry(u"            main.cpp"_ustr);
    mxTree->InsertEntry(u"        Header Files"_ustr);
    mxTree->InsertEntry(u"            utils.h"_ustr);
    mxTree->InsertEntry(u"        README.md"_ustr);
    mxTree->Show();
}

SolutionExplorerPanel::~SolutionExplorerPanel()
{
    disposeOnce();
}

void SolutionExplorerPanel::dispose()
{
    mxTree.disposeAndClear();
    DockingWindow::dispose();
}

void SolutionExplorerPanel::Resize()
{
    DockingWindow::Resize();
    if (mxTree)
        mxTree->SetPosSizePixel(Point(0, 0), GetOutputSizePixel());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
