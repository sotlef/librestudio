# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# jsuno -- the QuickJS-based JavaScript-to-UNO bridge used by scripting's
# jsprov script provider (Tools > Macros > JavaScript). Despite its
# rhinocompatibility.cxx source file (a Rhino-API-compatibility shim
# implemented on top of QuickJS), this library is pure C++ and needs no JDK:
# it embeds the QuickJS engine, not a JVM. Source list and defs from
# Library_jsuno.mk; not gated by LIBRESTUDIO_ENABLE_JAVA.
#
# quickjs (external/quickjs/StaticLibrary_quickjs.mk) is jsuno's only
# external dependency and has no CMake wiring anywhere else in this port, so
# it is vendored here, next to its only consumer -- same technique already
# used by svl for zxcvbn-c and package for argon2 (the tarball fetch itself
# is in the root CMakeLists.txt, next to those two). The upstream tarball is
# a pre-amalgamated single-file build: one generated object
# (UnpackedTarball/quickjs/quickjs-amalgam) and no patches
# (UnpackedTarball_quickjs.mk carries none), compiled with warnings disabled
# (gb_StaticLibrary_set_warnings_disabled). ExternalProject_quickjs.mk is an
# empty gb_ExternalProject_ExternalProject registration stub with no build
# commands, so it has no CMake equivalent to reproduce.
add_library(quickjs STATIC "${quickjs_SOURCE_DIR}/quickjs-amalgam.c")
target_include_directories(quickjs PUBLIC "${quickjs_SOURCE_DIR}")
target_compile_options(quickjs PRIVATE -w)
set_target_properties(quickjs PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(JSUNO_SOURCES
    jsuno/source/jsuno
    jsuno/source/rhinocompatibility
)

librestudio_expand_cxx(JSUNO_SOURCES)
add_library(jsuno SHARED ${JSUNO_SOURCES})

target_include_directories(jsuno PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(jsuno udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(jsuno PRIVATE LO_DLLIMPLEMENTATION_JSUNO)

target_link_libraries(jsuno PRIVATE
    comphelper cppu cppuhelper sal
    quickjs)

librestudio_set_library_output_name(jsuno jsuno OOO)

install(TARGETS jsuno LIBRARY DESTINATION program)
