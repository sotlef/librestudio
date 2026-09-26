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
# CppunitTest_libreofficekit_checkapi, CppunitTest_libreofficekit_tiledrendering,
# Package_selectionhandles and UIConfig_libreofficekit are excluded (tests
# and packaging, this port's established policy).

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
