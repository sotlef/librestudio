# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# svgio (module svgio) — the SVG import filter/renderer: parses SVG XML into a
# drawinglayer primitive tree via the com.sun.star.graphic.SvgTools UNO
# service. Source list from Library_svgio.mk. No *_DLLIMPLEMENTATION define is
# declared there (svgio exports no dllexport-guarded symbols of its own), so
# none is set here either.

set(SVGIO_SOURCES
    svgio/source/svgreader/svgcharacternode
    svgio/source/svgreader/svgcirclenode
    svgio/source/svgreader/svgclippathnode
    svgio/source/svgreader/svgdocument
    svgio/source/svgreader/svgdocumenthandler
    svgio/source/svgreader/svgellipsenode
    svgio/source/svgreader/svggnode
    svgio/source/svgreader/svganode
    svgio/source/svgreader/svgfeblendnode
    svgio/source/svgreader/svgfecolormatrixnode
    svgio/source/svgreader/svgfecompositenode
    svgio/source/svgreader/svgfedropshadownode
    svgio/source/svgreader/svgfefloodnode
    svgio/source/svgreader/svgfeimagenode
    svgio/source/svgreader/svgfegaussianblurnode
    svgio/source/svgreader/svgfemergenode
    svgio/source/svgreader/svgfemergenodenode
    svgio/source/svgreader/svgfeoffsetnode
    svgio/source/svgreader/svgfilternode
    svgio/source/svgreader/svggradientnode
    svgio/source/svgreader/svggradientstopnode
    svgio/source/svgreader/svgimagenode
    svgio/source/svgreader/svglinenode
    svgio/source/svgreader/svgmarkernode
    svgio/source/svgreader/svgmasknode
    svgio/source/svgreader/svgnode
    svgio/source/svgreader/SvgNumber
    svgio/source/svgreader/svgpathnode
    svgio/source/svgreader/svgpatternnode
    svgio/source/svgreader/svgpolynode
    svgio/source/svgreader/svgrectnode
    svgio/source/svgreader/svgstyleattributes
    svgio/source/svgreader/svgstylenode
    svgio/source/svgreader/svgsvgnode
    svgio/source/svgreader/svgswitchnode
    svgio/source/svgreader/svgsymbolnode
    svgio/source/svgreader/svgtextnode
    svgio/source/svgreader/svgtextposition
    svgio/source/svgreader/svgtitledescnode
    svgio/source/svgreader/svgtoken
    svgio/source/svgreader/svgtrefnode
    svgio/source/svgreader/svgtools
    svgio/source/svgreader/svgtextpathnode
    svgio/source/svgreader/svgtspannode
    svgio/source/svgreader/svgusenode
    svgio/source/svgreader/svgvisitor
    svgio/source/svguno/xsvgparser
)

librestudio_expand_cxx(SVGIO_SOURCES)

# Renamed from the plain "svgio" this library's own gbuild leaf name would
# otherwise give it: CppunitTest_svgio.mk's gb_CppunitTest_CppunitTest(svgio)
# also needs the literal CMake target name "svgio" (librestudio_add_
# cppunit_test(svgio ...), svgio/CppunitTest_deferred.cmake), and gbuild's
# two separate namespaces (gb_Library_get_target vs. gb_CppunitTest_get_
# target) collapse into CMake's single flat one -- the exact same collision,
# and the exact same fix, smoketest/CMakeLists.txt's own header comment
# documents for Library_smoketest vs. CppunitTest_smoketest ("renaming *this*
# side of the collision is the one with zero fallout"). Grepped project-wide
# before choosing which side to rename: no CMakeLists.txt/*.cmake anywhere
# outside svgio/ itself calls target_link_libraries(...,svgio,...) or
# add_dependencies(...,svgio,...) -- svgio's own UNO service (com.sun.star.
# graphic.SvgTools) is only ever reached through the ComponentContext/
# UNO_SERVICES machinery its svgio.component file registers, dlopened at
# runtime, never linked at build time by any other module -- so renaming the
# *library* target (to "svgio_lib") rather than the CppunitTest is the
# zero-fallout side here, matching smoketest_lib's own precedent exactly.
# OUTPUT_NAME below still resolves to "svgio" (librestudio_set_library_
# output_name's second argument, unrelated to the CMake target's own name),
# so the actual installed/built .so filename (libsvgiolo.so) and the
# librestudio_add_component() registration below (whose own "svgio" first
# argument is that call's own component-registry key, not a CMake target
# name, and is left unchanged) are both completely unaffected by this
# rename -- only the internal CMake target identity changes.
add_library(svgio_lib SHARED ${SVGIO_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(svgio_lib PRIVATE
        "${CMAKE_SOURCE_DIR}/svgio/inc/pch/precompiled_svgio.hxx")
    target_compile_definitions(svgio_lib PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(svgio_lib PRIVATE
    "${CMAKE_SOURCE_DIR}/svgio/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}")
add_dependencies(svgio_lib officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(svgio_lib PRIVATE
    basegfx comphelper cppu cppuhelper drawinglayercore drawinglayer
    i18nlangtag sal salhelper sax svt tk tl utl vcl
    Boost::boost)

librestudio_add_component(svgio
    COMPONENT "${CMAKE_SOURCE_DIR}/svgio/svgio.component"
    LIBRARY   svgio_lib
    LAYER     OOO)

librestudio_set_library_output_name(svgio_lib svgio OOO)

install(TARGETS svgio_lib LIBRARY DESTINATION program)

# --- CppunitTest_svgio_tools --------------------------------------------------
#
# CppunitTest_svgio_tools.mk, re-checked directly against the live file: 1
# source (svgio/qa/cppunit/SvgNumberTest), use_externals(boost_headers) only
# -- no use_sdk_api anywhere in this .mk, so no udkapi/offapi INCLUDE/DEPENDS
# is needed here (unlike svgio/svgio_read below); set_include is just
# "$$(INCLUDE) -I$(SRCDIR)/svgio/inc", i.e. nothing beyond the default global
# include dirs (cmake/LibreStudioPlatform.cmake's top-level include_
# directories()) plus svgio/inc itself. use_library_objects(svgio_tools,
# svgio) -> plain LIBRARIES svgio_lib (the CMake target above, renamed from
# the plain "svgio" its own comment explains): same reasoning as svx_
# removewhichrange's own use_library_objects(svx_removewhichrange,svxcore)
# call (see svx/CMakeLists.txt's comment on that test) -- this port sets no
# -fvisibility=hidden anywhere and svgio/CMakeLists.txt applies no
# -Wl,--version-script (no *.map file exists anywhere under svgio/), so
# whatever symbol SvgNumberTest.cxx needs from svgio (SvgNumber's own parsing
# helpers) is already reachable by linking the real svgio_lib shared library
# target, with no need to recompile svgio's ~44-file source list a second
# time into this one small test.
#
# No use_ure/use_vcl*/use_rdb/use_configuration/use_components/use_uiconfigs
# anywhere in this .mk either, so no PROTECTORS/CONFIGURATION/COMPONENTS/
# NONHEADLESS/UICONFIG, and critically no DEPENDS on anything add_
# subdirectory()'d after svgio's own position in the root CMakeLists.txt --
# wired directly here, no CppunitTest_deferred.cmake needed for this one test
# (unlike svgio/svgio_read, both deferred below -- see the DEFER call site
# comment at the end of this file for why those two differ).
librestudio_add_cppunit_test(svgio_tools
    SOURCES
        svgio/qa/cppunit/SvgNumberTest
    LIBRARIES
        svgio_lib basegfx drawinglayer drawinglayercore comphelper cppu cppuhelper
        i18nlangtag sal salhelper sax svt tk tl utl vcl
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/svgio/inc")

# --- CppunitTest_svgio / CppunitTest_svgio_read (deferred) --------------------
#
# svgio and svgio_read (CppunitTest_svgio.mk, CppunitTest_svgio_read.mk) both
# need data from modules add_subdirectory()'d LATER than svgio's own position
# (line ~1120) in the root CMakeLists.txt:
#   - svgio.mk's own gb_CppunitTest_use_components list names ucb/source/
#     core/ucb1 and ucb/source/ucp/file/ucpfile1 (this port's "ucb1"/
#     "ucpfile1", ucb/CMakeLists.txt) -- ucb is add_subdirectory()'d well
#     after svgio, so COMPONENTS' get_property(GLOBAL ...) lookup for those
#     two names would FATAL_ERROR here, right now.
#   - Both call gb_CppunitTest_use_configuration -> CONFIGURATION, which
#     requires LIBRESTUDIO_TEST_UNITTEST_DIR (only set once test/
#     CMakeLists.txt has run, also after svgio's own position).
#   - svgio_read.mk additionally calls gb_CppunitTest_use_rdb(svgio_read,
#     services) -> DEPENDS program_services_rdb, defined much later still,
#     right after librestudio_finalize_components(services) near the end of
#     the root CMakeLists.txt -- the same later-binding constraint every
#     other use_rdb(services) test in this port is already deferred to (see
#     e.g. svx/CppunitTest_deferred.cmake's or formula/CppunitTest_deferred.
#     cmake's own header comment for the identical reasoning).
# Both are therefore split into svgio/CppunitTest_deferred.cmake and
# scheduled with cmake_language(DEFER ...) below, the current, non-include()-
# in-root mechanism (see cmake/LibreStudioTest.cmake's header comment for the
# full mechanism and its two gotchas -- CMAKE_SOURCE_DIR, never CMAKE_
# CURRENT_SOURCE_DIR, in the path argument below, since DEFER's arguments are
# evaluated lazily in the DEFER DIRECTORY's own scope, not this one's).
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/svgio/CppunitTest_deferred.cmake")
