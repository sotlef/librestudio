# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# filter — msfilter, icg, filterconfig and storagefd are built here. msfilter
# is the shared MS-binary-format helper library (Escher/DFF drawing import,
# PPT drawing model (svdfppt), VBA helpers, RTF utilities, MS document codecs)
# that svx and other app modules link against. icg is the CGM (Computer
# Graphics Metafile) import filter engine, needed unconditionally by
# Library_sd (sd's gallery/theme code) — see filter/Library_icg.mk, no
# external dependencies beyond boost_headers, no .component (internal helper
# library, no UNO service registration). filterconfig is the type-detection
# and filter-factory cache (Library_filterconfig.mk, component filterconfig1)
# that framework's document loading goes through, and storagefd
# (Library_storagefd.mk, component storagefd) is the one-source OOXML/ODF
# package-vs-flat storage detector used during type detection; neither needs
# any external beyond boost_headers, confirmed by reading both .mk files
# directly.
#
# filterconfig and storagefd were previously swept into the list below of
# Library_*.mk files needing externals not yet confirmed in this port — that
# was wrong for these two specifically (verified: neither uses poppler,
# cairo, or any other unconfirmed external) and is corrected here now that
# both are ported. odfflatxml/xmlfa/xmlfd are ALSO now confirmed and ported
# (2026-09-28, closing the gap `sc_filters_test`/`chart2_import`/`import2`/
# `geometry`/`pivot_chart_test`/`trendcalculators` all separately documented
# this session) -- each is a single-source-file library needing only
# boost_headers (xmlfa/xmlfd) or no external at all (odfflatxml), use_sdk_api
# only (no officecfg/registry custom headers, same shape as storagefd above),
# every use_libraries entry already ported elsewhere in this tree.
#
# 2026-09-29: the remaining 7 Library_*.mk files under filter/ -- pdffilter,
# graphicfilter, svgfilter, t602filter, textfd, xsltfilter, xsltdlg -- are
# ALSO now ported. The earlier "needs externals not yet confirmed" caution
# turned out to be overly cautious: none of the 7 needs poppler, cairo, or
# any external this port hadn't already resolved (re-verified by reading
# every one of their own .mk files in full, not assumed): boost_headers
# (all 7), libxml2 (svgfilter, xsltfilter -- both already resolved
# elsewhere in this tree), libxslt+libexslt (xsltfilter only -- LIBXSLT
# already resolved once at CMakeLists.txt's own root scope and reused here
# the same way helpcompiler already does; LIBEXSLT resolved locally below,
# not previously declared anywhere in this tree). Every gb_Library_use_
# libraries entry across all 7 is already a real target somewhere in this
# port. See each library's own block below for its specific notes
# (pdffilter needs officecfg/registry custom headers like filterconfig
# above; svgfilter needs one genuinely-live piece of CustomTarget_svg's
# real codegen -- and one honestly-confirmed-dead piece, not reproduced).
#
# Source list, includes, defines and library set for msfilter/icg are
# transcribed from Library_msfilter.mk / Library_icg.mk; filterconfig's and
# storagefd's from their own .mk files. All four are registered in the OOO
# layer, matching Repository.mk's
# gb_Helper_register_libraries_for_install(OOOLIBS,ooo,...) — filterconfig at
# line 426, storagefd at line 477 of that block.

set(MSFILTER_SOURCES
    filter/source/msfilter/countryid
    filter/source/msfilter/dffpropset
    filter/source/msfilter/dffrecordheader
    filter/source/msfilter/escherex
    filter/source/msfilter/eschesdo
    filter/source/msfilter/mscodec
    filter/source/msfilter/msdffimp
    filter/source/msfilter/msocximex
    filter/source/msfilter/msoleexp
    filter/source/msfilter/mstoolbar
    filter/source/msfilter/msvbahelper
    filter/source/msfilter/svdfppt
    filter/source/msfilter/svxmsbas2
    filter/source/msfilter/rtfutil
    filter/source/msfilter/util
)

librestudio_expand_cxx(MSFILTER_SOURCES)
add_library(msfilter SHARED ${MSFILTER_SOURCES})

target_include_directories(msfilter PRIVATE
    "${CMAKE_SOURCE_DIR}/filter/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}")
add_dependencies(msfilter officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(msfilter PRIVATE MSFILTER_DLLIMPLEMENTATION)

target_link_libraries(msfilter PRIVATE
    svxcore editeng sfx xo sb salhelper svt tk vcl svl sot xmlscript utl tl
    comphelper basegfx cppuhelper cppu sal
    Boost::boost
    LibXml2::LibXml2)

librestudio_add_component(msfilter
    COMPONENT "${CMAKE_SOURCE_DIR}/filter/source/msfilter/msfilter.component"
    LIBRARY   msfilter
    LAYER     OOO)

librestudio_set_library_output_name(msfilter msfilter OOO)

install(TARGETS msfilter LIBRARY DESTINATION program)

set(ICG_SOURCES
    filter/source/graphicfilter/icgm/actimpr
    filter/source/graphicfilter/icgm/bitmap
    filter/source/graphicfilter/icgm/bundles
    filter/source/graphicfilter/icgm/cgm
    filter/source/graphicfilter/icgm/chart
    filter/source/graphicfilter/icgm/class0
    filter/source/graphicfilter/icgm/class1
    filter/source/graphicfilter/icgm/class2
    filter/source/graphicfilter/icgm/class3
    filter/source/graphicfilter/icgm/class4
    filter/source/graphicfilter/icgm/class5
    filter/source/graphicfilter/icgm/class7
    filter/source/graphicfilter/icgm/classx
    filter/source/graphicfilter/icgm/elements
)

librestudio_expand_cxx(ICG_SOURCES)
add_library(icg SHARED ${ICG_SOURCES})

target_include_directories(icg PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(icg officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(icg PRIVATE FILTER_DLLIMPLEMENTATION)

target_link_libraries(icg PRIVATE
    comphelper cppuhelper tk vcl utl tl cppu sal
    Boost::boost)

librestudio_set_library_output_name(icg icg OOO)

install(TARGETS icg LIBRARY DESTINATION program)

# --- filterconfig ---------------------------------------------------------------

# Library_filterconfig.mk: 10 sources under filter/source/config/cache/, the
# type-detection cache and filter/frameloader factories that framework's
# document loading goes through. use_custom_headers officecfg/registry is
# real here (unlike msfilter/icg above, which ask for use_sdk_api only), so
# this is the one library in this file that also depends on officecfg_headers
# and includes OFFICECFG_INCLUDE_DIR — mirroring the pattern already
# established for that combination elsewhere in the tree (e.g. sfx2, svx).
set(FILTERCONFIG_SOURCES
    filter/source/config/cache/basecontainer
    filter/source/config/cache/cacheitem
    filter/source/config/cache/cacheupdatelistener
    filter/source/config/cache/configflush
    filter/source/config/cache/contenthandlerfactory
    filter/source/config/cache/filtercache
    filter/source/config/cache/filterfactory
    filter/source/config/cache/frameloaderfactory
    filter/source/config/cache/querytokenizer
    filter/source/config/cache/typedetection
)

librestudio_expand_cxx(FILTERCONFIG_SOURCES)
add_library(filterconfig SHARED ${FILTERCONFIG_SOURCES})

target_include_directories(filterconfig PRIVATE
    "${CMAKE_SOURCE_DIR}/filter/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(filterconfig officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(filterconfig PRIVATE
    fwk utl tl comphelper cppuhelper cppu sal salhelper sfx i18nlangtag
    Boost::boost)

librestudio_add_component(filterconfig
    COMPONENT "${CMAKE_SOURCE_DIR}/filter/source/config/cache/filterconfig1.component"
    LIBRARY   filterconfig
    LAYER     OOO)

librestudio_set_library_output_name(filterconfig filterconfig OOO)

install(TARGETS filterconfig LIBRARY DESTINATION program)

# --- storagefd -------------------------------------------------------------------

# Library_storagefd.mk: 1 source (filter/source/storagefilterdetect/
# filterdetect), the package-vs-flat storage type detector. No
# use_custom_headers here (unlike filterconfig above), so no
# OFFICECFG_INCLUDE_DIR / officecfg_headers dependency — mirroring
# hwpfilter's CMakeLists.txt, the precedent for a use_sdk_api-only library
# with no officecfg dependency.
set(STORAGEFD_SOURCES
    filter/source/storagefilterdetect/filterdetect
)

librestudio_expand_cxx(STORAGEFD_SOURCES)
add_library(storagefd SHARED ${STORAGEFD_SOURCES})

target_include_directories(storagefd PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(storagefd udkapi_normal_headers offapi_normal_headers)

target_link_libraries(storagefd PRIVATE
    comphelper cppuhelper cppu sal sfx tl utl
    Boost::boost)

librestudio_add_component(storagefd
    COMPONENT "${CMAKE_SOURCE_DIR}/filter/source/storagefilterdetect/storagefd.component"
    LIBRARY   storagefd
    LAYER     OOO)

librestudio_set_library_output_name(storagefd storagefd OOO)

install(TARGETS storagefd LIBRARY DESTINATION program)

# --- odfflatxml --------------------------------------------------------------
#
# Library_odfflatxml.mk: 1 source (filter/source/odfflatxml/OdfFlatXml), a
# pass-through SAX import/export filter for ODF "flat XML" (single-file XML)
# documents. No use_external at all (re-checked directly), use_sdk_api only,
# `-I$(SRCDIR)/filter/inc` (same include this file's msfilter/filterconfig
# already add).
set(ODFFLATXML_SOURCES
    filter/source/odfflatxml/OdfFlatXml
)

librestudio_expand_cxx(ODFFLATXML_SOURCES)
add_library(odfflatxml SHARED ${ODFFLATXML_SOURCES})

target_include_directories(odfflatxml PRIVATE
    "${CMAKE_SOURCE_DIR}/filter/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(odfflatxml udkapi_normal_headers offapi_normal_headers)

target_link_libraries(odfflatxml PRIVATE
    xo tl ucbhelper comphelper cppuhelper cppu sal)

librestudio_add_component(odfflatxml
    COMPONENT "${CMAKE_SOURCE_DIR}/filter/source/odfflatxml/odfflatxml.component"
    LIBRARY   odfflatxml
    LAYER     OOO)

librestudio_set_library_output_name(odfflatxml odfflatxml OOO)

install(TARGETS odfflatxml LIBRARY DESTINATION program)

# --- xmlfa ---------------------------------------------------------------------
#
# Library_xmlfa.mk: 1 source (filter/source/xmlfilteradaptor/
# XmlFilterAdaptor), a generic import/export adaptor that instantiates
# whatever "converter" UNO service name filter configuration data hands it
# at runtime (no XSLT/codegen at build time -- the real XSLT machinery lives
# in the separate, still-unported xsltfilter sibling, not a build dependency
# of xmlfa itself). use_external(boost_headers) -> Boost::boost, use_sdk_api,
# same `-I$(SRCDIR)/filter/inc` (XmlFilterAdaptor.cxx includes strings.hrc
# from there, via the same FilterResId/resmgr runtime-translation mechanism
# chart2/CMakeLists.txt already exercises elsewhere in this port).
set(XMLFA_SOURCES
    filter/source/xmlfilteradaptor/XmlFilterAdaptor
)

librestudio_expand_cxx(XMLFA_SOURCES)
add_library(xmlfa SHARED ${XMLFA_SOURCES})

target_include_directories(xmlfa PRIVATE
    "${CMAKE_SOURCE_DIR}/filter/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(xmlfa udkapi_normal_headers offapi_normal_headers)

target_link_libraries(xmlfa PRIVATE
    comphelper cppuhelper cppu sal tl utl xo
    Boost::boost)

librestudio_add_component(xmlfa
    COMPONENT "${CMAKE_SOURCE_DIR}/filter/source/xmlfilteradaptor/xmlfa.component"
    LIBRARY   xmlfa
    LAYER     OOO)

librestudio_set_library_output_name(xmlfa xmlfa OOO)

install(TARGETS xmlfa LIBRARY DESTINATION program)

# --- xmlfd ---------------------------------------------------------------------
#
# Library_xmlfd.mk: 1 source (filter/source/xmlfilterdetect/filterdetect), a
# pure stream/encoding-sniffing XML format detector (ExtendedTypeDetection
# service) -- no external dependency beyond boost_headers, use_sdk_api only.
# No `-I$(SRCDIR)/filter/inc` in the real .mk (re-checked directly, genuinely
# absent unlike odfflatxml/xmlfa above).
set(XMLFD_SOURCES
    filter/source/xmlfilterdetect/filterdetect
)

librestudio_expand_cxx(XMLFD_SOURCES)
add_library(xmlfd SHARED ${XMLFD_SOURCES})

target_include_directories(xmlfd PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(xmlfd udkapi_normal_headers offapi_normal_headers)

target_link_libraries(xmlfd PRIVATE
    ucbhelper comphelper cppuhelper cppu sal svl utl tl
    Boost::boost)

librestudio_add_component(xmlfd
    COMPONENT "${CMAKE_SOURCE_DIR}/filter/source/xmlfilterdetect/xmlfd.component"
    LIBRARY   xmlfd
    LAYER     OOO)

librestudio_set_library_output_name(xmlfd xmlfd OOO)

install(TARGETS xmlfd LIBRARY DESTINATION program)

# --- pdffilter --------------------------------------------------------------
#
# Library_pdffilter.mk: 6 sources under filter/source/pdf/ (the PDF export
# dialog + PDFExportFilter itself -- Splash/xpdf-side PDF *writing*, not to
# be confused with sdext's xpdfimport, a wholly separate executable that
# *reads* PDFs). use_custom_headers officecfg/registry, same as
# filterconfig above -> OFFICECFG_INCLUDE_DIR + officecfg_headers.
set(PDFFILTER_SOURCES
    filter/source/pdf/impdialog
    filter/source/pdf/pdfdecomposer
    filter/source/pdf/pdfdialog
    filter/source/pdf/pdfexport
    filter/source/pdf/pdffilter
    filter/source/pdf/pdfinteract
)

librestudio_expand_cxx(PDFFILTER_SOURCES)
add_library(pdffilter SHARED ${PDFFILTER_SOURCES})

target_include_directories(pdffilter PRIVATE
    "${CMAKE_SOURCE_DIR}/filter/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(pdffilter officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(pdffilter PRIVATE
    svt svx sfx tk vcl utl tl svl i18nlangtag comphelper basegfx
    cppuhelper cppu sal salhelper drawinglayer drawinglayercore
    Boost::boost)

librestudio_add_component(pdffilter
    COMPONENT "${CMAKE_SOURCE_DIR}/filter/source/pdf/pdffilter.component"
    LIBRARY   pdffilter
    LAYER     OOO)

librestudio_set_library_output_name(pdffilter pdffilter OOO)

install(TARGETS pdffilter LIBRARY DESTINATION program)

# --- graphicfilter ------------------------------------------------------------
#
# Library_graphicfilter.mk: 1 source (filter/source/graphic/
# GraphicExportFilter), the generic GraphicExportFilter UNO service that
# dispatches to whichever concrete image-format writer vcl's own GraphicFilter
# registry knows about. No custom headers, use_sdk_api only.
set(GRAPHICFILTER_SOURCES
    filter/source/graphic/GraphicExportFilter
)

librestudio_expand_cxx(GRAPHICFILTER_SOURCES)
add_library(graphicfilter SHARED ${GRAPHICFILTER_SOURCES})

target_include_directories(graphicfilter PRIVATE
    "${CMAKE_SOURCE_DIR}/filter/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(graphicfilter udkapi_normal_headers offapi_normal_headers)

target_link_libraries(graphicfilter PRIVATE
    svt sfx tk vcl utl tl svl i18nlangtag comphelper basegfx
    cppuhelper cppu sal
    Boost::boost)

librestudio_add_component(graphicfilter
    COMPONENT "${CMAKE_SOURCE_DIR}/filter/source/graphic/graphicfilter.component"
    LIBRARY   graphicfilter
    LAYER     OOO)

librestudio_set_library_output_name(graphicfilter graphicfilter OOO)

install(TARGETS graphicfilter LIBRARY DESTINATION program)

# --- svgfilter ------------------------------------------------------------------
#
# Library_svgfilter.mk: 4 sources under filter/source/svg/ (ODF/Impress ->
# SVG export, including embedding an inline JS presentation-navigation
# engine for exported slideshows). use_custom_headers filter/source/svg
# points at CustomTarget_svg.mk's own real codegen -- read in full,
# transcribed only where actually consumed:
#  - svgscript.hxx (js2hxx.py, presentation_engine.js -> a C string constant
#    array): genuinely #include'd by svgexport.cxx -- reproduced below as a
#    real add_custom_command, same pattern as sw/CMakeLists.txt's own
#    gentoken.py-based codegen.
#  - tokens.hxx/tokens.cxx (tokens.txt -> gperf -> a perfect-hash SVG-token
#    lookup table): a HONEST DEAD-CODE FINDING, not reproduced -- grepped
#    every .cxx/.hxx under filter/source/svg/ (svgfilter's own real source
#    list) for "tokens.hxx"/"tokens.cxx": zero references anywhere, and
#    Library_svgfilter.mk itself never lists tokens.cxx as a compiled
#    source either. svgio (this tree's real SVG *importer*, already fully
#    ported, 3/3 tests) has no tokens.txt/gentoken.py of its own and never
#    references this generated table either -- this looks like a leftover
#    from before svg import was split out of filter/ into its own svgio
#    module, never cleaned from Module_filter.mk's CustomTarget_svg
#    registration. Skipping it costs nothing real: nothing anywhere in
#    this tree would ever link against it.
set(FILTER_SVG_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/svg_generated")
find_program(PYTHON3_EXECUTABLE NAMES python3 python REQUIRED)

add_custom_command(
    OUTPUT "${FILTER_SVG_GENERATED_DIR}/svgscript.hxx"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${FILTER_SVG_GENERATED_DIR}"
    COMMAND "${PYTHON3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/filter/source/svg/js2hxx.py"
            "${CMAKE_SOURCE_DIR}/filter/source/svg/presentation_engine.js"
            "${FILTER_SVG_GENERATED_DIR}/svgscript.hxx"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/filter/source/svg/presentation_engine.js"
        "${CMAKE_SOURCE_DIR}/filter/source/svg/js2hxx.py"
    COMMENT "js2hxx.py: filter/svg_generated/svgscript.hxx"
    VERBATIM)
add_custom_target(filter_svg_generated DEPENDS "${FILTER_SVG_GENERATED_DIR}/svgscript.hxx")

set(SVGFILTER_SOURCES
    filter/source/svg/svgfilter
    filter/source/svg/svgexport
    filter/source/svg/svgfontexport
    filter/source/svg/svgwriter
)

librestudio_expand_cxx(SVGFILTER_SOURCES)
add_library(svgfilter SHARED ${SVGFILTER_SOURCES})

target_include_directories(svgfilter PRIVATE
    "${CMAKE_SOURCE_DIR}/filter/inc"
    "${FILTER_SVG_GENERATED_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(svgfilter filter_svg_generated udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(svgfilter PRIVATE
    BOOST_ALL_NO_LIB
    USE_MODERN_SPIRIT
    FILTER_DLLIMPLEMENTATION)

target_link_libraries(svgfilter PRIVATE
    svxcore editeng xo svt vcl svl utl tl i18nlangtag sax salhelper
    comphelper drawinglayercore drawinglayer basegfx cppuhelper cppu sal
    Boost::boost
    LibXml2::LibXml2)

librestudio_add_component(svgfilter
    COMPONENT "${CMAKE_SOURCE_DIR}/filter/source/svg/svgfilter.component"
    LIBRARY   svgfilter
    LAYER     OOO)

librestudio_set_library_output_name(svgfilter svgfilter OOO)

install(TARGETS svgfilter LIBRARY DESTINATION program)

# --- t602filter -----------------------------------------------------------------
#
# Library_t602filter.mk: 1 source (filter/source/t602/t602filter), the T602
# (a legacy Czech/Slovak DOS word processor format) import filter.
set(T602FILTER_SOURCES
    filter/source/t602/t602filter
)

librestudio_expand_cxx(T602FILTER_SOURCES)
add_library(t602filter SHARED ${T602FILTER_SOURCES})

target_include_directories(t602filter PRIVATE
    "${CMAKE_SOURCE_DIR}/filter/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(t602filter udkapi_normal_headers offapi_normal_headers)

target_link_libraries(t602filter PRIVATE
    xo tl utl comphelper cppuhelper cppu sal i18nlangtag
    Boost::boost)

librestudio_add_component(t602filter
    COMPONENT "${CMAKE_SOURCE_DIR}/filter/source/t602/t602filter.component"
    LIBRARY   t602filter
    LAYER     OOO)

librestudio_set_library_output_name(t602filter t602filter OOO)

install(TARGETS t602filter LIBRARY DESTINATION program)

# --- textfd ---------------------------------------------------------------------
#
# Library_textfd.mk: 1 source (filter/source/textfilterdetect/
# filterdetect), a stream-sniffing plain-text/CSV format detector
# (ExtendedTypeDetection service, the plain-text sibling of xmlfd above).
set(TEXTFD_SOURCES
    filter/source/textfilterdetect/filterdetect
)

librestudio_expand_cxx(TEXTFD_SOURCES)
add_library(textfd SHARED ${TEXTFD_SOURCES})

target_include_directories(textfd PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(textfd udkapi_normal_headers offapi_normal_headers)

target_link_libraries(textfd PRIVATE
    comphelper ucbhelper cppuhelper cppu sal sfx tl utl svt
    Boost::boost)

librestudio_add_component(textfd
    COMPONENT "${CMAKE_SOURCE_DIR}/filter/source/textfilterdetect/textfd.component"
    LIBRARY   textfd
    LAYER     OOO)

librestudio_set_library_output_name(textfd textfd OOO)

install(TARGETS textfd LIBRARY DESTINATION program)

# --- xsltfilter -----------------------------------------------------------------
#
# Library_xsltfilter.mk: 3 sources under filter/source/xsltfilter/, the
# generic XSLT-based import/export filter engine (wraps libxslt directly,
# unlike xmlfa above which only dispatches to whatever converter service
# name filter config data hands it). LIBXSLT already resolved once at
# CMakeLists.txt's own root scope (reused here, same as helpcompiler/
# CMakeLists.txt already does); LIBEXSLT is a separate real pkg-config
# module (libxslt's own EXSLT extension-function library) not yet resolved
# anywhere else in this tree, so declared locally here.
pkg_check_modules(LIBEXSLT REQUIRED IMPORTED_TARGET libexslt)

set(XSLTFILTER_SOURCES
    filter/source/xsltfilter/LibXSLTTransformer
    filter/source/xsltfilter/OleHandler
    filter/source/xsltfilter/XSLTFilter
)

librestudio_expand_cxx(XSLTFILTER_SOURCES)
add_library(xsltfilter SHARED ${XSLTFILTER_SOURCES})

target_include_directories(xsltfilter PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(xsltfilter udkapi_normal_headers offapi_normal_headers)

target_link_libraries(xsltfilter PRIVATE
    xo tl package2 sax comphelper ucbhelper cppuhelper cppu salhelper sal
    Boost::boost
    LibXml2::LibXml2
    PkgConfig::LIBXSLT
    PkgConfig::LIBEXSLT
    ${LIBRESTUDIO_ZLIB_TARGET})

librestudio_add_component(xsltfilter
    COMPONENT "${CMAKE_SOURCE_DIR}/filter/source/xsltfilter/xsltfilter.component"
    LIBRARY   xsltfilter
    LAYER     OOO)

librestudio_set_library_output_name(xsltfilter xsltfilter OOO)

install(TARGETS xsltfilter LIBRARY DESTINATION program)

# --- xsltdlg --------------------------------------------------------------------
#
# Library_xsltdlg.mk: 9 sources under filter/source/xsltdialog/, the XML
# Filter Settings dialog (Tools > XML Filter Settings) that lets users
# create/edit/test XSLT-based import/export filter definitions -- the UI
# counterpart to xsltfilter above, not itself linked against it (they only
# share filter-configuration data at runtime, not a build dependency). Its
# own 3 .ui files (xmlfiltersettings/xmlfiltertabpagegeneral/
# xmlfiltertabpagetransformation) plus xsltfilterdialog.ui and
# testxmlfilter.ui are already part of this file's own UIConfig_filter
# block below (all 13 files were already being installed).
set(XSLTDLG_SOURCES
    filter/source/xsltdialog/typedetectionexport
    filter/source/xsltdialog/typedetectionimport
    filter/source/xsltdialog/xmlfilterdialogcomponent
    filter/source/xsltdialog/xmlfilterjar
    filter/source/xsltdialog/xmlfiltersettingsdialog
    filter/source/xsltdialog/xmlfiltertabdialog
    filter/source/xsltdialog/xmlfiltertabpagebasic
    filter/source/xsltdialog/xmlfiltertabpagexslt
    filter/source/xsltdialog/xmlfiltertestdialog
)

librestudio_expand_cxx(XSLTDLG_SOURCES)
add_library(xsltdlg SHARED ${XSLTDLG_SOURCES})

target_include_directories(xsltdlg PRIVATE
    "${CMAKE_SOURCE_DIR}/filter/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(xsltdlg udkapi_normal_headers offapi_normal_headers)

target_link_libraries(xsltdlg PRIVATE
    sfx svt tk vcl svl utl tl i18nlangtag comphelper cppuhelper cppu sal
    Boost::boost)

librestudio_add_component(xsltdlg
    COMPONENT "${CMAKE_SOURCE_DIR}/filter/source/xsltdialog/xsltdlg.component"
    LIBRARY   xsltdlg
    LAYER     OOO)

librestudio_set_library_output_name(xsltdlg xsltdlg OOO)

install(TARGETS xsltdlg LIBRARY DESTINATION program)

# --- UIConfig_filter ----------------------------------------------------------
#
# UIConfig_filter.mk: gb_UIConfig_UIConfig(filter) -- 13 .ui files under
# filter/uiconfig/ui/, unconditional (no ifeq/ifneq gates in the live .mk),
# packaged into share/config/soffice.cfg/filter/ui -- same convention as
# UIConfig_cui/UIConfig_vcl/UIConfig_svt/UIConfig_svx (module name "filter"
# used directly). All 13 files verified present on disk. CppunitTest_filter_
# dialogs_test.mk's own gb_CppunitTest_use_uiconfigs(filter_dialogs_test,
# filter) call (filter/CppunitTest_deferred.cmake) needs a real build-time
# ALL target to depend on -- introduced here as "filter_uiconfig", same
# pattern as every other *_dialogs_test module.
set(FILTER_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/pdfgeneralpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/pdflinkspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/pdfoptionsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/pdfsecuritypage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/pdfsignpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/pdfuserinterfacepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/pdfviewpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/testxmlfilter.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/warnpdfdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/xmlfiltersettings.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/xmlfiltertabpagegeneral.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/xmlfiltertabpagetransformation.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/xsltfilterdialog.ui")

install(FILES ${FILTER_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/filter/ui)

set(_filter_uiconfig_outputs "")
foreach (_filter_uiconfig_src IN LISTS FILTER_UICONFIG_UI_FILES)
    get_filename_component(_filter_uiconfig_name "${_filter_uiconfig_src}" NAME)
    set(_filter_uiconfig_dest "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/filter/ui/${_filter_uiconfig_name}")
    add_custom_command(
        OUTPUT "${_filter_uiconfig_dest}"
        COMMAND ${CMAKE_COMMAND} -E copy "${_filter_uiconfig_src}" "${_filter_uiconfig_dest}"
        DEPENDS "${_filter_uiconfig_src}"
        COMMENT "Copying ${_filter_uiconfig_name} into build-tree soffice.cfg/filter/ui")
    list(APPEND _filter_uiconfig_outputs "${_filter_uiconfig_dest}")
endforeach ()
unset(_filter_uiconfig_src)
unset(_filter_uiconfig_name)
unset(_filter_uiconfig_dest)

add_custom_target(filter_uiconfig ALL DEPENDS ${_filter_uiconfig_outputs})
unset(_filter_uiconfig_outputs)

# filter/CppunitTest_deferred.cmake needs data (a COMPONENTS name -- ucb's
# ucb1/ucpfile1, add_subdirectory()'d at root CMakeLists.txt line 1151, AFTER
# filter's own line 1130 -- and/or program_services_rdb) from modules
# add_subdirectory()'d later than this one -- scheduled here rather than
# included directly, so it runs only once every add_subdirectory() in the
# whole tree has completed. See that file's own header comment for the
# specific per-test constraint, and cmake/LibreStudioTest.cmake's header
# comment for why CMAKE_SOURCE_DIR (not CMAKE_CURRENT_SOURCE_DIR) is required
# in the path argument below.
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/filter/CppunitTest_deferred.cmake")
