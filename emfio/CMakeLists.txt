# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# emfio (module emfio) — the EMF/WMF (Windows metafile) image-format import
# filter: a single Graphic.EmfTools UNO service (XEmfParser) that reads
# EMF/WMF streams into drawinglayer primitives. Source list from
# Library_emfio.mk.

set(EMFIO_SOURCES
    emfio/source/emfuno/xemfparser
    emfio/source/reader/mtftools
    emfio/source/reader/wmfreader
    emfio/source/reader/emfreader
)

librestudio_expand_cxx(EMFIO_SOURCES)
add_library(emfio SHARED ${EMFIO_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(emfio PRIVATE
        "${CMAKE_SOURCE_DIR}/emfio/inc/pch/precompiled_emfio.hxx")
    target_compile_definitions(emfio PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(emfio PRIVATE
    "${CMAKE_SOURCE_DIR}/emfio/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(emfio officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(emfio PRIVATE EMFIO_DLLIMPLEMENTATION)

target_link_libraries(emfio PRIVATE
    basegfx comphelper cppu cppuhelper drawinglayercore drawinglayer
    sal salhelper svt tl utl vcl
    Boost::boost)

librestudio_add_component(emfio
    COMPONENT "${CMAKE_SOURCE_DIR}/emfio/emfio.component"
    LIBRARY   emfio
    LAYER     OOO)

librestudio_set_library_output_name(emfio emfio OOO)

install(TARGETS emfio LIBRARY DESTINATION program)
