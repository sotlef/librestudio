/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/code/core/JobStatus.hpp>
#include <com/sun/star/code/core/XBackgroundJob.hpp>
#include <comphelper/interfacecontainer4.hxx>
#include <cppuhelper/implbase.hxx>
#include <rtl/ref.hxx>

#include <atomic>
#include <functional>
#include <mutex>

namespace codeservices
{
/// A background job whose work is a function run on a thread of its own.
///
/// The function reports through SetMessage()/SetProgress() and watches
/// IsCancelled(); what it returns is the outcome (true = succeeded, false =
/// failed, unless it was cancelled meanwhile).  The listeners are told on
/// every change, from the job's thread.
class BackgroundJob final : public cppu::WeakImplHelper<css::code::core::XBackgroundJob>
{
public:
    using Work = std::function<bool(BackgroundJob&)>;

    /// Creates the job and starts running rWork at once.
    static rtl::Reference<BackgroundJob> Start(const OUString& rIdentifier, const OUString& rTitle,
                                               Work aWork);

    void SetMessage(const OUString& rMessage);
    /// 0..100, or -1 if unknown
    void SetProgress(sal_Int16 nProgress);
    bool IsCancelled() const { return m_bCancelled; }

    // XBackgroundJob
    OUString SAL_CALL getIdentifier() override { return m_aIdentifier; }
    OUString SAL_CALL getTitle() override { return m_aTitle; }
    css::code::core::JobStatus SAL_CALL getStatus() override;
    sal_Int16 SAL_CALL getProgress() override;
    OUString SAL_CALL getStatusMessage() override;
    sal_Bool SAL_CALL getCancellable() override { return true; }
    void SAL_CALL cancel() override { m_bCancelled = true; }
    void SAL_CALL
    addJobListener(const css::uno::Reference<css::code::core::XBackgroundJobListener>& xListener) override;
    void SAL_CALL
    removeJobListener(const css::uno::Reference<css::code::core::XBackgroundJobListener>& xListener) override;

private:
    BackgroundJob(OUString aIdentifier, OUString aTitle, Work aWork)
        : m_aIdentifier(std::move(aIdentifier))
        , m_aTitle(std::move(aTitle))
        , m_aWork(std::move(aWork))
    {
    }

    void Run();
    void Change(css::code::core::JobStatus eStatus);

    const OUString m_aIdentifier;
    const OUString m_aTitle;
    Work m_aWork;
    std::atomic<bool> m_bCancelled{ false };

    std::mutex m_aMutex;
    css::code::core::JobStatus m_eStatus = css::code::core::JobStatus_PENDING;
    sal_Int16 m_nProgress = -1;
    OUString m_aMessage;
    comphelper::OInterfaceContainerHelper4<css::code::core::XBackgroundJobListener> m_aListeners;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
