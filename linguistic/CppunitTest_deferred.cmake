# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# linguistic_restprotocol (CppunitTest_linguistic_restprotocol.mk) -- split
# out of linguistic/CMakeLists.txt into its own file, include()'d from the
# root CMakeLists.txt only after program_services_rdb exists, the same reason
# and call-site pattern as scaddins/CppunitTest_deferred.cmake and officecfg/
# CppunitTest_deferred.cmake (read scaddins/CppunitTest_deferred.cmake's
# header comment first for the full use_rdb(services)/ini-fallback reasoning,
# which applies identically here -- this test calls no use_components either,
# so it gets the same treatment: no COMPONENTS, DEPENDS program_services_rdb,
# and cppunittester's UNO bootstrap falls back to unorc's own
# "UNO_SERVICES=${ORIGIN}/services.rdb" ini default).
#
# Module_linguistic.mk gates this test behind `ifeq ($(OS),LINUX)` (the
# `else` branch drops it and keeps only JunitTest_linguistic_unoapi, itself
# out of scope like every other JunitTest in this port) -- this port only
# ever targets Linux (see cmake/LibreStudioPlatform.cmake), so that gate
# always takes the LINUX branch here and the test is wired unconditionally,
# not behind an `if (LINUX)` guard that could never be false.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not linguistic's, source paths below are still fine:
# librestudio_expand_cxx (cmake/LibreStudioPlatform.cmake) resolves them
# against CMAKE_SOURCE_DIR (the absolute tree root), never
# CMAKE_CURRENT_SOURCE_DIR, so it does not matter which directory's scope
# this file executes in.
#
# CppunitTest_linguistic_restprotocol.mk, re-checked directly against the
# live file: 1 source (linguistic/qa/restprotocol), use_libraries comphelper
# cppu cppuhelper sal svt utl test unotest ("test" -> test_fixtures, this
# port's established rename -- enable_testing() in the root CMakeLists.txt
# reserves the bare name "test" for CTest's own built-in target, see test/
# CMakeLists.txt's own comment on this), use_api(udkapi,offapi,oovbaapi) (all
# three "normal" variant), use_configuration -> CONFIGURATION, use_ure -> 2
# protector pairs (unobootstrapprotector/unoexceptionprotector -- no use_vcl
# in this .mk, confirmed by reading the whole file, so no vclbootstrapprotector
# pair), use_rdb(services) -> DEPENDS program_services_rdb (see above),
# use_custom_headers(officecfg/registry) -> OFFICECFG_INCLUDE_DIR +
# officecfg_headers, same as linguistic/CMakeLists.txt's own lng target.
#
# All libraries verified as real targets before wiring (grep -n
# "add_library(<name> "): comphelper (comphelper/CMakeLists.txt), cppu (cppu/
# CMakeLists.txt), cppuhelper (cppuhelper/CMakeLists.txt), sal (sal/
# CMakeLists.txt), svt (svtools/CMakeLists.txt), utl (unotools/CMakeLists.txt),
# test_fixtures (test/CMakeLists.txt), unotest (unotest/CMakeLists.txt).
# unobootstrapprotector/unoexceptionprotector (unotest/CMakeLists.txt) both
# resolve too, same protector targets svl_urihelper/svl_qa_cppunit already
# use.
librestudio_add_cppunit_test(linguistic_restprotocol
    SOURCES
        linguistic/qa/restprotocol
    LIBRARIES
        comphelper cppu cppuhelper sal svt utl test_fixtures unotest
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
    CONFIGURATION)
