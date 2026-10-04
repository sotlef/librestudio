/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// theFinder: the search in files.  A search is an enumeration of its results
// that is lazy all the way down: the files to look into are produced one at a
// time from a walk of the project (or the directory), a file is read when the
// enumeration needs its next result, and what is not asked for is not read.

#include "common.hxx"

#include <com/sun/star/code/document/XDocument.hpp>
#include <com/sun/star/code/document/XDocumentManager.hpp>
#include <com/sun/star/code/document/XTextDocument.hpp>
#include <com/sun/star/code/document/theDocumentManager.hpp>
#include <com/sun/star/code/project/ItemKind.hpp>
#include <com/sun/star/code/project/XProject.hpp>
#include <com/sun/star/code/project/XProjectFiles.hpp>
#include <com/sun/star/code/project/XProjectItem.hpp>
#include <com/sun/star/code/project/XProjectManager.hpp>
#include <com/sun/star/code/project/theProjectManager.hpp>
#include <com/sun/star/code/search/FindOptions.hpp>
#include <com/sun/star/code/search/FindResult.hpp>
#include <com/sun/star/code/search/XFindEnumeration.hpp>
#include <com/sun/star/code/search/XFindListener.hpp>
#include <com/sun/star/code/search/XFinder.hpp>
#include <com/sun/star/container/NoSuchElementException.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/interfacecontainer4.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <osl/file.hxx>
#include <rtl/character.hxx>
#include <rtl/string.hxx>
#include <rtl/strbuf.hxx>
#include <rtl/textcvt.h>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>

#include <unicode/regex.h>
#include <unicode/unistr.h>

#include <algorithm>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

using namespace css;
using namespace css::code::search;

namespace codeservices
{
namespace
{
// A file larger than this is not text anyone searches by hand.
constexpr sal_uInt64 MaxFileSize = 16 * 1024 * 1024;

/// "*.cpp;*.h" (also "," and blanks as separators) -> the masks; none = every file
std::vector<OUString> SplitMasks(const OUString& rFileTypes)
{
    std::vector<OUString> aMasks;
    OUStringBuffer aCurrent;
    auto flush = [&]
    {
        const OUString aMask = aCurrent.makeStringAndClear().trim();
        if (!aMask.isEmpty() && aMask != "*.*" && aMask != "*")
            aMasks.push_back(aMask.toAsciiLowerCase());
    };
    for (sal_Int32 i = 0; i < rFileTypes.getLength(); ++i)
    {
        const sal_Unicode c = rFileTypes[i];
        if (c == ';' || c == ',' || c == ' ')
            flush();
        else
            aCurrent.append(c);
    }
    flush();
    return aMasks;
}

bool WildMatch(std::u16string_view aMask, std::u16string_view aName)
{
    if (aMask.empty())
        return aName.empty();
    if (aMask[0] == '*')
    {
        for (size_t i = 0; i <= aName.size(); ++i)
            if (WildMatch(aMask.substr(1), aName.substr(i)))
                return true;
        return false;
    }
    if (aName.empty())
        return false;
    if (aMask[0] == '?' || aMask[0] == aName[0])
        return WildMatch(aMask.substr(1), aName.substr(1));
    return false;
}

OUString NameOf(const OUString& rUrl)
{
    return rUrl.copy(rUrl.lastIndexOf('/') + 1);
}

icu::UnicodeString PatternOf(const FindOptions& rOptions)
{
    const OUString aText = FindPattern(rOptions);
    return icu::UnicodeString(reinterpret_cast<const char16_t*>(aText.getStr()), aText.getLength());
}

/// The files to look into, one after the other: a function that gives the
/// next URL, false when there is none.
using FileSource = std::function<bool(OUString&)>;

/// the files of a project tree, a folder opened only when the walk reaches it
FileSource WalkItems(uno::Reference<code::project::XProjectItem> xRoot, bool bExternal)
{
    auto xStack = std::make_shared<std::vector<uno::Reference<code::project::XProjectItem>>>();
    if (xRoot.is())
        xStack->push_back(xRoot);
    return [xStack, bExternal](OUString& rUrl)
    {
        while (!xStack->empty())
        {
            uno::Reference<code::project::XProjectItem> xItem = xStack->back();
            xStack->pop_back();
            const auto eKind = xItem->getKind();
            if (eKind == code::project::ItemKind_EXTERNAL && !bExternal)
                continue;
            const auto aChildren = xItem->getChildren();
            // the first child is to come out first
            for (sal_Int32 i = aChildren.getLength() - 1; i >= 0; --i)
                xStack->push_back(aChildren[i]);
            if ((eKind == code::project::ItemKind_FILE || eKind == code::project::ItemKind_EXTERNAL)
                && !xItem->getUrl().isEmpty() && !aChildren.hasElements())
            {
                rUrl = xItem->getUrl();
                return true;
            }
        }
        return false;
    };
}

/// the files of a directory on disk, directories entered when the walk reaches them
FileSource WalkDirectory(const OUString& rDirectory, bool bSubfolders)
{
    auto xStack = std::make_shared<std::vector<std::unique_ptr<osl::Directory>>>();
    auto xFirst = std::make_unique<osl::Directory>(rDirectory);
    if (xFirst->open() == osl::FileBase::E_None)
        xStack->push_back(std::move(xFirst));
    return [xStack, bSubfolders](OUString& rUrl)
    {
        while (!xStack->empty())
        {
            osl::DirectoryItem aItem;
            if (xStack->back()->getNextItem(aItem) != osl::FileBase::E_None)
            {
                xStack->pop_back();
                continue;
            }
            osl::FileStatus aStatus(osl_FileStatus_Mask_Type | osl_FileStatus_Mask_FileURL);
            if (aItem.getFileStatus(aStatus) != osl::FileBase::E_None)
                continue;
            if (aStatus.getFileType() == osl::FileStatus::Directory)
            {
                // the metadata of a repository is not what anyone looks for
                if (!bSubfolders || NameOf(aStatus.getFileURL()) == ".git")
                    continue;
                auto xDirectory = std::make_unique<osl::Directory>(aStatus.getFileURL());
                if (xDirectory->open() == osl::FileBase::E_None)
                    xStack->push_back(std::move(xDirectory));
            }
            else if (aStatus.getFileType() == osl::FileStatus::Regular)
            {
                rUrl = aStatus.getFileURL();
                return true;
            }
        }
        return false;
    };
}

FileSource FromList(std::vector<OUString> aUrls)
{
    auto xUrls = std::make_shared<std::deque<OUString>>(aUrls.begin(), aUrls.end());
    return [xUrls](OUString& rUrl)
    {
        if (xUrls->empty())
            return false;
        rUrl = xUrls->front();
        xUrls->pop_front();
        return true;
    };
}

/// The sources one after the other; a source is made when its turn comes,
/// so that nothing is listed before it is needed.
FileSource Chain(std::vector<std::function<FileSource()>> aMakers)
{
    struct State
    {
        std::vector<std::function<FileSource()>> aMakers;
        size_t nNext = 0;
        FileSource aCurrent;
    };
    auto xState = std::make_shared<State>();
    xState->aMakers = std::move(aMakers);
    return [xState](OUString& rUrl)
    {
        for (;;)
        {
            if (xState->aCurrent && xState->aCurrent(rUrl))
                return true;
            if (xState->nNext >= xState->aMakers.size())
                return false;
            xState->aCurrent = xState->aMakers[xState->nNext++]();
        }
    };
}

class FindEnumeration final : public cppu::WeakImplHelper<XFindEnumeration>
{
public:
    FindEnumeration(uno::Reference<uno::XComponentContext> xContext, const FindOptions& rOptions)
        : m_xContext(std::move(xContext))
        , m_aMasks(SplitMasks(rOptions.FileTypes))
    {
        UErrorCode eError = U_ZERO_ERROR;
        const uint32_t nFlags = rOptions.MatchCase ? 0 : UREGEX_CASE_INSENSITIVE;
        m_xMatcher.reset(new icu::RegexMatcher(PatternOf(rOptions), nFlags, eError));
        if (U_FAILURE(eError) || rOptions.FindWhat.isEmpty())
            throw lang::IllegalArgumentException(
                rOptions.FindWhat.isEmpty() ? u"nothing to search for"_ustr
                                            : u"the regular expression is not valid"_ustr,
                nullptr, 0);
        m_aSource = MakeSource(rOptions);
    }

    sal_Bool SAL_CALL hasMoreResults() override
    {
        std::unique_lock aGuard(m_aMutex);
        return fill();
    }

    FindResult SAL_CALL nextResult() override
    {
        std::unique_lock aGuard(m_aMutex);
        if (!fill())
            throw container::NoSuchElementException();
        FindResult aResult = std::move(m_aPending.front());
        m_aPending.pop_front();
        return aResult;
    }

    sal_Int32 SAL_CALL getFilesSearched() override
    {
        std::unique_lock aGuard(m_aMutex);
        return m_nFilesSearched;
    }

    void SAL_CALL cancel() override
    {
        std::unique_lock aGuard(m_aMutex);
        m_bCancelled = true;
        m_aPending.clear();
        m_aSource = nullptr;
    }

private:
    uno::Reference<code::document::XDocumentManager> documents() const
    {
        return code::document::theDocumentManager::get(m_xContext);
    }

    FileSource MakeSource(const FindOptions& rOptions)
    {
        std::vector<std::function<FileSource()>> aMakers;
        const bool bExternal = rOptions.IncludeExternal;
        auto projectsOf = [this](bool bActiveOnly)
        {
            std::vector<uno::Reference<code::project::XProject>> aProjects;
            try
            {
                auto xManager = code::project::theProjectManager::get(m_xContext);
                if (bActiveOnly)
                {
                    if (auto xProject = xManager->getActiveProject())
                        aProjects.push_back(xProject);
                }
                else
                    for (const auto& xProject : xManager->getOpenProjects())
                        aProjects.push_back(xProject);
            }
            catch (const uno::Exception&)
            {
            }
            return aProjects;
        };
        auto openDocuments = [this]
        {
            std::vector<OUString> aUrls;
            try
            {
                for (const auto& xDocument : documents()->getOpenDocuments())
                    aUrls.push_back(xDocument->getUrl());
            }
            catch (const uno::Exception&)
            {
            }
            return aUrls;
        };
        auto projectFiles = [projectsOf, bExternal](bool bActiveOnly)
        {
            return [projectsOf, bExternal, bActiveOnly]() -> FileSource
            {
                std::vector<FileSource> aSources;
                for (const auto& xProject : projectsOf(bActiveOnly))
                {
                    uno::Reference<code::project::XProjectFiles> xFiles(xProject, uno::UNO_QUERY);
                    if (xFiles.is())
                        aSources.push_back(WalkItems(xFiles->getRootItem(), bExternal));
                }
                auto xIndex = std::make_shared<size_t>(0);
                auto xSources = std::make_shared<std::vector<FileSource>>(std::move(aSources));
                return [xIndex, xSources](OUString& rUrl)
                {
                    while (*xIndex < xSources->size())
                    {
                        if ((*xSources)[*xIndex](rUrl))
                            return true;
                        ++*xIndex;
                    }
                    return false;
                };
            };
        };

        switch (rOptions.Scope)
        {
            case FindScope_ENTIRE_SOLUTION:
                aMakers.push_back(projectFiles(false));
                if (rOptions.IncludeMisc)
                    aMakers.push_back([openDocuments] { return FromList(openDocuments()); });
                break;
            case FindScope_CURRENT_PROJECT:
                aMakers.push_back(projectFiles(true));
                break;
            case FindScope_DIRECTORY:
                aMakers.push_back([rOptions] {
                    return WalkDirectory(rOptions.Directory, rOptions.IncludeSubfolders);
                });
                break;
            case FindScope_OPEN_DOCUMENTS:
                aMakers.push_back([openDocuments] { return FromList(openDocuments()); });
                break;
            case FindScope_CURRENT_DOCUMENT:
                aMakers.push_back([rOptions] { return FromList({ rOptions.DocumentUrl }); });
                break;
        }
        return Chain(std::move(aMakers));
    }

    bool matchesMasks(const OUString& rUrl) const
    {
        if (m_aMasks.empty())
            return true;
        const OUString aName = NameOf(rUrl).toAsciiLowerCase();
        return std::any_of(m_aMasks.begin(), m_aMasks.end(),
                           [&](const OUString& rMask) { return WildMatch(rMask, aName); });
    }

    /// the text of a file: what the editor holds if it is open, else what is on disk;
    /// false for what is not text
    bool readText(const OUString& rUrl, OUString& rText)
    {
        try
        {
            uno::Reference<code::document::XTextDocument> xDocument(documents()->getDocument(rUrl),
                                                                    uno::UNO_QUERY);
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

    void searchFile(const OUString& rUrl)
    {
        OUString aText;
        if (!readText(rUrl, aText))
            return;
        ++m_nFilesSearched;

        sal_Int32 nLine = 0;
        sal_Int32 nStart = 0;
        while (nStart <= aText.getLength())
        {
            sal_Int32 nEnd = aText.indexOf('\n', nStart);
            const bool bLast = nEnd < 0;
            if (bLast)
                nEnd = aText.getLength();
            sal_Int32 nLineEnd = nEnd;
            if (nLineEnd > nStart && aText[nLineEnd - 1] == '\r')
                --nLineEnd;
            // a last empty "line" after the final terminator is not a line
            if (!(bLast && nStart == aText.getLength() && nStart > 0))
            {
                const OUString aLine = aText.copy(nStart, nLineEnd - nStart);
                const icu::UnicodeString aUnicode(reinterpret_cast<const char16_t*>(aLine.getStr()),
                                                  aLine.getLength());
                UErrorCode eError = U_ZERO_ERROR;
                m_xMatcher->reset(aUnicode);
                if (m_xMatcher->find(eError) && U_SUCCESS(eError))
                {
                    FindResult aResult;
                    aResult.Url = rUrl;
                    aResult.LineText = aLine;
                    aResult.Match.Start.Line = aResult.Match.End.Line = nLine;
                    aResult.Match.Start.Column = m_xMatcher->start(eError);
                    aResult.Match.End.Column = m_xMatcher->end(eError);
                    m_aPending.push_back(std::move(aResult));
                }
            }
            if (bLast)
                break;
            nStart = nEnd + 1;
            ++nLine;
        }
    }

    /// reads files until a result is there; false when the search is over
    bool fill()
    {
        while (m_aPending.empty())
        {
            if (m_bCancelled || !m_aSource)
                return false;
            OUString aUrl;
            if (!m_aSource(aUrl))
            {
                m_aSource = nullptr;
                return false;
            }
            // the same file reached by two ways (a project and the open documents) is read once
            if (!m_aVisited.insert(aUrl).second || !matchesMasks(aUrl))
                continue;
            searchFile(aUrl);
        }
        return true;
    }

    std::mutex m_aMutex;
    uno::Reference<uno::XComponentContext> m_xContext;
    const std::vector<OUString> m_aMasks;
    std::unique_ptr<icu::RegexMatcher> m_xMatcher;
    FileSource m_aSource;
    std::set<OUString> m_aVisited;
    std::deque<FindResult> m_aPending;
    sal_Int32 m_nFilesSearched = 0;
    bool m_bCancelled = false;
};

/// results that are known: what Replace All changed
class ListEnumeration final : public cppu::WeakImplHelper<XFindEnumeration>
{
public:
    ListEnumeration(const uno::Sequence<FindResult>& rResults, sal_Int32 nFilesSearched)
        : m_aResults(rResults)
        , m_nFilesSearched(nFilesSearched)
    {
    }

    sal_Bool SAL_CALL hasMoreResults() override
    {
        std::unique_lock aGuard(m_aMutex);
        return m_nNext < m_aResults.getLength();
    }
    FindResult SAL_CALL nextResult() override
    {
        std::unique_lock aGuard(m_aMutex);
        if (m_nNext >= m_aResults.getLength())
            throw container::NoSuchElementException();
        return m_aResults[m_nNext++];
    }
    sal_Int32 SAL_CALL getFilesSearched() override { return m_nFilesSearched; }
    void SAL_CALL cancel() override
    {
        std::unique_lock aGuard(m_aMutex);
        m_nNext = m_aResults.getLength();
    }

private:
    std::mutex m_aMutex;
    const uno::Sequence<FindResult> m_aResults;
    const sal_Int32 m_nFilesSearched;
    sal_Int32 m_nNext = 0;
};

class Finder final : public comphelper::WeakComponentImplHelper<XFinder, lang::XServiceInfo>
{
public:
    explicit Finder(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    // XFinder
    uno::Reference<XFindEnumeration> SAL_CALL find(const FindOptions& rOptions) override
    {
        return new FindEnumeration(m_xContext, rOptions);
    }

    uno::Reference<XFindEnumeration> SAL_CALL startSearch(const FindOptions& rOptions) override
    {
        return show(rOptions, new FindEnumeration(m_xContext, rOptions));
    }

    uno::Reference<XFindEnumeration> show(const FindOptions& rOptions,
                                          uno::Reference<XFindEnumeration> xSearch)
    {
        uno::Reference<XFindEnumeration> xPrevious;
        {
            std::unique_lock aGuard(m_aMutex);
            throwIfDisposed(aGuard);
            xPrevious = std::move(m_xShown);
            m_xShown = xSearch;
        }
        if (xPrevious.is())
            xPrevious->cancel();

        std::unique_lock aGuard(m_aMutex);
        m_aListeners.forEach(aGuard, [&](const uno::Reference<XFindListener>& xListener)
                             { xListener->searchStarted(rOptions, xSearch); });
        return xSearch;
    }

    uno::Reference<XFindEnumeration> SAL_CALL showResults(
        const FindOptions& rOptions, const uno::Sequence<FindResult>& rResults,
        sal_Int32 nFilesSearched) override
    {
        return show(rOptions, new ListEnumeration(rResults, nFilesSearched));
    }

    uno::Reference<XFindEnumeration> SAL_CALL getShownSearch() override
    {
        std::unique_lock aGuard(m_aMutex);
        return m_xShown;
    }

    void SAL_CALL addFindListener(const uno::Reference<XFindListener>& xListener) override
    {
        std::unique_lock aGuard(m_aMutex);
        m_aListeners.addInterface(aGuard, xListener);
    }

    void SAL_CALL removeFindListener(const uno::Reference<XFindListener>& xListener) override
    {
        std::unique_lock aGuard(m_aMutex);
        m_aListeners.removeInterface(aGuard, xListener);
    }

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.Finder"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

private:
    void disposing(std::unique_lock<std::mutex>& rGuard) override
    {
        m_xShown.clear();
        comphelper::WeakComponentImplHelperBase::disposing(rGuard);
    }

    uno::Reference<uno::XComponentContext> m_xContext;
    uno::Reference<XFindEnumeration> m_xShown;
    comphelper::OInterfaceContainerHelper4<XFindListener> m_aListeners;
};
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_Finder_get_implementation(uno::XComponentContext* pContext,
                                                  uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::Finder(pContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
