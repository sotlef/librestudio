# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# writerperfect (module writerperfect) -- five libraries.
#
# writerperfect (Library_writerperfect.mk) is the shared filter-plumbing base:
# a librevenge-to-UNO document handler, an input-stream adapter and the
# encoding-picker dialog used by import filters that cannot sniff a document's
# text encoding on their own. It has no .component file -- it registers no UNO
# service itself, only helper classes the four libraries below link against
# (confirmed with `find writerperfect -iname "*.component"`: only wpftcalc,
# wpftdraw, wpftimpress, wpftwriter have one).
#
# The other four are per-app filter fronts, each a thin set of
# ImportFilter/ExportFilter UNO services translating one or more librevenge-
# family document formats into the app's own XML import chain:
#   - wpftcalc    (Calc):    Numbers, MS Works, various MWAW spreadsheets, and
#                             StarOffice Calc documents (libstaroffice).
#   - wpftimpress (Impress): Keynote, MWAW presentations, StarOffice Impress.
#   - wpftwriter  (Writer):  AbiWord, MS Works, MWAW word processors, Pages,
#                             WordPerfect, StarOffice Writer, EPUB export, and
#                             the exp/* odfgen-to-Writer-XML bridge shared by
#                             all of the import filters above.
#   - wpftdraw    (Draw):    CorelDRAW/CMX, Freehand, MS Publisher, MWAW draw
#                             documents, PageMaker, QuarkXPress, StarOffice
#                             Draw, Visio, WordPerfect Graphics, Zoner Metafile.
#
# Not reproduced:
#  - gb_Library_use_common_precompiled_header,wpftdraw / wpftwriter --
#    precompiled headers are not ported anywhere in this build, same as every
#    other module so far.
#  - The Windows-only Bcrypt.lib link at the bottom of Library_wpftwriter.mk
#    (needed only when linking libepubgen/boost statically on WNT) -- out of
#    scope for this Linux port.
#  - AllLangMoTarget_wpt/UITest_* -- l10n/UITest infrastructure this port has
#    no equivalent for anywhere (same as every other module). Library_
#    wpftqahelper.mk and UIConfig_writerperfect.mk, by contrast, ARE now
#    ported below (2026-09-28) -- an earlier version of this comment called
#    them "out of scope for this pass, same as every other module so far",
#    which mischaracterized a not-yet-attempted to-do as a settled scope
#    decision (see [[feedback_gbuild_full_replacement_scope]] in project
#    memory for why that framing is wrong and not to be repeated).
#  - ENABLE_WASM_STRIP_* gates (BASIC_DRAW_MATH_IMPRESS, CALC, WRITER, EPUB,
#    SWEXPORTS, SCEXPORTS) are all unset (0) in this port, so every source and
#    componentimpl they guard is included unconditionally -- same resolution
#    already established for the same class of flag elsewhere in this port.
#
# Source lists, includes, defines, use_libraries and use_externals are
# transcribed from Library_writerperfect.mk / Library_wpftcalc.mk /
# Library_wpftimpress.mk / Library_wpftwriter.mk / Library_wpftdraw.mk.
# Every library is registered in the OOO layer (Repository.mk's
# gb_Helper_register_libraries_for_install(OOOLIBS,ooo,...)).

# Externals shared by more than one of the five libraries: declared once here
# and reused (one CMakeLists.txt = one scope, so one call per external
# suffices for every target below).
#
# 2026-09-29, system-or-vendor refactor: this whole writerperfect-family
# (18 libraries in total, spread across this file) converted from bare
# `pkg_check_modules(... REQUIRED ...)` to librestudio_pkgconfig() --
# real vendoring fallbacks in external/librevenge/librevenge.cmake and its
# 17 siblings (external/libwpd, libodfgen, libwpg, libwps, libmwaw,
# libetonyek, libstaroffice, libabw, libebook, libepubgen, libcdr,
# libfreehand, libmspub, libpagemaker, libqxp, libvisio, libzmf -- see
# each driver's own header comment). librevenge is the base dependency of
# every other one in the family (resolved once here so its own
# LIBRESTUDIO_REVENGE_FOUND is available to every dependent driver by the
# time it runs); VERSION floors below match configure.ac's own libo_
# CHECK_SYSTEM_MODULE/libo_PKG_VERSION calls exactly (re-checked directly,
# not assumed equal to this port's own previously bare, unconstrained
# pkg_check_modules calls) -- on this machine, libwps's system package
# (0.4.12) is one micro version short of the 0.4.14 floor, so libwps
# actually vendors here, unlike every other library in this family.
librestudio_pkgconfig(NAME REVENGE MODULES librevenge-0.0 VERSION 0.0.1 DRIVER librevenge)
librestudio_pkgconfig(NAME ODFGEN MODULES libodfgen-0.1 DRIVER libodfgen)
librestudio_pkgconfig(NAME ETONYEK MODULES libetonyek-0.1 VERSION 0.1.10 DRIVER libetonyek)
librestudio_pkgconfig(NAME MWAW MODULES libmwaw-0.3 VERSION 0.3.21 DRIVER libmwaw)
librestudio_pkgconfig(NAME STAROFFICE MODULES libstaroffice-0.0 VERSION 0.0.7 DRIVER libstaroffice)
librestudio_pkgconfig(NAME WPS MODULES libwps-0.4 VERSION 0.4.14 DRIVER libwps)
librestudio_pkgconfig(NAME WPD MODULES libwpd-0.10 DRIVER libwpd)
librestudio_pkgconfig(NAME WPG MODULES libwpg-0.3 DRIVER libwpg)

# lcms2 (2026-09-29, system-or-vendor refactor): resolved via
# librestudio_pkgconfig() into LIBRESTUDIO_LCMS2_TARGET, same as vcl/
# CMakeLists.txt's own call -- memoized (cmake/LibreStudioTarballs.cmake's
# own GLOBAL PROPERTY cache), so whichever of the two directory scopes
# resolves it first does the real work and the other just reads the
# cached CACHE INTERNAL result straight back, same pattern zlib/libpng
# already established via librestudio_find_package() below.
librestudio_pkgconfig(NAME LCMS2 MODULES lcms2)
librestudio_find_package(NAME ZLIB)
librestudio_find_package(NAME PNG)

# ---------------------------------------------------------------------------
# writerperfect -- base library. No .component file; every target below
# links it.
# ---------------------------------------------------------------------------

set(WRITERPERFECT_SOURCES
    writerperfect/source/common/DirectoryStream
    writerperfect/source/common/DocumentHandler
    writerperfect/source/common/WPFTEncodingDialog
    writerperfect/source/common/WPXSvInputStream
)

librestudio_expand_cxx(WRITERPERFECT_SOURCES)
add_library(writerperfect SHARED ${WRITERPERFECT_SOURCES})

target_include_directories(writerperfect PRIVATE
    "${CMAKE_SOURCE_DIR}/writerperfect/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(writerperfect udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(writerperfect PRIVATE WRITERPERFECT_DLLIMPLEMENTATION)

target_link_libraries(writerperfect PRIVATE
    comphelper cppu cppuhelper sal sot svx tl ucbhelper utl vcl xo
    Boost::boost
    ${LIBRESTUDIO_ODFGEN_TARGET}
    ${LIBRESTUDIO_REVENGE_TARGET})

librestudio_set_library_output_name(writerperfect writerperfect OOO)

install(TARGETS writerperfect LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# wpftcalc -- Calc import filters.
# ---------------------------------------------------------------------------

set(WPFTCALC_SOURCES
    writerperfect/source/calc/NumbersImportFilter
    writerperfect/source/calc/MSWorksCalcImportFilter
    writerperfect/source/calc/MWAWCalcImportFilter
    writerperfect/source/calc/StarOfficeCalcImportFilter
)

librestudio_expand_cxx(WPFTCALC_SOURCES)
add_library(wpftcalc SHARED ${WPFTCALC_SOURCES})

target_include_directories(wpftcalc PRIVATE
    "${CMAKE_SOURCE_DIR}/writerperfect/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(wpftcalc udkapi_normal_headers offapi_normal_headers)

target_link_libraries(wpftcalc PRIVATE
    comphelper cppu cppuhelper sal sfx sot svx tl ucbhelper utl vcl writerperfect xo
    Boost::boost
    ICU::uc ICU::i18n
    ${LIBRESTUDIO_ETONYEK_TARGET}
    ${LIBRESTUDIO_ODFGEN_TARGET}
    ${LIBRESTUDIO_REVENGE_TARGET}
    ${LIBRESTUDIO_MWAW_TARGET}
    ${LIBRESTUDIO_STAROFFICE_TARGET}
    ${LIBRESTUDIO_WPS_TARGET}
    ${LIBRESTUDIO_LCMS2_TARGET})

# gb_Library_add_componentimpls(wpftcalc, extended) -- gated by
# ENABLE_WASM_STRIP_SCEXPORTS, unset (0) in this port, so the Numbers
# importer's implementation is included, same resolution as every other
# ENABLE_WASM_STRIP_* flag in this file.
librestudio_add_component(wpftcalc
    COMPONENT      "${CMAKE_SOURCE_DIR}/writerperfect/source/calc/wpftcalc.component"
    IMPLEMENTATION "${CMAKE_SOURCE_DIR}/writerperfect/source/calc/wpftcalc.component.extended"
    LIBRARY        wpftcalc
    LAYER          OOO)

librestudio_set_library_output_name(wpftcalc wpftcalc OOO)

install(TARGETS wpftcalc LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# wpftimpress -- Impress import filters.
# ---------------------------------------------------------------------------

set(WPFTIMPRESS_SOURCES
    writerperfect/source/impress/KeynoteImportFilter
    writerperfect/source/impress/MWAWPresentationImportFilter
    writerperfect/source/impress/StarOfficePresentationImportFilter
)

librestudio_expand_cxx(WPFTIMPRESS_SOURCES)
add_library(wpftimpress SHARED ${WPFTIMPRESS_SOURCES})

target_include_directories(wpftimpress PRIVATE
    "${CMAKE_SOURCE_DIR}/writerperfect/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(wpftimpress udkapi_normal_headers offapi_normal_headers)

target_link_libraries(wpftimpress PRIVATE
    comphelper cppu cppuhelper sal sot svx tl ucbhelper utl vcl writerperfect xo
    Boost::boost
    ${LIBRESTUDIO_ETONYEK_TARGET}
    ${LIBRESTUDIO_MWAW_TARGET}
    ${LIBRESTUDIO_ODFGEN_TARGET}
    ${LIBRESTUDIO_STAROFFICE_TARGET}
    ${LIBRESTUDIO_REVENGE_TARGET})

librestudio_add_component(wpftimpress
    COMPONENT "${CMAKE_SOURCE_DIR}/writerperfect/source/impress/wpftimpress.component"
    LIBRARY   wpftimpress
    LAYER     OOO)

librestudio_set_library_output_name(wpftimpress wpftimpress OOO)

install(TARGETS wpftimpress LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# wpftwriter -- Writer import filters, EPUB export, and the exp/* odfgen ->
# Writer-XML bridge shared by all of the import filters above.
# ---------------------------------------------------------------------------

set(WPFTWRITER_SOURCES
    # ifneq ($(ENABLE_WASM_STRIP_EPUB),TRUE) -- applies, the flag is unset.
    writerperfect/source/writer/EPUBExportDialog
    writerperfect/source/writer/EPUBExportFilter
    writerperfect/source/writer/EPUBExportUIComponent
    writerperfect/source/writer/EPUBPackage

    # ifneq ($(ENABLE_WASM_STRIP_SWEXPORTS),TRUE) -- applies, the flag is unset.
    writerperfect/source/writer/AbiWordImportFilter
    writerperfect/source/writer/EBookImportFilter
    writerperfect/source/writer/MSWorksImportFilter
    writerperfect/source/writer/MWAWImportFilter
    writerperfect/source/writer/PagesImportFilter
    writerperfect/source/writer/WordPerfectImportFilter

    writerperfect/source/writer/StarOfficeWriterImportFilter
    writerperfect/source/writer/exp/XMLBase64ImportContext
    writerperfect/source/writer/exp/XMLFootnoteImportContext
    writerperfect/source/writer/exp/XMLSectionContext
    writerperfect/source/writer/exp/XMLTextFrameContext
    writerperfect/source/writer/exp/XMLTextListContext
    writerperfect/source/writer/exp/XMLTextListItemContext
    writerperfect/source/writer/exp/txtparai
    writerperfect/source/writer/exp/txtstyli
    writerperfect/source/writer/exp/xmlfmt
    writerperfect/source/writer/exp/xmlictxt
    writerperfect/source/writer/exp/xmlimp
    writerperfect/source/writer/exp/xmlmetai
    writerperfect/source/writer/exp/xmltbli
    writerperfect/source/writer/exp/xmltext
)

librestudio_expand_cxx(WPFTWRITER_SOURCES)
add_library(wpftwriter SHARED ${WPFTWRITER_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(wpftwriter REUSE_FROM precompiled_system)
endif ()

target_include_directories(wpftwriter PRIVATE
    "${CMAKE_SOURCE_DIR}/writerperfect/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(wpftwriter officecfg_headers udkapi_normal_headers offapi_normal_headers)

librestudio_pkgconfig(NAME ABW MODULES libabw-0.1 DRIVER libabw)
librestudio_pkgconfig(NAME EBOOK MODULES libe-book-0.1 VERSION 0.1.2 DRIVER libebook)
librestudio_pkgconfig(NAME EPUBGEN MODULES libepubgen-0.1 DRIVER libepubgen)

target_link_libraries(wpftwriter PRIVATE
    comphelper cppu cppuhelper i18nlangtag sal sax sfx sot svt svx tl ucbhelper utl vcl writerperfect xo
    Boost::boost
    ICU::uc ICU::i18n
    LibXml2::LibXml2
    PkgConfig::LIBLANGTAG
    ${LIBRESTUDIO_ABW_TARGET}
    ${LIBRESTUDIO_EBOOK_TARGET}
    ${LIBRESTUDIO_EPUBGEN_TARGET}
    ${LIBRESTUDIO_ETONYEK_TARGET}
    ${LIBRESTUDIO_MWAW_TARGET}
    ${LIBRESTUDIO_ODFGEN_TARGET}
    ${LIBRESTUDIO_REVENGE_TARGET}
    ${LIBRESTUDIO_STAROFFICE_TARGET}
    ${LIBRESTUDIO_WPD_TARGET}
    ${LIBRESTUDIO_WPG_TARGET}
    ${LIBRESTUDIO_WPS_TARGET}
    ${LIBRESTUDIO_ZLIB_TARGET})

# gb_Library_add_componentimpls(wpftwriter, extended extended2) -- both
# ifneq'd on ENABLE_WASM_STRIP_EPUB / ENABLE_WASM_STRIP_SWEXPORTS, both unset
# in this port, so both apply: wpftwriter.component.extended marks the two
# EPUB export implementations, wpftwriter.component.extended2 the six
# SWEXPORTS-block import filters (all <optional/> in wpftwriter.component).
librestudio_add_component(wpftwriter
    COMPONENT      "${CMAKE_SOURCE_DIR}/writerperfect/source/writer/wpftwriter.component"
    IMPLEMENTATION "${CMAKE_SOURCE_DIR}/writerperfect/source/writer/wpftwriter.component.extended"
                   "${CMAKE_SOURCE_DIR}/writerperfect/source/writer/wpftwriter.component.extended2"
    LIBRARY        wpftwriter
    LAYER          OOO)

librestudio_set_library_output_name(wpftwriter wpftwriter OOO)

install(TARGETS wpftwriter LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# wpftdraw -- Draw import filters.
# ---------------------------------------------------------------------------

set(WPFTDRAW_SOURCES
    writerperfect/source/draw/CDRImportFilter
    writerperfect/source/draw/CMXImportFilter
    writerperfect/source/draw/FreehandImportFilter
    writerperfect/source/draw/MSPUBImportFilter
    writerperfect/source/draw/MWAWDrawImportFilter
    writerperfect/source/draw/PageMakerImportFilter
    writerperfect/source/draw/QXPImportFilter
    writerperfect/source/draw/StarOfficeDrawImportFilter
    writerperfect/source/draw/VisioImportFilter
    writerperfect/source/draw/WPGImportFilter
    writerperfect/source/draw/ZMFImportFilter
)

librestudio_expand_cxx(WPFTDRAW_SOURCES)
add_library(wpftdraw SHARED ${WPFTDRAW_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(wpftdraw REUSE_FROM precompiled_system)
endif ()

target_include_directories(wpftdraw PRIVATE
    "${CMAKE_SOURCE_DIR}/writerperfect/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(wpftdraw udkapi_normal_headers offapi_normal_headers)

librestudio_pkgconfig(NAME CDR MODULES libcdr-0.1 DRIVER libcdr)
librestudio_pkgconfig(NAME FREEHAND MODULES libfreehand-0.1 DRIVER libfreehand)
librestudio_pkgconfig(NAME MSPUB MODULES libmspub-0.1 DRIVER libmspub)
librestudio_pkgconfig(NAME PAGEMAKER MODULES libpagemaker-0.0 DRIVER libpagemaker)
librestudio_pkgconfig(NAME QXP MODULES libqxp-0.0 DRIVER libqxp)
librestudio_pkgconfig(NAME VISIO MODULES libvisio-0.1 DRIVER libvisio)
librestudio_pkgconfig(NAME ZMF MODULES libzmf-0.0 DRIVER libzmf)

target_link_libraries(wpftdraw PRIVATE
    comphelper cppu cppuhelper sal sot svx tl utl vcl writerperfect xo
    Boost::boost
    ICU::uc ICU::i18n
    LibXml2::LibXml2
    ${LIBRESTUDIO_PNG_TARGET}
    ${LIBRESTUDIO_CDR_TARGET}
    ${LIBRESTUDIO_FREEHAND_TARGET}
    ${LIBRESTUDIO_MSPUB_TARGET}
    ${LIBRESTUDIO_MWAW_TARGET}
    ${LIBRESTUDIO_ODFGEN_TARGET}
    ${LIBRESTUDIO_PAGEMAKER_TARGET}
    ${LIBRESTUDIO_QXP_TARGET}
    ${LIBRESTUDIO_REVENGE_TARGET}
    ${LIBRESTUDIO_STAROFFICE_TARGET}
    ${LIBRESTUDIO_VISIO_TARGET}
    ${LIBRESTUDIO_WPG_TARGET}
    ${LIBRESTUDIO_WPD_TARGET}
    ${LIBRESTUDIO_LCMS2_TARGET}
    ${LIBRESTUDIO_ZMF_TARGET}
    ${LIBRESTUDIO_ZLIB_TARGET})

librestudio_add_component(wpftdraw
    COMPONENT "${CMAKE_SOURCE_DIR}/writerperfect/source/draw/wpftdraw.component"
    LIBRARY   wpftdraw
    LAYER     OOO)

librestudio_set_library_output_name(wpftdraw wpftdraw OOO)

# --- config_host/config_writerperfect.h -------------------------------------
#
# Real gap found via the remote test build: config_host/config_writerperfect.h
# is a genuine upstream config header (WpftFilterFixture.hxx #includes it) that
# this port never generated at all -- neither cmake/config/config_writerperfect.h.in
# nor an entry in cmake/LibreStudioConfigHeaders.cmake's _config_headers list
# exist (config_clang is the tree's only other real gap of this kind, a
# deliberate omission -- Clang-specific, irrelevant to this GCC port). Not
# folded into that generic upfront batch: that list is configure_file()'d
# very early, from root CMakeLists.txt, long before writerperfect's own
# librestudio_pkgconfig() calls above/below have resolved EBOOK/ETONYEK/MWAW/
# STAROFFICE/WPS -- their version numbers (the entire content of this header)
# would not exist yet at that point. Generated here instead, self-contained,
# right before this header's one and only real consumer.
#
# configure.ac's own libo_PKG_VERSION macro (m4/libo_pkg_version.m4, read in
# full): `pkg-config --modversion` when SYSTEM_<NAME> (this port's own
# LIBRESTUDIO_<NAME>_FOUND), else the hardcoded BUNDLED-VERSION literal --
# which is exactly this port's own VERSION floor for each of these 5
# librestudio_pkgconfig() calls above (0.1.10/0.3.21/0.0.7/0.4.14/0.1.2 all
# individually cross-checked against configure.ac's own libo_PKG_VERSION
# call sites, same values). A second, quiet pkg_check_modules() probe (no
# IMPORTED_TARGET -- version only, doesn't fight the router's own memoized
# PkgConfig::<NAME> target) reproduces the exact same `pkg-config
# --modversion` upstream itself runs.
#
# The MODULES names + VERSION floors passed to librestudio_pkgconfig() above/
# below, repeated here verbatim (not re-derived) so this block cannot drift
# out of sync with them silently.
set(_WPFT_EBOOK_MODULE "libe-book-0.1")
set(_WPFT_EBOOK_FLOOR "0.1.2")
set(_WPFT_ETONYEK_MODULE "libetonyek-0.1")
set(_WPFT_ETONYEK_FLOOR "0.1.10")
set(_WPFT_MWAW_MODULE "libmwaw-0.3")
set(_WPFT_MWAW_FLOOR "0.3.21")
set(_WPFT_STAROFFICE_MODULE "libstaroffice-0.0")
set(_WPFT_STAROFFICE_FLOOR "0.0.7")
set(_WPFT_WPS_MODULE "libwps-0.4")
set(_WPFT_WPS_FLOOR "0.4.14")

set(_WPFT_CONFIG_CONTENT
"/* Generated by LibreStudio's CMake build -- do not edit. */
")
foreach (_wpft_lib EBOOK ETONYEK MWAW STAROFFICE WPS)
    if (LIBRESTUDIO_${_wpft_lib}_FOUND)
        pkg_check_modules(_wpft_verprobe_${_wpft_lib} QUIET "${_WPFT_${_wpft_lib}_MODULE}")
        set(_wpft_ver "${_wpft_verprobe_${_wpft_lib}_VERSION}")
    else ()
        set(_wpft_ver "${_WPFT_${_wpft_lib}_FLOOR}")
    endif ()
    string(REPLACE "." ";" _wpft_ver_parts "${_wpft_ver}")
    list(GET _wpft_ver_parts 0 _wpft_major)
    list(GET _wpft_ver_parts 1 _wpft_minor)
    list(GET _wpft_ver_parts 2 _wpft_micro)
    string(APPEND _WPFT_CONFIG_CONTENT
"#define ${_wpft_lib}_VERSION_MAJOR ${_wpft_major}
#define ${_wpft_lib}_VERSION_MINOR ${_wpft_minor}
#define ${_wpft_lib}_VERSION_MICRO ${_wpft_micro}
")
    unset(_wpft_ver_parts)
    unset(_wpft_major)
    unset(_wpft_minor)
    unset(_wpft_micro)
    unset(_wpft_ver)
endforeach ()
unset(_wpft_lib)
file(WRITE "${LIBRESTUDIO_CONFIG_HOST_DIR}/config_writerperfect.h" "${_WPFT_CONFIG_CONTENT}")
unset(_WPFT_CONFIG_CONTENT)

# --- Library_wpftqahelper (a CppunitTest fixture helper, no .component) ----
#
# Library_wpftqahelper.mk, read in full: 3 sources (writerperfect/qa/unit/
# WpftFilterFixture, WpftFilterTestBase, WpftLoader), set_include (-I
# writerperfect/qa/unit $$(INCLUDE)), add_defs -DWPFTQAHELPER_DLLIMPLEMENTATION,
# use_externals(boost_headers cppunit), use_sdk_api, use_libraries (cppu sal
# test tl ucbhelper unotest -- "test" -> test_fixtures, same rename every
# other CppunitTest/helper library in this port already applies). Ships no
# .component (confirmed: no writerperfect/qa/unit/*.component file exists),
# consumed only by 6 of writerperfect's own 9 CppunitTests via plain
# LIBRARIES, same shape test_fixtures/unotest themselves already have.
set(WPFTQAHELPER_SOURCES
    writerperfect/qa/unit/WpftFilterFixture
    writerperfect/qa/unit/WpftFilterTestBase
    writerperfect/qa/unit/WpftLoader
)
librestudio_expand_cxx(WPFTQAHELPER_SOURCES)

add_library(wpftqahelper SHARED ${WPFTQAHELPER_SOURCES})

if (NOT TARGET PkgConfig::CPPUNIT)
    pkg_check_modules(CPPUNIT REQUIRED IMPORTED_TARGET cppunit)
endif ()

target_include_directories(wpftqahelper PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/qa/unit"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(wpftqahelper udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(wpftqahelper PRIVATE WPFTQAHELPER_DLLIMPLEMENTATION)

target_link_libraries(wpftqahelper PRIVATE
    cppu sal test_fixtures tl ucbhelper unotest
    Boost::boost PkgConfig::CPPUNIT)

librestudio_set_library_output_name(wpftqahelper wpftqahelper OOO)

install(TARGETS wpftqahelper LIBRARY DESTINATION program)

# --- UIConfig_writerperfect --------------------------------------------------
#
# UIConfig_writerperfect.mk: gb_UIConfig_UIConfig(writerperfect) -- 2
# uifiles (wpftencodingdialog, exportepub), unconditional. Same multi-file
# install()+build-tree-copy-loop+ALL-target pattern already established
# (vcl_uiconfig, svt_uiconfig, ..., swriter_uiconfig) -- single category
# here (ui only), so no per-category pair-list foreach is needed, just a
# plain per-file loop.
set(WRITERPERFECT_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/wpftencodingdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/exportepub.ui")

install(FILES ${WRITERPERFECT_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/writerperfect/ui)

set(_wpft_uiconfig_outputs "")
foreach (_wpft_uiconfig_src IN LISTS WRITERPERFECT_UICONFIG_UI_FILES)
    get_filename_component(_wpft_uiconfig_name "${_wpft_uiconfig_src}" NAME)
    set(_wpft_uiconfig_dest
        "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/writerperfect/ui/${_wpft_uiconfig_name}")
    add_custom_command(
        OUTPUT "${_wpft_uiconfig_dest}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/writerperfect/ui"
        COMMAND ${CMAKE_COMMAND} -E copy "${_wpft_uiconfig_src}" "${_wpft_uiconfig_dest}"
        DEPENDS "${_wpft_uiconfig_src}"
        COMMENT "Copying ${_wpft_uiconfig_name} into build-tree soffice.cfg/writerperfect/ui")
    list(APPEND _wpft_uiconfig_outputs "${_wpft_uiconfig_dest}")
endforeach ()
unset(_wpft_uiconfig_src)
unset(_wpft_uiconfig_name)
unset(_wpft_uiconfig_dest)

add_custom_target(writerperfect_uiconfig ALL DEPENDS ${_wpft_uiconfig_outputs})
unset(_wpft_uiconfig_outputs)

# --- CppunitTests --------------------------------------------------------------
#
# writerperfect has 9 CppunitTest_*.mk files. writerperfect_stream is the
# only one with no use_rdb(services) call (re-checked directly) -- its
# use_configuration/use_components both resolve against modules already
# add_subdirectory()'d before writerperfect's own position (officecfg,
# configmgr, i18npool, package, ucb, unotest, test all sit textually before
# writerperfect in the root CMakeLists.txt), so it is wired directly here.
# The other 8 all call use_rdb(services) -> DEPENDS program_services_rdb,
# defined much later (near the end of the root CMakeLists.txt, well after
# writerperfect) -- deferred to writerperfect/CppunitTest_deferred.cmake,
# same later-binding constraint documented in e.g. svgio/CppunitTest_
# deferred.cmake's own header comment.
#
# CppunitTest_writerperfect_stream.mk, re-checked directly: 3 sources
# (DirectoryStreamTest, WPXSvStreamTest, stream), set_include (-Iwriterperfect/
# source/common -Iwriterperfect/inc $$(INCLUDE)), use_sdk_api, use_ure +
# use_vcl (plain, headless) -> 3 protector pairs, use_externals(boost_headers
# revenge), use_libraries (comphelper cppu cppuhelper sal sot test tl
# ucbhelper unotest utl writerperfect -- "test" -> test_fixtures),
# use_configuration -> CONFIGURATION, use_components (configmgr/source/
# configmgr, i18npool/util/i18npool, package/util/package2, ucb/source/core/
# ucb1, ucb/source/ucp/file/ucpfile1, unotools/util/utl) -> COMPONENTS
# configmgr i18npool package2 ucb1 ucpfile1 utl (each verified against the
# owning module's own librestudio_add_component() call).
librestudio_add_cppunit_test(writerperfect_stream
    SOURCES
        writerperfect/qa/unit/DirectoryStreamTest
        writerperfect/qa/unit/WPXSvStreamTest
        writerperfect/qa/unit/stream
    LIBRARIES
        comphelper cppu cppuhelper sal sot test_fixtures tl ucbhelper unotest
        utl writerperfect
        Boost::boost ${LIBRESTUDIO_REVENGE_TARGET}
    INCLUDE
        "${CMAKE_SOURCE_DIR}/writerperfect/source/common"
        "${CMAKE_SOURCE_DIR}/writerperfect/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool package2 ucb1 ucpfile1 utl
    CONFIGURATION)

cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/writerperfect/CppunitTest_deferred.cmake")

install(TARGETS wpftdraw LIBRARY DESTINATION program)
