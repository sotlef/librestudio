/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "backgroundjob.hxx"
#include "cmakeproject.hxx"
#include "common.hxx"
#include "folderproject.hxx"
#include "process.hxx"

#include <com/sun/star/code/core/XPlugin.hpp>
#include <com/sun/star/code/project/ProjectConfiguration.hpp>
#include <com/sun/star/code/project/XBuildableProjectFormat.hpp>
#include <com/sun/star/code/project/XConfigurableProjectFormat.hpp>
#include <com/sun/star/code/project/XProjectFormatReader.hpp>
#include <com/sun/star/code/project/theProjectFormatRegistry.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/code/output/OutputStream.hpp>
#include <com/sun/star/code/output/XOutputChannel.hpp>
#include <com/sun/star/code/output/XOutputController.hpp>
#include <com/sun/star/code/output/theOutputController.hpp>
#include <cppuhelper/implbase.hxx>
#include <osl/file.hxx>

#include <algorithm>
#include <vector>
#include <cppuhelper/supportsservice.hxx>

using namespace css;
using namespace css::code;

namespace codeservices
{
namespace
{
// Configures each configuration in turn, writing cmake's output to the channel.
// True only if every one succeeded.
bool RunConfigure(codeservices::BackgroundJob& rJob,
                  const uno::Reference<output::XOutputChannel>& xChannel, const OUString& rProjectUrl,
                  const OUString& rSourceUrl,
                  const std::vector<project::ProjectConfiguration>& rConfigurations)
{
    auto property = [](const project::ProjectConfiguration& rConfiguration, const OUString& rName)
    {
        for (const beans::StringPair& rProperty : rConfiguration.Properties)
            if (rProperty.First == rName)
                return rProperty.Second;
        return OUString();
    };
    auto systemPath = [](const OUString& rUrl)
    {
        OUString aPath;
        return osl::FileBase::getSystemPathFromFileURL(rUrl, aPath) == osl::FileBase::E_None ? aPath : rUrl;
    };

    bool bAllSucceeded = true;
    int nDone = 0;
    for (const project::ProjectConfiguration& rConfiguration : rConfigurations)
    {
        if (rJob.IsCancelled())
            return false;
        rJob.SetProgress(static_cast<sal_Int16>(nDone * 100 / rConfigurations.size()));
        rJob.SetMessage("Configuring " + rConfiguration.Name);

        const OUString aBuildUrl = property(rConfiguration, u"BuildDirectory"_ustr);
        std::vector<OUString> aArguments{ u"-S"_ustr, systemPath(rSourceUrl), u"-B"_ustr,
                                          systemPath(aBuildUrl) };
        const OUString aBuildType = property(rConfiguration, u"BuildType"_ustr);
        if (!aBuildType.isEmpty())
            aArguments.push_back("-DCMAKE_BUILD_TYPE=" + aBuildType);

        OUString aCommandLine = u"cmake"_ustr;
        for (const OUString& rArgument : aArguments)
            aCommandLine += " " + rArgument;
        xChannel->appendLine("------ Configure started: " + rConfiguration.Name + " ------",
                             output::OutputStream_INFO);
        xChannel->appendLine(aCommandLine, output::OutputStream_INFO);

        // ask cmake for its code model, which Solution Explorer is built from
        cmakeproj::RequestCodeModel(aBuildUrl);

        OUString aStartError;
        const sal_Int32 nExit = codeservices::RunProcess(
            u"cmake"_ustr, aArguments, OUString(),
            [&](bool bStderr, const OUString& rLine)
            {
                xChannel->appendLine(rLine, bStderr ? output::OutputStream_STDERR
                                                    : output::OutputStream_STDOUT);
            },
            [&] { return rJob.IsCancelled(); }, aStartError);

        if (!aStartError.isEmpty())
        {
            xChannel->appendLine(aStartError, output::OutputStream_ERROR);
            return false;
        }
        if (rJob.IsCancelled())
        {
            xChannel->appendLine(u"========== Configure cancelled =========="_ustr, output::OutputStream_WARNING);
            return false;
        }
        if (nExit != 0)
        {
            bAllSucceeded = false;
            xChannel->appendLine("========== Configure failed: " + rConfiguration.Name + " (cmake exited with "
                                     + OUString::number(nExit) + ") ==========",
                                 output::OutputStream_ERROR);
        }
        else
            xChannel->appendLine("========== Configure succeeded: " + rConfiguration.Name + " ==========",
                                 output::OutputStream_INFO);
        ++nDone;
    }
    rJob.SetProgress(100);
    // the project (if open) reads the new structure and tells its listeners,
    // whether or not every configuration worked: a partial result still shows
    cmakeproj::NotifyConfigured(rProjectUrl);
    return bAllSucceeded;
}

// The "[12/40]" at the start of a line of ninja's (and make's "[ 30%]") says how
// far the build is; -1 for a line that does not say.
sal_Int16 ProgressOf(const OUString& rLine)
{
    if (rLine.getLength() < 5 || rLine[0] != '[')
        return -1;
    const sal_Int32 nEnd = rLine.indexOf(']');
    if (nEnd < 0)
        return -1;
    const OUString aInside = rLine.copy(1, nEnd - 1).trim();
    if (const sal_Int32 nSlash = aInside.indexOf('/'); nSlash > 0) // ninja: done/total
    {
        const sal_Int32 nTotal = aInside.copy(nSlash + 1).toInt32();
        return nTotal > 0 ? static_cast<sal_Int16>(std::min<sal_Int32>(aInside.toInt32() * 100 / nTotal, 100)) : -1;
    }
    if (aInside.endsWith("%")) // make: percent
        return static_cast<sal_Int16>(std::min<sal_Int32>(aInside.toInt32(), 100));
    return -1;
}

// cmake --build <build directory> [--config <type>], its output to the channel.
bool RunBuild(codeservices::BackgroundJob& rJob, const uno::Reference<output::XOutputChannel>& xChannel,
              const OUString& rProjectName, const project::ProjectConfiguration& rConfiguration)
{
    OUString aBuildUrl;
    OUString aBuildType;
    for (const beans::StringPair& rProperty : rConfiguration.Properties)
    {
        if (rProperty.First == "BuildDirectory")
            aBuildUrl = rProperty.Second;
        else if (rProperty.First == "BuildType")
            aBuildType = rProperty.Second;
    }
    OUString aBuildPath;
    if (osl::FileBase::getSystemPathFromFileURL(aBuildUrl, aBuildPath) != osl::FileBase::E_None)
        aBuildPath = aBuildUrl;

    std::vector<OUString> aArguments{ u"--build"_ustr, aBuildPath };
    // only a multi-configuration generator (Visual Studio, Xcode) looks at this;
    // for a single-configuration one the build type was fixed when configuring
    if (!aBuildType.isEmpty())
    {
        aArguments.push_back(u"--config"_ustr);
        aArguments.push_back(aBuildType);
    }

    OUString aCommandLine = u"cmake"_ustr;
    for (const OUString& rArgument : aArguments)
        aCommandLine += " " + rArgument;
    xChannel->appendLine("------ Build started: " + rProjectName + ", " + rConfiguration.Name + " ------",
                         output::OutputStream_INFO);
    xChannel->appendLine(aCommandLine, output::OutputStream_INFO);

    rJob.SetMessage("Building " + rProjectName + " (" + rConfiguration.Name + ")");
    OUString aStartError;
    const sal_Int32 nExit = codeservices::RunProcess(
        u"cmake"_ustr, aArguments, OUString(),
        [&](bool bStderr, const OUString& rLine)
        {
            // stderr is where compilers write diagnostics, but not everything on
            // it is one: the stream only says where it came from
            xChannel->appendLine(rLine, bStderr ? output::OutputStream_STDERR : output::OutputStream_STDOUT);
            if (const sal_Int16 nProgress = ProgressOf(rLine); nProgress >= 0)
                rJob.SetProgress(nProgress);
        },
        [&] { return rJob.IsCancelled(); }, aStartError);

    if (!aStartError.isEmpty())
    {
        xChannel->appendLine(aStartError, output::OutputStream_ERROR);
        return false;
    }
    if (rJob.IsCancelled())
    {
        xChannel->appendLine(u"========== Build cancelled =========="_ustr, output::OutputStream_WARNING);
        return false;
    }
    if (nExit != 0)
    {
        xChannel->appendLine("========== Build failed: " + rProjectName + " (cmake exited with "
                                 + OUString::number(nExit) + ") ==========",
                             output::OutputStream_ERROR);
        return false;
    }
    rJob.SetProgress(100);
    xChannel->appendLine("========== Build succeeded: " + rProjectName + " ==========", output::OutputStream_INFO);
    return true;
}

// A CMake project is the directory of its top-level CMakeLists.txt.  This
// first reader only recognises the format and shows the directory's tree
// under the project's own name and kind; reading targets and sources out of
// the CMake files is the format's own, later work.
class CMakeFormatReader final
    : public cppu::WeakImplHelper<project::XProjectFormatReader, project::XConfigurableProjectFormat,
                                  project::XBuildableProjectFormat>
{
public:
    explicit CMakeFormatReader(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    OUString SAL_CALL getFormatKind() override { return u"cmake"_ustr; }
    OUString SAL_CALL getDisplayName() override { return u"CMake projects"_ustr; }
    uno::Sequence<OUString> SAL_CALL getFilePatterns() override { return { u"CMakeLists.txt"_ustr }; }
    sal_Int32 SAL_CALL getPriority() override { return 0; }

    sal_Bool SAL_CALL canRead(const OUString& rUrl) override
    {
        const OUString aUrl = NormalizeUrl(rUrl);
        return aUrl.endsWith("/CMakeLists.txt") && !IsDirectory(aUrl);
    }

    uno::Reference<project::XProject> SAL_CALL read(const OUString& rUrl) override
    {
        return cmakeproj::CreateCMakeProject(m_xContext, rUrl);
    }

    // XConfigurableProjectFormat
    OUString SAL_CALL getConfiguratorServiceName() override
    {
        return u"com.sun.star.code.CMakeConfigurationPanel"_ustr;
    }

    // A Debug and a Release build, each in a directory of its own next to the
    // project -- what CMake's out-of-source builds want.
    uno::Sequence<project::ProjectConfiguration>
        SAL_CALL getDefaultConfigurations(const OUString& rProjectUrl) override
    {
        const OUString aRoot = GetParentUrl(rProjectUrl);
        auto make = [&](const OUString& rName, const OUString& rDirectory)
        {
            project::ProjectConfiguration aConfiguration;
            aConfiguration.Name = rName;
            aConfiguration.Properties = { { u"BuildType"_ustr, rName },
                                          { u"BuildDirectory"_ustr, aRoot + "/build/" + rDirectory } };
            return aConfiguration;
        };
        return { make(u"Debug"_ustr, u"debug"_ustr), make(u"Release"_ustr, u"release"_ustr) };
    }

    // XBuildableProjectFormat
    uno::Reference<core::XBackgroundJob>
        SAL_CALL build(const OUString& rProjectUrl, const project::ProjectConfiguration& rConfiguration) override
    {
        uno::Reference<output::XOutputChannel> xChannel
            = output::theOutputController::get(m_xContext)->getOrCreateChannel(u"build"_ustr, u"Build"_ustr);
        xChannel->clear();

        const OUString aProjectName = GetDisplayName(GetParentUrl(rProjectUrl));
        return rtl::Reference<codeservices::BackgroundJob>(codeservices::BackgroundJob::Start(
            u"cmake-build:"_ustr + rProjectUrl, "Building " + aProjectName + " (" + rConfiguration.Name + ")",
            [xChannel, aProjectName, rConfiguration](codeservices::BackgroundJob& rJob)
            { return RunBuild(rJob, xChannel, aProjectName, rConfiguration); }));
    }

    // cmake -S <project> -B <build directory> -DCMAKE_BUILD_TYPE=<type>, once
    // per configuration.  Everything cmake prints goes to the "cmake" output
    // channel, which exists before this returns, so that the Output window can
    // list it from the first moment.
    uno::Reference<core::XBackgroundJob>
        SAL_CALL configure(const OUString& rProjectUrl,
                           const uno::Sequence<project::ProjectConfiguration>& rConfigurations) override
    {
        uno::Reference<output::XOutputChannel> xChannel
            = output::theOutputController::get(m_xContext)->getOrCreateChannel(u"cmake"_ustr, u"CMake"_ustr);
        xChannel->clear();

        const OUString aSourceUrl = GetParentUrl(rProjectUrl);
        const OUString aProjectName = GetDisplayName(aSourceUrl);
        std::vector<project::ProjectConfiguration> aConfigurations(rConfigurations.begin(),
                                                                   rConfigurations.end());

        return rtl::Reference<codeservices::BackgroundJob>(codeservices::BackgroundJob::Start(
            u"cmake-configure:"_ustr + rProjectUrl, "Configuring " + aProjectName,
            [xChannel, rProjectUrl, aSourceUrl, aConfigurations](codeservices::BackgroundJob& rJob)
            { return RunConfigure(rJob, xChannel, rProjectUrl, aSourceUrl, aConfigurations); }));
    }

private:
    uno::Reference<uno::XComponentContext> m_xContext;
};

class CMakeFormatPlugin final : public cppu::WeakImplHelper<core::XPlugin, lang::XServiceInfo>
{
public:
    explicit CMakeFormatPlugin(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    void SAL_CALL activate() override
    {
        project::theProjectFormatRegistry::get(m_xContext)->registerReader(new CMakeFormatReader(m_xContext));
    }

    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.CMakeFormatPlugin"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override
    {
        return { u"com.sun.star.code.core.Plugin"_ustr };
    }

private:
    uno::Reference<uno::XComponentContext> m_xContext;
};
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_CMakeFormatPlugin_get_implementation(uno::XComponentContext* pContext,
                                                             uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::CMakeFormatPlugin(pContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
