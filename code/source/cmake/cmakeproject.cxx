/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "cmakeproject.hxx"

#include "cmakebuild.hxx"
#include "common.hxx"
#include "folderproject.hxx"

#include <com/sun/star/code/build/XBuildable.hpp>
#include <com/sun/star/code/build/XCompilable.hpp>
#include <com/sun/star/code/debug/LaunchDescriptor.hpp>
#include <com/sun/star/code/debug/XLaunchable.hpp>
#include <com/sun/star/code/project/ItemKind.hpp>
#include <com/sun/star/code/project/ProjectConfiguration.hpp>
#include <com/sun/star/code/project/TargetType.hpp>
#include <com/sun/star/code/project/XProjectConfigStore.hpp>
#include <com/sun/star/code/project/XProjectConfigurationState.hpp>
#include <com/sun/star/code/project/ProjectProperty.hpp>
#include <com/sun/star/code/project/ProjectPropertyPage.hpp>
#include <com/sun/star/code/project/XProjectFile.hpp>
#include <com/sun/star/code/project/XProjectFiles.hpp>
#include <com/sun/star/code/project/XProjectProperties.hpp>
#include <com/sun/star/code/project/XProjectPropertyEditor.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/code/tool/XToolDescriptions.hpp>
#include <com/sun/star/code/tool/theToolDescriptions.hpp>
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
#include <rtl/character.hxx>
#include <rtl/strbuf.hxx>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>

#include <startuptarget.hxx>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <map>
#include <memory>
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

// ---- property pages --------------------------------------------------------
//
// What a properties dialog shows (XProjectProperties): the project from the
// File API's index, cache and toolchains replies; a target from its own reply
// file; a source file from the compile_commands.json database.

using Rows = std::vector<ProjectProperty>;
using Pages = std::vector<ProjectPropertyPage>;

OUString Utf8(const std::string& rText) { return OUString::fromUtf8(rText); }

OUString SystemPathOf(const OUString& rUrl)
{
    OUString aPath;
    return osl::FileBase::getSystemPathFromFileURL(rUrl, aPath) == osl::FileBase::E_None ? aPath : rUrl;
}

// a row is left out when it has nothing to say
void AddRow(Rows& rRows, const OUString& rName, const OUString& rValue, const OUString& rDescription = OUString())
{
    if (!rValue.isEmpty())
        rRows.emplace_back(rName, rValue, rDescription, false, uno::Sequence<OUString>(), -1,
                           false);
}

void AddPage(Pages& rPages, const OUString& rPath, const Rows& rRows)
{
    if (!rRows.empty())
        rPages.emplace_back(rPath, comphelper::containerToSequence(rRows));
}

// several values in one cell; the dialog shows them one to a line
OUString Joined(const std::vector<OUString>& rParts, std::u16string_view aSeparator = u"; ")
{
    OUStringBuffer aText;
    for (const OUString& rPart : rParts)
    {
        if (aText.getLength())
            aText.append(aSeparator);
        aText.append(rPart);
    }
    return aText.makeStringAndClear();
}

// the object of a kind that the index names, read; false if cmake wrote none
bool ReadReplyObject(const OUString& rReplyUrl, const pt::ptree& rIndex, const char* pKind, pt::ptree& rObject)
{
    if (auto oObjects = rIndex.get_child_optional("objects"))
        for (const auto& rEntry : *oObjects)
            if (rEntry.second.get<std::string>("kind", "") == pKind)
                return ReadJson(rReplyUrl + "/" + Utf8(rEntry.second.get<std::string>("jsonFile", "")), rObject);
    return false;
}

std::vector<OUString> StringsOf(const pt::ptree& rTree, const char* pArray, const char* pKey)
{
    std::vector<OUString> aStrings;
    if (auto oArray = rTree.get_child_optional(pArray))
        for (const auto& rItem : *oArray)
        {
            const std::string aValue = rItem.second.get<std::string>(pKey, "");
            if (!aValue.empty())
                aStrings.push_back(Utf8(aValue));
        }
    return aStrings;
}

// "name::@hash" is how cmake identifies a target; the name is the part to show
OUString TargetNameOfId(const std::string& rId) { return Utf8(rId.substr(0, rId.find("::"))); }

// ---- commands through tool descriptions -------------------------------------
//
// theToolDescriptions, the service that knows the options of compilers and
// linkers, reads a command as properties in categories.  Here are only the
// compiler and its version, which the File API names, and how the answer
// becomes the pages of the dialog.

struct Toolchain
{
    std::string aCompiler; // path
    std::string aVersion;
};
// by language: "C", "CXX"
using Toolchains = std::map<std::string, Toolchain>;

// the words of a command fragment ("-O2 -g" is two arguments)
void AddWords(std::vector<std::string>& rArguments, const std::string& rFragment)
{
    std::istringstream aStream(rFragment);
    std::string aWord;
    while (aStream >> aWord)
        rArguments.push_back(aWord);
}

// What the tool descriptions service says of a command, page by page: one
// page per category that has something set, Other for what the description
// does not know.  What does not apply in the mode or the version of the tool
// is said so, not hidden.  Returns false when the service has no description
// for the compiler, and the caller shows the plain list.
bool CommandPages(const uno::Reference<uno::XComponentContext>& xContext, const Toolchain& rToolchain,
                  const OUString& rMode, const std::vector<std::string>& rArguments, const OUString& rRoot,
                  Pages& rPages, const std::vector<std::string>* pDefaults = nullptr)
{
    namespace tool = css::code::tool;
    try
    {
        const uno::Reference<tool::XToolDescriptions> xTools = tool::theToolDescriptions::get(xContext);
        const OUString aToolId = xTools->findTool(Utf8(rToolchain.aCompiler));
        if (aToolId.isEmpty())
            return false;
        std::vector<OUString> aArguments;
        for (const std::string& rArgument : rArguments)
            aArguments.push_back(Utf8(rArgument));
        std::vector<OUString> aDefaults;
        if (pDefaults)
            for (const std::string& rArgument : *pDefaults)
                aDefaults.push_back(Utf8(rArgument));
        // with defaults (a file against its target) the answer says what departs from them
        const tool::CommandDescription aCommand
            = pDefaults ? xTools->describeOverrides(aToolId, rMode, Utf8(rToolchain.aVersion),
                                                    comphelper::containerToSequence(aDefaults),
                                                    comphelper::containerToSequence(aArguments))
                        : xTools->describeCommand(aToolId, rMode, Utf8(rToolchain.aVersion),
                                                  comphelper::containerToSequence(aArguments));
        for (const tool::CommandCategory& rCategory : aCommand.Categories)
        {
            Rows aRows;
            for (const tool::CommandProperty& rProperty : rCategory.Properties)
            {
                std::vector<OUString> aValues(rProperty.Values.begin(), rProperty.Values.end());
                OUString aText = Joined(aValues);
                switch (rProperty.State)
                {
                    case tool::CommandPropertyState_NOT_IN_MODE:
                        aText += " (no effect in mode " + rProperty.Detail + ")";
                        break;
                    case tool::CommandPropertyState_NOT_YET:
                        aText += " (needs version " + rProperty.Detail + ")";
                        break;
                    case tool::CommandPropertyState_REMOVED:
                        aText += " (removed in version " + rProperty.Detail + ")";
                        break;
                    case tool::CommandPropertyState_DEPRECATED:
                        aText += " (deprecated since " + rProperty.Detail + ")";
                        break;
                    default:
                        break;
                }
                if (rProperty.Overridden)
                {
                    std::vector<OUString> aInherited(rProperty.InheritedValues.begin(), rProperty.InheritedValues.end());
                    if (rProperty.Values.hasElements())
                        aText += aInherited.empty() ? OUString(u"   [set for this file only]"_ustr)
                                                    : "   [overrides the target: " + Joined(aInherited) + "]";
                    else
                        aText.clear(); // nothing set here, and no default: an empty cell, in bold
                }
                // Bold: set here.  For a file that is what departs from its
                // target; for a target, what its command gives.  Normal: the
                // tool's default, or what the parent gave.  No value: an empty cell.
                // which of the choices it has: the value, when it is a single one of them
                sal_Int32 nSelected = -1;
                if (rProperty.Values.getLength() == 1)
                    for (sal_Int32 i = 0; i < rProperty.Choices.getLength(); ++i)
                        if (rProperty.Choices[i] == rProperty.Values[0])
                            nSelected = i;
                ProjectProperty aRow(rProperty.Name, aText, rProperty.Description,
                                     pDefaults ? bool(rProperty.Overridden) : !rProperty.IsDefault,
                                     rProperty.Choices, nSelected, false);
                aRows.push_back(std::move(aRow));
            }
            AddPage(rPages, rRoot + "/" + rCategory.Name, aRows);
        }
        Rows aOther;
        AddRow(aOther, u"Other Options"_ustr,
               Joined(std::vector<OUString>(aCommand.OtherArguments.begin(), aCommand.OtherArguments.end()), u" "));
        AddPage(rPages, rRoot + "/Other", aOther);
        return true;
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code.cmake", "cannot describe a command: " << rException.Message);
        return false;
    }
}

// "C" for a C source, else "CXX": what cmake calls the languages
std::string LanguageOfFile(const OUString& rUrl)
{
    return rUrl.endsWith(".c") ? "C" : "CXX";
}

// The arguments cmake gives the compiler for a compile group of a target, from
// its own reply: flags, definitions and include directories as the options
// they stand for.  This does not depend on a compile_commands.json, which a
// project may or may not export.
// how many of the target's sources a compile group has
size_t SourcesOfGroup(const pt::ptree& rGroup)
{
    const auto oIndexes = rGroup.get_child_optional("sourceIndexes");
    return oIndexes ? oIndexes->size() : 0;
}

// the group most of the target's files of a language are compiled with
std::map<std::string, const pt::ptree*> DefaultGroupsOf(const pt::ptree& rTarget)
{
    std::map<std::string, const pt::ptree*> aDefaults;
    if (auto oGroups = rTarget.get_child_optional("compileGroups"))
        for (const auto& rGroup : *oGroups)
        {
            const std::string aLanguage = rGroup.second.get<std::string>("language", "");
            const auto it = aDefaults.find(aLanguage);
            if (it == aDefaults.end() || SourcesOfGroup(rGroup.second) > SourcesOfGroup(*it->second))
                aDefaults[aLanguage] = &rGroup.second;
        }
    return aDefaults;
}

std::vector<std::string> CompileGroupArguments(const pt::ptree& rGroup)
{
    std::vector<std::string> aArguments;
    for (const OUString& rFragment : StringsOf(rGroup, "compileCommandFragments", "fragment"))
        AddWords(aArguments, rFragment.toUtf8().getStr());
    for (const OUString& rDefine : StringsOf(rGroup, "defines", "define"))
        aArguments.push_back("-D" + std::string(rDefine.toUtf8().getStr()));
    if (auto oIncludes = rGroup.get_child_optional("includes"))
        for (const auto& rInclude : *oIncludes)
        {
            aArguments.push_back(rInclude.second.get<bool>("isSystem", false) ? "-isystem" : "-I");
            aArguments.push_back(rInclude.second.get<std::string>("path", ""));
        }
    return aArguments;
}

Pages TargetPages(const uno::Reference<uno::XComponentContext>& xContext, const pt::ptree& rTarget, const std::string& rSourceRoot, const std::string& rBuildRoot,
                  const Toolchains& rToolchains)
{
    Pages aPages;
    Rows aGeneral;
    AddRow(aGeneral, u"Name"_ustr, Utf8(rTarget.get<std::string>("name", "")));
    AddRow(aGeneral, u"Type"_ustr, Utf8(rTarget.get<std::string>("type", "")));
    AddRow(aGeneral, u"Source Directory"_ustr,
           Utf8(NormalizePath(rSourceRoot + "/" + rTarget.get<std::string>("paths.source", "."))));
    AddRow(aGeneral, u"Build Directory"_ustr,
           Utf8(NormalizePath(rBuildRoot + "/" + rTarget.get<std::string>("paths.build", "."))));
    AddRow(aGeneral, u"Name On Disk"_ustr, Utf8(rTarget.get<std::string>("nameOnDisk", "")));
    std::vector<OUString> aOutputs;
    for (const OUString& rPath : StringsOf(rTarget, "artifacts", "path"))
        aOutputs.push_back(Utf8(NormalizePath(rBuildRoot + "/" + rPath.toUtf8().getStr())));
    AddRow(aGeneral, u"Output Files"_ustr, Joined(aOutputs));
    AddRow(aGeneral, u"Folder"_ustr, Utf8(rTarget.get<std::string>("folder.name", "")));
    std::vector<OUString> aDependencies;
    if (auto oDependencies = rTarget.get_child_optional("dependencies"))
        for (const auto& rDependency : *oDependencies)
            aDependencies.push_back(TargetNameOfId(rDependency.second.get<std::string>("id", "")));
    AddRow(aGeneral, u"Dependencies"_ustr, Joined(aDependencies));
    AddPage(aPages, u"General"_ustr, aGeneral);

    // one set of rows per language the target is compiled in; when it is
    // compiled in more than one, the rows say which
    Rows aIncludes, aPreprocessor, aCompiler;
    if (auto oGroups = rTarget.get_child_optional("compileGroups"))
    {
        const bool bSeveral = oGroups->size() > 1;
        for (const auto& rGroup : *oGroups)
        {
            const std::string aLanguage = rGroup.second.get<std::string>("language", "");
            const OUString aSuffix = bSeveral ? " (" + Utf8(aLanguage) + ")" : OUString();
            std::vector<OUString> aUser, aSystem;
            if (auto oIncludes = rGroup.second.get_child_optional("includes"))
                for (const auto& rInclude : *oIncludes)
                    (rInclude.second.get<bool>("isSystem", false) ? aSystem : aUser)
                        .push_back(Utf8(rInclude.second.get<std::string>("path", "")));
            AddRow(aIncludes, "Include Directories" + aSuffix, Joined(aUser));
            AddRow(aIncludes, "System Include Directories" + aSuffix, Joined(aSystem));
            AddRow(aPreprocessor, "Preprocessor Definitions" + aSuffix,
                   Joined(StringsOf(rGroup.second, "defines", "define")));
            AddRow(aCompiler, "Language" + aSuffix, Utf8(aLanguage));
            AddRow(aCompiler, "Language Standard" + aSuffix,
                   Utf8(rGroup.second.get<std::string>("languageStandard.standard", "")));
            AddRow(aCompiler, "Compiler Flags" + aSuffix,
                   Joined(StringsOf(rGroup.second, "compileCommandFragments", "fragment"), u" "));
        }
    }
    // with a description of the compiler, the options are shown by its
    // categories; the include directories and definitions stay as cmake
    // gave them (they are what it knows best)
    bool bDescribed = false;
    // The target's page is what its files are compiled with unless they say
    // otherwise: per language the group most of them are in.  A file in
    // another group is shown against it.
    const std::map<std::string, const pt::ptree*> aDefaultGroups = DefaultGroupsOf(rTarget);
    for (const auto& rDefault : aDefaultGroups)
        {
            const std::string& aLanguage = rDefault.first;
            const pt::ptree& rGroupTree = *rDefault.second;
            const auto itToolchain = rToolchains.find(aLanguage);
            if (itToolchain == rToolchains.end())
                continue;
            const std::vector<std::string> aArguments = CompileGroupArguments(rGroupTree);
            if (CommandPages(xContext, itToolchain->second, u"Compile"_ustr, aArguments,
                             "Compiler (" + Utf8(aLanguage) + ")", aPages))
                bDescribed = true;
        }
    if (!bDescribed)
    {
        AddPage(aPages, u"C/C++/General"_ustr, aIncludes);
        AddPage(aPages, u"C/C++/Preprocessor"_ustr, aPreprocessor);
        AddPage(aPages, u"C/C++/Command Line"_ustr, aCompiler);
    }

    // what the linker gets, by the role cmake gave each piece
    std::vector<OUString> aLibraries, aLibraryPaths, aFlags;
    if (auto oFragments = rTarget.get_child_optional("link.commandFragments"))
        for (const auto& rFragment : *oFragments)
        {
            const std::string aRole = rFragment.second.get<std::string>("role", "");
            const OUString aText = Utf8(rFragment.second.get<std::string>("fragment", ""));
            if (aRole == "libraries")
                aLibraries.push_back(aText);
            else if (aRole == "libraryPath" || aRole == "frameworkPath")
                aLibraryPaths.push_back(aText);
            else
                aFlags.push_back(aText);
        }
    Rows aLinkerInput, aLinkerCommand;
    AddRow(aLinkerInput, u"Libraries"_ustr, Joined(aLibraries));
    AddRow(aLinkerInput, u"Library Directories"_ustr, Joined(aLibraryPaths));
    AddRow(aLinkerCommand, u"Linker Language"_ustr, Utf8(rTarget.get<std::string>("link.language", "")));
    AddRow(aLinkerCommand, u"Linker Flags"_ustr, Joined(aFlags, u" "));
    // the link command is the compiler's, in its Link mode
    const auto itLinker = rToolchains.find(rTarget.get<std::string>("link.language", ""));
    std::vector<std::string> aLinkArguments;
    if (auto oFragments = rTarget.get_child_optional("link.commandFragments"))
        for (const auto& rFragment : *oFragments)
            AddWords(aLinkArguments, rFragment.second.get<std::string>("fragment", ""));
    if (itLinker == rToolchains.end()
        || !CommandPages(xContext, itLinker->second, u"Link"_ustr, aLinkArguments, u"Linker"_ustr, aPages))
    {
        AddPage(aPages, u"Linker/Input"_ustr, aLinkerInput);
        AddPage(aPages, u"Linker/Command Line"_ustr, aLinkerCommand);
    }
    return aPages;
}

// the options of one compile command, sorted into the rows of a page
struct CompileOptions
{
    std::vector<OUString> aIncludes, aSystemIncludes, aDefines, aOthers;
    OUString aStandard, aWorkingDirectory;
};

CompileOptions ParseCompileArguments(const uno::Sequence<OUString>& rArguments)
{
    CompileOptions aOptions;
    for (sal_Int32 i = 0; i < rArguments.getLength(); ++i)
    {
        const OUString& rArgument = rArguments[i];
        // "-I dir" and "-Idir" both
        auto valueOf = [&](std::u16string_view aFlag, OUString& rValue)
        {
            if (!rArgument.startsWith(aFlag))
                return false;
            if (rArgument.getLength() > static_cast<sal_Int32>(aFlag.size()))
                rValue = rArgument.copy(aFlag.size());
            else if (i + 1 < rArguments.getLength())
                rValue = rArguments[++i];
            return true;
        };
        OUString aValue;
        if (rArgument.startsWith("-working-directory="))
            aOptions.aWorkingDirectory = rArgument.copy(19);
        else if (rArgument.startsWith("-std="))
            aOptions.aStandard = rArgument.copy(5);
        else if (valueOf(u"-isystem", aValue))
            aOptions.aSystemIncludes.push_back(aValue);
        else if (valueOf(u"-I", aValue))
            aOptions.aIncludes.push_back(aValue);
        else if (valueOf(u"-D", aValue))
            aOptions.aDefines.push_back(aValue);
        else
            aOptions.aOthers.push_back(rArgument);
    }
    return aOptions;
}

// what cmake says of how a source file is compiled
struct CompileInfo
{
    std::string aLanguage;
    std::vector<std::string> aArguments;
    /// what most of the target's files of this language are compiled with
    std::vector<std::string> aTargetDefaults;
    OUString aWorkingDirectory; // where the compiler runs: the target's build directory
};

Pages FilePages(const uno::Reference<uno::XComponentContext>& xContext, const OUString& rUrl,
                const OUString& rMimeType, bool bGenerated, const OUString& rTarget,
                const std::shared_ptr<const Toolchains>& xToolchains,
                const std::shared_ptr<const CompileInfo>& xCompile)
{
    Pages aPages;
    Rows aGeneral;
    AddRow(aGeneral, u"Name"_ustr, codeservices::GetDisplayName(rUrl));
    AddRow(aGeneral, u"Full Path"_ustr, SystemPathOf(rUrl));
    AddRow(aGeneral, u"Target"_ustr, rTarget);
    AddRow(aGeneral, u"Content Type"_ustr, rMimeType);
    AddRow(aGeneral, u"Generated"_ustr, bGenerated ? u"Yes"_ustr : u"No"_ustr);
    AddRow(aGeneral, u"Compiled"_ustr, xCompile ? u"Yes"_ustr : u"No"_ustr);
    AddPage(aPages, u"General"_ustr, aGeneral);
    if (!xCompile)
        return aPages;

    // by the categories of the compiler's description
    if (xToolchains)
    {
        const auto itToolchain = xToolchains->find(xCompile->aLanguage);
        if (itToolchain != xToolchains->end()
            && CommandPages(xContext, itToolchain->second, u"Compile"_ustr, xCompile->aArguments, u"Compiler"_ustr,
                            aPages, &xCompile->aTargetDefaults))
        {
            Rows aDirectory;
            AddRow(aDirectory, u"Working Directory"_ustr, xCompile->aWorkingDirectory);
            AddPage(aPages, u"Compiler/General"_ustr, aDirectory);
            return aPages;
        }
    }

    // no description of the compiler: the plain list
    std::vector<OUString> aArguments;
    for (const std::string& rArgument : xCompile->aArguments)
        aArguments.push_back(Utf8(rArgument));
    const CompileOptions aOptions = ParseCompileArguments(comphelper::containerToSequence(aArguments));
    Rows aIncludes, aPreprocessor, aCommand;
    AddRow(aIncludes, u"Include Directories"_ustr, Joined(aOptions.aIncludes));
    AddRow(aIncludes, u"System Include Directories"_ustr, Joined(aOptions.aSystemIncludes));
    AddRow(aPreprocessor, u"Preprocessor Definitions"_ustr, Joined(aOptions.aDefines));
    AddRow(aCommand, u"Language Standard"_ustr, aOptions.aStandard);
    AddRow(aCommand, u"Other Options"_ustr, Joined(aOptions.aOthers, u" "));
    AddRow(aCommand, u"Working Directory"_ustr, xCompile->aWorkingDirectory);
    AddPage(aPages, u"C/C++/General"_ustr, aIncludes);
    AddPage(aPages, u"C/C++/Preprocessor"_ustr, aPreprocessor);
    AddPage(aPages, u"C/C++/Command Line"_ustr, aCommand);
    return aPages;
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

// ---- the stored launch properties ------------------------------------------
//
// An executable target can be run (debug::XLaunchable).  How is stored in the
// current configuration of the project, under the keys startuptarget.hxx
// spells (Debug and Release run with different paths and arguments), and
// shown as the editable "Debugging" page.

// the properties of the configuration builds go to just now; empty when the
// project has none
std::map<OUString, OUString>
CurrentConfigurationProperties(const uno::Reference<uno::XComponentContext>& xContext,
                               const OUString& rProjectUrl)
{
    std::map<OUString, OUString> aProperties;
    try
    {
        auto xStore = theProjectConfigStore::get(xContext);
        const OUString aCurrent = xStore->getCurrentConfiguration(rProjectUrl);
        for (const ProjectConfiguration& rConfiguration : xStore->getConfigurations(rProjectUrl))
            if (rConfiguration.Name == aCurrent)
                for (const beans::StringPair& rProperty : rConfiguration.Properties)
                    aProperties[rProperty.First] = rProperty.Second;
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code.cmake", "cannot read the configuration store: " << rException.Message);
    }
    return aProperties;
}

// a command line into arguments: split at blanks, with double quotes keeping
// a blank-carrying argument together
std::vector<OUString> SplitCommandLine(const OUString& rLine)
{
    std::vector<OUString> aArguments;
    OUStringBuffer aArgument;
    bool bQuoted = false, bAny = false;
    for (sal_Int32 i = 0; i < rLine.getLength(); ++i)
    {
        const sal_Unicode c = rLine[i];
        if (c == '"')
            bQuoted = !bQuoted, bAny = true;
        else if (!bQuoted && (c == ' ' || c == '\t'))
        {
            if (bAny)
                aArguments.push_back(aArgument.makeStringAndClear());
            bAny = false;
        }
        else
            aArgument.append(c), bAny = true;
    }
    if (bAny)
        aArguments.push_back(aArgument.makeStringAndClear());
    return aArguments;
}

std::vector<OUString> SplitList(const OUString& rText, sal_Unicode cSeparator)
{
    std::vector<OUString> aParts;
    for (sal_Int32 nIndex = 0; nIndex >= 0;)
    {
        const OUString aPart = rText.getToken(0, cSeparator, nIndex).trim();
        if (!aPart.isEmpty())
            aParts.push_back(aPart);
    }
    return aParts;
}

OUString FileUrlOf(const OUString& rPathOrUrl)
{
    if (rPathOrUrl.isEmpty() || rPathOrUrl.startsWith("file:"))
        return rPathOrUrl;
    OUString aUrl;
    return osl::FileBase::getFileURLFromSystemPath(rPathOrUrl, aUrl) == osl::FileBase::E_None
               ? aUrl
               : rPathOrUrl;
}

// A target can be built on its own, in the current configuration of its project.
class TargetNode final
    : public cppu::WeakImplHelper<XProjectItem, XProjectTarget, css::code::build::XBuildable,
                                  XProjectProperties, XProjectPropertyEditor,
                                  css::code::debug::XLaunchable>
{
public:
    TargetNode(uno::Reference<uno::XComponentContext> xContext, OUString aProjectUrl, const OUString& rName,
               TargetType eType, std::vector<OUString> aArtifactPaths)
        : m_xContext(std::move(xContext))
        , m_aProjectUrl(std::move(aProjectUrl))
        , m_eType(eType)
        , m_aArtifactPaths(std::move(aArtifactPaths))
    {
        m_aData.aName = rName;
    }
    TargetType SAL_CALL getType() override { return m_eType; }
    uno::Reference<css::code::core::XBackgroundJob> SAL_CALL build() override
    {
        return StartBuild(m_xContext, m_aProjectUrl, m_aData.aName, BuildMode::Build);
    }
    uno::Reference<css::code::core::XBackgroundJob> SAL_CALL rebuild() override
    {
        return StartBuild(m_xContext, m_aProjectUrl, m_aData.aName, BuildMode::Rebuild);
    }
    uno::Reference<css::code::core::XBackgroundJob> SAL_CALL clean() override
    {
        return StartBuild(m_xContext, m_aProjectUrl, m_aData.aName, BuildMode::Clean);
    }
    uno::Sequence<ProjectPropertyPage> SAL_CALL getPropertyPages() override;
    void SAL_CALL setProperty(const OUString& rPagePath, const OUString& rName,
                              const OUString& rValue) override;
    sal_Bool SAL_CALL getLaunchable() override { return m_eType == TargetType_EXECUTABLE; }
    css::code::debug::LaunchDescriptor SAL_CALL createLaunchDescriptor() override;
    CMAKEPROJ_ITEM_METHODS(ItemKind_TARGET)

public:
    // read together with the target, so that asking costs nothing
    Pages m_aPages;

    const uno::Reference<uno::XComponentContext> m_xContext;
    const OUString m_aProjectUrl;
    const TargetType m_eType;
    // what the build makes, as system paths; the first is what runs
    const std::vector<OUString> m_aArtifactPaths;

private:
    // the stored launch values, with the old configuration-wide keys the
    // Debug panel used to write as a read-only fallback
    struct LaunchValues
    {
        OUString aCommand, aArguments, aWorkingDirectory, aEnvironment;
        bool bCommandStored = false, bArgumentsStored = false, bWorkingDirectoryStored = false,
             bEnvironmentStored = false;
    };
    LaunchValues ReadLaunchValues() const;
};

TargetNode::LaunchValues TargetNode::ReadLaunchValues() const
{
    const std::map<OUString, OUString> aStored
        = CurrentConfigurationProperties(m_xContext, m_aProjectUrl);
    auto read = [&](const OUString& rKey, const OUString& rLegacyKey, OUString& rValue, bool& rbStored)
    {
        if (const auto it = aStored.find(rKey); it != aStored.end() && !it->second.isEmpty())
        {
            rValue = it->second;
            rbStored = true;
        }
        else if (const auto itLegacy = aStored.find(rLegacyKey); itLegacy != aStored.end())
            rValue = itLegacy->second; // the old key counts as a default, not as set
    };
    const OUString& rName = m_aData.aName;
    LaunchValues aValues;
    read(::code::startup::CommandKey(rName), u"DebugProgram"_ustr, aValues.aCommand,
         aValues.bCommandStored);
    read(::code::startup::ArgumentsKey(rName), u"DebugArguments"_ustr, aValues.aArguments,
         aValues.bArgumentsStored);
    read(::code::startup::WorkingDirectoryKey(rName), u"DebugWorkingDirectory"_ustr,
         aValues.aWorkingDirectory, aValues.bWorkingDirectoryStored);
    read(::code::startup::EnvironmentKey(rName), u""_ustr, aValues.aEnvironment,
         aValues.bEnvironmentStored);
    return aValues;
}

uno::Sequence<ProjectPropertyPage> TargetNode::getPropertyPages()
{
    Pages aPages = m_aPages;
    // The CMake pages are a frozen snapshot; the Debugging page is computed
    // on every call, so the dialog re-reads fresh values right after a write.
    if (getLaunchable())
    {
        const LaunchValues aValues = ReadLaunchValues();
        const OUString aArtifact = m_aArtifactPaths.empty() ? OUString() : m_aArtifactPaths.front();
        const OUString aArtifactDirectory
            = aArtifact.copy(0, std::max<sal_Int32>(aArtifact.lastIndexOf('/'), 0));
        Rows aRows;
        auto add = [&aRows](const OUString& rName, const OUString& rValue, bool bStored,
                            const OUString& rDescription)
        {
            // rows stay even when empty: an empty cell the user can fill
            aRows.emplace_back(rName, rValue, rDescription, bStored, uno::Sequence<OUString>(),
                               -1, true);
        };
        add(u"Command"_ustr, aValues.aCommand.isEmpty() ? aArtifact : aValues.aCommand,
            aValues.bCommandStored,
            u"The program Start runs; the target's built file when not set. Stored in the current configuration; an empty value brings the default back."_ustr);
        add(u"Command Arguments"_ustr, aValues.aArguments, aValues.bArgumentsStored,
            u"The command line the program gets, quoted as in a shell. Stored in the current configuration."_ustr);
        add(u"Working Directory"_ustr,
            aValues.aWorkingDirectory.isEmpty() ? aArtifactDirectory : aValues.aWorkingDirectory,
            aValues.bWorkingDirectoryStored,
            u"Where the program runs; its own directory when not set. Stored in the current configuration."_ustr);
        add(u"Environment"_ustr, aValues.aEnvironment, aValues.bEnvironmentStored,
            u"NAME=value entries added to the program's environment, separated by \";\". Stored in the current configuration."_ustr);
        AddPage(aPages, u"Debugging"_ustr, aRows);
    }
    return comphelper::containerToSequence(aPages);
}

void TargetNode::setProperty(const OUString& rPagePath, const OUString& rName,
                             const OUString& rValue)
{
    if (rPagePath != "Debugging")
        throw lang::IllegalArgumentException("the page is not editable: " + rPagePath, getXWeak(),
                                             0);
    OUString aKey;
    if (rName == "Command")
        aKey = ::code::startup::CommandKey(m_aData.aName);
    else if (rName == "Command Arguments")
        aKey = ::code::startup::ArgumentsKey(m_aData.aName);
    else if (rName == "Working Directory")
        aKey = ::code::startup::WorkingDirectoryKey(m_aData.aName);
    else if (rName == "Environment")
        aKey = ::code::startup::EnvironmentKey(m_aData.aName);
    else
        throw lang::IllegalArgumentException("no editable property named " + rName, getXWeak(), 1);

    auto xStore = theProjectConfigStore::get(m_xContext);
    const OUString aCurrent = xStore->getCurrentConfiguration(m_aProjectUrl);
    if (aCurrent.isEmpty())
        throw lang::IllegalArgumentException(
            u"the project has no configuration to keep the value in"_ustr, getXWeak(), 0);
    auto aConfigurations = comphelper::sequenceToContainer<std::vector<ProjectConfiguration>>(
        xStore->getConfigurations(m_aProjectUrl));
    for (ProjectConfiguration& rConfiguration : aConfigurations)
    {
        if (rConfiguration.Name != aCurrent)
            continue;
        auto aProperties = comphelper::sequenceToContainer<std::vector<beans::StringPair>>(
            rConfiguration.Properties);
        std::erase_if(aProperties,
                      [&](const beans::StringPair& r) { return r.First == aKey; });
        if (!rValue.isEmpty()) // empty forgets the value: the default returns
            aProperties.push_back({ aKey, rValue });
        rConfiguration.Properties = comphelper::containerToSequence(aProperties);
    }
    xStore->setConfigurations(m_aProjectUrl, comphelper::containerToSequence(aConfigurations));
}

css::code::debug::LaunchDescriptor TargetNode::createLaunchDescriptor()
{
    const LaunchValues aValues = ReadLaunchValues();
    css::code::debug::LaunchDescriptor aDescriptor;
    aDescriptor.Program = aValues.aCommand.isEmpty()
                              ? (m_aArtifactPaths.empty() ? OUString() : m_aArtifactPaths.front())
                              : aValues.aCommand;
    aDescriptor.Arguments = comphelper::containerToSequence(SplitCommandLine(aValues.aArguments));
    OUString aWorkingDirectory = aValues.aWorkingDirectory;
    if (aWorkingDirectory.isEmpty() && !m_aArtifactPaths.empty())
    {
        const OUString& rArtifact = m_aArtifactPaths.front();
        aWorkingDirectory = rArtifact.copy(0, std::max<sal_Int32>(rArtifact.lastIndexOf('/'), 0));
    }
    aDescriptor.WorkingDirectory = FileUrlOf(aWorkingDirectory);
    aDescriptor.Environment = comphelper::containerToSequence(SplitList(aValues.aEnvironment, ';'));
    // stopping at the entry is the debug session's way of starting, not a
    // property of the target: the caller sets it
    aDescriptor.StopAtEntry = false;
    return aDescriptor;
}

class GroupNode final : public cppu::WeakImplHelper<XSourceGroup>
{
public:
    explicit GroupNode(const OUString& rName) { m_aData.aName = rName; }
    OUString SAL_CALL getGroupPath() override { return m_aData.aName; }
    OUString SAL_CALL getFilter() override { return {}; }
    CMAKEPROJ_ITEM_METHODS(ItemKind_GROUP)
};

class FileNode final : public cppu::WeakImplHelper<XProjectFile, XProjectProperties>
{
public:
    FileNode(uno::Reference<uno::XComponentContext> xContext, const OUString& rName, const OUString& rUrl,
             bool bGenerated)
        : m_xContext(std::move(xContext))
        , m_aMime(codeservices::GetMimeType(rUrl))
        , m_bGenerated(bGenerated)
    {
        m_aData.aName = rName;
        m_aData.aUrl = rUrl;
    }
    OUString SAL_CALL getRelativePath() override { return m_aData.aName; }
    OUString SAL_CALL getMimeType() override { return m_aMime; }
    sal_Bool SAL_CALL getGenerated() override { return m_bGenerated; }
    uno::Sequence<ProjectPropertyPage> SAL_CALL getPropertyPages() override
    {
        return comphelper::containerToSequence(
            FilePages(m_xContext, m_aData.aUrl, m_aMime, m_bGenerated, OUString(), nullptr, nullptr));
    }
    CMAKEPROJ_ITEM_METHODS(ItemKind_FILE)

    const uno::Reference<uno::XComponentContext> m_xContext;
    const OUString m_aMime;
    const bool m_bGenerated;
};

// A source file the target compiles: it can be compiled on its own.  A file the
// compiler never sees (a header, a CMake file) is a FileNode, and says nothing
// about compiling.
class CompilableFileNode final
    : public cppu::WeakImplHelper<XProjectFile, css::code::build::XCompilable, XProjectProperties>
{
public:
    CompilableFileNode(uno::Reference<uno::XComponentContext> xContext, OUString aProjectUrl, OUString aTarget,
                       const OUString& rName, const OUString& rUrl, bool bGenerated,
                       std::shared_ptr<const Toolchains> xToolchains, std::shared_ptr<const CompileInfo> xCompile)
        : m_xContext(std::move(xContext))
        , m_xToolchains(std::move(xToolchains))
        , m_xCompile(std::move(xCompile))
        , m_aProjectUrl(std::move(aProjectUrl))
        , m_aTarget(std::move(aTarget))
        , m_aMime(codeservices::GetMimeType(rUrl))
        , m_bGenerated(bGenerated)
    {
        m_aData.aName = rName;
        m_aData.aUrl = rUrl;
    }
    OUString SAL_CALL getRelativePath() override { return m_aData.aName; }
    OUString SAL_CALL getMimeType() override { return m_aMime; }
    sal_Bool SAL_CALL getGenerated() override { return m_bGenerated; }
    uno::Reference<css::code::core::XBackgroundJob> SAL_CALL compile() override
    {
        return StartCompile(m_xContext, m_aProjectUrl, m_aTarget, m_aData.aUrl);
    }
    uno::Sequence<ProjectPropertyPage> SAL_CALL getPropertyPages() override
    {
        return comphelper::containerToSequence(
            FilePages(m_xContext, m_aData.aUrl, m_aMime, m_bGenerated, m_aTarget, m_xToolchains, m_xCompile));
    }
    CMAKEPROJ_ITEM_METHODS(ItemKind_FILE)

    const uno::Reference<uno::XComponentContext> m_xContext;
    const std::shared_ptr<const Toolchains> m_xToolchains;
    const std::shared_ptr<const CompileInfo> m_xCompile;
    const OUString m_aProjectUrl;
    const OUString m_aTarget;
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
Structure ReadStructure(const uno::Reference<uno::XComponentContext>& xContext, const OUString& rProjectUrl,
                        const OUString& rBuildUrl)
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

        // the compilers, by language: what a description is chosen by
        auto xToolchains = std::make_shared<Toolchains>();
        pt::ptree aToolchainReply;
        if (ReadReplyObject(aReplyUrl, aIndex, "toolchains", aToolchainReply))
            if (auto oToolchains = aToolchainReply.get_child_optional("toolchains"))
                for (const auto& rToolchain : *oToolchains)
                    (*xToolchains)[rToolchain.second.get<std::string>("language", "")]
                        = { rToolchain.second.get<std::string>("compiler.path", ""),
                            rToolchain.second.get<std::string>("compiler.version", "") };

        const std::string aSourceRoot = aModel.get<std::string>("paths.source");
        const std::string aBuildRoot = aModel.get<std::string>("paths.build", "");
        // A multi-configuration generator reports each configuration; the
        // targets are the same in all of them, so the first stands for them.
        const pt::ptree& rConfiguration = aModel.get_child("configurations").begin()->second;
        for (const auto& rTargetRef : rConfiguration.get_child("targets"))
        {
            pt::ptree aTarget;
            if (!ReadJson(aReplyUrl + "/" + OUString::fromUtf8(rTargetRef.second.get<std::string>("jsonFile")), aTarget))
                continue;

            // what the build makes, as the "Output Files" row shows it; the
            // first artifact is what XLaunchable runs
            std::vector<OUString> aArtifacts;
            for (const OUString& rPath : StringsOf(aTarget, "artifacts", "path"))
                aArtifacts.push_back(Utf8(NormalizePath(aBuildRoot + "/" + rPath.toUtf8().getStr())));

            rtl::Reference<TargetNode> xTarget = new TargetNode(
                xContext, rProjectUrl, OUString::fromUtf8(aTarget.get<std::string>("name", "")),
                TargetTypeOf(aTarget.get<std::string>("type", "")), std::move(aArtifacts));

            xTarget->m_aPages = TargetPages(xContext, aTarget, aSourceRoot, aBuildRoot, *xToolchains);

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

            const std::map<std::string, const pt::ptree*> aDefaultGroups = DefaultGroupsOf(aTarget);
            // what each compile group of the target says of how its files are compiled
            std::vector<std::shared_ptr<const CompileInfo>> aCompileInfos;
            if (auto oGroups = aTarget.get_child_optional("compileGroups"))
                for (const auto& rGroup : *oGroups)
                {
                    auto xInfo = std::make_shared<CompileInfo>();
                    xInfo->aLanguage = rGroup.second.get<std::string>("language", "");
                    xInfo->aArguments = CompileGroupArguments(rGroup.second);
                    const auto itDefault = aDefaultGroups.find(xInfo->aLanguage);
                    if (itDefault != aDefaultGroups.end())
                        xInfo->aTargetDefaults = CompileGroupArguments(*itDefault->second);
                    OUString aDirectory = FileUrlOf(aBuildRoot, aTarget.get<std::string>("paths.build", "."));
                    xInfo->aWorkingDirectory = SystemPathOf(aDirectory);
                    aCompileInfos.push_back(std::move(xInfo));
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
                    const OUString aName = codeservices::GetDisplayName(aUrl);
                    const bool bGenerated = rSource.second.get<bool>("isGenerated", false);
                    // cmake says which sources the compiler gets: they have a compile group
                    ItemData* pData;
                    uno::Reference<XProjectItem> xItem;
                    if (auto oGroupIndex = rSource.second.get_optional<size_t>("compileGroupIndex"))
                    {
                        auto* pFile = new CompilableFileNode(xContext, rProjectUrl, xTarget->getName(), aName, aUrl, bGenerated,
                                                             xToolchains,
                                                             *oGroupIndex < aCompileInfos.size() ? aCompileInfos[*oGroupIndex]
                                                                                                  : std::make_shared<const CompileInfo>());
                        xItem = pFile;
                        pData = &pFile->m_aData;
                    }
                    else
                    {
                        auto* pFile = new FileNode(xContext, aName, aUrl, bGenerated);
                        xItem = pFile;
                        pData = &pFile->m_aData;
                    }
                    pData->xParent = uno::Reference<XProjectItem>(xGroup.get());
                    xGroup->m_aData.aChildren.push_back(xItem);
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
                                                 XProjectConfigurationState, util::XModifyBroadcaster,
                                                 css::code::build::XBuildable, XProjectProperties>
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

    // XBuildable: the whole project
    uno::Reference<css::code::core::XBackgroundJob> SAL_CALL build() override
    {
        return StartBuild(m_xContext, m_aUrl, OUString(), BuildMode::Build);
    }
    uno::Reference<css::code::core::XBackgroundJob> SAL_CALL rebuild() override
    {
        return StartBuild(m_xContext, m_aUrl, OUString(), BuildMode::Rebuild);
    }
    uno::Reference<css::code::core::XBackgroundJob> SAL_CALL clean() override
    {
        return StartBuild(m_xContext, m_aUrl, OUString(), BuildMode::Clean);
    }

    // XProjectProperties
    uno::Sequence<ProjectPropertyPage> SAL_CALL getPropertyPages() override
    {
        return comphelper::containerToSequence(ProjectPages());
    }

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
        ProjectConfiguration aConfiguration;
        const OUString aBuildUrl = CurrentConfiguration(aConfiguration) ? BuildDirectoryOf(aConfiguration) : OUString();
        Structure aStructure = aBuildUrl.isEmpty() ? Structure() : ReadStructure(m_xContext, m_aUrl, aBuildUrl);

        std::unique_lock aGuard(m_aMutex);
        m_aStructure = std::move(aStructure);
        m_bLoaded = true;
        return m_aStructure;
    }

    // The configuration the project is built in now; each configuration has a
    // build tree of its own, and CMake's reply lives in it.
    bool CurrentConfiguration(ProjectConfiguration& rConfiguration)
    {
        try
        {
            auto xStore = theProjectConfigStore::get(m_xContext);
            const OUString aCurrent = xStore->getCurrentConfiguration(m_aUrl);
            for (const ProjectConfiguration& rCandidate : xStore->getConfigurations(m_aUrl))
                if (rCandidate.Name == aCurrent)
                {
                    rConfiguration = rCandidate;
                    return true;
                }
        }
        catch (const uno::Exception&)
        {
        }
        return false;
    }

    static OUString BuildDirectoryOf(const ProjectConfiguration& rConfiguration)
    {
        for (const beans::StringPair& rProperty : rConfiguration.Properties)
            if (rProperty.First == "BuildDirectory")
                return rProperty.Second;
        return {};
    }

    // the pages of the properties dialog: what the project and its current
    // configuration are, and what cmake says of itself in its replies
    Pages ProjectPages()
    {
        Pages aPages;
        ProjectConfiguration aConfiguration;
        const bool bConfiguration = CurrentConfiguration(aConfiguration);

        Rows aGeneral;
        AddRow(aGeneral, u"Name"_ustr, getName());
        AddRow(aGeneral, u"Project File"_ustr, SystemPathOf(m_aUrl));
        AddRow(aGeneral, u"Source Directory"_ustr, SystemPathOf(codeservices::GetParentUrl(m_aUrl)));
        AddRow(aGeneral, u"Configuration"_ustr, bConfiguration ? aConfiguration.Name : OUString());
        AddPage(aPages, u"General"_ustr, aGeneral);

        Rows aSettings;
        if (bConfiguration)
            for (const beans::StringPair& rProperty : aConfiguration.Properties)
            {
                // the store keeps CamelCase names and file URLs; the dialog shows words and paths
                OUStringBuffer aName;
                for (sal_Int32 i = 0; i < rProperty.First.getLength(); ++i)
                {
                    if (i > 0 && rtl::isAsciiUpperCase(rProperty.First[i]))
                        aName.append(' ');
                    aName.append(rProperty.First[i]);
                }
                AddRow(aSettings, aName.makeStringAndClear(),
                       rProperty.Second.startsWith("file:") ? SystemPathOf(rProperty.Second) : rProperty.Second);
            }
        AddPage(aPages, u"Configuration"_ustr, aSettings);

        const OUString aReplyUrl = bConfiguration ? ReplyDirectoryOf(BuildDirectoryOf(aConfiguration)) : OUString();
        const OUString aIndexUrl = aReplyUrl.isEmpty() ? OUString() : NewestIndexOf(aReplyUrl);
        pt::ptree aIndex;
        if (aIndexUrl.isEmpty() || !ReadJson(aIndexUrl, aIndex))
            return aPages;

        Rows aCMake;
        AddRow(aCMake, u"Version"_ustr, Utf8(aIndex.get<std::string>("cmake.version.string", "")));
        AddRow(aCMake, u"Executable"_ustr, Utf8(aIndex.get<std::string>("cmake.paths.cmake", "")));
        AddRow(aCMake, u"Generator"_ustr, Utf8(aIndex.get<std::string>("cmake.generator.name", "")));
        AddRow(aCMake, u"Multi-Configuration Generator"_ustr,
               aIndex.get<bool>("cmake.generator.multiConfig", false) ? u"Yes"_ustr : u"No"_ustr);
        pt::ptree aModel;
        if (ReadReplyObject(aReplyUrl, aIndex, "codemodel", aModel))
        {
            AddRow(aCMake, u"Source Root"_ustr, Utf8(aModel.get<std::string>("paths.source", "")));
            AddRow(aCMake, u"Build Root"_ustr, Utf8(aModel.get<std::string>("paths.build", "")));
        }
        AddPage(aPages, u"CMake/General"_ustr, aCMake);

        // the variables a user sets, not the ones cmake keeps for itself
        pt::ptree aCache;
        if (ReadReplyObject(aReplyUrl, aIndex, "cache", aCache))
        {
            Rows aVariables;
            if (auto oEntries = aCache.get_child_optional("entries"))
                for (const auto& rEntry : *oEntries)
                {
                    const std::string aType = rEntry.second.get<std::string>("type", "");
                    if (aType == "INTERNAL" || aType == "STATIC")
                        continue;
                    bool bAdvanced = false;
                    if (auto oProperties = rEntry.second.get_child_optional("properties"))
                        for (const auto& rProperty : *oProperties)
                            if (rProperty.second.get<std::string>("name", "") == "ADVANCED"
                                && rProperty.second.get<std::string>("value", "") == "1")
                                bAdvanced = true;
                    if (!bAdvanced)
                        AddRow(aVariables, Utf8(rEntry.second.get<std::string>("name", "")),
                               Utf8(rEntry.second.get<std::string>("value", "")));
                }
            AddPage(aPages, u"CMake/Cache Variables"_ustr, aVariables);
        }

        pt::ptree aToolchains;
        if (ReadReplyObject(aReplyUrl, aIndex, "toolchains", aToolchains))
        {
            Rows aCompilers;
            if (auto oToolchains = aToolchains.get_child_optional("toolchains"))
                for (const auto& rToolchain : *oToolchains)
                {
                    const OUString aLanguage = Utf8(rToolchain.second.get<std::string>("language", ""));
                    const pt::ptree aNone;
                    const auto oCompiler = rToolchain.second.get_child_optional("compiler");
                    const pt::ptree& rCompiler = oCompiler ? *oCompiler : aNone;
                    AddRow(aCompilers, aLanguage + " Compiler", Utf8(rCompiler.get<std::string>("path", "")));
                    AddRow(aCompilers, aLanguage + " Compiler Id", Utf8(rCompiler.get<std::string>("id", "")));
                    AddRow(aCompilers, aLanguage + " Compiler Version",
                           Utf8(rCompiler.get<std::string>("version", "")));
                    AddRow(aCompilers, aLanguage + " Target", Utf8(rCompiler.get<std::string>("target", "")));
                    AddRow(aCompilers, aLanguage + " Implicit Include Directories",
                           Joined(StringsOf(rCompiler, "implicit.includeDirectories", "")));
                }
            AddPage(aPages, u"Toolchains"_ustr, aCompilers);
        }
        return aPages;
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
    // the code model is the structure; the cache and the toolchains only
    // fill the properties dialog
    for (const OUString& rQuery : { u"codemodel-v2"_ustr, u"cache-v2"_ustr, u"toolchains-v1"_ustr })
    {
        osl::File aFile(aQueryDirectory + "/" + rQuery);
        if (aFile.open(osl_File_OpenFlag_Write | osl_File_OpenFlag_Create | osl_File_OpenFlag_NoExcl)
            == osl::FileBase::E_None)
            aFile.close();
    }
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
