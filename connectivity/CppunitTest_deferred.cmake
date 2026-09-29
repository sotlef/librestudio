# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# connectivity has 5 CppunitTest_*.mk files. 2 of them (connectivity_
# commontools, connectivity_sharedresources) need only "dbtools" (built
# unconditionally by connectivity/CMakeLists.txt, see that file's own header
# comment) plus the configmgr/i18npool components -- both real, already-
# registered targets by the time connectivity's own add_subdirectory() runs
# (officecfg/i18npool/configmgr all sit textually before connectivity's own
# add_subdirectory() line in the root CMakeLists.txt). The only reason either
# needs deferral at all is use_configuration -> CONFIGURATION itself, which
# requires LIBRESTUDIO_TEST_UNITTEST_DIR (only set once test/CMakeLists.txt
# has run, well AFTER connectivity's own position) -- the same later-binding
# constraint documented in e.g. svgio/CppunitTest_deferred.cmake's own header
# comment.
#
# The other 3 (connectivity_ado, connectivity_firebird_test, connectivity_
# mysql_test) are NOT ported here -- a real, current gap, not a settled
# scope decision (see connectivity/CMakeLists.txt's own module-level header
# comment, corrected 2026-09-28 after an earlier version of it mischaracterized
# this as "out of scope"; [[feedback_gbuild_full_replacement_scope]] in
# project memory covers why that framing is wrong). Each of these 3 tests
# links directly against exactly one driver library this port has not built
# yet, for its own reason -- connectivity_ado's own use_libraries names "ado"
# itself (Windows-only upstream -- ADO is a COM API, genuinely unreachable
# on this Linux port, gated `ifeq ($(OS),WNT)` in the real Module_
# connectivity.mk), connectivity_firebird_test's use_libraries needs no
# firebird-named library directly but its whole raison d'être is
# firebird_sdbc (re-checked: connectivity/qa/connectivity/firebird/firebird.
# cxx exercises the firebird SDBC driver's own connection/statement classes,
# which do not exist in this tree -- needs a Firebird client library this
# port has not vendored), and connectivity_mysql_test's own use_components
# list names connectivity/source/drivers/mysqlc/mysqlc directly (needs the
# MariaDB/MySQL Connector-C client library, also not vendored here --
# notably, configure.ac defaults this driver ON upstream, so this is a real
# to-do, not an inherited platform limitation like ado). None of the three
# has anywhere to link/depend on yet, so wiring them would mean either
# inventing a stub (misrepresenting what they test) or FATAL_ERROR-ing at
# configure time on a target that does not exist -- neither is done here.
# This is the same "not reproduced, not silently worked around either"
# treatment filter's own textfilterdetect/xslt gap and desktop_lib's
# postprocess_images gap already
# get.

# --- connectivity_commontools --------------------------------------------------
#
# CppunitTest_connectivity_commontools.mk, re-checked directly: 3 sources
# (connectivity/qa/connectivity/commontools/FValue_test, DBTypeConversion_
# test, SQLCommentTest), set_include (-Iconnectivity/source/inc $$(INCLUDE)),
# use_external(boost_headers), use_ure + use_vcl (plain, headless) -> 3
# protector pairs, use_sdk_api (normal), use_libraries (cppu dbtools sal test
# unotest -- "test" -> test_fixtures), use_components (configmgr/source/
# configmgr, i18npool/util/i18npool) -> COMPONENTS configmgr i18npool,
# use_configuration -> CONFIGURATION.
librestudio_add_cppunit_test(connectivity_commontools
    SOURCES
        connectivity/qa/connectivity/commontools/FValue_test
        connectivity/qa/connectivity/commontools/DBTypeConversion_test
        connectivity/qa/connectivity/commontools/SQLCommentTest
    LIBRARIES
        cppu dbtools sal test_fixtures unotest
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/connectivity/source/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool
    CONFIGURATION)

# --- connectivity_sharedresources -----------------------------------------------
#
# CppunitTest_connectivity_sharedresources.mk, re-checked directly: 1 source
# (connectivity/qa/connectivity/resource/sharedresources_test), set_include
# (-Iconnectivity/inc -Iconnectivity/source/inc $$(INCLUDE)), use_external(
# boost_headers), use_ure + use_vcl (plain, headless) -> same 3 protector
# pairs, use_sdk_api (normal), use_libraries (dbtools sal test utl unotest --
# "test" -> test_fixtures), use_components (configmgr/source/configmgr,
# i18npool/util/i18npool) -> COMPONENTS configmgr i18npool, use_configuration
# -> CONFIGURATION. Same shape as connectivity_commontools above, one
# source/no boost-Boost-only-need-vs-full-set difference (no "cppu" in this
# one's own use_libraries -- re-verified directly, not assumed from the other
# test's list).
librestudio_add_cppunit_test(connectivity_sharedresources
    SOURCES
        connectivity/qa/connectivity/resource/sharedresources_test
    LIBRARIES
        dbtools sal test_fixtures utl unotest
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/connectivity/inc"
        "${CMAKE_SOURCE_DIR}/connectivity/source/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool
    CONFIGURATION)

# connectivity/CppunitTest_deferred.cmake needs LIBRESTUDIO_TEST_UNITTEST_DIR
# (set only once test/CMakeLists.txt has run, well after connectivity's own
# add_subdirectory() position) -- see connectivity/CMakeLists.txt's own
# cmake_language(DEFER) call site comment for the scheduling mechanism, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required there.
