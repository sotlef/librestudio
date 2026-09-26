# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# toolkit_toolkit (CppunitTest_toolkit.mk) -- split out of toolkit/
# CMakeLists.txt into its own file, include()'d from the root CMakeLists.txt
# only after every module its COMPONENTS reference has already had its own
# add_subdirectory(), the same pattern as svl/CppunitTest_deferred.cmake and
# vcl/CppunitTest_deferred.cmake (read either file's own header comment for
# the general mechanism).
#
# CppunitTest_toolkit.mk, re-checked directly against the live file: 4
# sources (toolkit/qa/cppunit/Dialog, EventContainer, FontVariations,
# UnitConversion), use_libraries basegfx comphelper cppu cppuhelper sal
# salhelper test tk tl unotest vcl ("test" -> test_fixtures, this port's
# established rename -- see vcl/CppunitTest_deferred.cmake's own precedent),
# use_api(offapi,udkapi) (normal variant), use_externals(boost_headers),
# use_ure + use_vcl -> 3 protector pairs (unobootstrapprotector x2,
# unoexceptionprotector x2, vclbootstrapprotector x2 -- same precedent as
# vcl_app_test and friends in vcl/CppunitTest_deferred.cmake), use_
# configuration -> CONFIGURATION, use_more_fonts (see below -- N/A/inert,
# nothing to wire), use_components (8 names, resolved below).
#
# use_more_fonts (solenv/gbuild/CppunitTest.mk gb_CppunitTest_use_more_fonts,
# solenv/gbuild/TestHelpers.mk gb_TestHelpers_use_more_fonts): gated on
# "ifneq ($(filter MORE_FONTS,$(BUILD_TYPE)),)" and, in single-module mode,
# on whether a "more_fonts" module exists at all -- makes the test additionally
# depend on the packaged extras_font/more_fonts font files and denies use of
# any "non-application" font. This port's own BUILD_TYPE equivalent,
# LIBRESTUDIO_BUILD_TYPE (vcl/CMakeLists.txt), defaults to just "DESKTOP" --
# "MORE_FONTS" is never in it -- and there is no more_fonts module/Package_
# extras_font anywhere in this tree (checked: no CMakeLists.txt or *.cmake
# file anywhere references "more_fonts"). So this is genuinely inert here,
# the same way vcl's PDFIUM-gated branches are inert when PDFIUM is not in
# LIBRESTUDIO_BUILD_TYPE -- nothing is wired for it, not invented around.
#
# COMPONENTS lookup keys, each verified against the owning module's own
# librestudio_add_component() call (grep -n "librestudio_add_component("):
#   - "comphelper/util/comphelp" -> "comphelper" (comphelper/CMakeLists.txt).
#   - "configmgr/source/configmgr" -> "configmgr" (configmgr/CMakeLists.txt).
#   - "framework/util/fwk" -> "fwk" (framework/CMakeLists.txt).
#   - "i18npool/util/i18npool" -> "i18npool" (i18npool/CMakeLists.txt).
#   - "sfx2/util/sfx" -> "sfx" (sfx2/CMakeLists.txt).
#   - "toolkit/util/tk" -> "tk" (toolkit/CMakeLists.txt line 117 --
#     genuinely self-referential: toolkit/CMakeLists.txt does register tk's
#     own component via librestudio_add_component(tk ...), so this resolves
#     fine; the test just waits for tk's own component file to exist, same as
#     everyone else's).
#   - "ucb/source/core/ucb1" -> "ucb1" and "ucb/source/ucp/file/ucpfile1" ->
#     "ucpfile1" (both ucb/CMakeLists.txt) -- same mapping already
#     established by svl/CppunitTest_deferred.cmake's svl_urihelper.
# All eight resolve to real, already-registered components -- nothing here
# needed to be flagged as unresolved.
#
# Binding ordering constraint: framework (fwk, add_subdirectory()'d at line
# 517), sfx2 (sfx, line 527) and ucb (ucb1/ucpfile1, line 592) are all
# add_subdirectory()'d well AFTER toolkit's own natural position (line 491),
# and CONFIGURATION additionally requires officecfg (390, already satisfied
# at toolkit's own position) and test (unotest/test, lines 681-682, NOT yet
# satisfied there) -- so this test could not be registered at toolkit/
# CMakeLists.txt's own point either way. The latest of these is test (682);
# this file is include()'d right after svl/CppunitTest_deferred.cmake's own
# include() site (root CMakeLists.txt), which sits well after both ucb and
# test, satisfying every one of this test's dependencies. No use_rdb(services)
# here (the test uses explicit COMPONENTS instead), so unlike svtools_
# graphic below, program_services_rdb is not a constraint for this test.
librestudio_add_cppunit_test(toolkit_toolkit
    SOURCES
        toolkit/qa/cppunit/Dialog
        toolkit/qa/cppunit/EventContainer
        toolkit/qa/cppunit/FontVariations
        toolkit/qa/cppunit/UnitConversion
    LIBRARIES
        basegfx comphelper cppu cppuhelper sal salhelper test_fixtures tk tl
        unotest vcl Boost::boost
    INCLUDE
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS offapi_normal_headers udkapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        comphelper configmgr fwk i18npool sfx tk ucb1 ucpfile1
    CONFIGURATION)

# toolkit_a11y (CppunitTest_toolkit_a11y.mk): RESOLVED, but not wired above
# -- its former blocker, "subsequenttest" (re-checked directly against the
# live .mk, line 24), is now ported as a real target (test/CMakeLists.txt's
# add_library(subsequenttest SHARED ...); this is the same gap that was
# documented for CppunitTest_officecfg_accelerator_key_checker_test, now
# resolved there too, see officecfg/CppunitTest_deferred.cmake). It is not
# wired into toolkit_toolkit's own include() point above, though: this test
# also calls gb_CppunitTest_use_rdb,toolkit_a11y,services (re-verified
# against the live .mk, line 49 -- the leading "toolkit_a11y" is just
# gb_CppunitTest_use_rdb's own $(1) test-name parameter, the same $(call)
# convention every other use_rdb call site in this tree uses, e.g. sc's
# CppunitTest_sc_core.mk's "gb_CppunitTest_use_rdb,sc_core,services" --
# solenv/gbuild/CppunitTest.mk:326-330 takes exactly ($1)=test-name,
# ($2)=rdb-name, so this is the plain use_rdb(services) shape, not a
# different named-target form), which needs the program_services_rdb target
# that only exists near the very end of the root CMakeLists.txt -- well
# after this file's own current include() point (root CMakeLists.txt, right
# after svl/CppunitTest_deferred.cmake's own include(), long before
# program_services_rdb is defined). toolkit_toolkit itself has no such
# constraint (it uses explicit COMPONENTS instead of use_rdb, see its own
# comment above), but since both tests share this one file's single
# include() call site, that site was moved later in the root CMakeLists.txt
# (to right after program_services_rdb is defined, alongside scaddins_
# analysis/xmlscript_cppunit) rather than splitting toolkit_a11y into a
# second file -- registering toolkit_toolkit a little later than strictly
# necessary costs nothing, the same tradeoff svl/CppunitTest_deferred.cmake's
# own combined file already makes for svl_urihelper. See the root
# CMakeLists.txt include() call site for the updated ordering comment.
#
# CppunitTest_toolkit_a11y.mk, re-checked directly against the live file: 2
# sources (toolkit/qa/cppunit/a11y/AccessibleFixedTextTest,
# AccessibleStatusBarTest), use_libraries basegfx comphelper cppu cppuhelper
# sal salhelper subsequenttest test tk tl unotest vcl ("test" -> test_
# fixtures, same rename as toolkit_toolkit above), use_api(offapi,udkapi)
# and use_sdk_api (both resolve to the identical normal-variant udkapi+offapi
# headers -- gb_CppunitTest_use_sdk_api is defined as exactly gb_CppunitTest_
# use_api($1,"udkapi offapi"), solenv/gbuild/CppunitTest.mk:321-324 -- so the
# .mk's two separate calls are redundant with each other, not two different
# header sets; only one INCLUDE/DEPENDS pair is needed here), use_externals
# (boost_headers) -> Boost::boost, use_ure + use_vcl -> 3 protector pairs
# (same precedent as toolkit_toolkit above), use_configuration ->
# CONFIGURATION, use_rdb(services) -> DEPENDS program_services_rdb (no
# use_components in this .mk, so no COMPONENTS -- same treatment as
# scaddins_analysis/xmlscript_cppunit, see scaddins/CppunitTest_deferred.cmake
# for the ini-fallback reasoning).
#
# All libraries verified as real targets before wiring (grep -n
# "add_library(<name> "): basegfx, comphelper, cppu, cppuhelper, sal,
# salhelper, subsequenttest (test/CMakeLists.txt), test_fixtures (test/
# CMakeLists.txt), tk (toolkit/CMakeLists.txt, this module's own target),
# unotest, vcl -- all resolve. unobootstrapprotector/unoexceptionprotector
# (unotest/CMakeLists.txt) and vclbootstrapprotector (test/CMakeLists.txt)
# resolve too, same protector targets toolkit_toolkit above already uses.
librestudio_add_cppunit_test(toolkit_a11y
    SOURCES
        toolkit/qa/cppunit/a11y/AccessibleFixedTextTest
        toolkit/qa/cppunit/a11y/AccessibleStatusBarTest
    LIBRARIES
        basegfx comphelper cppu cppuhelper sal salhelper subsequenttest
        test_fixtures tk tl unotest vcl Boost::boost
    INCLUDE
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        offapi_normal_headers udkapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)
