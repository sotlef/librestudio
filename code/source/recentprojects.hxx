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

#include <vector>

namespace code
{
/// The projects the user opened most recently -- what File > Recent Projects
/// and Solutions lists.  A matter of the user interface, not of the projects:
/// the backend does not know it.  Kept in the user's profile
/// (<profile>/code/recent-projects.json), at most ten, newest first, each once.
///
/// The shell (which sees every project open) adds to it, the menu controller
/// (in another library, hence the file) reads and edits it; both compile this.
class RecentProjects
{
public:
    static constexpr size_t Max = 10;

    /// the entries; empty if there is no list, or it cannot be read
    static std::vector<OUString> Get();
    /// makes rUrl the newest entry
    static void Add(const OUString& rUrl);
    static void Remove(const OUString& rUrl);
    static void Clear();
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
