/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

// What the UI side of "Code" needs to listen to the com.sun.star.code.*
// services.  The services call their listeners on whatever thread the event
// happened on (a build's reader thread, say); VCL objects may only be touched
// on the main thread, so a listener never does UI work itself: it asks a
// UiNotifier, which posts one event to the main loop and runs the panel's
// refresh function there.

#include <com/sun/star/code/core/JobEvent.hpp>
#include <com/sun/star/code/core/XBackgroundJobListener.hpp>
#include <com/sun/star/code/document/DocumentEvent.hpp>
#include <com/sun/star/code/document/XDocumentListener.hpp>
#include <com/sun/star/code/output/XOutputListener.hpp>
#include <com/sun/star/code/project/ProjectEvent.hpp>
#include <com/sun/star/code/project/XProjectListener.hpp>
#include <com/sun/star/code/semantic/SemanticTokenEvent.hpp>
#include <com/sun/star/code/semantic/XSemanticTokenListener.hpp>
#include <com/sun/star/code/task/XTaskListener.hpp>
#include <cppuhelper/implbase.hxx>
#include <tools/link.hxx>
#include <vcl/svapp.hxx>

#include <functional>
#include <memory>
#include <mutex>

namespace code
{
/// Runs a function on the main thread, coalescing bursts: however many times
/// Notify() is called before the main loop gets to it, the function runs once.
class UiNotifier final : public std::enable_shared_from_this<UiNotifier>
{
public:
    explicit UiNotifier(std::function<void()> aFunction)
        : m_aFunction(std::move(aFunction))
    {
    }

    /// any thread
    void Notify();

    /// Main thread; called by the owner before it goes away.  An event that is
    /// already queued then finds nothing to run.
    void Detach()
    {
        std::scoped_lock aGuard(m_aMutex);
        m_aFunction = nullptr;
    }

private:
    DECL_STATIC_LINK(UiNotifier, RunHdl, void*, void);

    std::mutex m_aMutex;
    std::function<void()> m_aFunction;
    bool m_bPending = false;
};

/// One listener object for every kind of event the UI cares about: whatever
/// arrives, the answer is "refresh".  A panel subscribes it to the services it
/// shows and unsubscribes it in its destructor.
class ServiceListener final
    : public cppu::WeakImplHelper<css::code::core::XBackgroundJobListener,
                                  css::code::document::XDocumentListener,
                                  css::code::output::XOutputListener,
                                  css::code::project::XProjectListener,
                                  css::code::semantic::XSemanticTokenListener,
                                  css::code::task::XTaskListener>
{
public:
    explicit ServiceListener(std::shared_ptr<UiNotifier> xNotifier)
        : m_xNotifier(std::move(xNotifier))
    {
    }

    void SAL_CALL jobChanged(const css::code::core::JobEvent&) override { m_xNotifier->Notify(); }
    void SAL_CALL documentChanged(const css::code::document::DocumentEvent&) override
    {
        m_xNotifier->Notify();
    }
    void SAL_CALL channelAdded(const OUString&) override { m_xNotifier->Notify(); }
    void SAL_CALL lineAppended(const OUString&, const css::code::output::OutputLine&) override
    {
        m_xNotifier->Notify();
    }
    void SAL_CALL channelCleared(const OUString&) override { m_xNotifier->Notify(); }
    void SAL_CALL projectChanged(const css::code::project::ProjectEvent&) override
    {
        m_xNotifier->Notify();
    }
    void SAL_CALL tokensChanged(const css::code::semantic::SemanticTokenEvent&) override
    {
        m_xNotifier->Notify();
    }
    void SAL_CALL taskAdded(const css::code::task::Task&) override { m_xNotifier->Notify(); }
    void SAL_CALL taskRemoved(const OUString&) override { m_xNotifier->Notify(); }
    void SAL_CALL categoryCleared(const OUString&) override { m_xNotifier->Notify(); }
    void SAL_CALL tasksCleared() override { m_xNotifier->Notify(); }
    // the service went away; nothing to do, the panel is closing too
    void SAL_CALL disposing(const css::lang::EventObject&) override {}

private:
    std::shared_ptr<UiNotifier> m_xNotifier;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
