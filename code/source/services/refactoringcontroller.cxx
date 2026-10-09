/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <com/sun/star/code/refactor/XRefactoringController.hpp>
#include <com/sun/star/code/refactor/XRefactoringProvider.hpp>
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
using namespace css::code::refactor;

namespace codeservices
{
namespace
{
// The registry and the router of refactoring: the editor asks here which
// engine understands a document, and talks to that engine directly from then
// on (queryInterface for XRenameProvider and whatever capabilities come
// later).  Like the completion controller it keeps nothing per document and
// schedules nothing -- a refactoring is a question, not a subscription -- so
// no lock is held while a provider works.
class RefactoringController final
    : public comphelper::WeakComponentImplHelper<XRefactoringController, lang::XServiceInfo>
{
public:
    // XRefactoringController
    void SAL_CALL registerProvider(const uno::Reference<XRefactoringProvider>& xProvider) override;
    void SAL_CALL unregisterProvider(const OUString& rName) override;
    uno::Sequence<uno::Reference<XRefactoringProvider>> SAL_CALL getProviders() override;
    uno::Reference<XRefactoringProvider> SAL_CALL findProvider(const OUString& rUrl) override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.RefactoringController"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

private:
    void disposing(std::unique_lock<std::mutex>& rGuard) override;

    std::vector<uno::Reference<XRefactoringProvider>> m_aProviders;
};

void RefactoringController::registerProvider(const uno::Reference<XRefactoringProvider>& xProvider)
{
    if (!xProvider.is())
        throw lang::IllegalArgumentException(u"null provider"_ustr, getXWeak(), 0);
    const OUString aName = xProvider->getName();
    std::unique_lock aGuard(m_aMutex);
    throwIfDisposed(aGuard);
    std::erase_if(m_aProviders, [&](const uno::Reference<XRefactoringProvider>& x)
                  { return x->getName() == aName; });
    m_aProviders.push_back(xProvider);
}

void RefactoringController::unregisterProvider(const OUString& rName)
{
    std::unique_lock aGuard(m_aMutex);
    std::erase_if(m_aProviders, [&](const uno::Reference<XRefactoringProvider>& x)
                  { return x->getName() == rName; });
}

uno::Sequence<uno::Reference<XRefactoringProvider>> RefactoringController::getProviders()
{
    std::unique_lock aGuard(m_aMutex);
    return comphelper::containerToSequence(m_aProviders);
}

uno::Reference<XRefactoringProvider> RefactoringController::findProvider(const OUString& rUrl)
{
    std::vector<uno::Reference<XRefactoringProvider>> aProviders;
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        aProviders = m_aProviders;
    }
    for (const auto& xProvider : aProviders)
    {
        try
        {
            if (xProvider->supports(rUrl))
                return xProvider;
        }
        catch (const uno::Exception& rException)
        {
            SAL_WARN("code.services", "refactoring provider failed on " << rUrl << ": "
                                                                        << rException.Message);
        }
    }
    return {};
}

void RefactoringController::disposing(std::unique_lock<std::mutex>& rGuard)
{
    m_aProviders.clear();
    comphelper::WeakComponentImplHelperBase::disposing(rGuard);
}
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_RefactoringController_get_implementation(uno::XComponentContext*,
                                                                 uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::RefactoringController);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
