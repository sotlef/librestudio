/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// The tool descriptions: reading the format, and the way back and forth between
// a command line and its properties.

#include "tooldescription.hxx"

#include <sal/types.h>

#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/plugin/TestPlugIn.h>

#include <sstream>
#include <string>
#include <vector>

using namespace codeservices::tooldesc;

// cppunit prints what a failed comparison found
namespace CppUnit
{
template <> struct assertion_traits<std::vector<std::string>>
{
    static bool equal(const std::vector<std::string>& rX, const std::vector<std::string>& rY) { return rX == rY; }
    static std::string toString(const std::vector<std::string>& rStrings)
    {
        std::ostringstream aText;
        aText << "[";
        for (const std::string& rString : rStrings)
            aText << " \"" << rString << "\"";
        aText << " ]";
        return aText.str();
    }
};
}

namespace
{
using Strings = std::vector<std::string>;

Tool Gcc()
{
    LoadResult aResult = LoadFile(std::string(TOOLDESC_DIR) + "/gcc.xml");
    CPPUNIT_ASSERT_MESSAGE(aResult.aError, aResult.oTool.has_value());
    return *aResult.oTool;
}

Tool Clang()
{
    Tool aBase = Gcc();
    LoadResult aResult = LoadFile(std::string(TOOLDESC_DIR) + "/clang.xml");
    CPPUNIT_ASSERT_MESSAGE(aResult.aError, aResult.oTool.has_value());
    Merge(*aResult.oTool, aBase);
    return *aResult.oTool;
}

std::string Error(const char* pXml) { return Parse(pXml).aError; }

bool Contains(const std::string& rText, const char* pPart) { return rText.find(pPart) != std::string::npos; }

class ToolDescriptionTest : public CppUnit::TestFixture
{
public:
    void testShippedDescriptionsLoad()
    {
        const Tool aGcc = Gcc();
        CPPUNIT_ASSERT_EQUAL(std::string("gcc"), aGcc.aId);
        CPPUNIT_ASSERT(FindProperty(aGcc, "Includes"));
        CPPUNIT_ASSERT(!aGcc.aCategories.empty());
    }

    void testFormatIsRequiredAndChecked()
    {
        CPPUNIT_ASSERT(Contains(Error("<ToolDescription Id='A' Name='A'/>"), "no Format"));
        CPPUNIT_ASSERT(Contains(Error("<ToolDescription Format='other/1' Id='A' Name='A'/>"), "not a description"));
        CPPUNIT_ASSERT(Contains(Error("<ToolDescription Format='code.tool/x' Id='A' Name='A'/>"), "whole number"));
        CPPUNIT_ASSERT(Contains(Error("<ToolDescription Format='code.tool/0' Id='A' Name='A'/>"), "start at 1"));
        CPPUNIT_ASSERT(Parse("<ToolDescription Format='code.tool/1' Id='A' Name='A'/>").oTool.has_value());
    }

    void testNewerFormatIsRefusedWithoutReadingOn()
    {
        // the body is of a shape /1 does not know: only the version is said
        const std::string aError
            = Error("<ToolDescription Format='code.tool/2' Id='A' Name='A'><Fancy/></ToolDescription>");
        CPPUNIT_ASSERT(Contains(aError, "code.tool/2"));
        CPPUNIT_ASSERT(Contains(aError, "up to code.tool/1"));
    }

    void testMistakesAreSaid()
    {
        const std::string aHead = "<ToolDescription Format='code.tool/1' Id='A' Name='A'>";
        CPPUNIT_ASSERT(Contains(Error((aHead + "<Categry Id='C' Name='C'/></ToolDescription>").c_str()), "unknown element"));
        CPPUNIT_ASSERT(Contains(Error((aHead + "<Category Id='C' Name='C'><IntProperty Id='I' Name='I'/>"
                                               "</Category></ToolDescription>").c_str()),
                                "no Prefix"));
        CPPUNIT_ASSERT(Contains(Error((aHead + "<Category Id='C' Name='C'><BoolProperty Id='B' Name='B' Of='-x'/>"
                                               "</Category></ToolDescription>").c_str()),
                                "unknown attribute Of"));
        CPPUNIT_ASSERT(Contains(Error((aHead + "<Category Id='C' Name='C'><BoolProperty Id='B' Name='B'/>"
                                               "</Category></ToolDescription>").c_str()),
                                "neither On nor Off"));
        CPPUNIT_ASSERT(Contains(Error((aHead + "<Category Id='C' Name='C'><EnumProperty Id='E' Name='E' Default='-z'>"
                                               "<EnumValue Switch='-a'/></EnumProperty></Category></ToolDescription>").c_str()),
                                "none of its values"));
        CPPUNIT_ASSERT(Contains(Error((aHead + "<Category Id='C' Name='C'><BoolProperty Id='B' Name='B' On='-a'/>"
                                               "<BoolProperty Id='B' Name='B2' On='-b'/>"
                                               "</Category></ToolDescription>").c_str()),
                                "given twice"));
        CPPUNIT_ASSERT(Contains(Error("<ToolDescription"), "XML"));
    }

    void testExtensionsAreSkipped()
    {
        const LoadResult aResult = Parse(
            "<ToolDescription Format='code.tool/1' Id='A' Name='A' XNote='n'>"
            "<XEditor Hint='h'/>"
            "<Category Id='C' Name='C' XOrder='1'>"
            "<BoolProperty Id='B' Name='B' On='-b' XIcon='i'/><XMore/></Category></ToolDescription>");
        CPPUNIT_ASSERT_MESSAGE(aResult.aError, aResult.oTool.has_value());
        CPPUNIT_ASSERT(FindProperty(*aResult.oTool, "B"));
    }

    void testEnumAndBoolAreKnownByTheirSwitch()
    {
        const Tool aTool = Gcc();
        const std::vector<Item> aItems = ParseCommand(aTool, { "-O2", "-fno-rtti", "-Wall", "-std=c++17" });
        CPPUNIT_ASSERT_EQUAL(size_t(4), aItems.size());
        CPPUNIT_ASSERT_EQUAL(std::string("OptimizationLevel"), aItems[0].aPropertyId);
        CPPUNIT_ASSERT_EQUAL(std::string("-O2"), aItems[0].aValue);
        CPPUNIT_ASSERT_EQUAL(std::string("Rtti"), aItems[1].aPropertyId);
        CPPUNIT_ASSERT_EQUAL(std::string("false"), aItems[1].aValue);
        CPPUNIT_ASSERT_EQUAL(std::string("true"), aItems[2].aValue);
        CPPUNIT_ASSERT_EQUAL(std::string("Standard"), aItems[3].aPropertyId);
    }

    void testOptionsTakeTheirValue()
    {
        const Tool aTool = Gcc();
        const std::vector<Item> aItems
            = ParseCommand(aTool, { "-DFOO=1", "-D", "BAR", "-I/usr/include", "-isystem", "/opt/inc", "--sysroot=/r" });
        CPPUNIT_ASSERT_EQUAL(Strings({ "FOO=1", "BAR" }), ValuesOf(aItems, "Defines"));
        CPPUNIT_ASSERT_EQUAL(Strings({ "/usr/include" }), ValuesOf(aItems, "Includes"));
        CPPUNIT_ASSERT_EQUAL(Strings({ "/opt/inc" }), ValuesOf(aItems, "SystemIncludes"));
        CPPUNIT_ASSERT_EQUAL(Strings({ "/r" }), ValuesOf(aItems, "Sysroot"));
    }

    void testTheLongerOptionWins()
    {
        // -isystemfoo is not "-I" "systemfoo": the "-isystem" option is longer, and "-i" is not "-I"
        const Tool aTool = Gcc();
        const std::vector<Item> aItems = ParseCommand(aTool, { "-Isystem" });
        CPPUNIT_ASSERT_EQUAL(Strings({ "system" }), ValuesOf(aItems, "Includes"));
    }

    void testWhatIsNotKnownStaysWhereItWas()
    {
        const Tool aTool = Gcc();
        const Strings aArguments = { "-fsome-new-flag", "-O2", "-Xclang", "-load", "-DX", "--weird=1" };
        const std::vector<Item> aItems = ParseCommand(aTool, aArguments);
        CPPUNIT_ASSERT_EQUAL(aArguments.size(), aItems.size());
        CPPUNIT_ASSERT(aItems[0].aPropertyId.empty());
        CPPUNIT_ASSERT_EQUAL(std::string("-fsome-new-flag"), aItems[0].aValue);
        CPPUNIT_ASSERT(aItems[2].aPropertyId.empty());
        CPPUNIT_ASSERT_EQUAL(std::string("Defines"), aItems[4].aPropertyId);
    }

    void testIntNeedsANumber()
    {
        const LoadResult aResult = Parse(
            "<ToolDescription Format='code.tool/1' Id='A' Name='A'><Category Id='C' Name='C'>"
            "<IntProperty Id='Jobs' Name='Jobs' Prefix='-j'/></Category></ToolDescription>");
        CPPUNIT_ASSERT(aResult.oTool.has_value());
        const std::vector<Item> aItems = ParseCommand(*aResult.oTool, { "-j8", "-jx" });
        CPPUNIT_ASSERT_EQUAL(std::string("Jobs"), aItems[0].aPropertyId);
        CPPUNIT_ASSERT(aItems[1].aPropertyId.empty());
    }

    // what was read is written back the same: spellings, order, repeats, the unknown
    void testParseThenFormatGivesTheCommandBack()
    {
        const Strings aCommands[] = {
            { "-DA", "-D", "B=2", "-I", "inc", "-Iinc2", "-isystem", "/sys", "-O3", "-g", "-Wall", "-Wextra",
              "-Werror", "-fPIC", "-fno-exceptions", "-std=gnu++20", "--sysroot=/x", "-pthread" },
            { "-fweird", "-Xclang", "-foo", "-o", "-O1", "-O2", "-O0" },
            { "-march=native", "-march=x86-64", "-I", "-DLAST" },
            {},
        };
        for (const Tool& rTool : { Gcc(), Clang() })
            for (const Strings& rCommand : aCommands)
                CPPUNIT_ASSERT_EQUAL(rCommand, FormatCommand(rTool, ParseCommand(rTool, rCommand)));
    }

    void testFormatBuildsFromProperties()
    {
        const Tool aTool = Gcc();
        const Strings aCommand
            = FormatCommand(aTool, { { "Defines", "FOO", std::nullopt },
                                     { "Includes", "inc", std::nullopt },
                                     { "SystemIncludes", "/s", std::nullopt },
                                     { "Rtti", "false", std::nullopt },
                                     { "Sysroot", "/r", std::nullopt },
                                     { "", "-extra", std::nullopt } });
        CPPUNIT_ASSERT_EQUAL(Strings({ "-DFOO", "-Iinc", "-isystem", "/s", "-fno-rtti", "--sysroot=/r", "-extra" }),
                             aCommand);
    }

    void testBoolWithoutTheSwitchWritesNothing()
    {
        // -Wall has no Off: turning it off is leaving it out
        const Tool aTool = Gcc();
        CPPUNIT_ASSERT_EQUAL(Strings(), FormatCommand(aTool, { { "WarnAll", "false", std::nullopt } }));
    }

    void testDerivedDescriptionAddsAndReplaces()
    {
        const Tool aClang = Clang();
        CPPUNIT_ASSERT_EQUAL(std::string("clang"), aClang.aId);
        CPPUNIT_ASSERT(FindProperty(aClang, "Includes")); // from gcc
        CPPUNIT_ASSERT(FindProperty(aClang, "Stdlib")); // its own, in a category of the base
        const std::vector<Item> aItems = ParseCommand(aClang, { "-stdlib=libc++", "-Weverything" });
        CPPUNIT_ASSERT_EQUAL(std::string("Stdlib"), aItems[0].aPropertyId);
        CPPUNIT_ASSERT_EQUAL(std::string("Weverything"), aItems[1].aPropertyId);
    }

    void testMatchFindsTheTool()
    {
        const Tool aGcc = Gcc();
        const Tool aClang = Clang();
        CPPUNIT_ASSERT(Matches(aGcc, "/usr/bin/g++"));
        CPPUNIT_ASSERT(Matches(aGcc, "/usr/bin/x86_64-linux-gnu-gcc-13"));
        CPPUNIT_ASSERT(Matches(aGcc, "cc"));
        CPPUNIT_ASSERT(!Matches(aGcc, "/usr/bin/clang++"));
        CPPUNIT_ASSERT(Matches(aClang, "/usr/bin/clang++-17"));
        CPPUNIT_ASSERT(!Matches(aClang, "/usr/bin/gcc"));
    }

    void testModeIsFoundByTheSwitch()
    {
        const Tool aTool = Gcc();
        CPPUNIT_ASSERT_EQUAL(std::string("Compile"), DetectMode(aTool, { "-O2", "-c", "-DX" }));
        CPPUNIT_ASSERT_EQUAL(std::string("Assemble"), DetectMode(aTool, { "-S" }));
        CPPUNIT_ASSERT_EQUAL(std::string("Preprocess"), DetectMode(aTool, { "-E" }));
        // no switch: the default one -- gcc links
        CPPUNIT_ASSERT_EQUAL(std::string("Link"), DetectMode(aTool, { "-O2", "-L/x", "-lm" }));
        // the order of the description decides when there are two
        CPPUNIT_ASSERT_EQUAL(std::string("Preprocess"), DetectMode(aTool, { "-c", "-E" }));
        CPPUNIT_ASSERT_EQUAL(std::string(), DetectMode(Parse("<ToolDescription Format='code.tool/1' Id='A' Name='A'/>").oTool.value(), {}));
    }

    void testPropertiesAreForModes()
    {
        const Tool aTool = Gcc();
        const Category* pLinker = nullptr;
        const Category* pPreprocessor = nullptr;
        for (const Category& rCategory : aTool.aCategories)
        {
            if (rCategory.aId == "Linker")
                pLinker = &rCategory;
            if (rCategory.aId == "Preprocessor")
                pPreprocessor = &rCategory;
        }
        CPPUNIT_ASSERT(pLinker && pPreprocessor);
        CPPUNIT_ASSERT(AppliesInMode(pLinker->aModes, "Link"));
        CPPUNIT_ASSERT(!AppliesInMode(pLinker->aModes, "Compile"));
        CPPUNIT_ASSERT(AppliesInMode(pPreprocessor->aModes, "Compile"));
        CPPUNIT_ASSERT(!AppliesInMode(pPreprocessor->aModes, "Link"));
        // the mode is not known: everything applies
        CPPUNIT_ASSERT(AppliesInMode(pLinker->aModes, ""));
        // a category with no Modes is for all
        CPPUNIT_ASSERT(AppliesInMode(FindProperty(aTool, "OptimizationLevel")->aModes, "Link"));
    }

    void testASwitchOfAnotherModeIsStillKnown()
    {
        // -I is for compiling; in a link command it is still an include directory, not "other"
        const Tool aTool = Gcc();
        const std::vector<Item> aItems = ParseCommand(aTool, { "-I/inc", "-lm", "-L", "/lib" });
        CPPUNIT_ASSERT_EQUAL(Strings({ "/inc" }), ValuesOf(aItems, "Includes"));
        CPPUNIT_ASSERT_EQUAL(Strings({ "m" }), ValuesOf(aItems, "Libraries"));
        CPPUNIT_ASSERT_EQUAL(Strings({ "/lib" }), ValuesOf(aItems, "LibraryPaths"));
    }

    void testVersionsAreCompared()
    {
        CPPUNIT_ASSERT_EQUAL(-1, CompareVersions("4.9", "12"));
        CPPUNIT_ASSERT_EQUAL(1, CompareVersions("12.1", "12"));
        CPPUNIT_ASSERT_EQUAL(0, CompareVersions("12", "12.0.0"));
        CPPUNIT_ASSERT_EQUAL(-1, CompareVersions("9.10", "9.11"));
    }

    void testAvailabilityByVersion()
    {
        const Tool aTool = Gcc();
        const Property* pStandard = FindProperty(aTool, "Standard");
        const EnumValue* pOld = nullptr;
        const EnumValue* pNew = nullptr;
        for (const EnumValue& rValue : pStandard->aValues)
        {
            if (rValue.aSwitch == "-std=c++0x")
                pOld = &rValue;
            if (rValue.aSwitch == "-std=c++23")
                pNew = &rValue;
        }
        CPPUNIT_ASSERT(pOld && pNew);
        CPPUNIT_ASSERT(AvailabilityIn(pNew->aLifetime, "10.2") == Availability::NotYet);
        CPPUNIT_ASSERT(AvailabilityIn(pNew->aLifetime, "13") == Availability::Available);
        CPPUNIT_ASSERT(AvailabilityIn(pOld->aLifetime, "4.5") == Availability::Available);
        CPPUNIT_ASSERT(AvailabilityIn(pOld->aLifetime, "4.8") == Availability::Deprecated);
        CPPUNIT_ASSERT(AvailabilityIn(pOld->aLifetime, "11") == Availability::Removed);
        // the version is not known: nothing is said
        CPPUNIT_ASSERT(AvailabilityIn(pOld->aLifetime, "") == Availability::Available);
    }

    void testTheVersionDoesNotChangeWhatIsUnderstood()
    {
        // -std=c++23 in a command of an old compiler is still the Standard property
        const Tool aTool = Gcc();
        const std::vector<Item> aItems = ParseCommand(aTool, { "-std=c++23", "-std=c++0x" });
        CPPUNIT_ASSERT_EQUAL(std::string("Standard"), aItems[0].aPropertyId);
        CPPUNIT_ASSERT_EQUAL(std::string("Standard"), aItems[1].aPropertyId);
    }

    void testModeAndVersionMistakesAreSaid()
    {
        const std::string aHead = "<ToolDescription Format='code.tool/1' Id='A' Name='A'>";
        CPPUNIT_ASSERT(Contains(Error((aHead + "<Category Id='C' Name='C' Modes='Nope'/></ToolDescription>").c_str()),
                                "Mode Nope"));
        CPPUNIT_ASSERT(Contains(Error((aHead + "<Mode Id='M' Name='M'/></ToolDescription>").c_str()),
                                "no Switches"));
        CPPUNIT_ASSERT(Contains(Error((aHead + "<Mode Id='M' Name='M' Default='true'/>"
                                               "<Mode Id='N' Name='N' Default='true'/></ToolDescription>").c_str()),
                                "two Modes are the Default"));
        CPPUNIT_ASSERT(Contains(Error((aHead + "<Category Id='C' Name='C'><BoolProperty Id='B' Name='B' On='-b' Since='x'/>"
                                               "</Category></ToolDescription>").c_str()),
                                "not a version"));
        CPPUNIT_ASSERT(Contains(Error((aHead + "<Category Id='C' Name='C'><BoolProperty Id='B' Name='B' On='-b' Since='5' Until='4'/>"
                                               "</Category></ToolDescription>").c_str()),
                                "not before"));
    }

    void testDefaultFollowsAnotherProperty()
    {
        const Tool aTool = Gcc();
        const Property* pStrict = FindProperty(aTool, "StrictAliasing");
        const Property* pVectorize = FindProperty(aTool, "Vectorize");
        CPPUNIT_ASSERT(pStrict && pVectorize);
        const std::vector<Item> aNone;
        const std::vector<Item> aO2 = ParseCommand(aTool, { "-O2" });
        const std::vector<Item> aO3 = ParseCommand(aTool, { "-O3" });
        // no -O: the property's own Default
        CPPUNIT_ASSERT_EQUAL(std::string("false"), EffectiveDefault(aTool, aNone, *pStrict, ""));
        CPPUNIT_ASSERT_EQUAL(std::string("true"), EffectiveDefault(aTool, aO2, *pStrict, ""));
        // the rule that depends on the version is left out when it is not known, and holds from 12
        CPPUNIT_ASSERT_EQUAL(std::string("false"), EffectiveDefault(aTool, aO2, *pVectorize, ""));
        CPPUNIT_ASSERT_EQUAL(std::string("false"), EffectiveDefault(aTool, aO2, *pVectorize, "11.4"));
        CPPUNIT_ASSERT_EQUAL(std::string("true"), EffectiveDefault(aTool, aO2, *pVectorize, "12.1"));
        CPPUNIT_ASSERT_EQUAL(std::string("true"), EffectiveDefault(aTool, aO3, *pVectorize, ""));
        // what the command gives itself is not a default: the last -O wins as the condition
        const std::vector<Item> aLast = ParseCommand(aTool, { "-O3", "-O0" });
        CPPUNIT_ASSERT_EQUAL(std::string("false"), EffectiveDefault(aTool, aLast, *pVectorize, "13"));
    }

    void testDefaultRuleMistakesAreSaid()
    {
        const std::string aHead = "<ToolDescription Format='code.tool/1' Id='A' Name='A'><Category Id='C' Name='C'>"
                                  "<BoolProperty Id='O' Name='O' On='-o'/>";
        const std::string aTail = "</BoolProperty></Category></ToolDescription>";
        CPPUNIT_ASSERT(Parse((aHead + "<BoolProperty Id='B' Name='B' On='-b'><DefaultIf Property='O' Is='true' Value='true'/>" + aTail).c_str())
                           .oTool.has_value());
        CPPUNIT_ASSERT(Contains(Error((aHead + "<BoolProperty Id='B' Name='B' On='-b'><DefaultIf Property='Nope' Is='x' Value='true'/>" + aTail).c_str()),
                                "depends on Nope"));
        CPPUNIT_ASSERT(Contains(Error((aHead + "<BoolProperty Id='B' Name='B' On='-b'><DefaultIf Property='B' Is='x' Value='true'/>" + aTail).c_str()),
                                "itself"));
        CPPUNIT_ASSERT(Contains(Error((aHead + "<BoolProperty Id='B' Name='B' On='-b'><DefaultIf Property='O' Is='x' Value='maybe'/>" + aTail).c_str()),
                                "true or false"));
        CPPUNIT_ASSERT(Contains(Error((aHead + "<BoolProperty Id='B' Name='B' On='-b'><DefaultIf Property='O' Value='true'/>" + aTail).c_str()),
                                "no Is"));
    }

    void testDerivedDescriptionKeepsTheModes()
    {
        const Tool aClang = Clang();
        CPPUNIT_ASSERT_EQUAL(std::string("Compile"), DetectMode(aClang, { "-c" }));
        CPPUNIT_ASSERT_EQUAL(std::string("Link"), DetectMode(aClang, {}));
    }

    CPPUNIT_TEST_SUITE(ToolDescriptionTest);
    CPPUNIT_TEST(testShippedDescriptionsLoad);
    CPPUNIT_TEST(testFormatIsRequiredAndChecked);
    CPPUNIT_TEST(testNewerFormatIsRefusedWithoutReadingOn);
    CPPUNIT_TEST(testMistakesAreSaid);
    CPPUNIT_TEST(testExtensionsAreSkipped);
    CPPUNIT_TEST(testEnumAndBoolAreKnownByTheirSwitch);
    CPPUNIT_TEST(testOptionsTakeTheirValue);
    CPPUNIT_TEST(testTheLongerOptionWins);
    CPPUNIT_TEST(testWhatIsNotKnownStaysWhereItWas);
    CPPUNIT_TEST(testIntNeedsANumber);
    CPPUNIT_TEST(testParseThenFormatGivesTheCommandBack);
    CPPUNIT_TEST(testFormatBuildsFromProperties);
    CPPUNIT_TEST(testBoolWithoutTheSwitchWritesNothing);
    CPPUNIT_TEST(testDerivedDescriptionAddsAndReplaces);
    CPPUNIT_TEST(testMatchFindsTheTool);
    CPPUNIT_TEST(testModeIsFoundByTheSwitch);
    CPPUNIT_TEST(testPropertiesAreForModes);
    CPPUNIT_TEST(testASwitchOfAnotherModeIsStillKnown);
    CPPUNIT_TEST(testVersionsAreCompared);
    CPPUNIT_TEST(testAvailabilityByVersion);
    CPPUNIT_TEST(testTheVersionDoesNotChangeWhatIsUnderstood);
    CPPUNIT_TEST(testModeAndVersionMistakesAreSaid);
    CPPUNIT_TEST(testDefaultFollowsAnotherProperty);
    CPPUNIT_TEST(testDefaultRuleMistakesAreSaid);
    CPPUNIT_TEST(testDerivedDescriptionKeepsTheModes);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ToolDescriptionTest);
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
