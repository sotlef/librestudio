# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# cppu — the C++ UNO runtime: Any, Type, Reference, the type library loader, the
# thread pool and the environment stack.  This is the first module that compiles
# against generated com/sun/star bindings, so it is the first consumer of
# udkapi's headers.

set(CPPU_SOURCES
    cppu/source/cppu/compat
    cppu/source/cppu/cppu_opt
    cppu/source/threadpool/current
    cppu/source/threadpool/jobqueue
    cppu/source/threadpool/thread
    cppu/source/threadpool/threadident
    cppu/source/threadpool/threadpool
    cppu/source/typelib/static_types
    cppu/source/typelib/typelib
    cppu/source/uno/any
    cppu/source/uno/cascade_mapping
    cppu/source/uno/check
    cppu/source/uno/data
    cppu/source/uno/EnvDcp
    cppu/source/uno/EnvStack
    cppu/source/uno/IdentityMapping
    cppu/source/uno/lbenv
    cppu/source/uno/lbmap
    cppu/source/uno/loadmodule
    cppu/source/uno/sequence)

librestudio_expand_cxx(CPPU_SOURCES)

add_library(cppu SHARED ${CPPU_SOURCES})

# gb_Library_use_udk_api maps to the "normal" header variant - see
# gb_UnoApiHeadersTarget_select_variant in solenv/gbuild/UnoApiTarget.mk.
target_include_directories(cppu PRIVATE "${udkapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(cppu udkapi_normal_headers)

target_compile_definitions(cppu PRIVATE CPPU_DLLIMPLEMENTATION)
target_link_libraries(cppu PRIVATE sal salhelper)

target_link_options(cppu PRIVATE
    "-Wl,--version-script=${CMAKE_SOURCE_DIR}/cppu/util/cppu.map")

librestudio_set_library_output_name(cppu cppu UNO)

install(TARGETS cppu LIBRARY DESTINATION program)

# --- purpenvhelper -------------------------------------------------------------
#
# Library_purpenvhelper.mk: 3 sources, use_udk_api -> normal udkapi,
# use_libraries cppu sal, -DPURPENV_DLLIMPLEMENTATION, its own soversion
# script (cppu/util/uno_purpenvhelpergcc3.map, confirmed on disk), and
# gb_Library_set_is_ure_library_or_dependency (no live CMake effect anywhere
# in this port -- same "inert, not silently dropped" status as other such
# calls, see e.g. ENABLE_VALGRIND in sal/CMakeLists.txt).
#
# Output name layer: Repository.mk:770-773 registers purpenvhelper under
# gb_Helper_register_libraries_for_install(RTVERLIBS,ure, cppuhelper
# purpenvhelper salhelper) -- the SAME RTVERLIBS category as cppuhelper and
# salhelper, both of which this port already gives layer RT (see
# cppuhelper/CMakeLists.txt, salhelper/CMakeLists.txt). Same category, same
# layer: RT.
set(PURPENVHELPER_SOURCES
    cppu/source/helper/purpenv/helper_purpenv_Environment
    cppu/source/helper/purpenv/helper_purpenv_Mapping
    cppu/source/helper/purpenv/helper_purpenv_Proxy)

librestudio_expand_cxx(PURPENVHELPER_SOURCES)

add_library(purpenvhelper SHARED ${PURPENVHELPER_SOURCES})

target_include_directories(purpenvhelper PRIVATE "${udkapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(purpenvhelper udkapi_normal_headers)

target_compile_definitions(purpenvhelper PRIVATE PURPENV_DLLIMPLEMENTATION)
target_link_libraries(purpenvhelper PRIVATE cppu sal)

target_link_options(purpenvhelper PRIVATE
    "-Wl,--version-script=${CMAKE_SOURCE_DIR}/cppu/util/uno_purpenvhelpergcc3.map")

librestudio_set_library_output_name(purpenvhelper purpenvhelper RT)

install(TARGETS purpenvhelper LIBRARY DESTINATION program)

# --- affine_uno_uno / log_uno_uno / unsafe_uno_uno -----------------------------
#
# Three small UNO bridge libraries, all use_udk_api -> normal udkapi, none
# with a soversion script or set_is_ure_library_or_dependency call.
#
# Output name layer: Repository.mk:608-627 registers all three (and
# i18nlangtag) under gb_Helper_register_libraries_for_install(PLAINLIBS_URE,
# ure, affine_uno_uno ... i18nlangtag ... log_uno_uno unsafe_uno_uno).
# i18nlangtag/CMakeLists.txt gives i18nlangtag layer OOO, but that does not
# hold up: per solenv/gbuild/platform/unxgcc.mk:263-267 (gb_Library_FILENAMES),
# PLAINLIBS_URE and PLAINLIBS_OOO both resolve to the exact same bare
# filename pattern `$(gb_Library_SYSPRE)$(lib)$(gb_Library_PLAINEXT)` (e.g.
# libaffine_uno_uno.so) -- no "lo" suffix. The "lo" suffix
# (gb_Library_OOOEXT = gb_Library_DLLPOSTFIX ("lo", gbuild.mk:188) +
# gb_Library_PLAINEXT) is used only by the true OOOLIBS category, which is a
# different category from PLAINLIBS_OOO despite the similar name. So the
# PLAINLIBS_URE/PLAINLIBS_OOO distinction is purely an install-package
# distinction (gb_Library_LAYER: URELIB vs OOO, i.e. which install set the
# library ships in) and not a filename distinction. This matches the CMake
# PLAIN layer (bare OUTPUT_NAME, no suffix), not OOO (`<name>lo`).
# Independent confirmation already in this port: jpipe/juh/juhx
# (jurt/CMakeLists.txt, javaunohelper/CMakeLists.txt) are also PLAINLIBS_URE
# members (Repository.mk:608-627) and already use layer PLAIN, not OOO. So
# i18nlangtag's existing OOO choice looks like a pre-existing bug -- flagged
# here, left untouched (out of this task's scope). All three below use PLAIN.
set(AFFINE_UNO_UNO_SOURCES cppu/source/AffineBridge/AffineBridge)
librestudio_expand_cxx(AFFINE_UNO_UNO_SOURCES)
add_library(affine_uno_uno SHARED ${AFFINE_UNO_UNO_SOURCES})

target_include_directories(affine_uno_uno PRIVATE "${udkapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(affine_uno_uno udkapi_normal_headers)

target_link_libraries(affine_uno_uno PRIVATE purpenvhelper sal)

librestudio_set_library_output_name(affine_uno_uno affine_uno_uno PLAIN)

install(TARGETS affine_uno_uno LIBRARY DESTINATION program)

set(LOG_UNO_UNO_SOURCES cppu/source/LogBridge/LogBridge)
librestudio_expand_cxx(LOG_UNO_UNO_SOURCES)
add_library(log_uno_uno SHARED ${LOG_UNO_UNO_SOURCES})

target_include_directories(log_uno_uno PRIVATE "${udkapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(log_uno_uno udkapi_normal_headers)

target_link_libraries(log_uno_uno PRIVATE cppu purpenvhelper sal)

librestudio_set_library_output_name(log_uno_uno log_uno_uno PLAIN)

install(TARGETS log_uno_uno LIBRARY DESTINATION program)

set(UNSAFE_UNO_UNO_SOURCES cppu/source/UnsafeBridge/UnsafeBridge)
librestudio_expand_cxx(UNSAFE_UNO_UNO_SOURCES)
add_library(unsafe_uno_uno SHARED ${UNSAFE_UNO_UNO_SOURCES})

target_include_directories(unsafe_uno_uno PRIVATE "${udkapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(unsafe_uno_uno udkapi_normal_headers)

target_link_libraries(unsafe_uno_uno PRIVATE purpenvhelper sal)

librestudio_set_library_output_name(unsafe_uno_uno unsafe_uno_uno PLAIN)

install(TARGETS unsafe_uno_uno LIBRARY DESTINATION program)

# --- InternalUnoApi_cppu_qa_types / InternalUnoApi_cppu_qa_cppumaker_types ---
#
# Two module-private type registries used only by this module's own tests
# below (cppu_qa_any/cppu_qa_recursion/cppu_qa_reference read the first,
# cppu_test_cppumaker the second). Each gb_InternalUnoApi_InternalUnoApi call
# names a single loose .idl file as its registry root (cppu/qa/types.idl,
# cppu/qa/cppumaker/types.idl), not a directory tree -- confirmed by reading
# both files -- so this uses librestudio_uno_api_rdb_file(), the same shape
# testtools/CMakeLists.txt's bridgetest/performance targets already use for
# InternalUnoApi_bridgetest.mk / InternalUnoApi_performance.mk.
# gb_InternalUnoApi_use_api,<name>,udkapi maps to DEP_RDBS below. cppu_qa_types
# is consumed exclusively via gb_CppunitTest_use_internal_comprehensive_api
# (see cppu_qa_any/cppu_qa_recursion/cppu_qa_reference below), so only the
# comprehensive headers variant is generated for it. cppu_qa_cppumaker_types
# used to be the same story (only cppu_test_cppumaker read it, also via
# use_internal_comprehensive_api) -- now stoc/CMakeLists.txt's stoc_dump also
# reads it, but via plain gb_CppunitTest_use_internal_api (verified directly
# against stoc/CppunitTest_stoc_dump.mk, not assumed), which forwards
# unprefixed to gb_LinkTarget_use_internal_api (solenv/gbuild/LinkTarget.mk:
# 1260-1270) -- the plain/-L "normal" header variant, not comprehensive. So a
# second, normal-variant headers target is generated for it below too, purely
# for that external consumer (this module's own tests still only need
# comprehensive).
librestudio_uno_api_rdb_file(cppu_qa_types
    IDL      "${CMAKE_SOURCE_DIR}/cppu/qa/types.idl"
    OUTPUT   "${CMAKE_BINARY_DIR}/UnoApiTarget/cppu_qa_types.rdb"
    DEP_RDBS "${udkapi_RDB}")

librestudio_uno_api_headers(cppu_qa_types_comprehensive
    RDB        "${cppu_qa_types_RDB}"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/UnoApiHeadersTarget/cppu_qa_types/comprehensive"
    VARIANT    comprehensive
    EXTRA_RDBS "${udkapi_RDB}")

librestudio_uno_api_rdb_file(cppu_qa_cppumaker_types
    IDL      "${CMAKE_SOURCE_DIR}/cppu/qa/cppumaker/types.idl"
    OUTPUT   "${CMAKE_BINARY_DIR}/UnoApiTarget/cppu_qa_cppumaker_types.rdb"
    DEP_RDBS "${udkapi_RDB}")

librestudio_uno_api_headers(cppu_qa_cppumaker_types_comprehensive
    RDB        "${cppu_qa_cppumaker_types_RDB}"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/UnoApiHeadersTarget/cppu_qa_cppumaker_types/comprehensive"
    VARIANT    comprehensive
    EXTRA_RDBS "${udkapi_RDB}")

# Normal (-L) variant, for stoc/CMakeLists.txt's stoc_dump CppunitTest -- see
# the comment above.
librestudio_uno_api_headers(cppu_qa_cppumaker_types_normal
    RDB        "${cppu_qa_cppumaker_types_RDB}"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/UnoApiHeadersTarget/cppu_qa_cppumaker_types/normal"
    VARIANT    normal
    EXTRA_RDBS "${udkapi_RDB}")

# --- CppunitTest targets ------------------------------------------------------
#
# cppu's 7 CppunitTest_*.mk files, ported via librestudio_add_cppunit_test()
# (cmake/LibreStudioTest.cmake -- read its header comment first). API header
# wiring follows the plain/comprehensive/none split cppuhelper/CMakeLists.txt
# established: gb_CppunitTest_use_udk_api and gb_CppunitTest_use_api both
# resolve (via gb_LinkTarget_use_api/gb_LinkTarget__use_api,
# solenv/gbuild/LinkTarget.mk:1229-1244) to the "normal" variant -- the same
# one add_library(cppu ...) above links against, via
# udkapi_normal_HEADERS_INCLUDE_DIR / udkapi_normal_headers.
# gb_CppunitTest_use_internal_comprehensive_api resolves (LinkTarget.mk:
# 1278-1282, comprehensive_ prefix) to the "comprehensive" variant of every
# rdb it names.
#
# All 7 were re-checked directly against their live .mk files (not just the
# prior-research summary) before wiring.

# CppunitTest_cppu_any-external.mk: 1 source, use_libraries cppu sal,
# use_udk_api -> plain udkapi.
#
# Fix (first real `cmake --build`, 2026-09-27): the previous wave's own
# comment here was wrong about gb_CppunitTest_set_external_code -- re-checked
# directly against solenv/gbuild/LinkTarget.mk (lines 103/118/133/148/163)
# and platform/com_GCC_class.mk: EXTERNAL_CODE's real, load-bearing effect is
# "$(if $(EXTERNAL_CODE),$(gb_CXXFLAGS_Wundef),$(gb_DEFS_INTERNAL))" -- it
# REPLACES this one translation unit's -DLIBO_INTERNAL_ONLY with a plain
# -Wundef warning flag, not merely something -Werror-related (there being no
# -Werror anywhere in this port was true but irrelevant -- the substitution
# itself still needs modelling). any-external.cxx's own name says why: it
# calls Any::get<Any>()/has<Any>()/operator>>=/<<=(Any,Any), all of which
# include/com/sun/star/uno/Any.hxx deletes ONLY "#if defined
# LIBO_INTERNAL_ONLY" (lines 216/309/620/681) -- this test exists precisely
# to verify that an EXTERNAL consumer of the UNO API (never compiled with
# LIBO_INTERNAL_ONLY, unlike every internal LibreOffice translation unit)
# does not hit those deletions. cmake/LibreStudioPlatform.cmake's
# add_compile_definitions(... LIBO_INTERNAL_ONLY ...) is directory-scoped
# from the root CMakeLists.txt, inherited by every target including this
# one -- undone here with a target-level "-ULIBO_INTERNAL_ONLY" compile
# option, which GCC/Clang process left-to-right so it must, and does (CMake's
# generated command line orders directory+target DEFINES before
# directory+target FLAGS), land after the inherited -D on the actual compile
# command. gb_CXXFLAGS_Wundef itself (a plain -Wundef diagnostic, no
# -Werror to escalate it) is still skipped as genuinely inert, matching the
# "no -Werror anywhere" finding the previous wave already made -- only the
# LIBO_INTERNAL_ONLY substitution was the real, missed part.
librestudio_add_cppunit_test(cppu_any-external
    SOURCES
        cppu/qa/any-external
    LIBRARIES cppu sal
    INCLUDE "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers)

target_compile_options(cppu_any-external PRIVATE -ULIBO_INTERNAL_ONLY)

# CppunitTest_cppu_qa_any.mk: 1 source, use_libraries cppu sal,
# use_internal_comprehensive_api against cppu_qa_types and udkapi -- both
# comprehensive. cppu/qa/test_any.cxx's own #include list (Enum1.hpp,
# Interface1.hpp, Struct1.hpp, Rec.hpp, ...) confirms these are the
# top-level (unnamespaced) entities cppu/qa/types.idl declares.
librestudio_add_cppunit_test(cppu_qa_any
    SOURCES
        cppu/qa/test_any
    LIBRARIES cppu sal
    INCLUDE
        "${cppu_qa_types_comprehensive_HEADERS_INCLUDE_DIR}"
        "${udkapi_comprehensive_HEADERS_INCLUDE_DIR}"
    DEPENDS cppu_qa_types_comprehensive_headers udkapi_comprehensive_headers)

# CppunitTest_cppu_qa_recursion.mk: 1 source, use_libraries cppu sal,
# use_internal_comprehensive_api against cppu_qa_types and udkapi -- same
# shape as cppu_qa_any above (cppu/qa/test_recursion.cxx includes Rec.hpp,
# the recursive struct from the same cppu/qa/types.idl).
librestudio_add_cppunit_test(cppu_qa_recursion
    SOURCES
        cppu/qa/test_recursion
    LIBRARIES cppu sal
    INCLUDE
        "${cppu_qa_types_comprehensive_HEADERS_INCLUDE_DIR}"
        "${udkapi_comprehensive_HEADERS_INCLUDE_DIR}"
    DEPENDS cppu_qa_types_comprehensive_headers udkapi_comprehensive_headers)

# CppunitTest_cppu_qa_reference.mk: 1 source, use_libraries cppu sal,
# use_internal_comprehensive_api against cppu_qa_types and udkapi -- same
# shape again (cppu/qa/test_reference.cxx includes Interface1.hpp).
librestudio_add_cppunit_test(cppu_qa_reference
    SOURCES
        cppu/qa/test_reference
    LIBRARIES cppu sal
    INCLUDE
        "${cppu_qa_types_comprehensive_HEADERS_INCLUDE_DIR}"
        "${udkapi_comprehensive_HEADERS_INCLUDE_DIR}"
    DEPENDS cppu_qa_types_comprehensive_headers udkapi_comprehensive_headers)

# CppunitTest_cppu_qa_unotype.mk: 1 source, use_libraries cppu sal,
# use_internal_comprehensive_api against udkapi ONLY -- no cppu_qa_types
# dependency here (cppu/qa/test_unotype.cxx includes only com/sun/star/...
# headers and cppu/unotype.hxx, nothing from cppu/qa/types.idl).
librestudio_add_cppunit_test(cppu_qa_unotype
    SOURCES
        cppu/qa/test_unotype
    LIBRARIES cppu sal
    INCLUDE "${udkapi_comprehensive_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_comprehensive_headers)

# CppunitTest_cppu_test_cppumaker.mk: 1 source, use_libraries cppu sal,
# use_internal_comprehensive_api against cppu_qa_cppumaker_types and udkapi --
# both comprehensive. cppu/qa/cppumaker/test_cppumaker.cxx's #include list
# (FILE.hpp/lconv.hpp/tm.hpp/std.hpp at top level, plus the whole
# test/codemaker/cppumaker/... tree) all come from the single
# cppu/qa/cppumaker/types.idl file: everything under the
# "module test { module codemaker { module cppumaker { ... } } }" wrapper
# resolves to the test/codemaker/cppumaker/ header path, confirming no
# separate UNO API target is needed for those.
librestudio_add_cppunit_test(cppu_test_cppumaker
    SOURCES
        cppu/qa/cppumaker/test_cppumaker
    LIBRARIES cppu sal
    INCLUDE
        "${cppu_qa_cppumaker_types_comprehensive_HEADERS_INCLUDE_DIR}"
        "${udkapi_comprehensive_HEADERS_INCLUDE_DIR}"
    DEPENDS cppu_qa_cppumaker_types_comprehensive_headers udkapi_comprehensive_headers)

# CppunitTest_cppu_typelib.mk: 1 source, use_libraries cppu sal, use_api
# udkapi -> plain udkapi (use_api, unlike use_internal_comprehensive_api,
# selects the "normal" variant -- see the file-header note above).
#
# RESOLVED: this test's gbuild source also calls gb_CppunitTest_use_ure
# (solenv/gbuild/CppunitTest.mk:250-258), which requires
# unobootstrapprotector/unoexceptionprotector -- both Library_*protector.mk
# targets, previously deferred here because they live in the unotest module
# and themselves depend on comphelper, several layers above cppu in this
# port's bottom-up ordering. comphelper is now ported at the test-
# infrastructure level and both protector libraries are built in
# unotest/CMakeLists.txt, so PROTECTORS is wired in below the same way
# comphelper/CMakeLists.txt's comphelper_test now does. (The rest of what
# use_ure pulls in -- the ure/services rdb and its -env: arguments -- is
# still not modelled by librestudio_add_cppunit_test(); see that gap noted
# on comphelper_test.)
librestudio_add_cppunit_test(cppu_typelib
    SOURCES
        cppu/qa/typelib
    LIBRARIES cppu sal
    INCLUDE "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector)
