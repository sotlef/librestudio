/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// File > Recent Projects and Solutions: a sub-menu filled, each time it opens,
// with the projects theProjectManager remembers (at most ten, newest first).
// Modelled on framework's RecentFilesMenuController, which does the same for
// documents.

#include "recentprojects.hxx"

#include <com/sun/star/awt/MenuEvent.hpp>
#include <com/sun/star/awt/XPopupMenu.hpp>
#include <com/sun/star/code/core/theCode.hpp>
#include <com/sun/star/code/frontend/XStatusBar.hpp>
#include <com/sun/star/code/frontend/XWorkbench.hpp>
#include <com/sun/star/code/project/XProjectManager.hpp>
#include <com/sun/star/code/project/theProjectManager.hpp>
#include <com/sun/star/frame/FeatureStateEvent.hpp>
#include <comphelper/processfactory.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <sal/log.hxx>
#include <svtools/popupmenucontrollerbase.hxx>
#include <svtools/imagemgr.hxx>
#include <tools/urlobj.hxx>
#include <toolkit/awt/vclxmenu.hxx>
#include <vcl/svapp.hxx>


using namespace css;

namespace
{
constexpr OUString CMD_CLEAR = u".uno:CodeClearRecentProjects"_ustr;

class RecentProjectsMenuController final : public svt::PopupMenuControllerBase
{
    using svt::PopupMenuControllerBase::disposing;

public:
    explicit RecentProjectsMenuController(const uno::Reference<uno::XComponentContext>& xContext)
        : svt::PopupMenuControllerBase(xContext)
    {
    }

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.RecentProjectsMenuController"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override
    {
        return { u"com.sun.star.frame.PopupMenuController"_ustr };
    }

    // XStatusListener
    void SAL_CALL statusChanged(const frame::FeatureStateEvent&) override {}

    // XMenuListener
    void SAL_CALL itemSelected(const awt::MenuEvent& rEvent) override;
    void SAL_CALL itemActivated(const awt::MenuEvent&) override
    {
        std::unique_lock aLock(m_aMutex);
        impl_setPopupMenu(aLock);
    }

private:
    void impl_setPopupMenu(std::unique_lock<std::mutex>&) override
    {
        if (m_xPopupMenu.is())
            fill(m_xPopupMenu);
    }
    void fill(const uno::Reference<awt::XPopupMenu>& xMenu);

    // what the entries stand for, in menu order (an item's id is its index + 1)
    std::vector<OUString> m_aProjects;
};

void RecentProjectsMenuController::fill(const uno::Reference<awt::XPopupMenu>& xMenu)
{
    SolarMutexGuard aGuard;
    resetPopupMenu(xMenu);

    m_aProjects.clear();
    m_aProjects = ::code::RecentProjects::Get();

    if (m_aProjects.empty())
    {
        // Not disabled: a disabled entry would show the framework's own text instead.
        xMenu->insertItem(1, u"(no recent projects)"_ustr, static_cast<sal_Int16>(MenuItemBits::NOSELECT), -1);
        return;
    }

    for (size_t i = 0; i < m_aProjects.size(); ++i)
    {
        // "1. name", with the number as an accelerator; the tenth is "0"
        const INetURLObject aUrl(m_aProjects[i]);
        const OUString aName = aUrl.GetLastName(INetURLObject::DecodeMechanism::WithCharset);
        OUString aLabel;
        if (i < 9)
            aLabel = "~" + OUString::number(i + 1) + ". " + aName;
        else
            aLabel = "1~0. " + aName;

        const sal_uInt16 nId = static_cast<sal_uInt16>(i + 1);
        xMenu->insertItem(nId, aLabel, 0, -1);
        // the whole path as the tip: names alone do not tell two projects apart
        xMenu->setTipHelpText(nId, aUrl.getFSysPath(FSysStyle::Detect));
        xMenu->setCommand(nId, "vnd.sun.star.popup:RecentProjectList?entry=" + OUString::number(i));
    }
    xMenu->insertSeparator(-1);
    const sal_uInt16 nClear = static_cast<sal_uInt16>(m_aProjects.size() + 1);
    xMenu->insertItem(nClear, u"~Clear List"_ustr, 0, -1);
    xMenu->setCommand(nClear, CMD_CLEAR);
}

void RecentProjectsMenuController::itemSelected(const awt::MenuEvent& rEvent)
{
    rtl::Reference<VCLXPopupMenu> xMenu;
    {
        std::unique_lock aLock(m_aMutex);
        xMenu = m_xPopupMenu;
    }
    if (!xMenu.is())
        return;

    try
    {
        if (xMenu->getCommand(rEvent.MenuId) == CMD_CLEAR)
        {
            ::code::RecentProjects::Clear();
            return;
        }

        const size_t nIndex = static_cast<size_t>(rEvent.MenuId) - 1;
        if (nIndex >= m_aProjects.size())
            return;
        const OUString aUrl = m_aProjects[nIndex];
        try
        {
            css::code::project::theProjectManager::get(comphelper::getProcessComponentContext())
                ->openProject(aUrl);
            // The shell adds a project when it is opened; one that was open
            // already is not "opened" again, so it is moved up here.
            ::code::RecentProjects::Add(aUrl);
        }
        catch (const uno::Exception& rException)
        {
            // A project that is gone (moved, deleted) is dropped from the list
            // rather than failing for ever.
            ::code::RecentProjects::Remove(aUrl);
            auto xWorkbench = css::code::core::theCode::get(comphelper::getProcessComponentContext())->getWorkbench();
            if (xWorkbench.is() && xWorkbench->getStatusBar().is())
                xWorkbench->getStatusBar()->showMessage(u"Cannot open "_ustr + aUrl + ": " + rException.Message, 8000);
        }
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "recent projects: " << rException.Message);
    }
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_RecentProjectsMenuController_get_implementation(
    uno::XComponentContext* pContext, uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new RecentProjectsMenuController(pContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
