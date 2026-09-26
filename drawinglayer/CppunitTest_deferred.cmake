# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# drawinglayer_processors (CppunitTest_drawinglayer_processors.mk) and
# drawinglayer_border (CppunitTest_drawinglayer_border.mk) -- split out of
# drawinglayer/CMakeLists.txt into their own file, include()'d from the root
# CMakeLists.txt only after every module either one's COMPONENTS reference
# has already had its own add_subdirectory(); see that include() call site
# for the exact ordering rationale.
#
# Both tests' use_components need configmgr/source/configmgr and
# i18npool/util/i18npool -- both already add_subdirectory()'d well before
# drawinglayer's own position (configmgr at CMakeLists.txt line 486,
# i18npool at line 401, drawinglayer itself at line 501) -- but also
# ucb/source/core/ucb1 and ucb/source/ucp/file/ucpfile1, and
# drawinglayer_processors additionally framework/util/fwk, none of which are
# add_subdirectory()'d until well after drawinglayer (framework at line 517,
# ucb at line 592) -- ucb is the later-binding constraint of the two, so
# both tests are registered here, together, right after it. Defining them at
# drawinglayer's own natural position would hit configmgr_unit's/sax's/etc.
# already-established FATAL_ERROR (the referenced component's GLOBAL
# property not set yet) -- reordering ucb's/framework's own add_subdirectory()
# instead would be the much larger, riskier change (see the root
# CMakeLists.txt's own comment on framework's/ucb's tier placement). Same
# deferral pattern as comphelper/CppunitTest_deferred.cmake, sax/
# CppunitTest_deferred.cmake, configmgr/CppunitTest_deferred.cmake and svl/
# CppunitTest_deferred.cmake.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not drawinglayer's, nothing drawinglayer/CMakeLists.txt set up that
# is directory-scoped rather than global needs to be re-derived here --
# neither test references PkgConfig::CAIRO or anything else directory-scoped
# from there.
#
# LIBRARIES note common to both: gbuild's "test" (Library_test.mk) is this
# port's "test_fixtures", not "test" -- test/CMakeLists.txt's own add_library
# call (and its comment on why) renames it because CMake's enable_testing()
# reserves the target name "test" for CTest's own built-in target. Every
# other name below (basegfx, comphelper, cppcanvas, cppu, cppuhelper, sal,
# salhelper, drawinglayer, drawinglayercore, vcl, unotest, svt, tl, utl) is
# unchanged from the .mk's own spelling, each verified directly against its
# owning module's add_library() call.

# --- drawinglayer_processors -----------------------------------------------
#
# CppunitTest_drawinglayer_processors.mk, re-checked directly against the
# live file: 3 sources (drawinglayer/qa/unit/strokeattribute,
# .../vclmetafileprocessor2d, .../vclpixelprocessor2d), use_api(offapi
# udkapi), use_libraries basegfx comphelper $(if $(ENABLE_WASM_STRIP_CANVAS),
# ,cppcanvas) cppu cppuhelper sal salhelper drawinglayer drawinglayercore vcl
# test unotest svt tl utl, use_externals(boost_headers), use_ure + use_vcl
# (3 protector pairs), use_components (7 names, see below), use_configuration
# -> CONFIGURATION. ENABLE_WASM_STRIP_CANVAS is not defined anywhere in this
# port (grepped for it project-wide; matches drawinglayer/CMakeLists.txt's
# own header note that it "is not defined in this build, matching upstream
# default"), so cppcanvas is linked unconditionally, same as production
# drawinglayer already does.
#
# use_components lookup, each checked against the owning module's own
# librestudio_add_component() call rather than assumed from the .mk's own
# gbuild-path spelling:
#   - "configmgr/source/configmgr" -> "configmgr" (configmgr/CMakeLists.txt).
#   - "i18npool/util/i18npool" -> "i18npool" (i18npool/CMakeLists.txt).
#   - "ucb/source/core/ucb1" -> "ucb1", "ucb/source/ucp/file/ucpfile1" ->
#     "ucpfile1" (both ucb/CMakeLists.txt).
#   - "framework/util/fwk" -> "fwk" (framework/CMakeLists.txt).
#   All five (configmgr, i18npool, ucb1, ucpfile1, fwk) resolve to real,
#   already-registered components.
#   - "canvas/source/vcl/vclcanvas" and "canvas/source/factory/canvasfactory"
#     do NOT resolve -- FLAGGED, not invented. canvas/CMakeLists.txt only
#     ports canvastools (canvas/source/tools/*); its own header comment says
#     outright that Library_canvasfactory.mk and Library_vclcanvas.mk
#     (the canvas::Factory service and the VCL canvas back end) are "not
#     needed by anything ported so far and are not ported yet" -- that was
#     true until now, since drawinglayer_processors is the first thing in
#     this port to actually need them. No librestudio_add_component() call
#     for either exists anywhere in the tree (grepped project-wide), so
#     COMPONENTS below omits both rather than inventing lookup keys that
#     would just hit librestudio_add_cppunit_test()'s own FATAL_ERROR at
#     configure time. Concretely, this leaves a real runtime gap: qa/unit/
#     vclmetafileprocessor2d.cxx's setupCanvas() calls
#     mVclDevice->GetCanvas(), which (via canvas::vclcanvas + the
#     canvas::Factory UNO service canvas::vclcanvas registers itself
#     through) needs those two components in UNO_SERVICES to hand back a
#     working rendering::XCanvas -- without them it will most likely return
#     an empty reference at run time and the "draw the metafile using
#     canvas" assertions later in that test case will fail. strokeattribute
#     and vclpixelprocessor2d do not touch canvas at all (grepped both .cxx
#     files for "canvas"/"createInstance"/"rendering::", no hits), so only
#     part of one of the three source files is affected. This is a
#     pre-existing porting gap (canvas's own back-end libraries being
#     unported), not something to work around here; a later wave that ports
#     Library_vclcanvas.mk/Library_canvasfactory.mk into canvas/
#     CMakeLists.txt should add "vclcanvas"/"canvasfactory" (or whatever
#     names it registers them under) to this test's COMPONENTS list.
librestudio_add_cppunit_test(drawinglayer_processors
    SOURCES
        drawinglayer/qa/unit/strokeattribute
        drawinglayer/qa/unit/vclmetafileprocessor2d
        drawinglayer/qa/unit/vclpixelprocessor2d
    LIBRARIES basegfx comphelper cppcanvas cppu cppuhelper sal salhelper
        drawinglayer drawinglayercore vcl test_fixtures unotest svt tl utl
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
        configmgr i18npool ucb1 ucpfile1 fwk
    CONFIGURATION)

# --- drawinglayer_border -----------------------------------------------------
#
# CppunitTest_drawinglayer_border.mk, re-checked directly against the live
# file: 1 source (drawinglayer/qa/unit/border), use_api(offapi udkapi),
# use_libraries basegfx cppu cppuhelper sal salhelper drawinglayer
# drawinglayercore vcl test unotest svt tl (no comphelper, no cppcanvas, no
# utl -- this test does not touch canvas at all, confirmed by grepping qa/
# unit/border.cxx for "canvas", no hits, so the COMPONENTS gap above does not
# apply here), use_externals(boost_headers), use_ure + use_vcl (same 3
# protector pairs as drawinglayer_processors), use_components (4 names, all
# of which resolve -- configmgr/i18npool/ucb1/ucpfile1, same lookup as
# drawinglayer_processors above, no framework/canvas references here),
# use_configuration -> CONFIGURATION.
#
# Upstream registers this test via gb_Module_add_slowcheck_targets
# (Module_drawinglayer.mk), not gb_Module_add_check_targets like
# drawinglayer_processors -- upstream's "slowcheck" tier is a separate, more
# expensive opt-in test category not run by plain `make check`. This port's
# librestudio_add_cppunit_test() has no equivalent tiering concept, and
# nothing else registered so far in this tree uses
# set_tests_properties(... PROPERTIES LABELS ...) or any other CTest-level
# mechanism to reproduce a check/slowcheck-style split (grepped project-wide
# for both), so this is a plain comment, not new labeling infrastructure:
# drawinglayer_border becomes a normal CTest test like every other test here,
# functionally identical to drawinglayer_processors, but it corresponds to
# upstream's slower/opt-in tier -- worth knowing for anyone later wanting to
# filter CTest runs by that distinction (e.g. by retrofitting a CTest LABEL
# across every slowcheck-tier test once one actually needs to be excluded
# from a default run).
librestudio_add_cppunit_test(drawinglayer_border
    SOURCES
        drawinglayer/qa/unit/border
    LIBRARIES basegfx cppu cppuhelper sal salhelper drawinglayer
        drawinglayercore vcl test_fixtures unotest svt tl
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
        configmgr i18npool ucb1 ucpfile1
    CONFIGURATION)

# vim: set noet sw=4 ts=4:
