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
#include <com/sun/star/code/core/XBackgroundJob.hpp>
#include <com/sun/star/code/core/XJobRegistry.hpp>
#include <com/sun/star/code/core/theJobRegistry.hpp>
#include <com/sun/star/code/project/ProjectConfiguration.hpp>
#include <com/sun/star/code/project/XBuildableProjectFormat.hpp>
#include <com/sun/star/code/project/XProject.hpp>
#include <com/sun/star/code/project/XProjectFormatReader.hpp>
#include <com/sun/star/code/project/XProjectFormatRegistry.hpp>
#include <com/sun/star/code/project/XProjectManager.hpp>
#include <com/sun/star/code/project/theProjectFormatRegistry.hpp>
#include <com/sun/star/code/project/theProjectManager.hpp>
#include <com/sun/star/code/task/XTaskHub.hpp>
#include <com/sun/star/code/task/theTaskHub.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>

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
    uno::Reference<core::XBackgroundJob> SAL_CALL buildProject(const OUString& rProjectUrl) override
    {
        return start(rProjectUrl, Action::Build);
    }
    uno::Reference<core::XBackgroundJob> SAL_CALL rebuildProject(const OUString& rProjectUrl) override
    {
        return start(rProjectUrl, Action::Rebuild);
    }
    uno::Reference<core::XBackgroundJob> SAL_CALL cleanProject(const OUString& rProjectUrl) override
    {
        return start(rProjectUrl, Action::Clean);
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
    enum class Action
    {
        Build,
        Rebuild,
        Clean
    };
    uno::Reference<core::XBackgroundJob> start(const OUString& rProjectUrl, Action eAction);

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

uno::Reference<core::XBackgroundJob> BuildController::start(const OUString& rProjectUrl, Action eAction)
{
    const OUString aUrl = NormalizeUrl(rProjectUrl);

    auto xManager = project::theProjectManager::get(m_xContext);
    if (!xManager->getProject(aUrl).is())
        throw lang::IllegalArgumentException("no such project is open: " + aUrl, getXWeak(), 0);

    uno::Reference<project::XProjectFormatReader> xReader
        = project::theProjectFormatRegistry::get(m_xContext)->findReader(aUrl);
    uno::Reference<project::XBuildableProjectFormat> xFormat(xReader, uno::UNO_QUERY);
    if (!xFormat.is())
        throw io::IOException("projects of this format cannot be built: " + aUrl, getXWeak());

    // a build is in a configuration; a project that has none was never configured
    const OUString aCurrent = xManager->getCurrentConfiguration(aUrl);
    project::ProjectConfiguration aConfiguration;
    bool bFound = false;
    for (const project::ProjectConfiguration& rConfiguration : xManager->getConfigurations(aUrl))
        if (rConfiguration.Name == aCurrent)
        {
            aConfiguration = rConfiguration;
            bFound = true;
        }
    if (!bFound)
        throw io::IOException("the project has not been configured: " + aUrl, getXWeak());

    if (getRunningBuild().is())
        throw io::IOException(u"a build is running already"_ustr, getXWeak());

    // what the last build found is out of date the moment a new one starts
    task::theTaskHub::get(m_xContext)->clearCategory(u"build"_ustr);

    uno::Reference<core::XBackgroundJob> xJob
        = eAction == Action::Clean ? xFormat->clean(aUrl, aConfiguration)
                                   : xFormat->build(aUrl, aConfiguration, eAction == Action::Rebuild);
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
