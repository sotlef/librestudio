/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <sfx2/dllapi.h>

namespace sfx2::sidebar
{
class ResourceManager;
class Context;

/** Minimal interface a TabBar needs from whatever controller owns it.
    Lets TabBar be reused by any deck+tabbar host (e.g. the Sidebar's
    own SidebarController, or the "Code" app's bottom-only Output
    deck), not just SidebarController itself.
*/
class SFX2_DLLPUBLIC IDeckHost
{
public:
    virtual ~IDeckHost() = default;

    virtual ResourceManager* GetResourceManager() = 0;
    virtual const Context& GetCurrentContext() const = 0;
    virtual bool IsDocked() const = 0;
};

} // end of namespace sfx2::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
