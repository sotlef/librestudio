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

// Status bar items (code/uiconfig/code/statusbar/statusbar.xml):
// the background-tasks bubble button (.uno:CodeBackgroundTasks,
// code/source/tasksstatusbarcontrol.cxx) and the "Ready" text
// (.uno:CodeStatusReady, plain SfxStringItem state shown by the default
// SfxStatusBarControl).
#define SID_CODE_STATUS_TASKS        (SID_CODE_START + 2)
#define SID_CODE_STATUS_READY        (SID_CODE_START + 3)

// File > Open Folder (.uno:CodeOpenFolder) and File > Save All
// (.uno:CodeSaveAll): handled by code::Shell::Execute(), which hands them to
// the project and document services.
#define SID_CODE_OPENFOLDER          (SID_CODE_START + 4)
#define SID_CODE_SAVEALL             (SID_CODE_START + 5)

// File > Open > Project... (.uno:CodeOpenProject): asks for a project or
// solution file in any format a registered project-format reader knows.
#define SID_CODE_OPENPROJECT         (SID_CODE_START + 6)

// The toolbar's configuration combo box (.uno:CodeConfiguration,
// code/source/configurationcontrol.cxx).
#define SID_CODE_CONFIGURATION       (SID_CODE_START + 7)

// Build > Build Solution (.uno:CodeBuild) and Build > Cancel (.uno:CodeCancelBuild).
#define SID_CODE_BUILD               (SID_CODE_START + 8)
#define SID_CODE_CANCELBUILD         (SID_CODE_START + 9)

// File > Recent Projects and Solutions (.uno:CodeRecentProjects): a sub-menu
// made by RecentProjectsMenuController (code/source/recentprojectsmenucontroller.cxx).
#define SID_CODE_RECENTPROJECTS      (SID_CODE_START + 10)

// Build > Rebuild Solution (.uno:CodeRebuild): a build that starts from clean.
#define SID_CODE_REBUILD             (SID_CODE_START + 11)

// Build > Clean Solution (.uno:CodeClean): removes what the builds produced.
#define SID_CODE_CLEAN               (SID_CODE_START + 12)

// Tools > Options... (.uno:CodeOptions): Code's own settings dialog, in place
// of the cui-hosted .uno:OptionsTreeDialog of the office applications.
#define SID_CODE_OPTIONS             (SID_CODE_START + 13)

// Edit > Find in Files... / Replace in Files... (.uno:CodeFindInFiles,
// .uno:CodeReplaceInFiles): the Find and Replace dialog on its first or second tab.
#define SID_CODE_FINDINFILES         (SID_CODE_START + 14)
#define SID_CODE_REPLACEINFILES      (SID_CODE_START + 15)

// Modify Find of the Find in Files results: the Find dialog as it was left by
// the Find All that made the results; the argument is its FindOptions, as an Any.
#define SID_CODE_MODIFYFIND          (SID_CODE_START + 16)
#define SID_CODE_ARG_FINDOPTIONS     (SID_CODE_START + 17)

// Edit > Go To Definition (.uno:CodeGoToDefinition, F12): the definition of the
// name at the caret, from the symbol index.
#define SID_CODE_GOTODEFINITION      (SID_CODE_START + 18)

// Edit > Go To Symbol... (.uno:CodeGoToSymbol, Ctrl+T): a symbol of the open
// projects, found by a part of its name.
#define SID_CODE_GOTOSYMBOL          (SID_CODE_START + 19)

// The status bar's version control item (.uno:CodeVcsBranch): the current
// branch of the working copy the active project is in; empty (the item is
// hidden) if it is in none.  A click opens the branches popup.
#define SID_CODE_STATUS_VCS          (SID_CODE_START + 20)

// The toolbar's startup item (.uno:CodeStartup): the Start button with the
// combo box of the project's launchable targets; a custom item window, so
// the slot only keeps the item enabled.
#define SID_CODE_STARTUP             (SID_CODE_START + 21)

// Runs the startup target under the debugger (.uno:CodeStartDebugging).
#define SID_CODE_START_DEBUGGING     (SID_CODE_START + 22)

// Runs the startup target without it (.uno:CodeStartWithoutDebugging).
#define SID_CODE_START_WITHOUT_DEBUGGING (SID_CODE_START + 23)

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
