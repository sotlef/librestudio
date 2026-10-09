# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sb (module basic) — the StarBasic interpreter: scanner/parser/compiler,
# runtime, SBX value system, and (since BUILD_TYPE includes SCRIPTING by
# default -- configure.ac:3538) the full macro-editing/VBA-compatibility layer
# sfx2 needs. Source list from Library_sb.mk. Not ported from there: the
# precompiled header, and the WNT/WNTINTEL-only asm/dllmgr variants -- this is
# a non-Windows build, so dllmgr-none is the one that applies unconditionally.

# ifneq ($(filter SCRIPTING,$(BUILD_TYPE)),) in Library_sb.mk gates this
# chunk of exception objects (and, further below, the sb.component
# registration itself -- dlgcont/scriptcont implement the two services
# sb.component declares, so without these sources there is nothing to
# register). ENABLE_SCRIPTING defaults ON (matching configure.ac), so this
# is included by default; see cmake/LibreStudioConfigHeaders.cmake.
set(SB_SOURCES)
if (ENABLE_SCRIPTING)
    list(APPEND SB_SOURCES
        basic/source/basmgr/basicmanagerrepository
        basic/source/basmgr/basmgr
        basic/source/basmgr/vbahelper
        basic/source/classes/codecompletecache
        basic/source/classes/eventatt
        basic/source/classes/image
        basic/source/classes/propacc
        basic/source/classes/sb
        basic/source/classes/sbunoobj
        basic/source/classes/sbxmod
        basic/source/comp/buffer
        basic/source/comp/codegen
        basic/source/comp/dim
        basic/source/comp/exprtree
        basic/source/comp/exprgen
        basic/source/comp/exprnode
        basic/source/comp/io
        basic/source/comp/loops
        basic/source/comp/parser
        basic/source/comp/sbcomp
        basic/source/comp/scanner
        basic/source/comp/symtbl
        basic/source/comp/token
        basic/source/uno/dlgcont
        basic/source/uno/modsizeexceeded
        basic/source/uno/namecont
        basic/source/uno/scriptcont
        basic/source/runtime/comenumwrapper
        basic/source/runtime/ddectrl
        basic/source/runtime/inputbox
        basic/source/runtime/iosys
        basic/source/runtime/props
        basic/source/runtime/stdobj
        basic/source/runtime/stdobj1
        basic/source/classes/errobject
        basic/source/util/sbutil
    )
endif ()

list(APPEND SB_SOURCES
    basic/source/classes/global
    basic/source/classes/sbintern
    basic/source/comp/basiccharclass
    basic/source/runtime/basrdll
    basic/source/runtime/methods
    basic/source/runtime/methods1
    basic/source/runtime/runtime
    basic/source/sbx/sbxarray
    basic/source/sbx/sbxbool
    basic/source/sbx/sbxcoll
    basic/source/sbx/sbxcurr
    basic/source/sbx/sbxbase
    basic/source/sbx/sbxdate
    basic/source/sbx/sbxdec
    basic/source/sbx/sbxexec
    basic/source/sbx/sbxform
    basic/source/sbx/sbxint
    basic/source/sbx/sbxobj
    basic/source/sbx/sbxres
    basic/source/sbx/sbxscan
    basic/source/sbx/sbxstr
    basic/source/sbx/sbxvalue
    basic/source/sbx/sbxvar

)

# Library_sb.mk: dllmgr-x86 (+ wnt-x86.asm) for 32-bit Windows, dllmgr-x64 for
# 64-bit Windows, dllmgr-none everywhere else.
if (WIN32 AND (LIBRESTUDIO_CPUNAME STREQUAL "X86_64" OR LIBRESTUDIO_CPUNAME STREQUAL "AARCH64"))
    list(APPEND SB_SOURCES basic/source/runtime/dllmgr-x64)
elseif (WIN32)
    message(FATAL_ERROR "sb: only the x64 Windows DLL manager is ported (dllmgr-x86 needs wnt-x86.asm)")
else ()
    list(APPEND SB_SOURCES basic/source/runtime/dllmgr-none)
endif ()

librestudio_expand_cxx(SB_SOURCES)

# CACHE INTERNAL (not a plain directory-scope variable) so this already-
# expanded, already-absolute source list stays visible to
# basic/CppunitTest_deferred.cmake, include()'d later from the root
# CMakeLists.txt's own scope (a different scope than this add_subdirectory()
# call's own, which a plain set()/list() would not cross) -- same visibility
# need OFFICECFG_INCLUDE_DIR/LIBRESTUDIO_TEST_UNITTEST_DIR already have, for
# the same reason (see officecfg/CMakeLists.txt and test/CMakeLists.txt's own
# comments on those). basic_scanner needs it there for
# gb_CppunitTest_use_library_objects(basic_scanner,sb) -- CppunitTest_
# basic_scanner.mk recompiles sb's own already-expanded source list directly
# into the test MODULE instead of linking libsb.so, the same technique
# sal/CMakeLists.txt's CppunitTest_sal_osl_file_details and jvmfwk/
# CMakeLists.txt's CppunitTest_jvmfwk_sunversion already use for the
# identical "qa file reaches into an internal, non-exported header" reason
# (see either file's own comment on its librestudio_add_cppunit_test call).
set(SB_SOURCES "${SB_SOURCES}" CACHE INTERNAL
    "expanded Library_sb.mk source list, reused by basic_scanner's use_library_objects(sb)")

add_library(sb SHARED ${SB_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(sb PRIVATE
        "${CMAKE_SOURCE_DIR}/basic/inc/pch/precompiled_sb.hxx")
    target_compile_definitions(sb PRIVATE PCH_LEVEL=2)
endif ()

# gb_Library_set_include plus gb_Library_use_sdk_api,
# gb_Library_use_api(sb, oovbaapi), and
# gb_Library_use_custom_headers(... officecfg/registry).
target_include_directories(sb PRIVATE
    "${CMAKE_SOURCE_DIR}/basic/inc"
    "${CMAKE_SOURCE_DIR}/basic/source/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(sb officecfg_headers
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers)

target_compile_definitions(sb PRIVATE BASIC_DLLIMPLEMENTATION)

target_link_libraries(sb PRIVATE
    comphelper cppu cppuhelper sal salhelper i18nlangtag sot svl svt tl utl
    vcl xmlscript
    ICU::uc ICU::i18n)
if (WIN32)
    target_link_libraries(sb PRIVATE oleaut32)    # Library_sb.mk: use_system_win32_libs
endif ()

# Library_sb.mk's gb_Library_set_componentfile call sits inside the same
# SCRIPTING ifneq block as the sources above (its two declared services are
# implemented by dlgcont.cxx/scriptcont.cxx, which only exist in the build
# when ENABLE_SCRIPTING is ON), so the componentfile registration is gated
# the same way.
if (ENABLE_SCRIPTING)
    librestudio_add_component(sb
        COMPONENT "${CMAKE_SOURCE_DIR}/basic/util/sb.component"
        LIBRARY   sb
        LAYER     OOO)
endif ()

librestudio_set_library_output_name(sb sb OOO)

install(TARGETS sb LIBRARY DESTINATION program)

# --- CppunitTest targets -----------------------------------------------------
#
# Module_basic.mk's three CppunitTest check targets (basic_scanner,
# basic_macros, basic_util) all sit inside "ifneq ($(filter SCRIPTING,
# $(BUILD_TYPE)),)" -- this port always treats that as on (ENABLE_SCRIPTING
# defaults ON, matching configure.ac, see the comment on SB_SOURCES above),
# so all three are ported unconditionally.
#
# Only basic_util is wired here, at basic/CMakeLists.txt's own natural
# position: CppunitTest_basic_util.mk, re-checked directly against the live
# file, is trivial -- 1 source (basic/qa/cppunit/test_util), use_libraries(sb)
# only, no use_ure/use_vcl/use_rdb/use_configuration/use_api -- so it has no
# ordering need this module's own add_subdirectory() point doesn't already
# satisfy (sb, its only dependency, is the target defined immediately above).
#
# basic_scanner and basic_macros are NOT here: both need at least
# unobootstrapprotector/unoexceptionprotector (unotest/CMakeLists.txt) --
# basic_macros additionally needs vclbootstrapprotector (test/CMakeLists.txt),
# test_fixtures (test/CMakeLists.txt, "test" -> test_fixtures, this port's
# established rename -- see e.g. sax/CppunitTest_deferred.cmake's own comment
# on this exact rename) and program_services_rdb (defined near the very end
# of the root CMakeLists.txt) -- and add_subdirectory(basic) (root
# CMakeLists.txt) runs well before add_subdirectory(unotest)/add_subdirectory
# (test) and long before program_services_rdb is defined. Both are deferred
# to basic/CppunitTest_deferred.cmake instead, include()'d from the root
# CMakeLists.txt after every one of those exists -- see that file's own
# header comment for the full detail, matching the pattern already
# established by e.g. toolkit/CppunitTest_deferred.cmake and sax/
# CppunitTest_deferred.cmake (read either's header comment for the general
# mechanism).
librestudio_add_cppunit_test(basic_util
    SOURCES
        basic/qa/cppunit/test_util
    LIBRARIES sb)

# basic/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/basic/CppunitTest_deferred.cmake")
