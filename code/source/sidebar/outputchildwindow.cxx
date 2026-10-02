/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "outputchildwindow.hxx"

#include "outputdockingwindow.hxx"

#include <codeids.hxx>
#include <sfx2/chalign.hxx>

namespace code::sidebar
{

SFX_IMPL_DOCKINGWINDOW_WITHID(OutputChildWindow, SID_CODE_OUTPUT);

OutputChildWindow::OutputChildWindow(vcl::Window* pParent, sal_uInt16 nId, SfxBindings& rBindings,
                                      SfxChildWinInfo& rInfo)
    : SfxChildWindow(pParent, nId)
{
    auto pDockWin = VclPtr<OutputDockingWindow>::Create(rBindings, *this, pParent);
    SetWindow(pDockWin);

    // make window docked to the bottom initially (after first start) --
    // same pattern as starmath's own SmCmdBoxWrapper.
    SetAlignment(SfxChildAlignment::BOTTOM);

    pDockWin->Initialize(rInfo);
}

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
