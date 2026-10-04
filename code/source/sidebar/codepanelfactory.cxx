/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "codepanelfactory.hxx"

#include "errorlistpanel.hxx"
#include "findresultspanel.hxx"
#include "folderviewpanel.hxx"
#include "gitchangespanel.hxx"
#include "outputlogpanel.hxx"
#include "solutionpanel.hxx"

#include <com/sun/star/ui/XSidebar.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/namedvaluecollection.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <sfx2/sidebar/SidebarPanelBase.hxx>
#include <vcl/weld/TransportAsXWindow.hxx>

using namespace com::sun::star;

namespace code::sidebar
{

CodePanelFactory::CodePanelFactory()
{
}

uno::Reference<ui::XUIElement> SAL_CALL CodePanelFactory::createUIElement(
    const OUString& rsResourceURL, const uno::Sequence<beans::PropertyValue>& rArguments)
{
    const comphelper::NamedValueCollection aArguments(rArguments);
    uno::Reference<frame::XFrame> xFrame(sfx2::sidebar::GetFrame(aArguments));
    uno::Reference<awt::XWindow> xParentWindow(
        aArguments.getOrDefault(u"ParentWindow"_ustr, uno::Reference<awt::XWindow>()));
    uno::Reference<ui::XSidebar> xSidebar(
        aArguments.getOrDefault(u"Sidebar"_ustr, uno::Reference<ui::XSidebar>()));

    weld::Widget* pParent(nullptr);
    if (auto* pTunnel = dynamic_cast<weld::TransportAsXWindow*>(xParentWindow.get()))
        pParent = pTunnel->getWidget();

    if (!pParent)
        throw uno::RuntimeException(
            u"CodePanelFactory::createUIElement called without ParentWindow"_ustr, nullptr);
    if (!xFrame.is())
        throw uno::RuntimeException(
            u"CodePanelFactory::createUIElement called without Frame"_ustr, nullptr);

    std::unique_ptr<PanelLayout> xControl;
    // (0, -1, -1) = "minimum 0, take all available height, expandable" --
    // sw's NavigatorPanel precedent (SwPanelFactory.cxx) for a greedy panel
    // whose content scrolls itself. With all three -1 (most format panels)
    // SidebarPanelBase::getHeightForWidth falls back to the widget's
    // preferred size as a FIXED height: for a tree/log that is the full
    // content height, so once it outgrows the deck, the deck adds its own
    // vertical scrollbar and narrows the panel by a scrollbar's width.
    ui::LayoutSize aLayoutSize(0, -1, -1);

    if (rsResourceURL.endsWith("/FolderViewPanel"))
        xControl = std::make_unique<::code::sidebar::FolderViewPanel>(pParent, xSidebar);
    else if (rsResourceURL.endsWith("/SolutionPanel"))
        xControl = std::make_unique<::code::sidebar::SolutionPanel>(pParent, xSidebar);
    else if (rsResourceURL.endsWith("/OutputPanel"))
        xControl = std::make_unique<::code::sidebar::OutputLogPanel>(pParent);
    else if (rsResourceURL.endsWith("/ErrorListPanel"))
        xControl = std::make_unique<::code::sidebar::ErrorListPanel>(pParent);

    else if (rsResourceURL.endsWith("/FindResultsPanel"))
        xControl = std::make_unique<::code::sidebar::FindResultsPanel>(pParent);
    else if (rsResourceURL.endsWith("/GitChangesPanel"))
        xControl = std::make_unique<::code::sidebar::GitChangesPanel>(pParent);

    if (xControl)
        return sfx2::sidebar::SidebarPanelBase::Create(rsResourceURL, xFrame, std::move(xControl),
                                                        aLayoutSize);
    return uno::Reference<ui::XUIElement>();
}

OUString SAL_CALL CodePanelFactory::getImplementationName()
{
    return u"com.sun.star.comp.code.sidebar.PanelFactory"_ustr;
}

sal_Bool SAL_CALL CodePanelFactory::supportsService(const OUString& rServiceName)
{
    return cppu::supportsService(this, rServiceName);
}

uno::Sequence<OUString> SAL_CALL CodePanelFactory::getSupportedServiceNames()
{
    return { u"com.sun.star.ui.UIElementFactory"_ustr };
}

} // namespace code::sidebar

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_sidebar_PanelFactory_get_implementation(uno::XComponentContext*,
                                                                 uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new ::code::sidebar::CodePanelFactory);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
