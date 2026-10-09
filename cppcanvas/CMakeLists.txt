# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# cppcanvas — the cppcanvas library: a thin C++ wrapper around the canvas UNO
# API (wrapper/) plus the metafile-to-canvas renderer (mtfrenderer/) that
# drawinglayer's own processor2d links against. No component file of its own;
# nothing in it is a UNO service.
#
# Also ported here: mtfrenderer (Library_mtfrenderer.mk), a second, UNO-
# service-registered metafile renderer (com.sun.star.rendering.MtfRenderer)
# used by the legacy GraphicObject path -- 1 source
# (cppcanvas/source/uno/uno_mtfrenderer), needed as a CppunitTest_cppcanvas_
# emfplus.mk COMPONENTS entry ("cppcanvas/source/uno/mtfrenderer"). See the
# CppunitTest section below and cppcanvas/CppunitTest_deferred.cmake for
# CppunitTest_cppcanvas_test.mk/CppunitTest_cppcanvas_emfplus.mk themselves,
# now wired for real (deferred for build-order reasons, not left unported).

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

# --- mtfrenderer (Library_mtfrenderer.mk) -------------------------------------
#
# 1 source (cppcanvas/source/uno/uno_mtfrenderer), gb_Library_use_sdk_api
# (normal udkapi+offapi), gb_Library_use_libraries(sal cppu cppuhelper
# comphelper cppcanvas basegfx), gb_Library_set_componentfile(mtfrenderer,
# cppcanvas/source/uno/mtfrenderer,services) -- the "services" third arg is
# the office layer, same convention as cppcanvas/util-style components
# elsewhere in this port (e.g. uui/CMakeLists.txt's own uui.component note).
# uno_mtfrenderer.cxx's own #include <cppcanvas/vclfactory.hxx> resolves
# through the top-level "${CMAKE_SOURCE_DIR}/include" directory already on
# every target's include path (cmake/LibreStudioPlatform.cmake's global
# include_directories() call) -- include/cppcanvas/vclfactory.hxx is the real
# public header, confirmed present -- so no extra INCLUDE is needed here.

set(MTFRENDERER_SOURCES
    cppcanvas/source/uno/uno_mtfrenderer
)

librestudio_expand_cxx(MTFRENDERER_SOURCES)
add_library(mtfrenderer SHARED ${MTFRENDERER_SOURCES})

target_include_directories(mtfrenderer PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(mtfrenderer udkapi_normal_headers offapi_normal_headers)

target_link_libraries(mtfrenderer PRIVATE
    sal cppu cppuhelper comphelper cppcanvas basegfx)

librestudio_add_component(mtfrenderer
    COMPONENT "${CMAKE_SOURCE_DIR}/cppcanvas/source/uno/mtfrenderer.component"
    LIBRARY   mtfrenderer
    LAYER     OOO)

librestudio_set_library_output_name(mtfrenderer mtfrenderer OOO)

install(TARGETS mtfrenderer LIBRARY DESTINATION program)

# cppcanvas/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/cppcanvas/CppunitTest_deferred.cmake")
