/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "configurationcontrol.hxx"

#include "uibridge.hxx"

#include <com/sun/star/code/project/ProjectConfiguration.hpp>
#include <com/sun/star/code/project/XProject.hpp>
#include <com/sun/star/code/project/XProjectManager.hpp>
#include <com/sun/star/code/project/theProjectManager.hpp>
#include <comphelper/processfactory.hxx>
#include <sal/log.hxx>
#include <svl/voiditem.hxx>
#include <algorithm>
#include <vcl/InterimItemWindow.hxx>
#include <vcl/toolbox.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/ComboBox.hxx>
#include <vcl/weld/Container.hxx>

using namespace css;
using namespace css::code::project;

namespace code
{
namespace
{
// The combo box, built from configurationbox.ui, and the listener that keeps
// it in step with the project manager.
class ConfigurationBox final : public InterimItemWindow
{
public:
    ConfigurationBox(vcl::Window* pParent, ToolBox* pToolBox, ToolBoxItemId nItemId)
        : InterimItemWindow(pParent, u"code/ui/configurationbox.ui"_ustr, u"ConfigurationBox"_ustr)
        , m_xCombo(m_xBuilder->weld_combo_box(u"configuration"_ustr))
        , m_pToolBox(pToolBox)
        , m_nItemId(nItemId)
    {
        m_xCombo->connect_changed(LINK(this, ConfigurationBox, SelectHdl));
        // The toolbar lays out its items once, from the size set here, so the
        // size has to be the one the box will have WITH entries in it: an empty
        // combo box asks for 30 pixels, one showing text for 44, and a window
        // sized while it was empty cut off the bottom of the box.  A placeholder
        // entry is put in for the measurement.  The window is also at least as
        // tall as a toolbar button.
        m_xCombo->append_text(u"Release"_ustr);
        m_xCombo->set_active(0);
        Size aSize = m_xCombo->get_preferred_size();
        m_xCombo->clear();
        // clearing leaves the placeholder's text showing; nothing is selected
        m_xCombo->set_active(-1);
        if (auto* pToolBox = dynamic_cast<ToolBox*>(pParent))
            aSize.setHeight(std::max(aSize.Height(), pToolBox->GetDefaultImageSize().Height()));
        SetSizePixel(aSize);

        // Projects open, close, are configured and switched on any thread.
        m_xNotifier = std::make_shared<UiNotifier>([this] { Refresh(); });
        m_xListener = new ServiceListener(m_xNotifier);
        try
        {
            theProjectManager::get(comphelper::getProcessComponentContext())->addProjectListener(m_xListener);
        }
        catch (const uno::Exception& rException)
        {
            SAL_WARN("code", "the configuration box cannot follow the projects: " << rException.Message);
        }
        Refresh();
    }

    virtual ~ConfigurationBox() override { disposeOnce(); }

    virtual void dispose() override
    {
        if (m_xNotifier)
            m_xNotifier->Detach();
        try
        {
            if (m_xListener.is())
                theProjectManager::get(comphelper::getProcessComponentContext())
                    ->removeProjectListener(m_xListener);
        }
        catch (const uno::Exception&)
        {
        }
        m_pToolBox = nullptr;
        m_xCombo.reset();
        InterimItemWindow::dispose();
    }

    virtual void GetFocus() override
    {
        if (m_xCombo)
            m_xCombo->grab_focus();
        InterimItemWindow::GetFocus();
    }

private:
    // The active project's configurations, the current one selected; nothing
    // to choose from (and so disabled) when there are none.
    void Refresh()
    {
        if (!m_xCombo)
            return;
        m_bRefreshing = true;
        m_xCombo->clear();
        m_xCombo->set_active(-1); // no project, no configuration: show nothing
        m_aProjectUrl.clear();
        try
        {
            auto xManager = theProjectManager::get(comphelper::getProcessComponentContext());
            if (auto xProject = xManager->getActiveProject())
            {
                m_aProjectUrl = xProject->getUrl();
                for (const ProjectConfiguration& rConfiguration : xManager->getConfigurations(m_aProjectUrl))
                    m_xCombo->append_text(rConfiguration.Name);
                const OUString aCurrent = xManager->getCurrentConfiguration(m_aProjectUrl);
                const int nCurrent = m_xCombo->find_text(aCurrent);
                if (nCurrent != -1)
                    m_xCombo->set_active(nCurrent);
            }
        }
        catch (const uno::Exception& rException)
        {
            SAL_WARN("code", "cannot read the project's configurations: " << rException.Message);
        }
        m_bRefreshing = false;

        // The box is there only for a project that has configurations to
        // choose between; for one that has none (an opened folder, a format
        // that needs no configuring) the whole toolbar item is hidden, not
        // merely disabled.
        if (m_pToolBox)
            m_pToolBox->ShowItem(m_nItemId, m_xCombo->get_count() > 0);
    }

    DECL_LINK(SelectHdl, weld::ComboBox&, void);

    std::unique_ptr<weld::ComboBox> m_xCombo;
    ToolBox* m_pToolBox;
    ToolBoxItemId m_nItemId;
    OUString m_aProjectUrl;
    // set while the box is filled, so that filling it is not mistaken for a choice
    bool m_bRefreshing = false;
    std::shared_ptr<UiNotifier> m_xNotifier;
    uno::Reference<XProjectListener> m_xListener;
};

IMPL_LINK(ConfigurationBox, SelectHdl, weld::ComboBox&, rCombo, void)
{
    if (m_bRefreshing || m_aProjectUrl.isEmpty() || rCombo.get_active() < 0)
        return;
    try
    {
        theProjectManager::get(comphelper::getProcessComponentContext())
            ->setCurrentConfiguration(m_aProjectUrl, rCombo.get_active_text());
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot switch the configuration: " << rException.Message);
        Refresh(); // show what is still current
    }
}
}

SFX_IMPL_TOOLBOX_CONTROL(ConfigurationControl, SfxVoidItem);

ConfigurationControl::ConfigurationControl(sal_uInt16 nSlotId, ToolBoxItemId nId, ToolBox& rToolBox)
    : SfxToolBoxControl(nSlotId, nId, rToolBox)
{
}

ConfigurationControl::~ConfigurationControl() {}

void ConfigurationControl::StateChangedAtToolBoxControl(sal_uInt16 nSID, SfxItemState eState,
                                                        const SfxPoolItem* pState)
{
    GetToolBox().EnableItem(GetId(), eState != SfxItemState::DISABLED);
    SfxToolBoxControl::StateChangedAtToolBoxControl(nSID, eState, pState);
}

VclPtr<InterimItemWindow> ConfigurationControl::CreateItemWindow(vcl::Window* pParent)
{
    VclPtr<ConfigurationBox> xBox = VclPtr<ConfigurationBox>::Create(pParent, &GetToolBox(), GetId());
    xBox->Show();
    return xBox;
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
