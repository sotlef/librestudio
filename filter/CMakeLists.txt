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
# both are ported. The remaining Library_*.mk files under filter/ (pdffilter,
# graphicfilter, svgfilter, t602filter, xmlfa, xmlfd, textfd, odfflatxml,
# xsltfilter, xsltdlg) still need their externals individually re-verified in
# a future wave rather than assumed — they are merely unconfirmed, not
# confirmed to need poppler/cairo — and remain out of scope for this task.
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
