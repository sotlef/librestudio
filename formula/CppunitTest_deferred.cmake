# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# formula_dialogs_test (CppunitTest_formula_dialogs_test.mk, a
# CppunitScreenShot/"*_dialogs_test" target) and formula_ooxml
# (CppunitTest_formula_ooxml.mk) -- split out of formula/CMakeLists.txt into
# their own file, include()'d from the root CMakeLists.txt only after every
# module/target either needs has already run. formula itself is
# add_subdirectory()'d at line ~886, well before unotest/test (~1017-1018) --
# both tests call use_ure, whose protector plug-ins (unobootstrapprotector/
# unoexceptionprotector, plus vclbootstrapprotector for formula_dialogs_test's
# own use_vcl_non_headless_with_windows) come from unotest/test, so neither
# test could be registered at formula/CMakeLists.txt's own position either
# way. formula_dialogs_test additionally calls gb_CppunitTest_use_rdb(name,
# services) (see scaddins/CppunitTest_deferred.cmake's own header comment for
# the full "ini fallback" reasoning this port gives that call: no COMPONENTS,
# a DEPENDS on program_services_rdb -- not defined until the very end of the
# root CMakeLists.txt), so program_services_rdb is its binding constraint,
# same as every other NONHEADLESS+UICONFIG "*_dialogs_test" already deferred
# to this same point in this port (svx_dialogs_test/reportdesign_dialogs_
# test/basctl_dialogs_test are the closest precedent). Since both formula
# tests live in one file with one include() call site, the whole include()
# sits at the later-binding test's constraint (formula_dialogs_test's
# program_services_rdb), rather than splitting the file in two -- the same
# combined-file precedent toolkit/CppunitTest_deferred.cmake and editeng/
# CppunitTest_deferred.cmake already establish.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not formula's, source paths below are still fine: librestudio_
# expand_cxx (cmake/LibreStudioPlatform.cmake) resolves them against
# CMAKE_SOURCE_DIR (the absolute tree root), never CMAKE_CURRENT_SOURCE_DIR.
# formula_uiconfig (UICONFIG below) and the FOR/FORUI production libraries
# are all real targets by this point too, defined in formula/CMakeLists.txt
# itself, well before this file's own include() point.

# --- CppunitTest_formula_dialogs_test ----------------------------------------
#
# CppunitTest_formula_dialogs_test.mk, re-checked directly against the live
# file: 1 source (formula/qa/unit/formula-dialogs-test), use_sdk_api (normal
# udkapi+offapi, called twice -- a no-op the second time, same idiom svx_
# dialogs_test's own double use_sdk_api call already establishes), use_
# libraries (basegfx comphelper cppu cppuhelper drawinglayer editeng
# i18nlangtag i18nutil msfilter oox sal salhelper sax sfx sot svl svt test tl
# tk ucbhelper unotest utl vcl xo -- 22 names, "test" -> test_fixtures, this
# port's established rename), use_external(boost_headers), use_ure + use_vcl_
# non_headless_with_windows -> 3 protector pairs + NONHEADLESS (same precedent
# as svx_dialogs_test/reportdesign_dialogs_test/basctl_dialogs_test), use_rdb
# (services) -> DEPENDS program_services_rdb (no COMPONENTS, see file header
# above), use_configuration -> CONFIGURATION, use_uiconfigs(formula) ->
# UICONFIG formula (formula/CMakeLists.txt's own formula_uiconfig ALL target,
# ported for real this same session -- see that file's own comment on it).
#
# All libraries verified as real targets (grep -n "add_library(<name> "):
# basegfx, comphelper, cppu, cppuhelper, drawinglayer, editeng, i18nlangtag,
# i18nutil, msfilter (filter/CMakeLists.txt), oox, sal, salhelper, sax, sfx
# (sfx2/CMakeLists.txt), sot, svl, svt (svtools/CMakeLists.txt), test_fixtures
# (test/CMakeLists.txt), tl (tools/CMakeLists.txt), tk (toolkit/CMakeLists.txt),
# ucbhelper, unotest, utl (unotools/CMakeLists.txt), vcl, xo (xmloff/
# CMakeLists.txt) -- all resolve.
librestudio_add_cppunit_test(formula_dialogs_test
    SOURCES
        formula/qa/unit/formula-dialogs-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
        i18nutil msfilter oox sal salhelper sax sfx sot svl svt test_fixtures
        tl tk ucbhelper unotest utl vcl xo Boost::boost
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
    NONHEADLESS
    UICONFIG formula
    CONFIGURATION)

# formula-dialogs-test.cxx's screenshot fixture needs SRC_ROOT/
# WORKDIR_FOR_BUILD (BootstrapFixture's Directories member) -- now handled
# generically by librestudio_add_cppunit_test() itself (cmake/
# LibreStudioTest.cmake).

# --- CppunitTest_formula_ooxml -----------------------------------------------
#
# CppunitTest_formula_ooxml.mk, re-checked directly against the live file: 1
# source (formula/qa/unit/OOXMLRewriterTest), use_sdk_api (normal
# udkapi+offapi), use_ure -- NO use_vcl call anywhere in this .mk (re-checked
# directly, unlike every other test in this file/module), so only 2 protector
# pairs (unobootstrapprotector, unoexceptionprotector -- solenv/gbuild/
# CppunitTest.mk:250-259's gb_CppunitTest_use_ure adds exactly these two on
# its own; vclbootstrapprotector is only added by gb_CppunitTest__use_vcl,
# which this .mk never calls). set_include(-I$(SRCDIR)/formula/inc,
# $$(INCLUDE)) -- OOXMLRewriterTest.cxx's own #include "OOXMLRewriter.hxx"
# (unprefixed) resolves under formula/inc, confirmed by `find formula -iname
# OOXMLRewriter.hxx`. use_libraries (comphelper cppu cppuhelper for
# i18nlangtag sal svl svt tl utl vcl), use_external(boost_headers). No use_rdb,
# no use_configuration anywhere in this .mk -- re-checked directly -- so no
# program_services_rdb DEPENDS and no CONFIGURATION option here, unlike
# formula_dialogs_test above.
librestudio_add_cppunit_test(formula_ooxml
    SOURCES
        formula/qa/unit/OOXMLRewriterTest
    LIBRARIES
        comphelper cppu cppuhelper for i18nlangtag sal svl svt tl utl vcl
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/formula/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector)
