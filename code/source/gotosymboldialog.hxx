/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/code/index/Symbol.hpp>
#include <com/sun/star/uno/Sequence.hxx>

namespace weld { class Window; }

namespace code
{
/// Edit > Go To Symbol: a dialog in which a part of a name is typed and the
/// symbols of the open projects that contain it are listed (theSymbolIndex);
/// OK shows the chosen one in the editor.
void RunGoToSymbolDialog(weld::Window* pParent);

/// Edit > Go To Definition: shows the definition of the name at the caret in
/// the editor.  If it has several (overloads, a declaration in each of two
/// headers) the user picks one in a dialog; a message in the status bar says so
/// if there is none.
void GoToDefinition(weld::Window* pParent);

/// Shows where a symbol is declared, selecting its name, in the editor.
void ShowSymbol(const css::code::index::Symbol& rSymbol);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
