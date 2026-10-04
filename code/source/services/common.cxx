/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "common.hxx"

#include <com/sun/star/code/document/XDocument.hpp>
#include <com/sun/star/code/document/XDocumentManager.hpp>
#include <com/sun/star/code/document/XTextDocument.hpp>
#include <com/sun/star/code/document/theDocumentManager.hpp>
#include <osl/file.hxx>
#include <rtl/string.hxx>
#include <rtl/strbuf.hxx>
#include <rtl/textcvt.h>

using namespace css;

namespace codeservices
{
namespace
{
// A file larger than this is not text anyone reads by hand.
constexpr sal_uInt64 MaxFileSize = 16 * 1024 * 1024;
}

bool ReadText(const uno::Reference<uno::XComponentContext>& xContext, const OUString& rUrl,
              OUString& rText)
{
    try
    {
        uno::Reference<code::document::XTextDocument> xDocument(
            code::document::theDocumentManager::get(xContext)->getDocument(rUrl), uno::UNO_QUERY);
        if (xDocument.is())
        {
            rText = xDocument->getText();
            return rText.indexOf(u'\0') < 0;
        }
    }
    catch (const uno::Exception&)
    {
    }

    osl::File aFile(rUrl);
    if (aFile.open(osl_File_OpenFlag_Read) != osl::FileBase::E_None)
        return false;
    osl::DirectoryItem aItem;
    osl::FileStatus aStatus(osl_FileStatus_Mask_FileSize);
    if (osl::DirectoryItem::get(rUrl, aItem) != osl::FileBase::E_None
        || aItem.getFileStatus(aStatus) != osl::FileBase::E_None
        || aStatus.getFileSize() > MaxFileSize)
        return false;

    OStringBuffer aBytes;
    char aBuffer[16384];
    sal_uInt64 nRead = 0;
    while (aFile.read(aBuffer, sizeof aBuffer, nRead) == osl::FileBase::E_None && nRead > 0)
        aBytes.append(aBuffer, static_cast<sal_Int32>(nRead));
    const OString aAll = aBytes.makeStringAndClear();
    // a NUL byte: a binary file
    if (aAll.indexOf('\0') >= 0)
        return false;
    rText = OUString(aAll.getStr(), aAll.getLength(), RTL_TEXTENCODING_UTF8,
                     RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_DEFAULT
                         | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_DEFAULT
                         | RTL_TEXTTOUNICODE_FLAGS_INVALID_DEFAULT);
    return true;
}

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
