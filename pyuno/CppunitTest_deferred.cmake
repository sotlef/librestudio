# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# pyuno_pythonhome (CppunitTest_pyuno_pythonhome.mk) -- split out of
# pyuno/CMakeLists.txt into its own file, scheduled via cmake_language(DEFER)
# (see pyuno/CMakeLists.txt's own tail) because its gb_CppunitTest_use_rdb
# (services) call needs program_services_rdb, which does not exist yet at
# pyuno/CMakeLists.txt's own add_subdirectory() position (root CMakeLists.txt
# line 1278, well before program_services_rdb is defined). Missed during the
# 2026-09-27 "modules with 1-2 tests" wave -- this module simply was not on
# that wave's module list, not a deliberate exclusion; ported now the same
# way as every other test from that wave.
#
# CppunitTest_pyuno_pythonhome.mk, re-checked directly against the live file:
# 1 source (pyuno/qa/cppunit/testpythonhome), use_libraries cppu cppuhelper
# sal subsequenttest test tl unotest utl ("test" -> test_fixtures, this
# port's established rename), use_externals(libxml2), use_api(udkapi offapi)
# -- the normal variant of both (use_api, not use_sdk_api), use_components
# (pyuno/source/loader/pythonloader) -> "pythonloader" (pyuno/CMakeLists.txt's
# own librestudio_add_component(pythonloader ...) call), use_ure + use_vcl
# (plain, headless/SVP variant, not use_vcl_non_headless[_with_windows]) ->
# 3 protector pairs, no NONHEADLESS, use_rdb(services) -> DEPENDS
# program_services_rdb, use_configuration -> CONFIGURATION.
librestudio_add_cppunit_test(pyuno_pythonhome
    SOURCES
        pyuno/qa/cppunit/testpythonhome
    LIBRARIES
        cppu cppuhelper sal subsequenttest test_fixtures tl unotest utl
        LibXml2::LibXml2
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
    COMPONENTS
        pythonloader
    CONFIGURATION)
