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
/// Edit > Find in Files / Replace in Files: the Find and Replace dialog with
/// its two tabs.  The texts, the options and the histories are kept in
/// officecfg's Office/Code.xcs (group Find), so they outlive the dialog and
/// the application.  The searching itself is not there yet: the buttons only
/// put the entered text into the histories.
void RunFindDialog(weld::Window* pParent, bool bReplaceTab);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
