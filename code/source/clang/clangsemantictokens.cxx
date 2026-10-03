/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// The semantic-token provider for C and C++: libclang parses the text of the
// document (as it is in the editor, not as it is on disk) and every token of
// it is classified by what it names -- a class, a method, a parameter, a macro.
// Only this library knows libclang; the rest of Code sees XSemanticTokenProvider.
//
// Not done yet: the compiler flags.  The text is parsed with default flags, so
// what depends on the project's include paths and defines stays unclassified
// where its declarations cannot be found.  The flags are the CMake file API's to
// give, once the project passes them on.

#include <clang-c/Index.h>

#include <com/sun/star/code/core/XPlugin.hpp>
#include <com/sun/star/code/semantic/SemanticTokenKind.hpp>
#include <com/sun/star/code/semantic/SemanticTokenModifiers.hpp>
#include <com/sun/star/code/semantic/SemanticTokens.hpp>
#include <com/sun/star/code/semantic/XSemanticTokenProvider.hpp>
#include <com/sun/star/code/semantic/theSemanticTokenController.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/sequence.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <osl/file.hxx>
#include <sal/log.hxx>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

using namespace css;
using namespace css::code;
using namespace css::code::semantic;

namespace codeservices
{
namespace
{
std::string Spelling(CXString aString)
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

struct Classified
{
    SemanticTokenKind eKind;
    sal_Int32 nModifiers = 0;
};

bool Classify(CXCursor aCursor, Classified& rResult)
{
    CXCursor aTarget = aCursor;
    const CXCursorKind eCursor = clang_getCursorKind(aCursor);
    // a use is what it refers to
    if (clang_isReference(eCursor) || eCursor == CXCursor_DeclRefExpr
        || eCursor == CXCursor_MemberRefExpr || eCursor == CXCursor_CallExpr)
    {
        const CXCursor aReferenced = clang_getCursorReferenced(aCursor);
        if (!clang_Cursor_isNull(aReferenced) && !clang_isInvalid(clang_getCursorKind(aReferenced)))
            aTarget = aReferenced;
    }

    switch (clang_getCursorKind(aTarget))
    {
        case CXCursor_Namespace:
        case CXCursor_NamespaceAlias:
            rResult.eKind = SemanticTokenKind_NAMESPACE;
            break;
        case CXCursor_ClassDecl:
        case CXCursor_ClassTemplate:
        case CXCursor_ClassTemplatePartialSpecialization:
            rResult.eKind = SemanticTokenKind_CLASS;
            break;
        case CXCursor_StructDecl:
        case CXCursor_UnionDecl:
            rResult.eKind = SemanticTokenKind_STRUCT;
            break;
        case CXCursor_EnumDecl:
            rResult.eKind = SemanticTokenKind_ENUM;
            break;
        case CXCursor_EnumConstantDecl:
            rResult.eKind = SemanticTokenKind_ENUM_MEMBER;
            break;
        case CXCursor_TypedefDecl:
        case CXCursor_TypeAliasDecl:
        case CXCursor_TypeAliasTemplateDecl:
            rResult.eKind = SemanticTokenKind_TYPE;
            break;
        case CXCursor_TemplateTypeParameter:
        case CXCursor_TemplateTemplateParameter:
            rResult.eKind = SemanticTokenKind_TYPE_PARAMETER;
            break;
        case CXCursor_FunctionDecl:
        case CXCursor_FunctionTemplate:
            rResult.eKind = SemanticTokenKind_FUNCTION;
            break;
        case CXCursor_CXXMethod:
        case CXCursor_Constructor:
        case CXCursor_Destructor:
        case CXCursor_ConversionFunction:
            rResult.eKind = SemanticTokenKind_METHOD;
            if (clang_CXXMethod_isStatic(aTarget))
                rResult.nModifiers |= SemanticTokenModifiers::STATIC;
            if (clang_CXXMethod_isPureVirtual(aTarget))
                rResult.nModifiers |= SemanticTokenModifiers::ABSTRACT;
            break;
        case CXCursor_FieldDecl:
            rResult.eKind = SemanticTokenKind_PROPERTY;
            break;
        case CXCursor_VarDecl:
            rResult.eKind = SemanticTokenKind_VARIABLE;
            if (clang_Cursor_getStorageClass(aTarget) == CX_SC_Static)
                rResult.nModifiers |= SemanticTokenModifiers::STATIC;
            break;
        case CXCursor_ParmDecl:
            rResult.eKind = SemanticTokenKind_PARAMETER;
            break;
        case CXCursor_NonTypeTemplateParameter:
            rResult.eKind = SemanticTokenKind_PARAMETER;
            break;
        case CXCursor_LabelStmt:
            rResult.eKind = SemanticTokenKind_LABEL;
            break;
        case CXCursor_MacroDefinition:
        case CXCursor_MacroExpansion:
            rResult.eKind = SemanticTokenKind_MACRO;
            break;
        default:
            return false;
    }

    if (clang_getCursorAvailability(aTarget) == CXAvailability_Deprecated)
        rResult.nModifiers |= SemanticTokenModifiers::DEPRECATED;
    if (clang_isConstQualifiedType(clang_getCursorType(aTarget))
        && (rResult.eKind == SemanticTokenKind_VARIABLE || rResult.eKind == SemanticTokenKind_PROPERTY))
        rResult.nModifiers |= SemanticTokenModifiers::READONLY;
    // the token is the declaration itself, not a use of it
    if (clang_isDeclaration(eCursor))
    {
        rResult.nModifiers |= SemanticTokenModifiers::DECLARATION;
        if (clang_isCursorDefinition(aCursor))
            rResult.nModifiers |= SemanticTokenModifiers::DEFINITION;
    }
    return true;
}

bool IsStringLiteral(std::string_view aText)
{
    // a prefix (L, u, U, u8, R and combinations) and then a quote
    const size_t nQuote = aText.find_first_of("\"'");
    return nQuote != std::string_view::npos && nQuote <= 3;
}

class ClangSemanticTokenProvider final : public cppu::WeakImplHelper<XSemanticTokenProvider>
{
public:
    OUString SAL_CALL getName() override { return u"clang"_ustr; }
    uno::Sequence<OUString> SAL_CALL getSupportedMimeTypes() override
    {
        return { u"text/x-csrc"_ustr, u"text/x-chdr"_ustr, u"text/x-c++src"_ustr,
                 u"text/x-c++hdr"_ustr };
    }
    SemanticTokens SAL_CALL provideSemanticTokens(const OUString& rUrl, const OUString& rText,
                                                  sal_Int32 nVersion) override;
};

SemanticTokens ClangSemanticTokenProvider::provideSemanticTokens(const OUString& rUrl,
                                                                 const OUString& rText,
                                                                 sal_Int32 nVersion)
{
    SemanticTokens aResult;
    aResult.Version = nVersion;

    OUString aPath;
    if (osl::FileBase::getSystemPathFromFileURL(rUrl, aPath) != osl::FileBase::E_None)
        aPath = rUrl;
    const std::string aFile = OUStringToOString(aPath, RTL_TEXTENCODING_UTF8).getStr();
    const OString aUtf8 = OUStringToOString(rText, RTL_TEXTENCODING_UTF8);
    const std::string aText(aUtf8.getStr(), aUtf8.getLength());

    const bool bC = aFile.size() > 2 && aFile.compare(aFile.size() - 2, 2, ".c") == 0;
    std::vector<const char*> aArguments{ "-x", bC ? "c" : "c++" };
    if (!bC)
        aArguments.push_back("-std=c++20");

    CXUnsavedFile aUnsaved{ aFile.c_str(), aText.data(), static_cast<unsigned long>(aText.size()) };
    CXIndex xIndex = clang_createIndex(0, 0);
    CXTranslationUnit xUnit = nullptr;
    const CXErrorCode eError = clang_parseTranslationUnit2(
        xIndex, aFile.c_str(), aArguments.data(), static_cast<int>(aArguments.size()), &aUnsaved, 1,
        CXTranslationUnit_DetailedPreprocessingRecord | CXTranslationUnit_KeepGoing
            | CXTranslationUnit_Incomplete,
        &xUnit);
    if (eError != CXError_Success || !xUnit)
    {
        SAL_WARN("code.clang", "libclang cannot parse " << rUrl << " (error " << eError << ")");
        clang_disposeIndex(xIndex);
        aResult.Version = -1;
        return aResult;
    }

    CXFile xMain = clang_getFile(xUnit, aFile.c_str());
    const CXSourceRange aWhole
        = clang_getRange(clang_getLocationForOffset(xUnit, xMain, 0),
                         clang_getLocationForOffset(xUnit, xMain, static_cast<unsigned>(aText.size())));
    CXToken* pTokens = nullptr;
    unsigned nTokens = 0;
    clang_tokenize(xUnit, aWhole, &pTokens, &nTokens);
    std::vector<CXCursor> aCursors(nTokens);
    if (nTokens)
        clang_annotateTokens(xUnit, pTokens, nTokens, aCursors.data());

    const Lines aLines(aText);
    std::vector<SemanticToken> aOut;
    auto emit = [&](unsigned nLine, sal_Int32 nColumn, sal_Int32 nLength, SemanticTokenKind eKind,
                    sal_Int32 nModifiers)
    {
        if (nLength <= 0)
            return;
        SemanticToken aToken;
        aToken.Start.Line = static_cast<sal_Int32>(nLine) - 1;
        aToken.Start.Column = nColumn;
        aToken.Length = nLength;
        aToken.Kind = eKind;
        aToken.Modifiers = nModifiers;
        aOut.push_back(aToken);
    };
    // a token that runs over several lines becomes one token per line
    auto emitRange = [&](CXSourceRange aRange, SemanticTokenKind eKind, sal_Int32 nModifiers)
    {
        unsigned nFromLine, nFromColumn, nToLine, nToColumn;
        clang_getSpellingLocation(clang_getRangeStart(aRange), nullptr, &nFromLine, &nFromColumn,
                                  nullptr);
        clang_getSpellingLocation(clang_getRangeEnd(aRange), nullptr, &nToLine, &nToColumn, nullptr);
        if (nFromLine == nToLine)
        {
            const sal_Int32 nStart = aLines.Column(nFromLine, nFromColumn);
            emit(nFromLine, nStart, aLines.Column(nToLine, nToColumn) - nStart, eKind, nModifiers);
            return;
        }
        const sal_Int32 nStart = aLines.Column(nFromLine, nFromColumn);
        emit(nFromLine, nStart, aLines.Length(nFromLine) - nStart, eKind, nModifiers);
        for (unsigned nLine = nFromLine + 1; nLine < nToLine; ++nLine)
            emit(nLine, 0, aLines.Length(nLine), eKind, nModifiers);
        emit(nToLine, 0, aLines.Column(nToLine, nToColumn), eKind, nModifiers);
    };

    // state of the preprocessor directive being walked (tokens on the line of its '#')
    unsigned nDirectiveLine = 0;
    int nDirectiveIndex = 0;
    std::string aDirective;
    unsigned nPreviousLine = 0;
    bool bFirstOnLine = true;

    for (unsigned i = 0; i < nTokens; ++i)
    {
        const CXSourceRange aRange = clang_getTokenExtent(xUnit, pTokens[i]);
        unsigned nLine;
        clang_getSpellingLocation(clang_getRangeStart(aRange), nullptr, &nLine, nullptr, nullptr);
        bFirstOnLine = nLine != nPreviousLine;
        nPreviousLine = nLine;
        // the end of the last line of a multi-line token is where the next may begin
        unsigned nEndLine;
        clang_getSpellingLocation(clang_getRangeEnd(aRange), nullptr, &nEndLine, nullptr, nullptr);
        nPreviousLine = nEndLine;

        const CXTokenKind eKind = clang_getTokenKind(pTokens[i]);
        const std::string aSpelling = Spelling(clang_getTokenSpelling(xUnit, pTokens[i]));

        if (eKind == CXToken_Punctuation && aSpelling == "#" && bFirstOnLine)
        {
            nDirectiveLine = nLine;
            nDirectiveIndex = 0;
            aDirective.clear();
        }
        if (nDirectiveLine == nLine)
        {
            const int nIndex = nDirectiveIndex++;
            if (nIndex == 0)
            {
                emitRange(aRange, SemanticTokenKind_PREPROCESSOR, 0);
                continue;
            }
            if (nIndex == 1)
            {
                aDirective = aSpelling;
                emitRange(aRange, SemanticTokenKind_PREPROCESSOR, 0);
                continue;
            }
            if (aDirective == "include" || aDirective == "include_next" || aDirective == "import")
            {
                emitRange(aRange, SemanticTokenKind_STRING, 0);
                continue;
            }
            if ((aDirective == "define" || aDirective == "undef") && nIndex == 2)
            {
                emitRange(aRange, SemanticTokenKind_MACRO,
                          aDirective == "define" ? SemanticTokenModifiers::DEFINITION : 0);
                continue;
            }
        }

        switch (eKind)
        {
            case CXToken_Keyword:
                emitRange(aRange, SemanticTokenKind_KEYWORD, 0);
                break;
            case CXToken_Comment:
                emitRange(aRange, SemanticTokenKind_COMMENT, 0);
                break;
            case CXToken_Literal:
                emitRange(aRange, IsStringLiteral(aSpelling) ? SemanticTokenKind_STRING
                                                             : SemanticTokenKind_NUMBER,
                          0);
                break;
            case CXToken_Punctuation:
                if (aSpelling.find_first_of(";,(){}[]") == std::string::npos)
                    emitRange(aRange, SemanticTokenKind_OPERATOR, 0);
                break;
            case CXToken_Identifier:
            {
                Classified aClassified;
                if (Classify(aCursors[i], aClassified))
                    emitRange(aRange, aClassified.eKind, aClassified.nModifiers);
                break;
            }
        }
    }

    clang_disposeTokens(xUnit, pTokens, nTokens);
    clang_disposeTranslationUnit(xUnit);
    clang_disposeIndex(xIndex);

    aResult.Tokens = comphelper::containerToSequence(aOut);
    return aResult;
}

class ClangSemanticPlugin final : public cppu::WeakImplHelper<core::XPlugin, lang::XServiceInfo>
{
public:
    explicit ClangSemanticPlugin(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    void SAL_CALL activate() override
    {
        theSemanticTokenController::get(m_xContext)->registerProvider(new ClangSemanticTokenProvider);
    }

    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.ClangSemanticPlugin"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override
    {
        return { u"com.sun.star.code.core.Plugin"_ustr };
    }

private:
    uno::Reference<uno::XComponentContext> m_xContext;
};
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_ClangSemanticPlugin_get_implementation(uno::XComponentContext* pContext,
                                                               uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::ClangSemanticPlugin(pContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
