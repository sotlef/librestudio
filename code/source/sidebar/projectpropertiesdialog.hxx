/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/code/project/XProjectProperties.hpp>
#include <com/sun/star/uno/Reference.hxx>
#include <rtl/ustring.hxx>

namespace weld { class Widget; }

namespace code::sidebar
{
/// The Properties dialog of Solution Explorer: a tree of pages on the left,
/// the names and values of the chosen page on the right, the whole value of
/// the chosen row below it.  It shows what a project or one of its items says
/// of itself (XProjectProperties) and knows nothing of the format.
void RunProjectPropertiesDialog(
    weld::Widget* pParent, const OUString& rTitle,
    const css::uno::Reference<css::code::project::XProjectProperties>& xProperties);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
