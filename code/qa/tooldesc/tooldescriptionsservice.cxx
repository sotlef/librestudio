/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// theToolDescriptions as a client sees it: found through UNO, reading the
// descriptions that are installed, and answering in the service's own types.

#include <com/sun/star/code/tool/CommandDescription.hpp>
#include <com/sun/star/code/tool/XToolDescriptions.hpp>
#include <com/sun/star/code/tool/theToolDescriptions.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <comphelper/processfactory.hxx>

#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/plugin/TestPlugIn.h>

using namespace css;
using namespace css::code::tool;

namespace
{
uno::Sequence<OUString> Args(std::initializer_list<const char16_t*> aList)
{
    std::vector<OUString> aArguments;
    for (const char16_t* p : aList)
        aArguments.push_back(OUString(p));
    return uno::Sequence<OUString>(aArguments.data(), aArguments.size());
}

const CommandProperty* PropertyOf(const CommandDescription& rCommand, const OUString& rId)
{
    for (const CommandCategory& rCategory : rCommand.Categories)
        for (const CommandProperty& rProperty : rCategory.Properties)
            if (rProperty.Id == rId)
                return &rProperty;
    return nullptr;
}

class ToolDescriptionsServiceTest : public CppUnit::TestFixture
{
public:
    void setUp() override { m_xTools = theToolDescriptions::get(comphelper::getProcessComponentContext()); }

    void testToolsAreFoundByExecutable()
    {
        CPPUNIT_ASSERT(m_xTools->getTools().hasElements());
        CPPUNIT_ASSERT_EQUAL(u"gcc"_ustr, m_xTools->findTool(u"/usr/bin/g++-13"_ustr));
        // clang builds on gcc's description and is the better answer for itself
        CPPUNIT_ASSERT_EQUAL(u"clang"_ustr, m_xTools->findTool(u"/usr/bin/clang++"_ustr));
        CPPUNIT_ASSERT_EQUAL(OUString(), m_xTools->findTool(u"/usr/bin/rustc"_ustr));
    }

    void testCommandIsSortedIntoCategories()
    {
        const CommandDescription aCommand = m_xTools->describeCommand(
            u"gcc"_ustr, OUString(), OUString(),
            Args({ u"-c", u"-O2", u"-DFOO", u"-I/inc", u"-fno-rtti", u"-fmystery" }));
        CPPUNIT_ASSERT_EQUAL(u"Compile"_ustr, aCommand.Mode);
        CPPUNIT_ASSERT(PropertyOf(aCommand, u"OptimizationLevel"_ustr));
        CPPUNIT_ASSERT_EQUAL(u"Maximize speed (-O2)"_ustr, PropertyOf(aCommand, u"OptimizationLevel"_ustr)->Values[0]);
        CPPUNIT_ASSERT_EQUAL(u"No"_ustr, PropertyOf(aCommand, u"Rtti"_ustr)->Values[0]);
        CPPUNIT_ASSERT_EQUAL(u"/inc"_ustr, PropertyOf(aCommand, u"Includes"_ustr)->Values[0]);
        // what the description does not know is not lost
        CPPUNIT_ASSERT_EQUAL(sal_Int32(1), aCommand.OtherArguments.getLength());
        CPPUNIT_ASSERT_EQUAL(u"-fmystery"_ustr, aCommand.OtherArguments[0]);
    }

    void testModeAndVersionColourWhatIsSaid()
    {
        // a link command: -I is known, and says that it does nothing there
        const CommandDescription aLink
            = m_xTools->describeCommand(u"gcc"_ustr, OUString(), OUString(), Args({ u"-I/inc", u"-lm" }));
        CPPUNIT_ASSERT_EQUAL(u"Link"_ustr, aLink.Mode);
        CPPUNIT_ASSERT(PropertyOf(aLink, u"Includes"_ustr)->State == CommandPropertyState_NOT_IN_MODE);
        CPPUNIT_ASSERT(PropertyOf(aLink, u"Libraries"_ustr)->State == CommandPropertyState_AVAILABLE);

        // an old compiler does not have C++23, but the argument is still understood
        const CommandDescription aOld = m_xTools->describeCommand(
            u"gcc"_ustr, u"Compile"_ustr, u"10.2"_ustr, Args({ u"-std=c++23" }));
        const CommandProperty* pStandard = PropertyOf(aOld, u"Standard"_ustr);
        CPPUNIT_ASSERT(pStandard);
        CPPUNIT_ASSERT(pStandard->State == CommandPropertyState_NOT_YET);
        CPPUNIT_ASSERT_EQUAL(u"11"_ustr, pStandard->Detail);
        CPPUNIT_ASSERT_EQUAL(sal_Int32(0), aOld.OtherArguments.getLength());
    }

    void testParseAndFormatGiveTheArgumentsBack()
    {
        const uno::Sequence<OUString> aArguments = Args({ u"-DA", u"-I", u"inc", u"-O3", u"-fmystery", u"-isystem", u"/s" });
        CPPUNIT_ASSERT(m_xTools->formatCommand(u"gcc"_ustr, m_xTools->parseCommand(u"gcc"_ustr, aArguments)) == aArguments);
    }

    void testPropertiesNotGivenShowTheirDefaults()
    {
        const CommandDescription aCommand
            = m_xTools->describeCommand(u"gcc"_ustr, OUString(), OUString(), Args({ u"-c", u"-O2" }));
        // not on the command line: the tool uses its default, and the dialog says so
        const CommandProperty* pRtti = PropertyOf(aCommand, u"Rtti"_ustr);
        CPPUNIT_ASSERT(pRtti && pRtti->IsDefault);
        CPPUNIT_ASSERT_EQUAL(u"Yes"_ustr, pRtti->Values[0]);
        // one that is given is not
        CPPUNIT_ASSERT(!PropertyOf(aCommand, u"OptimizationLevel"_ustr)->IsDefault);
        // the linker's options do not apply when compiling, and are not listed
        CPPUNIT_ASSERT(!PropertyOf(aCommand, u"LibraryPaths"_ustr));
        // many more than the few the command names
        sal_Int32 nCount = 0;
        for (const CommandCategory& rCategory : aCommand.Categories)
            nCount += rCategory.Properties.getLength();
        CPPUNIT_ASSERT(nCount > 100);
    }

    void testDefaultsFollowTheOptimizationLevel()
    {
        // the same property, not given: off at -O0, on at -O2
        const CommandDescription aO0 = m_xTools->describeCommand(u"gcc"_ustr, OUString(), u"13"_ustr, Args({ u"-c" }));
        const CommandDescription aO2 = m_xTools->describeCommand(u"gcc"_ustr, OUString(), u"13"_ustr, Args({ u"-c", u"-O2" }));
        CPPUNIT_ASSERT_EQUAL(u"No"_ustr, PropertyOf(aO0, u"StrictAliasing"_ustr)->Values[0]);
        CPPUNIT_ASSERT(PropertyOf(aO2, u"StrictAliasing"_ustr)->IsDefault);
        CPPUNIT_ASSERT_EQUAL(u"Yes"_ustr, PropertyOf(aO2, u"StrictAliasing"_ustr)->Values[0]);
        // and a flag the command gives says what it says, whatever the level
        const CommandDescription aGiven
            = m_xTools->describeCommand(u"gcc"_ustr, OUString(), u"13"_ustr, Args({ u"-c", u"-O2", u"-fno-strict-aliasing" }));
        CPPUNIT_ASSERT(!PropertyOf(aGiven, u"StrictAliasing"_ustr)->IsDefault);
        CPPUNIT_ASSERT_EQUAL(u"No"_ustr, PropertyOf(aGiven, u"StrictAliasing"_ustr)->Values[0]);
    }

    void testAFileIsShownAgainstItsTarget()
    {
        // the target: -O2 and C++17; the file: -O0, C++17 as it was, and -DONLY_HERE
        const uno::Sequence<OUString> aTarget = Args({ u"-O2", u"-std=c++17", u"-DALL", u"-Wall" });
        const CommandDescription aFile = m_xTools->describeOverrides(
            u"gcc"_ustr, u"Compile"_ustr, OUString(), aTarget,
            Args({ u"-O0", u"-std=c++17", u"-DALL", u"-DONLY_HERE" }));

        const CommandProperty* pOptimization = PropertyOf(aFile, u"OptimizationLevel"_ustr);
        CPPUNIT_ASSERT(pOptimization && pOptimization->Overridden);
        CPPUNIT_ASSERT_EQUAL(u"Maximize speed (-O2)"_ustr, pOptimization->InheritedValues[0]);
        // the same as the target's is not an override
        CPPUNIT_ASSERT(!PropertyOf(aFile, u"Standard"_ustr)->Overridden);
        // the definitions differ: a list with one more is an override too
        CPPUNIT_ASSERT(PropertyOf(aFile, u"Defines"_ustr)->Overridden);
        // what the target has and the file leaves out is what the tool does without it: its default
        const CommandProperty* pWarnAll = PropertyOf(aFile, u"WarnAll"_ustr);
        CPPUNIT_ASSERT(pWarnAll && pWarnAll->Overridden && pWarnAll->IsDefault);
        CPPUNIT_ASSERT_EQUAL(u"No"_ustr, pWarnAll->Values[0]);
        CPPUNIT_ASSERT_EQUAL(u"Yes"_ustr, pWarnAll->InheritedValues[0]);

        // a file compiled like its target departs from nothing
        const CommandDescription aSame
            = m_xTools->describeOverrides(u"gcc"_ustr, u"Compile"_ustr, OUString(), aTarget, aTarget);
        for (const CommandCategory& rCategory : aSame.Categories)
            for (const CommandProperty& rProperty : rCategory.Properties)
                CPPUNIT_ASSERT(!rProperty.Overridden);
    }

    void testUnknownToolIsAnError()
    {
        CPPUNIT_ASSERT_THROW(m_xTools->detectMode(u"nope"_ustr, Args({})), lang::IllegalArgumentException);
        CPPUNIT_ASSERT_THROW(m_xTools->describeCommand(u"nope"_ustr, OUString(), OUString(), Args({})),
                             lang::IllegalArgumentException);
    }

    CPPUNIT_TEST_SUITE(ToolDescriptionsServiceTest);
    CPPUNIT_TEST(testToolsAreFoundByExecutable);
    CPPUNIT_TEST(testCommandIsSortedIntoCategories);
    CPPUNIT_TEST(testModeAndVersionColourWhatIsSaid);
    CPPUNIT_TEST(testParseAndFormatGiveTheArgumentsBack);
    CPPUNIT_TEST(testPropertiesNotGivenShowTheirDefaults);
    CPPUNIT_TEST(testDefaultsFollowTheOptimizationLevel);
    CPPUNIT_TEST(testAFileIsShownAgainstItsTarget);
    CPPUNIT_TEST(testUnknownToolIsAnError);
    CPPUNIT_TEST_SUITE_END();

private:
    uno::Reference<XToolDescriptions> m_xTools;
};

CPPUNIT_TEST_SUITE_REGISTRATION(ToolDescriptionsServiceTest);
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
