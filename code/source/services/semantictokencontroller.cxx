/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <com/sun/star/code/document/DocumentEvent.hpp>
#include <com/sun/star/code/document/XDocumentListener.hpp>
#include <com/sun/star/code/document/XTextDocument.hpp>
#include <com/sun/star/code/document/theDocumentManager.hpp>
#include <com/sun/star/code/semantic/SemanticTokenEvent.hpp>
#include <com/sun/star/code/semantic/XSemanticTokenController.hpp>
#include <com/sun/star/code/semantic/XSemanticTokenListener.hpp>
#include <com/sun/star/code/semantic/XSemanticTokenProvider.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/interfacecontainer4.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <rtl/ref.hxx>
#include <sal/log.hxx>

#include <algorithm>
#include <chrono>
#include <map>
#include <thread>
#include <vector>

using namespace css;
using namespace css::code;
using namespace css::code::semantic;

namespace codeservices
{
namespace
{
// How long a document must be left alone before it is analysed: typing a word
// is one analysis, not one per key.
constexpr std::chrono::milliseconds Pause(250);

class SemanticTokenController final
    : public comphelper::WeakComponentImplHelper<XSemanticTokenController, document::XDocumentListener,
                                                 lang::XServiceInfo>
{
public:
    explicit SemanticTokenController(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    // Subscribes to the documents; not in the constructor, which cannot hand out `this`.
    void Attach();

    // XSemanticTokenController
    void SAL_CALL registerProvider(const uno::Reference<XSemanticTokenProvider>& xProvider) override;
    void SAL_CALL unregisterProvider(const OUString& rName) override;
    uno::Sequence<uno::Reference<XSemanticTokenProvider>> SAL_CALL getProviders() override;
    SemanticTokens SAL_CALL getSemanticTokens(const OUString& rUrl) override;
    void SAL_CALL
    addSemanticTokenListener(const uno::Reference<XSemanticTokenListener>& xListener) override;
    void SAL_CALL
    removeSemanticTokenListener(const uno::Reference<XSemanticTokenListener>& xListener) override;

    // XDocumentListener
    void SAL_CALL documentChanged(const document::DocumentEvent& rEvent) override;
    void SAL_CALL disposing(const lang::EventObject&) override {}

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.SemanticTokenController"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

private:
    void disposing(std::unique_lock<std::mutex>& rGuard) override;

    // Starts the pause for a document; whatever was pending for it is outdated.
    void Schedule(const OUString& rUrl);
    void Analyse(const OUString& rUrl, sal_uInt64 nTicket);

    uno::Reference<uno::XComponentContext> m_xContext;
    std::vector<uno::Reference<XSemanticTokenProvider>> m_aProviders;
    std::map<OUString, SemanticTokens> m_aTokens;
    // The ticket of the newest request per document; an older one finds out it
    // was overtaken when its pause is over.
    std::map<OUString, sal_uInt64> m_aTickets;
    sal_uInt64 m_nLastTicket = 0;
    comphelper::OInterfaceContainerHelper4<XSemanticTokenListener> m_aListeners;
};

void SemanticTokenController::Attach()
{
    document::theDocumentManager::get(m_xContext)
        ->addDocumentListener(uno::Reference<document::XDocumentListener>(this));
    // Documents opened before we came into existence
    for (const auto& xDocument : document::theDocumentManager::get(m_xContext)->getOpenDocuments())
        Schedule(xDocument->getUrl());
}

void SemanticTokenController::Schedule(const OUString& rUrl)
{
    sal_uInt64 nTicket;
    {
        std::unique_lock aGuard(m_aMutex);
        if (m_bDisposed)
            return;
        nTicket = ++m_nLastTicket;
        m_aTickets[rUrl] = nTicket;
    }
    // The thread keeps the controller alive while it waits.
    std::thread(
        [xSelf = rtl::Reference<SemanticTokenController>(this), rUrl, nTicket]
        {
            std::this_thread::sleep_for(Pause);
            xSelf->Analyse(rUrl, nTicket);
        })
        .detach();
}

void SemanticTokenController::Analyse(const OUString& rUrl, sal_uInt64 nTicket)
{
    try
    {
        {
            std::unique_lock aGuard(m_aMutex);
            auto it = m_aTickets.find(rUrl);
            if (m_bDisposed || it == m_aTickets.end() || it->second != nTicket)
                return; // overtaken, or the document is gone
        }
        uno::Reference<document::XTextDocument> xDocument(
            document::theDocumentManager::get(m_xContext)->getDocument(rUrl), uno::UNO_QUERY);
        if (!xDocument.is())
            return;
        // text and version are read as a pair: an edit in between makes the
        // result outdated, which the ticket check below catches
        const OUString aText = xDocument->getText();
        const sal_Int32 nVersion = xDocument->getVersion();
        const OUString aMime = xDocument->getMimeType();

        uno::Reference<XSemanticTokenProvider> xProvider;
        {
            std::unique_lock aGuard(m_aMutex);
            for (const auto& xCandidate : m_aProviders)
            {
                const uno::Sequence<OUString> aTypes = xCandidate->getSupportedMimeTypes();
                if (std::find(aTypes.begin(), aTypes.end(), aMime) != aTypes.end())
                {
                    xProvider = xCandidate;
                    break;
                }
            }
        }
        if (!xProvider.is())
            return;

        SemanticTokens aResult = xProvider->provideSemanticTokens(rUrl, aText, nVersion);
        aResult.Version = nVersion;

        SemanticTokenEvent aEvent;
        aEvent.Source = getXWeak();
        aEvent.Url = rUrl;
        aEvent.Version = nVersion;
        std::unique_lock aGuard(m_aMutex);
        auto it = m_aTickets.find(rUrl);
        if (m_bDisposed || it == m_aTickets.end() || it->second != nTicket)
            return; // the document changed while the provider worked
        m_aTokens[rUrl] = std::move(aResult);
        m_aListeners.forEach(aGuard, [&](const uno::Reference<XSemanticTokenListener>& xListener)
                             { xListener->tokensChanged(aEvent); });
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code.services", "semantic tokens of " << rUrl << " failed: " << rException.Message);
    }
}

void SemanticTokenController::documentChanged(const document::DocumentEvent& rEvent)
{
    switch (rEvent.Kind)
    {
        case document::DocumentEventKind_CLOSED:
        {
            std::unique_lock aGuard(m_aMutex);
            m_aTickets.erase(rEvent.Url);
            m_aTokens.erase(rEvent.Url);
            break;
        }
        case document::DocumentEventKind_SAVED: // the text did not change
            break;
        default:
            Schedule(rEvent.Url);
    }
}

void SemanticTokenController::registerProvider(const uno::Reference<XSemanticTokenProvider>& xProvider)
{
    if (!xProvider.is())
        throw lang::IllegalArgumentException(u"null provider"_ustr, getXWeak(), 0);
    const OUString aName = xProvider->getName();
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        std::erase_if(m_aProviders, [&](const uno::Reference<XSemanticTokenProvider>& x)
                      { return x->getName() == aName; });
        m_aProviders.push_back(xProvider);
    }
    // what is open may be something the newcomer knows
    for (const auto& xDocument : document::theDocumentManager::get(m_xContext)->getOpenDocuments())
        Schedule(xDocument->getUrl());
}

void SemanticTokenController::unregisterProvider(const OUString& rName)
{
    std::unique_lock aGuard(m_aMutex);
    std::erase_if(m_aProviders, [&](const uno::Reference<XSemanticTokenProvider>& x)
                  { return x->getName() == rName; });
}

uno::Sequence<uno::Reference<XSemanticTokenProvider>> SemanticTokenController::getProviders()
{
    std::unique_lock aGuard(m_aMutex);
    return comphelper::containerToSequence(m_aProviders);
}

SemanticTokens SemanticTokenController::getSemanticTokens(const OUString& rUrl)
{
    std::unique_lock aGuard(m_aMutex);
    auto it = m_aTokens.find(rUrl);
    if (it != m_aTokens.end())
        return it->second;
    SemanticTokens aNone;
    aNone.Version = -1;
    return aNone;
}

void SemanticTokenController::addSemanticTokenListener(
    const uno::Reference<XSemanticTokenListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.addInterface(aGuard, xListener);
}

void SemanticTokenController::removeSemanticTokenListener(
    const uno::Reference<XSemanticTokenListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.removeInterface(aGuard, xListener);
}

void SemanticTokenController::disposing(std::unique_lock<std::mutex>& rGuard)
{
    lang::EventObject aSource(getXWeak());
    m_aListeners.disposeAndClear(rGuard, aSource);
    m_aProviders.clear();
    m_aTokens.clear();
    m_aTickets.clear();
    comphelper::WeakComponentImplHelperBase::disposing(rGuard);
}
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_SemanticTokenController_get_implementation(uno::XComponentContext* pContext,
                                                                   uno::Sequence<uno::Any> const&)
{
    rtl::Reference<codeservices::SemanticTokenController> xController(
        new codeservices::SemanticTokenController(pContext));
    xController->Attach();
    return cppu::acquire(xController.get());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
