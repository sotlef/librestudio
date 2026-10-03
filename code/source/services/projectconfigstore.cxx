/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "common.hxx"
#include "folderproject.hxx"

#include <com/sun/star/code/project/XProjectConfigStore.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/sequence.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <osl/detail/file.h>
#include <osl/file.hxx>
#include <rtl/strbuf.hxx>
#include <sal/log.hxx>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <mutex>
#include <sstream>

using namespace css;
using namespace css::code::project;

namespace codeservices
{
namespace
{
namespace pt = boost::property_tree;

// The remembered configurations of a project live in the project's own
// directory, in a file of this IDE's own format:
//
//   <project directory>/.librestudio/<project file name>.json
//
//   {
//     "format": "librestudio-project-configuration",
//     "version": "1",
//     "configurations": {              one member per configuration, in order
//       "Debug":   { "BuildType": "Debug", "BuildDirectory": "file:///..." },
//       "Release": { ... }
//     }
//   }
//
// Every value is a string, as in ProjectConfiguration.  The directory is
// hidden, so the project's own file tree does not show it.
constexpr char FormatName[] = "librestudio-project-configuration";
constexpr char FormatVersion[] = "1";

OUString ConfigDirectoryOf(const OUString& rProjectUrl)
{
    const OUString aUrl = NormalizeUrl(rProjectUrl);
    return (IsDirectory(aUrl) ? aUrl : GetParentUrl(aUrl)) + "/.librestudio";
}

OUString ConfigFileOf(const OUString& rProjectUrl)
{
    return ConfigDirectoryOf(rProjectUrl) + "/" + GetDisplayName(rProjectUrl) + ".json";
}

class ProjectConfigStore final : public cppu::WeakImplHelper<XProjectConfigStore, lang::XServiceInfo>
{
public:
    // XProjectConfigStore
    uno::Sequence<ProjectConfiguration> SAL_CALL getConfigurations(const OUString& rProjectUrl) override;
    void SAL_CALL setConfigurations(const OUString& rProjectUrl,
                                    const uno::Sequence<ProjectConfiguration>& rConfigurations) override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.ProjectConfigStore"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

private:
    std::mutex m_aMutex; // one reader or writer of the files at a time
};

uno::Sequence<ProjectConfiguration> ProjectConfigStore::getConfigurations(const OUString& rProjectUrl)
{
    std::scoped_lock aGuard(m_aMutex);

    osl::File aFile(ConfigFileOf(rProjectUrl));
    if (aFile.open(osl_File_OpenFlag_Read) != osl::FileBase::E_None)
        return {}; // no file: never configured

    OStringBuffer aBytes;
    char aBuffer[4096];
    sal_uInt64 nRead = 0;
    while (aFile.read(aBuffer, sizeof aBuffer, nRead) == osl::FileBase::E_None && nRead)
        aBytes.append(aBuffer, static_cast<sal_Int32>(nRead));
    aFile.close();

    std::vector<ProjectConfiguration> aResult;
    try
    {
        std::istringstream aStream(aBytes.makeStringAndClear().getStr());
        pt::ptree aTree;
        pt::read_json(aStream, aTree);

        // A file of another format, or of a newer version, is not understood:
        // treated as "not configured", which asks the user again and replaces it.
        if (aTree.get<std::string>("format", "") != FormatName
            || aTree.get<std::string>("version", "") != FormatVersion)
            return {};

        for (const auto& rConfiguration : aTree.get_child("configurations"))
        {
            ProjectConfiguration aConfiguration;
            aConfiguration.Name = OUString::fromUtf8(rConfiguration.first);
            std::vector<beans::StringPair> aProperties;
            for (const auto& rProperty : rConfiguration.second)
                aProperties.push_back({ OUString::fromUtf8(rProperty.first),
                                        OUString::fromUtf8(rProperty.second.data()) });
            aConfiguration.Properties = comphelper::containerToSequence(aProperties);
            aResult.push_back(std::move(aConfiguration));
        }
    }
    catch (const std::exception& rException)
    {
        // A damaged file must not stop the project from opening; it is configured again.
        SAL_WARN("code.services", "unreadable project configuration " << ConfigFileOf(rProjectUrl)
                                                                       << ": " << rException.what());
        return {};
    }
    return comphelper::containerToSequence(aResult);
}

void ProjectConfigStore::setConfigurations(const OUString& rProjectUrl,
                                           const uno::Sequence<ProjectConfiguration>& rConfigurations)
{
    pt::ptree aTree;
    aTree.put("format", FormatName);
    aTree.put("version", FormatVersion);
    pt::ptree aConfigurations;
    for (const ProjectConfiguration& rConfiguration : rConfigurations)
    {
        pt::ptree aProperties;
        for (const beans::StringPair& rProperty : rConfiguration.Properties)
            // push_back, not put(): a name is a key, never a path to split at dots
            aProperties.push_back({ OUStringToOString(rProperty.First, RTL_TEXTENCODING_UTF8).getStr(),
                                    pt::ptree(OUStringToOString(rProperty.Second, RTL_TEXTENCODING_UTF8).getStr()) });
        aConfigurations.push_back(
            { OUStringToOString(rConfiguration.Name, RTL_TEXTENCODING_UTF8).getStr(), aProperties });
    }
    aTree.add_child("configurations", aConfigurations);

    std::ostringstream aStream;
    pt::write_json(aStream, aTree);
    const std::string aText = aStream.str();

    std::scoped_lock aGuard(m_aMutex);
    // the directory may exist already; that is not an error
    osl::Directory::createPath(ConfigDirectoryOf(rProjectUrl));

    osl::File aFile(ConfigFileOf(rProjectUrl));
    if (aFile.open(osl_File_OpenFlag_Write | osl_File_OpenFlag_Create | osl_File_OpenFlag_NoExcl
                   | osl_File_OpenFlag_Trunc)
        != osl::FileBase::E_None)
        throw io::IOException("cannot write " + ConfigFileOf(rProjectUrl), getXWeak());
    sal_uInt64 nWritten = 0;
    const osl::FileBase::RC eRC = aFile.write(aText.data(), aText.size(), nWritten);
    aFile.close();
    if (eRC != osl::FileBase::E_None || nWritten != aText.size())
        throw io::IOException("cannot write " + ConfigFileOf(rProjectUrl), getXWeak());
}
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_ProjectConfigStore_get_implementation(uno::XComponentContext*,
                                                              uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::ProjectConfigStore);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
