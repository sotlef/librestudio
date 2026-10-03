/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/code/project/ProjectConfiguration.hpp>
#include <com/sun/star/ui/dialogs/XExecutableDialog.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <cppuhelper/implbase.hxx>

namespace weld { class Window; }

namespace code
{
/// The shell's generic way of showing a project-format's configuration panel:
/// a modal dialog (XExecutableDialog) with the panel built into its content
/// area.  The panel is the format's own service; this dialog knows only the
/// XProjectConfigurationPanel contract, so one dialog serves every format.
class ConfigureProjectDialog final : public cppu::WeakImplHelper<css::ui::dialogs::XExecutableDialog>
{
public:
    ConfigureProjectDialog(css::uno::Reference<css::uno::XComponentContext> xContext,
                           weld::Window* pParent, OUString aPanelService, OUString aProjectUrl,
                           css::uno::Sequence<css::code::project::ProjectConfiguration> aConfigurations);

    // XExecutableDialog
    void SAL_CALL setTitle(const OUString& rTitle) override { m_aTitle = rTitle; }
    sal_Int16 SAL_CALL execute() override;

    /// what the user chose; meaningful after execute() returned OK
    const css::uno::Sequence<css::code::project::ProjectConfiguration>& GetConfigurations() const
    {
        return m_aConfigurations;
    }

private:
    css::uno::Reference<css::uno::XComponentContext> m_xContext;
    weld::Window* m_pParent;
    OUString m_aPanelService;
    OUString m_aProjectUrl;
    OUString m_aTitle;
    css::uno::Sequence<css::code::project::ProjectConfiguration> m_aConfigurations;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
