/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "startupcontrol.hxx"

#include "startuptarget.hxx"
#include "uibridge.hxx"

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/code/project/XProject.hpp>
#include <com/sun/star/code/project/XProjectManager.hpp>
#include <com/sun/star/code/project/XProjectTarget.hpp>
#include <com/sun/star/code/project/XProjectTargets.hpp>
#include <com/sun/star/code/project/theProjectManager.hpp>
#include <com/sun/star/frame/XLayoutManager.hpp>
#include <comphelper/dispatchcommand.hxx>
#include <comphelper/processfactory.hxx>
#include <sal/log.hxx>
#include <sfx2/frame.hxx>
#include <sfx2/viewfrm.hxx>
#include <svl/voiditem.hxx>
#include <algorithm>
#include <vcl/InterimItemWindow.hxx>
#include <vcl/toolbox.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/Button.hxx>
#include <vcl/weld/ComboBox.hxx>
#include <vcl/weld/Container.hxx>
#include <vcl/weld/Menu.hxx>

using namespace css;
using namespace css::code::project;

namespace code
{
namespace
{
// The item window, built from startupbox.ui, and the listener that keeps it in
// step with the project manager.
class StartupBox final : public InterimItemWindow
{
public:
    StartupBox(vcl::Window* pParent, ToolBox* pToolBox, ToolBoxItemId nItemId)
        : InterimItemWindow(pParent, u"code/ui/startupbox.ui"_ustr, u"StartupBox"_ustr)
        , m_xStart(m_xBuilder->weld_button(u"start"_ustr))
        , m_xCombo(m_xBuilder->weld_combo_box(u"target"_ustr))
        , m_xMore(m_xBuilder->weld_button(u"more"_ustr))
        , m_pToolBox(pToolBox)
        , m_nItemId(nItemId)
    {
        m_xStart->set_from_icon_name(u"cmd/sc_nextrecord.png"_ustr);
        m_xStart->connect_clicked(LINK(this, StartupBox, StartHdl));
        m_xMore->connect_clicked(LINK(this, StartupBox, MoreHdl));
        m_xCombo->connect_changed(LINK(this, StartupBox, SelectHdl));
        // The toolbar lays out its items once, from the size set here, so the
        // size has to be the one the box will have WITH an entry in it (see
        // ConfigurationBox): a placeholder is put in for the measurement.
        m_xCombo->append_text(u"Release"_ustr);
        m_xCombo->set_active(0);
        Size aSize = GetOptimalSize();
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
            SAL_WARN("code", "the startup box cannot follow the projects: " << rException.Message);
        }
        Refresh();
    }

    virtual ~StartupBox() override { disposeOnce(); }

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
        m_xStart.reset();
        m_xCombo.reset();
        m_xMore.reset();
        InterimItemWindow::dispose();
    }

    virtual void GetFocus() override
    {
        if (m_xCombo)
            m_xCombo->grab_focus();
        InterimItemWindow::GetFocus();
    }

private:
    // The launchable targets of the project (the active one, else the first
    // open one), the one Start runs selected.
    void Refresh()
    {
        if (!m_xCombo)
            return;
        m_bRefreshing = true;
        m_xCombo->clear();
        m_xCombo->set_active(-1); // no project, no target: show nothing
        m_aProjectUrl.clear();
        try
        {
            const auto xContext = comphelper::getProcessComponentContext();
            auto xManager = theProjectManager::get(xContext);
            uno::Reference<XProject> xProject = xManager->getActiveProject();
            if (!xProject.is())
            {
                const auto aOpen = xManager->getOpenProjects();
                if (aOpen.hasElements())
                    xProject = aOpen[0];
            }
            uno::Reference<XProjectTargets> xTargets(xProject, uno::UNO_QUERY);
            if (xProject.is() && xTargets.is())
            {
                m_aProjectUrl = xProject->getUrl();
                for (const auto& rTarget : xTargets->getTargets())
                    if (startup::IsLaunchable(rTarget))
                        m_xCombo->append_text(rTarget->getName());
                if (const auto xStartup = startup::ResolveStartupTarget(xContext, xProject))
                {
                    const int nCurrent = m_xCombo->find_text(xStartup->getName());
                    if (nCurrent != -1)
                        m_xCombo->set_active(nCurrent);
                }
            }
        }
        catch (const uno::Exception& rException)
        {
            SAL_WARN("code", "cannot read the project's targets: " << rException.Message);
        }
        m_bRefreshing = false;

        // The item is there only while something can be started; otherwise the
        // whole toolbar item is hidden, not merely disabled.
        const bool bShow = m_xCombo->get_count() > 0;
        if (m_pToolBox && bShow != m_bShown)
        {
            m_bShown = bShow;
            m_pToolBox->ShowItem(m_nItemId, bShow);
            // See ConfigurationBox::Refresh: the item's rectangle is asked for
            // first so that the toolbox lays its items out again with this one
            // showing, then the toolbar is made as wide as its items want and
            // the frame is laid out again.
            (void)m_pToolBox->GetItemRect(m_nItemId);
            m_pToolBox->SetSizePixel(m_pToolBox->CalcWindowSizePixel());
            if (SfxViewFrame* pFrame = SfxViewFrame::Current())
            {
                uno::Reference<beans::XPropertySet> xFrame(pFrame->GetFrame().GetFrameInterface(), uno::UNO_QUERY);
                try
                {
                    uno::Reference<frame::XLayoutManager> xLayout;
                    if (xFrame.is() && (xFrame->getPropertyValue(u"LayoutManager"_ustr) >>= xLayout) && xLayout.is())
                        xLayout->doLayout();
                }
                catch (const uno::Exception&)
                {
                }
            }
        }
    }

    DECL_LINK(SelectHdl, weld::ComboBox&, void);
    DECL_LINK(StartHdl, weld::Button&, void);
    DECL_LINK(MoreHdl, weld::Button&, void);

    std::unique_ptr<weld::Button> m_xStart;
    std::unique_ptr<weld::ComboBox> m_xCombo;
    std::unique_ptr<weld::Button> m_xMore;
    ToolBox* m_pToolBox;
    // whether the toolbar item is showing; nothing has said otherwise yet
    bool m_bShown = true;
    ToolBoxItemId m_nItemId;
    OUString m_aProjectUrl;
    // set while the box is filled, so that filling it is not mistaken for a choice
    bool m_bRefreshing = false;
    std::shared_ptr<UiNotifier> m_xNotifier;
    uno::Reference<XProjectListener> m_xListener;
};

IMPL_LINK(StartupBox, SelectHdl, weld::ComboBox&, rCombo, void)
{
    if (m_bRefreshing || m_aProjectUrl.isEmpty() || rCombo.get_active() < 0)
        return;
    try
    {
        // the PROPERTIES_CHANGED event comes back and refreshes the box; that
        // finds the same choice selected, so it settles
        theProjectManager::get(comphelper::getProcessComponentContext())
            ->setStartupTarget(m_aProjectUrl, rCombo.get_active_text());
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot set the startup target: " << rException.Message);
        Refresh(); // show what is still the startup target
    }
}

IMPL_LINK_NOARG(StartupBox, StartHdl, weld::Button&, void)
{
    comphelper::dispatchCommand(u".uno:CodeStartDebugging"_ustr, {});
}

IMPL_LINK_NOARG(StartupBox, MoreHdl, weld::Button&, void)
{
    // The menu lives in startupbox.ui next to the box, and is built when asked for.
    std::unique_ptr<weld::Builder> xBuilder(
        Application::CreateBuilder(m_xMore.get(), u"code/ui/startupbox.ui"_ustr));
    std::unique_ptr<weld::Menu> xMenu = xBuilder->weld_menu(u"startmenu"_ustr);
    const OUString aCommand = xMenu->popup_at_rect(
        m_xMore.get(), tools::Rectangle(Point(0, 0), m_xMore->get_preferred_size()));
    if (aCommand == "withoutdebugging")
        comphelper::dispatchCommand(u".uno:CodeStartWithoutDebugging"_ustr, {});
}
}

SFX_IMPL_TOOLBOX_CONTROL(StartupControl, SfxVoidItem);

StartupControl::StartupControl(sal_uInt16 nSlotId, ToolBoxItemId nId, ToolBox& rToolBox)
    : SfxToolBoxControl(nSlotId, nId, rToolBox)
{
}

StartupControl::~StartupControl() {}

void StartupControl::StateChangedAtToolBoxControl(sal_uInt16 nSID, SfxItemState eState,
                                                  const SfxPoolItem* pState)
{
    GetToolBox().EnableItem(GetId(), eState != SfxItemState::DISABLED);
    SfxToolBoxControl::StateChangedAtToolBoxControl(nSID, eState, pState);
}

VclPtr<InterimItemWindow> StartupControl::CreateItemWindow(vcl::Window* pParent)
{
    VclPtr<StartupBox> xBox = VclPtr<StartupBox>::Create(pParent, &GetToolBox(), GetId());
    xBox->Show();
    return xBox;
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
