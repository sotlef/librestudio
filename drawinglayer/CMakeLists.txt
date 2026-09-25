# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# drawinglayer — the primitive-based rendering layer.
#
# Only the first of its two libraries is here.  drawinglayercore is the
# geometry and primitive part and needs nothing that is not built yet, which is
# why it comes before the GUI cascade; drawinglayer itself adds the processor
# and renderer implementations and links svt and vcl, so it waits until those
# exist.  Source lists from Library_drawinglayercore.mk.

set(DRAWINGLAYERCORE_SOURCES
    drawinglayer/source/primitive2d/baseprimitive2d
    drawinglayer/source/primitive2d/BufferedDecompositionPrimitive2D
    drawinglayer/source/primitive2d/BufferedDecompositionFlusher
    drawinglayer/source/primitive2d/BufferedDecompositionGroupPrimitive2D
    drawinglayer/source/primitive2d/Primitive2DContainer
    drawinglayer/source/primitive2d/groupprimitive2d
    drawinglayer/source/primitive2d/Tools
    drawinglayer/source/geometry/viewinformation2d
)

librestudio_expand_cxx(DRAWINGLAYERCORE_SOURCES)
add_library(drawinglayercore SHARED ${DRAWINGLAYERCORE_SOURCES})

target_include_directories(drawinglayercore PRIVATE
    "${CMAKE_SOURCE_DIR}/drawinglayer/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(drawinglayercore
    officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(drawinglayercore PRIVATE DRAWINGLAYERCORE_DLLIMPLEMENTATION)
target_link_libraries(drawinglayercore PRIVATE
    basegfx comphelper cppu cppuhelper i18nlangtag sal salhelper svl tl utl
    Boost::boost)

# No component file: this library is linked against, not loaded by name.
librestudio_set_library_output_name(drawinglayercore drawinglayercore OOO)
install(TARGETS drawinglayercore LIBRARY DESTINATION program)
