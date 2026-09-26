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
#  - Library_wpftqahelper.mk (a CppunitTest fixture helper), UIConfig_writerperfect.mk,
#    the CppunitTest_*/AllLangMoTarget_wpt/UITest_* targets -- test and
#    UI-config targets are out of scope for this pass, same as every other
#    module so far.
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
# and reused, since pkg_check_modules' IMPORTED_TARGET is directory-scoped in
# this port's convention (one CMakeLists.txt = one scope, so one call per
# external suffices for every target below).
pkg_check_modules(REVENGE   REQUIRED IMPORTED_TARGET librevenge-0.0)
pkg_check_modules(ODFGEN    REQUIRED IMPORTED_TARGET libodfgen-0.1)
pkg_check_modules(ETONYEK   REQUIRED IMPORTED_TARGET libetonyek-0.1)
pkg_check_modules(MWAW      REQUIRED IMPORTED_TARGET libmwaw-0.3)
pkg_check_modules(STAROFFICE REQUIRED IMPORTED_TARGET libstaroffice-0.0)
pkg_check_modules(WPS       REQUIRED IMPORTED_TARGET libwps-0.4)
pkg_check_modules(WPD       REQUIRED IMPORTED_TARGET libwpd-0.10)
pkg_check_modules(WPG       REQUIRED IMPORTED_TARGET libwpg-0.3)

# lcms2 and libpng are declared locally in every module that needs them in
# this port (vcl/CMakeLists.txt does the same) rather than assumed visible
# from elsewhere, since directory-scoped imported targets do not cross into
# sibling directories.
pkg_check_modules(LCMS2 REQUIRED IMPORTED_TARGET lcms2)
find_package(PNG REQUIRED)

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
    PkgConfig::ODFGEN
    PkgConfig::REVENGE)

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
    PkgConfig::ETONYEK
    PkgConfig::ODFGEN
    PkgConfig::REVENGE
    PkgConfig::MWAW
    PkgConfig::STAROFFICE
    PkgConfig::WPS
    PkgConfig::LCMS2)

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
    PkgConfig::ETONYEK
    PkgConfig::MWAW
    PkgConfig::ODFGEN
    PkgConfig::STAROFFICE
    PkgConfig::REVENGE)

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

pkg_check_modules(ABW REQUIRED IMPORTED_TARGET libabw-0.1)
pkg_check_modules(EBOOK REQUIRED IMPORTED_TARGET libe-book-0.1)
pkg_check_modules(EPUBGEN REQUIRED IMPORTED_TARGET libepubgen-0.1)

target_link_libraries(wpftwriter PRIVATE
    comphelper cppu cppuhelper i18nlangtag sal sax sfx sot svt svx tl ucbhelper utl vcl writerperfect xo
    Boost::boost
    ICU::uc ICU::i18n
    LibXml2::LibXml2
    PkgConfig::LIBLANGTAG
    PkgConfig::ABW
    PkgConfig::EBOOK
    PkgConfig::EPUBGEN
    PkgConfig::ETONYEK
    PkgConfig::MWAW
    PkgConfig::ODFGEN
    PkgConfig::REVENGE
    PkgConfig::STAROFFICE
    PkgConfig::WPD
    PkgConfig::WPG
    PkgConfig::WPS
    ZLIB::ZLIB)

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

pkg_check_modules(CDR REQUIRED IMPORTED_TARGET libcdr-0.1)
pkg_check_modules(FREEHAND REQUIRED IMPORTED_TARGET libfreehand-0.1)
pkg_check_modules(MSPUB REQUIRED IMPORTED_TARGET libmspub-0.1)
pkg_check_modules(PAGEMAKER REQUIRED IMPORTED_TARGET libpagemaker-0.0)
pkg_check_modules(QXP REQUIRED IMPORTED_TARGET libqxp-0.0)
pkg_check_modules(VISIO REQUIRED IMPORTED_TARGET libvisio-0.1)
pkg_check_modules(ZMF REQUIRED IMPORTED_TARGET libzmf-0.0)

target_link_libraries(wpftdraw PRIVATE
    comphelper cppu cppuhelper sal sot svx tl utl vcl writerperfect xo
    Boost::boost
    ICU::uc ICU::i18n
    LibXml2::LibXml2
    PNG::PNG
    PkgConfig::CDR
    PkgConfig::FREEHAND
    PkgConfig::MSPUB
    PkgConfig::MWAW
    PkgConfig::ODFGEN
    PkgConfig::PAGEMAKER
    PkgConfig::QXP
    PkgConfig::REVENGE
    PkgConfig::STAROFFICE
    PkgConfig::VISIO
    PkgConfig::WPG
    PkgConfig::WPD
    PkgConfig::LCMS2
    PkgConfig::ZMF
    ZLIB::ZLIB)

librestudio_add_component(wpftdraw
    COMPONENT "${CMAKE_SOURCE_DIR}/writerperfect/source/draw/wpftdraw.component"
    LIBRARY   wpftdraw
    LAYER     OOO)

librestudio_set_library_output_name(wpftdraw wpftdraw OOO)

install(TARGETS wpftdraw LIBRARY DESTINATION program)
