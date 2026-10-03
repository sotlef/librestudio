/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "backgroundjob.hxx"

#include <com/sun/star/code/core/JobEvent.hpp>
#include <com/sun/star/code/core/XBackgroundJobListener.hpp>
#include <sal/log.hxx>

#include <thread>

using namespace css;
using namespace css::code::core;

namespace codeservices
{
rtl::Reference<BackgroundJob> BackgroundJob::Start(const OUString& rIdentifier, const OUString& rTitle,
                                                   Work aWork)
{
    rtl::Reference<BackgroundJob> xJob(new BackgroundJob(rIdentifier, rTitle, std::move(aWork)));
    // The thread keeps the job alive for as long as it works, so the caller
    // may let go of it at once.
    std::thread([xJob] { xJob->Run(); }).detach();
    return xJob;
}

void BackgroundJob::Run()
{
    Change(JobStatus_RUNNING);
    bool bSucceeded = false;
    try
    {
        bSucceeded = m_aWork(*this);
    }
    catch (const uno::Exception& rException)
    {
        SetMessage(rException.Message);
    }
    catch (const std::exception& rException)
    {
        SAL_WARN("code.services", "job " << m_aIdentifier << " failed: " << rException.what());
    }
    Change(m_bCancelled ? JobStatus_CANCELLED : bSucceeded ? JobStatus_SUCCEEDED : JobStatus_FAILED);
}

void BackgroundJob::Change(JobStatus eStatus)
{
    JobEvent aEvent;
    aEvent.Source = getXWeak();
    aEvent.Status = eStatus;
    std::unique_lock aGuard(m_aMutex);
    m_eStatus = eStatus;
    aEvent.Progress = m_nProgress;
    aEvent.Message = m_aMessage;
    m_aListeners.forEach(aGuard, [&](const uno::Reference<XBackgroundJobListener>& xListener)
                         { xListener->jobChanged(aEvent); });
}

void BackgroundJob::SetMessage(const OUString& rMessage)
{
    {
        std::scoped_lock aGuard(m_aMutex);
        m_aMessage = rMessage;
    }
    Change(getStatus());
}

void BackgroundJob::SetProgress(sal_Int16 nProgress)
{
    {
        std::scoped_lock aGuard(m_aMutex);
        m_nProgress = nProgress;
    }
    Change(getStatus());
}

JobStatus BackgroundJob::getStatus()
{
    std::scoped_lock aGuard(m_aMutex);
    return m_eStatus;
}

sal_Int16 BackgroundJob::getProgress()
{
    std::scoped_lock aGuard(m_aMutex);
    return m_nProgress;
}

OUString BackgroundJob::getStatusMessage()
{
    std::scoped_lock aGuard(m_aMutex);
    return m_aMessage;
}

void BackgroundJob::addJobListener(const uno::Reference<XBackgroundJobListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.addInterface(aGuard, xListener);
}

void BackgroundJob::removeJobListener(const uno::Reference<XBackgroundJobListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.removeInterface(aGuard, xListener);
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
