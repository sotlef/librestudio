# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# lotuswordpro_import_test (CppunitTest_lotuswordpro_import_test.mk) -- split
# out of lotuswordpro/CMakeLists.txt into its own file, include()'d from the
# root CMakeLists.txt only after program_services_rdb is defined. Unlike
# lotuswordpro_test_lotuswordpro (wired directly in lotuswordpro/CMakeLists.
# txt -- see that file's own comment on it), this test calls gb_CppunitTest_
# use_rdb(name,services) (see scaddins/CppunitTest_deferred.cmake's own header
# comment for the full "ini fallback" reasoning this port gives that call: no
# COMPONENTS, a DEPENDS on program_services_rdb -- not defined until the very
# end of the root CMakeLists.txt, well after lotuswordpro's own
# add_subdirectory() position (~1028) -- and cppunittester's UNO bootstrap
# falling back to unorc's own ini default for UNO_SERVICES). Its other
# dependencies (unotest/test's protector plug-ins and subsequenttest,
# use_configuration's officecfg_registry/LIBRESTUDIO_TEST_UNITTEST_DIR) are
# all add_subdirectory()'d well before lotuswordpro's own position already
# (unotest/test sit at ~1017-1018, before lotuswordpro at ~1028), so
# program_services_rdb is the sole binding constraint here.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not lotuswordpro's, the source path below is still fine:
# librestudio_expand_cxx (cmake/LibreStudioPlatform.cmake) resolves it against
# CMAKE_SOURCE_DIR (the absolute tree root), never CMAKE_CURRENT_SOURCE_DIR.
#
# CppunitTest_lotuswordpro_import_test.mk, re-checked directly against the
# live file: 1 source (lotuswordpro/qa/cppunit/import_test), use_externals
# (boost_headers libxml2), use_libraries (comphelper cppu cppuhelper sal
# subsequenttest test tl unotest utl vcl -- "test" -> test_fixtures, this
# port's established rename; "subsequenttest" confirmed load-bearing by
# import_test.cxx's own #include <test/unoapi_test.hxx>, compiled into
# Library_subsequenttest.mk's "subsequenttest" target, not test_fixtures --
# same precedent as emfio_emf/svx_core/editeng_editeng's own subsequenttest
# uses), use_sdk_api (normal udkapi+offapi), use_ure + use_vcl -> 3 protector
# pairs (unobootstrapprotector x2, unoexceptionprotector x2,
# vclbootstrapprotector x2), use_rdb(services) -> DEPENDS program_services_rdb
# (no COMPONENTS, see file header above), use_configuration -> CONFIGURATION.
#
# All libraries verified as real targets (grep -n "add_library(<name> "):
# comphelper, cppu, cppuhelper, sal, subsequenttest (test/CMakeLists.txt),
# test_fixtures (test/CMakeLists.txt), tl (tools/CMakeLists.txt), unotest, utl
# (unotools/CMakeLists.txt), vcl -- all resolve.
librestudio_add_cppunit_test(lotuswordpro_import_test
    SOURCES
        lotuswordpro/qa/cppunit/import_test
    LIBRARIES
        comphelper cppu cppuhelper sal subsequenttest test_fixtures tl unotest
        utl vcl Boost::boost LibXml2::LibXml2
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

# import_test.cxx's fixture needs SRC_ROOT/WORKDIR_FOR_BUILD
# (BootstrapFixtureBase's Directories member) -- now handled generically by
# librestudio_add_cppunit_test() itself (cmake/LibreStudioTest.cmake).
