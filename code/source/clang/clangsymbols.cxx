/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// The symbol provider for C and C++: libclang parses a file (as the editor
// holds it, with the compiler arguments of the build) and every declaration of
// it becomes a Symbol, identified by its USR so that the declaration in a
// header and the definition in a source file are one entity.  Function bodies
// are skipped when symbols are extracted: only what the file declares is wanted
// there, and the bodies are most of the time.  Looking up the name under the
// caret parses them, since a use may be in one.

#include "clangcommon.hxx"

#include <com/sun/star/code/core/Position.hpp>
#include <com/sun/star/code/core/Range.hpp>
#include <com/sun/star/code/core/XPlugin.hpp>
#include <com/sun/star/code/index/Symbol.hpp>
#include <com/sun/star/code/index/SymbolKind.hpp>
#include <com/sun/star/code/index/XSymbolProvider.hpp>
#include <com/sun/star/code/index/theSymbolIndex.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/sequence.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <osl/file.hxx>
#include <rtl/string.hxx>
#include <sal/log.hxx>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

using namespace css;
using namespace css::code;
using namespace css::code::index;

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

std::string ReadFile(const std::string& rPath)
{
    OUString aUrl;
    osl::FileBase::getFileURLFromSystemPath(OUString::fromUtf8(rPath), aUrl);
    osl::File aFile(aUrl);
    std::string aText;
    if (aFile.open(osl_File_OpenFlag_Read) != osl::FileBase::E_None)
        return aText;
    char aBuffer[16384];
    sal_uInt64 nRead = 0;
    while (aFile.read(aBuffer, sizeof aBuffer, nRead) == osl::FileBase::E_None && nRead > 0)
        aText.append(aBuffer, nRead);
    return aText;
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
         const uno::Sequence<OUString>& rBuildArguments, bool bSkipBodies)
        : m_aFile(rFile)
        , m_aArguments(ParseArguments(rFile, rBuildArguments))
    {
        std::vector<const char*> aPointers;
        for (const std::string& rArgument : m_aArguments)
            aPointers.push_back(rArgument.c_str());
        CXUnsavedFile aUnsaved{ rFile.c_str(), rText.data(), static_cast<unsigned long>(rText.size()) };
        m_xIndex = clang_createIndex(0, 0);
        unsigned nOptions = CXTranslationUnit_DetailedPreprocessingRecord
                            | CXTranslationUnit_KeepGoing | CXTranslationUnit_Incomplete;
        if (bSkipBodies)
            nOptions |= CXTranslationUnit_SkipFunctionBodies;
        const CXErrorCode eError = clang_parseTranslationUnit2(
            m_xIndex, rFile.c_str(), aPointers.data(), static_cast<int>(aPointers.size()), &aUnsaved,
            1, nOptions, &m_xUnit);
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
    std::string m_aFile;
    std::vector<std::string> m_aArguments;
    CXIndex m_xIndex = nullptr;
    CXTranslationUnit m_xUnit = nullptr;
};

bool KindOf(CXCursorKind eCursor, SymbolKind& reKind)
{
    switch (eCursor)
    {
        case CXCursor_Namespace:
            reKind = SymbolKind_NAMESPACE;
            return true;
        case CXCursor_ClassDecl:
        case CXCursor_ClassTemplate:
        case CXCursor_ClassTemplatePartialSpecialization:
            reKind = SymbolKind_CLASS;
            return true;
        case CXCursor_StructDecl:
            reKind = SymbolKind_STRUCT;
            return true;
        case CXCursor_UnionDecl:
            reKind = SymbolKind_UNION;
            return true;
        case CXCursor_EnumDecl:
            reKind = SymbolKind_ENUM;
            return true;
        case CXCursor_EnumConstantDecl:
            reKind = SymbolKind_ENUM_MEMBER;
            return true;
        case CXCursor_TypedefDecl:
        case CXCursor_TypeAliasDecl:
        case CXCursor_TypeAliasTemplateDecl:
            reKind = SymbolKind_TYPEDEF;
            return true;
        case CXCursor_FunctionDecl:
        case CXCursor_FunctionTemplate:
            reKind = SymbolKind_FUNCTION;
            return true;
        case CXCursor_CXXMethod:
        case CXCursor_Destructor:
        case CXCursor_ConversionFunction:
            reKind = SymbolKind_METHOD;
            return true;
        case CXCursor_Constructor:
            reKind = SymbolKind_CONSTRUCTOR;
            return true;
        case CXCursor_FieldDecl:
            reKind = SymbolKind_FIELD;
            return true;
        case CXCursor_VarDecl:
            reKind = SymbolKind_VARIABLE;
            return true;
        case CXCursor_MacroDefinition:
            reKind = SymbolKind_MACRO;
            return true;
        case CXCursor_TemplateTypeParameter:
        case CXCursor_TemplateTemplateParameter:
            reKind = SymbolKind_TEMPLATE_PARAMETER;
            return true;
        default:
            return false;
    }
}

// where the walk goes into: scopes whose members are symbols
bool IsScope(CXCursorKind eCursor)
{
    switch (eCursor)
    {
        case CXCursor_Namespace:
        case CXCursor_ClassDecl:
        case CXCursor_StructDecl:
        case CXCursor_UnionDecl:
        case CXCursor_EnumDecl:
        case CXCursor_ClassTemplate:
        case CXCursor_ClassTemplatePartialSpecialization:
        case CXCursor_LinkageSpec:
            return true;
        default:
            return false;
    }
}

// "ns::Class" from the semantic parents of a cursor; an out-of-line definition
// is thereby found under the class it belongs to
std::string ScopeOf(CXCursor aCursor)
{
    std::vector<std::string> aNames;
    for (CXCursor aParent = clang_getCursorSemanticParent(aCursor);
         !clang_Cursor_isNull(aParent) && !clang_isInvalid(clang_getCursorKind(aParent))
         && clang_getCursorKind(aParent) != CXCursor_TranslationUnit;
         aParent = clang_getCursorSemanticParent(aParent))
    {
        if (clang_getCursorKind(aParent) == CXCursor_LinkageSpec)
            continue;
        aNames.push_back(Spelling(clang_getCursorSpelling(aParent)));
    }
    std::string aScope;
    for (auto it = aNames.rbegin(); it != aNames.rend(); ++it)
    {
        if (!aScope.empty())
            aScope += "::";
        aScope += it->empty() ? "(anonymous)" : *it;
    }
    return aScope;
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
        rOtherText = ReadFile(rOtherPath);
    }
    return rOtherText;
}

// What is known of a declaration; false if it is not one worth a symbol.
struct Describer
{
    const std::string& rMainPath;
    const std::string& rMainText;
    std::string aOtherPath;
    std::string aOtherText;

    bool operator()(CXCursor aCursor, Symbol& rSymbol)
    {
        SymbolKind eKind;
        if (!KindOf(clang_getCursorKind(aCursor), eKind))
            return false;
        const std::string aName = Spelling(clang_getCursorSpelling(aCursor));
        if (aName.empty())
            return false;
        const std::string aUsr = Spelling(clang_getCursorUSR(aCursor));
        if (aUsr.empty())
            return false;

        CXSourceRange aNameRange = clang_Cursor_getSpellingNameRange(aCursor, 0, 0);
        if (clang_Range_isNull(aNameRange))
            aNameRange = clang_getCursorExtent(aCursor);
        CXFile xFile;
        unsigned nFromLine, nFromColumn, nToLine, nToColumn;
        clang_getSpellingLocation(clang_getRangeStart(aNameRange), &xFile, &nFromLine, &nFromColumn,
                                  nullptr);
        clang_getSpellingLocation(clang_getRangeEnd(aNameRange), nullptr, &nToLine, &nToColumn,
                                  nullptr);
        if (!xFile)
            return false;
        const std::string aPath = Spelling(clang_getFileName(xFile));
        const Lines aLines(TextOf(aPath, rMainPath, rMainText, aOtherPath, aOtherText));

        rSymbol.Id = OUString::fromUtf8(aUsr);
        rSymbol.Name = OUString::fromUtf8(aName);
        const std::string aScope = ScopeOf(aCursor);
        rSymbol.Container = OUString::fromUtf8(aScope);
        rSymbol.QualifiedName = OUString::fromUtf8(aScope.empty() ? aName : aScope + "::" + aName);
        rSymbol.Kind = eKind;
        rSymbol.Url = ToUrl(aPath);
        rSymbol.Range.Start.Line = static_cast<sal_Int32>(nFromLine) - 1;
        rSymbol.Range.Start.Column = aLines.Column(nFromLine, nFromColumn);
        rSymbol.Range.End.Line = static_cast<sal_Int32>(nToLine) - 1;
        rSymbol.Range.End.Column = aLines.Column(nToLine, nToColumn);
        rSymbol.IsDefinition = clang_isCursorDefinition(aCursor) != 0;
        switch (eKind)
        {
            case SymbolKind_FUNCTION:
            case SymbolKind_METHOD:
            case SymbolKind_CONSTRUCTOR:
                rSymbol.Detail = OUString::fromUtf8(Spelling(clang_getCursorDisplayName(aCursor)));
                break;
            case SymbolKind_FIELD:
            case SymbolKind_VARIABLE:
                rSymbol.Detail
                    = OUString::fromUtf8(Spelling(clang_getTypeSpelling(clang_getCursorType(aCursor))));
                break;
            default:
                break;
        }
        return true;
    }
};

struct Walk
{
    Describer aDescribe;
    std::vector<Symbol> aSymbols;
};

CXChildVisitResult Visit(CXCursor aCursor, CXCursor, CXClientData pData)
{
    Walk& rWalk = *static_cast<Walk*>(pData);
    // what a header brings in is that header's own symbols
    if (!clang_Location_isFromMainFile(clang_getCursorLocation(aCursor)))
        return CXChildVisit_Continue;
    const CXCursorKind eKind = clang_getCursorKind(aCursor);
    Symbol aSymbol;
    if (rWalk.aDescribe(aCursor, aSymbol))
        rWalk.aSymbols.push_back(std::move(aSymbol));
    // a function's parameters and locals are not symbols of the file
    return IsScope(eKind) || eKind == CXCursor_FunctionTemplate ? CXChildVisit_Recurse : CXChildVisit_Continue;
}

class ClangSymbolProvider final : public cppu::WeakImplHelper<XSymbolProvider>
{
public:
    OUString SAL_CALL getName() override { return u"clang"_ustr; }
    uno::Sequence<OUString> SAL_CALL getSupportedMimeTypes() override
    {
        return { u"text/x-csrc"_ustr, u"text/x-chdr"_ustr, u"text/x-c++src"_ustr,
                 u"text/x-c++hdr"_ustr };
    }
    uno::Sequence<Symbol> SAL_CALL extractSymbols(const OUString& rUrl, const OUString& rText,
                                                  const uno::Sequence<OUString>& rArguments) override;
    uno::Any SAL_CALL resolveSymbolAt(const OUString& rUrl, const OUString& rText,
                                      const uno::Sequence<OUString>& rArguments,
                                      const core::Position& rPosition) override;
};

uno::Sequence<Symbol> ClangSymbolProvider::extractSymbols(const OUString& rUrl, const OUString& rText,
                                                          const uno::Sequence<OUString>& rArguments)
{
    const std::string aFile = ToPath(rUrl);
    const OString aUtf8 = OUStringToOString(rText, RTL_TEXTENCODING_UTF8);
    const std::string aText(aUtf8.getStr(), aUtf8.getLength());

    Unit aUnit(aFile, aText, rArguments, /*bSkipBodies*/ true);
    if (!aUnit)
        return {};
    Walk aWalk{ Describer{ aFile, aText, {}, {} }, {} };
    clang_visitChildren(clang_getTranslationUnitCursor(aUnit.Get()), Visit, &aWalk);
    return comphelper::containerToSequence(aWalk.aSymbols);
}

uno::Any ClangSymbolProvider::resolveSymbolAt(const OUString& rUrl, const OUString& rText,
                                              const uno::Sequence<OUString>& rArguments,
                                              const core::Position& rPosition)
{
    const std::string aFile = ToPath(rUrl);
    const OString aUtf8 = OUStringToOString(rText, RTL_TEXTENCODING_UTF8);
    const std::string aText(aUtf8.getStr(), aUtf8.getLength());

    Unit aUnit(aFile, aText, rArguments, /*bSkipBodies*/ false);
    if (!aUnit)
        return {};
    const Lines aLines(aText);
    CXFile xFile = clang_getFile(aUnit.Get(), aFile.c_str());
    if (!xFile)
        return {};
    const unsigned nLine = static_cast<unsigned>(rPosition.Line) + 1;
    const CXSourceLocation aLocation
        = clang_getLocation(aUnit.Get(), xFile, nLine, aLines.ByteColumn(nLine, rPosition.Column));
    CXCursor aCursor = clang_getCursor(aUnit.Get(), aLocation);
    if (clang_Cursor_isNull(aCursor) || clang_isInvalid(clang_getCursorKind(aCursor)))
        return {};

    CXCursor aTarget = clang_getCursorReferenced(aCursor);
    if (clang_Cursor_isNull(aTarget) || clang_isInvalid(clang_getCursorKind(aTarget)))
        aTarget = aCursor; // a declaration refers to itself
    // the definition, where there is one in what was parsed
    const CXCursor aDefinition = clang_getCursorDefinition(aTarget);
    if (!clang_Cursor_isNull(aDefinition) && !clang_isInvalid(clang_getCursorKind(aDefinition)))
        aTarget = aDefinition;

    Describer aDescribe{ aFile, aText, {}, {} };
    Symbol aSymbol;
    if (!aDescribe(aTarget, aSymbol))
        return {};
    return uno::Any(aSymbol);
}

class ClangSymbolPlugin final : public cppu::WeakImplHelper<core::XPlugin, lang::XServiceInfo>
{
public:
    explicit ClangSymbolPlugin(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    void SAL_CALL activate() override
    {
        theSymbolIndex::get(m_xContext)->registerProvider(new ClangSymbolProvider);
    }

    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.ClangSymbolPlugin"_ustr;
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
com_sun_star_comp_code_ClangSymbolPlugin_get_implementation(uno::XComponentContext* pContext,
                                                             uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::ClangSymbolPlugin(pContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
