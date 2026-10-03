/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// The provider run on a text and its tokens read back: classification of what
// the text names, and the positions in UTF-16 units (a non-BMP character
// counts for two, a token over several lines is cut into one per line).
// Includes the provider's source, whose classes are in an unnamed namespace.

#include "../../source/clang/clangsemantictokens.cxx"

#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/plugin/TestPlugIn.h>

namespace
{
const OUString Sample
    = u"#include <vector>\n"
      "#define MAX 3\n"
      "namespace ns { class Foo { public: int bar(int x) const; static int s; }; }\n"
      "/* multi\n"
      "   line */\n"
      "int ns::Foo::bar(int x) const { return x + MAX; }\n"
      "const char* k = \"héllo\"; // тест \U0001F600 x\n"_ustr;

class ClangSemanticTokensTest : public CppUnit::TestFixture
{
public:
    void setUp() override
    {
        codeservices::ClangSemanticTokenProvider aProvider;
        m_aTokens = aProvider.provideSemanticTokens(u"file:///tmp/clangtest.cpp"_ustr, Sample, 7);
    }

    // the token at a position, or null
    const semantic::SemanticToken* At(sal_Int32 nLine, sal_Int32 nColumn) const
    {
        for (const auto& rToken : m_aTokens.Tokens)
            if (rToken.Start.Line == nLine && rToken.Start.Column == nColumn)
                return &rToken;
        return nullptr;
    }

    void testVersionIsEchoed()
    {
        CPPUNIT_ASSERT_EQUAL(sal_Int32(7), m_aTokens.Version);
        CPPUNIT_ASSERT(m_aTokens.Tokens.hasElements());
    }

    void testDeclarations()
    {
        auto pNs = At(2, 10); // ns
        CPPUNIT_ASSERT(pNs);
        CPPUNIT_ASSERT_EQUAL(semantic::SemanticTokenKind_NAMESPACE, pNs->Kind);
        auto pFoo = At(2, 21); // Foo
        CPPUNIT_ASSERT(pFoo);
        CPPUNIT_ASSERT_EQUAL(semantic::SemanticTokenKind_CLASS, pFoo->Kind);
        CPPUNIT_ASSERT(pFoo->Modifiers & semantic::SemanticTokenModifiers::DEFINITION);
        auto pBar = At(2, 39); // bar, declared
        CPPUNIT_ASSERT(pBar);
        CPPUNIT_ASSERT_EQUAL(semantic::SemanticTokenKind_METHOD, pBar->Kind);
        CPPUNIT_ASSERT(pBar->Modifiers & semantic::SemanticTokenModifiers::DECLARATION);
        CPPUNIT_ASSERT(!(pBar->Modifiers & semantic::SemanticTokenModifiers::DEFINITION));
        auto pS = At(2, 68); // s
        CPPUNIT_ASSERT(pS);
        CPPUNIT_ASSERT(pS->Modifiers & semantic::SemanticTokenModifiers::STATIC);
    }

    void testUsesTakeTheKindOfWhatTheyName()
    {
        auto pNs = At(5, 4);
        CPPUNIT_ASSERT(pNs);
        CPPUNIT_ASSERT_EQUAL(semantic::SemanticTokenKind_NAMESPACE, pNs->Kind);
        auto pFoo = At(5, 8);
        CPPUNIT_ASSERT(pFoo);
        CPPUNIT_ASSERT_EQUAL(semantic::SemanticTokenKind_CLASS, pFoo->Kind);
        auto pX = At(5, 39); // x in the body
        CPPUNIT_ASSERT(pX);
        CPPUNIT_ASSERT_EQUAL(semantic::SemanticTokenKind_PARAMETER, pX->Kind);
        // the definition of bar is a definition
        auto pBar = At(5, 13);
        CPPUNIT_ASSERT(pBar);
        CPPUNIT_ASSERT(pBar->Modifiers & semantic::SemanticTokenModifiers::DEFINITION);
    }

    void testPreprocessor()
    {
        auto pHash = At(1, 0);
        CPPUNIT_ASSERT(pHash);
        CPPUNIT_ASSERT_EQUAL(semantic::SemanticTokenKind_PREPROCESSOR, pHash->Kind);
        auto pMacro = At(1, 8); // MAX in #define
        CPPUNIT_ASSERT(pMacro);
        CPPUNIT_ASSERT_EQUAL(semantic::SemanticTokenKind_MACRO, pMacro->Kind);
        CPPUNIT_ASSERT(pMacro->Modifiers & semantic::SemanticTokenModifiers::DEFINITION);
        auto pUse = At(5, 43); // MAX in the function
        CPPUNIT_ASSERT(pUse);
        CPPUNIT_ASSERT_EQUAL(semantic::SemanticTokenKind_MACRO, pUse->Kind);
        // the header name of an #include is a string
        auto pHeader = At(0, 10);
        CPPUNIT_ASSERT(pHeader);
        CPPUNIT_ASSERT_EQUAL(semantic::SemanticTokenKind_STRING, pHeader->Kind);
    }

    void testBlockCommentIsOneTokenPerLine()
    {
        auto pFirst = At(3, 0);
        auto pSecond = At(4, 0);
        CPPUNIT_ASSERT(pFirst && pSecond);
        CPPUNIT_ASSERT_EQUAL(semantic::SemanticTokenKind_COMMENT, pFirst->Kind);
        CPPUNIT_ASSERT_EQUAL(semantic::SemanticTokenKind_COMMENT, pSecond->Kind);
        CPPUNIT_ASSERT_EQUAL(sal_Int32(8), pFirst->Length); // "/* multi"
        CPPUNIT_ASSERT_EQUAL(sal_Int32(10), pSecond->Length); // "   line */"
    }

    void testPositionsAreUtf16()
    {
        // "héllo" is 7 units with its quotes, though 8 bytes in UTF-8
        auto pString = At(6, 16);
        CPPUNIT_ASSERT(pString);
        CPPUNIT_ASSERT_EQUAL(semantic::SemanticTokenKind_STRING, pString->Kind);
        CPPUNIT_ASSERT_EQUAL(sal_Int32(7), pString->Length);
        // "// тест <emoji> x": 3 + 4 + 1 + 2 (surrogate pair) + 2 units
        auto pComment = At(6, 25);
        CPPUNIT_ASSERT(pComment);
        CPPUNIT_ASSERT_EQUAL(semantic::SemanticTokenKind_COMMENT, pComment->Kind);
        CPPUNIT_ASSERT_EQUAL(sal_Int32(12), pComment->Length);
    }

    void testTokensDoNotOverlap()
    {
        sal_Int32 nLine = -1;
        sal_Int32 nEnd = 0;
        for (const auto& rToken : m_aTokens.Tokens)
        {
            if (rToken.Start.Line != nLine)
            {
                nLine = rToken.Start.Line;
                nEnd = 0;
            }
            CPPUNIT_ASSERT(rToken.Start.Column >= nEnd);
            CPPUNIT_ASSERT(rToken.Length > 0);
            nEnd = rToken.Start.Column + rToken.Length;
        }
    }

    CPPUNIT_TEST_SUITE(ClangSemanticTokensTest);
    CPPUNIT_TEST(testVersionIsEchoed);
    CPPUNIT_TEST(testDeclarations);
    CPPUNIT_TEST(testUsesTakeTheKindOfWhatTheyName);
    CPPUNIT_TEST(testPreprocessor);
    CPPUNIT_TEST(testBlockCommentIsOneTokenPerLine);
    CPPUNIT_TEST(testPositionsAreUtf16);
    CPPUNIT_TEST(testTokensDoNotOverlap);
    CPPUNIT_TEST_SUITE_END();

private:
    semantic::SemanticTokens m_aTokens;
};

CPPUNIT_TEST_SUITE_REGISTRATION(ClangSemanticTokensTest);
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
