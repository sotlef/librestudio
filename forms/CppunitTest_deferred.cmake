# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# forms_unit (CppunitTest_forms_unit.mk) -- split out of forms/CMakeLists.txt
# into its own file, include()'d from the root CMakeLists.txt only after
# program_services_rdb exists, the same reason and call-site pattern as
# scaddins/CppunitTest_deferred.cmake and officecfg/CppunitTest_
# deferred.cmake (read scaddins/CppunitTest_deferred.cmake's header comment
# first for the full use_rdb(services)/ini-fallback reasoning, which applies
# identically here -- this test calls no use_components either, so it gets
# the same treatment: no COMPONENTS, DEPENDS program_services_rdb, and
# cppunittester's UNO bootstrap falls back to unorc's own
# "UNO_SERVICES=${ORIGIN}/services.rdb" ini default).
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not forms's, source paths below are still fine: librestudio_expand_cxx
# (cmake/LibreStudioPlatform.cmake) resolves them against CMAKE_SOURCE_DIR
# (the absolute tree root), never CMAKE_CURRENT_SOURCE_DIR, so it does not
# matter which directory's scope this file executes in.
#
# CppunitTest_forms_unit.mk, re-checked directly against the live file: 1
# source (forms/qa/unit/forms_test), use_sdk_api (udkapi+offapi "normal"
# variant, use_sdk_api not use_api -- no oovbaapi here, unlike Library_frm.mk's
# own use_api trio), use_externals(libxml2) -> LibXml2::LibXml2 + its include
# dir (same as officecfg_accelerator_key_checker_test's own libxml2 wiring),
# use_libraries comphelper cppu cppuhelper sal subsequenttest test tl unotest
# utl ("test" -> test_fixtures, this port's established rename -- see
# linguistic/CppunitTest_deferred.cmake's own note on this), use_ure +
# use_vcl -> 3 protector pairs (unobootstrapprotector/unoexceptionprotector/
# vclbootstrapprotector, same precedent as officecfg_accelerator_key_
# checker_test's own use_ure+use_vcl combination), use_rdb(services) ->
# DEPENDS program_services_rdb (see above), use_configuration ->
# CONFIGURATION.
#
# All libraries verified as real targets before wiring (grep -n
# "add_library(<name> "): comphelper (comphelper/CMakeLists.txt), cppu (cppu/
# CMakeLists.txt), cppuhelper (cppuhelper/CMakeLists.txt), sal (sal/
# CMakeLists.txt), subsequenttest (test/CMakeLists.txt), test_fixtures (test/
# CMakeLists.txt), tl (tools/CMakeLists.txt), unotest (unotest/CMakeLists.txt),
# utl (unotools/CMakeLists.txt). unobootstrapprotector/unoexceptionprotector
# (unotest/CMakeLists.txt) and vclbootstrapprotector (test/CMakeLists.txt) all
# resolve too, same protector targets officecfg_accelerator_key_checker_test/
# toolkit_toolkit already use.
librestudio_add_cppunit_test(forms_unit
    SOURCES
        forms/qa/unit/forms_test
    LIBRARIES
        comphelper cppu cppuhelper sal subsequenttest test_fixtures tl
        unotest utl LibXml2::LibXml2
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${LIBXML2_INCLUDE_DIRS}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)
