# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# filter's 7 CppunitTest_*.mk files, re-checked directly. 5 are wired below;
# 2 are genuine, honest gaps -- NOT attempted, not silently invented:
#
#   - CppunitTest_filter_textfilterdetect.mk needs use_libraries(...,textfd,...)
#     -- "textfd" (Library_textfd.mk) is one of the filter/ libraries
#     filter/CMakeLists.txt's own module-level comment explicitly lists as
#     "still need their externals individually re-verified in a future wave
#     ... remain out of scope for this task" -- not ported anywhere in this
#     tree (grepped: no add_library(textfd ...) exists). Blocked on real
#     production work, not a wiring gap.
#   - CppunitTest_filter_xslt.mk needs use_components(...,filter/source/
#     xsltfilter/xsltfilter,...) -- "xsltfilter" (Library_xsltfilter.mk) is
#     in the exact same unported list. Same blocker.
#
# Neither CppunitTest_filter_pdf.mk nor CppunitTest_filter_svg.mk actually
# need filter's own unported pdffilter/svgfilter libraries despite their
# names -- re-checked directly: neither .mk's use_libraries lists pdffilter/
# svgfilter, and filter/qa/pdf.cxx / filter/qa/unit/svg.cxx both drive the
# document export purely through the UNO com.sun.star.document.XFilter/
# XExporter interfaces (real UNO service lookups at runtime, not a compile-
# time link) -- so both are genuinely wireable now. Whether the underlying
# PDF/SVG export UNO services actually resolve at runtime (they may not,
# since pdffilter/svgfilter themselves are unported) is a real, separate
# question this wiring does not resolve -- consistent with this project's
# "wire what compiles, let real gaps surface as runtime failures" posture
# from the 2026-09-27 "1-2 tests per module" wave.

# --- filter_msfilter (CppunitTest_filter_msfilter.mk) -------------------------
#
# Re-checked directly: 1 source (filter/qa/cppunit/msfilter-test), use_sdk_api
# (normal udkapi+offapi), use_ure -> 2 protector pairs (NO use_vcl call in
# this .mk -- confirmed, so no vclbootstrapprotector), use_configuration ->
# CONFIGURATION, use_externals(boost_headers), use_libraries tl comphelper
# unotest cppuhelper cppu msfilter sal, use_components (6 names):
#   - "configmgr/source/configmgr" -> "configmgr".
#   - "filter/source/config/cache/filterconfig1" -> "filterconfig"
#     (filter/CMakeLists.txt's own librestudio_add_component(filterconfig
#     ...) call, ported earlier in this same file).
#   - "framework/util/fwk" -> "fwk".
#   - "i18npool/util/i18npool" -> "i18npool".
#   - "ucb/source/core/ucb1" -> "ucb1", "ucb/source/ucp/file/ucpfile1" ->
#     "ucpfile1" -- both ucb/CMakeLists.txt, add_subdirectory()'d at root
#     CMakeLists.txt line 1151, AFTER filter's own line 1130, which is why
#     this whole file's include() must be deferred past that point.
librestudio_add_cppunit_test(filter_msfilter
    SOURCES
        filter/qa/cppunit/msfilter-test
    LIBRARIES
        tl comphelper unotest cppuhelper cppu msfilter sal Boost::boost
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
    COMPONENTS
        configmgr filterconfig fwk i18npool ucb1 ucpfile1
    CONFIGURATION)

# --- filter_priority (CppunitTest_filter_priority.mk) -------------------------
#
# Re-checked directly: 1 source (filter/qa/cppunit/priority-test), use_sdk_api,
# use_ure + use_vcl -> 3 protector pairs (unlike filter_msfilter above, this
# one DOES call use_vcl), use_configuration -> CONFIGURATION, use_external
# (boost_headers), use_libraries comphelper unotest cppuhelper cppu sal,
# use_components -- the identical 6-name set as filter_msfilter above (same
# resolution, same ucb1/ucpfile1 late-binding constraint).
librestudio_add_cppunit_test(filter_priority
    SOURCES
        filter/qa/cppunit/priority-test
    LIBRARIES
        comphelper unotest cppuhelper cppu sal Boost::boost
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr filterconfig fwk i18npool ucb1 ucpfile1
    CONFIGURATION)

# --- filter_pdf (CppunitTest_filter_pdf.mk) -----------------------------------
#
# Re-checked directly: 1 source (filter/qa/pdf), use_externals(boost_headers,
# libxml2), use_libraries basegfx comphelper cppu cppuhelper sal
# subsequenttest test tl unotest utl vcl ("test" -> test_fixtures, this
# port's established rename), use_sdk_api, use_ure + use_vcl -> 3 protector
# pairs, use_rdb(services) -> DEPENDS program_services_rdb (no COMPONENTS --
# cppunittester's UNO bootstrap falls back to unorc's own ini default for
# UNO_SERVICES, same as every other use_rdb(services)-only test in this
# port), use_custom_headers(officecfg/registry) -> OFFICECFG_INCLUDE_DIR +
# DEPENDS officecfg_headers, use_configuration -> CONFIGURATION.
librestudio_add_cppunit_test(filter_pdf
    SOURCES
        filter/qa/pdf
    LIBRARIES
        basegfx comphelper cppu cppuhelper sal subsequenttest test_fixtures
        tl unotest utl vcl Boost::boost LibXml2::LibXml2
    INCLUDE
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${LIBXML2_INCLUDE_DIRS}"
    DEPENDS
        officecfg_headers udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- filter_svg (CppunitTest_filter_svg.mk) -----------------------------------
#
# Re-checked directly: 1 source (filter/qa/unit/svg), use_externals(
# boost_headers, libxml2), use_libraries comphelper cppu cppuhelper sal
# subsequenttest test unotest utl tl (no "vcl" in this .mk's own list, unlike
# filter_pdf above -- re-verified, not an oversight), use_sdk_api, use_ure +
# use_vcl -> 3 protector pairs (use_vcl is still called for the protector/
# build-dependency side effect even though vcl itself is not linked -- same
# distinction cmake/LibreStudioTest.cmake's own header comment already
# documents for other tests), use_rdb(services) -> DEPENDS program_services_
# rdb, use_custom_headers(officecfg/registry) -> OFFICECFG_INCLUDE_DIR +
# DEPENDS officecfg_headers, use_configuration -> CONFIGURATION.
librestudio_add_cppunit_test(filter_svg
    SOURCES
        filter/qa/unit/svg
    LIBRARIES
        comphelper cppu cppuhelper sal subsequenttest test_fixtures unotest
        utl tl Boost::boost LibXml2::LibXml2
    INCLUDE
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${LIBXML2_INCLUDE_DIRS}"
    DEPENDS
        officecfg_headers udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- filter_dialogs_test (CppunitTest_filter_dialogs_test.mk) ----------------
#
# gb_CppunitTest_CppunitScreenShot -- the same *_dialogs_test screenshot-test
# family as vcl_dialogs_test/svtools_dialogs_test/svx_dialogs_test/
# basctl_dialogs_test/reportdesign_dialogs_test/cui_dialogs_test(_2/_3/_4).
# Re-checked directly: 1 source (filter/qa/unit/filter-dialogs-test),
# use_sdk_api, set_include(-Ifilter/inc), use_libraries basegfx comphelper
# cppu cppuhelper drawinglayer editeng i18nlangtag i18nutil msfilter oox sal
# salhelper sax sfx sot svl svt test tl tk ucbhelper unotest utl vcl xo
# ("test" -> test_fixtures, "sfx" -> sfx2's own library name, "tk" ->
# toolkit, "xo" -> xmloff -- all re-verified as real add_library() targets),
# use_external(boost_headers), use_ure + use_vcl_non_headless_with_windows ->
# 3 protector pairs + NONHEADLESS (same equivalence cmake/LibreStudioTest.cmake's
# own header comment already establishes for every other _with_windows call
# site in this port), use_rdb(services) -> DEPENDS program_services_rdb,
# use_configuration -> CONFIGURATION, use_uiconfigs(filter) -> UICONFIG
# filter (filter/CMakeLists.txt's own new "filter_uiconfig" ALL target,
# ported alongside this file).
librestudio_add_cppunit_test(filter_dialogs_test
    SOURCES
        filter/qa/unit/filter-dialogs-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
        i18nutil msfilter oox sal salhelper sax sfx sot svl svt test_fixtures
        tl tk ucbhelper unotest utl vcl xo Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/filter/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    NONHEADLESS
    UICONFIG filter
    CONFIGURATION)
