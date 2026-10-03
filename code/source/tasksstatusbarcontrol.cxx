/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "tasksstatusbarcontrol.hxx"
#include "workbench.hxx"

#include <svl/voiditem.hxx>
#include <tools/poly.hxx>
#include <vcl/event.hxx>
#include <vcl/settings.hxx>
#include <vcl/status.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/Label.hxx>
#include <vcl/weld/Popover.hxx>
#include <vcl/weld/Window.hxx>
#include <vcl/weld/weldutils.hxx>

namespace code
{

SFX_IMPL_STATUSBAR_CONTROL(TasksStatusBarControl, SfxVoidItem);

TasksStatusBarControl::TasksStatusBarControl(sal_uInt16 _nSlotId, sal_uInt16 _nId, StatusBar& rStb)
    : SfxStatusBarControl(_nSlotId, _nId, rStb)
{
    // Visual Studio's status bar has no separator lines between items at
    // all; vcl's StatusBar draws one left of every item unconditionally
    // (independent of the items' In/Out/Flat bits), so turn them off for
    // the whole bar from its one custom control.
    rStb.SetSeparatorsVisible(false);
}

TasksStatusBarControl::~TasksStatusBarControl()
{
}

void TasksStatusBarControl::Paint(const UserDrawEvent& rEvt)
{
    vcl::RenderContext* pDev = rEvt.GetRenderContext();
    const tools::Rectangle aRect = rEvt.GetRect();

    auto aPop = pDev->ScopedPush(vcl::PushFlags::LINECOLOR | vcl::PushFlags::FILLCOLOR);
    pDev->SetLineColor(
        pDev->GetSettings().GetStyleSettings().GetLabelTextColor());
    pDev->SetFillColor(); // outline only

    // Speech bubble, Visual Studio's background-tasks glyph: a rectangle
    // whose bottom edge's middle third folds into a downward corner (the
    // bubble's tail).
    constexpr tools::Long nW = 12; // bubble width
    constexpr tools::Long nBody = 8; // rectangle height
    constexpr tools::Long nTail = 3; // how far the corner dips below
    const tools::Long nX = aRect.Left() + (aRect.GetWidth() - nW) / 2;
    const tools::Long nY = aRect.Top() + (aRect.GetHeight() - (nBody + nTail)) / 2;

    tools::Polygon aBubble(7);
    aBubble.SetPoint(Point(nX, nY), 0);
    aBubble.SetPoint(Point(nX + nW, nY), 1);
    aBubble.SetPoint(Point(nX + nW, nY + nBody), 2);
    aBubble.SetPoint(Point(nX + nW * 2 / 3, nY + nBody), 3);
    aBubble.SetPoint(Point(nX + nW / 2, nY + nBody + nTail), 4);
    aBubble.SetPoint(Point(nX + nW / 3, nY + nBody), 5);
    aBubble.SetPoint(Point(nX, nY + nBody), 6);
    pDev->DrawPolygon(aBubble);
}

void TasksStatusBarControl::Click()
{
    if (mxPopover && mbShown)
    {
        // second click while the popover is up = toggle it away; mbShown
        // flips in PopoverClosedHdl
        mxPopover->popdown();
        return;
    }

    tools::Rectangle aRect = GetStatusBar().GetItemRect(GetId());
    weld::Window* pPopupParent = weld::GetPopupParent(GetStatusBar(), aRect);
    if (!mxPopover)
    {
        mxBuilder = Application::CreateBuilder(pPopupParent, u"code/ui/taskspopover.ui"_ustr);
        mxPopover = mxBuilder->weld_popover(u"popover"_ustr);
        mxPopover->connect_closed(LINK(this, TasksStatusBarControl, PopoverClosedHdl));
    }
    // what runs right now, asked each time the popover opens
    if (auto xLabel = mxBuilder->weld_label(u"label"_ustr))
        if (Workbench* pWorkbench = Workbench::Instance())
            xLabel->set_label(pWorkbench->GetJobsText());

    // weld::Placement only knows Under/End, but both backends' popovers
    // flip to the other side on their own when there is no room below --
    // and below a status bar at the screen's bottom edge there isn't
    mxPopover->popup_at_rect(pPopupParent, aRect);
    // the generic-vcl popover is a DockingWindow in popup mode and does
    // not size itself to its content (it popped up as a tiny empty
    // square); same explicit call sc's checklistmenu makes
    mxPopover->resize_to_request();
    mbShown = true;
}

IMPL_LINK_NOARG(TasksStatusBarControl, PopoverClosedHdl, weld::Popover&, void)
{
    // closed from outside (click elsewhere, Esc) or via popdown() above
    mbShown = false;
}

} // namespace code

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
