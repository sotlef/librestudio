# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# tl — the "toolkit-lite" layer: URLs, dates, streams, colours, XML walking,
# fraction and string helpers.  The module is called tools but the library it
# produces is tl (tools/Library_tl.mk); everything above comphelper builds on
# it, and utl and sot both link it.

# bestreversemap is a build-time helper: it turns the word list compiled into it
# into a reverse-map header.  It is defined first because tl's generated source
# is produced by running it.
add_executable(bestreversemap
    "${CMAKE_SOURCE_DIR}/tools/source/reversemap/bestreversemap.cxx")
target_link_libraries(bestreversemap PRIVATE sal)

# Source list from Library_tl.mk, with its conditions resolved for a Linux
# build:
#   * stream/strmunx, not stream/strmwnt (that one is Win32 API only);
#   * misc/fix16, which the .mk includes unless the system provides libfixmath.
# The precompiled header is not ported.

set(TL_SOURCES
    tools/source/datetime/datetime
    tools/source/datetime/datetimeutils
    tools/source/datetime/duration
    tools/source/datetime/systemdatetime
    tools/source/datetime/tdate
    tools/source/datetime/ttime
    tools/source/debug/debug
    tools/source/fsys/fileutil
    tools/source/fsys/urlobj
    tools/source/fsys/wldcrd
    tools/source/generic/b3dtrans
    tools/source/generic/bigint
    tools/source/generic/color
    tools/source/generic/config
    tools/source/generic/fract
    tools/source/generic/gen
    tools/source/generic/line
    tools/source/generic/point
    tools/source/generic/poly
    tools/source/generic/poly2
    tools/source/generic/svborder
    tools/source/inet/hostfilter
    tools/source/inet/inetmime
    tools/source/inet/inetmsg
    tools/source/inet/inetstrm
    tools/source/memtools/multisel
    tools/source/misc/cpuid
    tools/source/misc/extendapplicationenvironment
    tools/source/misc/fix16
    tools/source/misc/json_writer
    tools/source/misc/lazydelete
    tools/source/misc/UniqueID
    tools/source/ref/globname
    tools/source/ref/ref
    tools/source/stream/GenericTypeSerializer
    tools/source/stream/stream
    tools/source/stream/strmunx
    tools/source/stream/vcompat
    tools/source/string/tenccvt
    tools/source/xml/XmlWalker
    tools/source/xml/XmlWriter
    tools/source/xml/XPath
    tools/source/zcodec/zcodec
)

librestudio_expand_cxx(TL_SOURCES)

# gb_Library_add_generated_exception_objects(tl, CustomTarget/tools/string/reversemap):
# bestreversemap has the word list compiled in and writes the source to stdout.
set(TL_REVERSEMAP_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/reversemap.cxx")
add_custom_command(
    OUTPUT "${TL_REVERSEMAP_SOURCE}"
    COMMAND sh -c "\"$<TARGET_FILE:bestreversemap>\" > \"${TL_REVERSEMAP_SOURCE}\""
    DEPENDS bestreversemap
    COMMENT "bestreversemap: reversemap.cxx"
    VERBATIM)

add_library(tl SHARED ${TL_SOURCES} "${TL_REVERSEMAP_SOURCE}")

if (ENABLE_PCH)
    target_precompile_headers(tl PRIVATE
        "${CMAKE_SOURCE_DIR}/tools/inc/pch/precompiled_tl.hxx")
    target_compile_definitions(tl PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(tl PRIVATE
    "${CMAKE_SOURCE_DIR}/tools/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}"
    "${LIBXML2_INCLUDE_DIRS}")
add_dependencies(tl udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(tl PRIVATE TOOLS_DLLIMPLEMENTATION SYSTEM_ZLIB ZLIB_CONST)

target_link_libraries(tl PRIVATE
    basegfx comphelper i18nlangtag cppu cppuhelper sal
    ICU::uc ICU::i18n ZLIB::ZLIB
    "${LIBXML2_LIBRARIES}"
    rt)

# Library_tl.mk:
#   ifeq ($(OS),WNT)
#   $(eval $(call gb_Library_use_system_win32_libs,tl,\
#       mpr netapi32 ole32 shell32 uuid winmm \
#   ))
#   endif
if (WIN32)
    target_link_libraries(tl PRIVATE mpr netapi32 ole32 shell32 uuid winmm)
endif ()

librestudio_set_library_output_name(tl tl OOO)
install(TARGETS tl bestreversemap
    LIBRARY DESTINATION program
    RUNTIME DESTINATION program)

# --- StaticLibrary_ooopathutils, StaticLibrary_UseUnixWrappers -------------

# StaticLibrary_ooopathutils.mk: 1 source (tools/source/misc/pathutils),
# no use_libraries/use_api/use_sdk_api at all in the .mk. tools_config
# (CppunitTest_tools_config.mk, below) links it via use_static_libraries, so
# it is ported first here, ahead of that test.
add_library(ooopathutils STATIC
    "${CMAKE_SOURCE_DIR}/tools/source/misc/pathutils.cxx")

# StaticLibrary_UseUnixWrappers.mk: a deliberate dummy static library with
# exactly one C source (tools/source/misc/UseUnixWrappers.c, added via
# gb_StaticLibrary_add_cobjects -- a C, not C++, translation unit, hence the
# explicit absolute path with its own .c extension below rather than going
# through librestudio_expand_cxx, which always appends .cxx). Its own .mk
# says outright what it is for: nothing links or runs it -- it exists purely
# so <tools/UnixWrappers.h> gets compiled at least once as plain C, to catch
# it silently drifting into C++-only syntax before some (Windows-only, not
# yet ported) external C caller needs it to stay plain-C-compilable. Ported
# faithfully as a real (if outwardly pointless) build target, matching the
# .mk, not skipped as dead weight.
add_library(UseUnixWrappers STATIC
    "${CMAKE_SOURCE_DIR}/tools/source/misc/UseUnixWrappers.c")

# --- CppunitTest targets -----------------------------------------------------

# tools's 3 CppunitTest_*.mk files (Module_tools.mk's gb_Module_add_check_
# targets list), re-checked directly against their live .mk files. No
# CMake-target-name collision with the "tl" library (or with ooopathutils /
# UseUnixWrappers above) for any of the three, so none needed renaming.

# CppunitTest_tools_unique_id.mk: 1 source (tools/qa/cppunit/UniqueIdTest),
# use_libraries tl test unotest, no use_api/use_sdk_api/use_ure/use_vcl* at
# all -- only its own set_include (tools/inc). use_libraries "test" is the
# gbuild library name (Library_test.mk), but this port's CMake target for it
# is test_fixtures, not test -- see i18nutil/CMakeLists.txt's own
# CppunitTest_i18nutil.mk comment for the full reasoning (enable_testing()
# reserves the "test" target name for CTest itself), reused verbatim here.
librestudio_add_cppunit_test(tools_unique_id
    SOURCES
        tools/qa/cppunit/UniqueIdTest
    LIBRARIES tl test_fixtures unotest
    INCLUDE "${CMAKE_SOURCE_DIR}/tools/inc")

# CppunitTest_tools_config.mk: 1 source (tools/qa/cppunit/test_config),
# use_external boost_headers, use_sdk_api, use_libraries sal tl test unotest,
# use_static_libraries ooopathutils -- the static lib ported above, hence
# ported after it in this file.
librestudio_add_cppunit_test(tools_config
    SOURCES
        tools/qa/cppunit/test_config
    LIBRARIES sal tl test_fixtures unotest ooopathutils Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/tools/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers)

# CppunitTest_tools_test.mk: DEFERRED, not ported.
#
# What the task brief that drove this wave originally said to justify
# deferring this one ("vcl is a huge module far downstream in this port's
# bottom-up ordering, nowhere near being reached yet") turns out to be
# stale: vcl/CMakeLists.txt already exists and defines a real, fully wired
# add_library(vcl SHARED ...) target (confirmed by reading the file; several
# modules already link it, e.g. svtools/CMakeLists.txt's svt). So "vcl
# doesn't exist yet" is NOT the actual blocker any more.
#
# The real remaining reasons this is still deferred, found while double-
# checking that stale premise:
#   - Scope: 25 sources spanning most of tools' own qa/cppunit/ directory
#     (bigint, date, time, duration, fract, inetmime, json_writer,
#     pathutils, poly, reversemap, stream, urlobj, color, rectangle,
#     100mm2twips, xmlwalker, xmlwriter, xpath, GenericTypeSerializer, guid,
#     cpuid, Wildcard, zcodec, plus 3 more below) -- a much bigger unit of
#     work than every other test in this file combined.
#   - Three of those sources (test_cpu_runtime_detection_AVX2/SSE2/SSSE3)
#     are each compiled TWICE under different names, once plain and once
#     suffixed "_check", with the "_check" variant needing its own per-file
#     compile flags (CXXFLAGS_INTRINSICS_AVX2/SSE2/SSSE3 in the .mk, via a
#     third gb_CppunitTest_add_exception_objects argument) -- a per-source-
#     file compile-options mechanism librestudio_add_cppunit_test() does not
#     support today (it takes one shared DEFINITIONS/INCLUDE list for the
#     whole target, not per-source overrides).
# Neither of these is "vcl is unreachable" -- that part of the original
# reasoning should be treated as corrected, not repeated, in any later wave
# that picks this test back up.
