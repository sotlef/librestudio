/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "common.hxx"
#include "folderproject.hxx"

#include <com/sun/star/code/core/XBackgroundJob.hpp>
#include <com/sun/star/code/core/XJobRegistry.hpp>
#include <com/sun/star/code/core/theCode.hpp>
#include <com/sun/star/code/core/theJobRegistry.hpp>
#include <com/sun/star/code/frontend/XDialogs.hpp>
#include <com/sun/star/code/frontend/XWorkbench.hpp>
#include <com/sun/star/code/project/ProjectConfiguration.hpp>
#include <com/sun/star/code/project/ProjectEvent.hpp>
#include <com/sun/star/code/project/XConfigurableProjectFormat.hpp>
#include <com/sun/star/code/project/XProjectConfigurationState.hpp>
#include <com/sun/star/code/project/XProjectConfigStore.hpp>
#include <com/sun/star/code/project/theProjectConfigStore.hpp>
#include <com/sun/star/code/project/XProjectFormatReader.hpp>
#include <com/sun/star/code/project/XProjectListener.hpp>
#include <com/sun/star/code/project/XProjectManager.hpp>
#include <com/sun/star/code/project/theProjectFormatRegistry.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/util/XModifyBroadcaster.hpp>
#include <com/sun/star/util/XModifyListener.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <sal/log.hxx>

#include <algorithm>
#include <mutex>
#include <vector>

using namespace css;
using namespace css::code;
using namespace css::code::project;

namespace codeservices
{
namespace
{
// A project's identity here is its URL, and only its URL: deepcode keyed its
// table by URI but looked projects up by a separate id, so closing one by id
// never matched.
class ProjectManager final
    : public comphelper::WeakComponentImplHelper<XProjectManager, util::XModifyListener,
                                                 lang::XServiceInfo>
{
public:
    explicit ProjectManager(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    // XProjectManager
    uno::Reference<XProject> SAL_CALL openProject(const OUString& rUrl) override;
    void SAL_CALL closeProject(const OUString& rUrl) override;
    uno::Sequence<uno::Reference<XProject>> SAL_CALL getOpenProjects() override;
    uno::Reference<XProject> SAL_CALL getProject(const OUString& rUrl) override;
    uno::Reference<XProject> SAL_CALL getActiveProject() override;
    void SAL_CALL setActiveProject(const uno::Reference<XProject>& xProject) override;
    uno::Sequence<ProjectConfiguration> SAL_CALL getConfigurations(const OUString& rUrl) override;
    OUString SAL_CALL getCurrentConfiguration(const OUString& rUrl) override;
    void SAL_CALL setCurrentConfiguration(const OUString& rUrl, const OUString& rName) override;
    void SAL_CALL addProjectListener(const uno::Reference<XProjectListener>& xListener) override;
    void SAL_CALL removeProjectListener(const uno::Reference<XProjectListener>& xListener) override;

    // XModifyListener: a project that can change (a CMake project whose
    // configure run has ended) says so; listeners of the manager hear it as
    // ITEMS_CHANGED, the one event every project-showing panel already handles.
    void SAL_CALL modified(const lang::EventObject& rEvent) override
    {
        uno::Reference<XProject> xProject(rEvent.Source, uno::UNO_QUERY);
        if (!xProject.is())
            return;
        const OUString aUrl = xProject->getUrl();
        std::unique_lock aGuard(m_aMutex);
        fire(aGuard, xProject, ProjectEventKind_ITEMS_CHANGED, aUrl);
    }
    void SAL_CALL disposing(const lang::EventObject&) override {}

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.ProjectManager"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

private:
    void disposing(std::unique_lock<std::mutex>& rGuard) override;
    void fire(std::unique_lock<std::mutex>& rGuard, const uno::Reference<XProject>& xProject,
              ProjectEventKind eKind, const OUString& rProjectUrl);
    uno::Reference<XProject> find(const OUString& rUrl) const; // lock held
    // false if the user cancelled the configuration
    bool ensureConfigured(const uno::Reference<XProjectFormatReader>& xReader, const OUString& rUrl);

    uno::Reference<uno::XComponentContext> m_xContext;
    std::vector<uno::Reference<XProject>> m_aProjects; // in opening order
    uno::Reference<XProject> m_xActive;
    comphelper::OInterfaceContainerHelper4<XProjectListener> m_aListeners;
};

void ProjectManager::fire(std::unique_lock<std::mutex>& rGuard,
                          const uno::Reference<XProject>& xProject, ProjectEventKind eKind,
                          const OUString& rProjectUrl)
{
    ProjectEvent aEvent;
    // With no project (the active one was closed and none is left) the manager is the source.
    if (xProject.is())
        aEvent.Source = xProject;
    else
        aEvent.Source = getXWeak();
    aEvent.Kind = eKind;
    aEvent.ProjectUrl = rProjectUrl;
    m_aListeners.forEach(rGuard, [&](const uno::Reference<XProjectListener>& xListener)
                         { xListener->projectChanged(aEvent); });
}

uno::Reference<XProject> ProjectManager::find(const OUString& rUrl) const
{
    for (const auto& xProject : m_aProjects)
        if (xProject->getUrl() == rUrl)
            return xProject;
    return {};
}

bool ProjectManager::ensureConfigured(const uno::Reference<XProjectFormatReader>& xReader,
                                      const OUString& rUrl)
{
    uno::Reference<XConfigurableProjectFormat> xFormat(xReader, uno::UNO_QUERY);
    if (!xFormat.is())
        return true; // nothing to configure

    auto xStore = theProjectConfigStore::get(m_xContext);
    if (xStore->getConfigurations(rUrl).hasElements())
        return true; // configured before

    uno::Reference<frontend::XWorkbench> xWorkbench = core::theCode::get(m_xContext)->getWorkbench();
    uno::Reference<frontend::XDialogs> xDialogs
        = xWorkbench.is() ? xWorkbench->getDialogs() : uno::Reference<frontend::XDialogs>();
    if (!xDialogs.is())
        throw io::IOException(
            "the project must be configured first, and there is no user interface to do it in", getXWeak());

    uno::Sequence<ProjectConfiguration> aConfigurations = xFormat->getDefaultConfigurations(rUrl);
    if (!xDialogs->configureProject(xFormat->getConfiguratorServiceName(), rUrl, aConfigurations))
        return false;
    xStore->setConfigurations(rUrl, aConfigurations);
    // the first configuration is the current one until the user picks another
    if (aConfigurations.hasElements())
        xStore->setCurrentConfiguration(rUrl, aConfigurations[0].Name);

    // The configuration step runs in the background, its output in a channel
    // of the format's; the project opens without waiting for it.  A failure to
    // start it is reported, not allowed to stop the project from opening.
    try
    {
        uno::Reference<core::XBackgroundJob> xJob = xFormat->configure(rUrl, aConfigurations);
        if (xJob.is())
            core::theJobRegistry::get(m_xContext)->registerJob(xJob);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code.services", "cannot configure " << rUrl << ": " << rException.Message);
    }
    return true;
}

uno::Reference<XProject> ProjectManager::openProject(const OUString& rUrl)
{
    const OUString aUrl = NormalizeUrl(rUrl);
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        if (auto xOpen = find(aUrl))
            return xOpen;
    }

    // Reading a project is the reader's own, possibly slow, code: no lock.
    uno::Reference<XProject> xProject;
    uno::Reference<XProjectFormatReader> xReader
        = theProjectFormatRegistry::get(m_xContext)->findReader(aUrl);
    if (xReader.is())
    {
        // A format that has to be configured is, the first time, configured
        // by the user before the project is opened; what was chosen is
        // remembered next to the project, and a project that has been
        // configured opens without asking.
        if (!ensureConfigured(xReader, aUrl))
            return {}; // the user cancelled the configuration
        xProject = xReader->read(aUrl);
    }
    else if (IsDirectory(aUrl))
        xProject = CreateFolderProject(aUrl);
    else
        throw io::IOException("no project format reader accepts " + aUrl, getXWeak());
    if (!xProject.is())
        throw io::IOException("the reader returned no project for " + aUrl, getXWeak());

    std::unique_lock aGuard(m_aMutex);
    throwIfDisposed(aGuard);
    if (auto xRaced = find(aUrl)) // opened by someone else meanwhile: theirs wins
    {
        aGuard.unlock();
        xProject->dispose();
        return xRaced;
    }
    m_aProjects.push_back(xProject);
    if (uno::Reference<util::XModifyBroadcaster> xBroadcaster{ xProject, uno::UNO_QUERY })
        xBroadcaster->addModifyListener(this);
    fire(aGuard, xProject, ProjectEventKind_OPENED, aUrl);
    if (!m_xActive.is())
    {
        m_xActive = xProject;
        fire(aGuard, xProject, ProjectEventKind_ACTIVATED, aUrl);
    }
    return xProject;
}

void ProjectManager::closeProject(const OUString& rUrl)
{
    const OUString aUrl = NormalizeUrl(rUrl);
    uno::Reference<XProject> xProject;
    {
        std::unique_lock aGuard(m_aMutex);
        xProject = find(aUrl);
        if (!xProject.is())
            return;
        std::erase(m_aProjects, xProject);
        if (uno::Reference<util::XModifyBroadcaster> xBroadcaster{ xProject, uno::UNO_QUERY })
            xBroadcaster->removeModifyListener(this);
        fire(aGuard, xProject, ProjectEventKind_CLOSED, aUrl);

        if (m_xActive == xProject)
        {
            m_xActive = m_aProjects.empty() ? uno::Reference<XProject>() : m_aProjects.back();
            fire(aGuard, m_xActive, ProjectEventKind_ACTIVATED,
                 m_xActive.is() ? m_xActive->getUrl() : OUString());
        }
    }
    xProject->dispose();
}

uno::Sequence<uno::Reference<XProject>> ProjectManager::getOpenProjects()
{
    std::unique_lock aGuard(m_aMutex);
    return comphelper::containerToSequence(m_aProjects);
}

uno::Reference<XProject> ProjectManager::getProject(const OUString& rUrl)
{
    std::unique_lock aGuard(m_aMutex);
    return find(NormalizeUrl(rUrl));
}

uno::Reference<XProject> ProjectManager::getActiveProject()
{
    std::unique_lock aGuard(m_aMutex);
    return m_xActive;
}

void ProjectManager::setActiveProject(const uno::Reference<XProject>& xProject)
{
    std::unique_lock aGuard(m_aMutex);
    throwIfDisposed(aGuard);
    if (xProject.is() && std::find(m_aProjects.begin(), m_aProjects.end(), xProject) == m_aProjects.end())
        throw lang::IllegalArgumentException(u"the project is not open"_ustr, getXWeak(), 0);
    if (m_xActive == xProject)
        return;
    m_xActive = xProject;
    fire(aGuard, xProject, ProjectEventKind_ACTIVATED, xProject.is() ? xProject->getUrl() : OUString());
}

uno::Sequence<ProjectConfiguration> ProjectManager::getConfigurations(const OUString& rUrl)
{
    return theProjectConfigStore::get(m_xContext)->getConfigurations(NormalizeUrl(rUrl));
}

OUString ProjectManager::getCurrentConfiguration(const OUString& rUrl)
{
    return theProjectConfigStore::get(m_xContext)->getCurrentConfiguration(NormalizeUrl(rUrl));
}

void ProjectManager::setCurrentConfiguration(const OUString& rUrl, const OUString& rName)
{
    const OUString aUrl = NormalizeUrl(rUrl);
    if (getCurrentConfiguration(aUrl) == rName)
        return;
    theProjectConfigStore::get(m_xContext)->setCurrentConfiguration(aUrl, rName);

    uno::Reference<XProject> xProject;
    {
        std::unique_lock aGuard(m_aMutex);
        xProject = find(aUrl);
        if (xProject.is())
            fire(aGuard, xProject, ProjectEventKind_CONFIGURATION_CHANGED, aUrl);
    }
    // what the project reads its structure from depends on the configuration;
    // it tells its listeners (ITEMS_CHANGED, through modified()) if it changed
    if (uno::Reference<XProjectConfigurationState> xState{ xProject, uno::UNO_QUERY })
        xState->reload();
}

void ProjectManager::addProjectListener(const uno::Reference<XProjectListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.addInterface(aGuard, xListener);
}

void ProjectManager::removeProjectListener(const uno::Reference<XProjectListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.removeInterface(aGuard, xListener);
}

void ProjectManager::disposing(std::unique_lock<std::mutex>& rGuard)
{
    lang::EventObject aSource(getXWeak());
    m_aListeners.disposeAndClear(rGuard, aSource);
    m_aProjects.clear();
    m_xActive.clear();
}
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_ProjectManager_get_implementation(uno::XComponentContext* pContext,
                                                          uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::ProjectManager(pContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
