# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# basegfx — the 2D geometry library: points, rectangles, polygons, matrices,
# B-splines and the raster/vector converters built on them.  Everything that
# draws sits on it, but it does not depend on a toolkit, which is why it can
# be built here.
# Source list from Library_basegfx.mk (the precompiled header is not ported).

set(BASEGFX_SOURCES
    basegfx/source/color/bcolormodifier
    basegfx/source/color/bcolortools
    basegfx/source/curve/b2dbeziertools
    basegfx/source/curve/b2dcubicbezier
    basegfx/source/matrix/b2dhommatrix
    basegfx/source/matrix/b2dhommatrixtools
    basegfx/source/matrix/b3dhommatrix
    basegfx/source/matrix/b3dhommatrixtools
    basegfx/source/numeric/ftools
    basegfx/source/point/b2dpoint
    basegfx/source/point/b2ipoint
    basegfx/source/point/b3dpoint
    basegfx/source/polygon/b2dlinegeometry
    basegfx/source/polygon/b2dpolygon
    basegfx/source/polygon/b2dpolygonclipper
    basegfx/source/polygon/b2dpolygoncutandtouch
    basegfx/source/polygon/b2dpolygontools
    basegfx/source/polygon/b2dpolygontriangulator
    basegfx/source/polygon/b2dpolypolygon
    basegfx/source/polygon/b2dpolypolygoncutter
    basegfx/source/polygon/b2dpolypolygontools
    basegfx/source/polygon/b2dsvgpolypolygon
    basegfx/source/polygon/b2dtrapezoid
    basegfx/source/polygon/b3dpolygon
    basegfx/source/polygon/b3dpolygontools
    basegfx/source/polygon/b3dpolypolygon
    basegfx/source/polygon/b3dpolypolygontools
    basegfx/source/polygon/WaveLine
    basegfx/source/range/b2dpolyrange
    basegfx/source/range/b2drange
    basegfx/source/range/b2drangeclipper
    basegfx/source/range/b2xrange
    basegfx/source/range/b3drange
    basegfx/source/raster/rasterconvert3d
    basegfx/source/tools/b2dclipstate
    basegfx/source/tools/bgradient
    basegfx/source/tools/canvastools
    basegfx/source/tools/gradienttools
    basegfx/source/tools/keystoplerp
    basegfx/source/tools/numbertools
    basegfx/source/tools/stringconversiontools
    basegfx/source/tools/systemdependentdata
    basegfx/source/tools/tools
    basegfx/source/tools/unopolypolygon
    basegfx/source/tools/zoomtools
    basegfx/source/tuple/b2dtuple
    basegfx/source/tuple/b3dtuple
    basegfx/source/vector/b2dvector
    basegfx/source/vector/b2ivector
    basegfx/source/vector/b3dvector
)

librestudio_expand_cxx(BASEGFX_SOURCES)
add_library(basegfx SHARED ${BASEGFX_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(basegfx PRIVATE
        "${CMAKE_SOURCE_DIR}/basegfx/inc/pch/precompiled_basegfx.hxx")
    target_compile_definitions(basegfx PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(basegfx PRIVATE
    "${CMAKE_SOURCE_DIR}/basegfx/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(basegfx udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(basegfx PRIVATE BASEGFX_DLLIMPLEMENTATION)
target_link_libraries(basegfx PRIVATE comphelper cppu cppuhelper sal Boost::boost)

librestudio_set_library_output_name(basegfx basegfx OOO)
install(TARGETS basegfx LIBRARY DESTINATION program)

# --- CppunitTest targets -----------------------------------------------------

# basegfx's only CppunitTest, CppunitTest_basegfx.mk, ported via
# librestudio_add_cppunit_test() (cmake/LibreStudioTest.cmake -- read its
# header comment first). Re-checked directly against the live .mk file: it
# lists 24 source files, not the 23 the prior-research summary said (all 24
# were confirmed to exist under basegfx/test/), use_libraries basegfx
# comphelper cppu cppuhelper sal, and gb_CppunitTest_use_sdk_api only -- no
# use_ure/use_vcl*, so no PROTECTORS.
#
# gb_CppunitTest_use_sdk_api == gb_CppunitTest_use_api($1, udkapi offapi),
# the "normal" variant (same mapping o3tl/CMakeLists.txt's own
# CppunitTest_o3tl_tests section documents), matching the udkapi_normal_/
# offapi_normal_ headers add_library(basegfx ...) above already consumes --
# not use_udk_api (which would only pull in udkapi).
#
# Note the CppunitTest shares the plain name "basegfx" with the library
# target above -- gbuild's CppunitTest and Library namespaces are separate,
# but CMake's is not, so this would collide if the CppunitTest target were
# also literally named "basegfx" (same issue, same fix, as store/CMakeLists.
# txt's "store (CppunitTest)" section documents for CppunitTest_store.mk).
librestudio_add_cppunit_test(basegfx_test
    SOURCES
        basegfx/test/B2DRangeTest
        basegfx/test/B2IRangeTest
        basegfx/test/B2DPolyRangeTest
        basegfx/test/B2DHomMatrixTest
        basegfx/test/B2DPointTest
        basegfx/test/B2DPolygonTest
        basegfx/test/B2DPolygonToolsTest
        basegfx/test/B2DPolyPolygonTest
        basegfx/test/B2DPolyPolygonCutterTest
        basegfx/test/B2DSizeTest
        basegfx/test/B1DRangeTest
        basegfx/test/B2XRangeTest
        basegfx/test/B2IBoxTest
        basegfx/test/B2DTupleTest
        basegfx/test/B3DTupleTest
        basegfx/test/BColorModifierTest
        basegfx/test/BColorTest
        basegfx/test/SvgPathImportExport
        basegfx/test/basegfx3d
        basegfx/test/boxclipper
        basegfx/test/basegfxtools
        basegfx/test/clipstate
        basegfx/test/genericclipper
        basegfx/test/VectorTest
    LIBRARIES basegfx comphelper cppu cppuhelper sal
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers)
