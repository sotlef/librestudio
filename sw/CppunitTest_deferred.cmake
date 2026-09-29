# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sw has 175 CppunitTest_*.mk files -- split out of sw/CMakeLists.txt into
# this file, scheduled via cmake_language(DEFER) from that file's own tail,
# for the same reason every other module's own CppunitTest_deferred.cmake
# already is (program_services_rdb/services_rdb only set much later in the
# root CMakeLists.txt). Runs in the root CMakeLists.txt's own directory
# scope, so paths below are CMAKE_SOURCE_DIR-relative throughout.
#
# Same structure already found for sc: most of sw's 175 tests are not 175
# independent hand-written .mk bodies -- upstream factors the bulk of them
# through 6 `define ... endef` gbuild macros (sw/common_unoapi_tests.mk,
# sw/layoutwriter_setup.mk, sw/rtfexport_setup.mk, sw/ww8export_setup.mk,
# sw/ooxmlexport_setup.mk, sw/uiwriter_setup.mk), covering 87 of the 175;
# each ported here as one reusable CMake function per gbuild macro, called
# once per name -- not 87 near-identical hand-copied blocks. The remaining
# ~88 fully individual tests are a separate, later pass (same as sc's own
# wave 3), not attempted in this file yet.
#
# Two new libraries needed by several of these macros, neither ported
# before this file: swqahelper (sw/Library_swqahelper.mk, this module's own
# qa-helper library, the same PLAINLIBS_NONE/PLAIN-naming/never-installed
# shape as sc's own scqahelper -- ported directly in sw/CMakeLists.txt,
# right before this file's own DEFER call site) is needed by all 5 of the
# non-unoapi macros below.
#
# Shared notes, to avoid repeating them for every one of the 6 macros:
#   - "test" -> test_fixtures, boost_headers -> Boost::boost, libxml2 ->
#     LibXml2::LibXml2 (this port's usual conventions).
#   - use_common_precompiled_header -> no action (no PCH mechanism for
#     CppunitTests anywhere in this port).
#   - use_more_fonts / a bare `ifneq ($(filter MORE_FONTS,$(BUILD_TYPE)),)`
#     guard around gb_CppunitTest_set_non_application_font_use -> both
#     inert (MORE_FONTS is a BUILD_TYPE filter this port never sets, same
#     precedent already established for editeng's own two tests).
#   - A bare `$(if $(filter DICTIONARIES,$(BUILD_TYPE)),...)` inside some
#     use_packages calls (layoutwriter/ooxmlexport/uiwriter) resolves to an
#     empty list -- DICTIONARIES is, likewise, a BUILD_TYPE this port never
#     sets -- so those three dictionary packages are never actually named;
#     omitted rather than reproduced as an always-false branch.
#   - The "-env:arg-env=LD_LIBRARY_PATH..." add_arguments passthrough
#     (ooxmlexport/uiwriter base templates, several individual layoutwriter/
#     rtfexport/ww8export/ooxmlexport files) is the same genuine, narrow,
#     environment-dependent gap already established for sc's own tests in
#     sc/CppunitTest_deferred.cmake -- not reproduced here either, for the
#     identical reason (no shell interposes between add_test() and the
#     test binary in this CMake port).
#   - A Windows-only `ifeq ($(OS),WNT)` branch adding a gpgmepp package
#     (ww8export/ooxmlexport4) is dead on this Linux-only port -- omitted,
#     not reproduced.
#   - use_uiconfigs' own "modules/swriter" name DOES resolve to a real
#     target here (unlike sc's still-unported "modules/scalc") --
#     swriter_uiconfig already exists (sw/CMakeLists.txt), ported earlier
#     this session.

# --- swqahelper (Library_swqahelper.mk) --------------------------------------
#
# Not a product library -- sw's own qa-helper (SwModelTestBase and the
# tiled-rendering/view-callback test harness), needed by every one of the 5
# non-unoapi macros below via plain LIBRARIES linking (none of them use
# gb_CppunitTest_use_library_objects the way sc_ucalc_test did -- re-
# verified directly, none of sw's 6 shared-macro files mention it at all).
# Repository.mk registers it under gb_Helper_register_libraries(
# PLAINLIBS_NONE,...), the same "PLAIN naming, never installed" group as
# sc's own scqahelper -- so PLAIN naming, no install() call.
set(SWQAHELPER_SOURCES
    sw/qa/unit/swmodeltestbase
    sw/qa/unit/swtiledrenderingtest
    sw/qa/unit/swtestviewcallback
)
librestudio_expand_cxx(SWQAHELPER_SOURCES)
add_library(swqahelper SHARED ${SWQAHELPER_SOURCES})

target_include_directories(swqahelper PRIVATE
    "${CMAKE_SOURCE_DIR}/sw/inc"
    "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
    "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
    "${CMAKE_SOURCE_DIR}/sw/qa/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(swqahelper
    officecfg_headers
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers)

target_compile_definitions(swqahelper PRIVATE SWQAHELPER_DLLIMPLEMENTATION)

# PkgConfig::CPPUNIT -- same directory-scope-visibility guard as sc/
# CMakeLists.txt's own scqahelper (see that file's own comment on this
# exact point): librestudio_add_cppunit_test() only creates this target
# lazily, from a deferred file running in the root scope, long after sw's
# own (non-deferred) processing point.
if (NOT TARGET PkgConfig::CPPUNIT)
    pkg_check_modules(CPPUNIT REQUIRED IMPORTED_TARGET cppunit)
endif ()

target_link_libraries(swqahelper PRIVATE
    comphelper cppu cppuhelper editeng for sal sw sfx sot subsequenttest
    svl svt svx svxcore test_fixtures tl unotest utl ucbhelper vcl
    Boost::boost LibXml2::LibXml2 PkgConfig::CPPUNIT)

librestudio_set_library_output_name(swqahelper swqahelper PLAIN)

# librestudio_add_sw_unoapi_test(<name>)
#
# sw/common_unoapi_tests.mk's own `define sw_unoapi_common`, re-checked
# directly: gb_CppunitTest_CppunitTest(sw_apitests_$(1)) (note the name
# prefix -- NOT sw_$(1) the way sc_unoapi_common names its tests),
# use_externals(boost_headers libxml2), add_exception_objects(sw/qa/api/
# $(1)), use_libraries (basegfx comphelper cppu cppuhelper drawinglayer
# docmodel editeng for forui i18nlangtag msfilter oox sal salhelper sax sb
# sw sfx sot svl svt svx svxcore subsequenttest test tl tk ucbhelper
# unotest utl vbahelper[optional, SCRIPTING] vcl xo), set_include(sw/inc),
# use_api(offapi udkapi), use_ure + use_vcl -> 3 protector pairs, use_
# configuration -> CONFIGURATION, use_rdb(services) -> plain DEPENDS
# program_services_rdb, use_uiconfigs(svt svx) -> both real targets. A raw
# makefile prerequisite on AllLangPackage_get_target(autotextshare) is NOT
# reproduced -- extras/CMakeLists.txt's own header comment already
# documents AllLangPackage_autotextshare as a genuine, pre-existing,
# l10n-only gap ("no gb_WITH_LANG equivalent in this port"), unrelated to
# sw specifically.
function(librestudio_add_sw_unoapi_test name)
    librestudio_add_cppunit_test(sw_apitests_${name}
        SOURCES
            sw/qa/api/${name}
        LIBRARIES
            basegfx comphelper cppu cppuhelper drawinglayer docmodel
            editeng for forui i18nlangtag msfilter oox sal salhelper sax sb
            sw sfx sot svl svt svx svxcore subsequenttest test_fixtures tl
            tk ucbhelper unotest utl vbahelper vcl xo
            Boost::boost LibXml2::LibXml2
        INCLUDE
            "${CMAKE_SOURCE_DIR}/sw/inc"
            "${udkapi_normal_HEADERS_INCLUDE_DIR}"
            "${offapi_normal_HEADERS_INCLUDE_DIR}"
        DEPENDS
            udkapi_normal_headers offapi_normal_headers
            program_services_rdb
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
            vclbootstrapprotector vclbootstrapprotector
        UICONFIG
            svt svx
        CONFIGURATION)
endfunction()

foreach (_sw_unoapi_name IN ITEMS
    SwXAutoTextContainer SwXBodyText SwXBookmark SwXBookmarks
    SwXChapterNumbering SwXDocumentIndex SwXDocumentIndexes
    SwXDocumentIndexMark SwXDocumentSettings SwXFieldEnumeration
    SwXFootnote SwXFootnoteProperties SwXFootnotes SwXFootnoteText
    SwXFrames SwXHeadFootText SwXNumberingRules SwXParagraphEnumeration
    SwXReferenceMark SwXReferenceMarks SwXStyleFamilies SwXTableCellText
    SwXTextEmbeddedObjects SwXTextField SwXTextFieldMasters
    SwXTextFieldTypes SwXTextFrame SwXTextSections SwXTextTable
    SwXTextTables)
    librestudio_add_sw_unoapi_test("${_sw_unoapi_name}")
endforeach ()
unset(_sw_unoapi_name)

# librestudio_add_sw_layoutwriter_test(<suffix>)
#
# sw/layoutwriter_setup.mk's own `define sw_layoutwriter_test`: msword
# linked only for build-order (upstream's own comment: "make sw.check"
# would not otherwise see the dependency through services.rdb) -- kept in
# LIBRARIES the same way upstream links it, a real, harmless, already-
# ported target. use_custom_headers(officecfg/registry) -> OFFICECFG_
# INCLUDE_DIR + officecfg_headers. use_uiconfigs(modules/swriter sfx svt)
# -- all 3 real. A raw prerequisite on i18npool (a library, not a
# component) -> DEPENDS i18npool (real target, already built everywhere
# else in this tree).
function(librestudio_add_sw_layoutwriter_test suffix)
    librestudio_add_cppunit_test(sw_layoutwriter${suffix}
        SOURCES
            sw/qa/extras/layout/layout${suffix}
        LIBRARIES
            comphelper cppu cppuhelper docmodel editeng msword sal sfx
            subsequenttest svl svt svxcore sw swqahelper test_fixtures
            unotest vcl tl utl i18nlangtag
            Boost::boost LibXml2::LibXml2
        INCLUDE
            "${CMAKE_SOURCE_DIR}/sw/inc"
            "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
            "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
            "${CMAKE_SOURCE_DIR}/sw/qa/inc"
            "${OFFICECFG_INCLUDE_DIR}"
            "${udkapi_normal_HEADERS_INCLUDE_DIR}"
            "${offapi_normal_HEADERS_INCLUDE_DIR}"
            "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
        DEPENDS
            udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
            officecfg_headers i18npool
            program_services_rdb
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
            vclbootstrapprotector vclbootstrapprotector
        UICONFIG
            swriter sfx svt
        CONFIGURATION)
endfunction()

foreach (_sw_lw_suffix IN ITEMS "" 2 3 4 5 6)
    librestudio_add_sw_layoutwriter_test("${_sw_lw_suffix}")
endforeach ()
unset(_sw_lw_suffix)

# librestudio_add_sw_rtfexport_test(<suffix>)
#
# sw/rtfexport_setup.mk's own `define sw_rtfexport_test`: simpler shape,
# no custom_headers/uiconfigs/packages at the template level (rtfexport3/8
# add their own below).
function(librestudio_add_sw_rtfexport_test suffix)
    librestudio_add_cppunit_test(sw_rtfexport${suffix}
        SOURCES
            sw/qa/extras/rtfexport/rtfexport${suffix}
        LIBRARIES
            comphelper cppu cppuhelper docmodel editeng i18nlangtag sal sfx
            subsequenttest svl sw swqahelper test_fixtures unotest utl vcl
            tl basegfx
            Boost::boost LibXml2::LibXml2
        INCLUDE
            "${CMAKE_SOURCE_DIR}/sw/inc"
            "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
            "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
            "${CMAKE_SOURCE_DIR}/sw/qa/inc"
            "${udkapi_normal_HEADERS_INCLUDE_DIR}"
            "${offapi_normal_HEADERS_INCLUDE_DIR}"
            "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
        DEPENDS
            udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
            program_services_rdb
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
            vclbootstrapprotector vclbootstrapprotector
        CONFIGURATION)
endfunction()

foreach (_sw_rtf_suffix IN ITEMS "" 2 4 5 6 7)
    librestudio_add_sw_rtfexport_test("${_sw_rtf_suffix}")
endforeach ()
unset(_sw_rtf_suffix)

# --- sw_rtfexport3: base macro + use_uiconfigs(svx) -------------------------
librestudio_add_cppunit_test(sw_rtfexport3
    SOURCES
        sw/qa/extras/rtfexport/rtfexport3
    LIBRARIES
        comphelper cppu cppuhelper docmodel editeng i18nlangtag sal sfx
        subsequenttest svl sw swqahelper test_fixtures unotest utl vcl tl
        basegfx
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sw/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG
        svx
    CONFIGURATION)

# --- sw_rtfexport8: base macro + use_custom_headers(officecfg/registry) ----
librestudio_add_cppunit_test(sw_rtfexport8
    SOURCES
        sw/qa/extras/rtfexport/rtfexport8
    LIBRARIES
        comphelper cppu cppuhelper docmodel editeng i18nlangtag sal sfx
        subsequenttest svl sw swqahelper test_fixtures unotest utl vcl tl
        basegfx
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sw/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# librestudio_add_sw_ww8export_test(<suffix>)
#
# sw/ww8export_setup.mk's own `define sw_ww8export_test`: adds emboleobj
# (real, already-ported target), Windows-only gpgmepp branch dead here,
# use_packages(oox_customshapes) -> real target.
function(librestudio_add_sw_ww8export_test suffix)
    librestudio_add_cppunit_test(sw_ww8export${suffix}
        SOURCES
            sw/qa/extras/ww8export/ww8export${suffix}
        LIBRARIES
            comphelper cppu cppuhelper docmodel editeng emboleobj sal sfx
            test_fixtures unotest utl subsequenttest svl svxcore sw
            swqahelper tl vcl
            Boost::boost LibXml2::LibXml2
        INCLUDE
            "${CMAKE_SOURCE_DIR}/sw/inc"
            "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
            "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
            "${CMAKE_SOURCE_DIR}/sw/qa/inc"
            "${udkapi_normal_HEADERS_INCLUDE_DIR}"
            "${offapi_normal_HEADERS_INCLUDE_DIR}"
            "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
        DEPENDS
            udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
            program_services_rdb
            oox_customshapes
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
            vclbootstrapprotector vclbootstrapprotector
        PACKAGES
            oox_customshapes
        CONFIGURATION)
endfunction()

librestudio_add_sw_ww8export_test(2)
librestudio_add_sw_ww8export_test(3)

# --- sw_ww8export4: base macro (MORE_FONTS-gated set_non_application_font_
# use is inert, see this file's own header comment) -- no other change.
librestudio_add_sw_ww8export_test(4)

# --- sw_ww8export (base): base macro + use_uiconfigs(modules/swriter sfx
# svt) -- all 3 real.
librestudio_add_cppunit_test(sw_ww8export
    SOURCES
        sw/qa/extras/ww8export/ww8export
    LIBRARIES
        comphelper cppu cppuhelper docmodel editeng emboleobj sal sfx
        test_fixtures unotest utl subsequenttest svl svxcore sw swqahelper
        tl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sw/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
        oox_customshapes
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    PACKAGES
        oox_customshapes
    UICONFIG
        swriter sfx svt
    CONFIGURATION)

# librestudio_add_sw_ooxmlexport_test(<suffix> [CUSTOM_HEADERS])
#
# sw/ooxmlexport_setup.mk's own `define sw_ooxmlexport_test`: use_
# uiconfigs(modules/swriter sfx svt) in the base template (all real),
# use_packages(oox_customshapes oox_generated) (both real), a raw
# prerequisite on $(call gb_Library_get_target,iti) -- NOT reproduced: "iti"
# names no Library_iti.mk anywhere in this whole tree, not even upstream
# (grepped project-wide, confirmed absent) -- a stale reference in
# upstream's own source to a library that no longer exists, not something
# to invent a CMake equivalent for. CUSTOM_HEADERS is an extra boolean this
# wrapper adds (not upstream's own parameter) for the dozen sw_ooxmlexportN
# files that separately call gb_CppunitTest_use_custom_headers(officecfg/
# registry) on top of the base template -- kept as an explicit opt-in
# argument here rather than silently always adding it, to stay a faithful
# per-file transcription rather than a guess.
function(librestudio_add_sw_ooxmlexport_test suffix)
    cmake_parse_arguments(_swoox "CUSTOM_HEADERS" "" "" ${ARGN})
    set(_swoox_include
        "${CMAKE_SOURCE_DIR}/sw/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}")
    set(_swoox_depends
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb oox_customshapes oox_generated)
    if (_swoox_CUSTOM_HEADERS)
        list(APPEND _swoox_include "${OFFICECFG_INCLUDE_DIR}")
        list(APPEND _swoox_depends officecfg_headers)
    endif ()
    librestudio_add_cppunit_test(sw_ooxmlexport${suffix}
        SOURCES
            sw/qa/extras/ooxmlexport/ooxmlexport${suffix}
        LIBRARIES
            comphelper cppu cppuhelper docmodel editeng sal sfx
            subsequenttest svl sw swqahelper test_fixtures tl unotest utl
            vcl svxcore basegfx
            Boost::boost LibXml2::LibXml2
        INCLUDE
            ${_swoox_include}
        DEPENDS
            ${_swoox_depends}
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
            vclbootstrapprotector vclbootstrapprotector
        PACKAGES
            oox_customshapes oox_generated
        UICONFIG
            swriter sfx svt
        CONFIGURATION)
endfunction()

librestudio_add_sw_ooxmlexport_test("")
librestudio_add_sw_ooxmlexport_test(2)
librestudio_add_sw_ooxmlexport_test(3)
librestudio_add_sw_ooxmlexport_test(4 CUSTOM_HEADERS)
librestudio_add_sw_ooxmlexport_test(6)
librestudio_add_sw_ooxmlexport_test(7)
librestudio_add_sw_ooxmlexport_test(9)
librestudio_add_sw_ooxmlexport_test(10 CUSTOM_HEADERS)
librestudio_add_sw_ooxmlexport_test(11)
librestudio_add_sw_ooxmlexport_test(12)
librestudio_add_sw_ooxmlexport_test(13)
librestudio_add_sw_ooxmlexport_test(14 CUSTOM_HEADERS)
librestudio_add_sw_ooxmlexport_test(15)
librestudio_add_sw_ooxmlexport_test(16 CUSTOM_HEADERS)
librestudio_add_sw_ooxmlexport_test(17 CUSTOM_HEADERS)
librestudio_add_sw_ooxmlexport_test(18 CUSTOM_HEADERS)
librestudio_add_sw_ooxmlexport_test(19 CUSTOM_HEADERS)
librestudio_add_sw_ooxmlexport_test(20)
librestudio_add_sw_ooxmlexport_test(21 CUSTOM_HEADERS)
librestudio_add_sw_ooxmlexport_test(22 CUSTOM_HEADERS)
librestudio_add_sw_ooxmlexport_test(23 CUSTOM_HEADERS)
librestudio_add_sw_ooxmlexport_test(24 CUSTOM_HEADERS)
librestudio_add_sw_ooxmlexport_test(25 CUSTOM_HEADERS)
librestudio_add_sw_ooxmlexport_test(26)
librestudio_add_sw_ooxmlexport_test(27 CUSTOM_HEADERS)
librestudio_add_sw_ooxmlexport_test(_template)

# --- sw_ooxmlexport5: base macro + use_uiconfigs(svt svx) on top of the
# base template's own (modules/swriter sfx svt) -- svt duplicated harmlessly.
librestudio_add_cppunit_test(sw_ooxmlexport5
    SOURCES
        sw/qa/extras/ooxmlexport/ooxmlexport5
    LIBRARIES
        comphelper cppu cppuhelper docmodel editeng sal sfx subsequenttest
        svl sw swqahelper test_fixtures tl unotest utl vcl svxcore basegfx
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sw/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb oox_customshapes oox_generated
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    PACKAGES
        oox_customshapes oox_generated
    UICONFIG
        swriter sfx svt svx
    CONFIGURATION)

# --- sw_ooxmlexport_de_locale: base macro + LO_TEST_LOCALE=de env var -------
# Raw makefile prerequisite on AllLangMoTarget_get_target(sw) (l10n .mo
# catalog) NOT reproduced -- same genuine, pre-existing, l10n-only gap as
# every other AllLangMoTarget_*/AllLangPackage_* reference in this port
# (no gb_WITH_LANG equivalent anywhere). The `export LO_TEST_LOCALE=de`
# line IS reproduced, though -- a real environment variable the test reads
# at run time, set via plain CTest ENVIRONMENT test property right after
# registration (no new librestudio_add_cppunit_test() parameter needed).
librestudio_add_sw_ooxmlexport_test(_de_locale)
set_tests_properties(sw_ooxmlexport_de_locale PROPERTIES ENVIRONMENT "LO_TEST_LOCALE=de")

# librestudio_add_sw_uiwriter_test(<suffix>)
#
# sw/uiwriter_setup.mk's own `define sw_uiwriter_test`: use_custom_headers
# (officecfg/registry) is already part of the BASE template here (unlike
# ooxmlexport, where it is per-file) -- every uiwriter test gets it.
# use_uiconfigs(modules/swriter sfx svt svx) (all real), use_packages(
# oox_customshapes sfx2_classification) -- "sfx2_classification" resolves
# to the real target "sfx2_classification_package" (sfx2/CMakeLists.txt's
# own established renaming to avoid colliding with the CppunitTest of the
# same base name -- see sfx2/CppunitTest_deferred.cmake's own comment on
# this exact point), a raw prerequisite on i18npool (DEPENDS, real target).
function(librestudio_add_sw_uiwriter_test suffix)
    librestudio_add_cppunit_test(sw_uiwriter${suffix}
        SOURCES
            sw/qa/extras/uiwriter/uiwriter${suffix}
        LIBRARIES
            comphelper cppu cppuhelper docmodel editeng i18nlangtag msword
            sal sfx subsequenttest svl svt svxcore sw swqahelper
            test_fixtures unotest vcl tl utl svx
            Boost::boost LibXml2::LibXml2
        INCLUDE
            "${CMAKE_SOURCE_DIR}/sw/inc"
            "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
            "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
            "${CMAKE_SOURCE_DIR}/sw/source/filter/inc"
            "${CMAKE_SOURCE_DIR}/sw/source/filter/html"
            "${CMAKE_SOURCE_DIR}/sw/qa/inc"
            "${OFFICECFG_INCLUDE_DIR}"
            "${udkapi_normal_HEADERS_INCLUDE_DIR}"
            "${offapi_normal_HEADERS_INCLUDE_DIR}"
            "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
        DEPENDS
            udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
            officecfg_headers i18npool
            program_services_rdb
            oox_customshapes sfx2_classification_package
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
            vclbootstrapprotector vclbootstrapprotector
        PACKAGES
            oox_customshapes sfx2_classification_package
        UICONFIG
            swriter sfx svt svx
        CONFIGURATION)
endfunction()

librestudio_add_sw_uiwriter_test("")
librestudio_add_sw_uiwriter_test(2)
librestudio_add_sw_uiwriter_test(3)
librestudio_add_sw_uiwriter_test(5)
librestudio_add_sw_uiwriter_test(7)
librestudio_add_sw_uiwriter_test(8)
librestudio_add_sw_uiwriter_test(9)
librestudio_add_sw_uiwriter_test(10)
librestudio_add_sw_uiwriter_test(11)

# --- sw_uiwriter4 / sw_uiwriter6: base macro + a raw prerequisite on
# extras_autocorr (real, already-ported target -- extras/CMakeLists.txt's
# own add_custom_target(extras_autocorr ALL ...)).
librestudio_add_sw_uiwriter_test(4)
add_dependencies(sw_uiwriter4 extras_autocorr)
librestudio_add_sw_uiwriter_test(6)
add_dependencies(sw_uiwriter6 extras_autocorr)

# =============================================================================
# Wave 2: the remaining 88 sw tests with no shared gbuild macro at all
# =============================================================================
#
# Each read in full, individually. Shared notes (not repeated per test):
#   - Same "test" -> test_fixtures / boost_headers -> Boost::boost / libxml2
#     -> LibXml2::LibXml2 / no-PCH / MORE_FONTS-inert / DICTIONARIES-empty /
#     LD_LIBRARY_PATH-passthrough-gap / dead-WNT-branch conventions as wave 1
#     (see that section's own header comment).
#   - use_custom_headers(officecfg/registry) -> INCLUDE "${OFFICECFG_
#     INCLUDE_DIR}" + DEPENDS officecfg_headers, wherever a test's own .mk
#     actually calls it.
#   - use_uiconfigs(modules/swriter ...) -> UICONFIG swriter ... (swriter_
#     uiconfig is real, ported earlier this session), plus svt/svx/sfx/cui
#     where named (all real).
#   - use_system_win32_libs / use_system_darwin_frameworks / add_cxxflags
#     inside `ifeq ($(OS),WNT)`/`ifeq ($(OS),MACOSX)` guards are dead on
#     this Linux-only port -- omitted, not reproduced.
#   - Two tests (sw_a11y, sw_uibase_uiview) call use_instdir_configuration +
#     use_common_configuration (2 of the 3 layers plain CONFIGURATION
#     bundles) instead of the full composite -- same harmless-superset
#     resolution already used for sc_a11y in sc/CppunitTest_deferred.cmake.
#   - sw_a11y's own use_packages(postprocess_images) -- 2026-09-28: now
#     built (postprocess/CMakeLists.txt), sw_a11y's own PACKAGES below now
#     includes it.
#   - Real, narrow packaging gaps found while reading these 88 (none have a
#     CMake port anywhere in this tree, confirmed by grep): filter_docbook
#     (sw_docbookexport), extras_tplwizdesktop (sw_filters_test),
#     filter_xhtml/filter_xslt (sw_xhtmlexport, sw_xsltimport -- the same
#     gap already flagged for sc's own filter_html/filter_xml/
#     html_export_test). Each omits only the unresolvable PACKAGES name(s),
#     keeping any others real.
#   - Real, narrow library/component gaps: the accessibility bridge module
#     ("acc", a raw prerequisite of sw_accessible_relation_set) does not
#     exist anywhere in this tree (no accessibility/ module directory at
#     all) -- omitted from DEPENDS, documented rather than invented.

# --- sw_core_* (15 tests, no shared macro but a very consistent hand-copied
#     shape across all of them: plain LIBRARIES linking (no use_library_
#     objects), use_custom_headers(officecfg/registry), use_uiconfigs(
#     modules/swriter [+svt/svx/sfx where named]), use_more_fonts (inert),
#     use_rdb(services), use_configuration) ---------------------------------
librestudio_add_cppunit_test(sw_core_attr
    SOURCES sw/qa/core/attr/attr
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest svl
        svx svxcore sw swqahelper test_fixtures unotest utl vcl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_core_crsr
    SOURCES sw/qa/core/crsr/crsr
    LIBRARIES comphelper cppu cppuhelper sal sfx subsequenttest svxcore sw
        swqahelper test_fixtures unotest utl vcl svt tl svl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_core_doc
    SOURCES sw/qa/core/doc/doc sw/qa/core/doc/number sw/qa/core/doc/DocumentRedlineManager
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest
        svxcore sw swqahelper test_fixtures unotest utl vcl svt tl svl svx
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_core_docnode
    SOURCES sw/qa/core/docnode/docnode
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest sw
        swqahelper test_fixtures unotest utl vcl svt tl svl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_core_draw
    SOURCES sw/qa/core/draw/draw
    LIBRARIES comphelper cppu cppuhelper docmodel sal sfx subsequenttest
        svxcore sw swqahelper test_fixtures unotest utl vcl svt tl svl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_core_edit
    SOURCES sw/qa/core/edit/edit
    LIBRARIES editeng comphelper cppu cppuhelper sal sfx subsequenttest
        svxcore sw swqahelper test_fixtures unotest utl vcl svt tl svl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_core_fields
    SOURCES sw/qa/core/fields/fields
    LIBRARIES comphelper cppu cppuhelper sal sfx subsequenttest sw
        swqahelper test_fixtures unotest utl vcl svt tl svl svxcore
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter svt svx
    CONFIGURATION)

librestudio_add_cppunit_test(sw_core_frmedt
    SOURCES sw/qa/core/frmedt/frmedt
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest
        svxcore sw swqahelper test_fixtures unotest utl vcl svt tl svl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_core_header_footer
    SOURCES sw/qa/core/header_footer/HeaderFooterTest
    LIBRARIES comphelper cppu cppuhelper sal sfx subsequenttest sw
        swqahelper test_fixtures unotest utl vcl svt tl svl svxcore
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter svt svx
    CONFIGURATION)

librestudio_add_cppunit_test(sw_core_layout
    SOURCES sw/qa/core/layout/anchoredobject sw/qa/core/layout/calcmove
        sw/qa/core/layout/fly sw/qa/core/layout/flycnt sw/qa/core/layout/frmtool
        sw/qa/core/layout/ftnfrm sw/qa/core/layout/layact sw/qa/core/layout/layout
        sw/qa/core/layout/paintfrm sw/qa/core/layout/sortedobjs
        sw/qa/core/layout/tabfrm sw/qa/core/layout/trvlfrm
    LIBRARIES editeng comphelper cppu cppuhelper sal sfx subsequenttest sw
        swqahelper test_fixtures unotest utl vcl svt tl svl svxcore
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter sfx svt svx
    CONFIGURATION)

librestudio_add_cppunit_test(sw_core_objectpositioning
    SOURCES sw/qa/core/objectpositioning/objectpositioning
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest
        svxcore sw swqahelper test_fixtures unotest utl vcl svt tl svl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_core_text
    SOURCES sw/qa/core/text/EnhancedPDFExportHelper sw/qa/core/text/frmform
        sw/qa/core/text/itratr sw/qa/core/text/itrpaint sw/qa/core/text/itrform2
        sw/qa/core/text/porfld sw/qa/core/text/porlay sw/qa/core/text/porrst
        sw/qa/core/text/text sw/qa/core/text/txtfly sw/qa/core/text/widorp
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest svl
        svt sw swqahelper test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/core/text" "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter sfx svt
    CONFIGURATION)

librestudio_add_cppunit_test(sw_core_theme
    SOURCES sw/qa/core/theme/ThemeTest
    LIBRARIES comphelper cppu cppuhelper docmodel sal sfx sot subsequenttest
        sw swqahelper test_fixtures unotest utl vcl svt tl svl svxcore
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_core_tox
    SOURCES sw/qa/core/tox/tox
    LIBRARIES comphelper cppu cppuhelper sal sfx subsequenttest sw
        swqahelper test_fixtures unotest utl vcl svt tl svl svxcore
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter svt svx
    CONFIGURATION)

librestudio_add_cppunit_test(sw_core_txtnode
    SOURCES sw/qa/core/txtnode/txtnode sw/qa/core/txtnode/justify
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest
        svxcore sw swqahelper test_fixtures unotest utl vcl svt tl svl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/core/txtnode" "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_core_undo
    SOURCES sw/qa/core/undo/undo
    LIBRARIES comphelper cppu cppuhelper sal sfx subsequenttest svxcore sw
        swqahelper test_fixtures unotest utl vcl svt tl svl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_core_unocore
    SOURCES sw/qa/core/unocore/unocore sw/qa/core/unocore/unocrsrhelper
    LIBRARIES comphelper cppu cppuhelper editeng msword sal sfx
        subsequenttest svl svt svxcore sw swqahelper test_fixtures unotest
        vcl tl tk utl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        i18npool program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_core_view
    SOURCES sw/qa/core/view/view
    LIBRARIES cppu cppuhelper sal subsequenttest svl svt sw swqahelper
        test_fixtures unotest utl vcl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_core_accessibilitycheck
    SOURCES sw/qa/core/accessibilitycheck/AccessibilityCheckTest
    LIBRARIES comphelper cppu cppuhelper sal svx sfx subsequenttest sw
        swqahelper test_fixtures unotest utl tl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

# --- sw_a11y: postprocess_images now built (see wave header above), 2-layer
# configuration -> plain CONFIGURATION superset -------------------------
librestudio_add_cppunit_test(sw_a11y
    SOURCES sw/qa/extras/accessibility/basics sw/qa/extras/accessibility/dialogs
        sw/qa/extras/accessibility/unicode sw/qa/extras/accessibility/tdf155705
    LIBRARIES sal comphelper cppu cppuhelper cui subsequenttest test_fixtures
        unotest vcl
        Boost::boost
    INCLUDE "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    PACKAGES postprocess_images
    CONFIGURATION)

# --- sw_accessible_relation_set: raw prerequisite on "acc" (accessibility
# bridge) NOT reproduced -- no accessibility/ module exists anywhere in
# this tree at all, a real, standing gap, not a mapping error.
librestudio_add_cppunit_test(sw_accessible_relation_set
    SOURCES sw/qa/extras/accessibility/accessible_relation_set
    LIBRARIES comphelper cppu cppuhelper i18nlangtag sal subsequenttest sw
        test_fixtures tk tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_apiterminate
    SOURCES sw/qa/api/terminate
    LIBRARIES comphelper cppu sal subsequenttest test_fixtures unotest
    INCLUDE "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG svx
    CONFIGURATION)

librestudio_add_cppunit_test(sw_autocorrect
    SOURCES sw/qa/extras/autocorrect/autocorrect
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest svl
        svx svxcore sw swqahelper test_fixtures unotest utl vcl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_cjk
    SOURCES sw/qa/extras/cjk/cjk
    LIBRARIES comphelper editeng cppu cppuhelper sal svt sfx subsequenttest
        svl sw swqahelper test_fixtures unotest vcl tl utl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sw_dialogs_test / sw_dialogs_test_2: gb_CppunitTest_CppunitScreenShot
# only sets a "localized" flag on the target (used by upstream's own
# localized-screenshot comparison logic) -- no CMake-relevant effect (this
# port has no gb_WITH_LANG/localization infra at all); modelled identically
# to plain gb_CppunitTest_CppunitTest. sw_dialogs_test needs "swui" +
# use_vcl_non_headless_with_windows -> NONHEADLESS + use_packages(
# extras_palettes) (real, already-ported target). sw_dialogs_test_2 uses
# use_instdir_configuration only (1 layer) -> CONFIGURATION_INSTDIR_ONLY,
# an exact match this time (not a superset).
librestudio_add_cppunit_test(sw_dialogs_test
    SOURCES sw/qa/unit/sw-dialogs-test
    LIBRARIES basegfx comphelper cppu cppuhelper drawinglayer editeng
        i18nlangtag i18nutil msfilter oox sal salhelper sax sw swui sfx sot
        svl svt svx svxcore test_fixtures tl tk ucbhelper unotest utl vcl xo
        Boost::boost
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/source/ui/inc" "${CMAKE_SOURCE_DIR}/sw/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers program_services_rdb
        extras_palettes
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    NONHEADLESS
    PACKAGES extras_palettes
    UICONFIG swriter sfx svt svx
    CONFIGURATION)

librestudio_add_cppunit_test(sw_dialogs_test2
    SOURCES sw/qa/unit/sw-dialogs-test_2
    LIBRARIES basegfx comphelper cppu cppuhelper drawinglayer editeng
        i18nlangtag i18nutil msfilter oox sal salhelper sax sw swui sfx sot
        svl svt svx svxcore test_fixtures tl tk ucbhelper unotest utl vcl xo
        Boost::boost
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/source/ui/inc" "${CMAKE_SOURCE_DIR}/sw/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    NONHEADLESS
    CONFIGURATION_INSTDIR_ONLY)

# --- sw_docbookexport: filter_docbook package gap (not ported anywhere) --
librestudio_add_cppunit_test(sw_docbookexport
    SOURCES sw/qa/extras/docbookexport/docbookexport
    LIBRARIES comphelper cppu cppuhelper i18nlangtag sal sfx subsequenttest
        sw swqahelper test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_embedded_fonts
    SOURCES sw/qa/extras/embedded_fonts/embedded_fonts
    LIBRARIES comphelper editeng cppu cppuhelper sal svt sfx subsequenttest
        svl sw swqahelper test_fixtures unotest vcl tl utl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_filter_ascii
    SOURCES sw/qa/filter/ascii/ascii
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest svl
        svx svxcore sw swqahelper test_fixtures unotest utl vcl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_filter_html
    SOURCES sw/qa/filter/html/html
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest svl
        svx svxcore sw swqahelper test_fixtures unotest utl vcl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_filter_md
    SOURCES sw/qa/filter/md/md
    LIBRARIES cppu cppuhelper editeng sal subsequenttest svxcore sw
        swqahelper test_fixtures tl unotest utl svl sfx vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sw_filters_test: extras_tplwizdesktop package gap (not ported
# anywhere) -- omitted from DEPENDS.
librestudio_add_cppunit_test(sw_filters_test
    SOURCES sw/qa/core/filters-test
    LIBRARIES sw sfx subsequenttest svl svt sot test_fixtures tl ucbhelper
        unotest utl vcl i18nlangtag comphelper cppu cppuhelper sal
        Boost::boost
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_filter_ww8
    SOURCES sw/qa/filter/ww8/ww8
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest svl
        svx svxcore sw swqahelper test_fixtures unotest utl vcl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_filter_xml
    SOURCES sw/qa/filter/xml/xml
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest svl
        svx svxcore sw swqahelper test_fixtures unotest utl vcl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_fodfexport
    SOURCES sw/qa/extras/fodfexport/fodfexport
    LIBRARIES comphelper cppu cppuhelper sal sfx subsequenttest sw
        swqahelper test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_globalfilter
    SOURCES sw/qa/extras/globalfilter/globalfilter
    LIBRARIES comphelper cppu cppuhelper sal sfx subsequenttest svt sw
        swqahelper test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_htmlexport
    SOURCES sw/qa/extras/htmlexport/htmlexport
    LIBRARIES comphelper cppu cppuhelper editeng i18nlangtag msfilter sal
        sfx subsequenttest sot sw swqahelper svl svt test_fixtures tl
        unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_htmlexport2
    SOURCES sw/qa/extras/htmlexport/htmlexport2
    LIBRARIES comphelper cppu cppuhelper editeng i18nlangtag msfilter sal
        sfx subsequenttest sot sw swqahelper svl svt test_fixtures tl
        unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_htmlimport
    SOURCES sw/qa/extras/htmlimport/htmlimport
    LIBRARIES comphelper cppu cppuhelper editeng i18nlangtag sal sfx
        subsequenttest svl svt sw swqahelper test_fixtures tl unotest utl
        vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_indexingexport
    SOURCES sw/qa/extras/indexing/IndexingExportTest sw/qa/extras/indexing/SearchResultLocatorTest
    LIBRARIES comphelper cppu cppuhelper i18nlangtag msfilter sal sfx sot
        subsequenttest sw swqahelper svl svt test_fixtures tl unotest utl
        vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/source/filter/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_macros_test
    SOURCES sw/qa/core/macros-test
    LIBRARIES basegfx comphelper cppu cppuhelper drawinglayer docmodel
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sb sw
        sfx sot subsequenttest svl svt svx svxcore test_fixtures tl tk
        ucbhelper unotest utl vbahelper vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_mailmerge
    SOURCES sw/qa/extras/mailmerge/mailmerge
    LIBRARIES comphelper cppu cppuhelper sal sfx subsequenttest svl sw
        swqahelper test_fixtures tl unotest utl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_mailmerge2
    SOURCES sw/qa/extras/mailmerge/mailmerge2
    LIBRARIES comphelper cppu cppuhelper sal sfx subsequenttest svl sw
        swqahelper test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_odfexport
    SOURCES sw/qa/extras/odfexport/odfexport
    LIBRARIES comphelper cppu cppuhelper docmodel sal sfx subsequenttest
        svl sw swqahelper test_fixtures tl unotest utl vcl basegfx
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter svx
    CONFIGURATION)

librestudio_add_cppunit_test(sw_odfexport2
    SOURCES sw/qa/extras/odfexport/odfexport2
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest svl
        sw swqahelper test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter svx
    CONFIGURATION)

librestudio_add_cppunit_test(sw_odfexport3
    SOURCES sw/qa/extras/odfexport/odfexport3
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest svl
        sw swqahelper test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter svx
    CONFIGURATION)

librestudio_add_cppunit_test(sw_odfexport4
    SOURCES sw/qa/extras/odfexport/odfexport4
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest svl
        sw swqahelper test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter svx
    CONFIGURATION)

librestudio_add_cppunit_test(sw_odffeatures
    SOURCES sw/qa/extras/odfimport/odffeatures
    LIBRARIES comphelper cppu cppuhelper sal sw swqahelper subsequenttest
        test_fixtures unotest vcl tl utl
        # LibXml2::LibXml2 found missing via the real remote build:
        # odffeatures.cxx -> include/test/xmltesttools.hxx needs
        # <libxml/xpath.h> directly (same real gap already found and fixed
        # for svgio/svgio_read the same day, just a different consumer here).
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_odfimport
    SOURCES sw/qa/extras/odfimport/odfimport
    LIBRARIES comphelper editeng cppu cppuhelper sal svt sfx subsequenttest
        svl sw swqahelper test_fixtures unotest vcl tl utl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sw_ooxmlencryption / sw_ooxmlfieldexport / sw_ooxmllinks /
# sw_ooxml_theme_export / sw_ooxmlw14export: all reuse sw_ooxmlexport_
# libraries verbatim (same LIBRARIES set as the ooxmlexport family above).
librestudio_add_cppunit_test(sw_ooxmlencryption
    SOURCES sw/qa/extras/ooxmlexport/ooxmlencryption
    LIBRARIES comphelper cppu cppuhelper docmodel editeng sal sfx
        subsequenttest svl sw swqahelper test_fixtures tl unotest utl vcl
        svxcore basegfx
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb oox_generated
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    PACKAGES oox_generated
    CONFIGURATION)

librestudio_add_cppunit_test(sw_ooxmlexport8
    SOURCES sw/qa/extras/ooxmlexport/ooxmlexport8
    LIBRARIES basegfx comphelper cppu cppuhelper sal subsequenttest
        test_fixtures unotest utl sfx sw swqahelper tl vcl svxcore
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_ooxmlfieldexport
    SOURCES sw/qa/extras/ooxmlexport/ooxmlfieldexport
    LIBRARIES comphelper cppu cppuhelper docmodel editeng sal sfx
        subsequenttest svl sw swqahelper test_fixtures tl unotest utl vcl
        svxcore basegfx
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb oox_generated
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    PACKAGES oox_generated
    CONFIGURATION)

librestudio_add_cppunit_test(sw_ooxmlimport
    SOURCES sw/qa/extras/ooxmlimport/ooxmlimport
    LIBRARIES basegfx comphelper cppu cppuhelper docmodel sal sfx
        subsequenttest test_fixtures unotest utl sw swqahelper tl vcl
        svxcore
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_ooxmlimport2
    SOURCES sw/qa/extras/ooxmlimport/ooxmlimport2
    LIBRARIES basegfx comphelper cppu cppuhelper docmodel sal sfx
        subsequenttest test_fixtures unotest utl sw swqahelper tl vcl
        svxcore svl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_ooxmllinks
    SOURCES sw/qa/extras/ooxmlexport/ooxmllinks
    LIBRARIES comphelper cppu cppuhelper docmodel editeng sal sfx
        subsequenttest svl sw swqahelper test_fixtures tl unotest utl vcl
        svxcore basegfx
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_ooxml_theme_export
    SOURCES sw/qa/extras/ooxmlexport/ooxml_ThemeExport
    LIBRARIES comphelper cppu cppuhelper docmodel editeng sal sfx
        subsequenttest svl sw swqahelper test_fixtures tl unotest utl vcl
        svxcore basegfx
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb oox_generated
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    PACKAGES oox_generated
    CONFIGURATION)

librestudio_add_cppunit_test(sw_ooxmlw14export
    SOURCES sw/qa/extras/ooxmlexport/ooxmlw14export
    LIBRARIES comphelper cppu cppuhelper docmodel editeng sal sfx
        subsequenttest svl sw swqahelper test_fixtures tl unotest utl vcl
        svxcore basegfx
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_page_line_spacing
    SOURCES sw/qa/extras/pagelinespacing/pagelinespacing
    LIBRARIES comphelper cppu cppuhelper docmodel editeng i18nlangtag
        msword sal sfx subsequenttest svl svt svxcore sw swqahelper
        test_fixtures unotest vcl tl utl svx
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/core/text" "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

# --- sw_pdf_test: gb_CppunitTest_use_executable(sw_pdf_test,xpdfimport) --
# a real, GENUINE, already-flagged blocker predating this session (sdext/
# CMakeLists.txt's own header comment: "no xpdfimport target exists
# anywhere in this port" -- confirmed again here, the earlier "found it"
# grep hit was matching that same comment's own text, not a real
# add_executable() call). HybridPdfTest.cxx spawns xpdfimport as a
# subprocess at run time (never links it), so the test still compiles
# without it -- wired here with that one DEPENDS omitted, documented
# rather than silently dropped.
librestudio_add_cppunit_test(sw_pdf_test
    SOURCES sw/qa/extras/pdf/HybridPdfTest
    LIBRARIES comphelper cppu cppuhelper i18nlangtag msfilter sal sfx sot
        subsequenttest sw swqahelper svl svt test_fixtures tl unotest utl
        vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/source/filter/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_rtfimport
    SOURCES sw/qa/extras/rtfimport/rtfimport
    LIBRARIES comphelper cppu cppuhelper sal sfx subsequenttest i18nlangtag
        svl sw swqahelper test_fixtures unotest vcl tl utl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_rtfimport2
    SOURCES sw/qa/extras/rtfimport/rtfimport2
    LIBRARIES comphelper cppu cppuhelper sal sfx subsequenttest i18nlangtag
        svl sw swqahelper test_fixtures unotest vcl tl utl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/qa/inc" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_snap_to_grid
    SOURCES sw/qa/extras/snaptogrid/snaptogrid
    LIBRARIES comphelper cppu cppuhelper docmodel editeng i18nlangtag
        msword sal sfx subsequenttest svl svt svxcore sw swqahelper
        test_fixtures unotest vcl tl utl svx
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_tiledrendering
    SOURCES sw/qa/extras/tiledrendering/tiledrendering
    LIBRARIES comphelper cppu cppuhelper editeng docmodel sal sfx
        subsequenttest svl svt svxcore sw swqahelper test_fixtures unotest
        vcl tl utl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_tiledrendering2
    SOURCES sw/qa/extras/tiledrendering/tiledrendering2
    LIBRARIES comphelper cppu cppuhelper editeng docmodel sal sfx
        subsequenttest svl svt svxcore sw swqahelper test_fixtures unotest
        vcl tl utl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_txtencexport
    SOURCES sw/qa/extras/txtencexport/txtencexport
    LIBRARIES comphelper cppu cppuhelper i18nlangtag sal sfx subsequenttest
        sw swqahelper test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_txtexport
    SOURCES sw/qa/extras/txtexport/txtexport
    LIBRARIES comphelper cppu cppuhelper i18nlangtag sal sfx subsequenttest
        sw swqahelper test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sw_txtimport
    SOURCES sw/qa/extras/txtimport/txtimport
    LIBRARIES comphelper cppu cppuhelper i18nlangtag sal sfx subsequenttest
        sw swqahelper test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sw_uibase_* (9 tests, again a very consistent hand-copied shape) -----
librestudio_add_cppunit_test(sw_uibase_dialog
    SOURCES sw/qa/uibase/dialog/dialog
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest svl
        svx svxcore sw swqahelper test_fixtures unotest utl vcl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_uibase_dochdl
    SOURCES sw/qa/uibase/dochdl/dochdl
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest svl
        svx svxcore sw swqahelper test_fixtures unotest utl vcl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_uibase_docvw
    SOURCES sw/qa/uibase/docvw/docvw
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest svl
        svx svxcore sw swqahelper test_fixtures unotest utl vcl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_uibase_fldui
    SOURCES sw/qa/uibase/fldui/fldui
    LIBRARIES comphelper cppu cppuhelper sal sfx subsequenttest svxcore sw
        swqahelper test_fixtures unotest utl vcl svt tl svl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_uibase_frmdlg
    SOURCES sw/qa/uibase/frmdlg/frmdlg
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest svl
        svx svxcore sw swqahelper test_fixtures unotest utl vcl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_uibase_shells
    SOURCES sw/qa/uibase/shells/basesh sw/qa/uibase/shells/textfld
        sw/qa/uibase/shells/textsh sw/qa/uibase/shells/textsh1
        sw/qa/uibase/shells/shells
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest svl
        svx svxcore sw swqahelper test_fixtures unotest utl vcl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

# --- sw_uibase_uiview: 2-layer configuration (same superset resolution as
# sw_a11y above).
librestudio_add_cppunit_test(sw_uibase_uiview
    SOURCES sw/qa/uibase/uiview/uiview
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest
        svxcore sw swqahelper test_fixtures unotest utl vcl svt tl svl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

# --- sw_uibase_unit: tiny, no use_ure/use_vcl/use_rdb/use_configuration at
# all -- re-verified directly, none of the 4 are called in this .mk.
librestudio_add_cppunit_test(sw_uibase_unit
    SOURCES sw/qa/unit/uibase
    LIBRARIES sal sw tl
        Boost::boost
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers)

librestudio_add_cppunit_test(sw_uibase_uno
    SOURCES sw/qa/uibase/uno/uno
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest svl
        svx svxcore sw swqahelper test_fixtures unotest utl vcl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

librestudio_add_cppunit_test(sw_uibase_wrtsh
    SOURCES sw/qa/uibase/wrtsh/wrtsh
    LIBRARIES comphelper cppu cppuhelper editeng sal sfx subsequenttest svl
        svx svxcore sw swqahelper test_fixtures unotest utl vcl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

# --- sw_unowriter: raw prerequisite on i18npool (real, DEPENDS) ------------
librestudio_add_cppunit_test(sw_unowriter
    SOURCES sw/qa/extras/unowriter/unowriter
    LIBRARIES comphelper cppu cppuhelper editeng msword sal sfx
        subsequenttest svl svt svxcore sw swqahelper test_fixtures unotest
        vcl tl tk utl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}" "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}" "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers i18npool program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG swriter
    CONFIGURATION)

# --- sw_uwriter: gb_CppunitTest_use_library_objects(sw_uwriter,sw) --
# recompiles sw's own cached SW_SOURCES directly into the test binary (see
# sw/CMakeLists.txt's own comment on this, added alongside this call site --
# same technique already proven on editeng_core/sc_ucalc_test), so "sw" is
# dropped from LIBRARIES. AVMEDIA/DBCONNECTIVITY optionals both resolve to
# "on" in this port (avmedia and dbtools are both real, already-ported
# targets); SCRIPTING's vbahelper likewise on by default. "yrs" -> already
# an established inert external in this port (see editeng/CppunitTest_
# deferred.cmake's own header comment on this exact name). Windows-only
# use_system_win32_libs dead here. 11-name use_components list, all real
# (same mapping table already used for sc's own large COMPONENTS lists).
librestudio_add_cppunit_test(sw_uwriter
    SOURCES
        ${SW_SOURCES}
        sw/qa/core/uwriter sw/qa/core/Test-BigPtrArray
        sw/qa/core/test_ToxWhitespaceStripper sw/qa/core/test_ToxLinkProcessor
        sw/qa/core/test_ToxTextGenerator sw/qa/core/test_ToxMiscTest
        sw/qa/core/test_rect sw/qa/core/test_region
    LIBRARIES avmedia basegfx comphelper cui cppu cppuhelper dbtools
        docmodel drawinglayer drawinglayercore editeng fwk i18nlangtag
        i18npool i18nutil lng msfilter oox sal salhelper sax sb sfx sot svl
        svt svx svxcore swd test_fixtures textconversiondlgs tk tl
        ucbhelper unotest utl vbahelper vcl xmlreader xo
        Boost::boost ICU::uc ICU::i18n LibXml2::LibXml2 ${LIBRESTUDIO_MD4C_TARGET}
    # 2026-09-29, found via the real remote build: this recompiles SW_SOURCES
    # directly (not by linking "sw"), so it needs sw's own PRIVATE include
    # dirs too (target_include_directories(sw PRIVATE ...) in sw/CMakeLists.
    # txt) -- same real gap as sc_ucalc's own (see sc/CppunitTest_deferred.
    # cmake's comment on that, same day). 31+14+11+8+... real "No such file"
    # errors (wrtsh.hxx, officecfg/Office/{Common,Writer}.hxx, edtwin.hxx,
    # frozen/bits/defines.h, and others all under sw/source/uibase/inc or one
    # of the other dirs added below) confirm every one of these was actually
    # needed, not just plausible.
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/filter/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
        "${CMAKE_SOURCE_DIR}/sw/inc"
        "${SW_GENERATED_DIR}" "${sw_sdi_INCLUDE_DIR}" "${FROZEN_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEFINITIONS SW_DLLIMPLEMENTATION SWUI_DLL_NAME="libswuilo.so"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers sw_generated sw_sdi
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS comphelper configmgr fwk i18npool package2 xstor sfx ucb1
        ucpfile1 utl unoxml uui
    CONFIGURATION)

librestudio_add_cppunit_test(sw_ww8import
    SOURCES sw/qa/extras/ww8import/ww8import
    LIBRARIES comphelper cppu cppuhelper editeng sal subsequenttest
        test_fixtures unotest vcl sfx svl sw swqahelper tl utl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sw_xhtmlexport: filter_xhtml/filter_xslt package gap (same as sc's
# own sc_filter_html/filter_xml, not ported anywhere) -- omitted.
librestudio_add_cppunit_test(sw_xhtmlexport
    SOURCES sw/qa/extras/htmlexport/xhtmlexport
    LIBRARIES comphelper cppu cppuhelper i18nlangtag sal sfx subsequenttest
        sw swqahelper test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG svx
    CONFIGURATION)

# --- sw_xsltimport: filter_xslt package gap (same as above) -- omitted.
librestudio_add_cppunit_test(sw_xsltimport
    SOURCES sw/qa/extras/xsltimport/xsltimport
    LIBRARIES comphelper cppu cppuhelper i18nlangtag sal sfx subsequenttest
        sw swqahelper test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sw_writerfilter_dmapper / _filter / _filters_test / _misc / _ooxml /
# _rtftok -- 2026-09-28: sw_writerfilter itself is now built (sw/CMakeLists.
# txt), so all 6 are wired below. Each of the 5 "normal-shaped" ones (all but
# _misc) ends with the identical raw makefile prerequisite `$(call
# gb_CppunitTest_get_target,sw_writerfilter_*) : $(call gb_Library_get_
# target,sw_writerfilter)` -- a real RUNTIME dependency (writerfilter loads
# as a UNO service at test bootstrap, not linked in), reproduced via DEPENDS
# sw_writerfilter below, same as every other CppunitTest DEPENDS entry in
# this file. _filters_test and _misc additionally LINK sw_writerfilter
# directly (present in their own use_libraries), so DEPENDS is redundant
# there but kept anyway for parity/consistency with the other 4.

# --- sw_writerfilter_dmapper -----------------------------------------------
librestudio_add_cppunit_test(sw_writerfilter_dmapper
    SOURCES
        sw/qa/writerfilter/dmapper/CellColorHandler
        sw/qa/writerfilter/dmapper/DomainMapperTableHandler
        sw/qa/writerfilter/dmapper/DomainMapperTableManager
        sw/qa/writerfilter/dmapper/DomainMapper
        sw/qa/writerfilter/dmapper/DomainMapper_Impl
        sw/qa/writerfilter/dmapper/FontTable
        sw/qa/writerfilter/dmapper/GraphicImport
        sw/qa/writerfilter/dmapper/NumberingManager
        sw/qa/writerfilter/dmapper/TableManager
        sw/qa/writerfilter/dmapper/TextEffectsHandler
        sw/qa/writerfilter/dmapper/PropertyMap
        sw/qa/writerfilter/dmapper/SdtHelper
        sw/qa/writerfilter/dmapper/SettingsTable
    LIBRARIES
        basegfx comphelper cppu cppuhelper editeng oox sal subsequenttest
        test_fixtures unotest utl tl vcl svl sw swqahelper
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb sw_writerfilter
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sw_writerfilter_filter --------------------------------------------------
librestudio_add_cppunit_test(sw_writerfilter_filter
    SOURCES
        sw/qa/writerfilter/filter/WriterFilter
    LIBRARIES
        basegfx comphelper cppu cppuhelper oox sal subsequenttest test_fixtures
        unotest utl tl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb sw_writerfilter
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sw_writerfilter_filters_test --------------------------------------------
# DISABLE_CVE_TESTS's ifeq is unset in this port (same resolution used
# throughout this whole wave) -- the -DDISABLE_CVE_TESTS add_defs branch is
# skipped, not modeled. Links sw_writerfilter directly (use_libraries), plus
# a COMPONENTS entry for its own componentfile ("sw/util/sw_writerfilter" ->
# registered component name "sw_writerfilter", librestudio_add_component()
# call in sw/CMakeLists.txt) and "vcl/vcl.common" -> component name "vcl"
# (vcl/CMakeLists.txt's own librestudio_add_component(vcl COMPONENT
# vcl.common.component ...) call).
librestudio_add_cppunit_test(sw_writerfilter_filters_test
    SOURCES
        sw/qa/writerfilter/filters-test/filters-test
    LIBRARIES
        comphelper cppu cppuhelper sal test_fixtures unotest vcl sw_writerfilter
        Boost::boost
    INCLUDE
        # Found missing entirely via the real remote build: filters-test.cxx
        # -> include/unotest/bootstrapfixturebase.hxx needs the udkapi-
        # generated com/sun/star/uno/XComponentContext.hpp -- every other
        # CppunitTest in this tree lists these two explicitly, this one never
        # did. Real "fatal error: com/sun/star/uno/XComponentContext.hpp: No
        # such file or directory".
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb sw_writerfilter
    COMPONENTS
        configmgr fwk i18npool svt ucb1 ucpfile1 sw_writerfilter vcl
    CONFIGURATION)

# --- sw_writerfilter_misc ----------------------------------------------------
# The one genuinely minimal test of the 6: no use_ure/use_vcl/use_rdb/use_
# configuration/PROTECTORS at all in the real .mk (re-checked directly,
# confirmed absent, not an oversight) -- just cppu/sal/salhelper +
# sw_writerfilter linked directly, plus use_custom_headers(sw/source/
# writerfilter) -> SW_WRITERFILTER_GENERATED_DIR (same generated-headers
# root the library itself uses, sw/CMakeLists.txt).
librestudio_add_cppunit_test(sw_writerfilter_misc
    SOURCES
        sw/qa/writerfilter/misc/misc
    LIBRARIES
        sw_writerfilter cppu sal salhelper
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sw/source/writerfilter/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/writerfilter"
        "${CMAKE_SOURCE_DIR}/sw/source/writerfilter/dmapper"
        "${SW_WRITERFILTER_GENERATED_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers sw_writerfilter)

# --- sw_writerfilter_ooxml ---------------------------------------------------
librestudio_add_cppunit_test(sw_writerfilter_ooxml
    SOURCES
        sw/qa/writerfilter/ooxml/ooxml
    LIBRARIES
        basegfx comphelper cppu cppuhelper oox sal subsequenttest sw swqahelper
        svl test_fixtures unotest utl tl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb sw_writerfilter
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sw_writerfilter_rtftok ---------------------------------------------------
librestudio_add_cppunit_test(sw_writerfilter_rtftok
    SOURCES
        sw/qa/writerfilter/rtftok/rtfdispatchflag
        sw/qa/writerfilter/rtftok/rtfdispatchsymbol
        sw/qa/writerfilter/rtftok/rtfdispatchvalue
        sw/qa/writerfilter/rtftok/rtfdocumentimpl
        sw/qa/writerfilter/rtftok/rtfsdrimport
        sw/qa/writerfilter/rtftok/rtfsprm
        sw/qa/writerfilter/rtftok/rtftokenizer
    LIBRARIES
        basegfx comphelper cppu cppuhelper editeng oox sal subsequenttest svl
        sw swqahelper test_fixtures unotest utl tl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc" "${CMAKE_SOURCE_DIR}/sw/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}" "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb sw_writerfilter
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)
