/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "tooldescription.hxx"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>

namespace pt = boost::property_tree;

namespace codeservices::tooldesc
{
namespace
{
struct FormatError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

// What an extension is called: "XSomething" is skipped without a word, so that
// a description can carry what a newer reader will understand, or what only a
// tool of its own (an editor) cares about.
bool IsExtension(const std::string& rName) { return rName.size() > 1 && rName[0] == 'X' && std::isupper(static_cast<unsigned char>(rName[1])); }

const pt::ptree& NoAttributes()
{
    static const pt::ptree aEmpty;
    return aEmpty;
}

const pt::ptree& AttributesOf(const pt::ptree& rNode)
{
    const auto oAttributes = rNode.get_child_optional("<xmlattr>");
    return oAttributes ? *oAttributes : NoAttributes();
}

// an attribute that is not in the list is a mistake in the file, and said so
void CheckAttributes(const pt::ptree& rNode, const std::string& rElement, std::initializer_list<const char*> aAllowed)
{
    for (const auto& rAttribute : AttributesOf(rNode))
    {
        if (IsExtension(rAttribute.first))
            continue;
        if (std::none_of(aAllowed.begin(), aAllowed.end(),
                         [&](const char* pName) { return rAttribute.first == pName; }))
            throw FormatError("unknown attribute " + rAttribute.first + " of " + rElement);
    }
}

std::string AttributeOf(const pt::ptree& rNode, const char* pName) { return AttributesOf(rNode).get<std::string>(pName, ""); }

std::string Required(const pt::ptree& rNode, const std::string& rElement, const char* pName)
{
    std::string aValue = AttributeOf(rNode, pName);
    if (aValue.empty())
        throw FormatError(rElement + " has no " + pName);
    return aValue;
}

// "code.tool/1" -> 1
int FormatNumber(const std::string& rFormat)
{
    constexpr std::string_view aPrefix = "code.tool/";
    if (rFormat.compare(0, aPrefix.size(), aPrefix) != 0 || rFormat.size() == aPrefix.size())
        throw FormatError("Format is \"" + rFormat + "\"; this is not a description in a code.tool format");
    int nNumber = 0;
    for (size_t i = aPrefix.size(); i < rFormat.size(); ++i)
    {
        if (!std::isdigit(static_cast<unsigned char>(rFormat[i])) || nNumber > 100000)
            throw FormatError("Format is \"" + rFormat + "\"; the number after the slash is not a whole number");
        nNumber = nNumber * 10 + (rFormat[i] - '0');
    }
    return nNumber;
}

bool IsVersion(const std::string& rText)
{
    if (rText.empty() || rText.front() == '.' || rText.back() == '.' || rText.find("..") != std::string::npos)
        return false;
    return std::all_of(rText.begin(), rText.end(),
                       [](char c) { return c == '.' || std::isdigit(static_cast<unsigned char>(c)); });
}

std::vector<std::string> Words(const std::string& rText)
{
    std::vector<std::string> aWords;
    std::istringstream aStream(rText);
    std::string aWord;
    while (aStream >> aWord)
        aWords.push_back(aWord);
    return aWords;
}

Lifetime ReadLifetime(const pt::ptree& rNode, const std::string& rWhere)
{
    Lifetime aLifetime;
    aLifetime.aSince = AttributeOf(rNode, "Since");
    aLifetime.aUntil = AttributeOf(rNode, "Until");
    aLifetime.aDeprecated = AttributeOf(rNode, "Deprecated");
    for (const auto* pVersion : { &aLifetime.aSince, &aLifetime.aUntil, &aLifetime.aDeprecated })
        if (!pVersion->empty() && !IsVersion(*pVersion))
            throw FormatError("\"" + *pVersion + "\" in " + rWhere + " is not a version (dotted numbers)");
    if (!aLifetime.aSince.empty() && !aLifetime.aUntil.empty() && CompareVersions(aLifetime.aSince, aLifetime.aUntil) >= 0)
        throw FormatError("Since of " + rWhere + " is not before its Until");
    return aLifetime;
}

Joiner JoinerOf(const std::string& rText, const std::string& rWhere)
{
    if (rText.empty() || rText == "Attached")
        return Joiner::Attached;
    if (rText == "Separate")
        return Joiner::Separate;
    if (rText == "Equals")
        return Joiner::Equals;
    throw FormatError("Joiner of " + rWhere + " is \"" + rText + "\": Attached, Separate or Equals");
}

Property ReadProperty(const std::string& rElement, const pt::ptree& rNode)
{
    Property aProperty;
    aProperty.aId = Required(rNode, rElement, "Id");
    const std::string aWhere = rElement + " " + aProperty.aId;
    aProperty.aName = Required(rNode, aWhere, "Name");
    aProperty.aDescription = AttributeOf(rNode, "Description");
    aProperty.aDefault = AttributeOf(rNode, "Default");
    aProperty.aModes = Words(AttributeOf(rNode, "Modes"));
    aProperty.aLifetime = ReadLifetime(rNode, aWhere);

    if (rElement == "BoolProperty")
    {
        CheckAttributes(rNode, aWhere, { "Id", "Name", "Description", "Default", "On", "Off", "Modes", "Since", "Until", "Deprecated" });
        aProperty.eKind = Kind::Bool;
        aProperty.aOn = AttributeOf(rNode, "On");
        aProperty.aOff = AttributeOf(rNode, "Off");
        if (aProperty.aOn.empty() && aProperty.aOff.empty())
            throw FormatError(aWhere + " has neither On nor Off");
        if (!aProperty.aDefault.empty() && aProperty.aDefault != "true" && aProperty.aDefault != "false")
            throw FormatError("Default of " + aWhere + " is \"" + aProperty.aDefault + "\": true or false");
    }
    else if (rElement == "EnumProperty")
    {
        CheckAttributes(rNode, aWhere, { "Id", "Name", "Description", "Default", "Modes", "Since", "Until", "Deprecated" });
        aProperty.eKind = Kind::Enum;
        for (const auto& rChild : rNode)
        {
            if (rChild.first == "<xmlattr>" || rChild.first == "<xmlcomment>" || rChild.first == "<xmltext>"
                || IsExtension(rChild.first))
                continue;
            if (rChild.first != "EnumValue")
                throw FormatError("unknown element " + rChild.first + " in " + aWhere);
            CheckAttributes(rChild.second, "EnumValue of " + aWhere,
                            { "Name", "Switch", "Description", "Since", "Until", "Deprecated" });
            EnumValue aValue;
            aValue.aSwitch = Required(rChild.second, "EnumValue of " + aWhere, "Switch");
            aValue.aName = AttributeOf(rChild.second, "Name");
            if (aValue.aName.empty())
                aValue.aName = aValue.aSwitch;
            aValue.aDescription = AttributeOf(rChild.second, "Description");
            aValue.aLifetime = ReadLifetime(rChild.second, "EnumValue " + aValue.aSwitch + " of " + aWhere);
            for (const EnumValue& rOther : aProperty.aValues)
                if (rOther.aSwitch == aValue.aSwitch)
                    throw FormatError("the Switch " + aValue.aSwitch + " is given twice in " + aWhere);
            aProperty.aValues.push_back(std::move(aValue));
        }
        if (aProperty.aValues.empty())
            throw FormatError(aWhere + " has no EnumValue");
        if (!aProperty.aDefault.empty()
            && std::none_of(aProperty.aValues.begin(), aProperty.aValues.end(),
                            [&](const EnumValue& rValue) { return rValue.aSwitch == aProperty.aDefault; }))
            throw FormatError("Default of " + aWhere + " is none of its values");
    }
    else
    {
        CheckAttributes(rNode, aWhere, { "Id", "Name", "Description", "Default", "Prefix", "Joiner", "Kind", "Modes", "Since", "Until", "Deprecated" });
        aProperty.eKind = rElement == "IntProperty"        ? Kind::Int
                          : rElement == "StringListProperty" ? Kind::StringList
                                                             : Kind::String;
        aProperty.aPrefix = Required(rNode, aWhere, "Prefix");
        aProperty.eJoiner = JoinerOf(AttributeOf(rNode, "Joiner"), aWhere);
        const std::string aKind = AttributeOf(rNode, "Kind");
        if (!aKind.empty() && aKind != "Path")
            throw FormatError("Kind of " + aWhere + " is \"" + aKind + "\": only Path is known");
        aProperty.bPath = aKind == "Path";
    }
    // the rules for a default; an enumeration also has its EnumValues
    for (const auto& rChild : rNode)
    {
        if (rChild.first == "<xmlattr>" || rChild.first == "<xmlcomment>" || rChild.first == "<xmltext>"
            || IsExtension(rChild.first) || (rElement == "EnumProperty" && rChild.first == "EnumValue"))
            continue;
        if (rChild.first != "DefaultIf")
            throw FormatError("unknown element " + rChild.first + " in " + aWhere);
        const std::string aRuleWhere = "DefaultIf of " + aWhere;
        CheckAttributes(rChild.second, aRuleWhere, { "Property", "Is", "Value", "Since", "Until" });
        DefaultRule aRule;
        aRule.aProperty = Required(rChild.second, aRuleWhere, "Property");
        aRule.aIs = Words(Required(rChild.second, aRuleWhere, "Is"));
        if (!AttributesOf(rChild.second).count("Value"))
            throw FormatError(aRuleWhere + " has no Value");
        aRule.aValue = AttributeOf(rChild.second, "Value");
        const Lifetime aRange = ReadLifetime(rChild.second, aRuleWhere);
        aRule.aSince = aRange.aSince;
        aRule.aUntil = aRange.aUntil;
        if (aRule.aProperty == aProperty.aId)
            throw FormatError(aRuleWhere + " depends on the property itself");
        if (aProperty.eKind == Kind::Bool && aRule.aValue != "true" && aRule.aValue != "false")
            throw FormatError("Value of " + aRuleWhere + " is \"" + aRule.aValue + "\": true or false");
        if (aProperty.eKind == Kind::Enum
            && std::none_of(aProperty.aValues.begin(), aProperty.aValues.end(),
                            [&](const EnumValue& rValue) { return rValue.aSwitch == aRule.aValue; }))
            throw FormatError("Value of " + aRuleWhere + " is none of its values");
        aProperty.aDefaultRules.push_back(std::move(aRule));
    }
    return aProperty;
}

Tool ReadTool(const pt::ptree& rRoot)
{
    // the comments before the root are not it
    const pt::ptree* pRoot = nullptr;
    std::string aRootName;
    for (const auto& rChild : rRoot)
        if (rChild.first != "<xmlcomment>")
        {
            pRoot = &rChild.second;
            aRootName = rChild.first;
        }
    if (!pRoot || aRootName != "ToolDescription")
        throw FormatError("the root element is not ToolDescription");

    // the version first: a file of a format this reader does not know is not read further
    const std::string aFormat = AttributeOf(*pRoot, "Format");
    if (aFormat.empty())
        throw FormatError("ToolDescription has no Format (expected code.tool/" + std::to_string(SupportedFormat) + ")");
    const int nFormat = FormatNumber(aFormat);
    if (nFormat > SupportedFormat)
        throw FormatError("the description is in format " + aFormat + "; this version of Code reads up to code.tool/"
                          + std::to_string(SupportedFormat));
    if (nFormat < 1)
        throw FormatError("Format is \"" + aFormat + "\": the numbers start at 1");

    CheckAttributes(*pRoot, "ToolDescription", { "Format", "Id", "Name", "Revision", "Match", "Extends", "Since", "Until" });
    Tool aTool;
    aTool.aId = Required(*pRoot, "ToolDescription", "Id");
    aTool.aName = Required(*pRoot, "ToolDescription", "Name");
    aTool.aExtends = AttributeOf(*pRoot, "Extends");
    aTool.aMatch = AttributeOf(*pRoot, "Match");
    aTool.aVersions = ReadLifetime(*pRoot, "ToolDescription");
    if (!aTool.aVersions.aDeprecated.empty())
        throw FormatError("a ToolDescription has no Deprecated: it is for versions, not of them");
    const std::string aRevision = AttributeOf(*pRoot, "Revision");
    if (!aRevision.empty())
    {
        try
        {
            aTool.nRevision = std::stoi(aRevision);
        }
        catch (const std::exception&)
        {
            throw FormatError("Revision is \"" + aRevision + "\": a whole number");
        }
    }
    if (!aTool.aMatch.empty())
    {
        try
        {
            std::regex aTest(aTool.aMatch);
        }
        catch (const std::regex_error&)
        {
            throw FormatError("Match is not a regular expression: " + aTool.aMatch);
        }
    }

    std::set<std::string> aIds;
    for (const auto& rChild : *pRoot)
    {
        if (rChild.first == "<xmlattr>" || rChild.first == "<xmlcomment>" || rChild.first == "<xmltext>"
            || IsExtension(rChild.first))
            continue;
        if (rChild.first == "Mode")
        {
            CheckAttributes(rChild.second, "Mode", { "Id", "Name", "Switches", "Default" });
            Mode aMode;
            aMode.aId = Required(rChild.second, "Mode", "Id");
            aMode.aName = Required(rChild.second, "Mode " + aMode.aId, "Name");
            aMode.aSwitches = Words(AttributeOf(rChild.second, "Switches"));
            const std::string aDefault = AttributeOf(rChild.second, "Default");
            if (!aDefault.empty() && aDefault != "true" && aDefault != "false")
                throw FormatError("Default of Mode " + aMode.aId + " is \"" + aDefault + "\": true or false");
            aMode.bDefault = aDefault == "true";
            if (aMode.aSwitches.empty() && !aMode.bDefault)
                throw FormatError("Mode " + aMode.aId + " has no Switches and is not the Default");
            for (const Mode& rOther : aTool.aModes)
            {
                if (rOther.aId == aMode.aId)
                    throw FormatError("the Mode " + aMode.aId + " is given twice");
                if (rOther.bDefault && aMode.bDefault)
                    throw FormatError("two Modes are the Default: " + rOther.aId + " and " + aMode.aId);
            }
            aTool.aModes.push_back(std::move(aMode));
            continue;
        }
        if (rChild.first != "Category")
            throw FormatError("unknown element " + rChild.first + " in ToolDescription");
        CheckAttributes(rChild.second, "Category", { "Id", "Name", "Modes" });
        Category aCategory;
        aCategory.aModes = Words(AttributeOf(rChild.second, "Modes"));
        aCategory.aId = Required(rChild.second, "Category", "Id");
        aCategory.aName = Required(rChild.second, "Category " + aCategory.aId, "Name");
        for (const Category& rOther : aTool.aCategories)
            if (rOther.aId == aCategory.aId)
                throw FormatError("the Category " + aCategory.aId + " is given twice");
        for (const auto& rProperty : rChild.second)
        {
            if (rProperty.first == "<xmlattr>" || rProperty.first == "<xmlcomment>" || rProperty.first == "<xmltext>"
                || IsExtension(rProperty.first))
                continue;
            if (rProperty.first != "StringProperty" && rProperty.first != "StringListProperty"
                && rProperty.first != "BoolProperty" && rProperty.first != "IntProperty"
                && rProperty.first != "EnumProperty")
                throw FormatError("unknown element " + rProperty.first + " in Category " + aCategory.aId);
            Property aProperty = ReadProperty(rProperty.first, rProperty.second);
            if (!aIds.insert(aProperty.aId).second)
                throw FormatError("the Id " + aProperty.aId + " is given twice");
            aCategory.aProperties.push_back(std::move(aProperty));
        }
        aTool.aCategories.push_back(std::move(aCategory));
    }

    // a mode that is named is a mode that is declared
    auto checkModes = [&](const std::vector<std::string>& rModes, const std::string& rWhere)
    {
        for (const std::string& rId : rModes)
            if (std::none_of(aTool.aModes.begin(), aTool.aModes.end(), [&](const Mode& r) { return r.aId == rId; }))
                throw FormatError(rWhere + " is for the Mode " + rId + ", which the description does not have");
    };
    for (const Category& rCategory : aTool.aCategories)
    {
        checkModes(rCategory.aModes, "Category " + rCategory.aId);
        for (const Property& rProperty : rCategory.aProperties)
        {
            checkModes(rProperty.aModes, rProperty.aId);
            for (const DefaultRule& rRule : rProperty.aDefaultRules)
                if (!aIds.count(rRule.aProperty))
                    throw FormatError("DefaultIf of " + rProperty.aId + " depends on " + rRule.aProperty
                                      + ", which the description does not have");
        }
    }
    return aTool;
}
}

LoadResult Parse(std::string_view aXml)
{
    LoadResult aResult;
    try
    {
        pt::ptree aTree;
        std::istringstream aStream{ std::string(aXml) };
        pt::read_xml(aStream, aTree, pt::xml_parser::trim_whitespace);
        aResult.oTool = ReadTool(aTree);
    }
    catch (const pt::xml_parser_error& rException)
    {
        aResult.aError = "not well-formed XML: " + rException.message();
    }
    catch (const FormatError& rException)
    {
        aResult.aError = rException.what();
    }
    catch (const std::exception& rException)
    {
        aResult.aError = rException.what();
    }
    return aResult;
}

LoadResult LoadFile(const std::string& rPath)
{
    std::ifstream aFile(rPath, std::ios::binary);
    if (!aFile)
        return { std::nullopt, "cannot open " + rPath };
    std::ostringstream aText;
    aText << aFile.rdbuf();
    LoadResult aResult = Parse(aText.str());
    if (!aResult.oTool)
        aResult.aError = rPath + ": " + aResult.aError;
    return aResult;
}

void Merge(Tool& rDerived, const Tool& rBase)
{
    std::vector<Category> aCategories = rBase.aCategories;
    for (const Category& rCategory : rDerived.aCategories)
    {
        auto itCategory = std::find_if(aCategories.begin(), aCategories.end(),
                                       [&](const Category& r) { return r.aId == rCategory.aId; });
        if (itCategory == aCategories.end())
        {
            aCategories.push_back(rCategory);
            continue;
        }
        if (!rCategory.aName.empty())
            itCategory->aName = rCategory.aName;
        if (!rCategory.aModes.empty())
            itCategory->aModes = rCategory.aModes;
        for (const Property& rProperty : rCategory.aProperties)
        {
            auto itProperty = std::find_if(itCategory->aProperties.begin(), itCategory->aProperties.end(),
                                           [&](const Property& r) { return r.aId == rProperty.aId; });
            if (itProperty == itCategory->aProperties.end())
                itCategory->aProperties.push_back(rProperty);
            else
                *itProperty = rProperty;
        }
    }
    rDerived.aCategories = std::move(aCategories);
    // the base's modes come first, in its order; the derived one's replace by Id or follow
    std::vector<Mode> aModes = rBase.aModes;
    for (const Mode& rMode : rDerived.aModes)
    {
        auto it = std::find_if(aModes.begin(), aModes.end(), [&](const Mode& r) { return r.aId == rMode.aId; });
        if (it == aModes.end())
            aModes.push_back(rMode);
        else
            *it = rMode;
    }
    rDerived.aModes = std::move(aModes);
    if (rDerived.aMatch.empty())
        rDerived.aMatch = rBase.aMatch;
}

bool Matches(const Tool& rTool, std::string_view aExecutable)
{
    if (rTool.aMatch.empty())
        return false;
    try
    {
        return std::regex_search(aExecutable.begin(), aExecutable.end(), std::regex(rTool.aMatch));
    }
    catch (const std::regex_error&)
    {
        return false;
    }
}

const Property* FindProperty(const Tool& rTool, std::string_view aId)
{
    for (const Category& rCategory : rTool.aCategories)
        for (const Property& rProperty : rCategory.aProperties)
            if (rProperty.aId == aId)
                return &rProperty;
    return nullptr;
}

namespace
{
bool IsWholeNumber(const std::string& rText)
{
    size_t i = !rText.empty() && (rText[0] == '-' || rText[0] == '+') ? 1 : 0;
    if (i >= rText.size())
        return false;
    for (; i < rText.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(rText[i])))
            return false;
    return true;
}

bool StartsWith(const std::string& rText, const std::string& rPrefix)
{
    return rText.compare(0, rPrefix.size(), rPrefix) == 0;
}

// the text an option starts with, as it stands on the command line: the
// prefix, and the "=" an Equals option has before its value
std::string OptionStart(const Property& rProperty, Joiner eJoiner)
{
    return eJoiner == Joiner::Equals ? rProperty.aPrefix + "=" : rProperty.aPrefix;
}
}

std::vector<Item> ParseCommand(const Tool& rTool, const std::vector<std::string>& rArguments)
{
    std::vector<Item> aItems;
    for (size_t i = 0; i < rArguments.size(); ++i)
    {
        const std::string& rArgument = rArguments[i];

        // a switch the description names exactly (an enum value, a bool) is
        // not an option with a value, whatever its text starts with
        const Property* pExact = nullptr;
        std::string aExactValue;
        // the longest prefix that an option with a value matches
        const Property* pBest = nullptr;
        size_t nBestLength = 0;
        std::optional<Joiner> oBestJoiner;
        bool bBestTakesNext = false;

        for (const Category& rCategory : rTool.aCategories)
            for (const Property& rProperty : rCategory.aProperties)
            {
                switch (rProperty.eKind)
                {
                    case Kind::Enum:
                        for (const EnumValue& rValue : rProperty.aValues)
                            if (rValue.aSwitch == rArgument && !pExact)
                            {
                                pExact = &rProperty;
                                aExactValue = rValue.aSwitch;
                            }
                        break;
                    case Kind::Bool:
                        if (!pExact && !rProperty.aOn.empty() && rProperty.aOn == rArgument)
                        {
                            pExact = &rProperty;
                            aExactValue = "true";
                        }
                        else if (!pExact && !rProperty.aOff.empty() && rProperty.aOff == rArgument)
                        {
                            pExact = &rProperty;
                            aExactValue = "false";
                        }
                        break;
                    default:
                    {
                        // written the way the property says; an option that
                        // takes a value after a blank is also met without it
                        // and the other way round, but "=" is only "="
                        std::vector<Joiner> aForms;
                        switch (rProperty.eJoiner)
                        {
                            case Joiner::Attached:
                                aForms = { Joiner::Attached, Joiner::Separate };
                                break;
                            case Joiner::Separate:
                                aForms = { Joiner::Separate, Joiner::Attached };
                                break;
                            case Joiner::Equals:
                                aForms = { Joiner::Equals };
                                break;
                        }
                        for (Joiner eJoiner : aForms)
                        {
                            const std::string aStart = OptionStart(rProperty, eJoiner);
                            const bool bTakesNext = eJoiner == Joiner::Separate;
                            const bool bMatch = bTakesNext
                                                    ? rArgument == aStart && i + 1 < rArguments.size()
                                                    : StartsWith(rArgument, aStart) && rArgument.size() > aStart.size();
                            if (!bMatch)
                                continue;
                            const std::string aValue
                                = bTakesNext ? rArguments[i + 1] : rArgument.substr(aStart.size());
                            if (rProperty.eKind == Kind::Int && !IsWholeNumber(aValue))
                                continue;
                            // the longer option text wins (-isystem over -I); at a tie, the one without a following word
                            if (aStart.size() > nBestLength
                                || (aStart.size() == nBestLength && bBestTakesNext && !bTakesNext))
                            {
                                pBest = &rProperty;
                                nBestLength = aStart.size();
                                oBestJoiner = eJoiner;
                                bBestTakesNext = bTakesNext;
                            }
                        }
                    }
                }
            }

        Item aItem;
        if (pExact)
        {
            aItem.aPropertyId = pExact->aId;
            aItem.aValue = aExactValue;
        }
        else if (pBest)
        {
            aItem.aPropertyId = pBest->aId;
            aItem.oJoiner = oBestJoiner;
            if (bBestTakesNext)
                aItem.aValue = rArguments[++i];
            else
                aItem.aValue = rArgument.substr(OptionStart(*pBest, *oBestJoiner).size());
        }
        else
            aItem.aValue = rArgument;
        aItems.push_back(std::move(aItem));
    }
    return aItems;
}

std::vector<std::string> FormatCommand(const Tool& rTool, const std::vector<Item>& rItems)
{
    std::vector<std::string> aArguments;
    for (const Item& rItem : rItems)
    {
        const Property* pProperty = rItem.aPropertyId.empty() ? nullptr : FindProperty(rTool, rItem.aPropertyId);
        if (!pProperty)
        {
            aArguments.push_back(rItem.aValue); // not known: as it was
            continue;
        }
        switch (pProperty->eKind)
        {
            case Kind::Enum:
                aArguments.push_back(rItem.aValue);
                break;
            case Kind::Bool:
            {
                const std::string& rSwitch = rItem.aValue == "true" ? pProperty->aOn : pProperty->aOff;
                if (!rSwitch.empty())
                    aArguments.push_back(rSwitch);
                break;
            }
            default:
            {
                const Joiner eJoiner = rItem.oJoiner.value_or(pProperty->eJoiner);
                if (eJoiner == Joiner::Separate)
                {
                    aArguments.push_back(pProperty->aPrefix);
                    aArguments.push_back(rItem.aValue);
                }
                else
                    aArguments.push_back(OptionStart(*pProperty, eJoiner) + rItem.aValue);
            }
        }
    }
    return aArguments;
}

std::string DetectMode(const Tool& rTool, const std::vector<std::string>& rArguments)
{
    for (const Mode& rMode : rTool.aModes)
        for (const std::string& rSwitch : rMode.aSwitches)
            if (std::find(rArguments.begin(), rArguments.end(), rSwitch) != rArguments.end())
                return rMode.aId;
    for (const Mode& rMode : rTool.aModes)
        if (rMode.bDefault)
            return rMode.aId;
    return {};
}

namespace
{
std::string EffectiveDefaultRec(const Tool& rTool, const std::vector<Item>& rItems, const Property& rProperty,
                                std::string_view aVersion, int nDepth);

// what a property is for the purpose of a condition: what the command gives,
// else the default it would have
std::string ValueForCondition(const Tool& rTool, const std::vector<Item>& rItems, const Property& rProperty,
                              std::string_view aVersion, int nDepth)
{
    const std::vector<std::string> aGiven = ValuesOf(rItems, rProperty.aId);
    if (!aGiven.empty())
        return aGiven.back(); // the last wins on a command line
    return EffectiveDefaultRec(rTool, rItems, rProperty, aVersion, nDepth);
}

std::string EffectiveDefaultRec(const Tool& rTool, const std::vector<Item>& rItems, const Property& rProperty,
                                std::string_view aVersion, int nDepth)
{
    // a loop of conditions (A on B, B on A) is a mistake of the description: it stops here
    if (nDepth > 16)
        return rProperty.aDefault;
    for (const DefaultRule& rRule : rProperty.aDefaultRules)
    {
        if (!rRule.aSince.empty() && (aVersion.empty() || CompareVersions(aVersion, rRule.aSince) < 0))
            continue;
        if (!rRule.aUntil.empty() && (aVersion.empty() || CompareVersions(aVersion, rRule.aUntil) >= 0))
            continue;
        const Property* pOther = FindProperty(rTool, rRule.aProperty);
        if (!pOther)
            continue;
        const std::string aOther = ValueForCondition(rTool, rItems, *pOther, aVersion, nDepth + 1);
        if (std::find(rRule.aIs.begin(), rRule.aIs.end(), aOther) != rRule.aIs.end())
            return rRule.aValue;
    }
    return rProperty.aDefault;
}
}

std::string EffectiveDefault(const Tool& rTool, const std::vector<Item>& rItems, const Property& rProperty,
                             std::string_view aVersion)
{
    return EffectiveDefaultRec(rTool, rItems, rProperty, aVersion, 0);
}

bool AppliesInMode(const std::vector<std::string>& rModes, std::string_view aMode)
{
    return rModes.empty() || aMode.empty() || std::find(rModes.begin(), rModes.end(), aMode) != rModes.end();
}

int CompareVersions(std::string_view aLeft, std::string_view aRight)
{
    auto next = [](std::string_view& rText)
    {
        long nPart = 0;
        size_t i = 0;
        for (; i < rText.size() && rText[i] != '.'; ++i)
            nPart = nPart * 10 + (rText[i] - '0');
        rText.remove_prefix(i < rText.size() ? i + 1 : i);
        return nPart;
    };
    while (!aLeft.empty() || !aRight.empty())
    {
        const long nLeft = next(aLeft);
        const long nRight = next(aRight);
        if (nLeft != nRight)
            return nLeft < nRight ? -1 : 1;
    }
    return 0;
}

Availability AvailabilityIn(const Lifetime& rLifetime, std::string_view aVersion)
{
    if (aVersion.empty())
        return Availability::Available;
    if (!rLifetime.aSince.empty() && CompareVersions(aVersion, rLifetime.aSince) < 0)
        return Availability::NotYet;
    if (!rLifetime.aUntil.empty() && CompareVersions(aVersion, rLifetime.aUntil) >= 0)
        return Availability::Removed;
    if (!rLifetime.aDeprecated.empty() && CompareVersions(aVersion, rLifetime.aDeprecated) >= 0)
        return Availability::Deprecated;
    return Availability::Available;
}

std::vector<std::string> ValuesOf(const std::vector<Item>& rItems, std::string_view aPropertyId)
{
    std::vector<std::string> aValues;
    for (const Item& rItem : rItems)
        if (rItem.aPropertyId == aPropertyId)
            aValues.push_back(rItem.aValue);
    return aValues;
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
