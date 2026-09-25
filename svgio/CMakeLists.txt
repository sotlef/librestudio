# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# svgio (module svgio) — the SVG import filter/renderer: parses SVG XML into a
# drawinglayer primitive tree via the com.sun.star.graphic.SvgTools UNO
# service. Source list from Library_svgio.mk. No *_DLLIMPLEMENTATION define is
# declared there (svgio exports no dllexport-guarded symbols of its own), so
# none is set here either.

set(SVGIO_SOURCES
    svgio/source/svgreader/svgcharacternode
    svgio/source/svgreader/svgcirclenode
    svgio/source/svgreader/svgclippathnode
    svgio/source/svgreader/svgdocument
    svgio/source/svgreader/svgdocumenthandler
    svgio/source/svgreader/svgellipsenode
    svgio/source/svgreader/svggnode
    svgio/source/svgreader/svganode
    svgio/source/svgreader/svgfeblendnode
    svgio/source/svgreader/svgfecolormatrixnode
    svgio/source/svgreader/svgfecompositenode
    svgio/source/svgreader/svgfedropshadownode
    svgio/source/svgreader/svgfefloodnode
    svgio/source/svgreader/svgfeimagenode
    svgio/source/svgreader/svgfegaussianblurnode
    svgio/source/svgreader/svgfemergenode
    svgio/source/svgreader/svgfemergenodenode
    svgio/source/svgreader/svgfeoffsetnode
    svgio/source/svgreader/svgfilternode
    svgio/source/svgreader/svggradientnode
    svgio/source/svgreader/svggradientstopnode
    svgio/source/svgreader/svgimagenode
    svgio/source/svgreader/svglinenode
    svgio/source/svgreader/svgmarkernode
    svgio/source/svgreader/svgmasknode
    svgio/source/svgreader/svgnode
    svgio/source/svgreader/SvgNumber
    svgio/source/svgreader/svgpathnode
    svgio/source/svgreader/svgpatternnode
    svgio/source/svgreader/svgpolynode
    svgio/source/svgreader/svgrectnode
    svgio/source/svgreader/svgstyleattributes
    svgio/source/svgreader/svgstylenode
    svgio/source/svgreader/svgsvgnode
    svgio/source/svgreader/svgswitchnode
    svgio/source/svgreader/svgsymbolnode
    svgio/source/svgreader/svgtextnode
    svgio/source/svgreader/svgtextposition
    svgio/source/svgreader/svgtitledescnode
    svgio/source/svgreader/svgtoken
    svgio/source/svgreader/svgtrefnode
    svgio/source/svgreader/svgtools
    svgio/source/svgreader/svgtextpathnode
    svgio/source/svgreader/svgtspannode
    svgio/source/svgreader/svgusenode
    svgio/source/svgreader/svgvisitor
    svgio/source/svguno/xsvgparser
)

librestudio_expand_cxx(SVGIO_SOURCES)
add_library(svgio SHARED ${SVGIO_SOURCES})

target_include_directories(svgio PRIVATE
    "${CMAKE_SOURCE_DIR}/svgio/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}")
add_dependencies(svgio officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(svgio PRIVATE
    basegfx comphelper cppu cppuhelper drawinglayercore drawinglayer
    i18nlangtag sal salhelper sax svt tk tl utl vcl
    Boost::boost)

librestudio_add_component(svgio
    COMPONENT "${CMAKE_SOURCE_DIR}/svgio/svgio.component"
    LIBRARY   svgio
    LAYER     OOO)

librestudio_set_library_output_name(svgio svgio OOO)

install(TARGETS svgio LIBRARY DESTINATION program)
