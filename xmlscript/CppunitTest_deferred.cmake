# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# xmlscript_cppunit (CppunitTest_xmlscript_cppunit.mk) -- split out of
# xmlscript/CMakeLists.txt into its own file, include()'d from the root
# CMakeLists.txt only after program_services_rdb exists, the same reason and
# call-site pattern as scaddins/CppunitTest_deferred.cmake (read that file's
# header comment first for the full use_rdb(services)/ini-fallback
# reasoning, which applies identically here -- this test calls no
# use_components either, so it gets the same treatment: no COMPONENTS,
# DEPENDS program_services_rdb, and cppunittester's UNO bootstrap falls back
# to unorc's own "UNO_SERVICES=${ORIGIN}/services.rdb" ini default). This
# test additionally needs framework (fwk, add_subdirectory()'d well after
# xmlscript's own) and test_fixtures/the protector plug-ins
# (add_subdirectory(test), likewise after), so it could not be registered at
# xmlscript's own natural position even without the services.rdb ordering
# constraint.
#
# CppunitTest_xmlscript_cppunit.mk, re-checked directly against the live
# file: 1 source (xmlscript/qa/cppunit/test), use_sdk_api (normal udkapi/
# offapi headers), use_ure + use_vcl -> 3 protector pairs (same precedent as
# scaddins_analysis/hwpfilter_test_hwpfilter), use_rdb(services) -> see
# scaddins/CppunitTest_deferred.cmake's comment, use_custom_headers
# officecfg/registry -> OFFICECFG_INCLUDE_DIR + DEPENDS officecfg_headers
# (transcribed as the .mk states it, even though xmlscript/qa/cppunit/
# test.cxx's own #include list names no officecfg/*.hxx directly -- faithful
# to what the .mk declares rather than second-guessed from one file's
# includes, since a transitive include elsewhere in the same TU could still
# need it), use_externals libxml2 -> LibXml2::LibXml2 + its include dir
# (test.cxx includes <libxml/xpathInternals.h> directly), use_libraries
# comphelper cppu cppuhelper fwk sal test unotest utl xmlscript ("test" is
# test_fixtures here, same rename as every other CppunitTest in this port).
# use_configuration -> CONFIGURATION (cmake/LibreStudioTest.cmake).
librestudio_add_cppunit_test(xmlscript_cppunit
    SOURCES
        xmlscript/qa/cppunit/test
    LIBRARIES comphelper cppu cppuhelper fwk sal test_fixtures unotest utl xmlscript
        LibXml2::LibXml2
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
        "${LIBXML2_INCLUDE_DIRS}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)
