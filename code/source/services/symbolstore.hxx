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
#include <com/sun/star/code/index/XSymbolStore.hpp>
#include <cppuhelper/implbase.hxx>

namespace codeservices
{
/// The store that keeps the symbols in memory; the index starts with it.
css::uno::Reference<css::code::index::XSymbolStore> CreateSymbolStore();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
