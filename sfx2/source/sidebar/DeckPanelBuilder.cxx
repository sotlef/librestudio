/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sfx2/sidebar/DeckPanelBuilder.hxx>

#include <sfx2/sidebar/Context.hxx>
#include <sfx2/sidebar/Deck.hxx>
#include <sidebar/DeckDescriptor.hxx>
#include <sfx2/sidebar/Panel.hxx>
#include <sidebar/PanelDescriptor.hxx>
#include <sidebar/PanelTitleBar.hxx>
#include <sfx2/sidebar/ResourceManager.hxx>
#include <sfx2/sidebar/Theme.hxx>
#include <sfx2/bindings.hxx>
#include <sidebar/Tools.hxx>

#include <com/sun/star/awt/XWindow.hpp>
#include <com/sun/star/frame/XController.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/rendering/XSpriteCanvas.hpp>
#include <com/sun/star/ui/XSidebar.hpp>
#include <com/sun/star/ui/XUIElement.hpp>
#include <com/sun/star/ui/XUIElementFactory.hpp>
#include <com/sun/star/ui/theUIElementFactoryManager.hpp>

#include <comphelper/diagnose_ex.hxx>
#include <comphelper/namedvaluecollection.hxx>
#include <comphelper/processfactory.hxx>
#include <toolkit/helper/vclunohelper.hxx>
#include <vcl/dockwin.hxx>
#include <vcl/outdev.hxx>

using namespace css;
using namespace css::uno;

namespace sfx2::sidebar {

namespace {

Reference<ui::XUIElement> CreateUIElement(
    const Reference<awt::XWindow>& rxWindow,
    const OUString& rsImplementationURL,
    const bool bWantsCanvas,
    const Context& rContext,
    const Reference<frame::XFrame>& rxFrame,
    const Reference<frame::XController>& rxCurrentController,
    const Reference<ui::XSidebar>& rxSidebar,
    SfxBindings* pBindings)
{
    try
    {
        const Reference<XComponentContext>& xComponentContext(comphelper::getProcessComponentContext());
        const Reference<ui::XUIElementFactory> xUIElementFactory
            = ui::theUIElementFactoryManager::get(xComponentContext);

        comphelper::NamedValueCollection aCreationArguments;
        aCreationArguments.put(u"Frame"_ustr, Any(rxFrame));
        aCreationArguments.put(u"ParentWindow"_ustr, Any(rxWindow));
        if (pBindings != nullptr)
            aCreationArguments.put(u"SfxBindings"_ustr, Any(reinterpret_cast<sal_uInt64>(pBindings)));
        aCreationArguments.put(u"Theme"_ustr, Theme::GetPropertySet());
        if (rxSidebar.is())
            aCreationArguments.put(u"Sidebar"_ustr, Any(rxSidebar));
        if (bWantsCanvas)
        {
            Reference<rendering::XSpriteCanvas> xCanvas(
                VCLUnoHelper::GetWindow(rxWindow)->GetOutDev()->GetSpriteCanvas());
            aCreationArguments.put(u"Canvas"_ustr, Any(xCanvas));
        }

        if (rxCurrentController.is())
        {
            OUString aModule = Tools::GetModuleName(rxCurrentController);
            if (!aModule.isEmpty())
                aCreationArguments.put(u"Module"_ustr, Any(aModule));
            aCreationArguments.put(u"Controller"_ustr, Any(rxCurrentController));
        }

        aCreationArguments.put(u"ApplicationName"_ustr, Any(rContext.msApplication));
        aCreationArguments.put(u"ContextName"_ustr, Any(rContext.msContext));

        Reference<ui::XUIElement> xUIElement(
            xUIElementFactory->createUIElement(rsImplementationURL, aCreationArguments.getPropertyValues()),
            UNO_SET_THROW);

        return xUIElement;
    }
    catch (const Exception&)
    {
        TOOLS_WARN_EXCEPTION("sfx.sidebar", "Cannot create panel " << rsImplementationURL);
        return nullptr;
    }
}

std::shared_ptr<Panel> CreatePanel(
    ResourceManager& rResourceManager,
    std::u16string_view rsPanelId,
    weld::Widget* pParentWindow,
    const bool bIsInitiallyExpanded,
    const Context& rContext,
    const VclPtr<Deck>& pDeck,
    const Reference<frame::XFrame>& rxFrame,
    const Reference<frame::XController>& rxCurrentController,
    const std::function<Context()>& rContextAccess,
    const Reference<ui::XSidebar>& rxSidebar,
    SfxBindings* pBindings)
{
    std::shared_ptr<PanelDescriptor> xPanelDescriptor = rResourceManager.GetPanelDescriptor(rsPanelId);
    if (!xPanelDescriptor)
        return nullptr;

    auto xPanel = std::make_shared<Panel>(*xPanelDescriptor, pParentWindow, bIsInitiallyExpanded, pDeck.get(),
                                           rContextAccess, rxFrame);

    Reference<ui::XUIElement> xUIElement(CreateUIElement(
        xPanel->GetElementParentWindow(), xPanelDescriptor->msImplementationURL, xPanelDescriptor->mbWantsCanvas,
        rContext, rxFrame, rxCurrentController, rxSidebar, pBindings));
    if (xUIElement.is())
        xPanel->SetUIElement(xUIElement);
    else
        xPanel.reset();

    return xPanel;
}

} // end anonymous namespace

VclPtr<Deck> DeckPanelBuilder::CreateDeck(ResourceManager& rResourceManager, std::u16string_view rsDeckId,
                                          DockingWindow* pParentWindow, const std::function<void()>& rCloserAction,
                                          bool bForceCreate)
{
    std::shared_ptr<DeckDescriptor> xDeckDescriptor = rResourceManager.GetDeckDescriptor(rsDeckId);
    if (!xDeckDescriptor)
        return VclPtr<Deck>();

    VclPtr<Deck> aDeck = xDeckDescriptor->mpDeck;
    if (!aDeck || bForceCreate)
    {
        if (aDeck)
            aDeck.disposeAndClear();

        aDeck = VclPtr<Deck>::Create(*xDeckDescriptor, pParentWindow, rCloserAction);
    }
    xDeckDescriptor->mpDeck = aDeck;
    return aDeck;
}

void DeckPanelBuilder::CreatePanels(
    ResourceManager& rResourceManager, std::u16string_view rsDeckId, const Context& rContext,
    const VclPtr<Deck>& pDeck, const Reference<frame::XFrame>& rxFrame,
    const Reference<frame::XController>& rxController, bool bIsDocumentReadOnly,
    const std::function<Context()>& rContextAccess, const Reference<ui::XSidebar>& rxSidebar, SfxBindings* pBindings)
{
    ResourceManager::PanelContextDescriptorContainer aPanelContextDescriptors;
    rResourceManager.GetMatchingPanels(aPanelContextDescriptors, rContext, rsDeckId, rxController);

    const sal_Int32 nNewPanelCount(aPanelContextDescriptors.size());
    SharedPanelContainer aNewPanels;
    sal_Int32 nWriteIndex(0);
    aNewPanels.resize(nNewPanelCount);

    for (sal_Int32 nReadIndex = 0; nReadIndex < nNewPanelCount; ++nReadIndex)
    {
        const ResourceManager::PanelContextDescriptor& rPanelContexDescriptor(aPanelContextDescriptors[nReadIndex]);

        const bool bIsPanelVisible(!bIsDocumentReadOnly || rPanelContexDescriptor.mbShowForReadOnlyDocuments);
        if (!bIsPanelVisible)
            continue;

        auto xOldPanel(pDeck->GetPanel(rPanelContexDescriptor.msId));
        if (xOldPanel)
        {
            xOldPanel->SetLurkMode(false);
            aNewPanels[nWriteIndex] = xOldPanel;
            xOldPanel->SetExpanded(rPanelContexDescriptor.mbIsInitiallyVisible);
            ++nWriteIndex;
        }
        else
        {
            auto aPanel = CreatePanel(rResourceManager, rPanelContexDescriptor.msId, pDeck->GetPanelParentWindow(),
                                      rPanelContexDescriptor.mbIsInitiallyVisible, rContext, pDeck, rxFrame,
                                      rxController, rContextAccess, rxSidebar, pBindings);
            if (aPanel)
            {
                aNewPanels[nWriteIndex] = std::move(aPanel);

                PanelTitleBar* pTitleBar = aNewPanels[nWriteIndex]->GetTitleBar();
                if (pTitleBar)
                    pTitleBar->SetMoreOptionsCommand(rPanelContexDescriptor.msMenuCommand, rxFrame, rxController);
                ++nWriteIndex;
            }
        }
    }

    aNewPanels.resize(nWriteIndex);
    pDeck->ResetPanels(std::move(aNewPanels));
}

} // end of namespace sfx2::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
