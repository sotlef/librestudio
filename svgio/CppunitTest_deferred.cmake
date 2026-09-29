# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# svgio (CppunitTest_svgio.mk) and svgio_read (CppunitTest_svgio_read.mk) --
# split out of svgio/CMakeLists.txt into their own file, include()'d from the
# root CMakeLists.txt only once every add_subdirectory() in the whole tree
# has completed; see the cmake_language(DEFER ...) call site comment at the
# end of svgio/CMakeLists.txt for exactly which later-binding module each
# needs (ucb's ucb1/ucpfile1 components for svgio, program_services_rdb for
# svgio_read, LIBRESTUDIO_TEST_UNITTEST_DIR for both).
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not svgio's, anything svgio/CMakeLists.txt already built as a real
# CMake target (svgio, svgio_tools) is still visible here -- nothing
# directory-scoped that svgio/CMakeLists.txt set up (it creates no
# IMPORTED_TARGET of its own) needs to be re-derived.

# --- svgio ---------------------------------------------------------------
#
# CppunitTest_svgio.mk, re-checked directly against the live file: 1 source
# (svgio/qa/cppunit/SvgImportTest), set_include ($$(INCLUDE) -I$(SRCDIR)/
# svgio/inc), use_externals(boost_headers libxml2), use_sdk_api (normal
# udkapi+offapi), use_library_objects(svgio,svgio) -> plain LIBRARIES
# svgio_lib (the production library, renamed to that from the plain "svgio"
# CppunitTest_svgio.mk's own name needs for the CMake target below instead --
# see svgio/CMakeLists.txt's own comment on that rename and on svgio_tools's
# identical use_library_objects call -- no version-script/hidden-visibility
# restriction on svgio's real exported symbols in this port), use_libraries
# (basegfx drawinglayer drawinglayercore cppu cppuhelper
# comphelper i18nlangtag sal salhelper sax svt test unotest tk tl utl vcl --
# "test" -> test_fixtures, the same rename every other CppunitTest in this
# port already applies), use_ure + use_vcl (plain, headless) -> 3 protector
# pairs (unobootstrapprotector, unoexceptionprotector, vclbootstrapprotector
# -- gb_CppunitTest_use_vcl's own vclbootstrapprotector dependency is
# unconditional regardless of headless/non-headless, solenv/gbuild/
# CppunitTest.mk's gb_CppunitTest__use_vcl and gb_CppunitTest__make_args'
# "$(if $(VCL),--protector ... vclbootstrapprotector)" line, re-checked
# directly), use_components (configmgr/source/configmgr, framework/util/fwk,
# i18npool/util/i18npool, package/source/xstor/xstor, package/util/package2,
# sax/source/expatwrap/expwrap, sfx2/util/sfx, toolkit/util/tk, ucb/source/
# core/ucb1, ucb/source/ucp/file/ucpfile1, unotools/util/utl, vcl/vcl.common)
# -> COMPONENTS configmgr fwk i18npool xstor package2 sax sfx tk ucb1
# ucpfile1 utl vcl (each name verified against the owning module's own
# librestudio_add_component() call, NOT gbuild's use_components spelling --
# configmgr/CMakeLists.txt, framework/CMakeLists.txt "fwk", i18npool/
# CMakeLists.txt, package/CMakeLists.txt "xstor"/"package2", sax/
# CMakeLists.txt "sax" for its own expwrap.component -- same single-
# component-under-the-module-name shape sax_xmlimport's own precedent
# already established, see sax/CMakeLists.txt's comment -- sfx2/CMakeLists.
# txt "sfx", toolkit/CMakeLists.txt "tk", ucb/CMakeLists.txt "ucb1"/
# "ucpfile1", unotools/CMakeLists.txt "utl", vcl/CMakeLists.txt "vcl" for its
# own vcl.common.component), use_configuration -> CONFIGURATION, a comment
# ("assert if font/glyph fallback occurs") plus use_non_application_font_use
# and use_more_fonts that this port does not model anywhere (grepped: no
# other CppunitTest call site in this port models either gbuild call, so this
# is a pre-existing, not newly-introduced, gap -- not silently worked around
# here, simply not reproduced, same honesty this port already applies to
# e.g. the .user profile template gap documented in cmake/LibreStudioTest.
# cmake's own header comment).
librestudio_add_cppunit_test(svgio
    SOURCES
        svgio/qa/cppunit/SvgImportTest
    LIBRARIES
        svgio_lib basegfx drawinglayer drawinglayercore cppu cppuhelper comphelper
        i18nlangtag sal salhelper sax svt test_fixtures unotest tk tl utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/svgio/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr fwk i18npool xstor package2 sax sfx tk ucb1 ucpfile1 utl vcl
    CONFIGURATION)
# 2026-09-29, found via the real remote build: this call's own header comment
# already documented use_externals(boost_headers libxml2), but LibXml2::
# LibXml2 itself was missing from LIBRARIES above -- SvgImportTest.cxx's own
# include/test/xmltesttools.hxx needs <libxml/xpath.h> directly. Real "fatal
# error: libxml/xpath.h: No such file or directory".

# --- svgio_read ------------------------------------------------------------
#
# CppunitTest_svgio_read.mk, re-checked directly against the live file: 1
# source (svgio/qa/cppunit/SvgRead), set_include ($$(INCLUDE) -I$(SRCDIR)/
# svgio/inc), use_externals(boost_headers libxml2), use_libraries (basegfx
# comphelper cppu cppuhelper drawinglayer drawinglayercore editeng
# i18nlangtag sal salhelper sax sot svl svt svx svxcore test tl tk ucbhelper
# unotest utl vcl xo -- "test" -> test_fixtures, same rename), use_sdk_api,
# use_ure + use_vcl (plain, headless) -> same 3 protector pairs as svgio
# above, use_rdb(svgio_read,services) -> DEPENDS program_services_rdb (no
# COMPONENTS -- cppunittester's UNO bootstrap falls back to unorc's own ini
# default for UNO_SERVICES, the same reasoning every other bare use_rdb(
# services) test in this port already documents, e.g. svx/CppunitTest_
# deferred.cmake's svx_dialogs_test), use_custom_headers(officecfg/registry)
# -> INCLUDE OFFICECFG_INCLUDE_DIR + DEPENDS officecfg_headers (the same
# precedent every other use_custom_headers(officecfg/registry) call site in
# this port already applies, e.g. configmgr/CppunitTest_deferred.cmake --
# officecfg is add_subdirectory()'d well before svgio's own position, so
# officecfg_headers is not itself a source of deferral here, only along for
# the ride with CONFIGURATION/program_services_rdb below), use_configuration
# -> CONFIGURATION.
#
# Notably absent from use_libraries: "svgio" itself. Unlike svgio/svgio_tools
# above, this test never calls use_library_objects or use_libraries(svgio)
# -- confirmed by reading the live .mk directly -- so it is not linked here
# either; SvgRead.cxx drives svgio's own UNO service (com.sun.star.graphic.
# SvgTools) purely through the ComponentContext COMPONENTS/program_services_
# rdb would normally provide, not a direct C++ link, and since svgio itself
# has no explicit COMPONENTS entry in this .mk (only use_rdb(services)), the
# svgio.component the production svgio/CMakeLists.txt registers is reachable
# only via the ini-fallback path used_rdb/services already relies on, same as
# every other component this .mk does not explicitly list.
librestudio_add_cppunit_test(svgio_read
    SOURCES
        svgio/qa/cppunit/SvgRead
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng i18nlangtag sal salhelper sax sot svl svt svx svxcore
        test_fixtures tl tk ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/svgio/inc"
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
