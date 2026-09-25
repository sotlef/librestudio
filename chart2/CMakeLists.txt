# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# chart2 — only the chart2api library so far: a tiny abstract-interface library
# (a single translation unit) with no component file and no dependencies beyond
# the SDK API. It exists so that other modules (xmloff) can implement pivot
# table data providers without linking the full chart2 UI/model library, which
# is not ported yet.

set(CHART2API_SOURCES
    chart2/source/api/AbstractPivotTableDataProvider
)

librestudio_expand_cxx(CHART2API_SOURCES)
add_library(chart2api SHARED ${CHART2API_SOURCES})

target_include_directories(chart2api PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(chart2api udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(chart2api PRIVATE CHART2API_DLLIMPLEMENTATION)

librestudio_set_library_output_name(chart2api chart2api OOO)
install(TARGETS chart2api LIBRARY DESTINATION program)
