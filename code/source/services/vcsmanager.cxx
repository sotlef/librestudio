/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// theVcsManager: which open projects are working copies of some version
// control system, and the repository of each.  The systems are providers that
// register themselves, as the symbol providers do with theSymbolIndex, so
// that another system is another component and no change here.

#include <com/sun/star/code/project/ProjectEvent.hpp>
#include <com/sun/star/code/project/XProject.hpp>
#include <com/sun/star/code/project/XProjectListener.hpp>
#include <com/sun/star/code/project/XProjectManager.hpp>
#include <com/sun/star/code/project/theProjectManager.hpp>
#include <com/sun/star/code/vcs/RepositoryEvent.hpp>
#include <com/sun/star/code/vcs/XRepository.hpp>
#include <com/sun/star/code/vcs/XRepositoryListener.hpp>
#include <com/sun/star/code/vcs/XVcsManager.hpp>
#include <com/sun/star/code/vcs/XVcsProvider.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/interfacecontainer4.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <rtl/ref.hxx>
#include <sal/log.hxx>

#include <algorithm>
#include <map>
#include <thread>
#include <vector>

using namespace css;
using namespace css::code;
using namespace css::code::vcs;

namespace codeservices
{
namespace
{
// whether rUrl is rRoot or lies below it
bool IsUnder(const OUString& rRoot, const OUString& rUrl)
{
    return rUrl == rRoot || (rUrl.startsWith(rRoot) && rUrl[rRoot.getLength()] == '/');
}

class VcsManager final
    : public comphelper::WeakComponentImplHelper<XVcsManager, project::XProjectListener,
                                                 XRepositoryListener, lang::XServiceInfo>
{
public:
    explicit VcsManager(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    // Subscribes to the projects; not in the constructor, which cannot hand
    // out `this`.
    void Attach();

    // XVcsManager
    void SAL_CALL registerProvider(const uno::Reference<XVcsProvider>& xProvider) override;
    void SAL_CALL unregisterProvider(const OUString& rKind) override;
    uno::Reference<XRepository> SAL_CALL getRepository(const OUString& rUrl) override;
    uno::Sequence<uno::Reference<XRepository>> SAL_CALL getRepositories() override;
    void SAL_CALL addVcsListener(const uno::Reference<XRepositoryListener>& xListener) override;
    void SAL_CALL removeVcsListener(const uno::Reference<XRepositoryListener>& xListener) override;

    // XProjectListener
    void SAL_CALL projectChanged(const project::ProjectEvent& rEvent) override;
    // XRepositoryListener
    void SAL_CALL repositoryChanged(const RepositoryEvent& rEvent) override;
    void SAL_CALL disposing(const lang::EventObject&) override {}

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.VcsManager"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

private:
    void disposing(std::unique_lock<std::mutex>& rGuard) override;

    // Looks for a working copy under a project, off the calling thread: what a
    // system does to find out may take a while.
    void Detect(const OUString& rProjectUrl);
    void DetectNow(const OUString& rProjectUrl);
    void ForgetProject(const OUString& rProjectUrl);
    void DetectAll();
    void Fire(RepositoryEventKind eKind, const uno::Reference<XRepository>& xRepository);
    void NotifyListeners(const RepositoryEvent& rEvent);
    static void Dispose(const uno::Reference<XRepository>& xRepository);

    uno::Reference<uno::XComponentContext> m_xContext;
    std::vector<uno::Reference<XVcsProvider>> m_aProviders;
    // by root URL
    std::map<OUString, uno::Reference<XRepository>> m_aRepositories;
    // the root each open project belongs to
    std::map<OUString, OUString> m_aProjectRoots;
    comphelper::OInterfaceContainerHelper4<XRepositoryListener> m_aListeners;
};

void VcsManager::Attach()
{
    project::theProjectManager::get(m_xContext)
        ->addProjectListener(uno::Reference<project::XProjectListener>(this));
    DetectAll();
}

void VcsManager::DetectAll()
{
    for (const auto& xProject : project::theProjectManager::get(m_xContext)->getOpenProjects())
        Detect(xProject->getUrl());
}

void VcsManager::Detect(const OUString& rProjectUrl)
{
    // The thread keeps the manager alive while it works.
    std::thread(
        [xSelf = rtl::Reference<VcsManager>(this), rProjectUrl]
        {
            try
            {
                xSelf->DetectNow(rProjectUrl);
            }
            catch (const uno::Exception& rException)
            {
                SAL_WARN("code.services",
                         "version control of " << rProjectUrl << ": " << rException.Message);
            }
        })
        .detach();
}

void VcsManager::DetectNow(const OUString& rProjectUrl)
{
    std::vector<uno::Reference<XVcsProvider>> aProviders;
    {
        std::unique_lock aGuard(m_aMutex);
        if (m_bDisposed)
            return;
        aProviders = m_aProviders;
    }
    // the deepest working copy wins, as the nearest one is the one in force
    uno::Reference<XVcsProvider> xBest;
    OUString aBestRoot;
    for (const auto& xProvider : aProviders)
    {
        OUString aRoot;
        try
        {
            aRoot = xProvider->findRoot(rProjectUrl);
        }
        catch (const uno::Exception& rException)
        {
            SAL_WARN("code.services", xProvider->getKind() << " cannot tell: " << rException.Message);
        }
        if (!aRoot.isEmpty() && (!xBest.is() || aRoot.getLength() > aBestRoot.getLength()))
        {
            xBest = xProvider;
            aBestRoot = aRoot;
        }
    }

    uno::Reference<XRepository> xOpened;
    uno::Reference<XRepository> xClosed;
    {
        std::unique_lock aGuard(m_aMutex);
        if (m_bDisposed)
            return;
        auto itOld = m_aProjectRoots.find(rProjectUrl);
        const OUString aOldRoot = itOld == m_aProjectRoots.end() ? OUString() : itOld->second;
        if (aOldRoot == aBestRoot)
            return;
        m_aProjectRoots.erase(rProjectUrl);
        if (!aBestRoot.isEmpty())
            m_aProjectRoots[rProjectUrl] = aBestRoot;
        // the old working copy goes if nobody else is in it
        if (!aOldRoot.isEmpty()
            && std::none_of(m_aProjectRoots.begin(), m_aProjectRoots.end(),
                            [&](const auto& rEntry) { return rEntry.second == aOldRoot; }))
        {
            auto it = m_aRepositories.find(aOldRoot);
            if (it != m_aRepositories.end())
            {
                xClosed = it->second;
                m_aRepositories.erase(it);
            }
        }
    }
    if (xClosed.is())
    {
        xClosed->removeRepositoryListener(uno::Reference<XRepositoryListener>(this));
        Fire(RepositoryEventKind_CLOSED, xClosed);
        Dispose(xClosed);
    }
    if (aBestRoot.isEmpty())
        return;

    {
        std::unique_lock aGuard(m_aMutex);
        if (m_bDisposed)
            return;
        if (m_aRepositories.contains(aBestRoot))
            return; // another project of the same working copy opened it
    }
    uno::Reference<XRepository> xRepository;
    try
    {
        xRepository = xBest->openRepository(aBestRoot);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code.services", "cannot open " << aBestRoot << ": " << rException.Message);
    }
    if (!xRepository.is())
    {
        // not a working copy after all: the next look may find it otherwise
        std::unique_lock aGuard(m_aMutex);
        m_aProjectRoots.erase(rProjectUrl);
        return;
    }
    {
        std::unique_lock aGuard(m_aMutex);
        if (m_bDisposed || m_aRepositories.contains(aBestRoot))
        {
            aGuard.unlock();
            Dispose(xRepository); // another project of the working copy was quicker
            return;
        }
        m_aRepositories[aBestRoot] = xRepository;
        xOpened = xRepository;
    }
    xOpened->addRepositoryListener(uno::Reference<XRepositoryListener>(this));
    Fire(RepositoryEventKind_OPENED, xOpened);
}

void VcsManager::ForgetProject(const OUString& rProjectUrl)
{
    uno::Reference<XRepository> xClosed;
    {
        std::unique_lock aGuard(m_aMutex);
        auto itRoot = m_aProjectRoots.find(rProjectUrl);
        if (itRoot == m_aProjectRoots.end())
            return;
        const OUString aRoot = itRoot->second;
        m_aProjectRoots.erase(itRoot);
        if (std::any_of(m_aProjectRoots.begin(), m_aProjectRoots.end(),
                        [&](const auto& rEntry) { return rEntry.second == aRoot; }))
            return;
        auto it = m_aRepositories.find(aRoot);
        if (it == m_aRepositories.end())
            return;
        xClosed = it->second;
        m_aRepositories.erase(it);
    }
    xClosed->removeRepositoryListener(uno::Reference<XRepositoryListener>(this));
    Fire(RepositoryEventKind_CLOSED, xClosed);
    Dispose(xClosed);
}

void VcsManager::Dispose(const uno::Reference<XRepository>& xRepository)
{
    uno::Reference<lang::XComponent> xComponent(xRepository, uno::UNO_QUERY);
    if (xComponent.is())
        xComponent->dispose();
}

void VcsManager::Fire(RepositoryEventKind eKind, const uno::Reference<XRepository>& xRepository)
{
    RepositoryEvent aEvent;
    aEvent.Source = xRepository;
    aEvent.Kind = eKind;
    aEvent.RootUrl = xRepository->getRootUrl();
    aEvent.Summary = xRepository->getSummary();
    NotifyListeners(aEvent);
}

// The listeners are called without the lock: one of them may well ask us
// something from inside the call.
void VcsManager::NotifyListeners(const RepositoryEvent& rEvent)
{
    std::vector<uno::Reference<XRepositoryListener>> aListeners;
    {
        std::unique_lock aGuard(m_aMutex);
        if (m_bDisposed)
            return;
        aListeners = m_aListeners.getElements(aGuard);
    }
    for (const auto& xListener : aListeners)
        xListener->repositoryChanged(rEvent);
}

void VcsManager::projectChanged(const project::ProjectEvent& rEvent)
{
    switch (rEvent.Kind)
    {
        case project::ProjectEventKind_OPENED:
            Detect(rEvent.ProjectUrl);
            break;
        case project::ProjectEventKind_CLOSED:
            ForgetProject(rEvent.ProjectUrl);
            break;
        default:
            break;
    }
}

void VcsManager::repositoryChanged(const RepositoryEvent& rEvent)
{
    NotifyListeners(rEvent);
}

void VcsManager::registerProvider(const uno::Reference<XVcsProvider>& xProvider)
{
    if (!xProvider.is())
        throw lang::IllegalArgumentException(u"null provider"_ustr, getXWeak(), 0);
    const OUString aKind = xProvider->getKind();
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        std::erase_if(m_aProviders,
                      [&](const uno::Reference<XVcsProvider>& x) { return x->getKind() == aKind; });
        m_aProviders.push_back(xProvider);
    }
    // what is open may be a working copy of the newcomer
    DetectAll();
}

void VcsManager::unregisterProvider(const OUString& rKind)
{
    std::unique_lock aGuard(m_aMutex);
    std::erase_if(m_aProviders,
                  [&](const uno::Reference<XVcsProvider>& x) { return x->getKind() == rKind; });
}

uno::Reference<XRepository> VcsManager::getRepository(const OUString& rUrl)
{
    std::unique_lock aGuard(m_aMutex);
    uno::Reference<XRepository> xBest;
    sal_Int32 nBest = -1;
    for (const auto& rEntry : m_aRepositories)
        if (IsUnder(rEntry.first, rUrl) && rEntry.first.getLength() > nBest)
        {
            xBest = rEntry.second;
            nBest = rEntry.first.getLength();
        }
    return xBest;
}

uno::Sequence<uno::Reference<XRepository>> VcsManager::getRepositories()
{
    std::unique_lock aGuard(m_aMutex);
    std::vector<uno::Reference<XRepository>> aResult;
    for (const auto& rEntry : m_aRepositories)
        aResult.push_back(rEntry.second);
    return comphelper::containerToSequence(aResult);
}

void VcsManager::addVcsListener(const uno::Reference<XRepositoryListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.addInterface(aGuard, xListener);
}

void VcsManager::removeVcsListener(const uno::Reference<XRepositoryListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.removeInterface(aGuard, xListener);
}

void VcsManager::disposing(std::unique_lock<std::mutex>& rGuard)
{
    lang::EventObject aSource(getXWeak());
    m_aListeners.disposeAndClear(rGuard, aSource);
    m_aProviders.clear();
    m_aProjectRoots.clear();
    m_aRepositories.clear();
    comphelper::WeakComponentImplHelperBase::disposing(rGuard);
}
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_VcsManager_get_implementation(uno::XComponentContext* pContext,
                                                      uno::Sequence<uno::Any> const&)
{
    rtl::Reference<codeservices::VcsManager> xManager(new codeservices::VcsManager(pContext));
    xManager->Attach();
    return cppu::acquire(xManager.get());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
