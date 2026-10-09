/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// The completion provider for C and C++: libclang's clang_codeCompleteAt is
// asked what may stand at the caret, with the text as the editor holds it and
// the compiler arguments of the build.  Only this library knows libclang; the
// rest of Code sees XCompletionProvider.
//
// Latency is what completion lives or dies by, and parsing a translation unit
// from scratch takes seconds where answering from a parsed one takes tens of
// milliseconds.  The other providers of this library parse per request and may,
// since they answer off the typing path; completion cannot, so this one keeps
// the last few translation units, with their precompiled preambles, and only
// reparses one when the file's include lines or compiler arguments changed.
// The first request for a file still pays for the full parse.

#include "clangcommon.hxx"

#include <com/sun/star/code/complete/CompletionItem.hpp>
#include <com/sun/star/code/complete/CompletionItemKind.hpp>
#include <com/sun/star/code/complete/CompletionRequest.hpp>
#include <com/sun/star/code/complete/CompletionResult.hpp>
#include <com/sun/star/code/complete/XCompletionProvider.hpp>
#include <com/sun/star/code/complete/theCompletionController.hpp>
#include <com/sun/star/code/core/XPlugin.hpp>
#include <com/sun/star/code/index/theCompileCommandProvider.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/sequence.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <osl/file.hxx>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>

#include <algorithm>
#include <cctype>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

using namespace css;
using namespace css::code;
using namespace css::code::complete;

namespace codeservices
{
namespace
{
// how many proposals a result may carry; above it the tail of the ranking is
// dropped and the result says it is incomplete
constexpr sal_Int32 MaxItems = 10000;

// how many translation units are kept parsed; a unit with its preamble is tens
// to hundreds of megabytes, so only the few files being edited are worth it
constexpr size_t MaxUnits = 4;

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

bool HasSuffix(std::string_view aFile, std::string_view aSuffix)
{
    return aFile.size() >= aSuffix.size()
           && aFile.compare(aFile.size() - aSuffix.size(), aSuffix.size(), aSuffix) == 0;
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

// The preprocessor lines of the text.  clang_codeCompleteAt reparses the main
// file against the unit's precompiled preamble but does not notice that an
// #include changed; when this signature differs from the parse's, the unit is
// reparsed first so that a header just included starts completing.
std::string IncludeSignature(const std::string& rText)
{
    std::string aResult;
    size_t nLineStart = 0;
    for (size_t i = 0; i <= rText.size(); ++i)
    {
        if (i == rText.size() || rText[i] == '\n')
        {
            size_t nFirst = nLineStart;
            while (nFirst < i && (rText[nFirst] == ' ' || rText[nFirst] == '\t'))
                ++nFirst;
            if (nFirst < i && rText[nFirst] == '#')
            {
                aResult.append(rText, nLineStart, i - nLineStart);
                aResult += '\n';
            }
            nLineStart = i + 1;
        }
    }
    return aResult;
}

// One parsed file, kept for the next request.  Owns its index and unit.
class CachedUnit
{
public:
    CachedUnit(const std::string& rFile, const std::string& rText,
               const uno::Sequence<OUString>& rBuildArguments)
        : m_aFile(rFile)
        , m_aArguments(ParseArguments(rFile, rBuildArguments))
        , m_aIncludes(IncludeSignature(rText))
    {
        std::vector<const char*> aPointers;
        for (const std::string& rArgument : m_aArguments)
            aPointers.push_back(rArgument.c_str());
        CXUnsavedFile aUnsaved{ rFile.c_str(), rText.data(), static_cast<unsigned long>(rText.size()) };
        m_xIndex = clang_createIndex(0, 0);
        // the editing defaults carry the precompiled preamble and the cached
        // completion results, which is what makes the second request fast
        const unsigned nOptions = clang_defaultEditingTranslationUnitOptions()
                                  | CXTranslationUnit_DetailedPreprocessingRecord
                                  | CXTranslationUnit_KeepGoing;
        const CXErrorCode eError = clang_parseTranslationUnit2(
            m_xIndex, rFile.c_str(), aPointers.data(), static_cast<int>(aPointers.size()), &aUnsaved,
            1, nOptions, &m_xUnit);
        if (eError != CXError_Success)
        {
            SAL_WARN("code.clang", "libclang cannot parse " << rFile << " (error " << eError << ")");
            m_xUnit = nullptr;
        }
    }
    ~CachedUnit()
    {
        if (m_xUnit)
            clang_disposeTranslationUnit(m_xUnit);
        clang_disposeIndex(m_xIndex);
    }
    CachedUnit(const CachedUnit&) = delete;
    CachedUnit& operator=(const CachedUnit&) = delete;

    explicit operator bool() const { return m_xUnit != nullptr; }
    CXTranslationUnit Get() const { return m_xUnit; }
    const std::string& File() const { return m_aFile; }

    // whether the unit may answer for this text and these arguments as it is
    bool Fits(const std::vector<std::string>& rArguments) const { return m_aArguments == rArguments; }

    // brings the preamble up to date; false means the unit became unusable
    bool Refresh(const std::string& rText)
    {
        const std::string aIncludes = IncludeSignature(rText);
        if (aIncludes == m_aIncludes)
            return true;
        CXUnsavedFile aUnsaved{ m_aFile.c_str(), rText.data(),
                                static_cast<unsigned long>(rText.size()) };
        if (clang_reparseTranslationUnit(m_xUnit, 1, &aUnsaved,
                                         clang_defaultReparseOptions(m_xUnit))
            != 0)
        {
            // libclang says the unit must not be used after a failed reparse
            clang_disposeTranslationUnit(m_xUnit);
            m_xUnit = nullptr;
            return false;
        }
        m_aIncludes = aIncludes;
        return true;
    }

private:
    std::string m_aFile;
    std::vector<std::string> m_aArguments;
    std::string m_aIncludes;
    CXIndex m_xIndex = nullptr;
    CXTranslationUnit m_xUnit = nullptr;
};

bool KindOf(CXCursorKind eCursor, CompletionItemKind& reKind)
{
    switch (eCursor)
    {
        case CXCursor_Namespace:
        case CXCursor_NamespaceAlias:
            reKind = CompletionItemKind_NAMESPACE;
            return true;
        case CXCursor_ClassDecl:
        case CXCursor_ClassTemplate:
        case CXCursor_ClassTemplatePartialSpecialization:
            reKind = CompletionItemKind_CLASS;
            return true;
        case CXCursor_StructDecl:
        case CXCursor_UnionDecl:
            reKind = CompletionItemKind_STRUCT;
            return true;
        case CXCursor_EnumDecl:
            reKind = CompletionItemKind_ENUM;
            return true;
        case CXCursor_EnumConstantDecl:
            reKind = CompletionItemKind_ENUM_MEMBER;
            return true;
        case CXCursor_TypedefDecl:
        case CXCursor_TypeAliasDecl:
        case CXCursor_TypeAliasTemplateDecl:
        case CXCursor_TemplateTypeParameter:
        case CXCursor_TemplateTemplateParameter:
            reKind = CompletionItemKind_CLASS;
            return true;
        case CXCursor_FunctionDecl:
        case CXCursor_FunctionTemplate:
        case CXCursor_OverloadCandidate:
            reKind = CompletionItemKind_FUNCTION;
            return true;
        case CXCursor_CXXMethod:
        case CXCursor_Destructor:
        case CXCursor_ConversionFunction:
            reKind = CompletionItemKind_METHOD;
            return true;
        case CXCursor_Constructor:
            reKind = CompletionItemKind_CONSTRUCTOR;
            return true;
        case CXCursor_FieldDecl:
            reKind = CompletionItemKind_FIELD;
            return true;
        case CXCursor_VarDecl:
        case CXCursor_ParmDecl:
        case CXCursor_NonTypeTemplateParameter:
            reKind = CompletionItemKind_VARIABLE;
            return true;
        case CXCursor_MacroDefinition:
            reKind = CompletionItemKind_MACRO;
            return true;
        // keywords and whole patterns ("for (...) {...}") both come as this
        case CXCursor_NotImplemented:
            reKind = CompletionItemKind_KEYWORD;
            return true;
        default:
            reKind = CompletionItemKind_UNKNOWN;
            return true;
    }
}

// The label of a completion string: its chunks in order, a placeholder as its
// text ("push_back(const T& value)"), optional chunks (default arguments)
// included.  The result type is left out; it is the Detail.
void AppendChunks(CXCompletionString xString, OUStringBuffer& rLabel, CompletionItem& rItem,
                  bool bTopLevel)
{
    const unsigned nChunks = clang_getNumCompletionChunks(xString);
    for (unsigned i = 0; i < nChunks; ++i)
    {
        const CXCompletionChunkKind eChunk = clang_getCompletionChunkKind(xString, i);
        switch (eChunk)
        {
            case CXCompletionChunk_TypedText:
            {
                const std::string aText = Spelling(clang_getCompletionChunkText(xString, i));
                rLabel.append(OUString::fromUtf8(aText));
                if (bTopLevel && rItem.InsertText.isEmpty())
                    rItem.InsertText = OUString::fromUtf8(aText);
                break;
            }
            case CXCompletionChunk_ResultType:
                if (bTopLevel && rItem.Detail.isEmpty())
                    rItem.Detail
                        = OUString::fromUtf8(Spelling(clang_getCompletionChunkText(xString, i)));
                break;
            case CXCompletionChunk_Optional:
                AppendChunks(clang_getCompletionChunkCompletionString(xString, i), rLabel, rItem,
                             false);
                break;
            case CXCompletionChunk_Informative:
                // a qualifier the user does not type ("vector::"); not shown
                break;
            case CXCompletionChunk_VerticalSpace:
                rLabel.append(" ");
                break;
            default:
                rLabel.append(
                    OUString::fromUtf8(Spelling(clang_getCompletionChunkText(xString, i))));
                break;
        }
    }
}

// zero-padded, so that the lexicographic order of SortText is the numeric
// order of libclang's priority (smaller is better, and it fits in 5 digits)
OUString SortTextOf(unsigned nPriority)
{
    OUString aNumber = OUString::number(std::min(nPriority, 99999u));
    OUStringBuffer aResult;
    for (sal_Int32 i = aNumber.getLength(); i < 5; ++i)
        aResult.append("0");
    aResult.append(aNumber);
    return aResult.makeStringAndClear();
}

class ClangCompletionProvider final : public cppu::WeakImplHelper<XCompletionProvider>
{
public:
    explicit ClangCompletionProvider(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    OUString SAL_CALL getName() override { return u"clang"_ustr; }
    uno::Sequence<OUString> SAL_CALL getLanguages() override
    {
        return { u"c"_ustr, u"c++"_ustr };
    }
    sal_Bool SAL_CALL supports(const OUString& rUrl) override;
    CompletionResult SAL_CALL complete(const CompletionRequest& rRequest) override;

private:
    // the cached unit for this file and these arguments, parsing if there is
    // none; null if parsing failed.  Called with m_aMutex held.
    CachedUnit* GetUnit(const std::string& rFile, const std::string& rText,
                        const uno::Sequence<OUString>& rBuildArguments);

    uno::Reference<uno::XComponentContext> m_xContext;
    // One request at a time: a translation unit is not thread-safe, and one
    // caret asks one question.  Held across the whole parse-and-complete.
    std::mutex m_aMutex;
    // most recently used first
    std::list<std::unique_ptr<CachedUnit>> m_aUnits;
};

sal_Bool ClangCompletionProvider::supports(const OUString& rUrl)
{
    static constexpr std::string_view aSuffixes[]
        = { ".c", ".h", ".cc", ".cxx", ".cpp", ".c++", ".hh", ".hxx", ".hpp", ".h++" };
    const std::string aFile = ToPath(rUrl);
    std::string aLower(aFile);
    std::transform(aLower.begin(), aLower.end(), aLower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (std::string_view aSuffix : aSuffixes)
        if (HasSuffix(aLower, aSuffix))
            return true;
    return false;
}

CachedUnit* ClangCompletionProvider::GetUnit(const std::string& rFile, const std::string& rText,
                                             const uno::Sequence<OUString>& rBuildArguments)
{
    const std::vector<std::string> aArguments = ParseArguments(rFile, rBuildArguments);
    for (auto it = m_aUnits.begin(); it != m_aUnits.end(); ++it)
    {
        if ((*it)->File() != rFile)
            continue;
        if ((*it)->Fits(aArguments) && (*it)->Refresh(rText))
        {
            m_aUnits.splice(m_aUnits.begin(), m_aUnits, it);
            return m_aUnits.front().get();
        }
        // the arguments changed, or the reparse broke it: parse anew
        m_aUnits.erase(it);
        break;
    }
    auto pUnit = std::make_unique<CachedUnit>(rFile, rText, rBuildArguments);
    if (!*pUnit)
        return nullptr;
    m_aUnits.push_front(std::move(pUnit));
    while (m_aUnits.size() > MaxUnits)
        m_aUnits.pop_back();
    return m_aUnits.front().get();
}

CompletionResult ClangCompletionProvider::complete(const CompletionRequest& rRequest)
{
    CompletionResult aResult;
    aResult.Incomplete = false;

    const std::string aFile = ToPath(rRequest.Url);
    std::string aText;
    if (rRequest.Text.isEmpty())
        aText = ReadFile(aFile);
    else
    {
        const OString aUtf8 = OUStringToOString(rRequest.Text, RTL_TEXTENCODING_UTF8);
        aText.assign(aUtf8.getStr(), aUtf8.getLength());
    }

    uno::Sequence<OUString> aBuildArguments;
    try
    {
        aBuildArguments
            = index::theCompileCommandProvider::get(m_xContext)->getCompileArguments(rRequest.Url);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code.clang", "no compile arguments for " << rRequest.Url << ": "
                                                           << rException.Message);
    }

    std::unique_lock aGuard(m_aMutex);
    CachedUnit* pUnit = GetUnit(aFile, aText, aBuildArguments);
    if (!pUnit)
        return aResult;

    const Lines aLines(aText);
    const unsigned nLine = static_cast<unsigned>(rRequest.Position.Line) + 1;
    const unsigned nColumn = aLines.ByteColumn(nLine, rRequest.Position.Column);
    CXUnsavedFile aUnsaved{ aFile.c_str(), aText.data(), static_cast<unsigned long>(aText.size()) };
    CXCodeCompleteResults* pResults
        = clang_codeCompleteAt(pUnit->Get(), aFile.c_str(), nLine, nColumn, &aUnsaved, 1,
                               clang_defaultCodeCompleteOptions() | CXCodeComplete_IncludeMacros
                                   | CXCodeComplete_IncludeBriefComments);
    if (!pResults)
    {
        SAL_WARN("code.clang", "libclang cannot complete in " << rRequest.Url << " at " << nLine
                                                              << ":" << nColumn);
        return aResult;
    }

    std::vector<CompletionItem> aItems;
    aItems.reserve(pResults->NumResults);
    for (unsigned i = 0; i < pResults->NumResults; ++i)
    {
        const CXCompletionResult& rCandidate = pResults->Results[i];
        const CXAvailabilityKind eAvailability
            = clang_getCompletionAvailability(rCandidate.CompletionString);
        if (eAvailability == CXAvailability_NotAvailable
            || eAvailability == CXAvailability_NotAccessible)
            continue;

        CompletionItem aItem;
        aItem.Deprecated = eAvailability == CXAvailability_Deprecated;
        KindOf(rCandidate.CursorKind, aItem.Kind);
        OUStringBuffer aLabel;
        AppendChunks(rCandidate.CompletionString, aLabel, aItem, true);
        aItem.Label = aLabel.makeStringAndClear();
        if (aItem.InsertText.isEmpty())
            aItem.InsertText = aItem.Label;
        if (aItem.Label.isEmpty())
            continue;
        aItem.Documentation = OUString::fromUtf8(
            Spelling(clang_getCompletionBriefComment(rCandidate.CompletionString)));
        aItem.SortText = SortTextOf(clang_getCompletionPriority(rCandidate.CompletionString));
        aItems.push_back(std::move(aItem));
    }
    clang_disposeCodeCompleteResults(pResults);

    // the provider's ranking is the priority; the list is cut there when it is
    // too long, so what is dropped is the least likely
    std::stable_sort(aItems.begin(), aItems.end(),
                     [](const CompletionItem& rA, const CompletionItem& rB)
                     {
                         if (rA.SortText != rB.SortText)
                             return rA.SortText < rB.SortText;
                         return rA.Label < rB.Label;
                     });
    if (aItems.size() > static_cast<size_t>(MaxItems))
    {
        aItems.resize(MaxItems);
        aResult.Incomplete = true;
    }
    aResult.Items = comphelper::containerToSequence(aItems);
    return aResult;
}

class ClangCompletionPlugin final : public cppu::WeakImplHelper<core::XPlugin, lang::XServiceInfo>
{
public:
    explicit ClangCompletionPlugin(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    void SAL_CALL activate() override
    {
        theCompletionController::get(m_xContext)->registerProvider(
            new ClangCompletionProvider(m_xContext));
    }

    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.ClangCompletionPlugin"_ustr;
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
com_sun_star_comp_code_ClangCompletionPlugin_get_implementation(uno::XComponentContext* pContext,
                                                                 uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::ClangCompletionPlugin(pContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
