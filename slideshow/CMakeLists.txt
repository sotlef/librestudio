# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# slideshow (module slideshow) -- one library: slideshow, the Impress/Draw
# presentation playback engine (activities, animation nodes, transitions,
# box2d-based physics transitions). Source list, externals and dependencies
# are transcribed from Library_slideshow.mk.
#
# Not reproduced:
#  - gb_Library_set_precompiled_header,slideshow,... -- precompiled headers
#    are not ported anywhere in this build, same as every other module so
#    far.
#  - Library_OGLTrans / Package_opengl -- gated behind
#    $(eval $(call gb_Module_add_targets,slideshow,...)) under
#    ifeq ($(ENABLE_OPENGL_TRANSITIONS),TRUE) in Module_slideshow.mk;
#    ENABLE_OPENGL_TRANSITIONS is only referenced in config_host.mk.in and
#    configure.ac, never defined anywhere in this port, so this resolves the
#    same as every other unset ENABLE_* switch elsewhere in this port: out
#    of scope, not ported.
#  - CppunitTest_slideshow / CppunitTest_slideshow_engine -- tests, out of
#    scope.
#  - $(call gb_Helper_optional,AVMEDIA,avmedia) -- gated on ENABLE_AVMEDIA
#    below (default ON, cmake/LibreStudioConfigHeaders.cmake -- see
#    avmedia/CMakeLists.txt's own comment for the configure.ac evidence),
#    same established precedent as svx's own gb_Helper_optional(AVMEDIA,
#    avmedia) (Library_svx.mk, not Library_svxcore.mk's separate
#    unconditional avmedia link).

set(SLIDESHOW_SOURCES
    slideshow/source/engine/activities/activitiesfactory
    slideshow/source/engine/activities/activitybase
    slideshow/source/engine/activities/continuousactivitybase
    slideshow/source/engine/activities/continuouskeytimeactivitybase
    slideshow/source/engine/activities/discreteactivitybase
    slideshow/source/engine/activitiesqueue
    slideshow/source/engine/activities/simplecontinuousactivitybase
    slideshow/source/engine/animatedsprite
    slideshow/source/engine/animationfactory
    slideshow/source/engine/animationnodes/animationaudionode
    slideshow/source/engine/animationnodes/animationbasenode
    slideshow/source/engine/animationnodes/animationcolornode
    slideshow/source/engine/animationnodes/animationcommandnode
    slideshow/source/engine/animationnodes/animationnodefactory
    slideshow/source/engine/animationnodes/animationpathmotionnode
    slideshow/source/engine/animationnodes/animationsetnode
    slideshow/source/engine/animationnodes/animationphysicsnode
    slideshow/source/engine/animationnodes/animationtransformnode
    slideshow/source/engine/animationnodes/animationtransitionfilternode
    slideshow/source/engine/animationnodes/basecontainernode
    slideshow/source/engine/animationnodes/basenode
    slideshow/source/engine/animationnodes/generateevent
    slideshow/source/engine/animationnodes/nodetools
    slideshow/source/engine/animationnodes/paralleltimecontainer
    slideshow/source/engine/animationnodes/propertyanimationnode
    slideshow/source/engine/animationnodes/sequentialtimecontainer
    slideshow/source/engine/attributemap
    slideshow/source/engine/box2dtools
    slideshow/source/engine/color
    slideshow/source/engine/delayevent
    slideshow/source/engine/effectrewinder
    slideshow/source/engine/eventmultiplexer
    slideshow/source/engine/eventqueue
    slideshow/source/engine/expressionnodefactory
    slideshow/source/engine/rehearsetimingsactivity
    slideshow/source/engine/screenupdater
    slideshow/source/engine/shapeattributelayer
    slideshow/source/engine/shapes/appletshape
    slideshow/source/engine/shapes/backgroundshape
    slideshow/source/engine/shapes/drawinglayeranimation
    slideshow/source/engine/shapes/drawshape
    slideshow/source/engine/shapes/drawshapesubsetting
    slideshow/source/engine/shapes/externalshapebase
    slideshow/source/engine/shapes/gdimtftools
    slideshow/source/engine/shapes/intrinsicanimationactivity
    slideshow/source/engine/shapes/mediashape
    slideshow/source/engine/shapes/shapeimporter
    slideshow/source/engine/shapesubset
    slideshow/source/engine/shapes/viewappletshape
    slideshow/source/engine/shapes/viewbackgroundshape
    slideshow/source/engine/shapes/viewmediashape
    slideshow/source/engine/shapes/viewshape
    slideshow/source/engine/slidebitmap
    slideshow/source/engine/slide/layer
    slideshow/source/engine/slide/layermanager
    slideshow/source/engine/slide/shapemanagerimpl
    slideshow/source/engine/slideshowcontext
    slideshow/source/engine/slideshowimpl
    slideshow/source/engine/slide/slideanimations
    slideshow/source/engine/slide/slideimpl
    slideshow/source/engine/slide/targetpropertiescreator
    slideshow/source/engine/slide/userpaintoverlay
    slideshow/source/engine/slideview
    slideshow/source/engine/smilfunctionparser
    slideshow/source/engine/soundplayer
    slideshow/source/engine/tools
    slideshow/source/engine/transitions/barndoorwipe
    slideshow/source/engine/transitions/barwipepolypolygon
    slideshow/source/engine/transitions/boxwipe
    slideshow/source/engine/transitions/checkerboardwipe
    slideshow/source/engine/transitions/clippingfunctor
    slideshow/source/engine/transitions/clockwipe
    slideshow/source/engine/transitions/combtransition
    slideshow/source/engine/transitions/doublediamondwipe
    slideshow/source/engine/transitions/ellipsewipe
    slideshow/source/engine/transitions/fanwipe
    slideshow/source/engine/transitions/figurewipe
    slideshow/source/engine/transitions/fourboxwipe
    slideshow/source/engine/transitions/iriswipe
    slideshow/source/engine/transitions/parametricpolypolygonfactory
    slideshow/source/engine/transitions/pinwheelwipe
    slideshow/source/engine/transitions/randomwipe
    slideshow/source/engine/transitions/shapetransitionfactory
    slideshow/source/engine/transitions/slidechangebase
    slideshow/source/engine/transitions/slidetransitionfactory
    slideshow/source/engine/transitions/snakewipe
    slideshow/source/engine/transitions/spiralwipe
    slideshow/source/engine/transitions/sweepwipe
    slideshow/source/engine/transitions/transitionfactorytab
    slideshow/source/engine/transitions/transitiontools
    slideshow/source/engine/transitions/veewipe
    slideshow/source/engine/transitions/waterfallwipe
    slideshow/source/engine/transitions/zigzagwipe
    slideshow/source/engine/unoviewcontainer
    slideshow/source/engine/usereventqueue
    slideshow/source/engine/waitsymbol
    slideshow/source/engine/slideoverlaybutton
    slideshow/source/engine/pointersymbol
    slideshow/source/engine/wakeupevent
)

librestudio_expand_cxx(SLIDESHOW_SOURCES)
add_library(slideshow SHARED ${SLIDESHOW_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(slideshow PRIVATE
        "${CMAKE_SOURCE_DIR}/slideshow/inc/pch/precompiled_slideshow.hxx")
    target_compile_definitions(slideshow PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(slideshow PRIVATE
    "${CMAKE_SOURCE_DIR}/slideshow/source/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(slideshow officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(slideshow PRIVATE SLIDESHOW_DLLIMPLEMENTATION)

target_link_libraries(slideshow PRIVATE
    basegfx canvastools comphelper cppcanvas cppu cppuhelper sal
    salhelper svl svt svxcore tl utl vcl
    Boost::boost box2d::box2d)

# gb_Helper_optional(AVMEDIA, avmedia) in Library_slideshow.mk:31.
# ENABLE_AVMEDIA defaults ON.
if (ENABLE_AVMEDIA)
    target_link_libraries(slideshow PRIVATE avmedia)
endif ()

librestudio_add_component(slideshow
    COMPONENT "${CMAKE_SOURCE_DIR}/slideshow/util/slideshow.component"
    LIBRARY   slideshow
    LAYER     OOO)
librestudio_set_library_output_name(slideshow slideshow OOO)
install(TARGETS slideshow LIBRARY DESTINATION program)
