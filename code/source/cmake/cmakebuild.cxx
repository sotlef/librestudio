/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "cmakebuild.hxx"

#include "backgroundjob.hxx"
#include "common.hxx"
#include "folderproject.hxx"
#include "process.hxx"

#include <com/sun/star/beans/StringPair.hpp>
#include <com/sun/star/code/output/OutputStream.hpp>
#include <com/sun/star/code/output/XOutputChannel.hpp>
#include <com/sun/star/code/output/XOutputController.hpp>
#include <com/sun/star/code/output/theOutputController.hpp>
#include <com/sun/star/code/project/ProjectConfiguration.hpp>
#include <com/sun/star/code/project/XProjectConfigStore.hpp>
#include <com/sun/star/code/project/theProjectConfigStore.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <osl/file.hxx>
#include <rtl/strbuf.hxx>

#include <algorithm>
#include <vector>

using namespace css;
using namespace css::code;

namespace cmakeproj
{
namespace
{
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
// The generator the build directory was configured with, "" if unknown.
OUString GeneratorOf(const OUString& rBuildPath)
{
    OUString aUrl;
    if (osl::FileBase::getFileURLFromSystemPath(rBuildPath, aUrl) != osl::FileBase::E_None)
        return {};
    osl::File aFile(aUrl + "/CMakeCache.txt");
    if (aFile.open(osl_File_OpenFlag_Read) != osl::FileBase::E_None)
        return {};
    // the cache is small; read it all and find the line
    OStringBuffer aContent;
    char aBuffer[4096];
    sal_uInt64 nRead = 0;
    while (aFile.read(aBuffer, sizeof aBuffer, nRead) == osl::FileBase::E_None && nRead > 0)
        aContent.append(aBuffer, static_cast<sal_Int32>(nRead));
    const OString aText = aContent.makeStringAndClear();
    const OString aKey("\nCMAKE_GENERATOR:INTERNAL=");
    OUString aGenerator;
    if (const sal_Int32 nPos = aText.indexOf(aKey); nPos >= 0)
    {
        const sal_Int32 nStart = nPos + aKey.getLength();
        const sal_Int32 nEnd = aText.indexOf('\n', nStart);
        aGenerator = OStringToOUString(aText.subView(nStart, (nEnd < 0 ? aText.getLength() : nEnd) - nStart),
                                       RTL_TEXTENCODING_UTF8);
    }
    return aGenerator;
}

struct BuildStep
{
    OUString aProgram;
    std::vector<OUString> aArguments;
    OUString aError; // when the step cannot be made at all
};

bool RunBuild(codeservices::BackgroundJob& rJob, const uno::Reference<output::XOutputChannel>& xChannel,
              const OUString& rProjectName, const project::ProjectConfiguration& rConfiguration,
              const OUString& rTarget, BuildMode eMode)
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

    const OUString aVerb = eMode == BuildMode::Rebuild ? u"Rebuild"_ustr
                           : eMode == BuildMode::Clean ? u"Clean"_ustr
                                                       : u"Build"_ustr;
    const OUString aSubject = rTarget.isEmpty() ? rProjectName : rProjectName + "/" + rTarget;

    // only a multi-configuration generator (Visual Studio, Xcode) looks at --config;
    // for a single-configuration one the build type was fixed when configuring
    auto addConfig = [&](std::vector<OUString>& rArguments)
    {
        if (!aBuildType.isEmpty())
        {
            rArguments.push_back(u"--config"_ustr);
            rArguments.push_back(aBuildType);
        }
    };

    std::vector<BuildStep> aSteps;
    if (rTarget.isEmpty())
    {
        BuildStep aStep{ u"cmake"_ustr, { u"--build"_ustr, aBuildPath }, {} };
        if (eMode == BuildMode::Rebuild)
            aStep.aArguments.push_back(u"--clean-first"_ustr); // cmake cleans the tree first
        else if (eMode == BuildMode::Clean)
        {
            aStep.aArguments.push_back(u"--target"_ustr);
            aStep.aArguments.push_back(u"clean"_ustr);
        }
        addConfig(aStep.aArguments);
        aSteps.push_back(std::move(aStep));
    }
    else
    {
        if (eMode != BuildMode::Build)
        {
            // cmake can clean only the whole tree; one target is the generator's own business
            const OUString aGenerator = GeneratorOf(aBuildPath);
            BuildStep aClean;
            if (aGenerator.startsWith("Ninja"))
                aClean = { u"ninja"_ustr, { u"-C"_ustr, aBuildPath, u"-t"_ustr, u"clean"_ustr, rTarget }, {} };
            else if (aGenerator.endsWith("Makefiles"))
                aClean = { u"cmake"_ustr, { u"--build"_ustr, aBuildPath, u"--target"_ustr, rTarget + "/clean" }, {} };
            else
                aClean.aError = "cleaning a single target is not supported for the generator \"" + aGenerator + "\"";
            aSteps.push_back(std::move(aClean));
        }
        if (eMode != BuildMode::Clean)
        {
            BuildStep aStep{ u"cmake"_ustr, { u"--build"_ustr, aBuildPath, u"--target"_ustr, rTarget }, {} };
            addConfig(aStep.aArguments);
            aSteps.push_back(std::move(aStep));
        }
    }

    xChannel->appendLine("------ " + aVerb + " started: " + aSubject + ", " + rConfiguration.Name + " ------",
                         output::OutputStream_INFO);
    rJob.SetMessage(aVerb + " " + aSubject + " (" + rConfiguration.Name + ")");

    for (const BuildStep& rStep : aSteps)
    {
        if (!rStep.aError.isEmpty())
        {
            xChannel->appendLine(rStep.aError, output::OutputStream_ERROR);
            xChannel->appendLine("========== " + aVerb + " failed: " + aSubject + " ==========",
                                 output::OutputStream_ERROR);
            return false;
        }
        OUString aCommandLine = rStep.aProgram;
        for (const OUString& rArgument : rStep.aArguments)
            aCommandLine += " " + rArgument;
        xChannel->appendLine(aCommandLine, output::OutputStream_INFO);

        OUString aStartError;
        const sal_Int32 nExit = codeservices::RunProcess(
            rStep.aProgram, rStep.aArguments, OUString(),
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
            xChannel->appendLine("========== " + aVerb + " cancelled ==========", output::OutputStream_WARNING);
            return false;
        }
        if (nExit != 0)
        {
            xChannel->appendLine("========== " + aVerb + " failed: " + aSubject + " (" + rStep.aProgram
                                     + " exited with " + OUString::number(nExit) + ") ==========",
                                 output::OutputStream_ERROR);
            return false;
        }
    }
    rJob.SetProgress(100);
    xChannel->appendLine("========== " + aVerb + " succeeded: " + aSubject + " ==========", output::OutputStream_INFO);
    return true;
}

// The object file ninja makes of rSourceUrl in rTarget: its targets are the
// output files, "CMakeFiles/<target>.dir/<path of the source>.o".  Empty if ninja
// has no such target.
OUString ObjectOf(const OUString& rBuildPath, const OUString& rTarget, const OUString& rSourceUrl,
                  const OUString& rRootUrl)
{
    std::vector<OUString> aTargets;
    OUString aError;
    codeservices::RunProcess(
        u"ninja"_ustr, { u"-C"_ustr, rBuildPath, u"-t"_ustr, u"targets"_ustr, u"all"_ustr }, OUString(),
        [&](bool bStderr, const OUString& rLine)
        {
            if (!bStderr)
                aTargets.push_back(rLine.copy(0, std::max<sal_Int32>(rLine.lastIndexOf(':'), 0)));
        },
        [] { return false; }, aError);

    const OUString aPrefix = "CMakeFiles/" + rTarget + ".dir/";
    const OUString aRelative = rSourceUrl.startsWith(OUString(rRootUrl + "/")) ? rSourceUrl.copy(rRootUrl.getLength() + 1) : OUString();
    OUString aBest;
    for (const OUString& rCandidate : aTargets)
    {
        if (!rCandidate.startsWith(aPrefix) || !rCandidate.endsWith(".o"))
            continue;
        const OUString aPath = rCandidate.copy(aPrefix.getLength(), rCandidate.getLength() - aPrefix.getLength() - 2);
        // the source's path from the project root, or failing that its name
        if (!aRelative.isEmpty() && aPath.endsWith(aRelative))
            return rCandidate;
        if (aBest.isEmpty() && rSourceUrl.endsWith(OUString("/" + aPath.copy(aPath.lastIndexOf('/') + 1))))
            aBest = rCandidate;
    }
    return aBest;
}

bool RunCompile(codeservices::BackgroundJob& rJob, const uno::Reference<output::XOutputChannel>& xChannel,
                const project::ProjectConfiguration& rConfiguration, const OUString& rTarget,
                const OUString& rSourceUrl, const OUString& rRootUrl)
{
    OUString aBuildUrl;
    for (const beans::StringPair& rProperty : rConfiguration.Properties)
        if (rProperty.First == "BuildDirectory")
            aBuildUrl = rProperty.Second;
    OUString aBuildPath;
    if (osl::FileBase::getSystemPathFromFileURL(aBuildUrl, aBuildPath) != osl::FileBase::E_None)
        aBuildPath = aBuildUrl;

    const OUString aName = codeservices::GetDisplayName(rSourceUrl);
    xChannel->appendLine("------ Compile started: " + aName + ", " + rConfiguration.Name + " ------",
                         output::OutputStream_INFO);
    rJob.SetMessage("Compiling " + aName + " (" + rConfiguration.Name + ")");

    auto fail = [&](const OUString& rText)
    {
        xChannel->appendLine(rText, output::OutputStream_ERROR);
        xChannel->appendLine("========== Compile failed: " + aName + " ==========", output::OutputStream_ERROR);
        return false;
    };
    // cmake itself builds targets only; one file is the generator's own business
    const OUString aGenerator = GeneratorOf(aBuildPath);
    if (!aGenerator.startsWith("Ninja"))
        return fail("compiling a single file is not supported for the generator \"" + aGenerator + "\"");
    const OUString aObject = ObjectOf(aBuildPath, rTarget, rSourceUrl, rRootUrl);
    if (aObject.isEmpty())
        return fail("ninja has no rule to compile " + aName + " in " + rTarget);

    const std::vector<OUString> aArguments{ u"-C"_ustr, aBuildPath, aObject };
    xChannel->appendLine("ninja -C " + aBuildPath + " " + aObject, output::OutputStream_INFO);
    OUString aStartError;
    const sal_Int32 nExit = codeservices::RunProcess(
        u"ninja"_ustr, aArguments, OUString(),
        [&](bool bStderr, const OUString& rLine)
        {
            xChannel->appendLine(rLine, bStderr ? output::OutputStream_STDERR : output::OutputStream_STDOUT);
            if (const sal_Int16 nProgress = ProgressOf(rLine); nProgress >= 0)
                rJob.SetProgress(nProgress);
        },
        [&] { return rJob.IsCancelled(); }, aStartError);
    if (!aStartError.isEmpty())
        return fail(aStartError);
    if (rJob.IsCancelled())
    {
        xChannel->appendLine("========== Compile cancelled ==========", output::OutputStream_WARNING);
        return false;
    }
    if (nExit != 0)
        return fail("ninja exited with " + OUString::number(nExit));
    rJob.SetProgress(100);
    xChannel->appendLine("========== Compile succeeded: " + aName + " ==========", output::OutputStream_INFO);
    return true;
}

project::ProjectConfiguration CurrentConfigurationOf(const uno::Reference<uno::XComponentContext>& xContext,
                                                     const OUString& rProjectUrl)
{
    // a build is in a configuration; a project that has none was never configured
    auto xStore = project::theProjectConfigStore::get(xContext);
    const OUString aCurrent = xStore->getCurrentConfiguration(rProjectUrl);
    for (const project::ProjectConfiguration& rConfiguration : xStore->getConfigurations(rProjectUrl))
        if (rConfiguration.Name == aCurrent)
            return rConfiguration;
    throw io::IOException("the project has not been configured: " + rProjectUrl, nullptr);
}
}

uno::Reference<core::XBackgroundJob> StartBuild(const uno::Reference<uno::XComponentContext>& xContext,
                                                const OUString& rProjectUrl, const OUString& rTarget,
                                                BuildMode eMode)
{
    const project::ProjectConfiguration aConfiguration = CurrentConfigurationOf(xContext, rProjectUrl);

    uno::Reference<output::XOutputChannel> xChannel
        = output::theOutputController::get(xContext)->getOrCreateChannel(u"build"_ustr, u"Build"_ustr);
    xChannel->clear();

    const OUString aProjectName = codeservices::GetDisplayName(codeservices::GetParentUrl(rProjectUrl));
    const OUString aSubject = rTarget.isEmpty() ? aProjectName : aProjectName + "/" + rTarget;
    const OUString aVerb = eMode == BuildMode::Rebuild ? u"Rebuilding "_ustr
                           : eMode == BuildMode::Clean ? u"Cleaning "_ustr
                                                       : u"Building "_ustr;
    return rtl::Reference<codeservices::BackgroundJob>(codeservices::BackgroundJob::Start(
        u"cmake-build:"_ustr + rProjectUrl, aVerb + aSubject + " (" + aConfiguration.Name + ")",
        [xChannel, aProjectName, aConfiguration, rTarget, eMode](codeservices::BackgroundJob& rJob)
        { return RunBuild(rJob, xChannel, aProjectName, aConfiguration, rTarget, eMode); }));
}

uno::Reference<core::XBackgroundJob> StartCompile(const uno::Reference<uno::XComponentContext>& xContext,
                                                  const OUString& rProjectUrl, const OUString& rTarget,
                                                  const OUString& rFileUrl)
{
    const project::ProjectConfiguration aConfiguration = CurrentConfigurationOf(xContext, rProjectUrl);
    uno::Reference<output::XOutputChannel> xChannel
        = output::theOutputController::get(xContext)->getOrCreateChannel(u"build"_ustr, u"Build"_ustr);
    xChannel->clear();
    const OUString aRootUrl = codeservices::GetParentUrl(rProjectUrl);
    return rtl::Reference<codeservices::BackgroundJob>(codeservices::BackgroundJob::Start(
        u"cmake-compile:"_ustr + rFileUrl, "Compiling " + codeservices::GetDisplayName(rFileUrl),
        [xChannel, aConfiguration, rTarget, rFileUrl, aRootUrl](codeservices::BackgroundJob& rJob)
        { return RunCompile(rJob, xChannel, aConfiguration, rTarget, rFileUrl, aRootUrl); }));
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
