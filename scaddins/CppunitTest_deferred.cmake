# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# scaddins_analysis (CppunitTest_scaddins_analysis.mk) -- split out of
# scaddins/CMakeLists.txt into its own file, include()'d from the root
# CMakeLists.txt only after program_services_rdb exists (see that include()
# call site for why: this test needs the production services.rdb, and that
# target is not created until the very end of the root CMakeLists.txt, well
# after scaddins' own add_subdirectory()).
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not scaddins', anything scaddins/CMakeLists.txt built that is a real
# CMake target (scaddins_comprehensive_headers, and the cache/GLOBAL
# variables librestudio_uno_api_headers() records) is still visible here --
# only directory-scoped IMPORTED_TARGETs would not be, and none are needed
# below.
#
# CppunitTest_scaddins_analysis.mk, re-checked directly against the live
# file: 1 source (scaddins/qa/analysis), use_libraries comphelper cppu sal
# test unotest -- "test" is test_fixtures here, same rename every other
# CppunitTest in this port already applies (test/CMakeLists.txt's own
# comment explains why the bare name "test" is unavailable). use_ure +
# use_vcl -> 3 protector pairs, same precedent as sax_parser/
# hwpfilter_test_hwpfilter. use_sdk_api is NOT called (scaddins/qa/
# analysis.cxx never uses udkapi/offapi's plain headers on their own); what
# IS called is use_internal_comprehensive_api(scaddins_analysis, scaddins) --
# gb_LinkTarget_use_internal_comprehensive_api resolves udkapi, offapi AND
# the named module (scaddins) all to their "comprehensive" (-C) header
# variant together (LinkTarget.mk, confirmed by reading it directly, not
# assumed) -- so this needs the SAME three include dirs/header targets
# scaddins/CMakeLists.txt's own "analysis" production library already uses
# for the identical reason (see that target's own target_include_directories/
# add_dependencies): scaddins_comprehensive_HEADERS_INCLUDE_DIR (this
# module's own private UNO API, scaddins/CMakeLists.txt's
# librestudio_uno_api_headers(scaddins_comprehensive ...) call) together with
# udkapi_comprehensive_HEADERS_INCLUDE_DIR and
# offapi_comprehensive_HEADERS_INCLUDE_DIR -- not udkapi_comprehensive alone,
# confirmed against scaddins/qa/analysis.cxx's own #include list
# (com/sun/star/sheet/addin/XAnalysis.hpp needs scaddins_comprehensive;
# com/sun/star/lang/XMultiServiceFactory.hpp needs offapi_comprehensive).
#
# use_rdb(services,services) (gb_CppunitTest_use_rdb): appends the whole
# packed services.rdb's own file path to this test's UNO_SERVICES list, on
# top of (not instead of) whatever use_components would have added -- this
# .mk calls no use_components, so UNO_SERVICES is services.rdb alone.
# librestudio_add_cppunit_test() has no dedicated parameter for "the whole
# rdb", only COMPONENTS (named individual .component fragments) -- but since
# this test passes no COMPONENTS either, the generic function emits NO
# "-env:UNO_SERVICES=..." argument at all for it, which means cppunittester's
# own UNO bootstrap falls back to unorc's ini default instead (cppuhelper/
# source/paths.cxx's getUnoIniUri() finds unorc next to itself in
# LIBRESTUDIO_PROGRAM_DIR, the same directory cppunittester and every
# CppunitTest .so land in -- see cmake/LibreStudioUnorc.cmake) -- and that
# default is literally "UNO_SERVICES=${ORIGIN}/services.rdb"
# (librestudio_generate_unorc(), cmake/LibreStudioUnorc.cmake), i.e. exactly
# what use_rdb(services) asks for. This is not a coincidence being relied on
# blindly: this port already leans on the same ini fallback for every other
# piece of use_ure's environment it does not model explicitly (UNO_TYPES,
# CONFIGURATION_LAYERS's own instdir-only upstream default, BRAND_BASE_DIR --
# see cmake/LibreStudioTest.cmake's file header comment), so treating
# UNO_SERVICES the same way when nothing else claims it is consistent, not a
# new kind of gap. What DOES need to be explicit is the build-order
# dependency -- an ini fallback only helps if the file it names actually
# exists by the time the test runs -- hence DEPENDS program_services_rdb
# below (the root CMakeLists.txt target that copies services.rdb into
# LIBRESTUDIO_PROGRAM_DIR).
#
# use_configuration -> CONFIGURATION (cmake/LibreStudioTest.cmake).
librestudio_add_cppunit_test(scaddins_analysis
    SOURCES
        scaddins/qa/analysis
    LIBRARIES comphelper cppu sal test_fixtures unotest
    INCLUDE
        "${scaddins_comprehensive_HEADERS_INCLUDE_DIR}"
        "${udkapi_comprehensive_HEADERS_INCLUDE_DIR}"
        "${offapi_comprehensive_HEADERS_INCLUDE_DIR}"
    DEPENDS
        scaddins_comprehensive_headers
        udkapi_comprehensive_headers
        offapi_comprehensive_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)
