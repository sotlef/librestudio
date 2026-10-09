/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

// Tool descriptions: what a command (a compiler, a linker, any tool a build
// runs) takes on its command line, written down as data -- its options, in
// categories, with their types and the values an enumeration allows.  The file
// format is "code.tool/N" (docs/tool-description-format.md).
//
// A description works both ways.  ParseCommand() sorts the arguments of an
// existing command into properties; FormatCommand() builds the arguments from
// them.  What the description does not know stays in the list, in its place,
// as it was: no argument is ever lost, and formatting what was parsed gives
// the arguments back.
//
// This part knows nothing of UNO; strings are UTF-8.

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace codeservices::tooldesc
{
/// the newest "code.tool/N" this reader understands
constexpr int SupportedFormat = 1;

enum class Kind
{
    Bool, // a switch that is on, or has an opposite that is off
    String, // an option with one value
    Int, // an option with a whole number
    Enum, // one switch of a set
    StringList // an option that may be given again and again
};

/// how an option and its value are written
enum class Joiner
{
    Attached, // -Ipath
    Separate, // -I path
    Equals // --sysroot=path
};

/// Which versions of the tool have something: Since is the first that has it,
/// Until the first that has not any more (so a range is [Since, Until)), and
/// Deprecated the first from which it is discouraged.  Versions are dotted
/// numbers ("4.9", "12"); an empty one is no limit.  None of this changes
/// whether a command is understood, only what is said about it.
struct Lifetime
{
    std::string aSince;
    std::string aUntil;
    std::string aDeprecated;
};

struct EnumValue
{
    std::string aName; // shown
    std::string aSwitch; // as written on the command line
    std::string aDescription;
    Lifetime aLifetime;
};

/// A default that depends on another property: when that one has one of the
/// values (for a Bool "true"/"false", for an Enum a switch, else the text), and
/// the tool's version is in [aSince, aUntil), the property is aValue without
/// being told.  The first rule that holds wins.
struct DefaultRule
{
    std::string aProperty;
    std::vector<std::string> aIs;
    std::string aValue;
    std::string aSince;
    std::string aUntil;
};

struct Property
{
    Kind eKind = Kind::String;
    std::string aId;
    std::string aName;
    std::string aDescription;
    /// what the property has when the command does not give it
    std::string aDefault;
    /// tried in order before aDefault
    std::vector<DefaultRule> aDefaultRules;
    /// String, Int, StringList: the option's text, without the value
    std::string aPrefix;
    Joiner eJoiner = Joiner::Attached;
    /// Bool: the switch that turns it on, and the one that turns it off
    std::string aOn;
    std::string aOff;
    /// the value is a file or directory
    bool bPath = false;
    std::vector<EnumValue> aValues;
    /// the Ids of the modes it is for; none: for all of them
    std::vector<std::string> aModes;
    Lifetime aLifetime;
};

struct Category
{
    std::string aId;
    std::string aName;
    /// the Ids of the modes it is for; none: for all of them
    std::vector<std::string> aModes;
    std::vector<Property> aProperties;
};

/// What one program does, when it is several things: gcc compiles, or
/// assembles, or preprocesses, or links, by the switches it is given.
struct Mode
{
    std::string aId;
    std::string aName;
    /// the arguments that put the tool in this mode
    std::vector<std::string> aSwitches;
    /// the mode when no other is asked for
    bool bDefault = false;
};

struct Tool
{
    std::string aId;
    std::string aName;
    /// the Id of the description this one builds on, if any
    std::string aExtends;
    /// regular expression (ECMAScript) for the path of the tool's executable
    std::string aMatch;
    /// the revision of the description's content, not of its format
    int nRevision = 0;
    /// the versions of the tool the description is for (a range like Lifetime's)
    Lifetime aVersions;
    /// in the order they are tried; the default one is the last resort
    std::vector<Mode> aModes;
    std::vector<Category> aCategories;
};

struct LoadResult
{
    std::optional<Tool> oTool;
    /// what is wrong with the file, when oTool is empty
    std::string aError;
};

/// Reads a description from the text of a file.
LoadResult Parse(std::string_view aXml);
LoadResult LoadFile(const std::string& rPath);

/// Makes rDerived what it is on top of rBase: the categories and properties of
/// the base, those of the derived description added to them or, by the same
/// Id, put in their place.  The Name and Match of the derived one stay.
void Merge(Tool& rDerived, const Tool& rBase);

/// whether the description is for the tool at this path
bool Matches(const Tool& rTool, std::string_view aExecutable);

/// the property of this Id, or null
const Property* FindProperty(const Tool& rTool, std::string_view aId);

/// One argument, or one option with its value, of a command.
struct Item
{
    /// the property it belongs to; empty for an argument the description does not know
    std::string aPropertyId;
    /// Bool: "true" or "false"; Enum: the switch; Other: the argument itself
    std::string aValue;
    /// how the value was written; none for an item made by hand, which takes the property's way
    std::optional<Joiner> oJoiner;
};

/// Sorts the arguments of a command (without the compiler's own name) into
/// items, in the order they were given.
std::vector<Item> ParseCommand(const Tool& rTool, const std::vector<std::string>& rArguments);

/// The mode a command is in: the first of the description's modes (in its
/// order) whose switch the arguments contain, else the default mode; empty if
/// the description has no modes.
std::string DetectMode(const Tool& rTool, const std::vector<std::string>& rArguments);

/// What the tool does with a property that the command does not give: the
/// first DefaultRule that holds, else its Default.  The condition properties
/// are taken from the items (what the command sets) or, where it sets nothing,
/// from their own defaults.  Empty if there is none.  aVersion may be empty:
/// the rules limited by a version are then left out.
std::string EffectiveDefault(const Tool& rTool, const std::vector<Item>& rItems, const Property& rProperty,
                             std::string_view aVersion);

/// Whether a category or property is for the mode.  An empty mode (not known)
/// is every mode.  Recognising an argument does not depend on the mode: a
/// switch that does not belong to the mode is still known, so that it can be
/// shown as having no effect there instead of being lost among the others.
bool AppliesInMode(const std::vector<std::string>& rModes, std::string_view aMode);

enum class Availability
{
    Available,
    NotYet, // the version is older than Since
    Removed, // the version is Until or newer
    Deprecated
};

/// What a version of the tool has of something; an empty version is not known
/// and says Available.
Availability AvailabilityIn(const Lifetime& rLifetime, std::string_view aVersion);

/// -1, 0, 1: dotted numbers compared by part, a missing part being 0
int CompareVersions(std::string_view aLeft, std::string_view aRight);

/// The arguments the items stand for.
std::vector<std::string> FormatCommand(const Tool& rTool, const std::vector<Item>& rItems);

/// What a property is set to: the values it was given, in order; for a
/// property that holds one value that is the last one (the last one wins on a
/// command line).  Empty if the command does not give it.
std::vector<std::string> ValuesOf(const std::vector<Item>& rItems, std::string_view aPropertyId);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
