# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# starmath_qa_cppunit, starmath_import, starmath_export and
# starmath_dialogs_test (CppunitTest_starmath_qa_cppunit.mk, CppunitTest_
# starmath_import.mk, CppunitTest_starmath_export.mk, CppunitTest_starmath_
# dialogs_test.mk) -- split out of starmath/CMakeLists.txt into their own
# file, include()'d from the root CMakeLists.txt only once every add_
# subdirectory() in the whole tree has completed; see the cmake_language(
# DEFER ...) call site comment at the end of starmath/CMakeLists.txt for
# exactly which later-binding module each needs (LIBRESTUDIO_TEST_UNITTEST_DIR
# for all four via CONFIGURATION, program_services_rdb for the latter three
# via use_rdb(services)).
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not starmath's, anything starmath/CMakeLists.txt already built as a
# real CMake target (sm, smd, sm_sdi, sm_oox_generated, smath_uiconfig) is
# still visible here -- nothing directory-scoped that starmath/CMakeLists.txt
# set up (it creates no IMPORTED_TARGET of its own) needs to be re-derived.

# --- starmath_qa_cppunit ------------------------------------------------------
#
# CppunitTest_starmath_qa_cppunit.mk, re-checked directly against the live
# file: 5 sources (starmath/qa/cppunit/test_cursor, test_node, test_
# nodetotextvisitors, test_parse, test_starmath), set_include called twice
# ($$(INCLUDE) -I$(SRCDIR)/starmath/inc, then -I$(SRCDIR)/starmath/source
# $$(INCLUDE) again -- both folded into one INCLUDE list below, same "no-op
# re-inclusion of the existing INCLUDE value" idiom basctl_dialogs_test's own
# comment already documents), use_externals(boost_headers icuuc),
# use_sdk_api, use_library_objects(starmath_qa_cppunit,sm) -> plain LIBRARIES
# sm (same reasoning as svx_removewhichrange's/svgio_tools's own use_library_
# objects call -- no *.map/-fvisibility=hidden restriction anywhere under
# starmath/, re-checked directly), use_libraries (comphelper cppu cppuhelper
# editeng fwk i18nlangtag i18nutil msfilter oox sal salhelper sax sfx sot svl
# svt svxcore svx test tk tl unotest utl vcl xo -- "test" -> test_fixtures,
# the same rename every other CppunitTest in this port already applies),
# use_ure + use_vcl (plain, headless) -> 3 protector pairs (unobootstrap
# protector, unoexceptionprotector, vclbootstrapprotector -- gb_CppunitTest_
# use_vcl's own vclbootstrapprotector dependency is unconditional regardless
# of headless/non-headless, solenv/gbuild/CppunitTest.mk's gb_CppunitTest__
# use_vcl and gb_CppunitTest__make_args' "$(if $(VCL),--protector ...
# vclbootstrapprotector)" line, re-checked directly), use_components
# (configmgr/source/configmgr, framework/util/fwk, i18npool/util/i18npool,
# package/source/xstor/xstor, package/util/package2, toolkit/util/tk, sfx2/
# util/sfx, ucb/source/core/ucb1, ucb/source/ucp/file/ucpfile1, unotools/
# util/utl, vcl/vcl.common) -> COMPONENTS configmgr fwk i18npool xstor
# package2 tk sfx ucb1 ucpfile1 utl vcl (each name verified against the
# owning module's own librestudio_add_component() call, same set svgio/
# CppunitTest_deferred.cmake's own svgio test already verified -- all of
# these owning modules sit textually BEFORE starmath in the root CMakeLists.
# txt, so COMPONENTS alone would not have forced deferral; use_configuration
# below is the actual reason), use_configuration -> CONFIGURATION,
# use_more_fonts (not modelled anywhere in this port, same pre-existing gap
# svgio's own use_more_fonts call already left unaddressed -- not a new
# omission introduced here).
librestudio_add_cppunit_test(starmath_qa_cppunit
    SOURCES
        starmath/qa/cppunit/test_cursor
        starmath/qa/cppunit/test_node
        starmath/qa/cppunit/test_nodetotextvisitors
        starmath/qa/cppunit/test_parse
        starmath/qa/cppunit/test_starmath
    LIBRARIES
        sm comphelper cppu cppuhelper editeng fwk i18nlangtag i18nutil
        msfilter oox sal salhelper sax sfx sot svl svt svxcore svx
        test_fixtures tk tl unotest utl vcl xo
        Boost::boost ICU::uc
    INCLUDE
        "${CMAKE_SOURCE_DIR}/starmath/inc"
        "${CMAKE_SOURCE_DIR}/starmath/source"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr fwk i18npool xstor package2 tk sfx ucb1 ucpfile1 utl vcl
    CONFIGURATION)

# --- starmath_import ----------------------------------------------------------
#
# CppunitTest_starmath_import.mk, re-checked directly against the live file:
# 2 sources (starmath/qa/cppunit/test_import, starmath/qa/extras/mmlimport-
# test), set_include ($$(INCLUDE) -I$(SRCDIR)/starmath/inc), use_externals(
# boost_headers libxml2), use_sdk_api, use_libraries (comphelper cppu
# cppuhelper editeng i18nlangtag i18nutil msfilter oox sal salhelper sax sfx
# sm smd sot subsequenttest svl svt svx svxcore test tk tl unotest unoxml utl
# vcl xo -- "test" -> test_fixtures), use_ure + use_vcl (plain, headless) ->
# same 3 protector pairs as starmath_qa_cppunit above, use_rdb(starmath_
# import,services) -> DEPENDS program_services_rdb (no COMPONENTS --
# cppunittester's UNO bootstrap falls back to unorc's own ini default for
# UNO_SERVICES, the same reasoning every other bare use_rdb(services) test in
# this port already documents, e.g. svx/CppunitTest_deferred.cmake's svx_
# dialogs_test), use_configuration -> CONFIGURATION.
#
# Unlike starmath_qa_cppunit, this one links "sm"/"smd" through plain
# use_libraries (no use_library_objects call in this .mk), so both go into
# LIBRARIES the ordinary way, alongside subsequenttest and unoxml (both real
# add_library() targets, test/CMakeLists.txt's own subsequenttest and
# unoxml/CMakeLists.txt's own unoxml -- unoxml is add_subdirectory()'d well
# before starmath's own position, so it is not itself a source of deferral
# here).
librestudio_add_cppunit_test(starmath_import
    SOURCES
        starmath/qa/cppunit/test_import
        starmath/qa/extras/mmlimport-test
    LIBRARIES
        comphelper cppu cppuhelper editeng i18nlangtag i18nutil msfilter oox
        sal salhelper sax sfx sm smd sot subsequenttest svl svt svx svxcore
        test_fixtures tk tl unotest unoxml utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/starmath/inc"
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

# --- starmath_export ----------------------------------------------------------
#
# CppunitTest_starmath_export.mk, re-checked directly against the live file:
# 1 source (starmath/qa/extras/mmlexport-test), set_include (-I$(SRCDIR)/
# starmath/inc $$(INCLUDE)), use_externals(boost_headers libxml2 icuuc)
# (icuuc added vs. starmath_import above, libxml2/boost_headers shared),
# use_sdk_api, use_libraries -- IDENTICAL list to starmath_import's own
# (comphelper cppu cppuhelper editeng i18nlangtag i18nutil msfilter oox sal
# salhelper sax sfx sm smd sot subsequenttest svl svt svx svxcore test tk tl
# unotest unoxml utl vcl xo, re-verified directly rather than assumed from
# starmath_import's own list), use_ure + use_vcl (plain, headless) -> same 3
# protector pairs, use_rdb(starmath_export,services) -> DEPENDS
# program_services_rdb (same no-COMPONENTS reasoning as starmath_import
# above), use_configuration -> CONFIGURATION.
librestudio_add_cppunit_test(starmath_export
    SOURCES
        starmath/qa/extras/mmlexport-test
    LIBRARIES
        comphelper cppu cppuhelper editeng i18nlangtag i18nutil msfilter oox
        sal salhelper sax sfx sm smd sot subsequenttest svl svt svx svxcore
        test_fixtures tk tl unotest unoxml utl vcl xo
        Boost::boost LibXml2::LibXml2 ICU::uc
    INCLUDE
        "${CMAKE_SOURCE_DIR}/starmath/inc"
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

# --- starmath_dialogs_test (screenshot test) ----------------------------------
#
# CppunitTest_starmath_dialogs_test.mk (a gb_CppunitTest_CppunitScreenShot
# screenshot target), re-checked directly against the live file: 1 source
# (starmath/qa/unit/starmath-dialogs-test), NO set_include call anywhere in
# this .mk (unlike basctl_dialogs_test's own explicit one) -- so the udkapi/
# offapi INCLUDE dirs below come only from use_sdk_api (called twice, a
# no-op the second time, same idiom svx_unit's own repeated call already
# established), not from any set_include reproduction. use_libraries
# (basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
# i18nutil msfilter oox sal salhelper sax sfx sot svl svt test tl tk
# ucbhelper unotest utl vcl xo -- "test" -> test_fixtures), use_external(
# boost_headers), use_ure + use_vcl_non_headless_with_windows -> 3 protector
# pairs + NONHEADLESS (same mechanism as vcl_dialogs_test/svtools_dialogs_
# test/svx_dialogs_test/basctl_dialogs_test/reportdesign_dialogs_test/cui_
# dialogs_test's own precedent), use_rdb(starmath_dialogs_test,services) ->
# DEPENDS program_services_rdb (same no-COMPONENTS reasoning as starmath_
# import/_export above), use_configuration -> CONFIGURATION, use_uiconfigs(
# modules/smath) -> UICONFIG smath (starmath/CMakeLists.txt's own new
# "smath_uiconfig" ALL target, ported alongside this wiring in the same
# change -- starmath/CMakeLists.txt had no UIConfig_smath block at all
# beforehand, confirmed by grepping the file directly before this wave; see
# that file's own UIConfig_smath comment for why "smath", not "modules/
# smath", is the target name passed here).
#
# Notably absent from use_libraries: "sm"/"smd" themselves. Like basctl_
# dialogs_test's own absence of "basctl", this screenshot test never links
# starmath's production libraries -- confirmed by reading the live .mk
# directly, no use_library_objects/use_libraries(sm) or (smd) call anywhere
# in it -- it drives starmath's .ui files straight through VCL's dialog-
# builder machinery instead, so it needs no guard tied to any BUILD_TYPE
# filter the way starmath/CMakeLists.txt's own sm/smd targets might.
librestudio_add_cppunit_test(starmath_dialogs_test
    SOURCES
        starmath/qa/unit/starmath-dialogs-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
        i18nutil msfilter oox sal salhelper sax sfx sot svl svt test_fixtures
        tl tk ucbhelper unotest utl vcl xo
        Boost::boost
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
    UICONFIG smath
    CONFIGURATION)
