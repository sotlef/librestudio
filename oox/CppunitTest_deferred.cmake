# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# oox_crypto, oox_export, oox_helper, oox_mathml, oox_mcgr, oox_shape,
# oox_testscene3d, oox_vml and oox_wpc_drawing_canvas -- 9 of the 10 oox
# CppunitTest_*.mk files that call use_configuration (which requires
# test/CMakeLists.txt's own LIBRESTUDIO_TEST_UNITTEST_DIR, only set once
# test is add_subdirectory()'d, well after oox's own position -- see oox/
# CMakeLists.txt's own header comment). The 10th, oox_drawingml, is a
# documented, not-yet-attempted gap (needs a UIConfig_simpress-equivalent
# target sd/CMakeLists.txt does not have yet) and is not wired here either
# -- see oox/CMakeLists.txt's own comment for the full reasoning.
#
# Several of these end with the same bespoke gb_CppunitTest_add_arguments
# "-env:arg-env=$(gb_Helper_LIBRARY_PATH_VAR)..." idiom already solved for
# smoketest/svx_unit/unoxml/xmloff's own QA tests -- gb_Helper_LIBRARY_PATH_VAR
# is LD_LIBRARY_PATH on this unxgcc.mk platform, reproduced as a CMake-
# configure-time snapshot of the invoking environment (smoketest/CMakeLists.
# txt's own SMOKETEST_EXTRA_ARGS comment has the full reasoning), omitted
# entirely if LD_LIBRARY_PATH is unset/empty at configure time.
set(OOX_UNIT_EXTRA_ARGS "")
if (DEFINED ENV{LD_LIBRARY_PATH} AND NOT "$ENV{LD_LIBRARY_PATH}" STREQUAL "")
    list(APPEND OOX_UNIT_EXTRA_ARGS
        "-env:arg-env=LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}")
endif ()

# --- oox_crypto -------------------------------------------------------------
#
# CppunitTest_oox_crypto.mk, re-checked directly: 1 source (oox/qa/unit/
# CryptoTest), use_sdk_api, "ifeq ($(TLS),OPENSSL) ... else ifeq ($(TLS),NSS)
# ... endif" -- the OPENSSL branch calls the plain, undefined macro name
# "gb_CppunitTest_externals" (missing the "use_" prefix every other call in
# this port's whole tree uses -- confirmed: grepping "gb_CppunitTest_use_ex"
# vs. this one bare "gb_CppunitTest_externals" spelling finds this is the
# ONLY such call anywhere in the source tree), which is not a real gbuild
# macro (only gb_CppunitTest_use_external/use_externals are) -- GNU Make
# silently expands an unknown $(call ...) name to nothing, so upstream's own
# OPENSSL branch is a real, silent no-op bug (openssl/openssl_headers are
# never actually linked there). Not perpetuated here: this port's
# LIBRESTUDIO_TLS_BACKEND defaults "NSS" (matching xmlsecurity/svl/oox's own
# already-established default), so only that branch matters in practice;
# PkgConfig::NSS is guarded/re-created the same way vcl_type_serializer_test/
# desktop_lib's own "plc4/nss3 -> PkgConfig::NSS" precedent already
# establishes. use_libraries (basegfx comphelper cppu cppuhelper editeng
# drawinglayer msfilter sal oox sax sfx svl svt svx svxcore sot tl unotest
# utl vcl xo xmlscript), use_ure + use_vcl (plain, headless) -> 3 protector
# pairs, use_configuration -> CONFIGURATION (no use_rdb call in this .mk --
# deferred purely for CONFIGURATION's own test-module requirement, not
# program_services_rdb), use_components (comphelper/util/comphelp ->
# "comphelper", configmgr/source/configmgr, filter/source/config/cache/
# filterconfig1 -> "filterconfig", filter/source/storagefilterdetect/
# storagefd, i18npool/util/i18npool, package/source/xstor/xstor, package/
# util/package2, sfx2/util/sfx, sot/util/sot, svl/source/fsstor/fsstorage,
# svtools/util/svt, ucb/source/core/ucb1, ucb/source/ucp/file/ucpfile1,
# ucb/source/ucp/tdoc/ucptdoc1, unotools/util/utl, uui/util/uui, vcl/
# vcl.common -> "vcl", sax/source/expatwrap/expwrap -> "sax" -- each lookup
# key re-verified against its owning module's own librestudio_add_component()
# call, same "comphelper"/"filterconfig"/"vcl"/"sax" precedents already
# established by earlier waves).
if (NOT TARGET PkgConfig::NSS)
    pkg_check_modules(NSS REQUIRED IMPORTED_TARGET nss)
endif ()

librestudio_add_cppunit_test(oox_crypto
    SOURCES
        oox/qa/unit/CryptoTest
    LIBRARIES
        basegfx comphelper cppu cppuhelper editeng drawinglayer msfilter sal
        oox sax sfx svl svt svx svxcore sot tl unotest utl vcl xo xmlscript
        PkgConfig::NSS
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        comphelper configmgr filterconfig storagefd i18npool xstor package2
        sfx sot fsstorage svt ucb1 ucpfile1 ucptdoc1 utl uui vcl sax
    CONFIGURATION)

# --- oox_export ---------------------------------------------------------------
#
# CppunitTest_oox_export.mk, re-checked directly: 1 source (oox/qa/unit/
# export), use_externals(boost_headers libxml2), use_libraries (comphelper
# cppu cppuhelper oox sal subsequenttest test unotest utl tl -- "test" ->
# test_fixtures), use_sdk_api, use_ure + use_vcl (plain, headless) -> same 3
# protector pairs, use_rdb(services) -> DEPENDS program_services_rdb (no
# COMPONENTS), use_custom_headers(officecfg/registry) -> OFFICECFG_INCLUDE_DIR
# + DEPENDS officecfg_headers, use_configuration -> CONFIGURATION,
# add_arguments -> EXTRA_ARGS (see this file's own header comment).
librestudio_add_cppunit_test(oox_export
    SOURCES
        oox/qa/unit/export
    LIBRARIES
        comphelper cppu cppuhelper oox sal subsequenttest test_fixtures
        unotest utl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION
    EXTRA_ARGS ${OOX_UNIT_EXTRA_ARGS})

# --- oox_helper ---------------------------------------------------------------
#
# CppunitTest_oox_helper.mk, re-checked directly: 1 source (oox/qa/unit/
# helper), same shape as oox_export above minus the trailing add_arguments
# call (re-verified: no such call in this .mk).
librestudio_add_cppunit_test(oox_helper
    SOURCES
        oox/qa/unit/helper
    LIBRARIES
        comphelper cppu cppuhelper oox sal subsequenttest test_fixtures
        unotest utl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- oox_mathml ---------------------------------------------------------------
#
# CppunitTest_oox_mathml.mk, re-checked directly: 1 source (oox/qa/unit/
# mathml), identical shape to oox_helper above (same libraries, no
# add_arguments call).
librestudio_add_cppunit_test(oox_mathml
    SOURCES
        oox/qa/unit/mathml
    LIBRARIES
        comphelper cppu cppuhelper oox sal subsequenttest test_fixtures
        unotest utl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- oox_mcgr -----------------------------------------------------------------
#
# CppunitTest_oox_mcgr.mk, re-checked directly: 1 source (oox/qa/unit/mcgr),
# same shape as oox_export above (same libraries), plus the trailing
# add_arguments -> EXTRA_ARGS call.
librestudio_add_cppunit_test(oox_mcgr
    SOURCES
        oox/qa/unit/mcgr
    LIBRARIES
        comphelper cppu cppuhelper oox sal subsequenttest test_fixtures
        unotest utl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION
    EXTRA_ARGS ${OOX_UNIT_EXTRA_ARGS})

# --- oox_shape -----------------------------------------------------------------
#
# CppunitTest_oox_shape.mk, re-checked directly: 1 source (oox/qa/unit/
# shape), use_libraries (comphelper cppu cppuhelper docmodel oox sal sfx
# subsequenttest svx svxcore tl test unotest utl basegfx -- "test" ->
# test_fixtures, this one additionally needs docmodel/sfx/svx/svxcore/
# basegfx vs. oox_helper/mathml's plain set), no add_arguments call.
librestudio_add_cppunit_test(oox_shape
    SOURCES
        oox/qa/unit/shape
    LIBRARIES
        comphelper cppu cppuhelper docmodel oox sal sfx subsequenttest svx
        svxcore tl test_fixtures unotest utl basegfx
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- oox_testscene3d -----------------------------------------------------------
#
# CppunitTest_oox_testscene3d.mk, re-checked directly: 1 source (oox/qa/
# unit/testscene3d), use_libraries (comphelper cppu cppuhelper oox sal sfx
# subsequenttest svx svxcore test tl unotest utl vcl -- "test" ->
# test_fixtures, needs sfx/svx/svxcore/vcl vs. oox_helper/mathml's plain
# set, same additions as oox_shape plus vcl), plus the trailing
# add_arguments -> EXTRA_ARGS call.
librestudio_add_cppunit_test(oox_testscene3d
    SOURCES
        oox/qa/unit/testscene3d
    LIBRARIES
        comphelper cppu cppuhelper oox sal sfx subsequenttest svx svxcore
        test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION
    EXTRA_ARGS ${OOX_UNIT_EXTRA_ARGS})

# --- oox_vml -------------------------------------------------------------------
#
# CppunitTest_oox_vml.mk, re-checked directly: 1 source (oox/qa/unit/vml),
# same plain library set as oox_export/helper/mathml above, plus the
# trailing add_arguments -> EXTRA_ARGS call.
librestudio_add_cppunit_test(oox_vml
    SOURCES
        oox/qa/unit/vml
    LIBRARIES
        comphelper cppu cppuhelper oox sal subsequenttest test_fixtures
        unotest utl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION
    EXTRA_ARGS ${OOX_UNIT_EXTRA_ARGS})

# --- oox_wpc_drawing_canvas -----------------------------------------------------
#
# CppunitTest_oox_wpc_drawing_canvas.mk, re-checked directly: 1 source (oox/
# qa/unit/wpc_drawing_canvas), use_libraries (comphelper cppu cppuhelper
# docmodel oox sal subsequenttest test unotest utl tl -- "test" ->
# test_fixtures, needs docmodel vs. oox_export/helper/mathml/vml's plain
# set, same addition as oox_shape/oox_drawingml), plus the trailing
# add_arguments -> EXTRA_ARGS call.
librestudio_add_cppunit_test(oox_wpc_drawing_canvas
    SOURCES
        oox/qa/unit/wpc_drawing_canvas
    LIBRARIES
        comphelper cppu cppuhelper docmodel oox sal subsequenttest
        test_fixtures unotest utl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION
    EXTRA_ARGS ${OOX_UNIT_EXTRA_ARGS})

# --- oox_drawingml -------------------------------------------------------------
#
# CppunitTest_oox_drawingml.mk, re-checked directly: 1 source (oox/qa/unit/
# drawingml), use_externals(boost_headers libxml2), use_libraries (comphelper
# cppu cppuhelper docmodel oox sal subsequenttest test unotest utl basegfx --
# "test" -> test_fixtures), use_sdk_api, use_ure + use_vcl (plain, headless)
# -> same 3 protector pairs, use_rdb(services) -> DEPENDS program_services_rdb
# (no COMPONENTS), use_custom_headers(officecfg/registry) -> OFFICECFG_
# INCLUDE_DIR + DEPENDS officecfg_headers, use_configuration -> CONFIGURATION,
# use_uiconfigs("modules/simpress", sfx, svt) -> UICONFIG simpress sfx svt
# (sd/CMakeLists.txt's own new "simpress_uiconfig" ALL target, ported
# alongside this wiring; "sfx"/"svt" already existed), add_arguments ->
# EXTRA_ARGS (this file's own header comment), and a real order-only
# dependency on oox_customshapes (oox/CMakeLists.txt's own new "--- Package_
# customshapes ---" section, ported alongside oox_drawingml's own original
# wiring pass).
librestudio_add_cppunit_test(oox_drawingml
    SOURCES
        oox/qa/unit/drawingml
    LIBRARIES
        comphelper cppu cppuhelper docmodel oox sal subsequenttest
        test_fixtures unotest utl basegfx tl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        officecfg_headers
        program_services_rdb
        oox_customshapes
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG simpress sfx svt
    CONFIGURATION
    EXTRA_ARGS ${OOX_UNIT_EXTRA_ARGS})

# oox/CppunitTest_deferred.cmake needs test/CMakeLists.txt's own
# LIBRESTUDIO_TEST_UNITTEST_DIR (via CONFIGURATION) and, for most of these,
# program_services_rdb -- both only available once modules add_subdirectory()'d
# later than oox's own position have run. See oox/CMakeLists.txt's own
# cmake_language(DEFER) call site comment for the scheduling mechanism, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required there.
