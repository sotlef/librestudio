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

# jsuno/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/jsuno/CppunitTest_deferred.cmake")
