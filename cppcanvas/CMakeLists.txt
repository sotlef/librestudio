# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# cppcanvas — only the cppcanvas library so far: a thin C++ wrapper around the
# canvas UNO API (wrapper/) plus the metafile-to-canvas renderer
# (mtfrenderer/) that drawinglayer's own processor2d links against. No
# component file; nothing in it is a UNO service.
#
# Module_cppcanvas.mk also builds Library_mtfrenderer, a second metafile
# renderer used by the (still unported) legacy GraphicObject path, and the
# CppunitTest_cppcanvas_test/CppunitTest_cppcanvas_emfplus check targets.
# None of those are needed by anything ported so far and are not ported yet.

set(CPPCANVAS_SOURCES
    cppcanvas/source/mtfrenderer/bitmapaction
    cppcanvas/source/mtfrenderer/cachedprimitivebase
    cppcanvas/source/mtfrenderer/implrenderer
    cppcanvas/source/mtfrenderer/lineaction
    cppcanvas/source/mtfrenderer/mtftools
    cppcanvas/source/mtfrenderer/pointaction
    cppcanvas/source/mtfrenderer/polypolyaction
    cppcanvas/source/mtfrenderer/textaction
    cppcanvas/source/mtfrenderer/textlineshelper
    cppcanvas/source/mtfrenderer/transparencygroupaction
    cppcanvas/source/tools/canvasgraphichelper
    cppcanvas/source/tools/tools
    cppcanvas/source/wrapper/basegfxfactory
    cppcanvas/source/wrapper/implbitmap
    cppcanvas/source/wrapper/implbitmapcanvas
    cppcanvas/source/wrapper/implcanvas
    cppcanvas/source/wrapper/implcustomsprite
    cppcanvas/source/wrapper/implpolypolygon
    cppcanvas/source/wrapper/implsprite
    cppcanvas/source/wrapper/implspritecanvas
    cppcanvas/source/wrapper/vclfactory
)

librestudio_expand_cxx(CPPCANVAS_SOURCES)
add_library(cppcanvas SHARED ${CPPCANVAS_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(cppcanvas PRIVATE
        "${CMAKE_SOURCE_DIR}/cppcanvas/inc/pch/precompiled_cppcanvas.hxx")
    target_compile_definitions(cppcanvas PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(cppcanvas PRIVATE
    "${CMAKE_SOURCE_DIR}/cppcanvas/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(cppcanvas udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(cppcanvas PRIVATE CPPCANVAS_DLLIMPLEMENTATION)
target_link_libraries(cppcanvas PRIVATE
    basegfx canvastools comphelper cppu cppuhelper i18nlangtag sal salhelper
    svt tl vcl
    Boost::boost)

librestudio_set_library_output_name(cppcanvas cppcanvas OOO)
install(TARGETS cppcanvas LIBRARY DESTINATION program)
