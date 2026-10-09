/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// The refactoring provider for C and C++, whose one capability so far is
// Rename Symbol.  prepareRename parses the file (as the editor holds it, with
// the compiler arguments of the build), puts a cursor on the position and
// says whether a rename there can work at all; rename finds every occurrence
// and describes the edit, applying nothing.
//
// Occurrences are found precisely, not textually: the target's USR names the
// entity across translation units, and clang_findReferencesInFile collects
// where that entity is spelled in a file.  The symbol index only narrows the
// search -- its store knows which files the project has, and a file whose
// text does not even contain the name cannot refer to it -- every candidate
// that remains is parsed and asked for real.
//
// Honest limits of this first version:
//  - Only files the symbol index knows (the open projects' sources and
//    headers) are searched, and only those libclang can parse; a file whose
//    parse fails is skipped with a warning, not an error.
//  - Other files are read from disk; only the document the rename starts in
//    is taken from the editor's buffer (it is handed along to every parse,
//    for the case that it is an edited header the others include).
//  - An occurrence that only a macro expansion spells (token pasting, a name
//    assembled by the preprocessor) is not found, and an occurrence whose
//    spelled text is not exactly the name (a destructor's ~Foo, an implicit
//    conversion) is left alone rather than half-edited.
//  - Only plain identifiers are renamed: no operators, no destructors (the
//    class itself can be renamed), no literals, nothing from a system header.
//  - The new name is checked for identifier shape, not against the keyword
//    list of the language standard in use.

#include "clangcommon.hxx"

#include <com/sun/star/code/core/Position.hpp>
#include <com/sun/star/code/core/Range.hpp>
#include <com/sun/star/code/core/XPlugin.hpp>
#include <com/sun/star/code/index/XSymbolStore.hpp>
#include <com/sun/star/code/index/theCompileCommandProvider.hpp>
#include <com/sun/star/code/index/theSymbolIndex.hpp>
#include <com/sun/star/code/refactor/FileEdit.hpp>
#include <com/sun/star/code/refactor/RefactorException.hpp>
#include <com/sun/star/code/refactor/RenameTarget.hpp>
#include <com/sun/star/code/refactor/TextEdit.hpp>
#include <com/sun/star/code/refactor/WorkspaceEdit.hpp>
#include <com/sun/star/code/refactor/XRefactoringProvider.hpp>
#include <com/sun/star/code/refactor/XRenameProvider.hpp>
#include <com/sun/star/code/refactor/theRefactoringController.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/sequence.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <osl/file.hxx>
#include <sal/log.hxx>

#include <algorithm>
#include <cctype>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace css;
using namespace css::code;
using namespace css::code::refactor;

namespace codeservices
{
namespace
{
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

// One parse of one file; the translation unit lives as long as this object.
// Unlike the symbol provider's parse it may carry more than one unsaved file:
// the file itself and the document the rename started in, which may be an
// edited header this file includes.  The texts behind rUnsaved must outlive
// the Unit.
class Unit
{
public:
    Unit(const std::string& rFile, const uno::Sequence<OUString>& rBuildArguments,
         const std::vector<CXUnsavedFile>& rUnsaved)
        : m_aArguments(ParseArguments(rFile, rBuildArguments))
    {
        std::vector<const char*> aPointers;
        for (const std::string& rArgument : m_aArguments)
            aPointers.push_back(rArgument.c_str());
        m_xIndex = clang_createIndex(0, 0);
        const unsigned nOptions = CXTranslationUnit_DetailedPreprocessingRecord
                                  | CXTranslationUnit_KeepGoing | CXTranslationUnit_Incomplete;
        const CXErrorCode eError = clang_parseTranslationUnit2(
            m_xIndex, rFile.c_str(), aPointers.data(), static_cast<int>(aPointers.size()),
            const_cast<CXUnsavedFile*>(rUnsaved.data()), static_cast<unsigned>(rUnsaved.size()),
            nOptions, &m_xUnit);
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

// Good enough for C and C++: a letter or an underscore, then letters, digits
// and underscores.  What this refuses -- operators, destructors, universal
// character names -- this provider does not rename.
bool IsIdentifier(std::string_view aName)
{
    if (aName.empty())
        return false;
    if (!std::isalpha(static_cast<unsigned char>(aName[0])) && aName[0] != '_')
        return false;
    return std::all_of(aName.begin() + 1, aName.end(),
                       [](char c)
                       { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; });
}

// The identifier itself at a cursor: the spelled name of a declaration, the
// name piece of a reference, the extent as a last resort (which the caller's
// does-it-spell-the-name check then judges).
CXSourceRange NameRangeOf(CXCursor aCursor)
{
    CXSourceRange aRange = clang_isDeclaration(clang_getCursorKind(aCursor))
                               ? clang_Cursor_getSpellingNameRange(aCursor, 0, 0)
                               : clang_getCursorReferenceNameRange(aCursor, 0, 0);
    if (clang_Range_isNull(aRange))
        aRange = clang_getCursorExtent(aCursor);
    return aRange;
}

// The core::Range of aRange if it lies in xFile and spells exactly rName;
// anything else (a macro playing tricks, a name libclang could not isolate,
// an implicit mention with no text of its own) is not an edit we can make.
bool EditRangeOf(CXSourceRange aRange, CXFile xFile, const std::string& rText,
                 const Lines& rLines, const std::string& rName, core::Range& rResult)
{
    if (clang_Range_isNull(aRange))
        return false;
    CXFile xFromFile = nullptr;
    CXFile xToFile = nullptr;
    unsigned nFromLine, nFromColumn, nFromOffset, nToLine, nToColumn, nToOffset;
    clang_getFileLocation(clang_getRangeStart(aRange), &xFromFile, &nFromLine, &nFromColumn,
                          &nFromOffset);
    clang_getFileLocation(clang_getRangeEnd(aRange), &xToFile, &nToLine, &nToColumn, &nToOffset);
    if (!xFromFile || !xToFile || !clang_File_isEqual(xFromFile, xFile)
        || !clang_File_isEqual(xToFile, xFile))
        return false;
    if (nFromOffset > nToOffset || nToOffset > rText.size()
        || rText.compare(nFromOffset, nToOffset - nFromOffset, rName) != 0)
        return false;
    rResult.Start.Line = static_cast<sal_Int32>(nFromLine) - 1;
    rResult.Start.Column = rLines.Column(nFromLine, nFromColumn);
    rResult.End.Line = static_cast<sal_Int32>(nToLine) - 1;
    rResult.End.Column = rLines.Column(nToLine, nToColumn);
    return true;
}

// Follows the cursor at the request position to the declaration it names,
// or says in rReason why there is nothing to rename there.
bool ResolveTarget(CXCursor aCursor, CXCursor& rTarget, OUString& rReason)
{
    const CXCursorKind eKind = clang_getCursorKind(aCursor);
    if (clang_Cursor_isNull(aCursor) || clang_isInvalid(eKind)
        || eKind == CXCursor_TranslationUnit)
    {
        rReason = u"there is no name at this position"_ustr;
        return false;
    }
    switch (eKind)
    {
        case CXCursor_IntegerLiteral:
        case CXCursor_FloatingLiteral:
        case CXCursor_ImaginaryLiteral:
        case CXCursor_StringLiteral:
        case CXCursor_CharacterLiteral:
        case CXCursor_CXXBoolLiteralExpr:
        case CXCursor_CXXNullPtrLiteralExpr:
            rReason = u"a literal cannot be renamed"_ustr;
            return false;
        default:
            break;
    }
    CXCursor aTarget = clang_getCursorReferenced(aCursor);
    if (clang_Cursor_isNull(aTarget) || clang_isInvalid(clang_getCursorKind(aTarget)))
    {
        if (clang_isDeclaration(eKind))
            aTarget = aCursor;
        else
        {
            // a keyword or punctuation puts the cursor on the enclosing
            // statement or expression, which refers to nothing
            rReason = clang_isStatement(eKind) || clang_isExpression(eKind)
                          ? u"a keyword cannot be renamed"_ustr
                          : u"there is nothing renameable at this position"_ustr;
            return false;
        }
    }
    const CXCursorKind eTargetKind = clang_getCursorKind(aTarget);
    if (!clang_isDeclaration(eTargetKind) && eTargetKind != CXCursor_MacroDefinition)
    {
        rReason = u"the position does not name a declaration"_ustr;
        return false;
    }
    const std::string aName = Spelling(clang_getCursorSpelling(aTarget));
    if (aName.empty())
    {
        rReason = u"an anonymous entity has no name to rename"_ustr;
        return false;
    }
    if (!IsIdentifier(aName))
    {
        rReason = u"only a plain identifier can be renamed, not an operator or a destructor"_ustr;
        return false;
    }
    if (Spelling(clang_getCursorUSR(aTarget)).empty())
    {
        rReason = u"the entity has no identity to find its occurrences by"_ustr;
        return false;
    }
    // what a system header declares is not ours to rename; judge by the
    // definition where one was seen, the declaration otherwise
    CXCursor aWhere = clang_getCursorDefinition(aTarget);
    if (clang_Cursor_isNull(aWhere) || clang_isInvalid(clang_getCursorKind(aWhere)))
        aWhere = aTarget;
    if (clang_Location_isInSystemHeader(clang_getCursorLocation(aWhere)))
    {
        rReason = u"the name is declared in a system header"_ustr;
        return false;
    }
    rTarget = aTarget;
    return true;
}

// the scopes a search for a declaration recurses into; function bodies hold
// only local declarations, which no other file can refer to
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

// Finds a declaration with the wanted USR anywhere in a translation unit --
// a file that refers to an entity sees its declaration, usually from an
// included header, so the walk does not stop at the main file.
struct UsrSearch
{
    const std::string& rUsr;
    CXCursor aFound;
    bool bFound;
};

CXChildVisitResult VisitUsr(CXCursor aCursor, CXCursor, CXClientData pData)
{
    UsrSearch& rSearch = *static_cast<UsrSearch*>(pData);
    const CXCursorKind eKind = clang_getCursorKind(aCursor);
    if (clang_isDeclaration(eKind) || eKind == CXCursor_MacroDefinition)
    {
        if (Spelling(clang_getCursorUSR(aCursor)) == rSearch.rUsr)
        {
            rSearch.aFound = aCursor;
            rSearch.bFound = true;
            return CXChildVisit_Break;
        }
    }
    return IsScope(eKind) || eKind == CXCursor_FunctionTemplate ? CXChildVisit_Recurse
                                                                : CXChildVisit_Continue;
}

// collects what clang_findReferencesInFile reports, keeping only ranges that
// really spell the name in the searched file
struct Occurrences
{
    CXFile xFile;
    const std::string& rText;
    const Lines& rLines;
    const std::string& rName;
    std::vector<core::Range> aRanges;
};

CXVisitorResult VisitOccurrence(void* pData, CXCursor, CXSourceRange aRange)
{
    Occurrences& rOccurrences = *static_cast<Occurrences*>(pData);
    core::Range aEdit;
    if (EditRangeOf(aRange, rOccurrences.xFile, rOccurrences.rText, rOccurrences.rLines,
                    rOccurrences.rName, aEdit))
        rOccurrences.aRanges.push_back(aEdit);
    return CXVisit_Continue;
}

// document order; an edit never spans lines (it is one identifier), so the
// start decides
bool EditBefore(const core::Range& rA, const core::Range& rB)
{
    if (rA.Start.Line != rB.Start.Line)
        return rA.Start.Line < rB.Start.Line;
    return rA.Start.Column < rB.Start.Column;
}

bool EditSame(const core::Range& rA, const core::Range& rB)
{
    return rA.Start.Line == rB.Start.Line && rA.Start.Column == rB.Start.Column;
}

class ClangRenameProvider final
    : public cppu::WeakImplHelper<XRefactoringProvider, XRenameProvider>
{
public:
    explicit ClangRenameProvider(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    // XRefactoringProvider
    OUString SAL_CALL getName() override { return u"clang"_ustr; }
    uno::Sequence<OUString> SAL_CALL getLanguages() override
    {
        return { u"c"_ustr, u"c++"_ustr };
    }
    sal_Bool SAL_CALL supports(const OUString& rUrl) override;

    // XRenameProvider
    RenameTarget SAL_CALL prepareRename(const OUString& rUrl, const core::Position& rPosition,
                                        const OUString& rText) override;
    WorkspaceEdit SAL_CALL rename(const OUString& rUrl, const core::Position& rPosition,
                                  const OUString& rNewName, const OUString& rText) override;

private:
    // the parsed document and the resolved target, shared by both halves
    struct Request
    {
        std::string aFile;
        std::string aText;
        std::unique_ptr<Unit> pUnit;
        CXCursor aCursor = clang_getNullCursor(); // at the position
        CXCursor aTarget = clang_getNullCursor(); // the declaration it names
        std::string aName;
        std::string aUsr;
        OUString aReason; // why not, when this returns false
    };
    bool Resolve(const OUString& rUrl, const core::Position& rPosition, const OUString& rText,
                 Request& rRequest);

    uno::Sequence<OUString> CompileArgumentsOf(const OUString& rUrl);

    uno::Reference<uno::XComponentContext> m_xContext;
};

sal_Bool ClangRenameProvider::supports(const OUString& rUrl)
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

uno::Sequence<OUString> ClangRenameProvider::CompileArgumentsOf(const OUString& rUrl)
{
    try
    {
        return index::theCompileCommandProvider::get(m_xContext)->getCompileArguments(rUrl);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code.clang",
                 "no compile arguments for " << rUrl << ": " << rException.Message);
        return {};
    }
}

bool ClangRenameProvider::Resolve(const OUString& rUrl, const core::Position& rPosition,
                                  const OUString& rText, Request& rRequest)
{
    rRequest.aFile = ToPath(rUrl);
    if (rText.isEmpty())
        rRequest.aText = ReadFile(rRequest.aFile);
    else
    {
        const OString aUtf8 = OUStringToOString(rText, RTL_TEXTENCODING_UTF8);
        rRequest.aText.assign(aUtf8.getStr(), aUtf8.getLength());
    }

    const std::vector<CXUnsavedFile> aUnsaved{ { rRequest.aFile.c_str(), rRequest.aText.data(),
                                                 static_cast<unsigned long>(
                                                     rRequest.aText.size()) } };
    rRequest.pUnit
        = std::make_unique<Unit>(rRequest.aFile, CompileArgumentsOf(rUrl), aUnsaved);
    if (!*rRequest.pUnit)
    {
        rRequest.aReason = u"the file cannot be parsed"_ustr;
        return false;
    }
    CXFile xFile = clang_getFile(rRequest.pUnit->Get(), rRequest.aFile.c_str());
    if (!xFile)
    {
        rRequest.aReason = u"the file cannot be parsed"_ustr;
        return false;
    }
    const Lines aLines(rRequest.aText);
    const unsigned nLine = static_cast<unsigned>(rPosition.Line) + 1;
    const CXSourceLocation aLocation = clang_getLocation(
        rRequest.pUnit->Get(), xFile, nLine, aLines.ByteColumn(nLine, rPosition.Column));
    rRequest.aCursor = clang_getCursor(rRequest.pUnit->Get(), aLocation);

    if (!ResolveTarget(rRequest.aCursor, rRequest.aTarget, rRequest.aReason))
        return false;
    rRequest.aName = Spelling(clang_getCursorSpelling(rRequest.aTarget));
    rRequest.aUsr = Spelling(clang_getCursorUSR(rRequest.aTarget));
    return true;
}

RenameTarget ClangRenameProvider::prepareRename(const OUString& rUrl,
                                                const core::Position& rPosition,
                                                const OUString& rText)
{
    RenameTarget aResult;
    aResult.Valid = false;

    Request aRequest;
    if (!Resolve(rUrl, rPosition, rText, aRequest))
    {
        aResult.Reason = aRequest.aReason;
        return aResult;
    }

    // the identifier under the caret, for the editor to select; if even that
    // token cannot be isolated, a rename could not edit it either
    const Lines aLines(aRequest.aText);
    CXFile xFile = clang_getFile(aRequest.pUnit->Get(), aRequest.aFile.c_str());
    core::Range aRange;
    if (!EditRangeOf(NameRangeOf(aRequest.aCursor), xFile, aRequest.aText, aLines, aRequest.aName,
                     aRange))
    {
        aResult.Reason = u"the name cannot be isolated at this position"_ustr;
        return aResult;
    }

    aResult.Valid = true;
    aResult.Name = OUString::fromUtf8(aRequest.aName);
    aResult.Placeholder = aResult.Name;
    aResult.Range = aRange;
    return aResult;
}

WorkspaceEdit ClangRenameProvider::rename(const OUString& rUrl, const core::Position& rPosition,
                                          const OUString& rNewName, const OUString& rText)
{
    Request aRequest;
    if (!Resolve(rUrl, rPosition, rText, aRequest))
        throw RefactorException(aRequest.aReason, getXWeak(), rUrl);

    const OString aNewUtf8 = OUStringToOString(rNewName, RTL_TEXTENCODING_UTF8);
    const std::string aNewName(aNewUtf8.getStr(), aNewUtf8.getLength());
    if (!IsIdentifier(aNewName))
        throw RefactorException(u"the new name is not an identifier"_ustr, getXWeak(), rNewName);
    if (aNewName == aRequest.aName)
        throw RefactorException(u"the new name is the current name"_ustr, getXWeak(), rNewName);

    // every file's occurrences, the renamed document first
    std::vector<std::pair<OUString, std::vector<core::Range>>> aFiles;
    {
        const Lines aLines(aRequest.aText);
        CXFile xFile = clang_getFile(aRequest.pUnit->Get(), aRequest.aFile.c_str());
        Occurrences aOccurrences{ xFile, aRequest.aText, aLines, aRequest.aName, {} };
        CXCursorAndRangeVisitor aVisitor{ &aOccurrences, &VisitOccurrence };
        clang_findReferencesInFile(aRequest.aTarget, xFile, aVisitor);
        if (!aOccurrences.aRanges.empty())
            aFiles.emplace_back(rUrl, std::move(aOccurrences.aRanges));
    }

    // Candidate other files: whatever the symbol index has seen of the
    // project.  The index stores declarations, not references, so it cannot
    // say where the entity is used -- but a file whose text does not contain
    // the name cannot use it, which cuts the field down before the expensive
    // parses.
    std::vector<OUString> aCandidates;
    try
    {
        uno::Reference<index::XSymbolStore> xStore
            = index::theSymbolIndex::get(m_xContext)->getStore();
        if (xStore.is())
            for (const OUString& rCandidate : xStore->getDocumentUrls())
                if (rCandidate != rUrl && supports(rCandidate))
                    aCandidates.push_back(rCandidate);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code.clang",
                 "rename searches only " << rUrl << ", no index: " << rException.Message);
    }
    std::sort(aCandidates.begin(), aCandidates.end());

    for (const OUString& rCandidate : aCandidates)
    {
        const std::string aPath = ToPath(rCandidate);
        const std::string aText = ReadFile(aPath);
        if (aText.empty() || aText.find(aRequest.aName) == std::string::npos)
            continue;

        // the candidate's own text pinned against disk changes, and the
        // renamed document as the editor holds it, in case it is included
        const std::vector<CXUnsavedFile> aUnsaved{
            { aPath.c_str(), aText.data(), static_cast<unsigned long>(aText.size()) },
            { aRequest.aFile.c_str(), aRequest.aText.data(),
              static_cast<unsigned long>(aRequest.aText.size()) }
        };
        Unit aUnit(aPath, CompileArgumentsOf(rCandidate), aUnsaved);
        if (!aUnit)
        {
            SAL_WARN("code.clang", "rename skips " << rCandidate << ": it does not parse");
            continue;
        }
        UsrSearch aSearch{ aRequest.aUsr, clang_getNullCursor(), false };
        clang_visitChildren(clang_getTranslationUnitCursor(aUnit.Get()), VisitUsr, &aSearch);
        if (!aSearch.bFound)
            continue; // no declaration in sight, so no reference possible
        CXFile xFile = clang_getFile(aUnit.Get(), aPath.c_str());
        if (!xFile)
            continue;
        const Lines aLines(aText);
        Occurrences aOccurrences{ xFile, aText, aLines, aRequest.aName, {} };
        CXCursorAndRangeVisitor aVisitor{ &aOccurrences, &VisitOccurrence };
        clang_findReferencesInFile(aSearch.aFound, xFile, aVisitor);
        if (!aOccurrences.aRanges.empty())
            aFiles.emplace_back(rCandidate, std::move(aOccurrences.aRanges));
    }

    // one edit per occurrence: sorted, deduplicated (a macro expansion may
    // report one spelling many times), never overlapping
    const OUString aOldName = OUString::fromUtf8(aRequest.aName);
    sal_Int32 nOccurrences = 0;
    std::vector<FileEdit> aFileEdits;
    for (auto& rFile : aFiles)
    {
        std::sort(rFile.second.begin(), rFile.second.end(), EditBefore);
        rFile.second.erase(std::unique(rFile.second.begin(), rFile.second.end(), EditSame),
                           rFile.second.end());
        std::vector<TextEdit> aEdits;
        aEdits.reserve(rFile.second.size());
        for (const core::Range& rRange : rFile.second)
        {
            TextEdit aEdit;
            aEdit.Range = rRange;
            aEdit.NewText = rNewName;
            aEdits.push_back(std::move(aEdit));
        }
        nOccurrences += static_cast<sal_Int32>(aEdits.size());
        FileEdit aFileEdit;
        aFileEdit.Url = rFile.first;
        aFileEdit.Edits = comphelper::containerToSequence(aEdits);
        aFileEdits.push_back(std::move(aFileEdit));
    }
    if (aFileEdits.empty())
        throw RefactorException(u"no occurrence of the name was found"_ustr, getXWeak(), aOldName);

    const sal_Int32 nFiles = static_cast<sal_Int32>(aFileEdits.size());
    const OUString aOccurrenceWord
        = nOccurrences == 1 ? u" occurrence in "_ustr : u" occurrences in "_ustr;
    const OUString aFileWord = nFiles == 1 ? u" file)"_ustr : u" files)"_ustr;
    WorkspaceEdit aResult;
    aResult.Files = comphelper::containerToSequence(aFileEdits);
    aResult.Description = "Rename '" + aOldName + "' to '" + rNewName + "' ("
                          + OUString::number(nOccurrences) + aOccurrenceWord
                          + OUString::number(nFiles) + aFileWord;
    return aResult;
}

class ClangRenamePlugin final : public cppu::WeakImplHelper<core::XPlugin, lang::XServiceInfo>
{
public:
    explicit ClangRenamePlugin(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    void SAL_CALL activate() override
    {
        theRefactoringController::get(m_xContext)->registerProvider(
            new ClangRenameProvider(m_xContext));
    }

    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.ClangRenamePlugin"_ustr;
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
com_sun_star_comp_code_ClangRenamePlugin_get_implementation(uno::XComponentContext* pContext,
                                                             uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::ClangRenamePlugin(pContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
