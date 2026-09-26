# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# svtools_graphic (CppunitTest_svtools_graphic.mk) -- split out of svtools/
# CMakeLists.txt into its own file, include()'d from the root CMakeLists.txt
# only after program_services_rdb exists (and sw is add_subdirectory()'d),
# the same reason and call-site pattern as scaddins/CppunitTest_deferred.cmake,
# xmlscript/CppunitTest_deferred.cmake and vcl/CppunitTest_deferred.cmake --
# read scaddins' file header comment first for the full use_rdb(services)/
# ini-fallback reasoning, which applies identically here.
#
# CppunitTest_svtools_graphic.mk, re-checked directly against the live file:
# 1 source (svtools/qa/unit/GraphicObjectTest), use_externals(boost_headers),
# use_api(udkapi,offapi,oovbaapi) (normal variant), use_ure, use_vcl -> 3
# protector pairs (unobootstrapprotector x2, unoexceptionprotector x2,
# vclbootstrapprotector x2 -- same precedent as vcl_app_test and friends in
# vcl/CppunitTest_deferred.cmake), use_libraries comphelper cppu cppuhelper
# tl sal svt sw test unotest vcl ("test" -> test_fixtures, this port's
# established rename), set_include -I sw/inc (plus $$(INCLUDE), a no-op --
# same as svl_qa_cppunit's own set_include call, already fully subsumed by
# use_api's normal udkapi/offapi/oovbaapi headers), use_custom_headers
# (officecfg/registry) -> OFFICECFG_INCLUDE_DIR + DEPENDS officecfg_headers
# (same precedent as vcl_filter_igif's own use_custom_headers(officecfg/
# registry) wiring in vcl/CppunitTest_deferred.cmake -- vcl_graphic_test
# itself, despite the similar name, does NOT call use_custom_headers,
# re-checked directly), use_configuration -> CONFIGURATION, use_rdb(services)
# -> DEPENDS program_services_rdb, no COMPONENTS (cppunittester's UNO
# bootstrap falls back to unorc's own ini default for UNO_SERVICES, same as
# every other use_rdb(services) test in this port).
#
# sw really is needed and exists: confirmed ported earlier this session
# (sw/CMakeLists.txt: add_library(sw SHARED ...)); linking it here needs sw's
# own add_subdirectory() (root CMakeLists.txt line 795) to have already run
# for target_link_libraries(svtools_graphic PRIVATE ... sw ...) to resolve sw
# as a real CMake target (carrying its usage requirements and build-order
# edge) rather than degrade to a bare "-lsw" linker flag -- this file's
# include() site, right after vcl/CppunitTest_deferred.cmake's own (root
# CMakeLists.txt, itself right after program_services_rdb), sits well after
# line 795, so this is satisfied together with the program_services_rdb
# constraint above, not separately.
librestudio_add_cppunit_test(svtools_graphic
    SOURCES
        svtools/qa/unit/GraphicObjectTest
    LIBRARIES
        comphelper cppu cppuhelper tl sal svt sw test_fixtures unotest vcl
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sw/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- svtools_dialogs_test: RESOLVED --------------------------------------------
#
# CppunitTest_svtools_dialogs_test.mk (a gb_CppunitTest_CppunitScreenShot
# screenshot target, calling use_vcl_non_headless_with_windows +
# use_uiconfigs(svt)) was previously NOT wired here, or anywhere -- see
# svtools/CMakeLists.txt's own (now RESOLVED) comment, right after
# CppunitTest_svtools_html, for the old reasoning. The real gap was
# librestudio_add_cppunit_test() having no HEADLESS-toggle nor use_uiconfigs
# equivalent at all, plus UIConfig_svt (svtools/CMakeLists.txt) being
# install()-only, never a real build-time target. cmake/LibreStudioTest.cmake's
# NONHEADLESS and UICONFIG options (added this session -- read that file's
# header comment for the full mechanism, including the confirmed finding
# that this port has never forced --headless/SAL_USE_VCLPLUGIN=svp anywhere)
# close that gap, and UIConfig_svt now also defines a real build-time ALL
# target (svt_uiconfig) for UICONFIG to depend on. Wired below for real.
#
# CppunitTest_svtools_dialogs_test.mk, re-checked directly against the live
# file: 1 source (svtools/qa/unit/svtools-dialogs-test), use_external(
# boost_headers), use_sdk_api (normal udkapi+offapi, called twice -- a no-op
# the second time), set_include -I$(SRCDIR)/svtools/inc plus $$(INCLUDE) (a
# no-op re-inclusion of the existing INCLUDE value, same idiom svl_qa_
# cppunit's own set_include call already established -- the only real add is
# svtools/inc), use_libraries basegfx comphelper cppu cppuhelper drawinglayer
# editeng i18nlangtag i18nutil msfilter oox sal salhelper sax sfx sot svl svt
# test tl tk ucbhelper unotest utl vcl xo (23 names, identical set to vcl_
# dialogs_test's own list in vcl/CppunitTest_deferred.cmake, "test" ->
# test_fixtures, all 23 re-verified as real add_library() targets), use_ure +
# use_vcl_non_headless_with_windows -> 3 protector pairs + NONHEADLESS,
# use_rdb(services) -> DEPENDS program_services_rdb (no COMPONENTS, same
# ini-fallback reasoning as svtools_graphic above), use_configuration ->
# CONFIGURATION, use_uiconfigs(svt) -> UICONFIG svt.
librestudio_add_cppunit_test(svtools_dialogs_test
    SOURCES
        svtools/qa/unit/svtools-dialogs-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
        i18nutil msfilter oox sal salhelper sax sfx sot svl svt test_fixtures
        tl tk ucbhelper unotest utl vcl xo Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/svtools/inc"
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
    UICONFIG svt
    CONFIGURATION)
