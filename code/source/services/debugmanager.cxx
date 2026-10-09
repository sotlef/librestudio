/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// theDebugManager: the debuggers, as adapters that register themselves, the
// way the version control systems do with theVcsManager, so that another
// debugger is another component and no change here.  The manager only keeps
// the list and picks from it; the sessions are made by the adapters and live
// on their own.

#include <com/sun/star/code/debug/LaunchDescriptor.hpp>
#include <com/sun/star/code/debug/XDebugAdapter.hpp>
#include <com/sun/star/code/debug/XDebugManager.hpp>
#include <com/sun/star/code/debug/XDebugManagerListener.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <sal/log.hxx>

#include <algorithm>
#include <optional>
#include <vector>

using namespace css;
using namespace css::code;
using namespace css::code::debug;

namespace codeservices
{
namespace
{
class DebugManager final
    : public comphelper::WeakComponentImplHelper<XDebugManager, lang::XServiceInfo>
{
public:
    DebugManager() = default;

    // XDebugManager
    void SAL_CALL registerAdapter(const uno::Reference<XDebugAdapter>& xAdapter) override;
    uno::Sequence<uno::Reference<XDebugAdapter>> SAL_CALL getAdapters() override;
    uno::Reference<XDebugAdapter>
        SAL_CALL findAdapter(const LaunchDescriptor& rDescriptor) override;
    void SAL_CALL requestLaunch(const LaunchDescriptor& rDescriptor) override;
    void SAL_CALL
    addDebugManagerListener(const uno::Reference<XDebugManagerListener>& xListener) override;
    void SAL_CALL
    removeDebugManagerListener(const uno::Reference<XDebugManagerListener>& xListener) override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.DebugManager"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

private:
    void disposing(std::unique_lock<std::mutex>& rGuard) override;

    std::vector<uno::Reference<XDebugAdapter>> m_aAdapters;
    std::vector<uno::Reference<XDebugManagerListener>> m_aListeners;
    // A request made while no one listens (the asker opens the lazily
    // created debugging UI only after asking): the next listener hears it
    // the moment it subscribes.
    std::optional<LaunchDescriptor> m_oPending;
};

void DebugManager::registerAdapter(const uno::Reference<XDebugAdapter>& xAdapter)
{
    if (!xAdapter.is())
        throw lang::IllegalArgumentException(u"null adapter"_ustr, getXWeak(), 0);
    const OUString aName = xAdapter->getName();
    std::unique_lock aGuard(m_aMutex);
    throwIfDisposed(aGuard);
    std::erase_if(m_aAdapters,
                  [&](const uno::Reference<XDebugAdapter>& x) { return x->getName() == aName; });
    m_aAdapters.push_back(xAdapter);
}

uno::Sequence<uno::Reference<XDebugAdapter>> DebugManager::getAdapters()
{
    std::unique_lock aGuard(m_aMutex);
    return comphelper::containerToSequence(m_aAdapters);
}

uno::Reference<XDebugAdapter> DebugManager::findAdapter(const LaunchDescriptor& rDescriptor)
{
    std::vector<uno::Reference<XDebugAdapter>> aAdapters;
    {
        std::unique_lock aGuard(m_aMutex);
        if (m_bDisposed)
            return {};
        aAdapters = m_aAdapters;
    }
    // asked without the lock: an adapter may well run its tool to find out
    for (const auto& xAdapter : aAdapters)
    {
        try
        {
            if (xAdapter->canDebug(rDescriptor))
                return xAdapter;
        }
        catch (const uno::Exception& rException)
        {
            SAL_WARN("code.services",
                     xAdapter->getName() << " cannot tell: " << rException.Message);
        }
    }
    return {};
}

void DebugManager::requestLaunch(const LaunchDescriptor& rDescriptor)
{
    std::vector<uno::Reference<XDebugManagerListener>> aListeners;
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        if (m_aListeners.empty())
        {
            // no debugging UI yet: the request waits for the first listener
            m_oPending = rDescriptor;
            return;
        }
        aListeners = m_aListeners;
    }
    // told without the lock, on the asking thread; the UI marshals
    for (const auto& xListener : aListeners)
    {
        try
        {
            xListener->launchRequested(rDescriptor);
        }
        catch (const uno::Exception& rException)
        {
            SAL_WARN("code.services", "launch listener failed: " << rException.Message);
        }
    }
}

void DebugManager::addDebugManagerListener(const uno::Reference<XDebugManagerListener>& xListener)
{
    if (!xListener.is())
        throw lang::IllegalArgumentException(u"null listener"_ustr, getXWeak(), 0);
    std::optional<LaunchDescriptor> oPending;
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        m_aListeners.push_back(xListener);
        oPending.swap(m_oPending);
    }
    // the waiting request, delivered on the subscribing thread
    if (oPending)
    {
        try
        {
            xListener->launchRequested(*oPending);
        }
        catch (const uno::Exception& rException)
        {
            SAL_WARN("code.services", "launch listener failed: " << rException.Message);
        }
    }
}

void DebugManager::removeDebugManagerListener(
    const uno::Reference<XDebugManagerListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    std::erase(m_aListeners, xListener);
}

void DebugManager::disposing(std::unique_lock<std::mutex>& rGuard)
{
    m_aAdapters.clear();
    m_aListeners.clear();
    m_oPending.reset();
    comphelper::WeakComponentImplHelperBase::disposing(rGuard);
}
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_DebugManager_get_implementation(uno::XComponentContext*,
                                                        uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::DebugManager);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
