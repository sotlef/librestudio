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
#include <com/sun/star/uno/Reference.hxx>

namespace codeservices
{
/// The project of a directory no format reader claimed: its file tree, read
/// lazily (a directory's children are listed when first asked for).
/// @throws css::io::IOException if the URL is not a readable directory
css::uno::Reference<css::code::project::XProject> CreateFolderProject(const OUString& rUrl);

/// A project whose tree is that of a directory but whose identity is another
/// URL -- a project file (CMakeLists.txt) living in that directory -- and which
/// reports rFormatKind.  For readers of formats that, in this first wave, only
/// need to say "this directory is a project of this kind".
/// @throws css::io::IOException if rRootUrl is not a readable directory
css::uno::Reference<css::code::project::XProject>
CreateFileTreeProject(const OUString& rRootUrl, const OUString& rIdentityUrl,
                      const OUString& rFormatKind);

/// the URL of the directory a file URL lies in
OUString GetParentUrl(const OUString& rUrl);

/// true if the URL names an existing directory
bool IsDirectory(const OUString& rUrl);

/// the last path segment of a URL, decoded -- a project's or file's display name
OUString GetDisplayName(const OUString& rUrl);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
