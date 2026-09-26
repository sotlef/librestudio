# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# reportdesign_dialogs_test (CppunitTest_reportdesign_dialogs_test.mk, a
# gb_CppunitTest_CppunitScreenShot screenshot target) -- split out of
# reportdesign/CMakeLists.txt into its own file, include()'d from the root
# CMakeLists.txt only after program_services_rdb exists; see that include()
# call site for the ordering rationale, and svtools/CppunitTest_
# deferred.cmake's own "svtools_dialogs_test: RESOLVED" section (or basctl/
# CppunitTest_deferred.cmake's own basctl_dialogs_test, the same category of
# test ported alongside this one) for the full NONHEADLESS/UICONFIG
# mechanism this follows identically.
#
# CppunitTest_reportdesign_basic_test.mk (the module's other CppunitTest) is
# NOT wired anywhere -- see reportdesign/CMakeLists.txt's own "documented gap"
# comment on it (ENABLE_JAVA off by default, plus its use_jars(sdbc_hsqldb)
# dependency on an SDBC driver connectivity/CMakeLists.txt already documents
# as out of scope).
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), source/include paths below are still fine -- librestudio_expand_cxx
# resolves them against CMAKE_SOURCE_DIR, never CMAKE_CURRENT_SOURCE_DIR (same
# note as forms/CppunitTest_deferred.cmake's own).
#
# CppunitTest_reportdesign_dialogs_test.mk, re-checked directly against the
# live file: 1 source (reportdesign/qa/unit/reportdesign-dialogs-test),
# use_sdk_api (normal udkapi+offapi, called twice -- a no-op the second
# time), NO set_include call (unlike basctl_dialogs_test's own -- re-checked,
# this .mk really has none), use_libraries basegfx comphelper cppu cppuhelper
# drawinglayer editeng i18nlangtag i18nutil msfilter oox sal salhelper sax
# sfx sot svl svt test tl tk ucbhelper unotest utl vcl xo (the identical
# 23-name set basctl_dialogs_test/vcl_dialogs_test/svtools_dialogs_test all
# share, "test" -> test_fixtures), use_external(boost_headers), use_ure +
# use_vcl_non_headless_with_windows -> 3 protector pairs + NONHEADLESS,
# use_rdb(services) -> DEPENDS program_services_rdb (no COMPONENTS, same
# ini-fallback reasoning as every other use_rdb(services) test here),
# use_configuration -> CONFIGURATION, use_uiconfigs(modules/dbreport) ->
# UICONFIG dbreport (reportdesign/CMakeLists.txt's own "dbreport_uiconfig"
# ALL target).
#
# Notably absent from use_libraries: "rpt"/"rptui" themselves -- same "drives
# .ui files through VCL's dialog-builder machinery, not the production
# library" shape as basctl_dialogs_test, so no ordering dependency on those
# two targets either (both are add_subdirectory()'d unconditionally anyway,
# unlike basctl's own ENABLE_SCRIPTING-gated library).
librestudio_add_cppunit_test(reportdesign_dialogs_test
    SOURCES
        reportdesign/qa/unit/reportdesign-dialogs-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
        i18nutil msfilter oox sal salhelper sax sfx sot svl svt test_fixtures
        tl tk ucbhelper unotest utl vcl xo Boost::boost
    INCLUDE
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
    UICONFIG dbreport
    CONFIGURATION)
