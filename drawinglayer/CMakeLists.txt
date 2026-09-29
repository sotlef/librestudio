# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# drawinglayer — the primitive-based rendering layer, two libraries.
#
# drawinglayercore is the geometry and primitive part and needs nothing that
# is not built yet, which is why it was ported before the GUI cascade.
# drawinglayer itself adds the attribute/processor/renderer implementations,
# the 2D/3D primitive decompositions and the EMF+ helpers, and links svt, vcl,
# canvastools and cppcanvas — so it waits until those exist. Source lists from
# Library_drawinglayercore.mk and Library_drawinglayer.mk.
#
# USE_HEADLESS_CODE is TRUE in this build (see cmake/LibreStudioConfigHeaders.cmake),
# so the processor2d exception objects are the cairo-based ones
# (cairopixelprocessor2d, SDPRProcessor2dTools), matching upstream's
# ifeq ($(USE_HEADLESS_CODE),TRUE) branch; the Windows-only Direct2D branch
# (d2dpixelprocessor2d, under ifeq ($(OS)-$(USE_HEADLESS_CODE),WNT-)) does not
# apply here. ENABLE_WASM_STRIP_CANVAS is not defined in this build, matching
# upstream default, so canvastools/cppcanvas are always linked.

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

if (ENABLE_PCH)
    target_precompile_headers(drawinglayercore PRIVATE
        "${CMAKE_SOURCE_DIR}/drawinglayer/inc/pch/precompiled_drawinglayercore.hxx")
    target_compile_definitions(drawinglayercore PRIVATE PCH_LEVEL=2)
endif ()

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

pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)

set(DRAWINGLAYER_SOURCES
    drawinglayer/source/processor2d/cairopixelprocessor2d
    drawinglayer/source/processor2d/SDPRProcessor2dTools
    drawinglayer/source/animation/animationtiming
    drawinglayer/source/attribute/fillgraphicattribute
    drawinglayer/source/attribute/fillgradientattribute
    drawinglayer/source/attribute/fillhatchattribute
    drawinglayer/source/attribute/fontattribute
    drawinglayer/source/attribute/lineattribute
    drawinglayer/source/attribute/linestartendattribute
    drawinglayer/source/attribute/materialattribute3d
    drawinglayer/source/attribute/sdrallattribute3d
    drawinglayer/source/attribute/sdrfillattribute
    drawinglayer/source/attribute/sdrfillgraphicattribute
    drawinglayer/source/attribute/sdrglowattribute
    drawinglayer/source/attribute/sdrglowtextattribute
    drawinglayer/source/attribute/sdrlightattribute3d
    drawinglayer/source/attribute/sdrlightingattribute3d
    drawinglayer/source/attribute/sdrlineattribute
    drawinglayer/source/attribute/sdrlinestartendattribute
    drawinglayer/source/attribute/sdrobjectattribute3d
    drawinglayer/source/attribute/sdrsceneattribute3d
    drawinglayer/source/attribute/sdrshadowattribute
    drawinglayer/source/attribute/strokeattribute
    drawinglayer/source/geometry/viewinformation3d
    drawinglayer/source/primitive2d/animatedprimitive2d
    drawinglayer/source/primitive2d/backgroundcolorprimitive2d
    drawinglayer/source/primitive2d/bitmapprimitive2d
    drawinglayer/source/primitive2d/BitmapAlphaPrimitive2D
    drawinglayer/source/primitive2d/borderlineprimitive2d
    drawinglayer/source/primitive2d/controlprimitive2d
    drawinglayer/source/primitive2d/cropprimitive2d
    drawinglayer/source/primitive2d/discretebitmapprimitive2d
    drawinglayer/source/primitive2d/discreteshadowprimitive2d
    drawinglayer/source/primitive2d/embedded3dprimitive2d
    drawinglayer/source/primitive2d/exclusiveeditviewprimitive2d
    drawinglayer/source/primitive2d/epsprimitive2d
    drawinglayer/source/primitive2d/fillgraphicprimitive2d
    drawinglayer/source/primitive2d/fillgradientprimitive2d
    drawinglayer/source/primitive2d/fillhatchprimitive2d
    drawinglayer/source/primitive2d/glowprimitive2d
    drawinglayer/source/primitive2d/GlowSoftEgdeShadowTools
    drawinglayer/source/primitive2d/graphicprimitivehelper2d
    drawinglayer/source/primitive2d/graphicprimitive2d
    drawinglayer/source/primitive2d/gridprimitive2d
    drawinglayer/source/primitive2d/helplineprimitive2d
    drawinglayer/source/primitive2d/hiddengeometryprimitive2d
    drawinglayer/source/primitive2d/invertprimitive2d
    drawinglayer/source/primitive2d/markerarrayprimitive2d
    drawinglayer/source/primitive2d/maskprimitive2d
    drawinglayer/source/primitive2d/mediaprimitive2d
    drawinglayer/source/primitive2d/metafileprimitive2d
    drawinglayer/source/primitive2d/modifiedcolorprimitive2d
    drawinglayer/source/primitive2d/objectinfoprimitive2d
    drawinglayer/source/primitive2d/pagehierarchyprimitive2d
    drawinglayer/source/primitive2d/pagepreviewprimitive2d
    drawinglayer/source/primitive2d/patternfillprimitive2d
    drawinglayer/source/primitive2d/pointarrayprimitive2d
    drawinglayer/source/primitive2d/polygonprimitive2d
    drawinglayer/source/primitive2d/PolyPolygonHairlinePrimitive2D
    drawinglayer/source/primitive2d/PolyPolygonMarkerPrimitive2D
    drawinglayer/source/primitive2d/PolyPolygonStrokePrimitive2D
    drawinglayer/source/primitive2d/PolyPolygonColorPrimitive2D
    drawinglayer/source/primitive2d/PolyPolygonRGBAPrimitive2D
    drawinglayer/source/primitive2d/PolyPolygonAlphaGradientPrimitive2D
    drawinglayer/source/primitive2d/PolyPolygonGradientPrimitive2D
    drawinglayer/source/primitive2d/PolyPolygonHatchPrimitive2D
    drawinglayer/source/primitive2d/PolyPolygonGraphicPrimitive2D
    drawinglayer/source/primitive2d/PolyPolygonSelectionPrimitive2D
    drawinglayer/source/primitive2d/primitivetools2d
    drawinglayer/source/primitive2d/sceneprimitive2d
    drawinglayer/source/primitive2d/sdrdecompositiontools2d
    drawinglayer/source/primitive2d/shadowprimitive2d
    drawinglayer/source/primitive2d/softedgeprimitive2d
    drawinglayer/source/primitive2d/structuretagprimitive2d
    drawinglayer/source/primitive2d/svggradientprimitive2d
    drawinglayer/source/primitive2d/textbreakuphelper
    drawinglayer/source/primitive2d/textdecoratedprimitive2d
    drawinglayer/source/primitive2d/texteffectprimitive2d
    drawinglayer/source/primitive2d/textenumsprimitive2d
    drawinglayer/source/primitive2d/texthierarchyprimitive2d
    drawinglayer/source/primitive2d/textlayoutdevice
    drawinglayer/source/primitive2d/textlineprimitive2d
    drawinglayer/source/primitive2d/textprimitive2d
    drawinglayer/source/primitive2d/textstrikeoutprimitive2d
    drawinglayer/source/primitive2d/transformprimitive2d
    drawinglayer/source/primitive2d/transparenceprimitive2d
    drawinglayer/source/primitive2d/unifiedtransparenceprimitive2d
    drawinglayer/source/primitive2d/wallpaperprimitive2d
    drawinglayer/source/primitive2d/wrongspellprimitive2d
    drawinglayer/source/primitive3d/baseprimitive3d
    drawinglayer/source/primitive3d/groupprimitive3d
    drawinglayer/source/primitive3d/hatchtextureprimitive3d
    drawinglayer/source/primitive3d/hiddengeometryprimitive3d
    drawinglayer/source/primitive3d/modifiedcolorprimitive3d
    drawinglayer/source/primitive3d/polygonprimitive3d
    drawinglayer/source/primitive3d/polygontubeprimitive3d
    drawinglayer/source/primitive3d/polypolygonprimitive3d
    drawinglayer/source/primitive3d/sdrcubeprimitive3d
    drawinglayer/source/primitive3d/sdrdecompositiontools3d
    drawinglayer/source/primitive3d/sdrextrudelathetools3d
    drawinglayer/source/primitive3d/sdrextrudeprimitive3d
    drawinglayer/source/primitive3d/sdrlatheprimitive3d
    drawinglayer/source/primitive3d/sdrpolypolygonprimitive3d
    drawinglayer/source/primitive3d/sdrprimitive3d
    drawinglayer/source/primitive3d/sdrsphereprimitive3d
    drawinglayer/source/primitive3d/shadowprimitive3d
    drawinglayer/source/primitive3d/textureprimitive3d
    drawinglayer/source/primitive3d/transformprimitive3d
    drawinglayer/source/primitive3d/Tools
    drawinglayer/source/processor2d/baseprocessor2d
    drawinglayer/source/processor2d/processor2dtools
    drawinglayer/source/processor2d/contourextractor2d
    drawinglayer/source/processor2d/getdigitlanguage
    drawinglayer/source/processor2d/hittestprocessor2d
    drawinglayer/source/processor2d/linegeometryextractor2d
    drawinglayer/source/processor2d/objectinfoextractor2d
    drawinglayer/source/processor2d/textextractor2d
    drawinglayer/source/processor2d/textaspolygonextractor2d
    drawinglayer/source/processor2d/vclhelperbufferdevice
    drawinglayer/source/processor2d/vclmetafileprocessor2d
    drawinglayer/source/processor2d/vclpixelprocessor2d
    drawinglayer/source/processor2d/vclprocessor2d
    drawinglayer/source/processor3d/baseprocessor3d
    drawinglayer/source/processor3d/cutfindprocessor3d
    drawinglayer/source/processor3d/defaultprocessor3d
    drawinglayer/source/processor3d/geometry2dextractor
    drawinglayer/source/processor3d/shadow3dextractor
    drawinglayer/source/processor3d/zbufferprocessor3d
    drawinglayer/source/texture/texture3d
    drawinglayer/source/tools/converters
    drawinglayer/source/tools/emfplushelper
    drawinglayer/source/tools/emfphelperdata
    drawinglayer/source/tools/emfpbrush
    drawinglayer/source/tools/emfppath
    drawinglayer/source/tools/emfppen
    drawinglayer/source/tools/emfpregion
    drawinglayer/source/tools/emfpimage
    drawinglayer/source/tools/emfpimageattributes
    drawinglayer/source/tools/emfpfont
    drawinglayer/source/tools/emfpstringformat
    drawinglayer/source/tools/emfpcustomlinecap
    drawinglayer/source/tools/wmfemfhelper
    drawinglayer/source/tools/primitive2dxmldump
    drawinglayer/source/drawinglayeruno/xprimitive2drenderer
    drawinglayer/source/texture/texture
    drawinglayer/source/dumper/XShapeDumper
    drawinglayer/source/dumper/EnhancedShapeDumper
)

librestudio_expand_cxx(DRAWINGLAYER_SOURCES)
add_library(drawinglayer SHARED ${DRAWINGLAYER_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(drawinglayer PRIVATE
        "${CMAKE_SOURCE_DIR}/drawinglayer/inc/pch/precompiled_drawinglayer.hxx")
    target_compile_definitions(drawinglayer PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(drawinglayer PRIVATE
    "${CMAKE_SOURCE_DIR}/drawinglayer/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(drawinglayer officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(drawinglayer PRIVATE DRAWINGLAYER_DLLIMPLEMENTATION)
target_link_libraries(drawinglayer PRIVATE
    basegfx canvastools comphelper cppcanvas cppu cppuhelper drawinglayercore
    i18nlangtag sal salhelper svl svt tl vcl
    Boost::boost
    LibXml2::LibXml2
    PkgConfig::CAIRO)

# Library_drawinglayer.mk: gb_Library_use_system_win32_libs(drawinglayer,
# d2d1 dxguid) -- an unconditional call, but d2d1/dxguid (Direct2D) are
# meaningless outside Windows, same reasoning as ucbhelper's Winhttp case --
# plus the separate ifeq ($(OS),WNT) gb_Library_use_system_win32_libs(
# drawinglayer, gdi32) block, which resolves to the same WIN32 condition in
# practice. Kept structurally present, behind if (WIN32), per this port's
# policy of not silently dropping Windows-only gbuild targets -- see
# sal/CMakeLists.txt's sal_comtools for the same pattern. Inert on this
# Linux build.
if (WIN32)
    target_link_libraries(drawinglayer PRIVATE d2d1 dxguid gdi32)
endif ()

librestudio_add_component(drawinglayer
    COMPONENT "${CMAKE_SOURCE_DIR}/drawinglayer/drawinglayer.component"
    LIBRARY   drawinglayer
    LAYER     OOO)

librestudio_set_library_output_name(drawinglayer drawinglayer OOO)
install(TARGETS drawinglayer LIBRARY DESTINATION program)

# --- CppunitTest ---------------------------------------------------------------
#
# 2 CppunitTest_*.mk files exist for drawinglayer (CppunitTest_drawinglayer_
# processors.mk, CppunitTest_drawinglayer_border.mk); both are deferred to
# drawinglayer/CppunitTest_deferred.cmake, NOT wired here, include()'d from
# the root CMakeLists.txt only after ucb (both tests' use_components need
# ucb1/ucpfile1; drawinglayer_processors additionally needs framework's fwk)
# is add_subdirectory()'d -- configmgr's and i18npool's components, which
# both tests also need, ARE already available at drawinglayer's own position
# here, but ucb/framework are not. Defining either here would hit
# librestudio_add_cppunit_test()'s COMPONENTS lookup (a GLOBAL property read
# eagerly via get_property(GLOBAL ...)) before those modules have run, and
# abort configure with its own FATAL_ERROR -- the same shape of problem
# comphelper_test/sax_parser/svl_urihelper/etc. already hit (see svl/
# CppunitTest_deferred.cmake's header comment, the closest precedent). See
# drawinglayer/CppunitTest_deferred.cmake for the full per-test detail,
# including the canvas::vclcanvas/canvas::Factory components
# drawinglayer_processors needs but this port has not ported yet (flagged
# there, not invented), and its include() call site in the root
# CMakeLists.txt for why it sits where it does.

# drawinglayer/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/drawinglayer/CppunitTest_deferred.cmake")
