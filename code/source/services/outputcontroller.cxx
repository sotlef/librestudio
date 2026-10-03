/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <com/sun/star/code/output/XOutputChannel.hpp>
#include <com/sun/star/code/output/XOutputController.hpp>
#include <com/sun/star/code/output/XOutputFilter.hpp>
#include <com/sun/star/code/output/XOutputListener.hpp>
#include <com/sun/star/code/task/theTaskHub.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <osl/time.h>
#include <rtl/ref.hxx>
#include <sal/log.hxx>

#include <algorithm>
#include <deque>
#include <vector>

using namespace css;
using namespace css::code::output;

namespace codeservices
{
namespace
{
class OutputController;

util::DateTime Now()
{
    TimeValue aSystem;
    TimeValue aLocal;
    oslDateTime aDate{};
    osl_getSystemTime(&aSystem);
    if (osl_getLocalTimeFromSystemTime(&aSystem, &aLocal))
        osl_getDateTimeFromTimeValue(&aLocal, &aDate);
    else
        osl_getDateTimeFromTimeValue(&aSystem, &aDate);
    return util::DateTime(aDate.NanoSeconds, aDate.Seconds, aDate.Minutes, aDate.Hours, aDate.Day,
                          aDate.Month, aDate.Year, false);
}

// One channel.  Only the most recent MaxLines lines are kept; a build log can
// run to millions.
class OutputChannel final : public comphelper::WeakComponentImplHelper<XOutputChannel>
{
public:
    static constexpr size_t MaxLines = 5000;

    OutputChannel(rtl::Reference<OutputController> xOwner, OUString aName, OUString aDisplayName)
        : m_xOwner(std::move(xOwner))
        , m_aName(std::move(aName))
        , m_aDisplayName(std::move(aDisplayName))
    {
    }

    // XOutputChannel
    OUString SAL_CALL getName() override { return m_aName; }
    OUString SAL_CALL getDisplayName() override { return m_aDisplayName; }
    sal_Int32 SAL_CALL getLineCount() override
    {
        std::unique_lock aGuard(m_aMutex);
        return static_cast<sal_Int32>(m_aLines.size());
    }
    void SAL_CALL appendLine(const OUString& rText, OutputStream eStream) override;
    void SAL_CALL clear() override;
    uno::Sequence<OutputLine> SAL_CALL getContent() override
    {
        std::unique_lock aGuard(m_aMutex);
        return comphelper::containerToSequence<OutputLine>(m_aLines);
    }

    // Breaks the owner<->channel reference cycle.
    void detach()
    {
        std::unique_lock aGuard(m_aMutex);
        m_xOwner.clear();
    }

private:
    rtl::Reference<OutputController> m_xOwner;
    const OUString m_aName;
    const OUString m_aDisplayName;
    std::deque<OutputLine> m_aLines;
};

class OutputController final
    : public comphelper::WeakComponentImplHelper<XOutputController, lang::XServiceInfo>
{
public:
    explicit OutputController(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    // XOutputController
    uno::Reference<XOutputChannel> SAL_CALL getOrCreateChannel(const OUString& rName,
                                                               const OUString& rDisplayName) override;
    uno::Reference<XOutputChannel> SAL_CALL getChannel(const OUString& rName) override;
    uno::Sequence<uno::Reference<XOutputChannel>> SAL_CALL getChannels() override;
    void SAL_CALL registerFilter(const uno::Reference<XOutputFilter>& xFilter) override;
    void SAL_CALL unregisterFilter(const OUString& rName) override;
    void SAL_CALL addOutputListener(const uno::Reference<XOutputListener>& xListener) override;
    void SAL_CALL removeOutputListener(const uno::Reference<XOutputListener>& xListener) override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.OutputController"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

    // called by the channels, without the channel's lock held
    void lineAppended(const OUString& rChannel, const OutputLine& rLine);
    void channelCleared(const OUString& rChannel);

private:
    void disposing(std::unique_lock<std::mutex>& rGuard) override;

    uno::Reference<uno::XComponentContext> m_xContext;
    std::vector<rtl::Reference<OutputChannel>> m_aChannels;
    std::vector<uno::Reference<XOutputFilter>> m_aFilters;
    comphelper::OInterfaceContainerHelper4<XOutputListener> m_aListeners;
};

void OutputChannel::appendLine(const OUString& rText, OutputStream eStream)
{
    OutputLine aLine;
    aLine.Text = rText;
    aLine.Stream = eStream;
    aLine.Time = Now();

    rtl::Reference<OutputController> xOwner;
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        m_aLines.push_back(aLine);
        if (m_aLines.size() > MaxLines)
            m_aLines.pop_front();
        xOwner = m_xOwner;
    }
    if (xOwner.is())
        xOwner->lineAppended(m_aName, aLine);
}

void OutputChannel::clear()
{
    rtl::Reference<OutputController> xOwner;
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        m_aLines.clear();
        xOwner = m_xOwner;
    }
    if (xOwner.is())
        xOwner->channelCleared(m_aName);
}

uno::Reference<XOutputChannel> OutputController::getOrCreateChannel(const OUString& rName,
                                                                    const OUString& rDisplayName)
{
    rtl::Reference<OutputChannel> xNew;
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        for (const auto& xChannel : m_aChannels)
            if (xChannel->getName() == rName)
                return uno::Reference<XOutputChannel>(xChannel.get());
        xNew = new OutputChannel(this, rName, rDisplayName.isEmpty() ? rName : rDisplayName);
        m_aChannels.push_back(xNew);
        m_aListeners.forEach(aGuard, [&](const uno::Reference<XOutputListener>& xListener)
                             { xListener->channelAdded(rName); });
    }
    return uno::Reference<XOutputChannel>(xNew.get());
}

uno::Reference<XOutputChannel> OutputController::getChannel(const OUString& rName)
{
    std::unique_lock aGuard(m_aMutex);
    for (const auto& xChannel : m_aChannels)
        if (xChannel->getName() == rName)
            return uno::Reference<XOutputChannel>(xChannel.get());
    return {};
}

uno::Sequence<uno::Reference<XOutputChannel>> OutputController::getChannels()
{
    std::unique_lock aGuard(m_aMutex);
    uno::Sequence<uno::Reference<XOutputChannel>> aResult(m_aChannels.size());
    std::transform(m_aChannels.begin(), m_aChannels.end(), aResult.getArray(),
                   [](const rtl::Reference<OutputChannel>& x)
                   { return uno::Reference<XOutputChannel>(x.get()); });
    return aResult;
}

void OutputController::registerFilter(const uno::Reference<XOutputFilter>& xFilter)
{
    if (!xFilter.is())
        throw lang::IllegalArgumentException(u"null filter"_ustr, getXWeak(), 0);
    const OUString aName = xFilter->getName();

    std::unique_lock aGuard(m_aMutex);
    throwIfDisposed(aGuard);
    std::erase_if(m_aFilters,
                  [&](const uno::Reference<XOutputFilter>& x) { return x->getName() == aName; });
    m_aFilters.push_back(xFilter);
}

void OutputController::unregisterFilter(const OUString& rName)
{
    std::unique_lock aGuard(m_aMutex);
    std::erase_if(m_aFilters,
                  [&](const uno::Reference<XOutputFilter>& x) { return x->getName() == rName; });
}

void OutputController::addOutputListener(const uno::Reference<XOutputListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.addInterface(aGuard, xListener);
}

void OutputController::removeOutputListener(const uno::Reference<XOutputListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.removeInterface(aGuard, xListener);
}

void OutputController::lineAppended(const OUString& rChannel, const OutputLine& rLine)
{
    std::vector<uno::Reference<XOutputFilter>> aFilters;
    {
        std::unique_lock aGuard(m_aMutex);
        aFilters = m_aFilters;
        m_aListeners.forEach(aGuard, [&](const uno::Reference<XOutputListener>& xListener)
                             { xListener->lineAppended(rChannel, rLine); });
    }

    // The filters turn recognised lines into Error List entries.  Resolved
    // here rather than at construction: the hub need not exist yet.
    uno::Reference<css::code::task::XTaskHub> xHub;
    for (const auto& xFilter : aFilters)
    {
        try
        {
            const uno::Sequence<OUString> aApplicable = xFilter->getApplicableChannels();
            if (aApplicable.hasElements()
                && std::find(aApplicable.begin(), aApplicable.end(), rChannel) == aApplicable.end())
                continue;

            const uno::Sequence<css::code::task::Task> aTasks = xFilter->filterLine(rChannel, rLine);
            if (!aTasks.hasElements())
                continue;
            if (!xHub.is())
                xHub = css::code::task::theTaskHub::get(m_xContext);
            for (const auto& rTask : aTasks)
                xHub->addTask(rTask);
        }
        catch (const uno::Exception& rException)
        {
            SAL_WARN("code.services", "output filter failed: " << rException.Message);
        }
    }
}

void OutputController::channelCleared(const OUString& rChannel)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.forEach(aGuard, [&](const uno::Reference<XOutputListener>& xListener)
                         { xListener->channelCleared(rChannel); });
}

void OutputController::disposing(std::unique_lock<std::mutex>& rGuard)
{
    lang::EventObject aSource(getXWeak());
    m_aListeners.disposeAndClear(rGuard, aSource);
    for (const auto& xChannel : m_aChannels)
        xChannel->detach();
    m_aChannels.clear();
    m_aFilters.clear();
}
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_OutputController_get_implementation(uno::XComponentContext* pContext,
                                                            uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::OutputController(pContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
