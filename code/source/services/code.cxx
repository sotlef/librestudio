/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <com/sun/star/code/core/XCode.hpp>
#include <com/sun/star/code/core/XPlugin.hpp>
#include <com/sun/star/container/XContentEnumerationAccess.hpp>
#include <com/sun/star/container/XEnumeration.hpp>
#include <com/sun/star/lang/XMultiComponentFactory.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XSingleComponentFactory.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <sal/log.hxx>

#include <vector>

using namespace css;
using namespace css::code::core;

namespace codeservices
{
namespace
{
class Code final : public comphelper::WeakComponentImplHelper<XCode, lang::XServiceInfo>
{
public:
    explicit Code(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    // XCode
    OUString SAL_CALL getVersion() override { return u"0.1"_ustr; }
    uno::Reference<css::code::frontend::XWorkbench> SAL_CALL getWorkbench() override
    {
        std::unique_lock aGuard(m_aMutex);
        return m_xWorkbench;
    }
    void SAL_CALL setWorkbench(const uno::Reference<css::code::frontend::XWorkbench>& xWorkbench) override
    {
        std::unique_lock aGuard(m_aMutex);
        m_xWorkbench = xWorkbench;
    }
    void SAL_CALL activatePlugins() override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.Code"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

private:
    void disposing(std::unique_lock<std::mutex>& rGuard) override;

    uno::Reference<uno::XComponentContext> m_xContext;
    uno::Reference<css::code::frontend::XWorkbench> m_xWorkbench;
    bool m_bPluginsActivated = false;
    // Plugins stay alive for as long as the IDE does.  Most also hang on to
    // themselves by registering with a controller, but one that only listens
    // to something weakly must not vanish.
    std::vector<uno::Reference<XPlugin>> m_aPlugins;
};

void Code::activatePlugins()
{
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        if (m_bPluginsActivated)
            return;
        m_bPluginsActivated = true;
    }

    uno::Reference<container::XContentEnumerationAccess> xAccess(m_xContext->getServiceManager(),
                                                                 uno::UNO_QUERY_THROW);
    uno::Reference<container::XEnumeration> xEnum
        = xAccess->createContentEnumeration(u"com.sun.star.code.core.Plugin"_ustr);
    // No plugin registered at all: the enumeration is empty, not an error.
    if (!xEnum.is())
        return;

    while (xEnum->hasMoreElements())
    {
        // The elements are factories, not the plugins themselves.
        uno::Reference<uno::XInterface> xFactory;
        xEnum->nextElement() >>= xFactory;

        try
        {
            uno::Reference<uno::XInterface> xInstance;
            if (uno::Reference<lang::XSingleComponentFactory> xCtx{ xFactory, uno::UNO_QUERY })
                xInstance = xCtx->createInstanceWithContext(m_xContext);
            else if (uno::Reference<lang::XSingleServiceFactory> xSvc{ xFactory, uno::UNO_QUERY })
                xInstance = xSvc->createInstance();

            uno::Reference<XPlugin> xPlugin(xInstance, uno::UNO_QUERY);
            if (!xPlugin.is())
                continue;
            xPlugin->activate();

            std::unique_lock aGuard(m_aMutex);
            m_aPlugins.push_back(xPlugin);
        }
        catch (const uno::Exception& rException)
        {
            // One broken plugin must not take the others down with it -- but
            // it must not vanish silently either (deepcode swallowed these).
            SAL_WARN("code.services", "activating a plugin failed: " << rException.Message);
        }
    }
}

void Code::disposing(std::unique_lock<std::mutex>& /*rGuard*/)
{
    m_xWorkbench.clear();
    m_aPlugins.clear();
}
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_Code_get_implementation(uno::XComponentContext* pContext,
                                                uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::Code(pContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
