/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// What the libclang-based providers of "Code" share.

#pragma once

#include <clang-c/Index.h>

#include <sal/types.h>

#include <algorithm>
#include <string>
#include <vector>

namespace codeservices
{
inline std::string Spelling(CXString aString)
{
    std::string aResult = clang_getCString(aString) ? clang_getCString(aString) : "";
    clang_disposeString(aString);
    return aResult;
}

// The lines of the UTF-8 text, so that libclang's (line, byte column) can be
// turned into the (line, UTF-16 column) of this API.
class Lines
{
public:
    explicit Lines(const std::string& rText)
        : m_rText(rText)
    {
        m_aStarts.push_back(0);
        for (size_t i = 0; i < rText.size(); ++i)
            if (rText[i] == '\n')
                m_aStarts.push_back(i + 1);
    }

    // line and byte column are 1-based, as libclang has them
    sal_Int32 Column(unsigned nLine, unsigned nByteColumn) const
    {
        const size_t nFrom = LineStart(nLine);
        const size_t nTo = std::min(nFrom + (nByteColumn ? nByteColumn - 1 : 0), m_rText.size());
        return Units(nFrom, nTo);
    }

    // the inverse of Column: the 1-based byte column of a UTF-16 column of a line
    unsigned ByteColumn(unsigned nLine, sal_Int32 nUnits) const
    {
        const size_t nFrom = LineStart(nLine);
        size_t i = nFrom;
        sal_Int32 nSeen = 0;
        while (i < m_rText.size() && m_rText[i] != '\n' && nSeen < nUnits)
        {
            const unsigned char c = static_cast<unsigned char>(m_rText[i]);
            nSeen += c >= 0xF0 ? 2 : 1;
            ++i;
            while (i < m_rText.size() && (static_cast<unsigned char>(m_rText[i]) & 0xC0) == 0x80)
                ++i;
        }
        return static_cast<unsigned>(i - nFrom) + 1;
    }

    // length of a line without its terminator
    sal_Int32 Length(unsigned nLine) const
    {
        const size_t nFrom = LineStart(nLine);
        size_t nTo = nLine < m_aStarts.size() ? m_aStarts[nLine] : m_rText.size();
        while (nTo > nFrom && (m_rText[nTo - 1] == '\n' || m_rText[nTo - 1] == '\r'))
            --nTo;
        return Units(nFrom, nTo);
    }

private:
    size_t LineStart(unsigned nLine) const
    {
        return nLine >= 1 && nLine <= m_aStarts.size() ? m_aStarts[nLine - 1] : m_rText.size();
    }

    // UTF-16 code units in [nFrom, nTo): one per scalar value, two for those
    // outside the BMP (they start with a 4-byte lead)
    sal_Int32 Units(size_t nFrom, size_t nTo) const
    {
        sal_Int32 nUnits = 0;
        for (size_t i = nFrom; i < nTo; ++i)
        {
            const unsigned char c = static_cast<unsigned char>(m_rText[i]);
            if ((c & 0xC0) == 0x80)
                continue; // continuation byte
            nUnits += c >= 0xF0 ? 2 : 1;
        }
        return nUnits;
    }

    const std::string& m_rText;
    std::vector<size_t> m_aStarts;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
