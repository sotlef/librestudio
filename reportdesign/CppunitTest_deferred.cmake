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
# now wired for real too, gated behind if (LIBRESTUDIO_ENABLE_JAVA) below --
# its former blocker, connectivity's Jar_sdbc_hsqldb, is now ported
# (connectivity/CMakeLists.txt's own "--- sdbc_hsqldb ---" section, plus the
# bundled hsqldb external next to java_websocket in root CMakeLists.txt).
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

# --- reportdesign_basic_test (CppunitTest_reportdesign_basic_test.mk) --------
#
# Module_reportdesign.mk: `ifeq ($(ENABLE_JAVA),TRUE)` around this test's own
# gb_Module_add_check_targets call -- gated behind LIBRESTUDIO_ENABLE_JAVA
# below, this port's own real, tested (default OFF) switch. Re-checked
# directly against the live .mk: use_externals(boost_headers,libxml2), 1
# source (reportdesign/qa/unit/ReportDesignBasicTests), set_include(-I
# reportdesign/source/inc -I reportdesign/inc), use_libraries comphelper cppu
# cppuhelper dbaxml dbtools sal subsequenttest utl test tk tl unotest xo
# ("test" -> test_fixtures), use_api(offapi oovbaapi udkapi) -> the normal
# variant of all three (use_api, not use_sdk_api -- oovbaapi's own normal
# headers target already exists, wired the same way editeng/CMakeLists.txt's
# CppunitTest_deferred.cmake consumes it, see postprocess/CMakeLists.txt's
# own oovbaapi citation for another precedent), use_ure + use_vcl -> 3
# protector pairs, use_rdb(services) -> DEPENDS program_services_rdb,
# use_configuration -> CONFIGURATION, use_jars(sdbc_hsqldb) -> DEPENDS
# sdbc_hsqldb_jar (connectivity/CMakeLists.txt, a build-order-only
# dependency -- the jar is presumably loaded via the Java UNO bridge at
# runtime, never linked into this native C++ test binary, matching gb_
# CppunitTest_use_jars' own gb_CppunitTest_use_jar -> plain target
# prerequisite, no compile/link flags at all).
#
# use_uiconfigs(reportdesign_basic_test, reportdesign) is a genuine upstream
# oddity, not a porting gap: grepped this entire tree (including every
# UIConfig_*.mk) for "UIConfig_reportdesign" -- zero hits anywhere.
# Module_reportdesign.mk itself only ever adds "UIConfig_dbreport" (see its
# own gb_Module_add_targets call, and reportdesign_dialogs_test's own
# UICONFIG dbreport above) -- there is no "UIConfig_reportdesign" target
# anywhere in upstream LibreOffice for gb_UIConfig_get_target(reportdesign)
# to resolve to, so this exact dependency could not be satisfied even by a
# complete, real upstream gbuild build. Read as an upstream typo for
# "dbreport" (the module's only real UIConfig, already wired here as
# dbreport_uiconfig) and wired that way below, rather than left broken or
# given a fabricated "reportdesign_uiconfig" target that would correspond to
# nothing real.
if (LIBRESTUDIO_ENABLE_JAVA)
    librestudio_add_cppunit_test(reportdesign_basic_test
        SOURCES
            reportdesign/qa/unit/ReportDesignBasicTests
        LIBRARIES
            comphelper cppu cppuhelper dbaxml dbtools sal subsequenttest
            test_fixtures tk tl unotest utl xo Boost::boost LibXml2::LibXml2
        INCLUDE
            "${CMAKE_SOURCE_DIR}/reportdesign/source/inc"
            "${CMAKE_SOURCE_DIR}/reportdesign/inc"
            "${udkapi_normal_HEADERS_INCLUDE_DIR}"
            "${offapi_normal_HEADERS_INCLUDE_DIR}"
            "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
            "${LIBXML2_INCLUDE_DIRS}"
        DEPENDS
            udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
            program_services_rdb sdbc_hsqldb_jar
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
            vclbootstrapprotector vclbootstrapprotector
        UICONFIG dbreport
        CONFIGURATION)
endif ()
