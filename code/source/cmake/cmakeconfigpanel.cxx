/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// The panel in which a CMake project is configured: which configurations
// (Debug, Release, ...) it has, and for each its build type and build
// directory.  A UNO service of its own (com.sun.star.code.CMakeConfigurationPanel,
// named by CMakeFormatReader), built into whatever window it is given; it knows
// nothing of dialogs.

#include <com/sun/star/awt/XWindow.hpp>
#include <com/sun/star/code/project/ProjectConfiguration.hpp>
#include <com/sun/star/code/project/XProjectConfigurationPanel.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/namedvaluecollection.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/Button.hxx>
#include <vcl/weld/ComboBox.hxx>
#include <vcl/weld/Container.hxx>
#include <vcl/weld/Entry.hxx>
#include <vcl/weld/TransportAsXWindow.hxx>

#include <vector>

using namespace css;
using namespace css::code::project;

namespace
{
constexpr OUString BuildType = u"BuildType"_ustr;
constexpr OUString BuildDirectory = u"BuildDirectory"_ustr;

OUString PropertyOf(const ProjectConfiguration& rConfiguration, const OUString& rName)
{
    for (const beans::StringPair& rProperty : rConfiguration.Properties)
        if (rProperty.First == rName)
            return rProperty.Second;
    return {};
}

class CMakeConfigurationPanel final
    : public cppu::WeakImplHelper<XProjectConfigurationPanel, lang::XInitialization,
                                  lang::XServiceInfo>
{
public:
    // XInitialization
    void SAL_CALL initialize(const uno::Sequence<uno::Any>& rArguments) override;

    // XProjectConfigurationPanel
    uno::Sequence<ProjectConfiguration> SAL_CALL getConfigurations() override;
    OUString SAL_CALL validate() override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.CMakeConfigurationPanel"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override
    {
        return { u"com.sun.star.code.CMakeConfigurationPanel"_ustr };
    }

    ~CMakeConfigurationPanel() override
    {
        // the widgets are VCL's, and die under the solar mutex
        SolarMutexGuard aGuard;
        m_xAdd.reset();
        m_xRemove.reset();
        m_xName.reset();
        m_xBuildType.reset();
        m_xBuildDir.reset();
        m_xConfigs.reset();
        m_xRoot.reset();
        m_xBuilder.reset();
    }

private:
    // the entries show one configuration; this stores them back into it
    void StoreCurrent();
    void ShowCurrent();
    void RefillList(int nSelect);

    DECL_LINK(SelectHdl, weld::ComboBox&, void);
    DECL_LINK(AddHdl, weld::Button&, void);
    DECL_LINK(RemoveHdl, weld::Button&, void);

    std::unique_ptr<weld::Builder> m_xBuilder;
    std::unique_ptr<weld::Container> m_xRoot;
    std::unique_ptr<weld::ComboBox> m_xConfigs;
    std::unique_ptr<weld::Button> m_xAdd;
    std::unique_ptr<weld::Button> m_xRemove;
    std::unique_ptr<weld::Entry> m_xName;
    std::unique_ptr<weld::Entry> m_xBuildType;
    std::unique_ptr<weld::Entry> m_xBuildDir;

    std::vector<ProjectConfiguration> m_aConfigurations;
    int m_nCurrent = -1; // the one the entries show
    OUString m_aProjectUrl;
};

void CMakeConfigurationPanel::initialize(const uno::Sequence<uno::Any>& rArguments)
{
    SolarMutexGuard aGuard;
    const comphelper::NamedValueCollection aArguments(rArguments);
    uno::Reference<awt::XWindow> xParent(aArguments.getOrDefault(u"ParentWindow"_ustr, uno::Reference<awt::XWindow>()));
    auto* pTunnel = dynamic_cast<weld::TransportAsXWindow*>(xParent.get());
    if (!pTunnel || !pTunnel->getWidget())
        throw uno::RuntimeException(u"CMakeConfigurationPanel needs a ParentWindow"_ustr, getXWeak());

    aArguments.get(u"ProjectUrl"_ustr) >>= m_aProjectUrl;
    uno::Sequence<ProjectConfiguration> aConfigurations;
    aArguments.get(u"Configurations"_ustr) >>= aConfigurations;
    m_aConfigurations.assign(aConfigurations.begin(), aConfigurations.end());

    m_xBuilder = Application::CreateBuilder(pTunnel->getWidget(), u"code/ui/cmakeconfigpanel.ui"_ustr);
    m_xRoot = m_xBuilder->weld_container(u"CMakeConfigPanel"_ustr);
    m_xConfigs = m_xBuilder->weld_combo_box(u"configs"_ustr);
    m_xAdd = m_xBuilder->weld_button(u"add"_ustr);
    m_xRemove = m_xBuilder->weld_button(u"remove"_ustr);
    m_xName = m_xBuilder->weld_entry(u"name"_ustr);
    m_xBuildType = m_xBuilder->weld_entry(u"buildtype"_ustr);
    m_xBuildDir = m_xBuilder->weld_entry(u"builddir"_ustr);

    m_xConfigs->connect_changed(LINK(this, CMakeConfigurationPanel, SelectHdl));
    m_xAdd->connect_clicked(LINK(this, CMakeConfigurationPanel, AddHdl));
    m_xRemove->connect_clicked(LINK(this, CMakeConfigurationPanel, RemoveHdl));

    RefillList(m_aConfigurations.empty() ? -1 : 0);
}

void CMakeConfigurationPanel::RefillList(int nSelect)
{
    m_nCurrent = -1; // the entries are about to change under us, not edits
    m_xConfigs->clear();
    for (const ProjectConfiguration& rConfiguration : m_aConfigurations)
        m_xConfigs->append_text(rConfiguration.Name);
    if (nSelect >= 0 && nSelect < static_cast<int>(m_aConfigurations.size()))
        m_xConfigs->set_active(nSelect);
    m_nCurrent = nSelect;
    ShowCurrent();
    m_xRemove->set_sensitive(m_aConfigurations.size() > 1);
}

void CMakeConfigurationPanel::ShowCurrent()
{
    const bool bHave = m_nCurrent >= 0;
    m_xName->set_sensitive(bHave);
    m_xBuildType->set_sensitive(bHave);
    m_xBuildDir->set_sensitive(bHave);
    if (!bHave)
    {
        m_xName->set_text(OUString());
        m_xBuildType->set_text(OUString());
        m_xBuildDir->set_text(OUString());
        return;
    }
    const ProjectConfiguration& rConfiguration = m_aConfigurations[m_nCurrent];
    m_xName->set_text(rConfiguration.Name);
    m_xBuildType->set_text(PropertyOf(rConfiguration, BuildType));
    m_xBuildDir->set_text(PropertyOf(rConfiguration, BuildDirectory));
}

void CMakeConfigurationPanel::StoreCurrent()
{
    if (m_nCurrent < 0 || m_nCurrent >= static_cast<int>(m_aConfigurations.size()))
        return;
    ProjectConfiguration& rConfiguration = m_aConfigurations[m_nCurrent];
    rConfiguration.Name = m_xName->get_text();
    rConfiguration.Properties = { { BuildType, m_xBuildType->get_text() },
                                  { BuildDirectory, m_xBuildDir->get_text() } };
}

IMPL_LINK_NOARG(CMakeConfigurationPanel, SelectHdl, weld::ComboBox&, void)
{
    StoreCurrent();
    const int nSelected = m_xConfigs->get_active();
    // storing may have renamed the configuration being left
    RefillList(nSelected);
}

IMPL_LINK_NOARG(CMakeConfigurationPanel, AddHdl, weld::Button&, void)
{
    StoreCurrent();
    ProjectConfiguration aNew;
    aNew.Name = u"Configuration "_ustr + OUString::number(m_aConfigurations.size() + 1);
    aNew.Properties = { { BuildType, u"Debug"_ustr },
                        { BuildDirectory, OUString() } };
    m_aConfigurations.push_back(std::move(aNew));
    RefillList(static_cast<int>(m_aConfigurations.size()) - 1);
    m_xName->grab_focus();
}

IMPL_LINK_NOARG(CMakeConfigurationPanel, RemoveHdl, weld::Button&, void)
{
    if (m_nCurrent < 0 || m_aConfigurations.size() <= 1)
        return;
    m_aConfigurations.erase(m_aConfigurations.begin() + m_nCurrent);
    RefillList(std::min(m_nCurrent, static_cast<int>(m_aConfigurations.size()) - 1));
}

uno::Sequence<ProjectConfiguration> CMakeConfigurationPanel::getConfigurations()
{
    SolarMutexGuard aGuard;
    StoreCurrent();
    return comphelper::containerToSequence(m_aConfigurations);
}

OUString CMakeConfigurationPanel::validate()
{
    SolarMutexGuard aGuard;
    StoreCurrent();
    if (m_aConfigurations.empty())
        return u"Add at least one configuration."_ustr;
    for (size_t i = 0; i < m_aConfigurations.size(); ++i)
    {
        const ProjectConfiguration& rConfiguration = m_aConfigurations[i];
        if (rConfiguration.Name.trim().isEmpty())
            return u"Every configuration needs a name."_ustr;
        for (size_t j = 0; j < i; ++j)
            if (m_aConfigurations[j].Name == rConfiguration.Name)
                return u"Two configurations are named \""_ustr + rConfiguration.Name + "\".";
        if (PropertyOf(rConfiguration, BuildDirectory).trim().isEmpty())
            return u"The configuration \""_ustr + rConfiguration.Name + "\" needs a build directory.";
    }
    return {};
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_CMakeConfigurationPanel_get_implementation(uno::XComponentContext*,
                                                                   uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new CMakeConfigurationPanel);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
