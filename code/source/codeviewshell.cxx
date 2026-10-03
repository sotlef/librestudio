/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "codeviewshell.hxx"

#include "codedocshell.hxx"
#include "editormdipanel.hxx"
#include "sidebar/outputchildwindow.hxx"

#include <codeids.hxx>
#include <sfx2/objface.hxx>
#include <sfx2/request.hxx>
#include <sfx2/sfxsids.hrc>
#include <svl/stritem.hxx>
#include <svl/voiditem.hxx>
#include <sfx2/sidebar/SidebarChildWindow.hxx>
#include <sfx2/viewfrm.hxx>
#include <vcl/EnumContext.hxx>

// See codedocshell.cxx for why this is needed.
extern SFX2_DLLPUBLIC constinit const SfxType0 aSfxVoidItem_Impl;

#define ShellClass_code_Shell
#include <codeslots.hxx>

namespace code
{

SFX_IMPL_NAMED_VIEWFACTORY(Shell, "Default")
{
    SFX_VIEW_REGISTRATION(DocShell);
}

SFX_IMPL_INTERFACE(code_Shell, SfxViewShell)

void code_Shell::InitInterface_Impl()
{
    // Lets the real Sidebar (its "Solution" deck now hosts what used to be
    // the in-content SolutionExplorerPanel, see code/source/sidebar/) attach
    // its SfxChildWindow to this view -- the same call
    // sw/source/uibase/uiview/view0.cxx makes for Writer.
    GetStaticInterface()->RegisterChildWindow(
        sfx2::sidebar::SidebarChildWindow::GetChildWindowId());

    // Same for the bottom-only Output dock (code/source/sidebar/) --
    // the in-content OutputPanel it replaces is gone, see LayoutPanels()
    // below.
    GetStaticInterface()->RegisterChildWindow(
        code::sidebar::OutputChildWindow::GetChildWindowId());
}

Shell::Shell(SfxViewFrame& rFrame, SfxViewShell* /*pOldSh*/)
    : SfxViewShell(rFrame, SfxViewShellFlags::NO_NEWWINDOW)
{
    // Without this, SidebarController (sfx2/source/sidebar/SidebarController.cxx)
    // sits registered as a valid css::ui::XContextChangeEventListener but
    // never gets its first, deck-selecting ContextChangeEventObject: that
    // broadcast is gated on SfxShell::maContextChangeBroadcaster's stored
    // context name (sfx2/source/control/shell.cxx, BroadcastContextForActivation())
    // being non-empty, and it starts empty until SetContextName() is
    // called -- every real view shell (sw's textsh.cxx, sc's cellsh.cxx, ...)
    // does this in its own constructor. Shell activation itself already
    // happens normally through SfxDispatcher's ordinary shell-stack push,
    // so this is the one missing piece, not another Appear()-style cascade
    // to fake.
    SetContextName(vcl::EnumContext::GetContextName(vcl::EnumContext::Context::Default));

    InitPanels();
}

Shell::~Shell()
{
    // Unregister before destroying it -- same order as SwView's dtor, so
    // the base class never holds a dangling window pointer.
    SetWindow(nullptr);
    mxEditorArea.disposeAndClear();
}

void Shell::InitPanels()
{
    vcl::Window& rContentWindow = GetViewFrame().GetWindow();

    // The editor is a child of the frame's own content window, and -- like
    // every real app's edit window (sw's SwEditWin, basctl's window) -- it,
    // NOT the content window itself, is what gets registered via
    // SetWindow(). Two reasons, both load-bearing:
    //
    // - SfxViewFrame::DoAdjustPosSizePixel() (sfx2/source/view/viewfrm.cxx)
    //   only runs the OuterResizePixel()/SetBorderPixel() layout cascade at
    //   all when SfxViewShell::GetWindow() is non-null; our standalone
    //   Frame never resizes this view without it.
    //
    // - SfxViewFrame::SetBorderPixelImpl() positions GetWindow() at the
    //   edit area -- (0,0, content-window size) for a shell with no inner
    //   tools -- as a CHILD inside the content window. Registering the
    //   content window itself here made that call reposition the content
    //   window to (0,0) within the sfx2 WorkWindow, clobbering the
    //   tool-space border offset SfxFrame::SetToolSpaceBorderPixel_Impl()
    //   had just applied: any LEFT/TOP-docked strip (e.g. the collapsed
    //   sidebar's 8px fade-in splitter) ended up covered by the editor.
    //   RIGHT/BOTTOM docks survived only because (0,0) happens to be the
    //   correct position when the border is on those sides.
    //
    // With the editor registered instead, SetBorderPixelImpl() lays the
    // editor out inside the content window on every resize, so there is no
    // hand-written OuterResizePixel()/LayoutPanels() layout here anymore.
    // (The Output dock and Solution deck live outside this content area
    // entirely, as real sfx2 docks -- see code/source/sidebar/.)
    mxEditorArea = VclPtr<EditorMDIPanel>::Create(&rContentWindow);
    mxEditorArea->Show();
    SetWindow(mxEditorArea.get());

    // First paint happens before any resize reaches the cascade above;
    // start out filling the content window like SetBorderPixelImpl() will.
    mxEditorArea->SetPosSizePixel(Point(0, 0), rContentWindow.GetOutputSizePixel());
}

void Shell::Execute(SfxRequest& rReq)
{
    // Both SID_NEWDOC and SID_SAVEDOC are still decorative -- same "inert
    // menu item" spirit as v1, just reached through the real SfxDispatcher.
    rReq.Done();
}

void Shell::GetState(SfxItemSet& rSet)
{
    // SID_NEWDOC/SID_SAVEDOC: leaving them untouched keeps both slots
    // enabled; nothing here depends on document state yet.

    // Status bar items. There is no real task/readiness machinery yet:
    // "Ready" is the honest, constant idle state (like the bubble button's
    // constant "No background tasks running." popover).
    if (rSet.GetItemState(SID_CODE_STATUS_TASKS, false) != SfxItemState::UNKNOWN)
        rSet.Put(SfxVoidItem(SID_CODE_STATUS_TASKS));
    if (rSet.GetItemState(SID_CODE_STATUS_READY, false) != SfxItemState::UNKNOWN)
        rSet.Put(SfxStringItem(SID_CODE_STATUS_READY, u"Ready"_ustr));
}

} // namespace code

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
