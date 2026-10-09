/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/code/debug/LaunchDescriptor.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <rtl/ustring.hxx>

namespace code
{
// Start Without Debugging: runs the startup target as a cancellable
// background job, its output streamed into the Output dock's Run channel.
// Not tied to a view: the process outlives view changes and dies only on
// its own, by cancel, or with the application.
namespace run
{
/// whether a program started here is still running
bool IsRunning();

/// Starts rDescriptor as a background job (false and an untouched state when
/// one is still running; the caller tells the user).  The job registers with
/// theJobRegistry, so the tasks popup can cancel it, which terminates the
/// program.
bool Start(const css::uno::Reference<css::uno::XComponentContext>& xContext,
           const css::code::debug::LaunchDescriptor& rDescriptor);
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
