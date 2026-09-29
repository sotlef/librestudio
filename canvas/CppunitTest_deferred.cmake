# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# canvas_test (CppunitTest_canvas_test.mk) -- split out of canvas/
# CMakeLists.txt into its own file, include()'d from the root CMakeLists.txt
# only after every module/target it references has already run; see that
# include() call site for the exact ordering rationale.
#
# CppunitTest_canvas_test.mk, re-checked directly against the live file: 1
# source (canvas/qa/cppunit/canvastest), use_external(boost_headers),
# use_libraries(basegfx comphelper cppu cppuhelper sal test tl unotest vcl --
# "test" -> test_fixtures, this port's established rename -- note "canvas"/
# "canvastools" is NOT in this list: canvastest.cxx never links canvas code
# directly, it only calls VirtualDevice::GetCanvas(), which reaches the
# canvas::vclcanvas/canvas::Factory UNO services at run time -- see the
# COMPONENTS note below), use_sdk_api (normal udkapi+offapi), use_ure +
# use_vcl (3 protector pairs), use_components (4 names in the .mk, 2 resolve
# -- see below), use_configuration -> CONFIGURATION.
#
# Why this needs deferring past canvas's own natural add_subdirectory()
# position (CMakeLists.txt line ~841): CONFIGURATION requires
# LIBRESTUDIO_TEST_UNITTEST_DIR, a plain CMake variable only set inside
# test/CMakeLists.txt (CACHE INTERNAL, add_subdirectory()'d at line ~1018) --
# read immediately by librestudio_add_cppunit_test()'s own
# "if (NOT LIBRESTUDIO_TEST_UNITTEST_DIR) message(FATAL_ERROR ...)" check
# (cmake/LibreStudioTest.cmake), not deferred to CMake's generate step the
# way LIBRARIES/PROTECTORS forward references are (see sot/CMakeLists.txt's
# own sot_test_sot comment for a test that does NOT need deferral, and the
# contrast with this one). configmgr (834) and i18npool (730) are both
# already add_subdirectory()'d before canvas's own position, so COMPONENTS
# alone would not have forced this -- CONFIGURATION's LIBRESTUDIO_TEST_
# UNITTEST_DIR dependency is the sole reason, the identical shape
# drawinglayer/CppunitTest_deferred.cmake's and package/
# CppunitTest_deferred.cmake's own header comments already document (their
# fix-comment at the root CMakeLists.txt's own include() call site, "Fix
# (2026-09-27, first real cmake --build attempt)", describes the exact same
# mistake -- reasoning only about COMPONENTS ordering and missing
# CONFIGURATION's own separate requirement).
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not canvas's, source paths below are still fine:
# librestudio_expand_cxx (cmake/LibreStudioPlatform.cmake) resolves them
# against CMAKE_SOURCE_DIR (the absolute tree root), never
# CMAKE_CURRENT_SOURCE_DIR.
#
# COMPONENTS lookup, each checked against the owning module's own
# librestudio_add_component() call rather than assumed from the .mk's own
# gbuild-path spelling:
#   - "configmgr/source/configmgr" -> "configmgr" (configmgr/CMakeLists.txt).
#   - "i18npool/util/i18npool" -> "i18npool" (i18npool/CMakeLists.txt).
#   - "canvas/source/vcl/vclcanvas" and "canvas/source/factory/canvasfactory"
#     do NOT resolve -- FLAGGED, not invented, same real porting gap
#     drawinglayer_processors already hit and documented in detail
#     (drawinglayer/CppunitTest_deferred.cmake's own header comment,
#     re-checked directly before writing this one). canvas/CMakeLists.txt's
#     own header comment says outright that Library_canvasfactory.mk and
#     Library_vclcanvas.mk (the canvas::Factory service and the VCL canvas
#     back end) are "not needed by anything ported so far and are not ported
#     yet" -- still true as of this test. No librestudio_add_component() call
#     for either exists anywhere in the tree (grepped project-wide), so
#     COMPONENTS below omits both rather than inventing lookup keys that
#     would just hit librestudio_add_cppunit_test()'s own FATAL_ERROR at
#     configure time.
#
# Concrete runtime impact of that gap, worse here than for drawinglayer_
# processors: canvastest.cxx's setUp() itself does NOT call GetCanvas() (it
# only pre-computes two colour sequences), but every one of its 6 test
# methods (testDrawLine, testDrawBezier, testFillPolyPolygon,
# testFillTextureMappedPolyPolygon (if present), etc. -- grepped the live
# file's own CPPUNIT_TEST_SUITE list) calls setupCanvas() first, which calls
# mVclDevice->GetCanvas() and then CPPUNIT_ASSERT(mCanvas.is()) -- i.e. EVERY
# test case in this suite, not just part of one file like drawinglayer_
# processors, asserts that GetCanvas() returns a live canvas. Without
# canvas::vclcanvas/canvas::Factory registered in UNO_SERVICES, GetCanvas()
# most likely returns an empty reference and that first CPPUNIT_ASSERT in
# every test method fails immediately. This test is therefore expected to
# build successfully but FAIL every test case at run time until a later wave
# ports Library_vclcanvas.mk/Library_canvasfactory.mk into canvas/
# CMakeLists.txt and adds "vclcanvas"/"canvasfactory" (or whatever names it
# registers them under) to COMPONENTS below -- this is a pre-existing porting
# gap (canvas's own back-end libraries being unported), not something to work
# around here.
librestudio_add_cppunit_test(canvas_test
    SOURCES
        canvas/qa/cppunit/canvastest
    LIBRARIES
        basegfx comphelper cppu cppuhelper sal test_fixtures tl unotest vcl
        Boost::boost
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool
    CONFIGURATION)

# canvastest.cxx's CanvasTest needs SRC_ROOT/WORKDIR_FOR_BUILD
# (BootstrapFixture's Directories member) -- now handled generically by
# librestudio_add_cppunit_test() itself (cmake/LibreStudioTest.cmake).
