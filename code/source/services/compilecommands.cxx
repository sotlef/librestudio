/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// theCompileCommandProvider: how a source file is compiled, from the
// compile_commands.json that CMake (CMAKE_EXPORT_COMPILE_COMMANDS) and other
// build tools write.  A file the database does not name -- a header -- gets the
// arguments of a source file of the same directory, as clangd does.

#include <com/sun/star/code/index/XCompileCommandProvider.hpp>
#include <com/sun/star/code/project/XProject.hpp>
#include <com/sun/star/code/project/XProjectManager.hpp>
#include <com/sun/star/code/project/theProjectManager.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <osl/file.hxx>
#include <sal/log.hxx>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace css;
using namespace css::code;
using namespace css::code::index;

namespace pt = boost::property_tree;

namespace codeservices
{
namespace
{
using Arguments = std::vector<OUString>;

// One compile_commands.json, read: the arguments per source file.
struct Database
{
    sal_uInt64 nModified = 0;
    // by file URL
    std::map<OUString, Arguments> aByFile;
    // by directory URL: some source file in it, to lend its arguments to a header
    std::map<OUString, OUString> aSiblings;
};

OUString ToUrl(const OUString& rSystemPath)
{
    OUString aUrl;
    return osl::FileBase::getFileURLFromSystemPath(rSystemPath, aUrl) == osl::FileBase::E_None
               ? aUrl
               : OUString();
}

OUString ToSystemPath(const OUString& rUrl)
{
    OUString aPath;
    return osl::FileBase::getSystemPathFromFileURL(rUrl, aPath) == osl::FileBase::E_None
               ? aPath
               : OUString();
}

OUString DirectoryOf(const OUString& rUrl)
{
    const sal_Int32 nSlash = rUrl.lastIndexOf('/');
    return nSlash > 0 ? rUrl.copy(0, nSlash) : rUrl;
}

// "gcc -c a.c" as one string: split at blanks, honouring quotes and backslashes
std::vector<std::string> SplitCommand(const std::string& rCommand)
{
    std::vector<std::string> aWords;
    std::string aWord;
    bool bInWord = false;
    char cQuote = 0;
    for (size_t i = 0; i < rCommand.size(); ++i)
    {
        const char c = rCommand[i];
        if (cQuote)
        {
            if (c == cQuote)
                cQuote = 0;
            else if (c == '\\' && cQuote == '"' && i + 1 < rCommand.size())
                aWord += rCommand[++i];
            else
                aWord += c;
        }
        else if (c == '"' || c == '\'')
        {
            cQuote = c;
            bInWord = true;
        }
        else if (c == '\\' && i + 1 < rCommand.size())
        {
            aWord += rCommand[++i];
            bInWord = true;
        }
        else if (c == ' ' || c == '\t' || c == '\n')
        {
            if (bInWord)
                aWords.push_back(std::move(aWord));
            aWord.clear();
            bInWord = false;
        }
        else
        {
            aWord += c;
            bInWord = true;
        }
    }
    if (bInWord)
        aWords.push_back(std::move(aWord));
    return aWords;
}

OUString FromUtf8(const std::string& rText)
{
    return OUString::fromUtf8(std::string_view(rText));
}

// What the analyser needs of a compile command: the options, without the
// compiler, the file, and what only matters to producing an object file.
Arguments UsefulArguments(const std::vector<std::string>& rWords, const OUString& rDirectory)
{
    Arguments aResult;
    for (size_t i = 1; i < rWords.size(); ++i) // [0] is the compiler
    {
        const std::string& rWord = rWords[i];
        if (rWord == "-c" || rWord == "-S" || rWord == "-E")
            continue;
        if (rWord == "-o" || rWord == "-MF" || rWord == "-MT" || rWord == "-MQ")
        {
            ++i;
            continue;
        }
        if (rWord == "-MD" || rWord == "-MMD" || rWord == "-MP" || rWord.rfind("-o", 0) == 0)
            continue;
        if (rWord.empty() || rWord[0] != '-')
            continue; // the source file (or an object): not an option
        aResult.push_back(FromUtf8(rWord));
    }
    // relative include paths are relative to the directory of the command
    aResult.push_back("-working-directory=" + ToSystemPath(rDirectory));
    return aResult;
}

class CompileCommandProvider final
    : public comphelper::WeakComponentImplHelper<XCompileCommandProvider, lang::XServiceInfo>
{
public:
    explicit CompileCommandProvider(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    // XCompileCommandProvider
    uno::Sequence<OUString> SAL_CALL getCompileArguments(const OUString& rUrl) override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.CompileCommandProvider"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

private:
    std::vector<OUString> CandidateFiles(const OUString& rUrl);
    const Database* Load(const OUString& rJsonUrl); // lock held
    static sal_uInt64 ModifiedOf(const OUString& rUrl);

    uno::Reference<uno::XComponentContext> m_xContext;
    std::map<OUString, Database> m_aDatabases;
};

sal_uInt64 CompileCommandProvider::ModifiedOf(const OUString& rUrl)
{
    osl::DirectoryItem aItem;
    osl::FileStatus aStatus(osl_FileStatus_Mask_ModifyTime);
    if (osl::DirectoryItem::get(rUrl, aItem) != osl::FileBase::E_None
        || aItem.getFileStatus(aStatus) != osl::FileBase::E_None)
        return 0;
    const TimeValue aTime = aStatus.getModifyTime();
    return static_cast<sal_uInt64>(aTime.Seconds) * 1000000000 + aTime.Nanosec;
}

// where a compile_commands.json may be: next to the file or above it, or in a
// build directory there, and the same for the open projects
std::vector<OUString> CompileCommandProvider::CandidateFiles(const OUString& rUrl)
{
    std::vector<OUString> aRoots;
    for (OUString aDir = DirectoryOf(rUrl); aDir.getLength() > 8 /* file:/// */;
         aDir = DirectoryOf(aDir))
    {
        aRoots.push_back(aDir);
        if (aDir == DirectoryOf(aDir))
            break;
    }
    try
    {
        for (const auto& xProject : project::theProjectManager::get(m_xContext)->getOpenProjects())
        {
            const OUString aProjectUrl = xProject->getUrl();
            aRoots.push_back(aProjectUrl.endsWith(u".txt") || aProjectUrl.indexOf('.', aProjectUrl.lastIndexOf('/')) >= 0
                                 ? DirectoryOf(aProjectUrl)
                                 : aProjectUrl);
        }
    }
    catch (const uno::Exception&)
    {
    }
    std::vector<OUString> aFiles;
    std::set<OUString> aSeen;
    for (const OUString& rRoot : aRoots)
        for (const OUString& rName : { OUString(rRoot + "/compile_commands.json"),
                                       OUString(rRoot + "/build/compile_commands.json") })
            if (aSeen.insert(rName).second)
                aFiles.push_back(rName);
    return aFiles;
}

const Database* CompileCommandProvider::Load(const OUString& rJsonUrl)
{
    const sal_uInt64 nModified = ModifiedOf(rJsonUrl);
    if (nModified == 0)
    {
        m_aDatabases.erase(rJsonUrl);
        return nullptr; // not there
    }
    auto it = m_aDatabases.find(rJsonUrl);
    if (it != m_aDatabases.end() && it->second.nModified == nModified)
        return &it->second;

    Database aDatabase;
    aDatabase.nModified = nModified;
    try
    {
        const OUString aPath = ToSystemPath(rJsonUrl);
        const OString aPathUtf8 = OUStringToOString(aPath, RTL_TEXTENCODING_UTF8);
        pt::ptree aTree;
        pt::read_json(std::string(aPathUtf8.getStr(), aPathUtf8.getLength()), aTree);
        for (const auto& rEntry : aTree)
        {
            const std::string aDirectory = rEntry.second.get<std::string>("directory", "");
            const std::string aFile = rEntry.second.get<std::string>("file", "");
            if (aFile.empty())
                continue;
            const OUString aDirectoryUrl = ToUrl(FromUtf8(aDirectory));
            OUString aFileUrl;
            osl::FileBase::getAbsoluteFileURL(aDirectoryUrl, FromUtf8(aFile), aFileUrl);
            if (aFileUrl.isEmpty())
                continue;
            std::vector<std::string> aWords;
            if (auto xArguments = rEntry.second.get_child_optional("arguments"))
                for (const auto& rWord : *xArguments)
                    aWords.push_back(rWord.second.get_value<std::string>());
            else
                aWords = SplitCommand(rEntry.second.get<std::string>("command", ""));
            if (aWords.empty())
                continue;
            aDatabase.aByFile[aFileUrl] = UsefulArguments(aWords, aDirectoryUrl);
            aDatabase.aSiblings.try_emplace(DirectoryOf(aFileUrl), aFileUrl);
        }
    }
    catch (const std::exception& rException)
    {
        SAL_WARN("code.services", "cannot read " << rJsonUrl << ": " << rException.what());
    }
    return &(m_aDatabases[rJsonUrl] = std::move(aDatabase));
}

uno::Sequence<OUString> CompileCommandProvider::getCompileArguments(const OUString& rUrl)
{
    const std::vector<OUString> aCandidates = CandidateFiles(rUrl);
    std::unique_lock aGuard(m_aMutex);
    const Arguments* pSibling = nullptr;
    for (const OUString& rJson : aCandidates)
    {
        const Database* pDatabase = Load(rJson);
        if (!pDatabase)
            continue;
        auto it = pDatabase->aByFile.find(rUrl);
        if (it != pDatabase->aByFile.end())
            return comphelper::containerToSequence(it->second);
        if (!pSibling)
        {
            auto itDir = pDatabase->aSiblings.find(DirectoryOf(rUrl));
            if (itDir != pDatabase->aSiblings.end())
                pSibling = &pDatabase->aByFile.at(itDir->second);
        }
    }
    return pSibling ? comphelper::containerToSequence(*pSibling) : uno::Sequence<OUString>();
}
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_CompileCommandProvider_get_implementation(
    uno::XComponentContext* pContext, uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::CompileCommandProvider(pContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
