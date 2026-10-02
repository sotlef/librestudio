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

#include <sfx2/objface.hxx>
#include <sfx2/request.hxx>
#include <sfx2/sfxsids.hrc>
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
    mxEditorArea.disposeAndClear();
}

void Shell::InitPanels()
{
    vcl::Window& rContentWindow = GetViewFrame().GetWindow();

    // SfxViewFrame::DoAdjustPosSizePixel() -- the real caller of
    // OuterResizePixel() below (sfx2/source/view/viewfrm.cxx) -- only calls
    // it when SfxViewShell::GetWindow() is non-null:
    //   if (pSh && pSh->GetWindow() && !m_nAdjustPosPixelLock) ...
    // basctl::Shell never calls SetWindow() either, but basctl only ever
    // runs hosted inside a full soffice.bin session, where some other path
    // apparently keeps it laid out; our own standalone Frame never resizes
    // this view at all without this call, so the panels stay at whatever
    // (possibly 0x0) size they had when first created.
    SetWindow(&rContentWindow);

    mxEditorArea = VclPtr<EditorMDIPanel>::Create(&rContentWindow);
    mxEditorArea->Show();

    LayoutPanels(Point(0, 0), rContentWindow.GetOutputSizePixel());
}

void Shell::OuterResizePixel(const Point& rOfs, const Size& rSize)
{
    SfxViewShell::OuterResizePixel(rOfs, rSize);
    LayoutPanels(rOfs, rSize);
}

void Shell::LayoutPanels(const Point& rOfs, const Size& rSize)
{
    if (!mxEditorArea)
        return;

    // The Output dock (code/source/sidebar/) now lives outside this content
    // area entirely, the same way the real Sidebar's Solution deck does --
    // the editor simply reclaims the quarter-height strip the old, hand-docked
    // OutputPanel used to occupy here.
    mxEditorArea->SetPosSizePixel(rOfs, rSize);
}

void Shell::Execute(SfxRequest& rReq)
{
    // Both SID_NEWDOC and SID_SAVEDOC are still decorative -- same "inert
    // menu item" spirit as v1, just reached through the real SfxDispatcher.
    rReq.Done();
}

void Shell::GetState(SfxItemSet& /*rSet*/)
{
    // Leaving the set untouched keeps both slots enabled; nothing here
    // depends on document state yet.
}

} // namespace code

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
