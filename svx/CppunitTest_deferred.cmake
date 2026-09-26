# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# svx_unit (CppunitTest_svx_unit.mk), svx_gallery_test (CppunitTest_svx_
# gallery_test.mk) and svx_core (CppunitTest_svx_core.mk) -- split out of
# svx/CMakeLists.txt into their own file, include()'d from the root
# CMakeLists.txt only after program_services_rdb exists: all three call
# gb_CppunitTest_use_rdb(name, services) (see scaddins/CppunitTest_deferred.
# cmake's header comment first for the full use_rdb(services)/ini-fallback
# reasoning, which applies identically here -- no COMPONENTS, a DEPENDS on
# program_services_rdb, and cppunittester's UNO bootstrap falling back to
# unorc's own ini default for UNO_SERVICES). All three also need unotest
# and/or test_fixtures and/or subsequenttest (test/CMakeLists.txt, unotest/
# CMakeLists.txt), both add_subdirectory()'d AFTER svx's own position in the
# root CMakeLists.txt -- but program_services_rdb (defined much later still,
# right after librestudio_finalize_components(services)) is the later-
# binding constraint for all three, so they are registered at that point,
# the same one scaddins_analysis/xmlscript_cppunit/vcl's own eight tests/
# svtools_graphic are already deferred to.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not svx's, anything svx/CMakeLists.txt already built as a real CMake
# target (svxcore, svx, gcc3_uno, vclmain, etc.) is still visible here --
# nothing directory-scoped that svx/CMakeLists.txt set up (it creates no
# IMPORTED_TARGET of its own) needs to be re-derived.

# --- svx_unit ----------------------------------------------------------------
#
# CppunitTest_svx_unit.mk, re-checked directly against the live file: 13
# sources (svx/qa/unit/svdraw/test_SdrTextObject, customshapes,
# classicshapes, framelinkarray, gluepointTest, sdr, svdraw, table, unodraw,
# xoutdev, xml, XTableImportExportTest, ThemeTest -- all verified to exist on
# disk), use_externals(boost_headers libxml2), use_sdk_api (normal udkapi+
# offapi), use_libraries (basegfx docmodel drawinglayer drawinglayercore
# editeng sal subsequenttest sfx svl svxcore svx tl unotest vcl utl comphelper
# cppuhelper cppu, plus "test" twice and "unotest" twice -- gbuild tolerates
# repeats in use_libraries, collapsed to one entry each here), use_sdk_api
# (repeated a second time in the .mk, a no-op), use_ure + use_vcl (3
# protector pairs, same precedent as sax_parser/hwpfilter_test_hwpfilter --
# see scaddins/CppunitTest_deferred.cmake), use_rdb(services,services) (see
# file header above), use_configuration -> CONFIGURATION, and finally
# gb_CppunitTest_add_arguments with one bespoke "-env:arg-env=
# $(gb_Helper_LIBRARY_PATH_VAR)..." argument -- the exact same idiom already
# solved for smoketest (smoketest/CMakeLists.txt's own SMOKETEST_EXTRA_ARGS
# comment has the full reasoning: gb_Helper_LIBRARY_PATH_VAR is LD_LIBRARY_PATH
# on this unxgcc.mk platform, and there is no CMake equivalent of make's own
# "$${LD_LIBRARY_PATH+=$$LD_LIBRARY_PATH}" shell-expansion trick inside a
# plain add_test() COMMAND list, so it is reproduced as a CMake-configure-time
# snapshot of the invoking environment instead, omitted entirely if
# LD_LIBRARY_PATH is unset/empty at configure time). Reused verbatim here via
# the same EXTRA_ARGS mechanism.
#
# "test" (gbuild's use_libraries spelling) is "test_fixtures" in this port
# (test/CMakeLists.txt's own comment on why the bare name "test" is
# unavailable as a CMake target name -- same rename every other CppunitTest
# in this port already applies, e.g. scaddins_analysis/svl_urihelper).
set(SVX_UNIT_EXTRA_ARGS "")
if (DEFINED ENV{LD_LIBRARY_PATH} AND NOT "$ENV{LD_LIBRARY_PATH}" STREQUAL "")
    list(APPEND SVX_UNIT_EXTRA_ARGS
        "-env:arg-env=LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}")
endif ()

librestudio_add_cppunit_test(svx_unit
    SOURCES
        svx/qa/unit/svdraw/test_SdrTextObject
        svx/qa/unit/customshapes
        svx/qa/unit/classicshapes
        svx/qa/unit/framelinkarray
        svx/qa/unit/gluepointTest
        svx/qa/unit/sdr
        svx/qa/unit/svdraw
        svx/qa/unit/table
        svx/qa/unit/unodraw
        svx/qa/unit/xoutdev
        svx/qa/unit/xml
        svx/qa/unit/XTableImportExportTest
        svx/qa/unit/ThemeTest
    LIBRARIES basegfx docmodel drawinglayer drawinglayercore editeng sal
        subsequenttest sfx svl svxcore svx tl unotest vcl utl comphelper
        cppuhelper cppu test_fixtures Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/svx/source/inc"
        "${CMAKE_SOURCE_DIR}/svx/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION
    EXTRA_ARGS ${SVX_UNIT_EXTRA_ARGS})

# --- svx_gallery_test ----------------------------------------------------------
#
# CppunitTest_svx_gallery_test.mk, re-checked directly: gb_CppunitTest_
# CppunitScreenShot (not gb_CppunitTest_CppunitTest) -- but that macro (see
# solenv/gbuild/CppunitTest.mk:196-200) only sets a "gb_CppunitTest_localized"
# make variable on the target and otherwise expands to the exact same
# gb_CppunitTest__CppunitTest_impl every plain CppunitTest gets; it adds no
# actual screenshot-capture or non-headless-windowing machinery by itself
# (that only comes from gb_CppunitTest_use_vcl_non_headless_with_windows,
# which this .mk does NOT call -- it calls plain gb_CppunitTest_use_vcl,
# confirmed by reading the whole file). So despite being a "ScreenShot"-typed
# test in gbuild's own taxonomy, this one needs nothing this port's plain
# librestudio_add_cppunit_test() doesn't already provide, unlike svx_
# dialogs_test below.
#
# 1 source (svx/qa/unit/gallery/test_gallery), use_externals(boost_headers),
# use_sdk_api (normal udkapi+offapi), use_libraries (sal utl svxcore
# comphelper tl svl sfx unotest test_fixtures), use_vcl + use_ure (3
# protector pairs, same as svx_unit above), use_configuration ->
# CONFIGURATION, use_rdb(services,services) (see file header above).
librestudio_add_cppunit_test(svx_gallery_test
    SOURCES
        svx/qa/unit/gallery/test_gallery
    LIBRARIES sal utl svxcore comphelper tl svl sfx unotest test_fixtures
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/svx/source/inc"
        "${CMAKE_SOURCE_DIR}/svx/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- svx_core --------------------------------------------------------------------
#
# CppunitTest_svx_core.mk, re-checked directly: 1 source (svx/qa/unit/core --
# includes <test/unoapi_test.hxx>, confirming the subsequenttest dependency
# below: that header is compiled into Library_subsequenttest.mk's
# "subsequenttest" target, not test/CMakeLists.txt's "test_fixtures" -- same
# verification officecfg/CMakeLists.txt's own accelerator_key_checker_test
# comment already did for the identical header), use_externals(boost_headers
# libxml2), use_libraries (comphelper cppu cppuhelper sal subsequenttest svx
# svxcore test_fixtures tl unotest utl vcl xo), use_sdk_api (normal udkapi+
# offapi -- this .mk calls no set_include, confirmed by reading the whole
# file, and svx/qa/unit/core.cxx's own #include list needs nothing under
# svx/inc or svx/source/inc either, only public svx/graphichelper.hxx and
# xmloff/xmlgrhlp.hxx, both already reachable via the global "include/" root
# every target gets from the top-level include_directories() call), use_ure +
# use_vcl (3 protector pairs, same as above), use_rdb(services,services) (see
# file header above), use_custom_headers(officecfg/registry) -> the same
# OFFICECFG_INCLUDE_DIR + officecfg_headers dependency svxcore's own block in
# svx/CMakeLists.txt already uses, use_configuration -> CONFIGURATION.
librestudio_add_cppunit_test(svx_core
    SOURCES
        svx/qa/unit/core
    LIBRARIES comphelper cppu cppuhelper sal subsequenttest svx svxcore
        test_fixtures tl unotest utl vcl xo Boost::boost LibXml2::LibXml2
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- svx_dialogs_test: RESOLVED --------------------------------------------------
#
# CppunitTest_svx_dialogs_test.mk is a gb_CppunitTest_CppunitScreenShot
# calling gb_CppunitTest_use_vcl_non_headless_with_windows (real, actual
# on-screen windowing -- HEADLESS is explicitly unset for it, unlike every
# other test in this port; see solenv/gbuild/CppunitTest.mk:297-302) plus
# gb_CppunitTest_use_uiconfigs(svx). This was previously NOT wired here, or
# anywhere -- UIConfig_svx existing (svx/CMakeLists.txt) did not change that
# verdict, because the consistently-cited blocker across all five modules'
# own "*_dialogs_test" (uui, fpicker, desktop, vcl, svtools, svx) was never
# "does the UIConfig_* package exist" but always the complete absence, in
# cmake/LibreStudioTest.cmake's librestudio_add_cppunit_test(), of any
# HEADLESS-toggle parameter or use_uiconfigs-equivalent mechanism at all
# (re-verified directly against that file at the time: grepped empty, not
# merely undocumented).
#
# Both gaps are now closed: librestudio_add_cppunit_test() gained NONHEADLESS
# (a build-order dependency on vclplug_gen -- the real platform VCL backend
# CppunitTest.mk:264-271 makes a dependency under use_vcl_non_headless_with_
# windows's $(false)-argument call to gb_CppunitTest__use_vcl -- plus an
# explicit "-env:SAL_USE_VCLPLUGIN=gen" argument) and UICONFIG (a build-order
# dependency on a real "<module>_uiconfig" ALL target). Confirmed while
# adding NONHEADLESS: this port has never forced --headless or
# SAL_USE_VCLPLUGIN=svp anywhere -- every test here is "headless" only by
# omission, unlike upstream's gb_TEST_ENV_VARS, which actively sets
# SAL_USE_VCLPLUGIN=svp for every test and relies on use_vcl_non_headless_
# with_windows to undo it -- so NONHEADLESS only has to guarantee the real
# backend and pin it explicitly, not undo a forced setting. See cmake/
# LibreStudioTest.cmake's own header comment for the full mechanism of both
# options. UIConfig_svx (svx/CMakeLists.txt, above) now also defines a real
# build-time mirror -- the svx_uiconfig ALL target -- alongside its
# install(FILES ...) packaging rule, giving UICONFIG something real to
# depend on. gb_CppunitTest_CppunitScreenShot itself still adds nothing
# beyond what use_vcl_non_headless_with_windows provides (same no-op-shaped
# part svx_gallery_test's own comment above already establishes), so nothing
# else was needed.
#
# CppunitTest_svx_dialogs_test.mk, re-checked directly against the live file:
# 1 source (svx/qa/unit/svx-dialogs-test), use_external(boost_headers),
# use_sdk_api (normal udkapi+offapi, called twice -- a no-op the second
# time), set_include -I$(SRCDIR)/svx/source/inc -I$(SRCDIR)/svx/inc plus
# $$(INCLUDE) (a no-op re-inclusion, same idiom svtools_dialogs_test's own
# set_include already uses), use_custom_headers(officecfg/registry) ->
# OFFICECFG_INCLUDE_DIR + DEPENDS officecfg_headers (same precedent as
# svx_core's own use_custom_headers call above), use_libraries basegfx
# comphelper cppu cppuhelper drawinglayer editeng i18nlangtag i18nutil
# msfilter oox sal salhelper sax sfx sot svl svt svx svxcore test tl tk
# ucbhelper unotest utl vcl xo (25 names -- vcl_dialogs_test's/svtools_
# dialogs_test's own 23-name list plus svx and svxcore, "test" ->
# test_fixtures, all 25 re-verified as real add_library() targets), use_ure +
# use_vcl_non_headless_with_windows -> 3 protector pairs + NONHEADLESS,
# use_rdb(services) -> DEPENDS program_services_rdb (no COMPONENTS, same
# ini-fallback reasoning as svx_unit/svx_gallery_test/svx_core above),
# use_configuration -> CONFIGURATION, use_uiconfigs(svx) -> UICONFIG svx.
librestudio_add_cppunit_test(svx_dialogs_test
    SOURCES
        svx/qa/unit/svx-dialogs-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
        i18nutil msfilter oox sal salhelper sax sfx sot svl svt svx svxcore
        test_fixtures tl tk ucbhelper unotest utl vcl xo Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/svx/source/inc"
        "${CMAKE_SOURCE_DIR}/svx/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    NONHEADLESS
    UICONFIG svx
    CONFIGURATION)
