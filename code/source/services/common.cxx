/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "common.hxx"

namespace codeservices
{
OUString GetMimeType(const OUString& rUrl)
{
    // Only the file name's extension counts, not a dot in a directory name.
    const sal_Int32 nSlash = rUrl.lastIndexOf('/');
    const std::u16string_view aName = std::u16string_view(rUrl).substr(nSlash + 1);
    const size_t nDot = aName.rfind('.');
    if (nDot == std::u16string_view::npos)
        return u"text/plain"_ustr;

    const OUString aExt = OUString(aName.substr(nDot + 1)).toAsciiLowerCase();
    static const struct
    {
        const char16_t* pExt;
        const char16_t* pMime;
    } aTable[] = {
        { u"c", u"text/x-csrc" },
        { u"h", u"text/x-chdr" },
        { u"cpp", u"text/x-c++src" },
        { u"cxx", u"text/x-c++src" },
        { u"cc", u"text/x-c++src" },
        { u"hpp", u"text/x-c++hdr" },
        { u"hxx", u"text/x-c++hdr" },
        { u"hh", u"text/x-c++hdr" },
        { u"py", u"text/x-python" },
        { u"rs", u"text/rust" },
        { u"go", u"text/x-go" },
        { u"js", u"text/javascript" },
        { u"json", u"application/json" },
        { u"xml", u"application/xml" },
        { u"md", u"text/markdown" },
        { u"cmake", u"text/x-cmake" },
        { u"txt", u"text/plain" },
    };
    for (const auto& rEntry : aTable)
        if (aExt == rEntry.pExt)
            return OUString(rEntry.pMime);
    return u"text/plain"_ustr;
}

OUString NormalizeUrl(const OUString& rUrl)
{
    OUString aUrl = rUrl;
    while (aUrl.getLength() > 1 && aUrl.endsWith("/") && !aUrl.endsWith(":///"))
        aUrl = aUrl.copy(0, aUrl.getLength() - 1);
    return aUrl;
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
