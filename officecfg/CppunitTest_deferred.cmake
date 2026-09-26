# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# officecfg_accelerator_key_checker_test (CppunitTest_officecfg_
# accelerator_key_checker_test.mk) -- split out of officecfg/CMakeLists.txt
# into its own file, include()'d from the root CMakeLists.txt only after
# program_services_rdb exists, the same reason and call-site pattern as
# scaddins/CppunitTest_deferred.cmake and xmlscript/CppunitTest_deferred.cmake
# (read scaddins/CppunitTest_deferred.cmake's header comment first for the
# full use_rdb(services)/ini-fallback reasoning, which applies identically
# here -- this test calls no use_components either, so it gets the same
# treatment: no COMPONENTS, DEPENDS program_services_rdb, and cppunittester's
# UNO bootstrap falls back to unorc's own "UNO_SERVICES=${ORIGIN}/
# services.rdb" ini default).
#
# RESOLVED: officecfg/CMakeLists.txt previously left this test entirely
# unwired ("NOT wired here", in this test's old place there) because its one
# source, officecfg/qa/acceleratorkeychecker.cxx, #includes
# <test/unoapi_test.hxx>, which only exists in Library_subsequenttest.mk's
# "subsequenttest" target -- explicitly out of scope for this port's test
# tier at the time (test/CMakeLists.txt's own header comment). subsequenttest
# is now ported as a real target (test/CMakeLists.txt, add_library
# (subsequenttest SHARED ...)), so that blocker is gone; the only remaining
# reason this could not simply be registered in officecfg/CMakeLists.txt
# itself is the program_services_rdb ordering above, which is why it lives
# here instead.
#
# CppunitTest_officecfg_accelerator_key_checker_test.mk, re-checked directly
# against the live file: 1 source (officecfg/qa/acceleratorkeychecker),
# use_libraries cppu cppuhelper sal subsequenttest test unotest utl vcl tl
# ("test" -> test_fixtures, this port's established rename), use_api(udkapi,
# offapi,oovbaapi) (all three "normal" variant, use_api not use_sdk_api/
# use_internal_api -- same normal-header mapping this port's o3tl/basegfx/
# officecfg_cppheader_test precedent already applies), use_externals
# (boost_headers,libxml2) -> Boost::boost + LibXml2::LibXml2 (+ its include
# dir, same as xmlscript_cppunit above -- transcribed as the .mk declares it,
# even though acceleratorkeychecker.cxx's own #include list names no
# libxml/*.h directly, faithful to what the .mk declares rather than
# second-guessed from one file's includes, same posture xmlscript_cppunit's
# own officecfg/registry include already took), use_ure + use_vcl -> 3
# protector pairs (use_vcl here is called as "use_vcl,officecfg_
# accelerator_key_checker_test,services" in the live .mk, but gb_CppunitTest_
# use_vcl's own definition, solenv/gbuild/CppunitTest.mk:286-289, takes only
# one parameter and never references a $(2) -- make's $(call) silently
# ignores an extra argument nothing in the macro body reads, so the trailing
# ",services" there is inert, not a distinct call shape; verified against the
# macro definition rather than assumed), use_rdb(services) -> DEPENDS
# program_services_rdb (see above), use_configuration -> CONFIGURATION.
#
# All libraries verified as real targets before wiring (grep -n
# "add_library(<name> "): cppu (cppu/CMakeLists.txt), cppuhelper (cppuhelper/
# CMakeLists.txt), sal (sal/CMakeLists.txt), subsequenttest (test/
# CMakeLists.txt), test_fixtures (test/CMakeLists.txt), unotest (unotest/
# CMakeLists.txt), utl (unotools/CMakeLists.txt), vcl (vcl/CMakeLists.txt) --
# "tl" is tools' own target name mismatch note: tools/CMakeLists.txt's
# library is actually named "tl" too (gbuild's own Library_tl.mk spelling),
# so no rename needed there either. unobootstrapprotector/
# unoexceptionprotector (unotest/CMakeLists.txt) and vclbootstrapprotector
# (test/CMakeLists.txt) all resolve too, same protector targets toolkit_
# toolkit/scaddins_analysis already use.
librestudio_add_cppunit_test(officecfg_accelerator_key_checker_test
    SOURCES
        officecfg/qa/acceleratorkeychecker
    LIBRARIES
        cppu cppuhelper sal subsequenttest test_fixtures unotest utl vcl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
        "${LIBXML2_INCLUDE_DIRS}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)
