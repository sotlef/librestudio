# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# basctl_dialogs_test (CppunitTest_basctl_dialogs_test.mk, a gb_CppunitTest_
# CppunitScreenShot screenshot target) -- split out of basctl/CMakeLists.txt
# into its own file, include()'d from the root CMakeLists.txt only after
# program_services_rdb exists; see that include() call site for the ordering
# rationale, and svtools/CppunitTest_deferred.cmake's own "svtools_dialogs_
# test: RESOLVED" section for the full NONHEADLESS/UICONFIG mechanism this
# follows identically (this is the same category of test -- vcl_dialogs_test,
# svtools_dialogs_test, svx_dialogs_test, now this one -- that motivated both
# options in cmake/LibreStudioTest.cmake).
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), source/include paths below are still fine -- librestudio_expand_cxx
# resolves them against CMAKE_SOURCE_DIR, never CMAKE_CURRENT_SOURCE_DIR (same
# note as forms/CppunitTest_deferred.cmake's own).
#
# CppunitTest_basctl_dialogs_test.mk, re-checked directly against the live
# file: 1 source (basctl/qa/unit/basctl-dialogs-test), use_sdk_api (normal
# udkapi+offapi, called twice -- a no-op the second time), set_include
# -I$(SRCDIR)/basctl/source/inc -I$(SRCDIR)/basctl/inc plus $$(INCLUDE) (a
# no-op re-inclusion of the existing INCLUDE value -- the same idiom svl_qa_
# cppunit/svtools_dialogs_test's own set_include calls already established),
# use_libraries basegfx comphelper cppu cppuhelper drawinglayer editeng
# i18nlangtag i18nutil msfilter oox sal salhelper sax sfx sot svl svt test tl
# tk ucbhelper unotest utl vcl xo (23 names, identical set to vcl_dialogs_
# test's/svtools_dialogs_test's own list, "test" -> test_fixtures, all 23
# re-verified as real add_library() targets), use_external(boost_headers),
# use_ure + use_vcl_non_headless_with_windows -> 3 protector pairs +
# NONHEADLESS, use_rdb(services) -> DEPENDS program_services_rdb (no
# COMPONENTS, same ini-fallback reasoning as every other use_rdb(services)
# test in this port), use_configuration -> CONFIGURATION, use_uiconfigs(
# modules/BasicIDE) -> UICONFIG basicide (basctl/CMakeLists.txt's own
# "basicide_uiconfig" ALL target, introduced alongside this wiring -- see
# that file's own UIConfig_basicide comment for why "basicide", not
# "modules/BasicIDE", is the target name passed here).
#
# Notably absent from use_libraries: "basctl" itself. This screenshot test
# never links the basctl production library (it drives basctl's .ui files
# straight through VCL's dialog-builder machinery), so it needs no guard
# tied to ENABLE_SCRIPTING the way basctl/CMakeLists.txt's own library target
# does -- it is wired here unconditionally, matching Module_basctl.mk, whose
# gb_Module_add_screenshot_targets call for this test sits outside (after)
# the "ifneq ($(filter SCRIPTING,$(BUILD_TYPE)),)" block that gates Library_
# basctl/UIConfig_basicide. Its UICONFIG dependency on basicide_uiconfig
# still requires ENABLE_SCRIPTING to be on for that target to exist -- true
# by this port's own default, same caveat basctl/CMakeLists.txt's own
# UIConfig_basicide comment already flags.
librestudio_add_cppunit_test(basctl_dialogs_test
    SOURCES
        basctl/qa/unit/basctl-dialogs-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
        i18nutil msfilter oox sal salhelper sax sfx sot svl svt test_fixtures
        tl tk ucbhelper unotest utl vcl xo Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/basctl/source/inc"
        "${CMAKE_SOURCE_DIR}/basctl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    NONHEADLESS
    UICONFIG basicide
    CONFIGURATION)
