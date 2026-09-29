# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# editeng_editeng (CppunitTest_editeng_editeng.mk) and editeng_core
# (CppunitTest_editeng_core.mk) -- split out of editeng/CMakeLists.txt into
# their own file, include()'d from the root CMakeLists.txt only after every
# module/target either references has already run. editeng itself is
# add_subdirectory()'d at line ~539; both tests call use_ure/use_vcl, whose
# protector plug-ins (unobootstrapprotector/unoexceptionprotector,
# vclbootstrapprotector) come from unotest/test, add_subdirectory()'d at
# lines ~697-698 -- already after editeng's own position, so neither test
# could be registered at editeng/CMakeLists.txt's own place either way. Two
# further, test-specific constraints push the binding point later still:
#   - editeng_editeng calls use_rdb(services) (gb_CppunitTest_use_rdb), which
#     needs the program_services_rdb target -- not defined until the very end
#     of the root CMakeLists.txt (after librestudio_finalize_components), well
#     after unotest/test.
#   - editeng_core calls use_components naming, among others, ucb's ucb1 and
#     ucpfile1 -- ucb is add_subdirectory()'d at line 601, itself after
#     editeng's own position, and use_configuration also needs test/
#     CMakeLists.txt's LIBRESTUDIO_TEST_UNITTEST_DIR (same ~698 position as
#     the protectors above).
# Since editeng_editeng's program_services_rdb dependency is the
# later-binding constraint of the two, and both tests live in one file with
# one include() call site (the same combined-file precedent toolkit/
# CppunitTest_deferred.cmake already establishes for toolkit_toolkit/
# toolkit_a11y), the whole include() must sit after program_services_rdb is
# defined -- see the root CMakeLists.txt's own include() call site comment
# for the exact position chosen.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not editeng's, source paths below are still fine:
# librestudio_expand_cxx (cmake/LibreStudioPlatform.cmake) resolves them
# against CMAKE_SOURCE_DIR (the absolute tree root), never
# CMAKE_CURRENT_SOURCE_DIR.
#
# yrs (use_externals(...,yrs) in both .mk files): re-verified directly
# against editeng/CMakeLists.txt's own header comment (production Library_
# editeng) -- editeng/source/editeng/{editdoc,editview,impedit*}.cxx are the
# only places reaching editeng/yrs.hxx, all under "#if ENABLE_YRS", and
# cmake/config/config_collab.h.in stubs ENABLE_YRS/ENABLE_YRS_WEAK to 0 in
# this port. So, same as the production library, nothing here actually needs
# the yrs external at this flag setting either -- no yrs-related library or
# target is added to either test below; this is the same "inert,
# ENABLE_YRS=0" note carried forward, not a silent drop.
#
# use_more_fonts (both .mk files): same treatment already established by
# toolkit/CppunitTest_deferred.cmake's own header comment for toolkit_
# toolkit/toolkit_a11y -- gated on "MORE_FONTS" being in BUILD_TYPE and a
# "more_fonts" module existing, neither of which is true here (this port's
# LIBRESTUDIO_BUILD_TYPE, vcl/CMakeLists.txt, defaults to just "DESKTOP", and
# no more_fonts module/Package_extras_font exists anywhere in this tree) --
# genuinely inert, nothing to wire.

# --- CppunitTest_editeng_editeng -------------------------------------------
#
# CppunitTest_editeng_editeng.mk, re-checked directly against the live file:
# 2 sources (editeng/qa/editeng/CharacterConversionTest, editeng/qa/editeng/
# editeng), use_libraries(basegfx comphelper cppu cppuhelper docmodel
# drawinglayercore drawinglayer editeng i18nlangtag i18nutil lng sal
# salhelper sax sot sfx svl svt test tk tl ucbhelper unotest utl vcl xo --
# "test" -> test_fixtures, this port's established rename), use_externals
# (boost_headers icuuc libxml2 yrs -- yrs inert, see file header comment
# above), set_include(editeng/inc, editeng/source/editeng, $$(INCLUDE)),
# use_sdk_api (normal udkapi+offapi), use_ure + use_vcl -> 3 protector pairs
# (unobootstrapprotector x2, unoexceptionprotector x2, vclbootstrapprotector
# x2, same precedent as sax_parser/toolkit_toolkit), use_rdb(services) with
# NO use_components in this .mk -- same shape as scaddins_analysis (see
# scaddins/CppunitTest_deferred.cmake's own header comment for the "ini
# fallback" reasoning): no COMPONENTS param, just DEPENDS
# program_services_rdb, letting cppunittester's UNO bootstrap fall back to
# unorc's own ini default for UNO_SERVICES. use_configuration -> CONFIGURATION.
# use_more_fonts -> inert, see file header comment.
#
# All libraries verified as real targets (grep -n "add_library(<name> "):
# basegfx, comphelper, cppu, cppuhelper, docmodel, drawinglayercore,
# drawinglayer, editeng (this module's own target, defined in editeng/
# CMakeLists.txt above this file's own include() point), i18nlangtag,
# i18nutil, lng (linguistic/CMakeLists.txt), sal, salhelper, sax, sot, sfx
# (sfx2/CMakeLists.txt), svl, svt (svtools/CMakeLists.txt), test_fixtures
# (test/CMakeLists.txt), tk (toolkit/CMakeLists.txt), tl, ucbhelper, unotest,
# utl (unotools/CMakeLists.txt), vcl, xo (xmloff/CMakeLists.txt) -- all
# resolve. unobootstrapprotector/unoexceptionprotector (unotest/CMakeLists.txt)
# and vclbootstrapprotector (test/CMakeLists.txt) resolve too.
librestudio_add_cppunit_test(editeng_editeng
    SOURCES
        editeng/qa/editeng/CharacterConversionTest
        editeng/qa/editeng/editeng
    LIBRARIES
        basegfx comphelper cppu cppuhelper docmodel drawinglayercore
        drawinglayer editeng i18nlangtag i18nutil lng sal salhelper sax sot
        sfx svl svt test_fixtures tk tl ucbhelper unotest utl vcl xo
        Boost::boost ICU::uc LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/editeng/inc"
        "${CMAKE_SOURCE_DIR}/editeng/source/editeng"
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

# --- CppunitTest_editeng_core -----------------------------------------------
#
# CppunitTest_editeng_core.mk, re-checked directly against the live file: 7
# sources (editeng/qa/unit/core-test, ESelectionTest, EPaMTest, EditLineTest,
# EditLineListTest, EditPaMTest, EditSelectionTest), use_library_objects(
# editeng_core,editeng) -- NOT use_libraries(editeng) -- plus a full
# use_libraries list (basegfx comphelper cppu cppuhelper docmodel
# drawinglayercore drawinglayer i18nlangtag i18nutil lng sal salhelper sax
# sot sfx svl svt test tk tl ucbhelper unotest utl vcl xo -- "test" -> test_
# fixtures), use_externals(boost_headers icuuc libxml2 yrs -- yrs inert, see
# file header comment above), set_include(editeng/inc, editeng/source/
# editeng, $$(INCLUDE)), use_sdk_api (normal udkapi+offapi), use_ure +
# use_vcl -> 3 protector pairs (same precedent as editeng_editeng above),
# use_components (10 names, resolved below), use_configuration ->
# CONFIGURATION, use_more_fonts -> inert.
#
# use_library_objects(editeng_core,editeng): CORRECTED (first real
# `cmake --build`, 2026-09-27) -- the previous wave's "plain LIBRARIES
# editeng" modelling was wrong, the same wrong conclusion svx_removewhichrange
# reached for a genuinely different library. Checking only for a *.map
# version-script and a global -fvisibility=hidden flag is not sufficient:
# individual methods can carry hidden ELF visibility one at a time via
# SAL_DLLPRIVATE on their class-body declaration, regardless of either of
# those two other mechanisms. That is exactly what happens here --
# include/editeng/editeng.hxx declares EditEngine::GetParaPortions(),
# GetEditDoc(), GetParagraphInfos(), GetLineHeight(), InsertField() (all of
# which core-test.cxx's testLineSpacing/testHyperlinkCopyPaste call) as
# SAL_DLLPRIVATE -- verified directly with `readelf -sW` on the compiled
# editeng.cxx.o: these five show up as "FUNC GLOBAL HIDDEN", not "GLOBAL
# DEFAULT" like the rest of the class's methods, so they are never in
# libeditenglo.so's dynamic symbol table and linking editeng_core against the
# real shared library genuinely fails with "undefined reference" for exactly
# these five. Recompiling editeng's own sources directly into this test
# binary (this port's established technique for use_library_objects,
# see sal/CMakeLists.txt's sal_osl_file_details or basic/
# CppunitTest_deferred.cmake's basic_scanner, added this same session) sees
# them as ordinary internal symbols within the one test binary, the same way
# gbuild's own approach (reusing editeng's already-compiled .o files
# directly, never crossing a shared-library boundary at all) does. Wired
# below using editeng/CMakeLists.txt's own EDITENG_SOURCES (CACHE INTERNAL,
# added alongside this fix so it is visible from this root-scope include()),
# the same include dirs/DLLIMPLEMENTATION define/editeng_generated dependency
# the production editeng library itself uses -- and "editeng" itself dropped
# from LIBRARIES below, since its sources are compiled in directly now, not
# linked against.
#
# COMPONENTS lookup keys, each verified against the owning module's own
# librestudio_add_component() call (grep -n "librestudio_add_component("):
#   - "configmgr/source/configmgr" -> "configmgr" (configmgr/CMakeLists.txt).
#   - "framework/util/fwk" -> "fwk" (framework/CMakeLists.txt).
#   - "i18npool/util/i18npool" -> "i18npool" (i18npool/CMakeLists.txt).
#   - "i18npool/source/search/i18nsearch" -> "i18nsearch" (i18npool/
#     CMakeLists.txt -- a second, separate librestudio_add_component() call
#     in the same module, confirmed by reading it directly).
#   - "linguistic/source/lng" -> "lng" (linguistic/CMakeLists.txt).
#   - "sfx2/util/sfx" -> "sfx" (sfx2/CMakeLists.txt).
#   - "ucb/source/core/ucb1" -> "ucb1" and "ucb/source/ucp/file/ucpfile1" ->
#     "ucpfile1" (both ucb/CMakeLists.txt).
#   - "unoxml/source/service/unoxml" -> "unoxml" (unoxml/CMakeLists.txt).
#   - "sax/source/expatwrap/expwrap" -> "sax" (sax/CMakeLists.txt's own
#     librestudio_add_component(sax ...) call, for its expwrap.component --
#     same mapping already established by sax/CppunitTest_deferred.cmake's
#     own sax_xmlimport).
# All ten resolve to real, already-registered components.
#
# Binding ordering constraint, re-verified (grep -n "add_subdirectory("):
# sax (399), i18npool (401), configmgr (492), unoxml (513), linguistic (518),
# framework (523) and sfx2 (533) are all add_subdirectory()'d BEFORE
# editeng's own position (539) -- only ucb (601) is add_subdirectory()'d
# AFTER, so ucb1/ucpfile1 alone would already force deferral past editeng's
# own natural position, on top of the use_ure/use_vcl/use_configuration
# constraints already discussed in this file's header comment.
librestudio_add_cppunit_test(editeng_core
    SOURCES
        ${EDITENG_SOURCES}
        editeng/qa/unit/core-test
        editeng/qa/unit/ESelectionTest
        editeng/qa/unit/EPaMTest
        editeng/qa/unit/EditLineTest
        editeng/qa/unit/EditLineListTest
        editeng/qa/unit/EditPaMTest
        editeng/qa/unit/EditSelectionTest
    LIBRARIES
        basegfx comphelper cppu cppuhelper docmodel drawinglayercore
        drawinglayer i18nlangtag i18nutil lng sal salhelper sax sot sfx svl
        svt test_fixtures tk tl ucbhelper unotest utl vcl xo
        Boost::boost ICU::uc LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/editeng/inc"
        "${CMAKE_SOURCE_DIR}/editeng/source/editeng"
        "${EDITENG_GENERATED_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEFINITIONS EDITENG_DLLIMPLEMENTATION
    DEPENDS
        editeng_generated
        officecfg_headers
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr fwk i18npool i18nsearch lng sfx ucb1 ucpfile1 unoxml sax
    CONFIGURATION)
