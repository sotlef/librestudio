/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// The syntax analyzer for C and C++: libclang parses a file (with the compiler
// arguments of the build, from theCompileCommandProvider) and the whole tree
// of the file -- declarations, statements, expressions -- becomes one
// SyntaxNode tree, each node with the generic kind and with libclang's own
// kind spelling in DetailKind.  What a header brings in is not in the tree:
// the tree is of the file, as an outline or a folding view wants it.  The
// parser's complaints become SyntaxDiagnostic entries; only a file libclang
// cannot parse at all makes Succeeded false.

#include "clangcommon.hxx"

#include <com/sun/star/code/core/Position.hpp>
#include <com/sun/star/code/core/Range.hpp>
#include <com/sun/star/code/core/Severity.hpp>
#include <com/sun/star/code/core/XPlugin.hpp>
#include <com/sun/star/code/index/theCompileCommandProvider.hpp>
#include <com/sun/star/code/index/XCompileCommandProvider.hpp>
#include <com/sun/star/code/syntax/ParseResult.hpp>
#include <com/sun/star/code/syntax/SyntaxDiagnostic.hpp>
#include <com/sun/star/code/syntax/SyntaxNode.hpp>
#include <com/sun/star/code/syntax/SyntaxNodeKind.hpp>
#include <com/sun/star/code/syntax/XSyntaxAnalyzer.hpp>
#include <com/sun/star/code/syntax/cxx/XCxxSyntaxInfo.hpp>
#include <com/sun/star/code/syntax/theSyntaxAnalyzers.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/sequence.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <osl/file.hxx>
#include <sal/log.hxx>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace css;
using namespace css::code;
using namespace css::code::syntax;

namespace codeservices
{
namespace
{
OUString ToUrl(const std::string& rPath)
{
    OUString aUrl;
    const OUString aPath = OUString::fromUtf8(rPath);
    return osl::FileBase::getFileURLFromSystemPath(aPath, aUrl) == osl::FileBase::E_None ? aUrl
                                                                                         : OUString();
}

std::string ToPath(const OUString& rUrl)
{
    OUString aPath;
    if (osl::FileBase::getSystemPathFromFileURL(rUrl, aPath) != osl::FileBase::E_None)
        aPath = rUrl;
    return OUStringToOString(aPath, RTL_TEXTENCODING_UTF8).getStr();
}

bool ReadFile(const std::string& rPath, std::string& rText)
{
    OUString aUrl;
    osl::FileBase::getFileURLFromSystemPath(OUString::fromUtf8(rPath), aUrl);
    osl::File aFile(aUrl);
    rText.clear();
    if (aFile.open(osl_File_OpenFlag_Read) != osl::FileBase::E_None)
        return false;
    char aBuffer[16384];
    sal_uInt64 nRead = 0;
    while (aFile.read(aBuffer, sizeof aBuffer, nRead) == osl::FileBase::E_None && nRead > 0)
        rText.append(aBuffer, nRead);
    return true;
}

bool HasSuffix(const std::string& rFile, std::string_view aSuffix)
{
    return rFile.size() >= aSuffix.size()
           && rFile.compare(rFile.size() - aSuffix.size(), aSuffix.size(), aSuffix) == 0;
}

// The command line of the parse: the language first, then what the build
// passes, then what keeps libclang quiet about options meant for another compiler.
std::vector<std::string> ParseArguments(const std::string& rFile,
                                        const uno::Sequence<OUString>& rBuildArguments)
{
    std::vector<std::string> aArguments;
    bool bBuildC = false;
    for (const OUString& rArgument : rBuildArguments)
    {
        const OString aUtf8 = OUStringToOString(rArgument, RTL_TEXTENCODING_UTF8);
        aArguments.emplace_back(aUtf8.getStr(), aUtf8.getLength());
        // -std=c11, -std=gnu99: C; -std=c++20, -std=gnu++17: C++
        const std::string& rLast = aArguments.back();
        if (rLast.rfind("-std=", 0) == 0 && rLast.find("++") == std::string::npos)
            bBuildC = true;
    }
    const bool bC = HasSuffix(rFile, ".c") || (HasSuffix(rFile, ".h") && bBuildC);
    std::vector<std::string> aResult{ "-x", bC ? "c" : "c++" };
    if (!bC && rBuildArguments.getLength() == 0)
        aResult.push_back("-std=c++20");
    aResult.insert(aResult.end(), aArguments.begin(), aArguments.end());
    aResult.push_back("-Qunused-arguments");
    aResult.push_back("-Wno-unknown-warning-option");
    return aResult;
}

// One parse of one file; the translation unit lives as long as this object.
class Unit
{
public:
    Unit(const std::string& rFile, const std::string& rText,
         const uno::Sequence<OUString>& rBuildArguments)
        : m_aArguments(ParseArguments(rFile, rBuildArguments))
    {
        std::vector<const char*> aPointers;
        for (const std::string& rArgument : m_aArguments)
            aPointers.push_back(rArgument.c_str());
        CXUnsavedFile aUnsaved{ rFile.c_str(), rText.data(), static_cast<unsigned long>(rText.size()) };
        m_xIndex = clang_createIndex(0, 0);
        const CXErrorCode eError = clang_parseTranslationUnit2(
            m_xIndex, rFile.c_str(), aPointers.data(), static_cast<int>(aPointers.size()), &aUnsaved,
            1,
            CXTranslationUnit_DetailedPreprocessingRecord | CXTranslationUnit_KeepGoing
                | CXTranslationUnit_Incomplete,
            &m_xUnit);
        if (eError != CXError_Success)
        {
            SAL_WARN("code.clang", "libclang cannot parse " << rFile << " (error " << eError << ")");
            m_xUnit = nullptr;
        }
    }
    ~Unit()
    {
        if (m_xUnit)
            clang_disposeTranslationUnit(m_xUnit);
        clang_disposeIndex(m_xIndex);
    }
    Unit(const Unit&) = delete;
    Unit& operator=(const Unit&) = delete;

    explicit operator bool() const { return m_xUnit != nullptr; }
    CXTranslationUnit Get() const { return m_xUnit; }

private:
    std::vector<std::string> m_aArguments;
    CXIndex m_xIndex = nullptr;
    CXTranslationUnit m_xUnit = nullptr;
};

SyntaxNodeKind KindOf(CXCursorKind eCursor)
{
    switch (eCursor)
    {
        case CXCursor_Namespace:
        case CXCursor_NamespaceAlias:
            return SyntaxNodeKind_NAMESPACE;
        case CXCursor_ClassDecl:
        case CXCursor_ClassTemplate:
        case CXCursor_ClassTemplatePartialSpecialization:
            return SyntaxNodeKind_CLASS;
        case CXCursor_StructDecl:
        case CXCursor_UnionDecl:
            return SyntaxNodeKind_STRUCT;
        case CXCursor_EnumDecl:
            return SyntaxNodeKind_ENUM;
        case CXCursor_FunctionDecl:
        case CXCursor_FunctionTemplate:
            return SyntaxNodeKind_FUNCTION;
        case CXCursor_CXXMethod:
        case CXCursor_Constructor:
        case CXCursor_Destructor:
        case CXCursor_ConversionFunction:
            return SyntaxNodeKind_METHOD;
        case CXCursor_VarDecl:
        case CXCursor_FieldDecl:
        case CXCursor_EnumConstantDecl:
            return SyntaxNodeKind_VARIABLE;
        case CXCursor_ParmDecl:
            return SyntaxNodeKind_PARAMETER;
        case CXCursor_IntegerLiteral:
        case CXCursor_FloatingLiteral:
        case CXCursor_ImaginaryLiteral:
        case CXCursor_StringLiteral:
        case CXCursor_CharacterLiteral:
        case CXCursor_CXXBoolLiteralExpr:
        case CXCursor_CXXNullPtrLiteralExpr:
            return SyntaxNodeKind_LITERAL;
        default:
            break;
    }
    if (clang_isStatement(eCursor))
        return SyntaxNodeKind_STATEMENT;
    if (clang_isExpression(eCursor))
        return SyntaxNodeKind_EXPRESSION;
    if (clang_isPreprocessing(eCursor))
        return SyntaxNodeKind_PREPROCESSOR;
    return SyntaxNodeKind_UNKNOWN;
}

struct TreeWalk
{
    const Lines& rLines;
    std::vector<SyntaxNode>* pChildren;
};

CXChildVisitResult VisitNode(CXCursor aCursor, CXCursor, CXClientData pData)
{
    TreeWalk& rWalk = *static_cast<TreeWalk*>(pData);
    // what a header brings in is not the tree of this file
    if (!clang_Location_isFromMainFile(clang_getCursorLocation(aCursor)))
        return CXChildVisit_Continue;

    const CXCursorKind eKind = clang_getCursorKind(aCursor);
    SyntaxNode aNode;
    aNode.Kind = KindOf(eKind);
    aNode.Spelling = OUString::fromUtf8(Spelling(clang_getCursorSpelling(aCursor)));
    aNode.DetailKind = OUString::fromUtf8(Spelling(clang_getCursorKindSpelling(eKind)));

    const CXSourceRange aExtent = clang_getCursorExtent(aCursor);
    unsigned nFromLine, nFromColumn, nToLine, nToColumn;
    clang_getSpellingLocation(clang_getRangeStart(aExtent), nullptr, &nFromLine, &nFromColumn,
                              nullptr);
    clang_getSpellingLocation(clang_getRangeEnd(aExtent), nullptr, &nToLine, &nToColumn, nullptr);
    aNode.Range.Start.Line = static_cast<sal_Int32>(nFromLine) - 1;
    aNode.Range.Start.Column = rWalk.rLines.Column(nFromLine, nFromColumn);
    aNode.Range.End.Line = static_cast<sal_Int32>(nToLine) - 1;
    aNode.Range.End.Column = rWalk.rLines.Column(nToLine, nToColumn);

    std::vector<SyntaxNode> aChildren;
    TreeWalk aInner{ rWalk.rLines, &aChildren };
    clang_visitChildren(aCursor, VisitNode, &aInner);
    aNode.Children = comphelper::containerToSequence(aChildren);

    rWalk.pChildren->push_back(std::move(aNode));
    return CXChildVisit_Continue;
}

// the text of a file named by libclang, for turning its byte columns into ours
const std::string& TextOf(std::string_view aPath, const std::string& rMainPath,
                          const std::string& rMainText, std::string& rOtherPath,
                          std::string& rOtherText)
{
    if (aPath == rMainPath)
        return rMainText;
    if (rOtherPath != aPath)
    {
        rOtherPath = aPath;
        ReadFile(rOtherPath, rOtherText);
    }
    return rOtherText;
}

void CollectDiagnostics(CXTranslationUnit xUnit, const std::string& rMainPath,
                        const std::string& rMainText, std::vector<SyntaxDiagnostic>& rOut)
{
    std::string aOtherPath, aOtherText;
    const unsigned nCount = clang_getNumDiagnostics(xUnit);
    for (unsigned i = 0; i < nCount; ++i)
    {
        CXDiagnostic xDiagnostic = clang_getDiagnostic(xUnit, i);
        core::Severity eLevel;
        bool bWanted = true;
        switch (clang_getDiagnosticSeverity(xDiagnostic))
        {
            case CXDiagnostic_Note:
                eLevel = core::Severity_INFO;
                break;
            case CXDiagnostic_Warning:
                eLevel = core::Severity_WARNING;
                break;
            case CXDiagnostic_Error:
                eLevel = core::Severity_ERROR;
                break;
            case CXDiagnostic_Fatal:
                eLevel = core::Severity_FATAL;
                break;
            default: // Ignored: suppressed by the arguments, not worth a row
                bWanted = false;
                eLevel = core::Severity_INFO;
                break;
        }
        if (bWanted)
        {
            SyntaxDiagnostic aDiagnostic;
            aDiagnostic.Level = eLevel;
            aDiagnostic.Message = OUString::fromUtf8(Spelling(clang_getDiagnosticSpelling(xDiagnostic)));
            CXFile xFile = nullptr;
            unsigned nLine = 0, nColumn = 0;
            clang_getSpellingLocation(clang_getDiagnosticLocation(xDiagnostic), &xFile, &nLine,
                                      &nColumn, nullptr);
            if (xFile)
            {
                const std::string aPath = Spelling(clang_getFileName(xFile));
                const Lines aLines(TextOf(aPath, rMainPath, rMainText, aOtherPath, aOtherText));
                aDiagnostic.Url = ToUrl(aPath);
                aDiagnostic.Location.Line = nLine ? static_cast<sal_Int32>(nLine) - 1 : 0;
                aDiagnostic.Location.Column = aLines.Column(nLine, nColumn);
            }
            rOut.push_back(std::move(aDiagnostic));
        }
        clang_disposeDiagnostic(xDiagnostic);
    }
}

class ClangSyntaxAnalyzer final
    : public cppu::WeakImplHelper<XSyntaxAnalyzer, syntax::cxx::XCxxSyntaxInfo>
{
public:
    explicit ClangSyntaxAnalyzer(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    // XSyntaxAnalyzer
    OUString SAL_CALL getName() override { return u"clang"_ustr; }
    uno::Sequence<OUString> SAL_CALL getLanguages() override
    {
        return { u"c"_ustr, u"c++"_ustr };
    }
    sal_Bool SAL_CALL supports(const OUString& rUrl) override;
    ParseResult SAL_CALL parse(const OUString& rUrl) override;
    ParseResult SAL_CALL parseText(const OUString& rUrl, const OUString& rText) override;

    // XCxxSyntaxInfo
    OUString SAL_CALL getStandard(const OUString& rUrl) override;

private:
    uno::Sequence<OUString> ArgumentsFor(const OUString& rUrl);
    ParseResult Parse(const std::string& rFile, const std::string& rText,
                      const uno::Sequence<OUString>& rArguments);

    uno::Reference<uno::XComponentContext> m_xContext;
};

// the suffixes codecommon's MIME table calls C and C++ (this library does not
// link it, as the other providers here do not)
sal_Bool ClangSyntaxAnalyzer::supports(const OUString& rUrl)
{
    const std::string aFile = ToPath(rUrl);
    for (const std::string_view aSuffix :
         { ".c", ".h", ".cpp", ".cxx", ".cc", ".hpp", ".hxx", ".hh" })
        if (HasSuffix(aFile, aSuffix))
            return true;
    return false;
}

uno::Sequence<OUString> ClangSyntaxAnalyzer::ArgumentsFor(const OUString& rUrl)
{
    try
    {
        return index::theCompileCommandProvider::get(m_xContext)->getCompileArguments(rUrl);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code.clang", "compile arguments of " << rUrl << ": " << rException.Message);
        return {};
    }
}

ParseResult ClangSyntaxAnalyzer::Parse(const std::string& rFile, const std::string& rText,
                                       const uno::Sequence<OUString>& rArguments)
{
    ParseResult aResult;
    aResult.Succeeded = false;

    Unit aUnit(rFile, rText, rArguments);
    if (!aUnit)
    {
        SyntaxDiagnostic aDiagnostic;
        aDiagnostic.Level = core::Severity_FATAL;
        aDiagnostic.Message = u"libclang cannot parse the file"_ustr;
        aDiagnostic.Url = ToUrl(rFile);
        aResult.Diagnostics = { aDiagnostic };
        return aResult;
    }

    const Lines aLines(rText);
    std::vector<SyntaxNode> aChildren;
    TreeWalk aWalk{ aLines, &aChildren };
    clang_visitChildren(clang_getTranslationUnitCursor(aUnit.Get()), VisitNode, &aWalk);

    sal_Int32 nLines = 1;
    for (const char c : rText)
        if (c == '\n')
            ++nLines;
    aResult.Root.Kind = SyntaxNodeKind_TRANSLATION_UNIT;
    aResult.Root.Spelling = OUString::fromUtf8(rFile);
    aResult.Root.DetailKind
        = OUString::fromUtf8(Spelling(clang_getCursorKindSpelling(CXCursor_TranslationUnit)));
    aResult.Root.Range.Start.Line = 0;
    aResult.Root.Range.Start.Column = 0;
    aResult.Root.Range.End.Line = nLines - 1;
    aResult.Root.Range.End.Column = aLines.Length(static_cast<unsigned>(nLines));
    aResult.Root.Children = comphelper::containerToSequence(aChildren);

    std::vector<SyntaxDiagnostic> aDiagnostics;
    CollectDiagnostics(aUnit.Get(), rFile, rText, aDiagnostics);
    aResult.Diagnostics = comphelper::containerToSequence(aDiagnostics);
    aResult.Succeeded = true;
    return aResult;
}

ParseResult ClangSyntaxAnalyzer::parse(const OUString& rUrl)
{
    const std::string aFile = ToPath(rUrl);
    std::string aText;
    if (!ReadFile(aFile, aText))
    {
        ParseResult aResult;
        aResult.Succeeded = false;
        SyntaxDiagnostic aDiagnostic;
        aDiagnostic.Level = core::Severity_FATAL;
        aDiagnostic.Message = u"cannot read the file"_ustr;
        aDiagnostic.Url = rUrl;
        aResult.Diagnostics = { aDiagnostic };
        return aResult;
    }
    return Parse(aFile, aText, ArgumentsFor(rUrl));
}

ParseResult ClangSyntaxAnalyzer::parseText(const OUString& rUrl, const OUString& rText)
{
    const std::string aFile = ToPath(rUrl);
    const OString aUtf8 = OUStringToOString(rText, RTL_TEXTENCODING_UTF8);
    const std::string aText(aUtf8.getStr(), aUtf8.getLength());
    return Parse(aFile, aText, ArgumentsFor(rUrl));
}

OUString ClangSyntaxAnalyzer::getStandard(const OUString& rUrl)
{
    // the last -std= wins, as it does on a compiler command line
    OUString aStandard;
    for (const OUString& rArgument : ArgumentsFor(rUrl))
    {
        OUString aValue;
        if (rArgument.startsWith(u"-std=", &aValue) && !aValue.isEmpty())
            aStandard = aValue;
    }
    return aStandard;
}

class ClangSyntaxPlugin final : public cppu::WeakImplHelper<core::XPlugin, lang::XServiceInfo>
{
public:
    explicit ClangSyntaxPlugin(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    void SAL_CALL activate() override
    {
        theSyntaxAnalyzers::get(m_xContext)->registerAnalyzer(new ClangSyntaxAnalyzer(m_xContext));
    }

    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.ClangSyntaxPlugin"_ustr;
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
com_sun_star_comp_code_ClangSyntaxPlugin_get_implementation(uno::XComponentContext* pContext,
                                                             uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::ClangSyntaxPlugin(pContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
