/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <com/sun/star/code/task/XTaskHandler.hpp>
#include <com/sun/star/code/task/XTaskHub.hpp>
#include <com/sun/star/code/task/XTaskListener.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <sal/log.hxx>

#include <algorithm>
#include <iterator>
#include <vector>

using namespace css;
using namespace css::code::task;

namespace codeservices
{
namespace
{
class TaskHub final : public comphelper::WeakComponentImplHelper<XTaskHub, lang::XServiceInfo>
{
public:
    // XTaskHub
    OUString SAL_CALL addTask(const Task& rTask) override;
    void SAL_CALL removeTask(const OUString& rIdentifier) override;
    void SAL_CALL clearCategory(const OUString& rCategory) override;
    void SAL_CALL clearAll() override;
    uno::Sequence<Task> SAL_CALL getTasks() override;
    uno::Sequence<Task> SAL_CALL getTasksByCategory(const OUString& rCategory) override;
    uno::Sequence<Task> SAL_CALL getTasksByUrl(const OUString& rUrl) override;
    sal_Int32 SAL_CALL countBySeverity(css::code::core::Severity eLevel) override;
    void SAL_CALL registerHandler(const uno::Reference<XTaskHandler>& xHandler) override;
    void SAL_CALL unregisterHandler(const OUString& rName) override;
    sal_Bool SAL_CALL handleTask(const OUString& rIdentifier) override;
    void SAL_CALL addTaskListener(const uno::Reference<XTaskListener>& xListener) override;
    void SAL_CALL removeTaskListener(const uno::Reference<XTaskListener>& xListener) override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.TaskHub"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

private:
    void disposing(std::unique_lock<std::mutex>& rGuard) override;

    template <typename Pred> uno::Sequence<Task> select(Pred aPred);

    std::vector<Task> m_aTasks;
    std::vector<uno::Reference<XTaskHandler>> m_aHandlers;
    sal_Int64 m_nNextId = 1;
    comphelper::OInterfaceContainerHelper4<XTaskListener> m_aListeners;
};

template <typename Pred> uno::Sequence<Task> TaskHub::select(Pred aPred)
{
    std::unique_lock aGuard(m_aMutex);
    std::vector<Task> aResult;
    std::copy_if(m_aTasks.begin(), m_aTasks.end(), std::back_inserter(aResult), aPred);
    return comphelper::containerToSequence(aResult);
}

OUString TaskHub::addTask(const Task& rTask)
{
    Task aTask(rTask);
    std::unique_lock aGuard(m_aMutex);
    throwIfDisposed(aGuard);
    aTask.Identifier = "task-" + OUString::number(m_nNextId++);
    m_aTasks.push_back(aTask);
    m_aListeners.forEach(aGuard, [&](const uno::Reference<XTaskListener>& xListener)
                         { xListener->taskAdded(aTask); });
    return aTask.Identifier;
}

void TaskHub::removeTask(const OUString& rIdentifier)
{
    std::unique_lock aGuard(m_aMutex);
    if (std::erase_if(m_aTasks, [&](const Task& r) { return r.Identifier == rIdentifier; }) == 0)
        return;
    m_aListeners.forEach(aGuard, [&](const uno::Reference<XTaskListener>& xListener)
                         { xListener->taskRemoved(rIdentifier); });
}

void TaskHub::clearCategory(const OUString& rCategory)
{
    std::unique_lock aGuard(m_aMutex);
    if (std::erase_if(m_aTasks, [&](const Task& r) { return r.Category == rCategory; }) == 0)
        return;
    m_aListeners.forEach(aGuard, [&](const uno::Reference<XTaskListener>& xListener)
                         { xListener->categoryCleared(rCategory); });
}

void TaskHub::clearAll()
{
    std::unique_lock aGuard(m_aMutex);
    if (m_aTasks.empty())
        return;
    m_aTasks.clear();
    m_aListeners.forEach(aGuard, [](const uno::Reference<XTaskListener>& xListener)
                         { xListener->tasksCleared(); });
}

uno::Sequence<Task> TaskHub::getTasks()
{
    return select([](const Task&) { return true; });
}

uno::Sequence<Task> TaskHub::getTasksByCategory(const OUString& rCategory)
{
    return select([&](const Task& r) { return r.Category == rCategory; });
}

uno::Sequence<Task> TaskHub::getTasksByUrl(const OUString& rUrl)
{
    return select([&](const Task& r) { return r.Url == rUrl; });
}

sal_Int32 TaskHub::countBySeverity(css::code::core::Severity eLevel)
{
    std::unique_lock aGuard(m_aMutex);
    return static_cast<sal_Int32>(
        std::count_if(m_aTasks.begin(), m_aTasks.end(), [&](const Task& r) { return r.Level == eLevel; }));
}

void TaskHub::registerHandler(const uno::Reference<XTaskHandler>& xHandler)
{
    if (!xHandler.is())
        throw lang::IllegalArgumentException(u"null handler"_ustr, getXWeak(), 0);
    const OUString aName = xHandler->getName();

    std::unique_lock aGuard(m_aMutex);
    throwIfDisposed(aGuard);
    // Same name replaces; a later registration is tried first, so a plugin can
    // override a built-in handler.
    std::erase_if(m_aHandlers, [&](const uno::Reference<XTaskHandler>& x)
                  { return x->getName() == aName; });
    m_aHandlers.insert(m_aHandlers.begin(), xHandler);
}

void TaskHub::unregisterHandler(const OUString& rName)
{
    std::unique_lock aGuard(m_aMutex);
    std::erase_if(m_aHandlers, [&](const uno::Reference<XTaskHandler>& x)
                  { return x->getName() == rName; });
}

sal_Bool TaskHub::handleTask(const OUString& rIdentifier)
{
    Task aTask;
    std::vector<uno::Reference<XTaskHandler>> aHandlers;
    {
        std::unique_lock aGuard(m_aMutex);
        auto it = std::find_if(m_aTasks.begin(), m_aTasks.end(),
                               [&](const Task& r) { return r.Identifier == rIdentifier; });
        if (it == m_aTasks.end())
            return false;
        aTask = *it;
        aHandlers = m_aHandlers;
    }

    // Handlers run outside the lock: opening a file re-enters the IDE.
    for (const auto& xHandler : aHandlers)
    {
        const uno::Sequence<OUString> aCategories = xHandler->getCategories();
        if (aCategories.hasElements()
            && std::find(aCategories.begin(), aCategories.end(), aTask.Category)
                   == aCategories.end())
            continue;
        if (!xHandler->canHandle(aTask))
            continue;
        xHandler->handle(aTask);
        return true;
    }
    return false;
}

void TaskHub::addTaskListener(const uno::Reference<XTaskListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.addInterface(aGuard, xListener);
}

void TaskHub::removeTaskListener(const uno::Reference<XTaskListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.removeInterface(aGuard, xListener);
}

void TaskHub::disposing(std::unique_lock<std::mutex>& rGuard)
{
    lang::EventObject aSource(getXWeak());
    m_aListeners.disposeAndClear(rGuard, aSource);
    m_aTasks.clear();
    m_aHandlers.clear();
}
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_TaskHub_get_implementation(uno::XComponentContext*,
                                                   uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::TaskHub);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
