/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "uibridge.hxx"

namespace code
{
void UiNotifier::Notify()
{
    {
        std::scoped_lock aGuard(m_aMutex);
        if (!m_aFunction || m_bPending)
            return;
        m_bPending = true;
    }
    // The posted event owns a reference, so the notifier outlives a panel
    // that is destroyed while the event is still queued.
    auto* pKeepAlive = new std::shared_ptr<UiNotifier>(shared_from_this());
    if (!Application::PostUserEvent(LINK(nullptr, UiNotifier, RunHdl), pKeepAlive))
    {
        delete pKeepAlive;
        std::scoped_lock aGuard(m_aMutex);
        m_bPending = false;
    }
}

IMPL_STATIC_LINK(UiNotifier, RunHdl, void*, pData, void)
{
    std::unique_ptr<std::shared_ptr<UiNotifier>> xSelf(
        static_cast<std::shared_ptr<UiNotifier>*>(pData));
    std::function<void()> aFunction;
    {
        std::scoped_lock aGuard((*xSelf)->m_aMutex);
        (*xSelf)->m_bPending = false;
        aFunction = (*xSelf)->m_aFunction;
    }
    if (aFunction)
        aFunction();
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
