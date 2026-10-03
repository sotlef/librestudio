/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "folderproject.hxx"

#include "common.hxx"

#include <com/sun/star/code/project/ItemKind.hpp>
#include <com/sun/star/code/project/XProjectFile.hpp>
#include <com/sun/star/code/project/XProjectFiles.hpp>
#include <com/sun/star/code/project/XProjectFolder.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <cppuhelper/weakref.hxx>
#include <comphelper/compbase.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/implbase.hxx>
#include <osl/file.hxx>
#include <rtl/ref.hxx>
#include <rtl/uri.hxx>

#include <algorithm>
#include <vector>

using namespace css;
using namespace css::code::project;

namespace codeservices
{
bool IsDirectory(const OUString& rUrl)
{
    osl::DirectoryItem aItem;
    if (osl::DirectoryItem::get(rUrl, aItem) != osl::FileBase::E_None)
        return false;
    osl::FileStatus aStatus(osl_FileStatus_Mask_Type);
    return aItem.getFileStatus(aStatus) == osl::FileBase::E_None
           && aStatus.getFileType() == osl::FileStatus::Directory;
}

OUString GetDisplayName(const OUString& rUrl)
{
    const OUString aUrl = NormalizeUrl(rUrl);
    const OUString aLast = aUrl.copy(aUrl.lastIndexOf('/') + 1);
    return rtl::Uri::decode(aLast, rtl_UriDecodeWithCharset, RTL_TEXTENCODING_UTF8);
}

namespace
{
// What the folder and file nodes have in common.
struct NodeData
{
    css::uno::WeakReference<XProjectItem> xParent; // weak: the parent owns its children
    OUString aName;
    OUString aUrl;
    OUString aRelativePath;
};

OUString RelativePathOf(const NodeData& rParent, const OUString& rName)
{
    return rParent.aRelativePath.isEmpty() ? rName : rParent.aRelativePath + "/" + rName;
}

class FolderNode final : public cppu::WeakImplHelper<XProjectFolder>
{
public:
    FolderNode(NodeData aData)
        : m_aData(std::move(aData))
    {
    }

    OUString SAL_CALL getName() override { return m_aData.aName; }
    ItemKind SAL_CALL getKind() override { return ItemKind_FOLDER; }
    OUString SAL_CALL getUrl() override { return m_aData.aUrl; }
    uno::Reference<XProjectItem> SAL_CALL getParent() override { return m_aData.xParent; }
    uno::Sequence<uno::Reference<XProjectItem>> SAL_CALL getChildren() override;
    OUString SAL_CALL getRelativePath() override { return m_aData.aRelativePath; }

private:
    NodeData m_aData;
    std::mutex m_aMutex;
    bool m_bListed = false;
    std::vector<uno::Reference<XProjectItem>> m_aChildren;
};

class FileNode final : public cppu::WeakImplHelper<XProjectFile>
{
public:
    FileNode(NodeData aData)
        : m_aData(std::move(aData))
        , m_aMime(GetMimeType(m_aData.aUrl))
    {
    }

    OUString SAL_CALL getName() override { return m_aData.aName; }
    ItemKind SAL_CALL getKind() override { return ItemKind_FILE; }
    OUString SAL_CALL getUrl() override { return m_aData.aUrl; }
    uno::Reference<XProjectItem> SAL_CALL getParent() override { return m_aData.xParent; }
    uno::Sequence<uno::Reference<XProjectItem>> SAL_CALL getChildren() override { return {}; }
    OUString SAL_CALL getRelativePath() override { return m_aData.aRelativePath; }
    OUString SAL_CALL getMimeType() override { return m_aMime; }
    sal_Bool SAL_CALL getGenerated() override { return false; }

private:
    NodeData m_aData;
    const OUString m_aMime;
};

uno::Sequence<uno::Reference<XProjectItem>> FolderNode::getChildren()
{
    std::unique_lock aGuard(m_aMutex);
    if (!m_bListed)
    {
        m_bListed = true;

        struct Entry
        {
            OUString aName;
            OUString aUrl;
            bool bDirectory;
        };
        std::vector<Entry> aEntries;
        osl::Directory aDirectory(m_aData.aUrl);
        if (aDirectory.open() == osl::FileBase::E_None)
        {
            osl::DirectoryItem aItem;
            while (aDirectory.getNextItem(aItem) == osl::FileBase::E_None)
            {
                osl::FileStatus aStatus(osl_FileStatus_Mask_Type | osl_FileStatus_Mask_FileName
                                        | osl_FileStatus_Mask_FileURL);
                if (aItem.getFileStatus(aStatus) != osl::FileBase::E_None)
                    continue;
                const OUString aName = aStatus.getFileName();
                // Hidden entries (.git ...) are not part of the project; links are
                // skipped so that a cycle cannot make the tree infinite.
                if (aName.startsWith(".") || aStatus.getFileType() == osl::FileStatus::Link)
                    continue;
                aEntries.push_back(
                    { aName, aStatus.getFileURL(),
                      aStatus.getFileType() == osl::FileStatus::Directory });
            }
            aDirectory.close();
        }

        // Folders before files, each group in name order.
        std::sort(aEntries.begin(), aEntries.end(),
                  [](const Entry& a, const Entry& b)
                  {
                      if (a.bDirectory != b.bDirectory)
                          return a.bDirectory;
                      return a.aName.compareToIgnoreAsciiCase(b.aName) < 0;
                  });

        uno::Reference<XProjectItem> xSelf(this);
        for (const Entry& rEntry : aEntries)
        {
            NodeData aData{ xSelf, rEntry.aName, NormalizeUrl(rEntry.aUrl),
                            RelativePathOf(m_aData, rEntry.aName) };
            if (rEntry.bDirectory)
                m_aChildren.emplace_back(new FolderNode(std::move(aData)));
            else
                m_aChildren.emplace_back(new FileNode(std::move(aData)));
        }
    }
    return comphelper::containerToSequence(m_aChildren);
}

class FolderProject final : public comphelper::WeakComponentImplHelper<XProject, XProjectFiles>
{
public:
    FolderProject(const OUString& rRootUrl, const OUString& rIdentityUrl, const OUString& rKind)
        : m_aUrl(rIdentityUrl)
        , m_aKind(rKind)
        , m_xRoot(new FolderNode(NodeData{ {}, GetDisplayName(rRootUrl), rRootUrl, OUString() }))
    {
    }

    // XProject
    OUString SAL_CALL getName() override { return m_xRoot->getName(); }
    OUString SAL_CALL getUrl() override { return m_aUrl; }
    OUString SAL_CALL getFormatKind() override { return m_aKind; }
    uno::Sequence<beans::PropertyValue> SAL_CALL getProperties() override { return {}; }

    // XProjectFiles
    uno::Reference<XProjectItem> SAL_CALL getRootItem() override { return m_xRoot; }

private:
    void disposing(std::unique_lock<std::mutex>& rGuard) override
    {
        comphelper::WeakComponentImplHelperBase::disposing(rGuard);
    }

    const OUString m_aUrl;
    const OUString m_aKind;
    uno::Reference<XProjectItem> m_xRoot;
};
}

uno::Reference<XProject> CreateFolderProject(const OUString& rUrl)
{
    const OUString aUrl = NormalizeUrl(rUrl);
    if (!IsDirectory(aUrl))
        throw io::IOException("not a directory: " + aUrl);
    return new FolderProject(aUrl, aUrl, u"folder"_ustr);
}

uno::Reference<XProject> CreateFileTreeProject(const OUString& rRootUrl, const OUString& rIdentityUrl,
                                               const OUString& rFormatKind)
{
    const OUString aRoot = NormalizeUrl(rRootUrl);
    if (!IsDirectory(aRoot))
        throw io::IOException("not a directory: " + aRoot);
    return new FolderProject(aRoot, NormalizeUrl(rIdentityUrl), rFormatKind);
}

OUString GetParentUrl(const OUString& rUrl)
{
    const OUString aUrl = NormalizeUrl(rUrl);
    return aUrl.copy(0, std::max<sal_Int32>(aUrl.lastIndexOf('/'), 0));
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
