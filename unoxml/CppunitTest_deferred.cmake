# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# unoxml_domtest (CppunitTest_unoxml_domtest.mk) and unoxml_rdftest
# (CppunitTest_unoxml_rdftest.mk) -- split out of unoxml/CMakeLists.txt into
# their own file, include()'d from the root CMakeLists.txt only after every
# module either test references has already run.
#
# unoxml itself is add_subdirectory()'d at line ~855. unoxml_domtest's own
# use_components names configmgr/source/configmgr, framework/util/fwk,
# i18npool/util/i18npool, sfx2/util/sfx, ucb/source/core/ucb1,
# ucb/source/ucp/file/ucpfile1, unoxml/source/service/unoxml -- resolved
# (grep -n "librestudio_add_component(") to "configmgr" (configmgr/
# CMakeLists.txt), "fwk" (framework/CMakeLists.txt), "i18npool" (i18npool/
# CMakeLists.txt), "sfx" (sfx2/CMakeLists.txt), "ucb1"/"ucpfile1" (both
# ucb/CMakeLists.txt) and "unoxml" (this module's own, already registered by
# the time this file runs). add_subdirectory() positions, re-verified: sax
# and i18npool (~730) and configmgr (~834) all sit BEFORE unoxml's own
# (~855), but framework (~865), sfx2 (~875) and ucb (~943) all sit AFTER it
# -- ucb, the latest of the three, is still earlier than test's own position
# (~1018), which both tests also need (use_configuration's
# LIBRESTUDIO_TEST_UNITTEST_DIR requirement, plus use_ure/use_vcl's
# protector plug-ins from unotest/test). unoxml_rdftest additionally calls
# use_rdb(services) (not use_components), which needs program_services_rdb --
# not defined until the very end of the root CMakeLists.txt, well after
# test's own position -- so that is the later-binding constraint of the two,
# and (same combined-file precedent as editeng's own editeng_editeng/
# editeng_core, or toolkit's toolkit_toolkit/toolkit_a11y) both tests are
# kept in this one file, include()'d at the point program_services_rdb
# exists.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not unoxml's, anything unoxml/CMakeLists.txt already built as a real
# CMake target (the "unoxml" production library, its component, etc.) is
# still visible here -- nothing directory-scoped needs to be re-derived.

# --- unoxml_domtest -----------------------------------------------------

# CppunitTest_unoxml_domtest.mk, re-checked directly against the live file:
# 1 source (unoxml/qa/unit/domtest), use_sdk_api (normal udkapi+offapi
# headers), use_components (see above), use_externals(boost_headers),
# use_libraries comphelper cppu cppuhelper sal sax test unotest ("test" ->
# test_fixtures, this port's established rename), use_configuration ->
# CONFIGURATION, use_ure + use_vcl -> 3 protector pairs (same precedent as
# sax_parser/hwpfilter_test_hwpfilter).
librestudio_add_cppunit_test(unoxml_domtest
    SOURCES
        unoxml/qa/unit/domtest
    LIBRARIES
        comphelper cppu cppuhelper sal sax test_fixtures unotest
        Boost::boost
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
        configmgr fwk i18npool sfx ucb1 ucpfile1 unoxml
    CONFIGURATION)

# --- unoxml_rdftest -------------------------------------------------------

# CppunitTest_unoxml_rdftest.mk, re-checked directly against the live file:
# 1 source (unoxml/qa/unit/rdftest), use_sdk_api, use_rdb(services) -> see
# file header above (no COMPONENTS, DEPENDS program_services_rdb,
# cppunittester's UNO bootstrap falls back to unorc's own ini default for
# UNO_SERVICES), use_externals(boost_headers, libxml2) -> Boost::boost +
# LibXml2::LibXml2 + its include dir (same as xmlscript_cppunit/officecfg_
# accelerator_key_checker_test's own precedent), use_libraries comphelper
# cppu cppuhelper sal sax subsequenttest test tl unotest utl ("test" ->
# test_fixtures), use_configuration -> CONFIGURATION, use_ure + use_vcl -> 3
# protector pairs, and a final gb_CppunitTest_add_arguments with one bespoke
# "-env:arg-env=$(gb_Helper_LIBRARY_PATH_VAR)..." argument -- the same idiom
# already solved for smoketest/svx_unit (see svx/CppunitTest_deferred.cmake's
# own comment): a CMake-configure-time snapshot of LD_LIBRARY_PATH, omitted
# entirely if unset/empty at configure time.
set(UNOXML_RDFTEST_EXTRA_ARGS "")
if (DEFINED ENV{LD_LIBRARY_PATH} AND NOT "$ENV{LD_LIBRARY_PATH}" STREQUAL "")
    list(APPEND UNOXML_RDFTEST_EXTRA_ARGS
        "-env:arg-env=LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}")
endif ()

librestudio_add_cppunit_test(unoxml_rdftest
    SOURCES
        unoxml/qa/unit/rdftest
    LIBRARIES
        comphelper cppu cppuhelper sal sax subsequenttest test_fixtures tl
        unotest utl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${LIBXML2_INCLUDE_DIRS}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION
    EXTRA_ARGS ${UNOXML_RDFTEST_EXTRA_ARGS})
