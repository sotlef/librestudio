/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/ui/XUIElementFactory.hpp>
#include <comphelper/compbase.hxx>

namespace code::sidebar
{

// The Sidebar's ResourceManager instantiates a panel's content dynamically,
// by UNO service name, at the moment the user actually shows that panel --
// unlike code::DocShell (constructed directly in codeapp.cxx, never
// UNO-discovered), this factory must be a real, dlopen()able UNO component.
// Modelled directly on svx::sidebar::PanelFactory
// (svx/source/sidebar/PanelFactory.cxx), reduced to the one panel "Code"
// has today; adding a second one later is a single extra "endsWith" branch.
class CodePanelFactory final
    : public comphelper::WeakComponentImplHelper<css::ui::XUIElementFactory, css::lang::XServiceInfo>
{
public:
    CodePanelFactory();

    // XUIElementFactory
    css::uno::Reference<css::ui::XUIElement> SAL_CALL createUIElement(
        const OUString& rsResourceURL,
        const css::uno::Sequence<css::beans::PropertyValue>& rArguments) override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override;
    sal_Bool SAL_CALL supportsService(const OUString& rServiceName) override;
    css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;
};

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
