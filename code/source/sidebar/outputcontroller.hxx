/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sfx2/sidebar/Context.hxx>
#include <sfx2/sidebar/IDeckHost.hxx>
#include <sfx2/sidebar/ResourceManager.hxx>

#include <com/sun/star/frame/XFrame.hpp>

#include <vcl/vclptr.hxx>

class DockingWindow;
class SfxBindings;

namespace sfx2::sidebar { class Deck; }
namespace sfx2::sidebar { class TabBar; }

namespace code::sidebar
{

/** Owns the decks of the "Code" app's bottom-only Output dock
    ("OutputDeck" and "ErrorListDeck", Sidebar.xcu; one visible at a
    time, switched via the TabBar), built through
    sfx2::sidebar::DeckPanelBuilder -- the same Deck/Panel construction
    machinery the real, left/right Sidebar is itself built on (see
    sfx2::sidebar::SidebarController) -- just hosted by our own
    OutputDockingWindow instead of a SidebarDockingWindow.

    Deliberately simpler than SidebarController in one respect: it never
    calls ResourceManager::GetMatchingDecks() to discover "which decks
    match the current context" the way the real Sidebar does, because
    that would also match every other deck whose ContextList says "any"
    (SolutionDeck, PropertyDeck, ...) and add them as extra tabs here --
    this dock only ever shows the decks it was built for, so its own
    TabBar is fed a hand-built DeckContextDescriptor list instead.
*/
class OutputController final : public sfx2::sidebar::IDeckHost
{
public:
    OutputController(DockingWindow* pParentWindow, SfxBindings& rBindings,
                      css::uno::Reference<css::frame::XFrame> xFrame);
    ~OutputController();

    // IDeckHost
    virtual sfx2::sidebar::ResourceManager* GetResourceManager() override
    {
        return &maResourceManager;
    }
    virtual const sfx2::sidebar::Context& GetCurrentContext() const override { return maContext; }
    virtual bool IsDocked() const override;

    /// Re-layout the TabBar+Deck for the parent docking window's current size.
    void NotifyResize();

private:
    /** Show rsDeckId (creating it through DeckPanelBuilder on first use,
        which caches it in maResourceManager's DeckDescriptor), hiding the
        previously shown deck. */
    void SwitchToDeck(std::u16string_view rsDeckId);

    DockingWindow* mpParentWindow;
    SfxBindings& mrBindings;
    css::uno::Reference<css::frame::XFrame> mxFrame;
    sfx2::sidebar::Context maContext;
    sfx2::sidebar::ResourceManager maResourceManager;
    VclPtr<sfx2::sidebar::TabBar> mxTabBar;
    VclPtr<sfx2::sidebar::Deck> mxDeck;
};

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
