/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <tools/mapunit.hxx>
#include <sfx2/chalign.hxx>
#include <sfx2/sidebar/TabBar.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/sidebar/SidebarChildWindow.hxx>
#include <sfx2/sidebar/SidebarController.hxx>
#include <sfx2/sidebar/SidebarDockingWindow.hxx>
#include <sfx2/sfxsids.hrc>
#include <helpids.h>
#include <comphelper/lok.hxx>

namespace {
SfxChildAlignment gDefaultSidebarAlignment = SfxChildAlignment::RIGHT;
}

namespace sfx2::sidebar {

SFX_IMPL_DOCKINGWINDOW_WITHID(SidebarChildWindow, SID_SIDEBAR);

void SidebarChildWindow::SetDefaultAlignment(SfxChildAlignment eAlign)
{
    gDefaultSidebarAlignment = eAlign;
}

SidebarChildWindow::SidebarChildWindow(vcl::Window* pParentWindow, sal_uInt16 nId,
                                       SfxBindings& rBindings, SfxChildWinInfo& rInfo)
    : SfxChildWindow(pParentWindow, nId)
{
    auto pDockWin = VclPtr<SidebarDockingWindow>::Create(
        rBindings, *this, pParentWindow,
        WB_STDDOCKWIN | WB_OWNERDRAWDECORATION | WB_CLIPCHILDREN | WB_SIZEABLE | WB_3DLOOK);
    SetWindow(pDockWin);

    // Real, persisted (or drag-and-dropped) alignment state -- if any --
    // still wins over the app-configurable default below.
    SfxChildAlignment eSavedAlign;
    SetAlignment(rInfo.GetExtraData_Impl(&eSavedAlign) ? eSavedAlign : gDefaultSidebarAlignment);

    pDockWin->SetHelpId(HID_SIDEBAR_WINDOW);
    pDockWin->SetOutputSizePixel(Size(GetDefaultWidth(pDockWin), 450));

    if (rInfo.aExtraString.isEmpty() && rInfo.aModule != "sdraw" && rInfo.aModule != "simpress"
        && rInfo.aModule != "smath")
    {
        // When this is the first start (never had the sidebar open yet),
        // default to non-expanded sidebars in Writer and Calc.
        //
        // HACK: unfortunately I haven't found a clean solution to do
        // this, so do it this way:
        //
        if (!comphelper::LibreOfficeKit::isActive())
        {
            pDockWin->SetSizePixel(
                Size(TabBar::GetDefaultWidth(),
                     pDockWin->GetSizePixel().Height()));
        }
    }

    pDockWin->Initialize(rInfo);

    if (comphelper::LibreOfficeKit::isActive())
    {
        // Undock sidebar in LOK to allow for resizing freely
        // (i.e. when the client window is resized) and collapse
        // it so the client can open it on demand.
        pDockWin->SetFloatingSize(Size(pDockWin->GetSizePixel().Width(),
                                       pDockWin->GetSizePixel().Height()));
        pDockWin->SetFloatingMode(true);
    }

    SetHideNotDelete(true);

    pDockWin->Show();

    // GetOrCreateSidebarController() above (SidebarDockingWindow's own
    // constructor, called from the very first line of this constructor)
    // creates the SidebarController -- and, whenever this module's context
    // is already active (e.g. this is a recreation from a ".uno:Sidebar"
    // menu/toolbar toggle -- SfxWorkWindow::ToggleChildWindow_Impl,
    // sfx2/source/appl/workwin.cxx -- unconditionally destroys and recreates
    // this whole child window every time, regardless of
    // SetHideNotDelete()), its first deck-selection/layout pass can run
    // synchronously right then and there -- before SetOutputSizePixel()
    // above has even run, let alone Show(). That pass's own width
    // computations (SidebarController::RestrictWidth()/NotifyResize()) are
    // therefore made against this window's still-default (zero) size, and
    // nothing here re-does them once a real size actually exists. Ask once
    // more, now that it does; NotifyResize() is idempotent and cheap.
    if (SidebarController* pController = pDockWin->GetOrCreateSidebarController().get())
        pController->NotifyResize();
}

sal_Int32 SidebarChildWindow::GetDefaultWidth(vcl::Window const* pWindow)
{
    if (pWindow != nullptr)
    {
        // Width of the paragraph panel.
        const static sal_Int32 nMaxPropertyPageWidth(146);

        return pWindow->LogicToPixel(Point(nMaxPropertyPageWidth,1), MapMode(MapUnit::MapAppFont)).X()
            + TabBar::GetDefaultWidth();
    }
    else
        return 0;
}

} // end of namespace sfx2::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
