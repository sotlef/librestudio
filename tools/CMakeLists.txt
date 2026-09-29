# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# tl — the "toolkit-lite" layer: URLs, dates, streams, colours, XML walking,
# fraction and string helpers.  The module is called tools but the library it
# produces is tl (tools/Library_tl.mk); everything above comphelper builds on
# it, and utl and sot both link it.

# zlib, resolved via librestudio_find_package() (cmake/LibreStudioTarballs.cmake).
librestudio_find_package(NAME ZLIB)

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
#
# Bug found 2026-09-27, building in a brand-new binary directory for the
# first time (this project's own build/ had always already had libsal_
# textenclo.so sitting in instdir/program/ from an earlier build, masking
# this): bestreversemap.cxx's Encoder constructor exercises rtl_TextEncoding
# values including the rare ones (e.g. RTL_TEXTENCODING_MS_1361), which
# sal/textenc/textenc.cxx's own FullTextEncodingData::FullTextEncodingData()
# resolves by osl::Module::loadRelative()-ing sal_textenc as a runtime
# dlopen() plug-in -- a dependency invisible to the build graph (bestreversemap
# only target_link_libraries()'s plain "sal", never sal_textenc, the same
# "dlopen'd, not linked" shape sal/CMakeLists.txt's own sal_rtl CppunitTest
# already has to add_dependencies(...,sal_textenc) for above). Without
# sal_textenc already built and installed to instdir/program/ by the time
# this custom_command actually RUNS (not just configured), loadRelative()
# fails and FullTextEncodingData's constructor std::abort()s. Same real,
# already-established fix as sal_rtl's own: force the ordering explicitly.
set(TL_REVERSEMAP_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/reversemap.cxx")
add_custom_command(
    OUTPUT "${TL_REVERSEMAP_SOURCE}"
    COMMAND sh -c "\"$<TARGET_FILE:bestreversemap>\" > \"${TL_REVERSEMAP_SOURCE}\""
    DEPENDS bestreversemap sal_textenc
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
    ICU::uc ICU::i18n ${LIBRESTUDIO_ZLIB_TARGET}
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

# CppunitTest_tools_test.mk, re-checked directly against the live file: 25
# ordinary sources (tools/qa/cppunit/test_bigint through test_zcodec, listed
# below) plus 3 extra translation units of matching leaf names with "_check"
# appended (test_cpu_runtime_detection_AVX2_check/SSE2_check/SSSE3_check)
# that gb_CppunitTest_add_exception_objects's own third, per-file
# CXXFLAGS_INTRINSICS_AVX2/SSE2/SSSE3 argument (.mk lines 43-53) compiles
# with the matching -mavx2/-msse2/-mssse3 flag.
#
# Corrects an earlier (2026-09-27) note here that deferred this whole test,
# citing two reasons -- re-checked directly, neither holds:
#   - "vcl is a huge module far downstream, nowhere near being reached yet":
#     already stale even then (vcl/CMakeLists.txt exists and is a real,
#     fully wired add_library(vcl SHARED ...) target; several modules
#     already link it, e.g. svtools/CMakeLists.txt's svt).
#   - "the three '_check' sources need a per-source-file compile-options
#     mechanism librestudio_add_cppunit_test() does not support": also
#     false on inspection -- `ls tools/qa/cppunit/*_check.cxx` confirms the
#     three "_check" leaf names are their OWN separate .cxx files on disk
#     (test_cpu_runtime_detection_AVX2_check.cxx etc.), not the same source
#     recompiled twice under an alias. So no change to librestudio_add_
#     cppunit_test() was ever needed for this: they go into SOURCES like
#     every other source here, and the one CXXFLAGS_INTRINSICS_* flag each
#     needs is applied afterwards with a plain set_source_files_properties(
#     ... PROPERTIES COMPILE_OPTIONS ...) call below -- a mechanism entirely
#     independent of the shared cppunit-test helper.
#
# use_sdk_api -> normal udkapi+offapi INCLUDE/DEPENDS, same pattern as
# tools_config above. use_libraries (basegfx comphelper sal tl test unotest
# utl vcl -- "test" -> test_fixtures, same rename tools_unique_id/
# tools_config's own comments already explain). use_static_libraries
# ooopathutils, the same static library ported above. use_externals(
# boost_headers libxml2) -> Boost::boost, LibXml2::LibXml2 (the same
# imported target svx_removewhichrange's own CppunitTest already uses for
# this external, see svx/CMakeLists.txt).
#
# No use_ure/use_vcl*/use_rdb/use_configuration/use_components/
# use_uiconfigs anywhere in this .mk, so no PROTECTORS/CONFIGURATION/
# COMPONENTS/NONHEADLESS/UICONFIG -- and, critically, no DEPENDS on anything
# add_subdirectory()'d after tools's own position in the root CMakeLists.txt
# either. test_fixtures/unotest (test/CMakeLists.txt, unotest/CMakeLists.txt,
# both add_subdirectory()'d AFTER tools) are referenced only through
# LIBRARIES -> target_link_libraries(), which CMake resolves against the
# whole project at generate time regardless of add_subdirectory() order --
# unlike add_dependencies() (this function's DEPENDS) or the COMPONENTS/
# CONFIGURATION/NONHEADLESS/UICONFIG options' own immediate if(NOT TARGET
# ...)/get_property(GLOBAL ...) checks, which do need the named target to
# already exist at the point of the call (the real reason those other
# options force a CppunitTest_deferred.cmake split elsewhere in this port,
# e.g. svgio/starmath's own use_rdb(services)/use_configuration tests). This
# is exactly why tools_unique_id/tools_config above already work with
# LIBRARIES test_fixtures/unotest despite the same ordering, and why this
# test needs no CppunitTest_deferred.cmake either -- wired directly here.
librestudio_add_cppunit_test(tools_test
    SOURCES
        tools/qa/cppunit/test_bigint
        tools/qa/cppunit/test_date
        tools/qa/cppunit/test_time
        tools/qa/cppunit/test_duration
        tools/qa/cppunit/test_fract
        tools/qa/cppunit/test_inetmime
        tools/qa/cppunit/test_json_writer
        tools/qa/cppunit/test_pathutils
        tools/qa/cppunit/test_poly
        tools/qa/cppunit/test_reversemap
        tools/qa/cppunit/test_stream
        tools/qa/cppunit/test_urlobj
        tools/qa/cppunit/test_color
        tools/qa/cppunit/test_rectangle
        tools/qa/cppunit/test_100mm2twips
        tools/qa/cppunit/test_xmlwalker
        tools/qa/cppunit/test_xmlwriter
        tools/qa/cppunit/test_xpath
        tools/qa/cppunit/test_GenericTypeSerializer
        tools/qa/cppunit/test_guid
        tools/qa/cppunit/test_cpuid
        tools/qa/cppunit/test_cpu_runtime_detection_AVX2
        tools/qa/cppunit/test_cpu_runtime_detection_SSE2
        tools/qa/cppunit/test_cpu_runtime_detection_SSSE3
        tools/qa/cppunit/test_Wildcard
        tools/qa/cppunit/test_zcodec
        tools/qa/cppunit/test_cpu_runtime_detection_AVX2_check
        tools/qa/cppunit/test_cpu_runtime_detection_SSE2_check
        tools/qa/cppunit/test_cpu_runtime_detection_SSSE3_check
    LIBRARIES
        basegfx comphelper sal tl test_fixtures unotest utl vcl
        ooopathutils Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/tools/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers)

# gb_CppunitTest_add_exception_objects,tools_test,tools/qa/cppunit/test_cpu_
# runtime_detection_{AVX2,SSE2,SSSE3}_check,$(CXXFLAGS_INTRINSICS_*) (.mk
# lines 43-53): the per-file compile flag each "_check" translation unit
# needs to actually emit the intrinsics its own #ifdef LO_*_AVAILABLE guard
# (include/tools/simdsupport.hxx) is testing for -- without it, neither
# __AVX2__ nor __SSE2__/__SSSE3__ is predefined by the compiler and the
# function body compiles away to nothing (harmless, just untested).
# configure.ac's own GCC/Clang branch (lines 8642-8647 and, identically,
# 13365-13370) sets flag_sse2=-msse2, flag_ssse3=-mssse3, flag_avx2=-mavx2
# -- the exact three flags applied below, one source file each, via
# set_source_files_properties() rather than any per-source extension to
# librestudio_add_cppunit_test() itself (see the comment above for why none
# was needed). Guarded to x86/x86_64 targets only: these are x86-only
# codegen flags that GCC/Clang reject outright as an unrecognized command-
# line option on other target architectures -- this port's equivalent, at
# the CMake level, of the AC_COMPILE_IFELSE probe configure.ac itself runs
# before ever setting CXXFLAGS_INTRINSICS_* (which would simply stay empty,
# never applying the flag, on a target where the probe fails).
if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64|i[3-6]86|x86)$")
    set_source_files_properties(
        "${CMAKE_SOURCE_DIR}/tools/qa/cppunit/test_cpu_runtime_detection_AVX2_check.cxx"
        PROPERTIES COMPILE_OPTIONS "-mavx2")
    set_source_files_properties(
        "${CMAKE_SOURCE_DIR}/tools/qa/cppunit/test_cpu_runtime_detection_SSE2_check.cxx"
        PROPERTIES COMPILE_OPTIONS "-msse2")
    set_source_files_properties(
        "${CMAKE_SOURCE_DIR}/tools/qa/cppunit/test_cpu_runtime_detection_SSSE3_check.cxx"
        PROPERTIES COMPILE_OPTIONS "-mssse3")
endif ()
