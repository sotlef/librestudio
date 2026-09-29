# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# i18npool_defaultnumberingprovider (CppunitTest_i18npool_
# defaultnumberingprovider.mk) -- the only one of i18npool's 8 CppunitTest_
# *.mk files that needs deferral; the other 7 are wired directly in
# i18npool/CMakeLists.txt (see that file's own header comment on this
# module-level split).
#
# Re-checked directly against the live file: 1 source (i18npool/qa/cppunit/
# test_defaultnumberingprovider), use_libraries comphelper cppu sal test
# unotest utl ("test" -> test_fixtures), use_sdk_api, use_ure + use_vcl -> 3
# protector pairs, use_rdb(services) -> DEPENDS program_services_rdb (no
# COMPONENTS -- ini-fallback default, same as every other use_rdb(services)-
# only test in this port), use_custom_headers(officecfg/registry) ->
# OFFICECFG_INCLUDE_DIR + DEPENDS officecfg_headers, use_configuration ->
# CONFIGURATION.
#
# Historical note: project memory once claimed this test was already wired;
# it was not (re-verified directly against the live tree before writing
# this file, per this project's own "never trust a prior claim, re-check the
# .mk" convention) -- ported here for real now.
librestudio_add_cppunit_test(i18npool_defaultnumberingprovider
    SOURCES
        i18npool/qa/cppunit/test_defaultnumberingprovider
    LIBRARIES
        comphelper cppu sal test_fixtures unotest utl
    INCLUDE
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        officecfg_headers udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)
