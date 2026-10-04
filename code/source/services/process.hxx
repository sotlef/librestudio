/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <rtl/ustring.hxx>
#include <sal/types.h>

#include <functional>
#include <vector>

namespace codeservices
{
/// Runs a program and hands each line it prints, as it is printed, to rSink
/// (bStderr says which stream it came from).  rSink is called from helper
/// threads, one per stream.
///
/// rIsCancelled is polled while the program runs; once it answers true the
/// program is terminated.  Returns the program's exit code; -1 if it could not
/// be started or was terminated.  rStartError gets the reason for the first.
/// The program is looked up in PATH; rWorkingDirectory is a file URL, or empty.
/// rEnvironment is "NAME=value" entries that are added to (or replace those
/// of) the environment of this process; honoured on Linux, where there is an
/// environment block to merge with, ignored elsewhere.
sal_Int32 RunProcess(const OUString& rProgram, const std::vector<OUString>& rArguments,
                     const OUString& rWorkingDirectory,
                     const std::function<void(bool bStderr, const OUString& rLine)>& rSink,
                     const std::function<bool()>& rIsCancelled, OUString& rStartError,
                     const std::vector<OUString>& rEnvironment = {});
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
