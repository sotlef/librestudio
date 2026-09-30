/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mainwindow.hxx"

#include "editorarea.hxx"
#include "outputpanel.hxx"
#include "solutionexplorer.hxx"

#include <vcl/status.hxx>
#include <vcl/svapp.hxx>
#include <vcl/toolbox.hxx>

namespace {
    // Menu-bar top-level entries.
    constexpr sal_uInt16 ID_MENU_FILE = 1;
    constexpr sal_uInt16 ID_MENU_EDIT = 2;
    constexpr sal_uInt16 ID_MENU_VIEW = 3;
    constexpr sal_uInt16 ID_MENU_PROJECT = 4;
    constexpr sal_uInt16 ID_MENU_BUILD = 5;
    constexpr sal_uInt16 ID_MENU_DEBUG = 6;
    constexpr sal_uInt16 ID_MENU_TOOLS = 7;
    constexpr sal_uInt16 ID_MENU_WINDOW = 8;
    constexpr sal_uInt16 ID_MENU_HELP = 9;

    // Leaf items. Only ID_FILE_EXIT and the two ID_VIEW_* ones actually do
    // anything -- everything else is a prop, present for the look of it.
    constexpr sal_uInt16 ID_FILE_NEW = 100;
    constexpr sal_uInt16 ID_FILE_OPEN = 101;
    constexpr sal_uInt16 ID_FILE_SAVE = 102;
    constexpr sal_uInt16 ID_FILE_EXIT = 103;

    constexpr sal_uInt16 ID_EDIT_UNDO = 110;
    constexpr sal_uInt16 ID_EDIT_REDO = 111;
    constexpr sal_uInt16 ID_EDIT_CUT = 112;
    constexpr sal_uInt16 ID_EDIT_COPY = 113;
    constexpr sal_uInt16 ID_EDIT_PASTE = 114;

    constexpr sal_uInt16 ID_VIEW_SOLUTION_EXPLORER = 120;
    constexpr sal_uInt16 ID_VIEW_OUTPUT = 121;

    constexpr sal_uInt16 ID_PROJECT_ADD_NEW_ITEM = 130;
    constexpr sal_uInt16 ID_PROJECT_PROPERTIES = 131;

    constexpr sal_uInt16 ID_BUILD_BUILD_SOLUTION = 140;
    constexpr sal_uInt16 ID_BUILD_REBUILD_SOLUTION = 141;
    constexpr sal_uInt16 ID_BUILD_CLEAN_SOLUTION = 142;

    constexpr sal_uInt16 ID_DEBUG_START = 150;
    constexpr sal_uInt16 ID_DEBUG_START_WITHOUT = 151;

    constexpr sal_uInt16 ID_TOOLS_OPTIONS = 160;

    constexpr sal_uInt16 ID_WINDOW_SPLIT = 170;

    constexpr sal_uInt16 ID_HELP_ABOUT = 180;

    constexpr tools::Long TOOLBOX_HEIGHT = 30;
    constexpr tools::Long STATUSBAR_HEIGHT = 20;
}

CodeMainWindow::CodeMainWindow(vcl::Window* pParent)
    : WorkWindow(pParent, WB_APP | WB_STDWORK | WB_SIZEABLE | WB_CLOSEABLE | WB_CLIPCHILDREN)
{
    SetText(u"LibreStudio Code"_ustr);

    InitMenuBar();
    InitToolBox();
    InitStatusBar();
    InitPanels();
}

CodeMainWindow::~CodeMainWindow()
{
    disposeOnce();
}

void CodeMainWindow::dispose()
{
    mxSolutionExplorer.disposeAndClear();
    mxEditorArea.disposeAndClear();
    mxOutputPanel.disposeAndClear();
    mxToolBox.disposeAndClear();
    mxStatusBar.disposeAndClear();

    SetMenuBar(nullptr);
    mxFileMenu.disposeAndClear();
    mxEditMenu.disposeAndClear();
    mxViewMenu.disposeAndClear();
    mxProjectMenu.disposeAndClear();
    mxBuildMenu.disposeAndClear();
    mxDebugMenu.disposeAndClear();
    mxToolsMenu.disposeAndClear();
    mxWindowMenu.disposeAndClear();
    mxHelpMenu.disposeAndClear();
    mxMenuBar.disposeAndClear();

    WorkWindow::dispose();
}

void CodeMainWindow::InitMenuBar()
{
    mxMenuBar = VclPtr<MenuBar>::Create();

    mxFileMenu = VclPtr<PopupMenu>::Create();
    mxFileMenu->InsertItem(ID_FILE_NEW, u"New Project..."_ustr);
    mxFileMenu->InsertItem(ID_FILE_OPEN, u"Open Project..."_ustr);
    mxFileMenu->InsertItem(ID_FILE_SAVE, u"Save"_ustr);
    mxFileMenu->InsertSeparator();
    mxFileMenu->InsertItem(ID_FILE_EXIT, u"Exit"_ustr);

    mxEditMenu = VclPtr<PopupMenu>::Create();
    mxEditMenu->InsertItem(ID_EDIT_UNDO, u"Undo"_ustr);
    mxEditMenu->InsertItem(ID_EDIT_REDO, u"Redo"_ustr);
    mxEditMenu->InsertSeparator();
    mxEditMenu->InsertItem(ID_EDIT_CUT, u"Cut"_ustr);
    mxEditMenu->InsertItem(ID_EDIT_COPY, u"Copy"_ustr);
    mxEditMenu->InsertItem(ID_EDIT_PASTE, u"Paste"_ustr);

    mxViewMenu = VclPtr<PopupMenu>::Create();
    mxViewMenu->InsertItem(ID_VIEW_SOLUTION_EXPLORER, u"Solution Explorer"_ustr);
    mxViewMenu->InsertItem(ID_VIEW_OUTPUT, u"Output"_ustr);

    mxProjectMenu = VclPtr<PopupMenu>::Create();
    mxProjectMenu->InsertItem(ID_PROJECT_ADD_NEW_ITEM, u"Add New Item..."_ustr);
    mxProjectMenu->InsertItem(ID_PROJECT_PROPERTIES, u"Properties"_ustr);

    mxBuildMenu = VclPtr<PopupMenu>::Create();
    mxBuildMenu->InsertItem(ID_BUILD_BUILD_SOLUTION, u"Build Solution"_ustr);
    mxBuildMenu->InsertItem(ID_BUILD_REBUILD_SOLUTION, u"Rebuild Solution"_ustr);
    mxBuildMenu->InsertItem(ID_BUILD_CLEAN_SOLUTION, u"Clean Solution"_ustr);

    mxDebugMenu = VclPtr<PopupMenu>::Create();
    mxDebugMenu->InsertItem(ID_DEBUG_START, u"Start Debugging"_ustr);
    mxDebugMenu->InsertItem(ID_DEBUG_START_WITHOUT, u"Start Without Debugging"_ustr);

    mxToolsMenu = VclPtr<PopupMenu>::Create();
    mxToolsMenu->InsertItem(ID_TOOLS_OPTIONS, u"Options..."_ustr);

    mxWindowMenu = VclPtr<PopupMenu>::Create();
    mxWindowMenu->InsertItem(ID_WINDOW_SPLIT, u"Split"_ustr);

    mxHelpMenu = VclPtr<PopupMenu>::Create();
    mxHelpMenu->InsertItem(ID_HELP_ABOUT, u"About LibreStudio Code..."_ustr);

    mxMenuBar->InsertItem(ID_MENU_FILE, u"~File"_ustr);
    mxMenuBar->SetPopupMenu(ID_MENU_FILE, mxFileMenu);
    mxMenuBar->InsertItem(ID_MENU_EDIT, u"~Edit"_ustr);
    mxMenuBar->SetPopupMenu(ID_MENU_EDIT, mxEditMenu);
    mxMenuBar->InsertItem(ID_MENU_VIEW, u"~View"_ustr);
    mxMenuBar->SetPopupMenu(ID_MENU_VIEW, mxViewMenu);
    mxMenuBar->InsertItem(ID_MENU_PROJECT, u"~Project"_ustr);
    mxMenuBar->SetPopupMenu(ID_MENU_PROJECT, mxProjectMenu);
    mxMenuBar->InsertItem(ID_MENU_BUILD, u"~Build"_ustr);
    mxMenuBar->SetPopupMenu(ID_MENU_BUILD, mxBuildMenu);
    mxMenuBar->InsertItem(ID_MENU_DEBUG, u"De~bug"_ustr);
    mxMenuBar->SetPopupMenu(ID_MENU_DEBUG, mxDebugMenu);
    mxMenuBar->InsertItem(ID_MENU_TOOLS, u"~Tools"_ustr);
    mxMenuBar->SetPopupMenu(ID_MENU_TOOLS, mxToolsMenu);
    mxMenuBar->InsertItem(ID_MENU_WINDOW, u"~Window"_ustr);
    mxMenuBar->SetPopupMenu(ID_MENU_WINDOW, mxWindowMenu);
    mxMenuBar->InsertItem(ID_MENU_HELP, u"~Help"_ustr);
    mxMenuBar->SetPopupMenu(ID_MENU_HELP, mxHelpMenu);

    mxMenuBar->SetSelectHdl(LINK(this, CodeMainWindow, MenuSelectHdl));

    SetMenuBar(mxMenuBar);
}

void CodeMainWindow::InitToolBox()
{
    mxToolBox = VclPtr<ToolBox>::Create(this, WB_BORDER);
    mxToolBox->SetButtonType(ButtonType::TEXT);

    mxToolBox->InsertItem(ToolBoxItemId(1), u"New"_ustr, OUString());
    mxToolBox->InsertItem(ToolBoxItemId(2), u"Open"_ustr, OUString());
    mxToolBox->InsertItem(ToolBoxItemId(3), u"Save"_ustr, OUString());
    mxToolBox->InsertSeparator();
    mxToolBox->InsertItem(ToolBoxItemId(4), u"Build"_ustr, OUString());
    mxToolBox->InsertItem(ToolBoxItemId(5), u"Run"_ustr, OUString());

    mxToolBox->SetClickHdl(LINK(this, CodeMainWindow, ToolboxClickHdl));
    mxToolBox->Show();
}

void CodeMainWindow::InitStatusBar()
{
    mxStatusBar = VclPtr<StatusBar>::Create(this, WB_BORDER | WB_RIGHT);
    mxStatusBar->InsertItem(1, 300);
    mxStatusBar->InsertItem(2, 150);
    mxStatusBar->SetItemText(1, u"Ready"_ustr);
    mxStatusBar->SetItemText(2, u"Debug | x64"_ustr);
    mxStatusBar->Show();
}

void CodeMainWindow::InitPanels()
{
    mxSolutionExplorer = VclPtr<SolutionExplorerPanel>::Create(this);
    mxSolutionExplorer->Show();

    mxEditorArea = VclPtr<EditorAreaPanel>::Create(this);
    mxEditorArea->Show();

    mxOutputPanel = VclPtr<OutputPanel>::Create(this);
    mxOutputPanel->Show();

    LayoutPanels();
}

void CodeMainWindow::Resize()
{
    WorkWindow::Resize();
    LayoutPanels();
}

void CodeMainWindow::LayoutPanels()
{
    if (!mxToolBox || !mxStatusBar || !mxSolutionExplorer || !mxEditorArea || !mxOutputPanel)
        return;

    const Size aWinSize = GetOutputSizePixel();

    mxToolBox->SetPosSizePixel(Point(0, 0), Size(aWinSize.Width(), TOOLBOX_HEIGHT));
    mxStatusBar->SetPosSizePixel(Point(0, aWinSize.Height() - STATUSBAR_HEIGHT),
        Size(aWinSize.Width(), STATUSBAR_HEIGHT));

    const tools::Long nContentTop = TOOLBOX_HEIGHT;
    const tools::Long nContentHeight = aWinSize.Height() - TOOLBOX_HEIGHT - STATUSBAR_HEIGHT;
    const tools::Long nExplorerWidth = mxSolutionExplorer->IsVisible() ? aWinSize.Width() / 5 : 0;
    const tools::Long nOutputHeight = mxOutputPanel->IsVisible() ? nContentHeight / 4 : 0;

    if (mxSolutionExplorer->IsVisible())
        mxSolutionExplorer->SetPosSizePixel(Point(0, nContentTop),
            Size(nExplorerWidth, nContentHeight));

    mxEditorArea->SetPosSizePixel(Point(nExplorerWidth, nContentTop),
        Size(aWinSize.Width() - nExplorerWidth, nContentHeight - nOutputHeight));

    if (mxOutputPanel->IsVisible())
        mxOutputPanel->SetPosSizePixel(
            Point(nExplorerWidth, nContentTop + nContentHeight - nOutputHeight),
            Size(aWinSize.Width() - nExplorerWidth, nOutputHeight));
}

IMPL_LINK_NOARG(CodeMainWindow, MenuSelectHdl, Menu*, bool)
{
    switch (mxMenuBar->GetCurItemId())
    {
        case ID_FILE_EXIT:
            Application::Quit();
            break;
        case ID_VIEW_SOLUTION_EXPLORER:
            mxSolutionExplorer->Show(!mxSolutionExplorer->IsVisible());
            LayoutPanels();
            break;
        case ID_VIEW_OUTPUT:
            mxOutputPanel->Show(!mxOutputPanel->IsVisible());
            LayoutPanels();
            break;
        default:
            // Everything else is a prop: acknowledged, does nothing.
            break;
    }
    return false;
}

IMPL_LINK_NOARG(CodeMainWindow, ToolboxClickHdl, ToolBox*, void)
{
    // Toolbar buttons are props too, same as most menu items.
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
