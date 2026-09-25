# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# docmodel — the document-level colour, theme and chart styles the drawing and
# chart code shares.  Source list from Library_docmodel.mk.
#
# No component file: this library is linked against by name rather than loaded
# by the service manager.

set(DOCMODEL_SOURCES
    docmodel/source/uno/UnoComplexColor
    docmodel/source/uno/UnoChartStyle
    docmodel/source/uno/UnoChartColorStyle
    docmodel/source/uno/UnoGradientTools
    docmodel/source/uno/UnoTheme
    docmodel/source/theme/ColorSet
    docmodel/source/theme/Theme
    docmodel/source/color/ComplexColor
    docmodel/source/styles/ChartStyle
    docmodel/source/styles/ChartColorStyle
)

librestudio_expand_cxx(DOCMODEL_SOURCES)
add_library(docmodel SHARED ${DOCMODEL_SOURCES})

target_include_directories(docmodel PRIVATE
    "${CMAKE_SOURCE_DIR}/docmodel/inc"
    "${FROZEN_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(docmodel udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(docmodel PRIVATE DOCMODEL_DLLIMPLEMENTATION)
target_link_libraries(docmodel PRIVATE
    basegfx comphelper cppu cppuhelper sal tl
    Boost::boost
    LibXml2::LibXml2)

librestudio_set_library_output_name(docmodel docmodel OOO)
install(TARGETS docmodel LIBRARY DESTINATION program)
