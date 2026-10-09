/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "runcontroller.hxx"

#include "backgroundjob.hxx"
#include "process.hxx"

#include <com/sun/star/code/core/theJobRegistry.hpp>
#include <com/sun/star/code/output/XOutputChannel.hpp>
#include <com/sun/star/code/output/theOutputController.hpp>
#include <comphelper/sequence.hxx>
#include <osl/file.hxx>
#include <rtl/ref.hxx>
#include <sal/log.hxx>

#include <mutex>

using namespace css;
using namespace css::code;

namespace code::run
{
namespace
{
std::mutex g_aMutex;
rtl::Reference<codeservices::BackgroundJob> g_xRunning; // the one program running

OUString SystemPathOf(const OUString& rPathOrUrl)
{
    OUString aPath;
    if (rPathOrUrl.startsWith("file:")
        && osl::FileBase::getSystemPathFromFileURL(rPathOrUrl, aPath) == osl::FileBase::E_None)
        return aPath;
    return rPathOrUrl;
}
}

bool IsRunning()
{
    std::scoped_lock aGuard(g_aMutex);
    return g_xRunning.is() && g_xRunning->getStatus() == core::JobStatus_RUNNING;
}

bool Start(const uno::Reference<uno::XComponentContext>& xContext,
           const debug::LaunchDescriptor& rDescriptor)
{
    std::scoped_lock aGuard(g_aMutex);
    if (g_xRunning.is() && g_xRunning->getStatus() == core::JobStatus_RUNNING)
        return false;

    uno::Reference<output::XOutputChannel> xChannel
        = output::theOutputController::get(xContext)->getOrCreateChannel(u"run"_ustr,
                                                                         u"Run"_ustr);
    xChannel->clear();

    // RunProcess finds the program in PATH too, but what runs here is a built
    // artifact: a path (gdb gets the same descriptor, so both accept both)
    const OUString aProgram = SystemPathOf(rDescriptor.Program);
    const std::vector<OUString> aArguments
        = comphelper::sequenceToContainer<std::vector<OUString>>(rDescriptor.Arguments);
    const std::vector<OUString> aEnvironment
        = comphelper::sequenceToContainer<std::vector<OUString>>(rDescriptor.Environment);
    const OUString aWorkingDirectory = rDescriptor.WorkingDirectory;

    OUString aCommandLine = aProgram;
    for (const OUString& rArgument : aArguments)
        aCommandLine += " " + rArgument;
    xChannel->appendLine("> " + aCommandLine, output::OutputStream_INFO);

    rtl::Reference<codeservices::BackgroundJob> xJob = codeservices::BackgroundJob::Start(
        "run:" + aProgram, "Running " + aProgram,
        [xChannel, aProgram, aArguments, aWorkingDirectory,
         aEnvironment](codeservices::BackgroundJob& rJob)
        {
            OUString aStartError;
            const sal_Int32 nExit = codeservices::RunProcess(
                aProgram, aArguments, aWorkingDirectory,
                [&](bool bStderr, const OUString& rLine) {
                    xChannel->appendLine(rLine, bStderr ? output::OutputStream_STDERR
                                                        : output::OutputStream_STDOUT);
                },
                [&] { return rJob.IsCancelled(); }, aStartError, aEnvironment);
            if (!aStartError.isEmpty())
            {
                xChannel->appendLine(aStartError, output::OutputStream_ERROR);
                return false;
            }
            if (rJob.IsCancelled())
            {
                xChannel->appendLine(u"The program was terminated."_ustr,
                                     output::OutputStream_WARNING);
                return false;
            }
            xChannel->appendLine("Process exited with code " + OUString::number(nExit) + ".",
                                 nExit == 0 ? output::OutputStream_INFO
                                            : output::OutputStream_ERROR);
            return nExit == 0;
        });
    try
    {
        core::theJobRegistry::get(xContext)->registerJob(xJob);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot register the run job: " << rException.Message);
    }
    g_xRunning = std::move(xJob);
    return true;
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
