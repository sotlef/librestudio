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

target_include_directories(basegfx PRIVATE
    "${CMAKE_SOURCE_DIR}/basegfx/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(basegfx udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(basegfx PRIVATE BASEGFX_DLLIMPLEMENTATION)
target_link_libraries(basegfx PRIVATE comphelper cppu cppuhelper sal Boost::boost)

librestudio_set_library_output_name(basegfx basegfx OOO)
install(TARGETS basegfx LIBRARY DESTINATION program)
