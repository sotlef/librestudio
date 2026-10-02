/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "outputdockingwindow.hxx"

#include "outputcontroller.hxx"

#include <sfx2/bindings.hxx>
#include <sfx2/chalign.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/frame.hxx>
#include <sfx2/viewfrm.hxx>

using namespace css;

namespace code::sidebar
{

OutputDockingWindow::OutputDockingWindow(SfxBindings& rBindings, SfxChildWindow& rChildWindow,
                                          vcl::Window* pParent)
    : SfxDockingWindow(rBindings, &rChildWindow, pParent,
                       WB_STDDOCKWIN | WB_CLIPCHILDREN | WB_SIZEABLE | WB_3DLOOK)
{
    uno::Reference<frame::XFrame> xFrame;
    if (SfxDispatcher* pDispatcher = rBindings.GetDispatcher())
        if (SfxViewFrame* pViewFrame = pDispatcher->GetFrame())
            xFrame = pViewFrame->GetFrame().GetFrameInterface();

    mpOutputController = std::make_unique<OutputController>(this, rBindings, xFrame);

    // default docked height for the very first start: SfxDockingWindow::
    // Initialize() snapshots GetOutputSizePixel() as the initial splitter
    // size, and with no persisted SfxChildWinInfo this window has never
    // been laid out -- 0x0 -- so the bottom dock would come up as a 1px
    // sliver the user has to drag open every session.
    SetOutputSizePixel(Size(600, 200));
}

OutputDockingWindow::~OutputDockingWindow()
{
    disposeOnce();
}

void OutputDockingWindow::dispose()
{
    mpOutputController.reset();
    SfxDockingWindow::dispose();
}

SfxChildAlignment OutputDockingWindow::CheckAlignment(SfxChildAlignment eCurrentAlignment,
                                                       SfxChildAlignment eRequestedAlignment)
{
    switch (eRequestedAlignment)
    {
        case SfxChildAlignment::BOTTOM:
        case SfxChildAlignment::LOWESTBOTTOM:
        case SfxChildAlignment::HIGHESTBOTTOM:
        case SfxChildAlignment::NOALIGNMENT:
            return eRequestedAlignment;

        default:
            return eCurrentAlignment;
    }
}

void OutputDockingWindow::Resize()
{
    SfxDockingWindow::Resize();

    if (mpOutputController)
        mpOutputController->NotifyResize();
}

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
