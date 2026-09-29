# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# cppcanvas's 2 CppunitTest_*.mk files (CppunitTest_cppcanvas_test.mk,
# CppunitTest_cppcanvas_emfplus.mk), both split out here rather than wired at
# cppcanvas/CMakeLists.txt's own add_subdirectory() position (root
# CMakeLists.txt line ~842) because their COMPONENTS lists each name at least
# one component whose owning module is add_subdirectory()'d well after
# cppcanvas -- see the per-test detail below for exactly which. Include this
# file from the root CMakeLists.txt only after add_subdirectory(sd) has run
# (the latest-binding constraint, from cppcanvas_emfplus's "sd"/"sdd"
# COMPONENTS) -- e.g. alongside the existing cluster of *_dialogs_test/other
# deferred-test include() calls already sitting well after that point (root
# CMakeLists.txt lines ~1231-1389 all postdate add_subdirectory(sd) at 1150).
#
# Both tests' Module_cppcanvas.mk gating was re-checked directly:
#   $(eval $(call gb_Module_add_check_targets,cppcanvas,\
#       CppunitTest_cppcanvas_test \
#   ))
#   ifeq ($(ENABLE_CAIRO_CANVAS),TRUE)
#   ifneq ($(DISPLAY),)
#   $(eval $(call gb_Module_add_slowcheck_targets,cppcanvas,\
#       CppunitTest_cppcanvas_emfplus \
#   ))
#   endif
#   endif
# cppcanvas_test is unconditional. cppcanvas_emfplus is gated on
# ENABLE_CAIRO_CANVAS (a real switch here: cmake/LibreStudioConfigHeaders.cmake
# sets it to 1 unconditionally) and $(DISPLAY) being non-empty -- a gbuild
# *configure-time* environment probe with no equivalent anywhere in this
# CMake port (nothing here re-evaluates a build-time DISPLAY the way gbuild
# does), the same posture this port already takes for NONHEADLESS tests that
# need a real X11 backend at *run* time regardless (vcl_dialogs_test and
# friends) -- so cppcanvas_emfplus is registered unconditionally too, same as
# every other CppunitTest in this port.

# --- cppcanvas_test (CppunitTest_cppcanvas_test.mk) ---------------------------
#
# Re-checked directly against the live file: 1 source (cppcanvas/qa/unit/
# test), use_external(boost_headers), use_libraries cppcanvas basegfx
# canvastools comphelper cppu cppuhelper i18nlangtag sal svt test tl unotest
# vcl ("test" -> test_fixtures, this port's established rename; all others
# re-verified as real add_library() targets), use_sdk_api (normal
# udkapi+offapi), use_ure + use_vcl (headless/SVP variant, NOT use_vcl_non_
# headless[_with_windows]) -> 3 protector pairs, no NONHEADLESS (use_vcl's
# plain form never makes vclplug_gen a build dependency -- solenv/gbuild/
# CppunitTest.mk:286-289's $(true) argument to gb_CppunitTest__use_vcl takes
# the "no real backend needed" branch at line 265; only the _non_headless
# variants take the other branch), use_configuration -> CONFIGURATION,
# use_components (7 names, some $(if $(ENABLE_CAIRO_CANVAS),...)-gated but
# ENABLE_CAIRO_CANVAS is 1 here so unconditional in practice):
#   - "canvas/source/vcl/vclcanvas" and "canvas/source/factory/canvasfactory"
#     do NOT resolve -- FLAGGED, not invented, same finding drawinglayer_
#     processors (drawinglayer/CppunitTest_deferred.cmake) already made for
#     the identical pair: canvas/CMakeLists.txt only ports canvastools; its
#     own header comment says outright that Library_canvasfactory.mk and
#     Library_vclcanvas.mk are "not needed by anything ported so far and are
#     not ported yet". No librestudio_add_component() call for either exists
#     anywhere in the tree (grepped project-wide). canvas is not one of this
#     wave's 5 assigned modules, so porting either here is out of scope;
#     COMPONENTS below omits both rather than inventing lookup keys that
#     would hit librestudio_add_cppunit_test()'s own FATAL_ERROR. Concretely:
#     cppcanvas/qa/unit/test.cxx exercises cppcanvas::VCLFactory, which needs
#     a working canvas::vclcanvas-backed rendering::XCanvas from the
#     canvas::Factory UNO service to render into -- without these two
#     components in UNO_SERVICES, those code paths will most likely fail at
#     run time rather than at build time. A later wave that ports Library_
#     vclcanvas.mk/Library_canvasfactory.mk into canvas/CMakeLists.txt should
#     add their registered names here.
#   - "configmgr/source/configmgr" -> "configmgr" (configmgr/CMakeLists.txt).
#   - "i18npool/util/i18npool" -> "i18npool" (i18npool/CMakeLists.txt).
#   - "linguistic/source/lng" -> "lng" (linguistic/CMakeLists.txt).
#   - "package/util/package2" -> "package2" (package/CMakeLists.txt).
#   - "sax/source/expatwrap/expwrap" -> "sax" (sax/CMakeLists.txt's own
#     librestudio_add_component(sax ...) call -- same resolution already
#     established by configmgr/CppunitTest_deferred.cmake and others).
#   - "ucb/source/core/ucb1" -> "ucb1" (ucb/CMakeLists.txt).
#   - "unoxml/source/service/unoxml" -> "unoxml" (unoxml/CMakeLists.txt).
#   All six of those resolve to real, already-registered components; the
#   latest-binding one for ordering purposes is ucb1 (ucb/CMakeLists.txt,
#   add_subdirectory() line 943), superseded below by cppcanvas_emfplus's own
#   "sd"/"sdd" requirement (line 1150) as this file's binding constraint.
librestudio_add_cppunit_test(cppcanvas_test
    SOURCES
        cppcanvas/qa/unit/test
    LIBRARIES
        cppcanvas basegfx canvastools comphelper cppu cppuhelper i18nlangtag
        sal svt test_fixtures tl unotest vcl Boost::boost
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool lng package2 sax ucb1 unoxml
    CONFIGURATION)

# --- cppcanvas_emfplus (CppunitTest_cppcanvas_emfplus.mk) ---------------------
#
# Re-checked directly against the live file: 1 source (cppcanvas/qa/extras/
# emfplus/emfplus), use_libraries comphelper cppu cppuhelper sal sfx
# subsequenttest test tl unotest utl vcl ("test" -> test_fixtures), use_
# externals(boost_headers, libxml2), use_sdk_api (normal udkapi+offapi),
# use_ure + use_vcl_non_headless (NOT the _with_windows variant) -> 3
# protector pairs + NONHEADLESS. In this port's model the two variants are
# equivalent: cmake/LibreStudioTest.cmake's own header comment already
# establishes that nothing here ever emits "--headless"/"SAL_USE_VCLPLUGIN=
# svp" in the first place (re-verified against solenv/gbuild/CppunitTest.mk:
# 291-302 -- the only difference between use_vcl_non_headless and use_vcl_
# non_headless_with_windows is the latter's extra `HEADLESS :=` line, clearing
# a flag this port's cppunittester wrapper never sets to begin with), so
# NONHEADLESS (real vclplug_gen dependency + explicit SAL_USE_VCLPLUGIN=gen)
# covers both identically -- same reasoning already applied for reportdesign_
# dialogs_test/basctl_dialogs_test/etc.'s own _with_windows calls, just
# confirming here that the plain variant needs nothing different. No use_rdb
# call anywhere in this .mk (re-checked -- unlike uui_dialogs_test/fpicker_
# dialogs_test/embeddedobj_general, this test needs no program_services_rdb
# DEPENDS), use_configuration -> CONFIGURATION, use_components (20 names):
#   - "canvas/source/cairo/cairocanvas" and "canvas/source/factory/
#     canvasfactory" do NOT resolve -- same canvas-module gap as cppcanvas_
#     test above (canvas/CMakeLists.txt: Library_cairocanvas.mk/Library_
#     canvasfactory.mk "not needed by anything ported so far and are not
#     ported yet"; canvas is out of this wave's 5-module scope). Omitted,
#     documented, not invented.
#   - "extensions/source/scanner/scn" does NOT resolve -- extensions/
#     CMakeLists.txt's own header comment documents Library_scn as out of
#     scope on this machine (needs the "sane_headers" external -- SANE
#     scanner API headers -- and no sane pkg-config/headers are present).
#     extensions is also not one of this wave's 5 assigned modules. Omitted,
#     documented, not invented.
#   - "cppcanvas/source/uno/mtfrenderer" -> "mtfrenderer" (cppcanvas/
#     CMakeLists.txt, ported for real this session -- see that file's own
#     "--- mtfrenderer ---" block, added specifically because this test is
#     the first thing in this port to actually need it, and it is a small,
#     self-contained, in-scope addition, unlike the three canvas/extensions
#     gaps above).
#   - "configmgr/source/configmgr" -> "configmgr".
#   - "emfio/emfio" -> "emfio" (emfio/CMakeLists.txt).
#   - "filter/source/config/cache/filterconfig1" -> "filterconfig" (filter/
#     CMakeLists.txt: librestudio_add_component(filterconfig COMPONENT
#     ".../filterconfig1.component" ...) -- the .component filename keeps the
#     gbuild "1" suffix, the registered lookup name does not).
#   - "framework/util/fwk" -> "fwk" (framework/CMakeLists.txt).
#   - "i18npool/util/i18npool" -> "i18npool".
#   - "package/util/package2" -> "package2".
#   - "sax/source/expatwrap/expwrap" -> "sax".
#   - "sfx2/util/sfx" -> "sfx" (sfx2/CMakeLists.txt).
#   - "sd/util/sd" -> "sd", "sd/util/sdd" -> "sdd" (sd/CMakeLists.txt -- the
#     latest-binding pair: add_subdirectory(sd) is root CMakeLists.txt line
#     1150, after every other module named here, which is why this whole
#     file's include() must sit after that line).
#   - "svl/source/fsstor/fsstorage" -> "fsstorage" (svl/CMakeLists.txt).
#   - "toolkit/util/tk" -> "tk" (toolkit/CMakeLists.txt).
#   - "vcl/vcl.common" -> "vcl" (vcl/CMakeLists.txt: librestudio_add_component
#     (vcl COMPONENT ".../vcl.common.component" ...) -- the component *file*
#     is vcl.common.component, the registered lookup name is plain "vcl").
#   - "ucb/source/core/ucb1" -> "ucb1", "ucb/source/ucp/file/ucpfile1" ->
#     "ucpfile1" (both ucb/CMakeLists.txt).
#   - "unoxml/source/service/unoxml" -> "unoxml".
#   - "uui/util/uui" -> "uui" (uui/CMakeLists.txt).
#   - "svtools/util/svt" -> "svt" (svtools/CMakeLists.txt).
#   17 of the 20 named components resolve to real, already-registered
#   components; the 3 that don't (cairocanvas, canvasfactory, scn) are
#   omitted and documented above, not invented -- consistent with this port's
#   librestudio_add_cppunit_test() COMPONENTS contract (a name with no
#   registered component FATAL_ERRORs at configure time) and with the
#   drawinglayer_processors precedent for the identical kind of gap.
librestudio_add_cppunit_test(cppcanvas_emfplus
    SOURCES
        cppcanvas/qa/extras/emfplus/emfplus
    LIBRARIES
        comphelper cppu cppuhelper sal sfx subsequenttest test_fixtures tl
        unotest utl vcl Boost::boost LibXml2::LibXml2
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${LIBXML2_INCLUDE_DIRS}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    NONHEADLESS
    COMPONENTS
        mtfrenderer configmgr emfio filterconfig fwk i18npool package2 sax
        sfx sd sdd fsstorage tk vcl ucb1 ucpfile1 unoxml uui svt
    CONFIGURATION)
