/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "recentprojects.hxx"

#include <osl/detail/file.h>
#include <osl/file.hxx>
#include <rtl/bootstrap.hxx>
#include <rtl/strbuf.hxx>
#include <sal/log.hxx>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <mutex>
#include <sstream>

namespace code
{
namespace
{
namespace pt = boost::property_tree;

constexpr char FormatName[] = "librestudio-recent-projects";
constexpr char FormatVersion[] = "1";

// one reader or writer at a time within this library; the other copy of this
// code (exe or codesidebar) is a different process-wide object, but only the
// shell writes in practice
std::mutex g_aMutex;

// the file, or empty if there is no user profile
OUString FileUrl()
{
    OUString aProfile;
    if (!rtl::Bootstrap::get(u"UserInstallation"_ustr, aProfile) || aProfile.isEmpty())
        return {};
    while (aProfile.getLength() > 1 && aProfile.endsWith("/"))
        aProfile = aProfile.copy(0, aProfile.getLength() - 1);
    return aProfile + "/code/recent-projects.json";
}

std::vector<OUString> Load()
{
    std::vector<OUString> aRecent;
    const OUString aFileUrl = FileUrl();
    if (aFileUrl.isEmpty())
        return aRecent;

    osl::File aFile(aFileUrl);
    if (aFile.open(osl_File_OpenFlag_Read) != osl::FileBase::E_None)
        return aRecent;
    OStringBuffer aBytes;
    char aBuffer[4096];
    sal_uInt64 nRead = 0;
    while (aFile.read(aBuffer, sizeof aBuffer, nRead) == osl::FileBase::E_None && nRead)
        aBytes.append(aBuffer, static_cast<sal_Int32>(nRead));
    aFile.close();

    try
    {
        std::istringstream aStream(aBytes.makeStringAndClear().getStr());
        pt::ptree aTree;
        pt::read_json(aStream, aTree);
        if (aTree.get<std::string>("format", "") != FormatName
            || aTree.get<std::string>("version", "") != FormatVersion)
            return aRecent;
        for (const auto& rProject : aTree.get_child("projects"))
            aRecent.push_back(OUString::fromUtf8(rProject.second.get_value<std::string>()));
    }
    catch (const std::exception& rException)
    {
        // a damaged list is an empty one, not a reason to stop opening projects
        SAL_WARN("code", "unreadable recent projects " << aFileUrl << ": " << rException.what());
        aRecent.clear();
    }
    if (aRecent.size() > RecentProjects::Max)
        aRecent.resize(RecentProjects::Max);
    return aRecent;
}

void Save(const std::vector<OUString>& rRecent)
{
    const OUString aFileUrl = FileUrl();
    if (aFileUrl.isEmpty())
        return;

    pt::ptree aTree;
    aTree.put("format", FormatName);
    aTree.put("version", FormatVersion);
    pt::ptree aProjects;
    for (const OUString& rUrl : rRecent)
        aProjects.push_back({ "", pt::ptree(OUStringToOString(rUrl, RTL_TEXTENCODING_UTF8).getStr()) });
    aTree.add_child("projects", aProjects);
    std::ostringstream aStream;
    pt::write_json(aStream, aTree);
    const std::string aText = aStream.str();

    osl::Directory::createPath(aFileUrl.copy(0, aFileUrl.lastIndexOf('/')));
    osl::File aFile(aFileUrl);
    if (aFile.open(osl_File_OpenFlag_Write | osl_File_OpenFlag_Create | osl_File_OpenFlag_NoExcl
                   | osl_File_OpenFlag_Trunc)
        != osl::FileBase::E_None)
    {
        SAL_WARN("code", "cannot write " << aFileUrl);
        return;
    }
    sal_uInt64 nWritten = 0;
    aFile.write(aText.data(), aText.size(), nWritten);
    aFile.close();
}
}

std::vector<OUString> RecentProjects::Get()
{
    std::scoped_lock aGuard(g_aMutex);
    return Load();
}

void RecentProjects::Add(const OUString& rUrl)
{
    std::scoped_lock aGuard(g_aMutex);
    std::vector<OUString> aRecent = Load();
    std::erase(aRecent, rUrl);
    aRecent.insert(aRecent.begin(), rUrl);
    if (aRecent.size() > Max)
        aRecent.resize(Max);
    Save(aRecent);
}

void RecentProjects::Remove(const OUString& rUrl)
{
    std::scoped_lock aGuard(g_aMutex);
    std::vector<OUString> aRecent = Load();
    if (std::erase(aRecent, rUrl) > 0)
        Save(aRecent);
}

void RecentProjects::Clear()
{
    std::scoped_lock aGuard(g_aMutex);
    Save({});
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
