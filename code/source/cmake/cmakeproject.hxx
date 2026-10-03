/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/code/project/XProject.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

namespace cmakeproj
{
/// A CMake project: the directory of a top-level CMakeLists.txt.  Besides the
/// plain file tree (XProjectFiles) it offers, once cmake has configured it, the
/// structure CMake itself reports through its File API -- targets, and the
/// source files of each (XProjectStructure, XProjectTargets) -- and says
/// whether that has happened (XProjectConfigurationState), telling its
/// listeners (XModifyBroadcaster) when it does.
css::uno::Reference<css::code::project::XProject>
CreateCMakeProject(const css::uno::Reference<css::uno::XComponentContext>& xContext,
                   const OUString& rCMakeListsUrl);

/// Writes the File API query that makes cmake produce the code model on its
/// next configure run in this build directory (a file URL).
void RequestCodeModel(const OUString& rBuildDirectoryUrl);

/// The configure run of this project has ended: the open project, if any,
/// reads the result again and tells its listeners.
void NotifyConfigured(const OUString& rCMakeListsUrl);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
