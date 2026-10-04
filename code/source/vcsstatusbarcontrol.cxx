/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "vcsstatusbarcontrol.hxx"

#include "gitbranchespopover.hxx"
#include "uibridge.hxx"

#include <com/sun/star/code/project/XProject.hpp>
#include <com/sun/star/code/project/XProjectManager.hpp>
#include <com/sun/star/code/project/theProjectManager.hpp>
#include <com/sun/star/code/vcs/RepositorySummary.hpp>
#include <com/sun/star/code/vcs/XVcsManager.hpp>
#include <com/sun/star/code/vcs/theVcsManager.hpp>
#include <comphelper/processfactory.hxx>
#include <sal/log.hxx>
#include <svl/stritem.hxx>
#include <tools/poly.hxx>
#include <vcl/event.hxx>
#include <vcl/settings.hxx>
#include <vcl/status.hxx>
#include <vcl/weld/weldutils.hxx>

using namespace css;
using namespace css::code;
using namespace css::code::vcs;

namespace code
{
uno::Reference<XRepository> CurrentRepository()
{
    try
    {
        auto xContext = comphelper::getProcessComponentContext();
        uno::Reference<XVcsManager> xManager = theVcsManager::get(xContext);
        if (auto xProject = project::theProjectManager::get(xContext)->getActiveProject())
            if (auto xRepository = xManager->getRepository(xProject->getUrl()))
                return xRepository;
        // the active project is in none; one of the others may be
        const auto aRepositories = xManager->getRepositories();
        if (aRepositories.hasElements())
            return aRepositories[0];
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot tell the repository: " << rException.Message);
    }
    return {};
}

OUString VcsStatusText()
{
    const auto xRepository = CurrentRepository();
    if (!xRepository.is())
        return OUString();
    try
    {
        const RepositorySummary aSummary = xRepository->getSummary();
        OUString aText = aSummary.Label;
        if (aSummary.Ahead > 0)
            aText += " ↑" + OUString::number(aSummary.Ahead);
        if (aSummary.Behind > 0)
            aText += " ↓" + OUString::number(aSummary.Behind);
        if (aSummary.Busy)
            aText += u" …"_ustr;
        return aText;
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot read the repository: " << rException.Message);
    }
    return OUString();
}

SFX_IMPL_STATUSBAR_CONTROL(VcsStatusBarControl, SfxStringItem);

VcsStatusBarControl::VcsStatusBarControl(sal_uInt16 nSlotId, sal_uInt16 nId, StatusBar& rStb)
    : SfxStatusBarControl(nSlotId, nId, rStb)
{
    mxNotifier = std::make_shared<UiNotifier>([this] { Update(); });
    mxListener = new ServiceListener(mxNotifier);
    try
    {
        theVcsManager::get(comphelper::getProcessComponentContext())
            ->addVcsListener(mxListener);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "the branch cannot follow version control: " << rException.Message);
    }
    // what is already known
    mxNotifier->Notify();
}

VcsStatusBarControl::~VcsStatusBarControl()
{
    mxNotifier->Detach();
    try
    {
        theVcsManager::get(comphelper::getProcessComponentContext())->removeVcsListener(mxListener);
    }
    catch (const uno::Exception&)
    {
    }
}

namespace
{
// The room left of the text for the branch icon: vcl sizes the item by its
// text, so the text it is given starts with spaces as wide as the icon.
constexpr std::u16string_view IconRoom = u"\u2003";
}

void VcsStatusBarControl::Update()
{
    const OUString aText = VcsStatusText();
    const sal_uInt16 nId = GetId();
    StatusBar& rBar = GetStatusBar();
    maText = aText;
    rBar.SetItemText(nId, aText.isEmpty() ? aText : OUString(IconRoom) + aText);
    // outside a working copy there is nothing to show
    if (aText.isEmpty())
    {
        rBar.HideItem(nId);
        return;
    }
    rBar.ShowItem(nId);
    OUString aTip;
    if (const auto xRepository = CurrentRepository())
        try
        {
            aTip = xRepository->getSummary().Tooltip;
        }
        catch (const uno::Exception&)
        {
        }
    rBar.SetQuickHelpText(nId, aTip);
}

void VcsStatusBarControl::StateChangedAtStatusBarControl(sal_uInt16, SfxItemState,
                                                         const SfxPoolItem*)
{
    // what to show is asked of the repository, not carried by the slot
    Update();
}

// Git's branch mark: a line with a dot at each end, and a second dot that a
// line leads from to the first line.
void VcsStatusBarControl::Paint(const UserDrawEvent& rEvt)
{
    vcl::RenderContext* pDev = rEvt.GetRenderContext();
    const tools::Rectangle aRect = rEvt.GetRect();
    const StyleSettings& rStyle = pDev->GetSettings().GetStyleSettings();

    auto aPop = pDev->ScopedPush(vcl::PushFlags::LINECOLOR | vcl::PushFlags::FILLCOLOR
                                 | vcl::PushFlags::TEXTCOLOR);
    pDev->SetLineColor(rStyle.GetLabelTextColor());
    pDev->SetTextColor(rStyle.GetLabelTextColor());
    pDev->SetFillColor(); // outlines only

    constexpr tools::Long nW = 11; // icon width
    constexpr tools::Long nH = 14; // icon height
    constexpr tools::Long nR = 2; // radius of a dot
    const tools::Long nX = aRect.Left() + 2;
    const tools::Long nY = aRect.Top() + (aRect.GetHeight() - nH) / 2;
    const tools::Long nLeft = nX + nR;
    const tools::Long nRight = nX + nW - nR - 1;
    const Point aTop(nLeft, nY + nR);
    const Point aBottom(nLeft, nY + nH - nR - 1);
    const Point aSide(nRight, nY + nR + 2);
    const tools::Long nMid = nY + nH / 2 + 1;

    pDev->DrawLine(Point(aTop.X(), aTop.Y() + nR), Point(aBottom.X(), aBottom.Y() - nR));
    tools::Polygon aJoin(4);
    aJoin.SetPoint(Point(aSide.X(), aSide.Y() + nR), 0);
    aJoin.SetPoint(Point(aSide.X(), nMid - 3), 1);
    aJoin.SetPoint(Point(aSide.X() - 3, nMid), 2);
    aJoin.SetPoint(Point(nLeft, nMid + 3), 3);
    pDev->DrawPolyLine(aJoin);
    for (const Point& rCentre : { aTop, aBottom, aSide })
        pDev->DrawEllipse(tools::Rectangle(rCentre.X() - nR, rCentre.Y() - nR, rCentre.X() + nR,
                                           rCentre.Y() + nR));

    // the text a few pixels after the icon, whatever width the backend's font
    // gives the blank that keeps the room (the item is sized by that blank)
    const tools::Long nTextX = nX + nW + 7;
    pDev->DrawText(Point(nTextX, aRect.Top() + (aRect.GetHeight() - pDev->GetTextHeight()) / 2),
                   maText);
}

void VcsStatusBarControl::Click()
{
    if (mxPopover && mxPopover->IsShown())
    {
        mxPopover->Close(); // a second click puts it away
        return;
    }
    mxPopover.reset();

    const auto xRepository = CurrentRepository();
    if (!xRepository.is())
        return;
    // the popup of a system is the system's own; one without is not an error
    if (xRepository->getKind() != "git")
        return;

    tools::Rectangle aRect = GetStatusBar().GetItemRect(GetId());
    weld::Window* pPopupParent = weld::GetPopupParent(GetStatusBar(), aRect);
    mxPopover = std::make_unique<GitBranchesPopover>(pPopupParent, aRect, xRepository);
    mxPopover->Show();
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
