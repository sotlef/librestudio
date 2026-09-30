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
#include "editorarea.hxx"
#include "outputpanel.hxx"
#include "solutionexplorer.hxx"

#include <sfx2/objface.hxx>
#include <sfx2/request.hxx>
#include <sfx2/sfxsids.hrc>
#include <sfx2/viewfrm.hxx>

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
}

Shell::Shell(SfxViewFrame& rFrame, SfxViewShell* /*pOldSh*/)
    : SfxViewShell(rFrame, SfxViewShellFlags::NO_NEWWINDOW)
{
    InitPanels();
}

Shell::~Shell()
{
    mxSolutionExplorer.disposeAndClear();
    mxEditorArea.disposeAndClear();
    mxOutputPanel.disposeAndClear();
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

    mxSolutionExplorer = VclPtr<SolutionExplorerPanel>::Create(&rContentWindow);
    mxSolutionExplorer->Show();

    mxEditorArea = VclPtr<EditorAreaPanel>::Create(&rContentWindow);
    mxEditorArea->Show();

    mxOutputPanel = VclPtr<OutputPanel>::Create(&rContentWindow);
    mxOutputPanel->Show();

    LayoutPanels(Point(0, 0), rContentWindow.GetOutputSizePixel());
}

void Shell::OuterResizePixel(const Point& rOfs, const Size& rSize)
{
    SfxViewShell::OuterResizePixel(rOfs, rSize);
    LayoutPanels(rOfs, rSize);
}

void Shell::LayoutPanels(const Point& rOfs, const Size& rSize)
{
    if (!mxSolutionExplorer || !mxEditorArea || !mxOutputPanel)
        return;

    const tools::Long nExplorerWidth = mxSolutionExplorer->IsVisible() ? rSize.Width() / 5 : 0;
    const tools::Long nOutputHeight = mxOutputPanel->IsVisible() ? rSize.Height() / 4 : 0;

    if (mxSolutionExplorer->IsVisible())
        mxSolutionExplorer->SetPosSizePixel(rOfs, Size(nExplorerWidth, rSize.Height()));

    mxEditorArea->SetPosSizePixel(Point(rOfs.X() + nExplorerWidth, rOfs.Y()),
        Size(rSize.Width() - nExplorerWidth, rSize.Height() - nOutputHeight));

    if (mxOutputPanel->IsVisible())
        mxOutputPanel->SetPosSizePixel(
            Point(rOfs.X() + nExplorerWidth, rOfs.Y() + rSize.Height() - nOutputHeight),
            Size(rSize.Width() - nExplorerWidth, nOutputHeight));
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
