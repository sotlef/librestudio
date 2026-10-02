/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "outputcontroller.hxx"

#include <sfx2/sidebar/Deck.hxx>
#include <sfx2/sidebar/DeckPanelBuilder.hxx>
#include <sfx2/sidebar/TabBar.hxx>

#include <vcl/dockwin.hxx>
#include <vcl/EnumContext.hxx>

using namespace css;

namespace code::sidebar
{

OutputController::OutputController(DockingWindow* pParentWindow, SfxBindings& rBindings,
                                    uno::Reference<frame::XFrame> xFrame)
    : mpParentWindow(pParentWindow)
    , mrBindings(rBindings)
    , mxFrame(std::move(xFrame))
    , maContext(u"com.sun.star.code.CodeDocument"_ustr,
                vcl::EnumContext::GetContextName(vcl::EnumContext::Context::Default))
{
    maResourceManager.InitDeckContext(maContext);

    mxDeck = sfx2::sidebar::DeckPanelBuilder::CreateDeck(
        maResourceManager, u"OutputDeck", mpParentWindow, [] {});

    sfx2::sidebar::DeckPanelBuilder::CreatePanels(
        maResourceManager, u"OutputDeck", maContext, mxDeck, mxFrame,
        mxFrame.is() ? mxFrame->getController() : uno::Reference<frame::XController>(),
        /*bIsDocumentReadOnly=*/false, [this] { return maContext; },
        uno::Reference<ui::XSidebar>(), &mrBindings);

    // No deck-switching menu wiring: this dock only ever shows one deck, so
    // the "..." popup menu's dock/undock/customization entries (see
    // sfx2::sidebar::SidebarController::ConnectMenuActivateHandlers) have
    // nothing to do yet -- future work if a second deck/tab is ever added.
    mxTabBar = VclPtr<sfx2::sidebar::TabBar>::Create(
        mpParentWindow, mxFrame, [](const OUString&) {}, [](weld::Menu&, weld::Menu&) {}, *this);

    sfx2::sidebar::ResourceManager::DeckContextDescriptorContainer aDecks;
    sfx2::sidebar::ResourceManager::DeckContextDescriptor aOutputDeck;
    aOutputDeck.msId = u"OutputDeck"_ustr;
    aOutputDeck.mbIsEnabled = true;
    aDecks.push_back(aOutputDeck);
    mxTabBar->SetDecks(aDecks);
    mxTabBar->HighlightDeck(u"OutputDeck");

    mxDeck->Show();
    mxTabBar->Show();

    NotifyResize();
}

OutputController::~OutputController()
{
    mxTabBar.disposeAndClear();
    mxDeck.disposeAndClear();
}

bool OutputController::IsDocked() const
{
    return !mpParentWindow->IsFloatingMode();
}

void OutputController::NotifyResize()
{
    if (!mxTabBar || !mxDeck)
        return;

    const sal_Int32 nTabBarWidth = sfx2::sidebar::TabBar::GetDefaultWidth();
    const Size aParentSize(mpParentWindow->GetSizePixel());

    // Same edge-strip-plus-filled-rest geometry SidebarController itself
    // uses for a left-docked Sidebar (see SidebarController::NotifyResize),
    // just inside a wide-and-short rectangle instead of a narrow-and-tall
    // one: TabBar hugs one edge (here: left) at its natural width, Deck
    // fills the remainder.
    mxTabBar->setPosSizePixel(0, 0, nTabBarWidth, aParentSize.Height());
    mxDeck->setPosSizePixel(nTabBarWidth, 0, aParentSize.Width() - nTabBarWidth, aParentSize.Height());

    // Deck has no Resize() hook of its own -- SidebarController::NotifyResize
    // follows its setPosSizePixel with an explicit RequestLayout(), and so
    // must this. Without it the deck never re-runs DeckLayouter at its real
    // size, so size-dependent decisions stay at their construction-time
    // defaults -- concretely, the lone OutputPanel's optional title bar
    // ("Build Output" + expander) is only hidden inside LayoutDeck, and
    // gtk3 masked the gap by accident: its post-startup DataChanged events
    // also trigger a relayout, qt6 sends none.
    mxDeck->RequestLayout();
}

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
