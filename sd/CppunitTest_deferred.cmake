# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sd has 35 CppunitTest_*.mk files -- split out of sd/CMakeLists.txt into
# this file, scheduled via cmake_language(DEFER) from that file's own tail,
# for the same reason every other module's own CppunitTest_deferred.cmake
# already is. Same overall structure already found for sc/sw: 2 shared
# gbuild macros (sd/import_setup.mk, sd/export_setup.mk) cover 12 of the
# 35; the remaining ~23 are individual, a separate pass in this same file
# below (not attempted until the two macro families are done).
#
# Shared notes (not repeated per test): "test" -> test_fixtures,
# boost_headers -> Boost::boost, libxml2 -> LibXml2::LibXml2, no-PCH,
# MORE_FONTS-inert, LD_LIBRARY_PATH-passthrough-gap -- all the same
# conventions already established in sc/CppunitTest_deferred.cmake's and
# sw/CppunitTest_deferred.cmake's own header comments. AVMEDIA's optional
# avmedia resolves to "on" (a real, already-ported target, same resolution
# already used for sw_uwriter). ENABLE_POPPLER is unset anywhere in this
# port (grepped project-wide, confirmed absent) -- both the $(if $(ENABLE_
# POPPLER),...) COMPONENTS entry and the `ifneq ($(ENABLE_POPPLER),)`
# xpdfimport use_executable block resolve to nothing, consistently (xpdfimport
# itself is also a confirmed, separately-flagged gap -- see sw/CppunitTest_
# deferred.cmake's own sw_pdf_test comment).

# librestudio_add_sd_import_test(<suffix> [NONHEADLESS])
#
# NONHEADLESS is an extra boolean this wrapper adds (not upstream's own
# parameter) for sd_import_tests_skia, the one file that separately calls
# gb_CppunitTest_use_vcl_non_headless (not the base template's own plain
# use_vcl) -- same underlying gb_CppunitTest__use_vcl mechanism as this
# port's own NONHEADLESS option already models (see sc_screenshots/
# sw_dialogs_test's own precedent for _with_windows; the plain, non-
# "_with_windows" variant used here sets the identical gb_CppunitTest__
# vcl_no_svp flag, so the same CMake option covers both).
#
# sd/import_setup.mk's own `define sd_import_test`, re-checked directly:
# gb_CppunitTest_CppunitTest(sd_import_tests$(1)), use_libraries (AVMEDIA's
# optional avmedia, basegfx comphelper cppu cppuhelper drawinglayer editeng
# for forui i18nlangtag msfilter oox sal salhelper sax sd sfx sot
# subsequenttest svl svt svx svxcore test tl tk ucbhelper unotest utl vcl
# xo), set_include(sd/source/ui/inc, sd/inc), use_sdk_api, use_ure +
# use_vcl -> 3 protector pairs, use_configuration -> CONFIGURATION, use_
# packages(oox_customshapes) -> real target, add_arguments' LD_LIBRARY_PATH
# passthrough -- the usual gap.
#
# use_components: 42 names in the real .mk. Originally 5 resolved to
# nothing anywhere in this tree: filter/source/odfflatxml/odfflatxml,
# filter/source/svg/svgfilter, filter/source/pdf/pdffilter, filter/source/
# xmlfilteradaptor/xmlfa (the same missing name already flagged for
# sc_filters_test), filter/source/xmlfilterdetect/xmlfd -- back when
# filter/ only ever registered msfilter/filterconfig/storagefd. 2026-09-29:
# all 5 are now real, registered components (filter/CMakeLists.txt's own
# odfflatxml/xmlfa/xmlfd wave from 2026-09-28, plus pdffilter/svgfilter
# from the remaining-7-libraries wave the same day as this comment) --
# added to COMPONENTS below. ENABLE_POPPLER's own pdfimport remains unset/
# omitted (a separate, still-open ENABLE_POPPLER gate, unrelated to this).
function(librestudio_add_sd_import_test suffix)
    cmake_parse_arguments(_sdimp "NONHEADLESS" "" "" ${ARGN})
    set(_sdimp_nonheadless_args "")
    if (_sdimp_NONHEADLESS)
        set(_sdimp_nonheadless_args NONHEADLESS)
    endif ()
    librestudio_add_cppunit_test(sd_import_tests${suffix}
        SOURCES
            sd/qa/unit/import-tests${suffix}
        LIBRARIES
            avmedia basegfx comphelper cppu cppuhelper drawinglayer editeng
            for forui i18nlangtag msfilter oox sal salhelper sax sd sfx sot
            subsequenttest svl svt svx svxcore test_fixtures tl tk
            ucbhelper unotest utl vcl xo
            Boost::boost LibXml2::LibXml2
        INCLUDE
            "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
            "${CMAKE_SOURCE_DIR}/sd/inc"
            "${udkapi_normal_HEADERS_INCLUDE_DIR}"
            "${offapi_normal_HEADERS_INCLUDE_DIR}"
        DEPENDS
            udkapi_normal_headers offapi_normal_headers
            program_services_rdb oox_customshapes
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
            vclbootstrapprotector vclbootstrapprotector
        COMPONENTS
            animcore sb chart2 comphelper configmgr dba deployment
            drawinglayer embobj emfio filterconfig storagefd frm fwk
            i18npool lng oox xstor package2 sax sd sdd sfx sot fsstorage
            svt svxcore svx svgio tk ucb1 ucpexpand1 ucpfile1 ucppkg1
            ucptdoc1 utl unordf unoxml uui vcl xmlscript xo xmlsecurity
            odfflatxml svgfilter pdffilter xmlfa xmlfd
        PACKAGES
            oox_customshapes
        ${_sdimp_nonheadless_args}
        CONFIGURATION)
endfunction()

librestudio_add_sd_import_test(2)
librestudio_add_sd_import_test(3)
librestudio_add_sd_import_test(4)
librestudio_add_sd_import_test("")
librestudio_add_sd_import_test(_skia NONHEADLESS)

# --- sd_import_tests-smartart: base macro + MORE_FONTS-gated inert only ---
librestudio_add_sd_import_test(-smartart)

# librestudio_add_sd_export_test(<suffix> [UICONFIG])
#
# sd/export_setup.mk's own `define sd_export_test`: simpler than import --
# use_rdb(services) (plain, not use_components), use_custom_headers(
# officecfg/registry), add_arguments has TWO entries: "-env:
# SVG_DISABLE_FONT_EMBEDDING=" (a real, static, empty-valued environment
# override -- reproduced for real via EXTRA_ARGS, unlike the LD_LIBRARY_PATH
# one right after it, which stays the usual unreproducible gap). UICONFIG is
# an extra boolean this wrapper adds (not upstream's own parameter) for the
# two sd_export_testsN files that separately call gb_CppunitTest_
# use_uiconfigs(modules/simpress sfx svt) on top of the base template --
# simpress_uiconfig already ported this session (sd/CMakeLists.txt's own
# UIConfig_simpress work), sfx_uiconfig/svt_uiconfig both pre-existing.
function(librestudio_add_sd_export_test suffix)
    cmake_parse_arguments(_sdexp "UICONFIG" "" "" ${ARGN})
    set(_sdexp_uiconfig_args "")
    if (_sdexp_UICONFIG)
        set(_sdexp_uiconfig_args UICONFIG simpress sfx svt)
    endif ()
    librestudio_add_cppunit_test(sd_export_tests${suffix}
        SOURCES
            sd/qa/unit/export-tests${suffix}
        LIBRARIES
            avmedia basegfx comphelper cppu cppuhelper drawinglayer
            docmodel editeng for forui i18nlangtag msfilter oox sal
            salhelper sax sd sfx sot subsequenttest svl svt svx svxcore
            test_fixtures tl tk ucbhelper unotest utl vcl xo
            Boost::boost LibXml2::LibXml2
        INCLUDE
            "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
            "${CMAKE_SOURCE_DIR}/sd/inc"
            "${OFFICECFG_INCLUDE_DIR}"
            "${udkapi_normal_HEADERS_INCLUDE_DIR}"
            "${offapi_normal_HEADERS_INCLUDE_DIR}"
        DEPENDS
            udkapi_normal_headers offapi_normal_headers
            officecfg_headers program_services_rdb
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
            vclbootstrapprotector vclbootstrapprotector
        EXTRA_ARGS
            "-env:SVG_DISABLE_FONT_EMBEDDING="
        ${_sdexp_uiconfig_args}
        CONFIGURATION)
endfunction()

librestudio_add_sd_export_test("")
librestudio_add_sd_export_test(-ooxml1)
librestudio_add_sd_export_test(2 UICONFIG)
librestudio_add_sd_export_test(-ooxml2 UICONFIG)

# --- sd_export_tests-ooxml3 / -ooxml4: base macro + raw prerequisite on
# postprocess_images -- 2026-09-28: postprocess_images is now built
# (postprocess/CMakeLists.txt), so the real makefile order-only dependency
# (`$(call gb_CppunitTest_get_target,X) :| $(call gb_Package_get_target,
# postprocess_images)`) is now reproduced via add_dependencies() below. Both
# tests were already registered via the macro call (the dependency was what
# was missing, not the registration itself).
librestudio_add_sd_export_test(-ooxml3)
librestudio_add_sd_export_test(-ooxml4)
add_dependencies(sd_export_tests-ooxml3 postprocess_images)
add_dependencies(sd_export_tests-ooxml4 postprocess_images)

# =============================================================================
# Wave 2 -- the remaining ~23 individual CppunitTests, no shared gbuild macro.
# Each read in full and transcribed directly; DISABLE_CVE_TESTS (sd_filters_
# test's own ifeq) is unset in this port, same resolution as every other
# module -- the branch is skipped, not modeled. The LD_LIBRARY_PATH-
# passthrough add_arguments gap present on most of these (this wave's own
# header comment) is not repeated per-test below.
# =============================================================================

# --- sd_a11y -------------------------------------------------------------
# CppunitTest_sd_a11y.mk: 1 source (sd/qa/unit/a11y/layout), use_libraries
# (sal cppu subsequenttest test unotest vcl), use_api(offapi udkapi),
# use_sdk_api, use_rdb(services), use_ure+use_vcl -> 3 protectors, use_
# instdir_configuration + use_common_configuration -- same 2-of-3-layer
# plain CONFIGURATION superset already used for sc_a11y/sw_a11y (their own
# comment explains why the superset is harmless here). use_packages
# (postprocess_images) is not present in this particular .mk (unlike sc_
# a11y/sw_a11y) -- nothing to omit here.
librestudio_add_cppunit_test(sd_a11y
    SOURCES
        sd/qa/unit/a11y/layout
    LIBRARIES
        sal cppu subsequenttest test_fixtures unotest vcl
        Boost::boost
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

# --- sd_activex_controls_tests -----------------------------------------------
librestudio_add_cppunit_test(sd_activex_controls_tests
    SOURCES
        sd/qa/unit/activex-controls-tests
    LIBRARIES
        avmedia basegfx comphelper cppu cppuhelper drawinglayer editeng for
        forui i18nlangtag msfilter oox sal salhelper sax sd sfx sot
        subsequenttest svl svt svx svxcore test_fixtures tl tk ucbhelper
        unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/inc"
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

# --- sd_annotation_tests ------------------------------------------------------
librestudio_add_cppunit_test(sd_annotation_tests
    SOURCES
        sd/qa/unit/AnnotationTest
    LIBRARIES
        basegfx comphelper cppu cppuhelper docmodel drawinglayer editeng for
        forui i18nlangtag sal salhelper sax sd sfx sot subsequenttest svl svt
        svx svxcore test_fixtures tl tk ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sd_dialogs_test -----------------------------------------------------
# gb_CppunitTest_CppunitScreenShot (functionally identical to plain
# CppunitTest -- see sw_dialogs_test's own precedent comment for the full
# rationale), use_vcl_non_headless_with_windows -> NONHEADLESS, use_
# uiconfigs(cui modules/sdraw modules/simpress). 2026-09-28: sdraw_uiconfig
# is now built (sd/CMakeLists.txt's own "UIConfig_sdraw" section) -- the
# previously-omitted "sdraw" is included below for real now.
librestudio_add_cppunit_test(sd_dialogs_test
    SOURCES
        sd/qa/unit/dialogs-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
        i18nutil msfilter oox sal salhelper sax sd sdui sfx sot svl svt svx
        svxcore test_fixtures tl tk ucbhelper unotest utl vcl xo
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/inc"
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
        cui sdraw simpress
    NONHEADLESS
    CONFIGURATION)

# --- sd_filter_eppt ------------------------------------------------------
librestudio_add_cppunit_test(sd_filter_eppt
    SOURCES
        sd/qa/filter/eppt/eppt
    LIBRARIES
        comphelper cppu cppuhelper docmodel sd sal subsequenttest test_fixtures
        unotest utl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sd_filters_test -------------------------------------------------------
# DISABLE_CVE_TESTS's ifeq is unset in this port (see wave header) -- the
# -DDISABLE_CVE_TESTS add_defs branch is skipped, not modeled. Needs icg
# (filter/CMakeLists.txt's own add_library(icg ...), confirmed already
# ported).
librestudio_add_cppunit_test(sd_filters_test
    SOURCES
        sd/qa/unit/filters-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
        i18nutil icg msfilter oox sal salhelper sax sd sfx sot svl svt svx
        svxcore test_fixtures tl tk ucbhelper unotest utl vcl xo
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/inc"
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

# --- sd_font_embedding_tests -----------------------------------------------
librestudio_add_cppunit_test(sd_font_embedding_tests
    SOURCES
        sd/qa/unit/FontEmbeddingTest
    LIBRARIES
        basegfx comphelper cppu cppuhelper docmodel drawinglayer editeng for
        forui i18nlangtag sal salhelper sax sd sfx sot subsequenttest svl svt
        svx svxcore test_fixtures tl tk ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sd_html_export_tests ----------------------------------------------------
librestudio_add_cppunit_test(sd_html_export_tests
    SOURCES
        sd/qa/unit/HtmlExportTest
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng for forui
        i18nlangtag msfilter oox sal salhelper sax sd sfx sot subsequenttest
        svl svt svx svxcore test_fixtures tl tk ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/inc"
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

# --- sd_layout_tests -----------------------------------------------------
librestudio_add_cppunit_test(sd_layout_tests
    SOURCES
        sd/qa/unit/layout-tests
    LIBRARIES
        avmedia basegfx comphelper cppu cppuhelper drawinglayer editeng for
        forui i18nlangtag msfilter oox sal salhelper sax sd sfx sot
        subsequenttest svl svt svx svxcore test_fixtures tl tk ucbhelper
        unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sd_lokit_search -------------------------------------------------------
# Needs the just-ported sdqahelper (sd/CMakeLists.txt), same shape as sw_
# uwriter's own swqahelper linking. use_uiconfigs(modules/simpress svx).
librestudio_add_cppunit_test(sd_lokit_search
    SOURCES
        sd/qa/unit/tiledrendering/LOKitSearchTest
    LIBRARIES
        comphelper cppu cppuhelper drawinglayer editeng sal sfx subsequenttest
        svl svt svxcore sd sdqahelper test_fixtures unotest vcl tl utl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/inc"
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/qa/inc"
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
        simpress svx
    CONFIGURATION)

# --- sd_misc_tests -----------------------------------------------------------
librestudio_add_cppunit_test(sd_misc_tests
    SOURCES
        sd/qa/unit/misc-tests
    LIBRARIES
        avmedia basegfx comphelper cppu cppuhelper drawinglayer docmodel
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sd sfx
        sot subsequenttest svl svt svx svxcore test_fixtures tl tk ucbhelper
        unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sd_pdf_import_test -------------------------------------------------------
librestudio_add_cppunit_test(sd_pdf_import_test
    SOURCES
        sd/qa/unit/SdrPdfImportTest
    LIBRARIES
        avmedia basegfx canvastools comphelper cppcanvas cppu cppuhelper
        drawinglayer editeng for forui i18nlangtag i18nutil msfilter oox sal
        salhelper sax sb sd sfx sot subsequenttest svl svt svx svxcore
        test_fixtures tl tk ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/inc"
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/source/ui/slidesorter/inc"
        "${CMAKE_SOURCE_DIR}/svx/source/inc"
        "${CMAKE_SOURCE_DIR}/svx/inc"
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

# --- sd_png_export_tests -----------------------------------------------------
librestudio_add_cppunit_test(sd_png_export_tests
    SOURCES
        sd/qa/unit/PNGExportTests
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng for forui
        i18nlangtag msfilter oox sal salhelper sax sd sfx sot subsequenttest
        svl svt svx svxcore test_fixtures tl tk ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/inc"
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

# --- sd_shape_import_export_tests ---------------------------------------------
librestudio_add_cppunit_test(sd_shape_import_export_tests
    SOURCES
        sd/qa/unit/ShapeImportExportTest
    LIBRARIES
        avmedia basegfx canvastools comphelper cppcanvas cppu cppuhelper
        drawinglayer editeng for forui i18nlangtag i18nutil msfilter oox sal
        salhelper sax sb sd sfx sot subsequenttest svl svt svx svxcore
        test_fixtures tl tk ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/inc"
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/source/ui/slidesorter/inc"
        "${CMAKE_SOURCE_DIR}/svx/source/inc"
        "${CMAKE_SOURCE_DIR}/svx/inc"
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

# --- sd_slide_section_tests ---------------------------------------------------
# Needs sdqahelper (sd/CMakeLists.txt).
librestudio_add_cppunit_test(sd_slide_section_tests
    SOURCES
        sd/qa/unit/SlideSectionTest
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer docmodel editeng for
        forui i18nlangtag msfilter oox sal salhelper sax sd sdqahelper sfx
        sot subsequenttest svl svt svx svxcore test_fixtures tl tk ucbhelper
        unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/inc"
        "${CMAKE_SOURCE_DIR}/sd/qa/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sd_svg_export_tests -----------------------------------------------------
librestudio_add_cppunit_test(sd_svg_export_tests
    SOURCES
        sd/qa/unit/SVGExportTests
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng for forui
        i18nlangtag msfilter oox sal salhelper sax sd sfx sot subsequenttest
        svl svt svx svxcore test_fixtures tl tk ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/inc"
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

# --- sd_textfitting_tests -----------------------------------------------------
librestudio_add_cppunit_test(sd_textfitting_tests
    SOURCES
        sd/qa/unit/TextFittingTest
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng for forui
        i18nlangtag msfilter oox sal salhelper sax sd sfx sot subsequenttest
        svl svt svx svxcore test_fixtures tl tk ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sd_theme_tests ------------------------------------------------------
librestudio_add_cppunit_test(sd_theme_tests
    SOURCES
        sd/qa/unit/ThemeTest
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer docmodel editeng for
        forui i18nlangtag sal salhelper sax sd sfx sot subsequenttest svl svt
        svx svxcore test_fixtures tl tk ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sd_tiledrendering -----------------------------------------------------
# Needs sdqahelper. use_externals adds "frozen" here (unlike sd_tiledrendering2
# below) -- FROZEN_INCLUDE_DIR is the same already-global root variable used
# throughout this tree (comphelper/sc/xmloff/etc.). use_uiconfigs(modules/
# simpress svx).
librestudio_add_cppunit_test(sd_tiledrendering
    SOURCES
        sd/qa/unit/tiledrendering/tiledrendering
    LIBRARIES
        comphelper cppu cppuhelper drawinglayer editeng sal sfx subsequenttest
        svl svt svxcore sd sdqahelper test_fixtures unotest vcl tl utl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/inc"
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/qa/unit"
        "${CMAKE_SOURCE_DIR}/sd/qa/inc"
        "${FROZEN_INCLUDE_DIR}"
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
        simpress svx
    CONFIGURATION)

# --- sd_tiledrendering2 ------------------------------------------------------
librestudio_add_cppunit_test(sd_tiledrendering2
    SOURCES
        sd/qa/unit/tiledrendering/tiledrendering2
    LIBRARIES
        comphelper cppu cppuhelper drawinglayer editeng sal sfx subsequenttest
        svl svt svxcore sd sdqahelper test_fixtures unotest vcl tl utl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/inc"
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/qa/unit"
        "${CMAKE_SOURCE_DIR}/sd/qa/inc"
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
        simpress svx
    CONFIGURATION)

# --- sd_ui_func ------------------------------------------------------------
librestudio_add_cppunit_test(sd_ui_func
    SOURCES
        sd/qa/ui/func/func
    LIBRARIES
        basegfx canvastools comphelper cppcanvas cppu cppuhelper docmodel
        drawinglayer editeng for forui i18nlangtag i18nutil msfilter oox sal
        salhelper sax sb sd sfx sot subsequenttest svl svt svx svxcore
        test_fixtures tl tk ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/inc"
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/source/ui/slidesorter/inc"
        "${CMAKE_SOURCE_DIR}/sd/qa/unit"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sd_uiimpress ------------------------------------------------------------
librestudio_add_cppunit_test(sd_uiimpress
    SOURCES
        sd/qa/unit/uiimpress
    LIBRARIES
        avmedia basegfx canvastools comphelper cppcanvas cppu cppuhelper
        docmodel drawinglayer editeng for forui i18nlangtag i18nutil msfilter
        oox sal salhelper sax sb sd sfx sot subsequenttest svl svt svx svxcore
        test_fixtures tl tk ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/inc"
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/source/ui/slidesorter/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sd_uimpress: gb_CppunitTest_use_library_objects(sd_uimpress,sd) --------
# Same SAL_DLLPRIVATE-visibility recompile-not-link technique already
# established for editeng_core/sc_ucalc_test/sw_uwriter -- SD_SOURCES
# (cached CACHE INTERNAL in sd/CMakeLists.txt) is spliced directly into
# SOURCES instead of linking "sd" itself (correctly absent from this test's
# own use_libraries list in the real .mk). INCLUDE/DEFINITIONS/DEPENDS below
# mirror sd/CMakeLists.txt's own private include dirs, defines and
# dependencies for the "sd" target exactly, since the exact same source set
# is being recompiled here. OS==WNT/OS==MACOSX branches (use_system_win32_
# libs, -lobjc, IOBluetooth) are out of scope for this Linux-only port,
# matching every other platform-gated branch already resolved this way in
# this tree. DBUS_GLIB's `ifneq ($(DBUS_HAVE_GLIB),)` branch is modeled on
# the live DBUS_GLIB_FOUND CMake variable (cmake/LibreStudioConfigHeaders.
# cmake), same as sd/CMakeLists.txt's own ENABLE_SDREMOTE_BLUETOOTH-gated
# DBUS_GLIB_LOCAL block. PkgConfig::DBUS/PkgConfig::AVAHI/PkgConfig::
# DBUS_GLIB_LOCAL all need their own directory-scope re-guard here (this
# deferred file runs in the ROOT directory's own scope, which cannot see
# targets created inside sd/CMakeLists.txt's own child directory scope --
# the same PkgConfig visibility pattern already documented for PkgConfig::
# CPPUNIT throughout this whole wave).
if (NOT TARGET PkgConfig::DBUS)
    pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)
endif ()
set(_sd_uimpress_avahi "")
if (ENABLE_AVAHI)
    if (NOT TARGET PkgConfig::AVAHI)
        pkg_check_modules(AVAHI REQUIRED IMPORTED_TARGET avahi-client)
    endif ()
    set(_sd_uimpress_avahi PkgConfig::AVAHI)
endif ()
set(_sd_uimpress_dbus_glib "")
if (DBUS_GLIB_FOUND)
    if (NOT TARGET PkgConfig::DBUS_GLIB_LOCAL)
        pkg_check_modules(DBUS_GLIB_LOCAL REQUIRED IMPORTED_TARGET dbus-glib-1)
    endif ()
    set(_sd_uimpress_dbus_glib PkgConfig::DBUS_GLIB_LOCAL)
endif ()
# Same real gap as sd/CMakeLists.txt's own "sd" target (see its own comment
# next to this exact -I, added the same day): sd_uimpress recompiles
# SD_SOURCES, which includes BluetoothServer.cxx whenever
# ENABLE_SDREMOTE_BLUETOOTH is on.
set(_sd_uimpress_bluez_include "")
if (ENABLE_SDREMOTE_BLUETOOTH)
    set(_sd_uimpress_bluez_include "${CMAKE_SOURCE_DIR}/external/bluez_bluetooth/inc")
endif ()
# Same real gap, same day: sd/CMakeLists.txt's own "sd" target does
# `if (ENABLE_SDREMOTE) target_compile_definitions(sd PRIVATE -DENABLE_
# SDREMOTE) if (ENABLE_SDREMOTE_BLUETOOTH) ... -DENABLE_SDREMOTE_BLUETOOTH`
# (sd/inc/sddll.hxx guards RegisterRemotes()'s own DECLARATION with #ifdef
# ENABLE_SDREMOTE; Server.cxx's real, unconditional OUT-OF-CLASS definition
# of SdDLL::RegisterRemotes() then has nothing to match without it). Missing
# here caused a real "error: no declaration matches 'void
# SdDLL::RegisterRemotes()'" on the remote build -- not a missing-file error
# like the others, but the exact same recompile-not-link root cause.
set(_sd_uimpress_sdremote_defs "")
if (ENABLE_SDREMOTE)
    list(APPEND _sd_uimpress_sdremote_defs ENABLE_SDREMOTE)
    if (ENABLE_SDREMOTE_BLUETOOTH)
        list(APPEND _sd_uimpress_sdremote_defs ENABLE_SDREMOTE_BLUETOOTH)
    endif ()
endif ()

librestudio_add_cppunit_test(sd_uimpress
    SOURCES
        ${SD_SOURCES}
        sd/qa/unit/uimpress
    LIBRARIES
        avmedia basegfx canvastools comphelper cppcanvas cppu cppuhelper
        docmodel drawinglayer drawinglayercore editeng fwk i18nlangtag
        i18nutil icg msfilter oox sal sax salhelper sb sfx sot svl svt
        svxcore svx test_fixtures textconversiondlgs tk tl ucbhelper utl vcl
        xo
        Boost::boost ICU::uc LibXml2::LibXml2 PkgConfig::DBUS
        ${_sd_uimpress_avahi} ${_sd_uimpress_dbus_glib}
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sd/inc"
        "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sd/source/ui/slidesorter/inc"
        "${sd_sdslots_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
        "${OOX_GENERATED_DIR}"
        "${FROZEN_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        ${_sd_uimpress_bluez_include}
    DEFINITIONS
        SD_DLLIMPLEMENTATION
        SDUI_DLL_NAME="libsduilo.so"
        ${_sd_uimpress_sdremote_defs}
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        sd_sdslots sd_sdgslots oox_generated
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr fwk i18npool lng ucb1 ucpfile1 unoxml
    CONFIGURATION)
unset(_sd_uimpress_avahi)
unset(_sd_uimpress_dbus_glib)
unset(_sd_uimpress_bluez_include)
unset(_sd_uimpress_sdremote_defs)
