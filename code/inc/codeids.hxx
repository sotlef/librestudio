/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <svl/solar.hrc>

// Slot (command) ids, a different id space from ifaceids.hxx's
// SfxInterfaceId constants. #define, not constexpr: this header doubles as
// an svidl SlotIdFile (code/sdi/codeslots.sdi), and svidl's own tokenizer
// only understands "#define NAME (EXPR)" macro tables (see e.g.
// starmath/inc/starmath.hrc, also both a plain C++ header and a SlotIdFile),
// not real C++ declarations.
//
// include/svl/solar.hrc's own table reserves 20000-27999 for sw/sc/sd and
// leaves 28000-29999 explicitly free (the gap between SID_SD_END and
// SID_OBJ_START) -- clear of every module's own SID_*_START/_END range, so
// this app (which doesn't exist upstream) gets its own small range there
// instead of picking an arbitrary unreserved-looking number.
#define SID_CODE_START               28000

// code/source/sidebar/outputdockingwindow.cxx's SfxChildWindow id --
// toggled by the View menu's .uno:ShowOutput (see code/sdi/code.sdi).
#define SID_CODE_OUTPUT              (SID_CODE_START + 1)

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
