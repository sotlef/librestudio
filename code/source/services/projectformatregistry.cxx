/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <com/sun/star/code/project/XProjectFormatReader.hpp>
#include <com/sun/star/code/project/XProjectFormatRegistry.hpp>
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
using namespace css::code::project;

namespace codeservices
{
namespace
{
class ProjectFormatRegistry final
    : public comphelper::WeakComponentImplHelper<XProjectFormatRegistry, lang::XServiceInfo>
{
public:
    // XProjectFormatRegistry
    void SAL_CALL registerReader(const uno::Reference<XProjectFormatReader>& xReader) override;
    void SAL_CALL unregisterReader(const OUString& rFormatKind) override;
    uno::Sequence<uno::Reference<XProjectFormatReader>> SAL_CALL getReaders() override;
    uno::Reference<XProjectFormatReader> SAL_CALL findReader(const OUString& rUrl) override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.ProjectFormatRegistry"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

private:
    void disposing(std::unique_lock<std::mutex>& rGuard) override
    {
        m_aReaders.clear();
        comphelper::WeakComponentImplHelperBase::disposing(rGuard);
    }

    // kept ordered by decreasing priority, equal priorities in registration order
    std::vector<uno::Reference<XProjectFormatReader>> m_aReaders;
};

void ProjectFormatRegistry::registerReader(const uno::Reference<XProjectFormatReader>& xReader)
{
    if (!xReader.is())
        throw lang::IllegalArgumentException(u"null reader"_ustr, getXWeak(), 0);
    const OUString aKind = xReader->getFormatKind();
    const sal_Int32 nPriority = xReader->getPriority();

    std::unique_lock aGuard(m_aMutex);
    throwIfDisposed(aGuard);
    std::erase_if(m_aReaders, [&](const uno::Reference<XProjectFormatReader>& x)
                  { return x->getFormatKind() == aKind; });
    auto it = std::find_if(m_aReaders.begin(), m_aReaders.end(),
                           [&](const uno::Reference<XProjectFormatReader>& x)
                           { return x->getPriority() < nPriority; });
    m_aReaders.insert(it, xReader);
}

void ProjectFormatRegistry::unregisterReader(const OUString& rFormatKind)
{
    std::unique_lock aGuard(m_aMutex);
    std::erase_if(m_aReaders, [&](const uno::Reference<XProjectFormatReader>& x)
                  { return x->getFormatKind() == rFormatKind; });
}

uno::Sequence<uno::Reference<XProjectFormatReader>> ProjectFormatRegistry::getReaders()
{
    std::unique_lock aGuard(m_aMutex);
    return comphelper::containerToSequence(m_aReaders);
}

uno::Reference<XProjectFormatReader> ProjectFormatRegistry::findReader(const OUString& rUrl)
{
    std::vector<uno::Reference<XProjectFormatReader>> aReaders;
    {
        std::unique_lock aGuard(m_aMutex);
        aReaders = m_aReaders;
    }
    // canRead() is the readers' own code: outside the lock, and one that throws
    // is skipped, not allowed to hide the readers behind it.
    for (const auto& xReader : aReaders)
    {
        try
        {
            if (xReader->canRead(rUrl))
                return xReader;
        }
        catch (const uno::Exception& rException)
        {
            SAL_WARN("code.services", "project reader " << xReader->getFormatKind()
                                                        << " failed to probe: " << rException.Message);
        }
    }
    return {};
}
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_ProjectFormatRegistry_get_implementation(uno::XComponentContext*,
                                                                 uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::ProjectFormatRegistry);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
