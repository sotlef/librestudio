# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# embeddedobj_general (CppunitTest_embeddedobj_general.mk) and
# embeddedobj_msole (CppunitTest_embeddedobj_msole.mk, if (WIN32)-gated
# below) -- both split out of embeddedobj/CMakeLists.txt into this file,
# scheduled via cmake_language(DEFER) (see embeddedobj/CMakeLists.txt's own
# tail) because their gb_CppunitTest_use_rdb(services) calls need
# program_services_rdb, which does not exist at embeddedobj/CMakeLists.txt's
# own add_subdirectory() position. Same call-site pattern as officecfg/
# CppunitTest_deferred.cmake's officecfg_accelerator_key_checker_test (read
# that file's header comment for the full program_services_rdb reasoning,
# identical here).
#
# CppunitTest_embeddedobj_general.mk, re-checked directly against the live
# file: use_externals(boost_headers,libxml2) -> Boost::boost + LibXml2::LibXml2
# + LIBXML2_INCLUDE_DIRS (find_package(LibXml2 REQUIRED), root CMakeLists.txt),
# 1 source (embeddedobj/qa/cppunit/general), use_libraries comphelper cppu
# cppuhelper embobj sal subsequenttest test tl unotest utl ("test" -> test_
# fixtures, this port's established rename; "embobj" is embeddedobj/
# CMakeLists.txt's own production library, add_subdirectory()'d well before
# this file's include() point; all others re-verified as real add_library()
# targets), use_sdk_api (normal udkapi+offapi), use_ure + use_vcl (headless/
# SVP variant, NOT use_vcl_non_headless[_with_windows] -- re-checked, this
# .mk really only calls plain use_vcl) -> 3 protector pairs, no NONHEADLESS
# (plain use_vcl never makes vclplug_gen a build dependency -- see cppcanvas/
# CppunitTest_deferred.cmake's own cppcanvas_test comment for the same
# use_vcl-vs-use_vcl_non_headless distinction, re-derived there directly
# against solenv/gbuild/CppunitTest.mk), use_rdb(services) -> DEPENDS
# program_services_rdb (no COMPONENTS -- cppunittester's UNO bootstrap falls
# back to unorc's own ini default for UNO_SERVICES, same as every other
# use_rdb(services)-only test in this port), use_custom_headers(officecfg/
# registry) -> OFFICECFG_INCLUDE_DIR + DEPENDS officecfg_headers (officecfg is
# add_subdirectory()'d at root CMakeLists.txt line 719, well before this
# file's include() point), use_configuration -> CONFIGURATION.
librestudio_add_cppunit_test(embeddedobj_general
    SOURCES
        embeddedobj/qa/cppunit/general
    LIBRARIES
        comphelper cppu cppuhelper embobj sal subsequenttest test_fixtures tl
        unotest utl Boost::boost LibXml2::LibXml2
    INCLUDE
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${LIBXML2_INCLUDE_DIRS}"
    DEPENDS
        officecfg_headers udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- embeddedobj_msole (CppunitTest_embeddedobj_msole.mk) --------------------
#
# Module_embeddedobj.mk: `ifeq ($(OS),WNT)` around this test's own
# gb_Module_add_slowcheck_targets call -- genuinely Windows-only upstream
# (embeddedobj_general above is its Linux sibling, `ifeq ($(OS),LINUX)`).
# Re-checked directly against the live file: same shape as embeddedobj_
# general (use_externals(boost_headers,libxml2), use_libraries comphelper
# cppu embobj sal subsequenttest test unotest utl vcl tl, use_sdk_api,
# use_ure + use_vcl -> 3 protector pairs, use_rdb(services) -> DEPENDS
# program_services_rdb, use_custom_headers(officecfg/registry) ->
# OFFICECFG_INCLUDE_DIR + DEPENDS officecfg_headers, use_configuration ->
# CONFIGURATION), plus one addition: use_system_win32_libs(ole32).
# Transcribed for real inside if (WIN32), following cpputools/CMakeLists.txt's
# own sp2bv precedent (kept structurally present, not silently dropped);
# inert on this Linux build.
if (WIN32)
    librestudio_add_cppunit_test(embeddedobj_msole
        SOURCES
            embeddedobj/qa/cppunit/msole
        LIBRARIES
            comphelper cppu cppuhelper embobj sal subsequenttest test_fixtures
            tl unotest utl vcl Boost::boost LibXml2::LibXml2 ole32
        INCLUDE
            "${OFFICECFG_INCLUDE_DIR}"
            "${udkapi_normal_HEADERS_INCLUDE_DIR}"
            "${offapi_normal_HEADERS_INCLUDE_DIR}"
            "${LIBXML2_INCLUDE_DIRS}"
        DEPENDS
            officecfg_headers udkapi_normal_headers offapi_normal_headers
            program_services_rdb
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
            vclbootstrapprotector vclbootstrapprotector
        CONFIGURATION)
endif ()
