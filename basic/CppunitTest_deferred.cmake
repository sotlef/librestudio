# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# basic_scanner (CppunitTest_basic_scanner.mk) and basic_macros
# (CppunitTest_basic_macros.mk) -- split out of basic/CMakeLists.txt into
# their own file, include()'d from the root CMakeLists.txt only after
# unotest/test (unobootstrapprotector, unoexceptionprotector,
# vclbootstrapprotector, test_fixtures) have had their own add_subdirectory(),
# and -- for basic_macros specifically -- only after program_services_rdb
# exists (defined near the very end of the root CMakeLists.txt). See that
# include() call site for the exact ordering rationale, and basic/
# CMakeLists.txt's own "--- CppunitTest targets ---" comment (right after
# basic_util, the one CppunitTest simple enough to stay at its own natural
# position) for why these two specifically could not stay there. General
# mechanism precedent: toolkit/CppunitTest_deferred.cmake and sax/
# CppunitTest_deferred.cmake (each combines tests with different ordering
# needs into one file, placed at the position satisfying the latest one).
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not basic's, source paths below are still fine: librestudio_expand_
# cxx (cmake/LibreStudioPlatform.cmake) resolves a relative entry against
# CMAKE_SOURCE_DIR (the absolute tree root), never CMAKE_CURRENT_SOURCE_DIR,
# and passes an already-absolute entry through unchanged -- so it does not
# matter which directory's scope this file executes in. Directory-scoped
# CACHE INTERNAL variables (SB_SOURCES, offapi_normal_HEADERS_INCLUDE_DIR and
# friends) are visible here for the same reason OFFICECFG_INCLUDE_DIR/
# LIBRESTUDIO_TEST_UNITTEST_DIR already are in the other deferred files.

# basic_scanner: 1 source (basic/qa/cppunit/test_scanner), use_ure (no
# use_vcl) -> PROTECTORS is 2 pairs only (unobootstrapprotector x2,
# unoexceptionprotector x2 -- NOT vclbootstrapprotector, re-verified directly
# against the live .mk: it never calls gb_CppunitTest_use_vcl), use_externals
# (icu_headers icuuc icui18n) -> ICU::uc ICU::i18n (same external-name
# collapse basic/CMakeLists.txt's own sb target already applies, see its
# target_link_libraries call), use_libraries(comphelper cppu cppuhelper sal
# salhelper i18nlangtag sot svl svt tl utl vcl xmlscript), set_include adds
# basic/source/inc and basic/inc (kept as "$$(INCLUDE)" otherwise, i.e. adds
# nothing beyond those two since the .mk calls no use_api itself).
#
# gb_CppunitTest_use_library_objects(basic_scanner,sb) (solenv/gbuild/
# CppunitTest.mk -> gb_LinkTarget_use_library_objects) pulls sb's own
# already-compiled object files straight into this test's link line, instead
# of gb_CppunitTest_use_libraries(sb) (which would link against libsblo.so):
# basic/qa/cppunit/test_scanner.cxx #includes "scanner.hxx" directly (basic/
# source/inc/scanner.hxx), an internal header, not part of sb's public ABI.
# This is the exact situation sal/CMakeLists.txt already documents for
# CppunitTest_sal_osl_file_details.mk and jvmfwk/CMakeLists.txt for
# CppunitTest_jvmfwk_sunversion.mk (see either file's own comment on its
# librestudio_add_cppunit_test call): CMake has no direct equivalent of
# "relink another target's already-built .o files", so the established
# technique recompiles the *same already-expanded source list* the
# production target compiles, directly into the CppunitTest MODULE, rather
# than restructuring the working production `sb` target into an OBJECT
# library. SB_SOURCES (basic/CMakeLists.txt, cached CACHE INTERNAL
# specifically for this reuse -- see that file's own comment on the cache
# call, right after librestudio_expand_cxx(SB_SOURCES)) is reused as-is here.
#
# Because this recompiles sb's sources from scratch rather than reusing
# prebuilt objects, it needs the exact same include dirs/define/link
# libraries add_library(sb ...) itself uses (basic/CMakeLists.txt) --
# CppunitTest_basic_scanner.mk itself declares neither BASIC_DLLIMPLEMENTATION
# nor the officecfg/udkapi/offapi/oovbaapi include dirs (gbuild didn't need
# to: it reuses sb's already -- correctly -- compiled .o files), but
# recompiling the same sources here without them would not reproduce the
# production compile; same reasoning jvmfwk/CMakeLists.txt's own comment on
# jvmfwk_sunversion/JVMFWK_DLLIMPLEMENTATION already applies to that test's
# identical situation.
#
# All libraries/targets verified as real before wiring (grep -n
# "add_library(<name> "): comphelper, cppu, cppuhelper, sal, salhelper,
# i18nlangtag, sot, svl, svt (svtools/CMakeLists.txt), tl (sal/CMakeLists.txt),
# utl (unotools/CMakeLists.txt), vcl, xmlscript, sb (basic/CMakeLists.txt,
# this module's own target, for SB_SOURCES/its include dirs/define) --
# ICU::uc/ICU::i18n are the same global imported targets sb's own
# target_link_libraries call already uses. unobootstrapprotector/
# unoexceptionprotector (unotest/CMakeLists.txt) resolve too. add_subdirectory
# order re-checked directly against the root CMakeLists.txt: i18nlangtag,
# unotools (utl), sot, svl, xmlscript, vcl, svtools (svt) are all
# add_subdirectory()'d well before both basic (line 528) and unotest/test
# (lines 697-698), so none of this test's LIBRARIES needs are new ordering
# constraints beyond unotest/test themselves.
librestudio_add_cppunit_test(basic_scanner
    SOURCES
        ${SB_SOURCES}
        basic/qa/cppunit/test_scanner
    LIBRARIES
        comphelper cppu cppuhelper sal salhelper i18nlangtag sot svl svt tl
        utl vcl xmlscript
        ICU::uc ICU::i18n
    INCLUDE
        "${CMAKE_SOURCE_DIR}/basic/source/inc"
        "${CMAKE_SOURCE_DIR}/basic/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEFINITIONS BASIC_DLLIMPLEMENTATION
    DEPENDS
        officecfg_headers
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector)

# basic_macros (CppunitTest_basic_macros.mk), re-checked directly against the
# live file: 10 sources (basic/qa/cppunit/basictest, basic_coverage,
# test_append, test_compiler_checks, test_language_conditionals,
# test_nested_struct, test_vba, test_global_as_new, test_global_array,
# test_tdf149714_bits_per_pixel), use_libraries(comphelper cppu cppuhelper
# i18nlangtag sal salhelper sb sot svl svt test tl unotest utl vcl xmlscript)
# -- "test" is test_fixtures here, this port's established rename (test/
# CMakeLists.txt's own comment explains why the bare name "test" is
# unavailable: enable_testing() in the root CMakeLists.txt reserves it for
# CTest's own built-in target -- same rename e.g. sax/CppunitTest_
# deferred.cmake's sax_parser/sax_xmlimport and toolkit/CppunitTest_
# deferred.cmake's toolkit_toolkit/toolkit_a11y already apply). use_api
# (offapi udkapi oovbaapi) is the normal variant (gb_CppunitTest_use_api's
# plain form, not use_sdk_api/use_internal_api) -- no explicit set_include
# call in the .mk, so INCLUDE below is exactly those three normal-variant
# header dirs; the qa sources' own <basic/...>/<test/...>/<unotest/...>
# includes resolve via this port's global include_directories("${CMAKE_
# SOURCE_DIR}/include") (cmake/LibreStudioPlatform.cmake), which every target
# in the tree already gets for free (confirmed against include/basic/,
# include/test/, include/unotest/ all containing the headers these qa
# sources include -- no extra INCLUDE entry needed for those). use_ure +
# use_vcl -> 3 protector pairs (unobootstrapprotector x2, unoexceptionprotector
# x2, vclbootstrapprotector x2 -- same precedent as toolkit_toolkit/
# sax_parser above). use_rdb(services) (no use_components in this .mk) ->
# per this port's established precedent for that exact combination (see
# scaddins/CppunitTest_deferred.cmake's own comment on scaddins_analysis for
# the full ini-fallback reasoning): just DEPENDS program_services_rdb, no
# COMPONENTS argument and no synthesized -env:UNO_SERVICES -- cppunittester
# falls back to unorc's ini default, which already points UNO_SERVICES at
# services.rdb (cmake/LibreStudioUnorc.cmake). use_configuration ->
# CONFIGURATION.
#
# All libraries/targets verified as real before wiring (grep -n
# "add_library(<name> "): comphelper, cppu, cppuhelper, i18nlangtag, sal,
# salhelper, sb (basic/CMakeLists.txt, this module's own target -- linked
# normally here, unlike basic_scanner above, since none of these 10 qa
# sources reach into sb's internal headers the way test_scanner.cxx does),
# sot, svl, svt (svtools/CMakeLists.txt), test_fixtures (test/CMakeLists.txt),
# tl (sal/CMakeLists.txt), unotest, utl (unotools/CMakeLists.txt), vcl,
# xmlscript -- all resolve. unobootstrapprotector/unoexceptionprotector
# (unotest/CMakeLists.txt) and vclbootstrapprotector (test/CMakeLists.txt)
# resolve too, same protector targets basic_scanner/toolkit_toolkit/
# sax_parser above already use.
librestudio_add_cppunit_test(basic_macros
    SOURCES
        basic/qa/cppunit/basictest
        basic/qa/cppunit/basic_coverage
        basic/qa/cppunit/test_append
        basic/qa/cppunit/test_compiler_checks
        basic/qa/cppunit/test_language_conditionals
        basic/qa/cppunit/test_nested_struct
        basic/qa/cppunit/test_vba
        basic/qa/cppunit/test_global_as_new
        basic/qa/cppunit/test_global_array
        basic/qa/cppunit/test_tdf149714_bits_per_pixel
    LIBRARIES
        comphelper cppu cppuhelper i18nlangtag sal salhelper sb sot svl svt
        test_fixtures tl unotest utl vcl xmlscript
    INCLUDE
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        offapi_normal_headers udkapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)
