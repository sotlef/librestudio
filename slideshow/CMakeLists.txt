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

# Cached (already librestudio_expand_cxx()'d, absolute-path) copy of the
# source list, reused below by the "slideshow" CppunitTest's own
# gb_CppunitTest_use_library_objects(slideshow,slideshow) call -- see that
# target's own comment for why. Same CACHE INTERNAL pattern editeng/
# CMakeLists.txt's EDITENG_SOURCES already established for the identical
# use_library_objects shape (editeng_core).
set(SLIDESHOW_SOURCES "${SLIDESHOW_SOURCES}" CACHE INTERNAL
    "librestudio_expand_cxx()'d source list of the slideshow library, reused by CppunitTest_slideshow.mk's use_library_objects")

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

# --- CppunitTest_slideshow ---------------------------------------------------
#
# CppunitTest_slideshow.mk, re-checked directly against the live file: 4
# sources (slideshow/test/slidetest, testshape, testview, views),
# set_include (-I$(SRCDIR)/slideshow/source/inc + $$(INCLUDE)), use_externals
# (box2d), use_sdk_api (normal udkapi+offapi headers),
# use_library_objects(slideshow,slideshow) -- recompiles the whole slideshow
# library's own sources directly into the test binary rather than linking
# against the built slideshow.so (gb_CppunitTest__forward_to_Linktarget,
# solenv/gbuild/CppunitTest.mk:497) -- and use_libraries
# ($(gb_Helper_optional,AVMEDIA,avmedia)) basegfx canvastools comphelper
# cppcanvas cppu cppuhelper sal salhelper svl svt svx svxcore tl utl vcl.
#
# No use_ure / use_vcl / use_rdb / use_components / use_configuration
# anywhere in this .mk -- unlike slideshow_engine below, this test never
# bootstraps a UNO ComponentContext, so it needs none of
# librestudio_add_cppunit_test()'s PROTECTORS/COMPONENTS/CONFIGURATION
# machinery, and (unlike slideshow_engine) has no ordering dependency on
# program_services_rdb either -- it can be registered right here, at
# slideshow's own natural position, no deferred file needed.
#
# SLIDESHOW_SOURCES is the cached, already-librestudio_expand_cxx()'d list
# set above; the 4 qa/test sources below are plain module-relative paths,
# expanded the normal way by librestudio_add_cppunit_test()'s own SOURCES
# handling -- same combined-list shape as editeng_core's own
# EDITENG_SOURCES + qa/unit/*.
#
# Naming, a first-of-its-kind wrinkle in this port: upstream's own gbuild
# name for this CppunitTest is the bare "slideshow"
# (gb_CppunitTest_CppunitTest,slideshow) -- identical to Library_slideshow's
# own name -- because gbuild keeps CppunitTest and Library in two separate
# target namespaces (CppunitTest.mk's internal target is actually
# "CppunitTest_slideshow", never colliding with "Library_slideshow").
# librestudio_add_cppunit_test()'s <name> argument, by contrast, becomes the
# bare CMake target name directly (add_library(${name} MODULE ...),
# cmake/LibreStudioTest.cmake) -- CMake has one flat target namespace, so
# passing "slideshow" here would collide with add_library(slideshow SHARED
# ...) above and fail configure ("add_library cannot create target
# 'slideshow' because another target with that name already exists").
# Checked whether this port had already solved this exact shape (a
# CppunitTest whose gbuild name equals its own module's Library name): grep
# across every already-ported CppunitTest_*.mk turned up sax, svgio, store,
# basegfx and i18nutil as the only other .mk files with this same bare-name
# shape, and NONE of them are ported yet (each module's own CMakeLists.txt
# instead ports a *different*, separately-named CppunitTest_<name>_test.mk
# from the same module, e.g. basegfx_test/store_test/i18nutil_test are their
# own distinct upstream .mk files, not this one) -- so there is no existing
# precedent to follow here, this is the first time this port hits it. The
# fix, local to this file only: the CMake/CTest target is named
# "slideshow_test" instead (not otherwise used anywhere in this tree,
# grepped) -- clearly disambiguated, in the same spirit as this port's other
# forced boundary renames (e.g. test_fixtures for gbuild's bare "test",
# test/CMakeLists.txt's own comment) -- with OUTPUT_NAME force-corrected
# right after the call so the actual installed/dlopened file is still named
# "libtest_slideshow.so", matching what upstream's own gb_CppunitTest_
# get_filename would have produced for the name "slideshow" (no test source
# here hardcodes that path the way sal_osl_Module's Module_DLL test does --
# grepped slideshow/test/*.cxx for "libtest_"/"getDllURL", no hits -- so this
# is a faithfulness correction, not a functional requirement). One real,
# accepted deviation from upstream follows from the rename: `ctest -R
# slideshow_test` is the way to run this test in this port, not `ctest -R
# slideshow` (which is now unavailable, since only slideshow_engine below,
# and no test at all named bare "slideshow", exist in CTest's target list).
librestudio_add_cppunit_test(slideshow_test
    SOURCES
        ${SLIDESHOW_SOURCES}
        slideshow/test/slidetest
        slideshow/test/testshape
        slideshow/test/testview
        slideshow/test/views
    LIBRARIES
        basegfx canvastools comphelper cppcanvas cppu cppuhelper sal
        salhelper svl svt svx svxcore tl utl vcl
        Boost::boost box2d::box2d
    INCLUDE
        "${CMAKE_SOURCE_DIR}/slideshow/source/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEFINITIONS SLIDESHOW_DLLIMPLEMENTATION
    DEPENDS
        officecfg_headers
        udkapi_normal_headers offapi_normal_headers)
set_target_properties(slideshow_test PROPERTIES OUTPUT_NAME "test_slideshow")
if (ENABLE_AVMEDIA)
    target_link_libraries(slideshow_test PRIVATE avmedia)
endif ()

# slideshow/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/slideshow/CppunitTest_deferred.cmake")
