/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "cmakeproject.hxx"

#include "common.hxx"
#include "folderproject.hxx"

#include <com/sun/star/code/project/ItemKind.hpp>
#include <com/sun/star/code/project/ProjectConfiguration.hpp>
#include <com/sun/star/code/project/TargetType.hpp>
#include <com/sun/star/code/project/XProjectConfigStore.hpp>
#include <com/sun/star/code/project/XProjectConfigurationState.hpp>
#include <com/sun/star/code/project/XProjectFile.hpp>
#include <com/sun/star/code/project/XProjectFiles.hpp>
#include <com/sun/star/code/project/XProjectTarget.hpp>
#include <com/sun/star/code/project/XProjectTargets.hpp>
#include <com/sun/star/code/project/XSourceGroup.hpp>
#include <com/sun/star/code/project/theProjectConfigStore.hpp>
#include <com/sun/star/util/XModifyBroadcaster.hpp>
#include <com/sun/star/util/XModifyListener.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/weakref.hxx>
#include <osl/detail/file.h>
#include <rtl/ref.hxx>
#include <osl/file.hxx>
#include <rtl/strbuf.hxx>
#include <sal/log.hxx>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <map>
#include <mutex>
#include <sstream>
#include <vector>

using namespace css;
using namespace css::code::project;

namespace pt = boost::property_tree;

namespace cmakeproj
{
namespace
{
// ---- the File API --------------------------------------------------------
//
// CMake's own description of a configured project (cmake-file-api(7)): a
// client leaves an empty file <build>/.cmake/api/v1/query/codemodel-v2 before
// the configure run, and cmake then writes <build>/.cmake/api/v1/reply/,
// whose index-*.json names the "codemodel" object, which names one JSON file
// per target.  This is what CLion and Qt Creator use; reading it means the
// structure is exactly what CMake built, not our guess at what the
// CMakeLists.txt files say.

OUString ReplyDirectoryOf(const OUString& rBuildUrl) { return codeservices::NormalizeUrl(rBuildUrl) + "/.cmake/api/v1/reply"; }

bool ReadFile(const OUString& rUrl, std::string& rText)
{
    osl::File aFile(rUrl);
    if (aFile.open(osl_File_OpenFlag_Read) != osl::FileBase::E_None)
        return false;
    OStringBuffer aBytes;
    char aBuffer[16384];
    sal_uInt64 nRead = 0;
    while (aFile.read(aBuffer, sizeof aBuffer, nRead) == osl::FileBase::E_None && nRead)
        aBytes.append(aBuffer, static_cast<sal_Int32>(nRead));
    aFile.close();
    rText = aBytes.makeStringAndClear().getStr();
    return true;
}

bool ReadJson(const OUString& rUrl, pt::ptree& rTree)
{
    std::string aText;
    if (!ReadFile(rUrl, aText))
        return false;
    try
    {
        std::istringstream aStream(aText);
        pt::read_json(aStream, rTree);
        return true;
    }
    catch (const std::exception& rException)
    {
        SAL_WARN("code.cmake", "unreadable " << rUrl << ": " << rException.what());
        return false;
    }
}

// the newest index-*.json of the reply directory (their names sort by time)
OUString NewestIndexOf(const OUString& rReplyUrl)
{
    OUString aNewest;
    osl::Directory aDirectory(rReplyUrl);
    if (aDirectory.open() != osl::FileBase::E_None)
        return aNewest;
    osl::DirectoryItem aItem;
    while (aDirectory.getNextItem(aItem) == osl::FileBase::E_None)
    {
        osl::FileStatus aStatus(osl_FileStatus_Mask_FileName);
        if (aItem.getFileStatus(aStatus) != osl::FileBase::E_None)
            continue;
        const OUString aName = aStatus.getFileName();
        if (aName.startsWith("index-") && aName.endsWith(".json") && aName > aNewest)
            aNewest = aName;
    }
    return aNewest.isEmpty() ? aNewest : rReplyUrl + "/" + aNewest;
}

// "/a/b/../c" -> "/a/c"
std::string NormalizePath(const std::string& rPath)
{
    std::vector<std::string> aParts;
    std::string aPart;
    std::istringstream aStream(rPath);
    while (std::getline(aStream, aPart, '/'))
    {
        if (aPart.empty() || aPart == ".")
            continue;
        if (aPart == "..")
        {
            if (!aParts.empty())
                aParts.pop_back();
        }
        else
            aParts.push_back(aPart);
    }
    std::string aResult;
    for (const std::string& rSegment : aParts)
        aResult += "/" + rSegment;
    return aResult.empty() ? "/" : aResult;
}

OUString FileUrlOf(const std::string& rSourceRoot, const std::string& rPath)
{
    const std::string aAbsolute = !rPath.empty() && rPath[0] == '/' ? rPath : rSourceRoot + "/" + rPath;
    OUString aUrl;
    osl::FileBase::getFileURLFromSystemPath(OUString::fromUtf8(NormalizePath(aAbsolute)), aUrl);
    return aUrl;
}

// ---- the structure ---------------------------------------------------------

struct ItemData
{
    css::uno::WeakReference<XProjectItem> xParent; // weak: the parent owns its children
    OUString aName;
    OUString aUrl;
    std::vector<uno::Reference<XProjectItem>> aChildren;
};

// XProjectItem's part, the same for every kind of node
#define CMAKEPROJ_ITEM_METHODS(KIND)                                                              \
    OUString SAL_CALL getName() override { return m_aData.aName; }                                \
    ItemKind SAL_CALL getKind() override { return KIND; }                                         \
    OUString SAL_CALL getUrl() override { return m_aData.aUrl; }                                  \
    uno::Reference<XProjectItem> SAL_CALL getParent() override { return m_aData.xParent; }       \
    uno::Sequence<uno::Reference<XProjectItem>> SAL_CALL getChildren() override                   \
    {                                                                                             \
        return comphelper::containerToSequence(m_aData.aChildren);                                \
    }                                                                                             \
                                                                                                  \
public:                                                                                           \
    ItemData m_aData;                                                                             \
                                                                                                  \
private:

class TargetNode final : public cppu::WeakImplHelper<XProjectItem, XProjectTarget>
{
public:
    TargetNode(const OUString& rName, TargetType eType)
        : m_eType(eType)
    {
        m_aData.aName = rName;
    }
    TargetType SAL_CALL getType() override { return m_eType; }
    CMAKEPROJ_ITEM_METHODS(ItemKind_TARGET)

    const TargetType m_eType;
};

class GroupNode final : public cppu::WeakImplHelper<XSourceGroup>
{
public:
    explicit GroupNode(const OUString& rName) { m_aData.aName = rName; }
    OUString SAL_CALL getGroupPath() override { return m_aData.aName; }
    OUString SAL_CALL getFilter() override { return {}; }
    CMAKEPROJ_ITEM_METHODS(ItemKind_GROUP)
};

class FileNode final : public cppu::WeakImplHelper<XProjectFile>
{
public:
    FileNode(const OUString& rName, const OUString& rUrl, bool bGenerated)
        : m_aMime(codeservices::GetMimeType(rUrl))
        , m_bGenerated(bGenerated)
    {
        m_aData.aName = rName;
        m_aData.aUrl = rUrl;
    }
    OUString SAL_CALL getRelativePath() override { return m_aData.aName; }
    OUString SAL_CALL getMimeType() override { return m_aMime; }
    sal_Bool SAL_CALL getGenerated() override { return m_bGenerated; }
    CMAKEPROJ_ITEM_METHODS(ItemKind_FILE)

    const OUString m_aMime;
    const bool m_bGenerated;
};

TargetType TargetTypeOf(const std::string& rType)
{
    if (rType == "EXECUTABLE")
        return TargetType_EXECUTABLE;
    if (rType == "SHARED_LIBRARY" || rType == "MODULE_LIBRARY")
        return TargetType_SHARED_LIBRARY;
    if (rType == "STATIC_LIBRARY" || rType == "OBJECT_LIBRARY")
        return TargetType_STATIC_LIBRARY;
    if (rType == "UTILITY" || rType == "INTERFACE_LIBRARY")
        return TargetType_UTILITY;
    return TargetType_OTHER;
}

struct Structure
{
    bool bValid = false; // a code model was read (it may have no targets)
    std::vector<uno::Reference<XProjectTarget>> aTargets;
};

// Reads the code model in rBuildUrl's reply directory; an empty root if there
// is none (never configured, or configuring failed).
Structure ReadStructure(const OUString& rBuildUrl)
{
    Structure aResult;
    const OUString aReplyUrl = ReplyDirectoryOf(rBuildUrl);
    const OUString aIndexUrl = NewestIndexOf(aReplyUrl);
    if (aIndexUrl.isEmpty())
        return aResult;

    try
    {
        pt::ptree aIndex;
        if (!ReadJson(aIndexUrl, aIndex))
            return aResult;
        std::string aModelFile;
        for (const auto& rObject : aIndex.get_child("objects"))
            if (rObject.second.get<std::string>("kind", "") == "codemodel")
                aModelFile = rObject.second.get<std::string>("jsonFile", "");
        pt::ptree aModel;
        if (aModelFile.empty() || !ReadJson(aReplyUrl + "/" + OUString::fromUtf8(aModelFile), aModel))
            return aResult;

        const std::string aSourceRoot = aModel.get<std::string>("paths.source");
        // A multi-configuration generator reports each configuration; the
        // targets are the same in all of them, so the first stands for them.
        const pt::ptree& rConfiguration = aModel.get_child("configurations").begin()->second;
        for (const auto& rTargetRef : rConfiguration.get_child("targets"))
        {
            pt::ptree aTarget;
            if (!ReadJson(aReplyUrl + "/" + OUString::fromUtf8(rTargetRef.second.get<std::string>("jsonFile")), aTarget))
                continue;

            rtl::Reference<TargetNode> xTarget = new TargetNode(
                OUString::fromUtf8(aTarget.get<std::string>("name", "")),
                TargetTypeOf(aTarget.get<std::string>("type", "")));

            // the target's files, by group; a file in no group goes under the default one
            std::vector<std::pair<OUString, rtl::Reference<GroupNode>>> aGroups;
            auto groupNamed = [&](const OUString& rName) -> rtl::Reference<GroupNode>
            {
                for (auto& rGroup : aGroups)
                    if (rGroup.first == rName)
                        return rGroup.second;
                rtl::Reference<GroupNode> xGroup = new GroupNode(rName);
                xGroup->m_aData.xParent = uno::Reference<XProjectItem>(xTarget.get());
                xTarget->m_aData.aChildren.push_back(uno::Reference<XProjectItem>(xGroup.get()));
                aGroups.emplace_back(rName, xGroup);
                return xGroup;
            };

            std::vector<OUString> aGroupOfSource;
            if (auto oGroups = aTarget.get_child_optional("sourceGroups"))
            {
                const size_t nSources = aTarget.get_child("sources").size();
                aGroupOfSource.resize(nSources);
                for (const auto& rGroup : *oGroups)
                    for (const auto& rIndex : rGroup.second.get_child("sourceIndexes"))
                    {
                        const size_t nIndex = rIndex.second.get_value<size_t>();
                        if (nIndex < nSources)
                            aGroupOfSource[nIndex] = OUString::fromUtf8(rGroup.second.get<std::string>("name", ""));
                    }
            }

            size_t nSource = 0;
            if (auto oSources = aTarget.get_child_optional("sources"))
                for (const auto& rSource : *oSources)
                {
                    const std::string aPath = rSource.second.get<std::string>("path", "");
                    const OUString aUrl = FileUrlOf(aSourceRoot, aPath);
                    OUString aGroupName = nSource < aGroupOfSource.size() ? aGroupOfSource[nSource] : OUString();
                    ++nSource;
                    if (aGroupName.isEmpty())
                        aGroupName = u"Source Files"_ustr;

                    rtl::Reference<GroupNode> xGroup = groupNamed(aGroupName);
                    rtl::Reference<FileNode> xFile
                        = new FileNode(codeservices::GetDisplayName(aUrl), aUrl, rSource.second.get<bool>("isGenerated", false));
                    xFile->m_aData.xParent = uno::Reference<XProjectItem>(xGroup.get());
                    xGroup->m_aData.aChildren.push_back(uno::Reference<XProjectItem>(xFile.get()));
                }

            aResult.aTargets.push_back(uno::Reference<XProjectTarget>(xTarget.get()));
        }
        aResult.bValid = true;
    }
    catch (const std::exception& rException)
    {
        // a reply of a shape we do not know: treated as not configured
        SAL_WARN("code.cmake", "cannot read the CMake code model of " << rBuildUrl << ": " << rException.what());
        aResult = Structure();
    }
    return aResult;
}

// ---- the project -------------------------------------------------------------

class CMakeProject;

// the open projects by their CMakeLists.txt, to tell them when cmake is done
std::mutex g_aRegistryMutex;
std::map<OUString, css::uno::WeakReference<XProject>> g_aOpenProjects;

class CMakeProject final
    : public comphelper::WeakComponentImplHelper<XProject, XProjectFiles, XProjectTargets,
                                                 XProjectConfigurationState, util::XModifyBroadcaster>
{
public:
    CMakeProject(uno::Reference<uno::XComponentContext> xContext, uno::Reference<XProject> xFolder,
                 const OUString& rUrl)
        : m_xContext(std::move(xContext))
        , m_xFolder(std::move(xFolder))
        , m_xFiles(m_xFolder, uno::UNO_QUERY_THROW)
        , m_aUrl(rUrl)
    {
    }

    // XProject
    OUString SAL_CALL getName() override { return m_xFolder->getName(); }
    OUString SAL_CALL getUrl() override { return m_aUrl; }
    OUString SAL_CALL getFormatKind() override { return u"cmake"_ustr; }
    uno::Sequence<beans::PropertyValue> SAL_CALL getProperties() override { return {}; }

    // XProjectFiles
    uno::Reference<XProjectItem> SAL_CALL getRootItem() override { return m_xFiles->getRootItem(); }

    // XProjectTargets
    uno::Sequence<uno::Reference<XProjectTarget>> SAL_CALL getTargets() override
    {
        return comphelper::containerToSequence(structure().aTargets);
    }

    // XProjectConfigurationState
    sal_Bool SAL_CALL getConfigured() override { return structure().bValid; }

    // XModifyBroadcaster
    void SAL_CALL addModifyListener(const uno::Reference<util::XModifyListener>& xListener) override
    {
        std::unique_lock aGuard(m_aMutex);
        m_aModifyListeners.addInterface(aGuard, xListener);
    }
    void SAL_CALL removeModifyListener(const uno::Reference<util::XModifyListener>& xListener) override
    {
        std::unique_lock aGuard(m_aMutex);
        m_aModifyListeners.removeInterface(aGuard, xListener);
    }

    // XProjectConfigurationState
    void SAL_CALL reload() override { Reload(); }

    /// cmake has run, or another configuration was made current: forget what
    /// was read and tell the listeners
    void Reload()
    {
        std::unique_lock aGuard(m_aMutex);
        m_bLoaded = false;
        lang::EventObject aEvent(getXWeak());
        m_aModifyListeners.forEach(aGuard, [&](const uno::Reference<util::XModifyListener>& xListener)
                                   { xListener->modified(aEvent); });
    }

private:
    Structure structure()
    {
        {
            std::unique_lock aGuard(m_aMutex);
            if (m_bLoaded)
                return m_aStructure;
        }
        // reading is slow and goes to disk: not under the lock
        OUString aBuildUrl;
        try
        {
            // The structure is read from the build directory of the current
            // configuration: each configuration has a build tree of its own,
            // and CMake's reply lives in it.
            auto xStore = theProjectConfigStore::get(m_xContext);
            const OUString aCurrent = xStore->getCurrentConfiguration(m_aUrl);
            for (const ProjectConfiguration& rConfiguration : xStore->getConfigurations(m_aUrl))
                if (rConfiguration.Name == aCurrent)
                    for (const beans::StringPair& rProperty : rConfiguration.Properties)
                        if (rProperty.First == "BuildDirectory")
                            aBuildUrl = rProperty.Second;
        }
        catch (const uno::Exception&)
        {
        }
        Structure aStructure = aBuildUrl.isEmpty() ? Structure() : ReadStructure(aBuildUrl);

        std::unique_lock aGuard(m_aMutex);
        m_aStructure = std::move(aStructure);
        m_bLoaded = true;
        return m_aStructure;
    }

    uno::Reference<uno::XComponentContext> m_xContext;
    uno::Reference<XProject> m_xFolder;
    uno::Reference<XProjectFiles> m_xFiles;
    const OUString m_aUrl;

    bool m_bLoaded = false;
    Structure m_aStructure;
    comphelper::OInterfaceContainerHelper4<util::XModifyListener> m_aModifyListeners;
};
}

uno::Reference<XProject> CreateCMakeProject(const uno::Reference<uno::XComponentContext>& xContext,
                                            const OUString& rCMakeListsUrl)
{
    // The plain file tree is the folder project's; this adds what CMake knows.
    uno::Reference<XProject> xFolder = codeservices::CreateFileTreeProject(
        codeservices::GetParentUrl(rCMakeListsUrl), rCMakeListsUrl, u"cmake"_ustr);
    uno::Reference<XProject> xProject(new CMakeProject(xContext, xFolder, codeservices::NormalizeUrl(rCMakeListsUrl)));

    std::scoped_lock aGuard(g_aRegistryMutex);
    g_aOpenProjects[codeservices::NormalizeUrl(rCMakeListsUrl)] = xProject;
    return xProject;
}

void RequestCodeModel(const OUString& rBuildDirectoryUrl)
{
    const OUString aQueryDirectory = codeservices::NormalizeUrl(rBuildDirectoryUrl) + "/.cmake/api/v1/query";
    osl::Directory::createPath(aQueryDirectory);
    osl::File aFile(aQueryDirectory + "/codemodel-v2");
    if (aFile.open(osl_File_OpenFlag_Write | osl_File_OpenFlag_Create | osl_File_OpenFlag_NoExcl)
        == osl::FileBase::E_None)
        aFile.close();
}

void NotifyConfigured(const OUString& rCMakeListsUrl)
{
    uno::Reference<XProject> xProject;
    {
        std::scoped_lock aGuard(g_aRegistryMutex);
        const auto it = g_aOpenProjects.find(codeservices::NormalizeUrl(rCMakeListsUrl));
        if (it != g_aOpenProjects.end())
            xProject = it->second;
    }
    if (auto* pProject = dynamic_cast<CMakeProject*>(xProject.get()))
        pProject->Reload();
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
