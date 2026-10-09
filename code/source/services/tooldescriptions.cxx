/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// theToolDescriptions: what the options of a tool are, from the descriptions
// that come with Code (share/code/tooldescriptions) -- the one place that
// reads a command line as properties in categories, and writes one back.  The
// format reader and the parsing are tooldescription.{hxx,cxx}, without UNO.

#include "tooldescription.hxx"

#include <com/sun/star/code/tool/CommandCategory.hpp>
#include <com/sun/star/code/tool/CommandDescription.hpp>
#include <com/sun/star/code/tool/CommandItem.hpp>
#include <com/sun/star/code/tool/CommandProperty.hpp>
#include <com/sun/star/code/tool/CommandPropertyState.hpp>
#include <com/sun/star/code/tool/ToolInfo.hpp>
#include <com/sun/star/code/tool/XToolDescriptions.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <osl/file.hxx>
#include <rtl/bootstrap.hxx>
#include <sal/log.hxx>

#include <algorithm>
#include <mutex>
#include <string>
#include <vector>

using namespace css;
using namespace css::code::tool;

namespace codeservices
{
namespace
{
namespace td = tooldesc;

OUString Utf8(const std::string& rText) { return OUString::fromUtf8(rText); }

std::string ToUtf8(const OUString& rText)
{
    const OString aBytes = OUStringToOString(rText, RTL_TEXTENCODING_UTF8);
    return std::string(aBytes.getStr(), aBytes.getLength());
}

std::vector<std::string> ArgumentsOf(const uno::Sequence<OUString>& rArguments)
{
    std::vector<std::string> aResult;
    aResult.reserve(rArguments.getLength());
    for (const OUString& rArgument : rArguments)
        aResult.push_back(ToUtf8(rArgument));
    return aResult;
}

uno::Sequence<OUString> SequenceOf(const std::vector<std::string>& rStrings)
{
    uno::Sequence<OUString> aResult(rStrings.size());
    std::transform(rStrings.begin(), rStrings.end(), aResult.getArray(), Utf8);
    return aResult;
}

// what a person reads of one value of a property
OUString ValueText(const td::Property& rProperty, const std::string& rValue)
{
    if (rProperty.eKind == td::Kind::Bool)
        return rValue == "true" ? u"Yes"_ustr : u"No"_ustr;
    if (rProperty.eKind == td::Kind::Enum)
        for (const td::EnumValue& rEnum : rProperty.aValues)
            if (rEnum.aSwitch == rValue)
                return Utf8(rEnum.aName) + " (" + Utf8(rValue) + ")";
    return Utf8(rValue);
}

// the elements of a sequence, to change in place
template <typename T> std::pair<T*, T*> MutableRangeOf(uno::Sequence<T>& rSequence)
{
    T* pBegin = rSequence.getArray();
    return { pBegin, pBegin + rSequence.getLength() };
}
template <typename T> struct MutableView
{
    std::pair<T*, T*> aRange;
    T* begin() const { return aRange.first; }
    T* end() const { return aRange.second; }
};
template <typename T> MutableView<T> Mutable(uno::Sequence<T>& rSequence) { return { MutableRangeOf(rSequence) }; }

// How a property is written on the command line, as Visual Studio shows it
// after the description: "-U[value]", "-fno-rtti", "-O0, -O2, -Os".
std::string SpellingOf(const td::Property& rProperty)
{
    switch (rProperty.eKind)
    {
        case td::Kind::Bool:
        {
            std::string aText = rProperty.aOn;
            if (!rProperty.aOff.empty())
                aText += aText.empty() ? rProperty.aOff : ", " + rProperty.aOff;
            return aText;
        }
        case td::Kind::Enum:
        {
            std::string aText;
            for (const td::EnumValue& rValue : rProperty.aValues)
                aText += (aText.empty() ? "" : ", ") + rValue.aSwitch;
            return aText;
        }
        default:
        {
            const char* pValue = rProperty.eKind == td::Kind::Int ? "[number]" : rProperty.bPath ? "[path]" : "[value]";
            return rProperty.aPrefix + (rProperty.eJoiner == td::Joiner::Equals     ? "="
                                        : rProperty.eJoiner == td::Joiner::Separate ? " "
                                                                                    : "")
                   + pValue;
        }
    }
}

// the text under the list of a dialog: what the description says, and the way to write it
OUString DescriptionOf(const td::Property& rProperty)
{
    OUString aText = Utf8(rProperty.aDescription);
    const std::string aSpelling = SpellingOf(rProperty);
    if (!aSpelling.empty())
        aText += (aText.isEmpty() ? OUString() : u" "_ustr) + "(" + Utf8(aSpelling) + ")";
    return aText;
}

class ToolDescriptions final : public comphelper::WeakComponentImplHelper<XToolDescriptions, lang::XServiceInfo>
{
public:
    explicit ToolDescriptions(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    // XToolDescriptions
    uno::Sequence<ToolInfo> SAL_CALL getTools() override;
    OUString SAL_CALL findTool(const OUString& rExecutable) override;
    OUString SAL_CALL detectMode(const OUString& rToolId, const uno::Sequence<OUString>& rArguments) override;
    CommandDescription SAL_CALL describeCommand(const OUString& rToolId, const OUString& rMode,
                                                const OUString& rVersion,
                                                const uno::Sequence<OUString>& rArguments) override;
    CommandDescription SAL_CALL describeOverrides(const OUString& rToolId, const OUString& rMode,
                                                  const OUString& rVersion,
                                                  const uno::Sequence<OUString>& rDefaults,
                                                  const uno::Sequence<OUString>& rArguments) override;
    uno::Sequence<CommandItem> SAL_CALL parseCommand(const OUString& rToolId,
                                                     const uno::Sequence<OUString>& rArguments) override;
    uno::Sequence<OUString> SAL_CALL formatCommand(const OUString& rToolId,
                                                   const uno::Sequence<CommandItem>& rItems) override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override { return u"com.sun.star.comp.code.ToolDescriptions"_ustr; }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override { return cppu::supportsService(this, rName); }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

private:
    /// the tools, read once; the descriptions that build on another have it merged in
    const std::vector<td::Tool>& Tools();
    const td::Tool& ToolOf(const OUString& rToolId);

    uno::Reference<uno::XComponentContext> m_xContext;
    std::once_flag m_aLoaded;
    std::vector<td::Tool> m_aTools;
};

const std::vector<td::Tool>& ToolDescriptions::Tools()
{
    std::call_once(m_aLoaded,
                   [this]
                   {
                       // where the descriptions are installed, unless the process says otherwise
                       // (-env:CodeToolDescriptions=<url>; a test runs without a brand directory)
                       OUString aUrl;
                       rtl::Bootstrap::get(u"CodeToolDescriptions"_ustr, aUrl,
                                           u"$BRAND_BASE_DIR/share/code/tooldescriptions"_ustr);
                       rtl::Bootstrap::expandMacros(aUrl);
                       osl::Directory aDirectory(aUrl);
                       if (aDirectory.open() != osl::FileBase::E_None)
                       {
                           SAL_WARN("code.services", "no tool descriptions in " << aUrl);
                           return;
                       }
                       std::vector<td::Tool> aRead;
                       osl::DirectoryItem aItem;
                       while (aDirectory.getNextItem(aItem) == osl::FileBase::E_None)
                       {
                           osl::FileStatus aStatus(osl_FileStatus_Mask_FileName | osl_FileStatus_Mask_FileURL);
                           if (aItem.getFileStatus(aStatus) != osl::FileBase::E_None
                               || !aStatus.getFileName().endsWith(".xml"))
                               continue;
                           OUString aPath;
                           osl::FileBase::getSystemPathFromFileURL(aStatus.getFileURL(), aPath);
                           td::LoadResult aLoaded = td::LoadFile(ToUtf8(aPath));
                           if (aLoaded.oTool)
                               aRead.push_back(std::move(*aLoaded.oTool));
                           else // a file that cannot be read is said, and the others still work
                               SAL_WARN("code.services", "tool description: " << aLoaded.aError.c_str());
                       }
                       // a description builds on the one it Extends, and that one on its own
                       for (td::Tool& rTool : aRead)
                       {
                           std::vector<std::string> aSeen{ rTool.aId };
                           for (std::string aBase = rTool.aExtends; !aBase.empty();)
                           {
                               const auto it = std::find_if(aRead.begin(), aRead.end(),
                                                            [&](const td::Tool& r) { return r.aId == aBase; });
                               if (it == aRead.end() || std::find(aSeen.begin(), aSeen.end(), aBase) != aSeen.end())
                                   break; // a base that is missing, or a loop
                               aSeen.push_back(aBase);
                               td::Merge(rTool, *it);
                               aBase = it->aExtends;
                           }
                       }
                       m_aTools = std::move(aRead);
                   });
    return m_aTools;
}

const td::Tool& ToolDescriptions::ToolOf(const OUString& rToolId)
{
    const std::string aId = ToUtf8(rToolId);
    for (const td::Tool& rTool : Tools())
        if (rTool.aId == aId)
            return rTool;
    throw lang::IllegalArgumentException("no description for the tool " + rToolId, getXWeak(), 1);
}

uno::Sequence<ToolInfo> ToolDescriptions::getTools()
{
    std::vector<ToolInfo> aInfos;
    for (const td::Tool& rTool : Tools())
        aInfos.push_back({ Utf8(rTool.aId), Utf8(rTool.aName), rTool.nRevision });
    return comphelper::containerToSequence(aInfos);
}

OUString ToolDescriptions::findTool(const OUString& rExecutable)
{
    // The most specific description: one that builds on another matches
    // what the base matches too, and is the better answer.
    const td::Tool* pFound = nullptr;
    const std::string aExecutable = ToUtf8(rExecutable);
    for (const td::Tool& rTool : Tools())
        if (td::Matches(rTool, aExecutable) && (!pFound || !rTool.aExtends.empty()))
            pFound = &rTool;
    return pFound ? Utf8(pFound->aId) : OUString();
}

OUString ToolDescriptions::detectMode(const OUString& rToolId, const uno::Sequence<OUString>& rArguments)
{
    return Utf8(td::DetectMode(ToolOf(rToolId), ArgumentsOf(rArguments)));
}

CommandDescription ToolDescriptions::describeCommand(const OUString& rToolId, const OUString& rMode,
                                                     const OUString& rVersion,
                                                     const uno::Sequence<OUString>& rArguments)
{
    const td::Tool& rTool = ToolOf(rToolId);
    const std::vector<std::string> aArguments = ArgumentsOf(rArguments);
    const std::string aMode = rMode.isEmpty() ? td::DetectMode(rTool, aArguments) : ToUtf8(rMode);
    const std::string aVersion = ToUtf8(rVersion);
    const std::vector<td::Item> aItems = td::ParseCommand(rTool, aArguments);

    CommandDescription aResult;
    aResult.ToolId = Utf8(rTool.aId);
    aResult.ToolName = Utf8(rTool.aName);
    aResult.Mode = Utf8(aMode);
    std::vector<CommandCategory> aCategories;
    for (const td::Category& rCategory : rTool.aCategories)
    {
        std::vector<CommandProperty> aProperties;
        for (const td::Property& rProperty : rCategory.aProperties)
        {
            std::vector<std::string> aValues = td::ValuesOf(aItems, rProperty.aId);
            bool bDefault = false;
            if (aValues.empty())
            {
                // Not given: what the tool does without being told, if it applies here at all
                if (!td::AppliesInMode(rCategory.aModes, aMode) || !td::AppliesInMode(rProperty.aModes, aMode)
                    || td::AvailabilityIn(rProperty.aLifetime, aVersion) == td::Availability::NotYet
                    || td::AvailabilityIn(rProperty.aLifetime, aVersion) == td::Availability::Removed)
                    continue;
                bDefault = true;
                const std::string aDefault = td::EffectiveDefault(rTool, aItems, rProperty, aVersion);
                if (!aDefault.empty())
                    aValues.push_back(aDefault);
            }
            CommandProperty aProperty;
            aProperty.IsDefault = bDefault;
            aProperty.Id = Utf8(rProperty.aId);
            aProperty.Name = Utf8(rProperty.aName);
            aProperty.Description = DescriptionOf(rProperty);
            std::vector<OUString> aTexts;
            for (const std::string& rValue : aValues)
                aTexts.push_back(ValueText(rProperty, rValue));
            aProperty.Values = comphelper::containerToSequence(aTexts);
            // the set of values, for a property that is one of a set
            std::vector<OUString> aChoices;
            if (rProperty.eKind == td::Kind::Bool)
                aChoices = { ValueText(rProperty, "true"), ValueText(rProperty, "false") };
            else if (rProperty.eKind == td::Kind::Enum)
                for (const td::EnumValue& rEnum : rProperty.aValues)
                    aChoices.push_back(ValueText(rProperty, rEnum.aSwitch));
            aProperty.Choices = comphelper::containerToSequence(aChoices);
            aProperty.State = CommandPropertyState_AVAILABLE;
            // The version first: what is gone says more than where it was for.
            // The property has its own lifetime, and so has each value of an
            // enumeration; the first of them that is not plain available speaks.
            std::vector<const td::Lifetime*> aLifetimes{ &rProperty.aLifetime };
            if (rProperty.eKind == td::Kind::Enum)
                for (const std::string& rValue : aValues)
                    for (const td::EnumValue& rEnum : rProperty.aValues)
                        if (rEnum.aSwitch == rValue)
                            aLifetimes.push_back(&rEnum.aLifetime);
            for (const td::Lifetime* pLifetime : aLifetimes)
            {
                const td::Availability eAvailability = td::AvailabilityIn(*pLifetime, aVersion);
                if (eAvailability == td::Availability::NotYet)
                {
                    aProperty.State = CommandPropertyState_NOT_YET;
                    aProperty.Detail = Utf8(pLifetime->aSince);
                }
                else if (eAvailability == td::Availability::Removed)
                {
                    aProperty.State = CommandPropertyState_REMOVED;
                    aProperty.Detail = Utf8(pLifetime->aUntil);
                }
                else if (eAvailability == td::Availability::Deprecated)
                {
                    aProperty.State = CommandPropertyState_DEPRECATED;
                    aProperty.Detail = Utf8(pLifetime->aDeprecated);
                }
                else
                    continue;
                break;
            }
            if (aProperty.State == CommandPropertyState_AVAILABLE
                && (!td::AppliesInMode(rCategory.aModes, aMode) || !td::AppliesInMode(rProperty.aModes, aMode)))
            {
                aProperty.State = CommandPropertyState_NOT_IN_MODE;
                aProperty.Detail = Utf8(aMode);
            }
            aProperties.push_back(std::move(aProperty));
        }
        if (!aProperties.empty())
            aCategories.push_back({ Utf8(rCategory.aId), Utf8(rCategory.aName), comphelper::containerToSequence(aProperties) });
    }
    aResult.Categories = comphelper::containerToSequence(aCategories);
    std::vector<std::string> aOther;
    for (const td::Item& rItem : aItems)
    {
        if (!rItem.aPropertyId.empty())
            continue;
        // the switch that put the tool in its mode is the Mode, not an unknown argument
        const auto itMode = std::find_if(rTool.aModes.begin(), rTool.aModes.end(), [&](const td::Mode& rMode)
                                         { return rMode.aId == aMode
                                                  && std::find(rMode.aSwitches.begin(), rMode.aSwitches.end(),
                                                               rItem.aValue) != rMode.aSwitches.end(); });
        if (itMode == rTool.aModes.end())
            aOther.push_back(rItem.aValue);
    }
    aResult.OtherArguments = SequenceOf(aOther);
    return aResult;
}

CommandDescription ToolDescriptions::describeOverrides(const OUString& rToolId, const OUString& rMode,
                                                       const OUString& rVersion,
                                                       const uno::Sequence<OUString>& rDefaults,
                                                       const uno::Sequence<OUString>& rArguments)
{
    const CommandDescription aBase = describeCommand(rToolId, rMode, rVersion, rDefaults);
    CommandDescription aResult = describeCommand(rToolId, rMode, rVersion, rArguments);
    auto find = [](const CommandDescription& rCommand, const OUString& rId) -> const CommandProperty*
    {
        for (const CommandCategory& rCategory : rCommand.Categories)
            for (const CommandProperty& rProperty : rCategory.Properties)
                if (rProperty.Id == rId)
                    return &rProperty;
        return nullptr;
    };
    // what the arguments set, marked where the defaults say otherwise
    for (sal_Int32 nCategory = 0; nCategory < aResult.Categories.getLength(); ++nCategory)
        for (CommandProperty& rProperty : Mutable(aResult.Categories.getArray()[nCategory].Properties))
            if (const CommandProperty* pBase = find(aBase, rProperty.Id))
            {
                if (pBase->Values != rProperty.Values)
                {
                    rProperty.Overridden = true;
                    rProperty.InheritedValues = pBase->Values;
                }
            }
            else
                rProperty.Overridden = true; // the defaults do not set it
    // what only the defaults set, the arguments leave out: that is a departure too
    std::vector<CommandCategory> aCategories(aResult.Categories.begin(), aResult.Categories.end());
    for (const CommandCategory& rBaseCategory : aBase.Categories)
        for (const CommandProperty& rBaseProperty : rBaseCategory.Properties)
        {
            if (find(aResult, rBaseProperty.Id))
                continue;
            CommandProperty aGone = rBaseProperty;
            aGone.Values = {};
            aGone.Overridden = true;
            aGone.InheritedValues = rBaseProperty.Values;
            auto it = std::find_if(aCategories.begin(), aCategories.end(),
                                   [&](const CommandCategory& r) { return r.Id == rBaseCategory.Id; });
            if (it == aCategories.end())
            {
                aCategories.push_back({ rBaseCategory.Id, rBaseCategory.Name, { aGone } });
            }
            else
            {
                std::vector<CommandProperty> aProperties(it->Properties.begin(), it->Properties.end());
                aProperties.push_back(std::move(aGone));
                it->Properties = comphelper::containerToSequence(aProperties);
            }
        }
    aResult.Categories = comphelper::containerToSequence(aCategories);
    return aResult;
}

uno::Sequence<CommandItem> ToolDescriptions::parseCommand(const OUString& rToolId,
                                                          const uno::Sequence<OUString>& rArguments)
{
    std::vector<CommandItem> aResult;
    for (const td::Item& rItem : td::ParseCommand(ToolOf(rToolId), ArgumentsOf(rArguments)))
    {
        OUString aJoiner;
        if (rItem.oJoiner)
            aJoiner = *rItem.oJoiner == td::Joiner::Attached   ? u"Attached"_ustr
                      : *rItem.oJoiner == td::Joiner::Separate ? u"Separate"_ustr
                                                               : u"Equals"_ustr;
        aResult.push_back({ Utf8(rItem.aPropertyId), Utf8(rItem.aValue), aJoiner });
    }
    return comphelper::containerToSequence(aResult);
}

uno::Sequence<OUString> ToolDescriptions::formatCommand(const OUString& rToolId, const uno::Sequence<CommandItem>& rItems)
{
    const td::Tool& rTool = ToolOf(rToolId);
    std::vector<td::Item> aItems;
    for (const CommandItem& rItem : rItems)
    {
        td::Item aItem;
        aItem.aPropertyId = ToUtf8(rItem.PropertyId);
        aItem.aValue = ToUtf8(rItem.Value);
        if (rItem.Joiner == "Attached")
            aItem.oJoiner = td::Joiner::Attached;
        else if (rItem.Joiner == "Separate")
            aItem.oJoiner = td::Joiner::Separate;
        else if (rItem.Joiner == "Equals")
            aItem.oJoiner = td::Joiner::Equals;
        else if (!rItem.Joiner.isEmpty())
            throw lang::IllegalArgumentException("Joiner is \"" + rItem.Joiner + "\": Attached, Separate or Equals",
                                                 getXWeak(), 2);
        aItems.push_back(std::move(aItem));
    }
    return SequenceOf(td::FormatCommand(rTool, aItems));
}
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_ToolDescriptions_get_implementation(uno::XComponentContext* pContext,
                                                           uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::ToolDescriptions(pContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
