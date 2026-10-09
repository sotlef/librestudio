/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// theSyntaxAnalyzers: the parsers that are around, nothing more.  Unlike the
// symbol index and the token controller this registry watches no documents and
// keeps no results: whoever wants a tree picks an analyzer and parses, on a
// thread of its own choosing.

#include <com/sun/star/code/syntax/XSyntaxAnalyzer.hpp>
#include <com/sun/star/code/syntax/XSyntaxAnalyzers.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <sal/log.hxx>

#include <algorithm>
#include <vector>

using namespace css;
using namespace css::code::syntax;

namespace codeservices
{
namespace
{
class SyntaxAnalyzers final
    : public comphelper::WeakComponentImplHelper<XSyntaxAnalyzers, lang::XServiceInfo>
{
public:
    // XSyntaxAnalyzers
    void SAL_CALL registerAnalyzer(const uno::Reference<XSyntaxAnalyzer>& xAnalyzer) override;
    void SAL_CALL unregisterAnalyzer(const OUString& rName) override;
    uno::Sequence<uno::Reference<XSyntaxAnalyzer>> SAL_CALL getAnalyzers() override;
    uno::Reference<XSyntaxAnalyzer> SAL_CALL findAnalyzer(const OUString& rUrl) override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.SyntaxAnalyzers"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

private:
    void disposing(std::unique_lock<std::mutex>& rGuard) override
    {
        m_aAnalyzers.clear();
        comphelper::WeakComponentImplHelperBase::disposing(rGuard);
    }

    std::vector<uno::Reference<XSyntaxAnalyzer>> m_aAnalyzers;
};

void SyntaxAnalyzers::registerAnalyzer(const uno::Reference<XSyntaxAnalyzer>& xAnalyzer)
{
    if (!xAnalyzer.is())
        throw lang::IllegalArgumentException(u"null analyzer"_ustr, getXWeak(), 0);
    const OUString aName = xAnalyzer->getName();

    std::unique_lock aGuard(m_aMutex);
    throwIfDisposed(aGuard);
    std::erase_if(m_aAnalyzers,
                  [&](const uno::Reference<XSyntaxAnalyzer>& x) { return x->getName() == aName; });
    m_aAnalyzers.push_back(xAnalyzer);
}

void SyntaxAnalyzers::unregisterAnalyzer(const OUString& rName)
{
    std::unique_lock aGuard(m_aMutex);
    std::erase_if(m_aAnalyzers,
                  [&](const uno::Reference<XSyntaxAnalyzer>& x) { return x->getName() == rName; });
}

uno::Sequence<uno::Reference<XSyntaxAnalyzer>> SyntaxAnalyzers::getAnalyzers()
{
    std::unique_lock aGuard(m_aMutex);
    return comphelper::containerToSequence(m_aAnalyzers);
}

uno::Reference<XSyntaxAnalyzer> SyntaxAnalyzers::findAnalyzer(const OUString& rUrl)
{
    std::vector<uno::Reference<XSyntaxAnalyzer>> aAnalyzers;
    {
        std::unique_lock aGuard(m_aMutex);
        aAnalyzers = m_aAnalyzers;
    }
    // supports() is the analyzers' own code: outside the lock, and one that
    // throws is skipped, not allowed to hide the analyzers behind it.
    for (const auto& xAnalyzer : aAnalyzers)
    {
        try
        {
            if (xAnalyzer->supports(rUrl))
                return xAnalyzer;
        }
        catch (const uno::Exception& rException)
        {
            SAL_WARN("code.services", "syntax analyzer " << xAnalyzer->getName()
                                                         << " failed to probe: " << rException.Message);
        }
    }
    return {};
}
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_SyntaxAnalyzers_get_implementation(uno::XComponentContext*,
                                                           uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::SyntaxAnalyzers);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
