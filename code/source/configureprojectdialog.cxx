/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "configureprojectdialog.hxx"

#include <com/sun/star/awt/XWindow.hpp>
#include <com/sun/star/code/project/XProjectConfigurationPanel.hpp>
#include <com/sun/star/lang/XMultiComponentFactory.hpp>
#include <com/sun/star/ui/dialogs/ExecutableDialogResults.hpp>
#include <comphelper/propertyvalue.hxx>
#include <comphelper/namedvaluecollection.hxx>
#include <sal/log.hxx>
#include <vcl/svapp.hxx>
#include <vcl/vclenum.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/Container.hxx>
#include <vcl/weld/Dialog.hxx>
#include <vcl/weld/MessageDialog.hxx>
#include <vcl/weld/TransportAsXWindow.hxx>

using namespace css;
using namespace css::code::project;

namespace code
{
ConfigureProjectDialog::ConfigureProjectDialog(uno::Reference<uno::XComponentContext> xContext,
                                               weld::Window* pParent, OUString aPanelService,
                                               OUString aProjectUrl,
                                               uno::Sequence<ProjectConfiguration> aConfigurations)
    : m_xContext(std::move(xContext))
    , m_pParent(pParent)
    , m_aPanelService(std::move(aPanelService))
    , m_aProjectUrl(std::move(aProjectUrl))
    , m_aConfigurations(std::move(aConfigurations))
{
}

sal_Int16 ConfigureProjectDialog::execute()
{
    SolarMutexGuard aGuard;

    std::unique_ptr<weld::Builder> xBuilder(
        Application::CreateBuilder(m_pParent, u"code/ui/configureprojectdialog.ui"_ustr));
    std::unique_ptr<weld::Dialog> xDialog(xBuilder->weld_dialog(u"ConfigureProjectDialog"_ustr));
    std::unique_ptr<weld::Container> xContent(xBuilder->weld_container(u"content"_ustr));
    if (!m_aTitle.isEmpty())
        xDialog->set_title(m_aTitle);

    // The panel is the format's: built, by service name, into the content area.
    uno::Reference<XProjectConfigurationPanel> xPanel;
    {
        uno::Reference<awt::XWindow> xHost(new weld::TransportAsXWindow(xContent.get()));
        uno::Sequence<uno::Any> aArguments{
            uno::Any(comphelper::makePropertyValue(u"ParentWindow"_ustr, xHost)),
            uno::Any(comphelper::makePropertyValue(u"ProjectUrl"_ustr, m_aProjectUrl)),
            uno::Any(comphelper::makePropertyValue(u"Configurations"_ustr, m_aConfigurations)),
        };
        // The panel service reads its arguments as named values.
        uno::Sequence<uno::Any> aNamed(3);
        aNamed.getArray()[0] <<= comphelper::makePropertyValue(u"ParentWindow"_ustr, xHost);
        aNamed.getArray()[1] <<= comphelper::makePropertyValue(u"ProjectUrl"_ustr, m_aProjectUrl);
        aNamed.getArray()[2] <<= comphelper::makePropertyValue(u"Configurations"_ustr, m_aConfigurations);
        xPanel.set(m_xContext->getServiceManager()->createInstanceWithArgumentsAndContext(
                       m_aPanelService, aNamed, m_xContext),
                   uno::UNO_QUERY);
    }
    if (!xPanel.is())
        throw uno::RuntimeException("no configuration panel: " + m_aPanelService);

    for (;;)
    {
        if (xDialog->run() != RET_OK)
            return ui::dialogs::ExecutableDialogResults::CANCEL;

        // The panel says what is wrong; the dialog stays up until it is right
        // or the user gives up.
        const OUString aProblem = xPanel->validate();
        if (aProblem.isEmpty())
            break;
        std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(
            xDialog.get(), VclMessageType::Warning, VclButtonsType::Ok, aProblem));
        xBox->run();
    }

    m_aConfigurations = xPanel->getConfigurations();
    return ui::dialogs::ExecutableDialogResults::OK;
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
