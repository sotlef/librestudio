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

#include <com/sun/star/code/project/ProjectEvent.hpp>
#include <com/sun/star/code/project/XProjectFormatReader.hpp>
#include <com/sun/star/code/project/XProjectListener.hpp>
#include <com/sun/star/code/project/XProjectManager.hpp>
#include <com/sun/star/code/project/theProjectFormatRegistry.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/supportsservice.hxx>

#include <algorithm>
#include <vector>

using namespace css;
using namespace css::code::project;

namespace codeservices
{
namespace
{
// A project's identity here is its URL, and only its URL: deepcode keyed its
// table by URI but looked projects up by a separate id, so closing one by id
// never matched.
class ProjectManager final
    : public comphelper::WeakComponentImplHelper<XProjectManager, lang::XServiceInfo>
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
    void SAL_CALL addProjectListener(const uno::Reference<XProjectListener>& xListener) override;
    void SAL_CALL removeProjectListener(const uno::Reference<XProjectListener>& xListener) override;

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
        xProject = xReader->read(aUrl);
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
