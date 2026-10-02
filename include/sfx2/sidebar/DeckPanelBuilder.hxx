/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <sfx2/dllapi.h>
#include <sfx2/sidebar/Deck.hxx>

#include <com/sun/star/uno/Reference.hxx>

#include <functional>

namespace com::sun::star::frame { class XFrame; }
namespace com::sun::star::frame { class XController; }
namespace com::sun::star::ui { class XSidebar; }

class SfxBindings;
class DockingWindow;

namespace sfx2::sidebar {

class Context;
class ResourceManager;

/** Builds a Deck and its Panels from a ResourceManager's configuration
    data (Sidebar.xcu's DeckList/PanelList), the same way
    SidebarController does for the real, left/right-docked Sidebar --
    factored out so any other deck+tabbar host (e.g. the "Code" app's
    bottom-only Output dock, see code::sidebar::OutputController) can
    reuse the exact same construction logic instead of re-implementing
    it, which would otherwise be impossible: DeckDescriptor/
    PanelDescriptor are sfx2-private types and Panel's constructor is
    not even exported (SAL_DLLPRIVATE), so only code compiled as part
    of sfx2 itself can build one.
*/
class SFX2_DLLPUBLIC DeckPanelBuilder
{
public:
    /** Return the Deck for rsDeckId, creating (or re-creating, if
        bForceCreate) it if necessary. Returns an empty VclPtr if
        rsDeckId is not known to rResourceManager.
    */
    static VclPtr<Deck> CreateDeck(
        ResourceManager& rResourceManager,
        std::u16string_view rsDeckId,
        DockingWindow* pParentWindow,
        const std::function<void()>& rCloserAction,
        bool bForceCreate = false);

    /** Populate pDeck with the panels that match rContext for rsDeckId,
        reusing existing panel instances where possible and creating
        the rest by resolving their configured ImplementationURL
        through the normal UNO XUIElementFactory machinery (the same
        one a Sidebar.xcu PanelList entry's factory -- e.g.
        code::sidebar::CodePanelFactory -- is registered under).

        rxSidebar and pBindings are passed through, unmodified, as the
        "Sidebar"/"SfxBindings" UNO creation arguments some panel
        implementations expect; pass an empty Reference/nullptr when
        the host has neither (e.g. does not implement XSidebar).
    */
    static void CreatePanels(
        ResourceManager& rResourceManager,
        std::u16string_view rsDeckId,
        const Context& rContext,
        const VclPtr<Deck>& pDeck,
        const css::uno::Reference<css::frame::XFrame>& rxFrame,
        const css::uno::Reference<css::frame::XController>& rxController,
        bool bIsDocumentReadOnly,
        const std::function<Context()>& rContextAccess,
        const css::uno::Reference<css::ui::XSidebar>& rxSidebar,
        SfxBindings* pBindings);
};

} // end of namespace sfx2::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
