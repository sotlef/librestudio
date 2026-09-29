# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# libreofficekit (module libreofficekit) — the GTK tiled-rendering demo
# viewer and benchmark tool that exercise the LibreOfficeKit (LOK) C API.
#
# This is NOT the LOK API implementation: that is desktop's sofficeapp
# target (add_library(sofficeapp SHARED ...) in desktop/CMakeLists.txt).
# Everything below consumes sofficeapp exclusively at RUNTIME, via
# dlopen()/dlsym() — include/LibreOfficeKit/LibreOfficeKitInit.h's
# lok_dlopen() resolves libsofficeapp.so by a compiled-in LOK_PATH string
# and pulls "libreofficekit_hook"/"libreofficekit_hook_2" out of it with
# dlsym(). lokdocview.cxx and tilebench.cxx both #include
# LibreOfficeKitInit.h for exactly this. None of the three targets below
# link sofficeapp — do not "fix" that by adding it to a
# target_link_libraries() call here.
#
# Scope, from Module_libreofficekit.mk (Linux branch): ENABLE_GTK3 gates
# Library_libreofficekitgtk + Executable_gtktiledviewer; DISABLE_DYNLOADING
# (not tracked by this port, so always off) would gate out
# Executable_tilebench, which is otherwise unconditional.
# Package_selectionhandles and UIConfig_libreofficekit remain excluded
# (packaging, this port's established policy). CppunitTest_libreofficekit_
# checkapi and CppunitTest_libreofficekit_tiledrendering -- previously listed
# here too as excluded -- are now ported below (see the "--- CppunitTest
# targets ---" block at the end of this file); that old blanket "tests
# excluded" characterization is superseded for these two, following this
# session's own investigation.

if (ENABLE_GTK3)

# configure.ac's own GTK3 package set, re-declared locally the same way every
# other GTK3-consuming module in this port does (vcl/CMakeLists.txt is the
# precedent) even though vcl already declares its own copy — pkg_check_modules
# is scoped per directory. Shared by libreofficekitgtk and gtktiledviewer
# below.
pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET
    gtk+-3.0>=3.24 gmodule-no-export-2.0 glib-2.0>=2.38 atk>=2.36 cairo)

# ---------------------------------------------------------------------------
# libreofficekitgtk — Library_libreofficekitgtk.mk. GtkWidget wrapper
# (LOKDocView) around the LOK API, plus its tile cache (tilebuffer). Only
# use_sdk_api (udkapi + offapi) — unlike most libraries in this port it does
# not pull officecfg, and grepping both sources for "officecfg" confirms
# neither needs it. It also declares no gb_Library_use_libraries at all: the
# com/sun/star/awt/Key.hpp and vcl/event.hxx includes are used header-only
# (no vcl:: symbols are actually called), so there is nothing here to link
# beyond GTK3/boost/dl/m.
# ---------------------------------------------------------------------------

set(LIBREOFFICEKITGTK_SOURCES
    libreofficekit/source/gtk/lokdocview
    libreofficekit/source/gtk/tilebuffer
)

librestudio_expand_cxx(LIBREOFFICEKITGTK_SOURCES)
add_library(libreofficekitgtk SHARED ${LIBREOFFICEKITGTK_SOURCES})

target_include_directories(libreofficekitgtk PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(libreofficekitgtk udkapi_normal_headers offapi_normal_headers)

# LOK_PATH is where lok_dlopen() (LibreOfficeKitInit.h) looks for
# libsofficeapp.so when no path is passed explicitly (lokdocview.cxx's
# LOK_PATH uses, lines around the lok_init_2 calls). Upstream bakes in
# $(LIBDIR)/libreoffice/$(LIBO_LIB_FOLDER), the install-time program
# directory; this port's build-time equivalent is LIBRESTUDIO_PROGRAM_DIR
# (cmake/LibreStudioPlatform.cmake), which is also where sofficeapp itself
# lands. Quoting follows this port's established convention for a quoted
# string compile definition (sd/CMakeLists.txt's -DSDUI_DLL_NAME=\"...\").
target_compile_definitions(libreofficekitgtk PRIVATE
    -DLOK_PATH=\"${LIBRESTUDIO_PROGRAM_DIR}\"
    -DLOK_DOC_VIEW_IMPLEMENTATION)

target_link_libraries(libreofficekitgtk PRIVATE
    Boost::boost
    PkgConfig::GTK3
    m dl)

# No .component file: libreofficekitgtk is a plain GTK widget library, not a
# UNO service (confirmed: no *.component file anywhere under libreofficekit/).
librestudio_set_library_output_name(libreofficekitgtk libreofficekitgtk OOO)

install(TARGETS libreofficekitgtk LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# gtktiledviewer — Executable_gtktiledviewer.mk. The GTK demo viewer
# application built on top of libreofficekitgtk/LOKDocView.
# ---------------------------------------------------------------------------

# X11/XEXT/XRENDER plus libSM/libICE (found by name, no pkg-config file on
# this distribution): same precedent as vcl/CMakeLists.txt's own
# vclplug_gen/vclplug_gtk3 declarations.
pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)
pkg_check_modules(XEXT REQUIRED IMPORTED_TARGET xext)
pkg_check_modules(XRENDER REQUIRED IMPORTED_TARGET xrender)
find_library(SM_LIBRARY NAMES SM REQUIRED)
find_library(ICE_LIBRARY NAMES ICE REQUIRED)

set(GTKTILEDVIEWER_SOURCES
    libreofficekit/qa/gtktiledviewer/gtv-main
    libreofficekit/qa/gtktiledviewer/gtv-application
    libreofficekit/qa/gtktiledviewer/gtv-application-window
    libreofficekit/qa/gtktiledviewer/gtv-main-toolbar
    libreofficekit/qa/gtktiledviewer/gtv-signal-handlers
    libreofficekit/qa/gtktiledviewer/gtv-helpers
    libreofficekit/qa/gtktiledviewer/gtv-lokdocview-signal-handlers
    libreofficekit/qa/gtktiledviewer/gtv-calc-header-bar
    libreofficekit/qa/gtktiledviewer/gtv-comments-sidebar
    libreofficekit/qa/gtktiledviewer/gtv-lok-dialog
)

librestudio_expand_cxx(GTKTILEDVIEWER_SOURCES)
add_executable(gtktiledviewer ${GTKTILEDVIEWER_SOURCES})

# The .mk's set_include also carries -I$(SRCDIR)/desktop/inc, but none of
# these 10 sources has a quoted #include that resolves there (every quoted
# include here is another gtv-*.hxx in this same qa/gtktiledviewer/
# directory) — it is vestigial and dropped.
target_include_directories(gtktiledviewer PRIVATE
    "${CMAKE_SOURCE_DIR}/libreofficekit/qa/gtktiledviewer"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(gtktiledviewer udkapi_normal_headers offapi_normal_headers)

target_link_libraries(gtktiledviewer PRIVATE
    libreofficekitgtk
    Boost::boost
    PkgConfig::GTK3
    PkgConfig::X11 PkgConfig::XEXT PkgConfig::XRENDER
    "${SM_LIBRARY}" "${ICE_LIBRARY}"
    m dl)

install(TARGETS gtktiledviewer RUNTIME DESTINATION program)

endif ()

# ---------------------------------------------------------------------------
# tilebench — Executable_tilebench.mk. A standalone tile-rendering benchmark;
# not gated by ENABLE_GTK3. It links only sal and dlopens sofficeapp itself
# through the same LibreOfficeKitInit.h machinery as libreofficekitgtk above
# (LOK_PATH is not baked in here — the .mk adds no such define for this
# target, and the tool takes the install path as a command-line argument
# instead).
# ---------------------------------------------------------------------------

set(TILEBENCH_SOURCES
    libreofficekit/qa/tilebench/tilebench
)

librestudio_expand_cxx(TILEBENCH_SOURCES)
add_executable(tilebench ${TILEBENCH_SOURCES})

# Same vestigial -I desktop/inc as gtktiledviewer above: tilebench.cxx has no
# quoted #include at all, so there is nothing under desktop/inc for it to
# resolve; dropped.
#
# Unlike the two targets above, Executable_tilebench.mk has no
# gb_Executable_use_sdk_api call, and none of tilebench.cxx's includes are
# com/sun/star headers (LibreOfficeKit.hxx is a standalone wrapper, not
# UNO/IDL-generated), so no udkapi/offapi include dirs are added here.

target_link_libraries(tilebench PRIVATE
    sal
    Boost::boost
    m dl)

install(TARGETS tilebench RUNTIME DESTINATION program)

# --- CppunitTest targets -------------------------------------------------------
#
# Both need real investigation, not the old "tests excluded" blanket verdict
# -- neither needs use_ure/use_vcl (no unotest/test protector plug-ins), and
# libreofficekit itself is add_subdirectory()'d very late (~1182, well after
# officecfg (719), test (1018) and desktop (1174)), so both can be wired
# directly here, no deferred file needed.

# --- CppunitTest_libreofficekit_checkapi --------------------------------------
#
# CppunitTest_libreofficekit_checkapi.mk, re-checked directly against the
# live file: NO use_libraries/use_sdk_api/use_ure call at all -- this is a
# pure compile-sanity target with no linked LibreOffice libraries and no
# CppUnit test cases of its own (checkapi.cxx is just CPPUNIT_PLUGIN_
# IMPLEMENT(), confirmed by reading it directly). Its whole purpose is
# add_cobjects(compile_test) -- libreofficekit/qa/unit/compile_test.c, a
# plain C translation unit whose only content is `LibreOfficeKit*
# compile_test(void) { return lok_init("install/path"); }` -- proving the
# public LOK C headers (LibreOfficeKit/LibreOfficeKit.h, .../
# LibreOfficeKitInit.h, both under include/, already on every target's
# include path via cmake/LibreStudioPlatform.cmake's global
# include_directories()) compile from a plain C file, the way a real external
# LOK consumer would use them. add_cxxflags($(gb_CXX03FLAGS)) applies only to
# the .cxx exception object (checkapi.cxx) -- gb_CppunitTest_add_cobjects's
# own C objects get no such flag upstream either. On this port's GCC (13.2.1,
# not Clang), gb_CXX03FLAGS resolves (solenv/gbuild/platform/com_GCC_defs.mk:
# 182-186) to "-std=gnu++98 -Wno-long-long -Wno-variadic-macros -Wno-
# deprecated-declarations" -- verified directly compilable against this
# system's cppunit headers with a standalone `g++ -std=gnu++98 -c` probe
# before wiring it in, not assumed.
#
# set_external_code(libreofficekit_checkapi): re-verified against solenv/
# gbuild/LinkTarget.mk (same lines cppu_any-external's own comment in cppu/
# CMakeLists.txt already cites) -- EXTERNAL_CODE's real effect is replacing
# this translation unit's -DLIBO_INTERNAL_ONLY with a plain -Wundef flag.
# cmake/LibreStudioPlatform.cmake's add_compile_definitions(...
# LIBO_INTERNAL_ONLY ...) is directory-scoped from the root CMakeLists.txt,
# inherited here too -- undone below with the same target-level
# "-ULIBO_INTERNAL_ONLY" compile-option technique cppu_any-external already
# established, scoped to the CXX source only via a COMPILE_LANGUAGE generator
# expression so it does not also get passed to the C compiler for compile_
# test.c (GCC/Clang would just warn "valid for C++/ObjC++ but not for C" for
# that one, but there is no reason to pass it there at all: checkapi.cxx is
# the only source that actually contains the `#if defined LIBO_INTERNAL_ONLY
# / #error` guard this substitution exists to defeat). -Wundef itself is a
# plain diagnostic (no -Werror anywhere in this port, same finding cppu_any-
# external's own comment already made), so it is not reproduced -- only the
# LIBO_INTERNAL_ONLY substitution is load-bearing.
#
# On Linux: add_libs(-ldl) -- compile_test.c never calls dlopen() itself
# (that is LibreOfficeKitInit.h's own lok_init(), inline in the header), but
# reproduced anyway for fidelity with the .mk's own `ifeq ($(OS),LINUX)`
# block.
librestudio_add_cppunit_test(libreofficekit_checkapi
    SOURCES
        libreofficekit/qa/unit/checkapi
        "${CMAKE_SOURCE_DIR}/libreofficekit/qa/unit/compile_test.c")

target_compile_options(libreofficekit_checkapi PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-ULIBO_INTERNAL_ONLY -std=gnu++98 -Wno-long-long -Wno-variadic-macros -Wno-deprecated-declarations>)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(libreofficekit_checkapi PRIVATE dl)
endif ()

# --- CppunitTest_libreofficekit_tiledrendering --------------------------------
#
# CppunitTest_libreofficekit_tiledrendering.mk, re-checked directly against
# the live file: 1 source (libreofficekit/qa/unit/tiledrendering), use_
# external(boost_headers), use_libraries(sal) only, on Linux add_libs(-lm
# -ldl), use_api(offapi) -- NOT use_sdk_api, so only offapi's normal headers
# (no udkapi), use_configuration -> CONFIGURATION.
#
# tiledrendering.cxx's TiledRenderingTest constructor calls getenv("SRC_ROOT")/
# getenv("INSTDIR") and runAllTests() calls getenv("WORKDIR_FOR_BUILD") --
# all three are now set generically by librestudio_add_cppunit_test() itself
# (cmake/LibreStudioTest.cmake's own header comment on its ENVIRONMENT
# property has the full history of this gap; this call site is in fact the
# one that originally discovered and documented it, before the fix moved
# into the shared helper). Whether this is actually sufficient to make the
# test PASS, rather than merely start up without an immediate getenv() crash,
# is a much larger question this wiring does not resolve: runAllTests() then
# dlopens the real libsofficeapp.so (via lok_cpp_init(), include/
# LibreOfficeKit/LibreOfficeKitInit.h) and loads real .odt/.odp/.ods documents
# from libreofficekit/qa/data/ through the complete filter/service registry --
# genuinely exercising far more of this port's production stack (every
# import filter, VCL, the full UNO service graph) than any other CppunitTest
# ported this session. This is deliberately not pre-judged here one way or
# the other -- see this session's own report for the real `ctest -R
# libreofficekit_tiledrendering` outcome observed.
librestudio_add_cppunit_test(libreofficekit_tiledrendering
    SOURCES
        libreofficekit/qa/unit/tiledrendering
    LIBRARIES
        sal Boost::boost
    INCLUDE
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        offapi_normal_headers
    CONFIGURATION)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(libreofficekit_tiledrendering PRIVATE m dl)
endif ()

# GENUINE BLOCKER, found only by actually compiling this (first time this
# .cxx has ever been built in this port -- these two CppunitTests were never
# wired before this session): tiledrendering.cxx's testPaintPartTile() calls
# `pDocument->paintPartTile(aBuffer.data(), /*nPart=*/0, nCanvasWidth,
# nCanvasHeight, /*nTilePosX=*/0, /*nTilePosY=*/0, /*nTileWidth=*/3840,
# /*nTileHeight=*/3840)` -- 8 arguments -- but this tree's own include/
# LibreOfficeKit/LibreOfficeKit.hxx declares `lok::Document::paintPartTile`
# with 9 parameters (pBuffer, nPart, nMode, nCanvasWidth, nCanvasHeight,
# nTilePosX, nTilePosY, nTileWidth, nTileHeight -- an "nMode" parameter sits
# between nPart and nCanvasWidth that the qa source never passes), a real
# `g++ -std=c++20` compile error ("no matching function ... candidate expects
# 9 arguments, 8 provided"), not a CMake wiring problem. This is a genuine
# content mismatch already present in the checked-out tree -- this fork's qa/
# unit/tiledrendering.cxx predates LibreOfficeKit.hxx's own nMode parameter
# being added -- and fixing test *source* logic (which value nMode should be,
# whether the header or the test is the one that drifted) is a judgment call
# outside this session's scope (porting the CMake build, not patching
# upstream API-version drift in application/test code). Left EXCLUDE_FROM_ALL
# below so this real, pre-existing bug does not regress the project's
# from-scratch `cmake --build` (every other target here, including
# libreofficekit_checkapi just above and every other CppunitTest ported this
# session, builds and is part of the default ALL target) -- the target is
# still fully registered and `cmake --build build --target
# libreofficekit_tiledrendering` still reproduces the exact error above for
# whoever picks this up.
set_target_properties(libreofficekit_tiledrendering PROPERTIES EXCLUDE_FROM_ALL TRUE)
