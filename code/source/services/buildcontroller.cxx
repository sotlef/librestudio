/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "common.hxx"

#include <com/sun/star/code/build/XBuildController.hpp>
#include <com/sun/star/code/build/XBuildable.hpp>
#include <com/sun/star/code/build/XCompilable.hpp>
#include <com/sun/star/code/core/XBackgroundJob.hpp>
#include <com/sun/star/code/core/XJobRegistry.hpp>
#include <com/sun/star/code/core/theJobRegistry.hpp>
#include <com/sun/star/code/project/ProjectConfiguration.hpp>
#include <com/sun/star/code/project/XProject.hpp>
#include <com/sun/star/code/task/XTaskHub.hpp>
#include <com/sun/star/code/task/theTaskHub.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>

#include <functional>
#include <mutex>

using namespace css;
using namespace css::code;

namespace codeservices
{
namespace
{
class BuildController final : public cppu::WeakImplHelper<build::XBuildController, lang::XServiceInfo>
{
public:
    explicit BuildController(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    // XBuildController
    uno::Reference<core::XBackgroundJob> SAL_CALL build(const uno::Reference<build::XBuildable>& xItem) override
    {
        return start([&] { return xItem->build(); });
    }
    uno::Reference<core::XBackgroundJob> SAL_CALL rebuild(const uno::Reference<build::XBuildable>& xItem) override
    {
        return start([&] { return xItem->rebuild(); });
    }
    uno::Reference<core::XBackgroundJob> SAL_CALL clean(const uno::Reference<build::XBuildable>& xItem) override
    {
        return start([&] { return xItem->clean(); });
    }
    uno::Reference<core::XBackgroundJob> SAL_CALL compile(const uno::Reference<build::XCompilable>& xItem) override
    {
        return start([&] { return xItem->compile(); });
    }
    uno::Reference<core::XBackgroundJob> SAL_CALL getRunningBuild() override;
    void SAL_CALL cancelBuild() override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.BuildController"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

private:
    uno::Reference<core::XBackgroundJob> start(const std::function<uno::Reference<core::XBackgroundJob>()>& rStart);

    uno::Reference<uno::XComponentContext> m_xContext;
    std::mutex m_aMutex;
    uno::Reference<core::XBackgroundJob> m_xRunning;
};

uno::Reference<core::XBackgroundJob> BuildController::getRunningBuild()
{
    std::scoped_lock aGuard(m_aMutex);
    if (m_xRunning.is() && IsFinalStatus(m_xRunning->getStatus()))
        m_xRunning.clear();
    return m_xRunning;
}

void BuildController::cancelBuild()
{
    if (auto xJob = getRunningBuild())
        xJob->cancel();
}

uno::Reference<core::XBackgroundJob>
BuildController::start(const std::function<uno::Reference<core::XBackgroundJob>()>& rStart)
{
    if (getRunningBuild().is())
        throw io::IOException(u"a build is running already"_ustr, getXWeak());

    // what the last build found is out of date the moment a new one starts
    task::theTaskHub::get(m_xContext)->clearCategory(u"build"_ustr);

    uno::Reference<core::XBackgroundJob> xJob = rStart();
    if (xJob.is())
    {
        core::theJobRegistry::get(m_xContext)->registerJob(xJob);
        std::scoped_lock aGuard(m_aMutex);
        m_xRunning = xJob;
    }
    return xJob;
}
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_BuildController_get_implementation(uno::XComponentContext* pContext,
                                                           uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::BuildController(pContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
