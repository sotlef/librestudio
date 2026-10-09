/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <com/sun/star/code/complete/CompletionRequest.hpp>
#include <com/sun/star/code/complete/CompletionResult.hpp>
#include <com/sun/star/code/complete/XCompletionController.hpp>
#include <com/sun/star/code/complete/XCompletionProvider.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <rtl/ref.hxx>
#include <sal/log.hxx>

#include <vector>

using namespace css;
using namespace css::code;
using namespace css::code::complete;

namespace codeservices
{
namespace
{
// The registry and the router of completion: the editor asks here, the
// provider for the document answers.  Unlike the semantic-token controller it
// keeps nothing per document and schedules nothing -- completion is a
// question, not a subscription -- so a request is served on the thread it
// arrives on, with no lock held while a provider works.
class CompletionController final
    : public comphelper::WeakComponentImplHelper<XCompletionController, lang::XServiceInfo>
{
public:
    // XCompletionController
    void SAL_CALL registerProvider(const uno::Reference<XCompletionProvider>& xProvider) override;
    void SAL_CALL unregisterProvider(const OUString& rName) override;
    uno::Sequence<uno::Reference<XCompletionProvider>> SAL_CALL getProviders() override;
    CompletionResult SAL_CALL complete(const CompletionRequest& rRequest) override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.CompletionController"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

private:
    void disposing(std::unique_lock<std::mutex>& rGuard) override;

    std::vector<uno::Reference<XCompletionProvider>> m_aProviders;
};

void CompletionController::registerProvider(const uno::Reference<XCompletionProvider>& xProvider)
{
    if (!xProvider.is())
        throw lang::IllegalArgumentException(u"null provider"_ustr, getXWeak(), 0);
    const OUString aName = xProvider->getName();
    std::unique_lock aGuard(m_aMutex);
    throwIfDisposed(aGuard);
    std::erase_if(m_aProviders, [&](const uno::Reference<XCompletionProvider>& x)
                  { return x->getName() == aName; });
    m_aProviders.push_back(xProvider);
}

void CompletionController::unregisterProvider(const OUString& rName)
{
    std::unique_lock aGuard(m_aMutex);
    std::erase_if(m_aProviders, [&](const uno::Reference<XCompletionProvider>& x)
                  { return x->getName() == rName; });
}

uno::Sequence<uno::Reference<XCompletionProvider>> CompletionController::getProviders()
{
    std::unique_lock aGuard(m_aMutex);
    return comphelper::containerToSequence(m_aProviders);
}

CompletionResult CompletionController::complete(const CompletionRequest& rRequest)
{
    std::vector<uno::Reference<XCompletionProvider>> aProviders;
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        aProviders = m_aProviders;
    }
    for (const auto& xProvider : aProviders)
    {
        try
        {
            if (!xProvider->supports(rRequest.Url))
                continue;
            return xProvider->complete(rRequest);
        }
        catch (const uno::Exception& rException)
        {
            SAL_WARN("code.services",
                     "completion of " << rRequest.Url << " failed: " << rException.Message);
            break;
        }
    }
    CompletionResult aNone;
    aNone.Incomplete = false;
    return aNone;
}

void CompletionController::disposing(std::unique_lock<std::mutex>& rGuard)
{
    m_aProviders.clear();
    comphelper::WeakComponentImplHelperBase::disposing(rGuard);
}
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_CompletionController_get_implementation(uno::XComponentContext*,
                                                                uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::CompletionController);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
