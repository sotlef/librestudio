/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/code/core/XBackgroundJob.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

namespace cmakeproj
{
enum class BuildMode
{
    Build,
    Rebuild,
    Clean
};

/// Starts cmake building the open project with this CMakeLists.txt URL in its
/// current configuration, as a background job; rTarget names one target, empty
/// is the whole project.  Output goes to the "build" channel.
/// @throws css::io::IOException if the project has not been configured
css::uno::Reference<css::code::core::XBackgroundJob>
StartBuild(const css::uno::Reference<css::uno::XComponentContext>& xContext, const OUString& rProjectUrl,
           const OUString& rTarget, BuildMode eMode);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
