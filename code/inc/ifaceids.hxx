/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sfx2/shell.hxx>

// Own small range, not a port of any upstream module's IDs (this app doesn't
// exist upstream). The highest constant in include/sfx2/shell.hxx and
// include/svx/ifaceids.hxx is SVX_INTERFACE_FORM_SH == SfxInterfaceId(410);
// 500/501 are safely clear of every reserved range.
constexpr auto CODE_INTERFACE_DOCSH  = SfxInterfaceId(500);
constexpr auto CODE_INTERFACE_VIEWSH = SfxInterfaceId(501);

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
