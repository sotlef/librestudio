# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# canvas — only canvastools so far: geometry/colour helpers and small UNO
# service plumbing (sprite redraw, surface proxies) shared by every canvas
# back end, and the one piece cppcanvas and drawinglayer actually link.  It
# has no component file of its own; nothing in it is a UNO service.
#
# Module_canvas.mk also builds Library_canvasfactory (the canvas::Factory
# service that picks a back end at run time), Library_vclcanvas and, under
# ENABLE_CAIRO_CANVAS/ENABLE_OPENGL_CANVAS, Library_cairocanvas and
# Library_oglcanvas — the actual renderer back ends. None of those are needed
# by anything ported so far and are not ported yet.
#
# The ENABLE_AGG branch of Library_canvastools.mk (canvas/source/tools/bitmap,
# canvas/source/tools/image, linking the agg library) is skipped: this build
# has no ENABLE_AGG switch, matching upstream where AGG is off by default.

set(CANVASTOOLS_SOURCES
    canvas/source/tools/CanvasFontBase
    canvas/source/tools/cachedprimitivebase
    canvas/source/tools/canvascustomspritehelper
    canvas/source/tools/canvastools
    canvas/source/tools/elapsedtime
    canvas/source/tools/page
    canvas/source/tools/pagemanager
    canvas/source/tools/parametricpolypolygon
    canvas/source/tools/propertysethelper
    canvas/source/tools/spriteredrawmanager
    canvas/source/tools/surface
    canvas/source/tools/surfaceproxy
    canvas/source/tools/surfaceproxymanager
    canvas/source/tools/verifyinput
)

librestudio_expand_cxx(CANVASTOOLS_SOURCES)
add_library(canvastools SHARED ${CANVASTOOLS_SOURCES})

target_include_directories(canvastools PRIVATE
    "${CMAKE_SOURCE_DIR}/canvas/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(canvastools udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(canvastools PRIVATE CANVASTOOLS_DLLIMPLEMENTATION)
target_link_libraries(canvastools PRIVATE
    basegfx comphelper cppu cppuhelper sal tk tl vcl
    Boost::boost)

librestudio_set_library_output_name(canvastools canvastools OOO)
install(TARGETS canvastools LIBRARY DESTINATION program)
