/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/code/core/JobStatus.hpp>
#include <com/sun/star/code/search/FindOptions.hpp>
#include <rtl/character.hxx>
#include <rtl/ustrbuf.hxx>
#include <rtl/ustring.hxx>

// Helpers shared by the services of the "codeservices" component library.
namespace codeservices
{
/// The MIME type of a file, by its extension.  The one table the project and
/// document services both use (deepcode kept two copies of it).
OUString GetMimeType(const OUString& rUrl);

/// Strips a trailing '/' (but never from a bare "file:///"), so that one
/// directory has one URL and therefore one identity in the managers.
OUString NormalizeUrl(const OUString& rUrl);

/// the text to search for as an ICU regular expression: what was typed, taken
/// literally unless it is one, with the whole-word frame if asked for
inline OUString FindPattern(const css::code::search::FindOptions& rOptions)
{
    OUStringBuffer aPattern;
    if (rOptions.UseRegex)
        aPattern.append(rOptions.FindWhat);
    else
    {
        for (sal_Int32 i = 0; i < rOptions.FindWhat.getLength(); ++i)
        {
            const sal_Unicode c = rOptions.FindWhat[i];
            if (c < 128 && !rtl::isAsciiAlphanumeric(c) && c != ' ' && c != '_')
                aPattern.append('\\');
            aPattern.append(c);
        }
    }
    OUString aText = aPattern.makeStringAndClear();
    if (rOptions.MatchWholeWord)
        aText = "\\b(?:" + aText + ")\\b";
    return aText;
}


inline bool IsFinalStatus(css::code::core::JobStatus eStatus)
{
    return eStatus == css::code::core::JobStatus_SUCCEEDED
           || eStatus == css::code::core::JobStatus_FAILED
           || eStatus == css::code::core::JobStatus_CANCELLED;
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
