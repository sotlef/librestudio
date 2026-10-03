/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "process.hxx"

#include <osl/file.h>
#include <osl/process.h>
#include <osl/security.h>
#include <rtl/strbuf.hxx>

#include <thread>

namespace codeservices
{
namespace
{
// Reads one pipe to its end, calling rSink for every complete line.  A line
// ends at \n; a \r before it is dropped.  Output that does not end in a
// newline still counts as a line when the pipe closes.
void ReadLines(oslFileHandle hPipe, bool bStderr,
               const std::function<void(bool, const OUString&)>& rSink)
{
    OStringBuffer aLine;
    auto flush = [&]
    {
        sal_Int32 nLength = aLine.getLength();
        if (nLength > 0 && aLine[nLength - 1] == '\r')
            aLine.setLength(--nLength);
        rSink(bStderr, OUString::fromUtf8(aLine.toString()));
        aLine.setLength(0);
    };

    char aBuffer[4096];
    sal_uInt64 nRead = 0;
    while (osl_readFile(hPipe, aBuffer, sizeof aBuffer, &nRead) == osl_File_E_None && nRead > 0)
    {
        for (sal_uInt64 i = 0; i < nRead; ++i)
        {
            if (aBuffer[i] == '\n')
                flush();
            else
                aLine.append(aBuffer[i]);
        }
    }
    if (aLine.getLength() > 0)
        flush();
}
}

sal_Int32 RunProcess(const OUString& rProgram, const std::vector<OUString>& rArguments,
                     const OUString& rWorkingDirectory,
                     const std::function<void(bool, const OUString&)>& rSink,
                     const std::function<bool()>& rIsCancelled, OUString& rStartError)
{
    std::vector<rtl_uString*> aArguments;
    for (const OUString& rArgument : rArguments)
        aArguments.push_back(rArgument.pData);

    oslProcess hProcess = nullptr;
    oslFileHandle hIn = nullptr;
    oslFileHandle hOut = nullptr;
    oslFileHandle hErr = nullptr;
    oslSecurity hSecurity = osl_getCurrentSecurity();
    const oslProcessError eError = osl_executeProcess_WithRedirectedIO(
        rProgram.pData, aArguments.data(), static_cast<sal_uInt32>(aArguments.size()),
        osl_Process_SEARCHPATH | osl_Process_HIDDEN, hSecurity,
        rWorkingDirectory.isEmpty() ? nullptr : rWorkingDirectory.pData, nullptr, 0, &hProcess, &hIn,
        &hOut, &hErr);
    osl_freeSecurityHandle(hSecurity);

    if (eError != osl_Process_E_None || !hOut || !hErr)
    {
        if (eError == osl_Process_E_NotFound)
            rStartError = rProgram + " was not found (is it installed and on PATH?)";
        else
            rStartError = "cannot start " + rProgram;
        if (hProcess)
            osl_freeProcessHandle(hProcess);
        for (oslFileHandle h : { hIn, hOut, hErr })
            if (h)
                osl_closeFile(h);
        return -1;
    }
    // nothing is written to the program; closing its input lets it see end-of-file
    osl_closeFile(hIn);

    // Both streams are read at once: a program that fills one pipe while we
    // wait on the other would otherwise block for ever.
    std::thread aOut(ReadLines, hOut, false, std::cref(rSink));
    std::thread aErr(ReadLines, hErr, true, std::cref(rSink));

    bool bTerminated = false;
    for (;;)
    {
        TimeValue aTimeout{ 0, 200 * 1000 * 1000 }; // 200 ms
        if (osl_joinProcessWithTimeout(hProcess, &aTimeout) != osl_Process_E_TimedOut)
            break;
        if (!bTerminated && rIsCancelled())
        {
            osl_terminateProcess(hProcess);
            bTerminated = true;
        }
    }
    aOut.join();
    aErr.join();
    osl_closeFile(hOut);
    osl_closeFile(hErr);

    sal_Int32 nExitCode = -1;
    oslProcessInfo aInfo;
    aInfo.Size = sizeof aInfo;
    if (!bTerminated && osl_getProcessInfo(hProcess, osl_Process_EXITCODE, &aInfo) == osl_Process_E_None)
        nExitCode = static_cast<sal_Int32>(aInfo.Code);
    osl_freeProcessHandle(hProcess);
    return nExitCode;
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
