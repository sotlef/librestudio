# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# framework_dispatch, framework_loadenv, framework_CheckXTitle,
# framework_accelerators, framework_services -- all 5 of framework's
# CppunitTest_*.mk files, split out of framework/CMakeLists.txt into their
# own file, include()'d from the root CMakeLists.txt only after
# program_services_rdb exists -- the same reason and call-site pattern as
# scaddins/CppunitTest_deferred.cmake, xmlscript/CppunitTest_deferred.cmake
# and vcl/CppunitTest_deferred.cmake's first sub-wave (read scaddins' file
# header comment first for the full use_rdb(services)/ini-fallback
# reasoning, which applies identically to all 5 tests below: each calls
# gb_CppunitTest_use_rdb(name,services) -- re-checked directly against every
# one of these tests' live .mk files, not assumed from any prior summary --
# so each gets no COMPONENTS, a DEPENDS on program_services_rdb, and relies
# on cppunittester's UNO bootstrap falling back to unorc's own
# "UNO_SERVICES=${ORIGIN}/services.rdb" ini default). Every one of them also
# calls gb_CppunitTest_use_configuration, which additionally requires
# LIBRESTUDIO_TEST_UNITTEST_DIR (test/CMakeLists.txt's staged test/
# user-template copy) -- re-checked against the root CMakeLists.txt's own
# add_subdirectory() order: framework itself is add_subdirectory()'d at line
# ~523, well BEFORE unotest/test (~687-688) -- so CONFIGURATION could not
# have been satisfied at framework's own natural registration point either,
# even setting use_rdb(services)/program_services_rdb aside entirely -- both
# gaps push these tests to the same deferred-file treatment, and
# program_services_rdb (only defined much later still, right before this
# file's own include() point) ends up being the binding constraint in
# practice, exactly as it was for scaddins_analysis/xmlscript_cppunit/vcl's
# own use_rdb(services) tests.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not framework's, source paths below are still fine:
# librestudio_expand_cxx resolves them against CMAKE_SOURCE_DIR, not
# CMAKE_CURRENT_SOURCE_DIR (see sax/CppunitTest_deferred.cmake's own comment
# on this same point). fwk, vcl and every other LIBRARIES entry below are
# already real targets by this point -- framework/vcl/etc. were all
# add_subdirectory()'d long before this file's own include() site.
#
# All 5 tests share the identical shape: 1 source under framework/qa/cppunit/,
# use_sdk_api (udkapi+offapi "normal" headers), use_externals(boost_headers
# libxml2), use_ure + use_vcl (3 protector pairs -- unobootstrapprotector,
# unoexceptionprotector, vclbootstrapprotector, same precedent as every other
# use_ure+use_vcl test in this port), use_rdb(services), use_configuration.
# None calls set_include or use_components. They differ only in their
# use_libraries list:
#   - framework_dispatch / framework_accelerators: comphelper cppu cppuhelper
#     fwk sal subsequenttest utl tl test unotest (fwk itself, no salhelper/vcl
#     directly).
#   - framework_loadenv / framework_CheckXTitle / framework_services:
#     comphelper cppu cppuhelper sal salhelper subsequenttest test unotest
#     utl tl vcl (salhelper+vcl directly, no fwk directly).
# "test" -> test_fixtures and "subsequenttest" -> subsequenttest, this port's
# already-established renames (test/CMakeLists.txt: add_library(test_fixtures
# ...) / add_library(subsequenttest ...); "unotest" keeps its own bare name,
# unotest/CMakeLists.txt: add_library(unotest ...)). "fwk" is framework/
# CMakeLists.txt's own add_library(fwk SHARED ...) target.

# --- framework_dispatch ---------------------------------------------------------
#
# CppunitTest_framework_dispatch.mk: 1 source (framework/qa/cppunit/
# dispatchtest).
librestudio_add_cppunit_test(framework_dispatch
    SOURCES
        framework/qa/cppunit/dispatchtest
    LIBRARIES
        comphelper cppu cppuhelper fwk sal subsequenttest utl tl test_fixtures
        unotest Boost::boost LibXml2::LibXml2
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
    CONFIGURATION)

# --- framework_loadenv ----------------------------------------------------------
#
# CppunitTest_framework_loadenv.mk: 1 source (framework/qa/cppunit/loadenv).
librestudio_add_cppunit_test(framework_loadenv
    SOURCES
        framework/qa/cppunit/loadenv
    LIBRARIES
        comphelper cppu cppuhelper sal salhelper subsequenttest test_fixtures
        unotest utl tl vcl Boost::boost LibXml2::LibXml2
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
    CONFIGURATION)

# --- framework_CheckXTitle ------------------------------------------------------
#
# CppunitTest_framework_CheckXTitle.mk: 1 source (framework/qa/cppunit/
# CheckXTitle).
librestudio_add_cppunit_test(framework_CheckXTitle
    SOURCES
        framework/qa/cppunit/CheckXTitle
    LIBRARIES
        comphelper cppu cppuhelper sal salhelper subsequenttest test_fixtures
        unotest utl tl vcl Boost::boost LibXml2::LibXml2
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
    CONFIGURATION)

# --- framework_accelerators -----------------------------------------------------
#
# CppunitTest_framework_accelerators.mk: 1 source (framework/qa/cppunit/
# accelerators).
librestudio_add_cppunit_test(framework_accelerators
    SOURCES
        framework/qa/cppunit/accelerators
    LIBRARIES
        comphelper cppu cppuhelper fwk sal subsequenttest utl tl test_fixtures
        unotest Boost::boost LibXml2::LibXml2
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
    CONFIGURATION)

# --- framework_services ---------------------------------------------------------
#
# CppunitTest_framework_services.mk: 1 source (framework/qa/cppunit/
# services). Module_framework.mk wraps this one target's registration in
# "ifneq ($(OS),MACOSX)" with the comment "Not sure why this is not stable on
# macOS" -- preserved faithfully here as if (NOT APPLE), rather than dropped
# or silently made unconditional, so this test is simply never registered
# (not registered-then-skipped) on an Apple build of this port.
if (NOT APPLE)
    librestudio_add_cppunit_test(framework_services
        SOURCES
            framework/qa/cppunit/services
        LIBRARIES
            comphelper cppu cppuhelper sal salhelper subsequenttest
            test_fixtures unotest utl tl vcl Boost::boost LibXml2::LibXml2
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
        CONFIGURATION)
endif ()
