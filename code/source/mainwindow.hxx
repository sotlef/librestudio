/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vcl/wrkwin.hxx>
#include <vcl/vclptr.hxx>
#include <vcl/menu.hxx>
#include <tools/link.hxx>

class ToolBox;
class StatusBar;
class SolutionExplorerPanel;
class EditorAreaPanel;
class OutputPanel;

// The main shell window of the "Code" prop: a visual-only stand-in for a
// future Visual-Studio-like IDE. Everything here is decoration -- no file is
// ever opened, no project is ever built. Individual pieces (the tree, the
// editor area, the output panel) are meant to be swapped for real
// implementations one at a time, without touching the rest of the shell.
class CodeMainWindow final : public WorkWindow
{
public:
    explicit CodeMainWindow(vcl::Window* pParent);
    virtual ~CodeMainWindow() override;
    virtual void dispose() override;
    virtual void Resize() override;

private:
    void InitMenuBar();
    void InitToolBox();
    void InitStatusBar();
    void InitPanels();
    void LayoutPanels();

    DECL_LINK(MenuSelectHdl, Menu*, bool);
    DECL_LINK(ToolboxClickHdl, ToolBox*, void);

    VclPtr<MenuBar> mxMenuBar;
    VclPtr<PopupMenu> mxFileMenu;
    VclPtr<PopupMenu> mxEditMenu;
    VclPtr<PopupMenu> mxViewMenu;
    VclPtr<PopupMenu> mxProjectMenu;
    VclPtr<PopupMenu> mxBuildMenu;
    VclPtr<PopupMenu> mxDebugMenu;
    VclPtr<PopupMenu> mxToolsMenu;
    VclPtr<PopupMenu> mxWindowMenu;
    VclPtr<PopupMenu> mxHelpMenu;

    VclPtr<ToolBox> mxToolBox;
    VclPtr<StatusBar> mxStatusBar;

    VclPtr<SolutionExplorerPanel> mxSolutionExplorer;
    VclPtr<EditorAreaPanel> mxEditorArea;
    VclPtr<OutputPanel> mxOutputPanel;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
