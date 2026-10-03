/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "common.hxx"

#include <com/sun/star/code/core/JobEvent.hpp>
#include <com/sun/star/code/core/XBackgroundJob.hpp>
#include <com/sun/star/code/core/XBackgroundJobListener.hpp>
#include <com/sun/star/code/core/XJobRegistry.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/supportsservice.hxx>

#include <algorithm>
#include <vector>

using namespace css;
using namespace css::code::core;

namespace codeservices
{
namespace
{
// The registry is itself the listener it hangs on every job it is given, so
// a job's events reach the registry's own listeners (the status bar) without
// each of them having to find and subscribe to every job.
class JobRegistry final
    : public comphelper::WeakComponentImplHelper<XJobRegistry, XBackgroundJobListener,
                                                 lang::XServiceInfo>
{
public:
    // XJobRegistry
    void SAL_CALL registerJob(const uno::Reference<XBackgroundJob>& xJob) override;
    uno::Sequence<uno::Reference<XBackgroundJob>> SAL_CALL getActiveJobs() override;
    void SAL_CALL addJobListener(const uno::Reference<XBackgroundJobListener>& xListener) override;
    void SAL_CALL
    removeJobListener(const uno::Reference<XBackgroundJobListener>& xListener) override;

    // XBackgroundJobListener
    void SAL_CALL jobChanged(const JobEvent& rEvent) override;
    // XEventListener -- a job went away without reaching a final status
    void SAL_CALL disposing(const lang::EventObject& rSource) override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.JobRegistry"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

private:
    void disposing(std::unique_lock<std::mutex>& rGuard) override;
    void forward(const JobEvent& rEvent);

    std::vector<uno::Reference<XBackgroundJob>> m_aJobs;
    comphelper::OInterfaceContainerHelper4<XBackgroundJobListener> m_aListeners;
};

void JobRegistry::registerJob(const uno::Reference<XBackgroundJob>& xJob)
{
    if (!xJob.is())
        throw lang::IllegalArgumentException(u"null job"_ustr, getXWeak(), 0);

    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        if (std::find(m_aJobs.begin(), m_aJobs.end(), xJob) != m_aJobs.end())
            return;
        m_aJobs.push_back(xJob);
    }

    // Subscribe before reading the status: a job that finishes in between is
    // then seen by the listener rather than lost.
    xJob->addJobListener(this);

    JobEvent aEvent;
    aEvent.Source = xJob;
    aEvent.Status = xJob->getStatus();
    aEvent.Progress = xJob->getProgress();
    aEvent.Message = xJob->getStatusMessage();
    jobChanged(aEvent);
}

uno::Sequence<uno::Reference<XBackgroundJob>> JobRegistry::getActiveJobs()
{
    std::unique_lock aGuard(m_aMutex);
    return comphelper::containerToSequence(m_aJobs);
}

void JobRegistry::addJobListener(const uno::Reference<XBackgroundJobListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.addInterface(aGuard, xListener);
}

void JobRegistry::removeJobListener(const uno::Reference<XBackgroundJobListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.removeInterface(aGuard, xListener);
}

void JobRegistry::forward(const JobEvent& rEvent)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.forEach(aGuard, [&](const uno::Reference<XBackgroundJobListener>& xListener)
                         { xListener->jobChanged(rEvent); });
}

void JobRegistry::jobChanged(const JobEvent& rEvent)
{
    uno::Reference<XBackgroundJob> xJob(rEvent.Source, uno::UNO_QUERY);
    const bool bFinal = IsFinalStatus(rEvent.Status);
    if (bFinal && xJob.is())
    {
        std::unique_lock aGuard(m_aMutex);
        std::erase(m_aJobs, xJob);
    }

    // Listeners hear about the final status first; only then is the job let go.
    forward(rEvent);

    if (bFinal && xJob.is())
        xJob->removeJobListener(this);
}

void JobRegistry::disposing(const lang::EventObject& rSource)
{
    std::unique_lock aGuard(m_aMutex);
    std::erase_if(m_aJobs, [&](const uno::Reference<XBackgroundJob>& xJob)
                  { return uno::Reference<uno::XInterface>(xJob) == rSource.Source; });
}

void JobRegistry::disposing(std::unique_lock<std::mutex>& rGuard)
{
    lang::EventObject aSource(getXWeak());
    m_aListeners.disposeAndClear(rGuard, aSource);
    m_aJobs.clear();
}
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_JobRegistry_get_implementation(uno::XComponentContext*,
                                                       uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::JobRegistry);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
