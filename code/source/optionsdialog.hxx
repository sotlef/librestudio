/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace weld { class Window; }

namespace code
{
/// Tools > Options: Code's own settings dialog, replacing the cui-hosted
/// options tree of the office applications.  The settings live in officecfg's
/// Office/Code.xcs; the dialog reads them, and writes them back when it is
/// closed with OK.  Returns true then, so the caller can refresh what shows
/// the settings.
bool RunOptionsDialog(weld::Window* pParent);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
