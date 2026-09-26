# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# unoidl — the UNO IDL type registry: a reader/writer for .rdb type registries
# and the provider that turns a tree of .idl files into one.
#
# This is the first half of the toolchain the rest of the build depends on: the
# sources here deliberately do not include any com/sun/star headers, so the
# library and the tools below it can be built before any C++ bindings exist.

find_package(BISON 3.0 REQUIRED)
find_package(FLEX 2.6 REQUIRED)

set(UNOIDL_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

# The parser and scanner are C++ and are wired together by name: the grammar
# emits sourceprovider-parser.hxx, which the checked-in
# sourceprovider-scanner.hxx and the scanner itself include.  Upstream generates
# them into workdir/YaccTarget and workdir/LexTarget (solenv/gbuild/LinkTarget.mk);
# the file names are what matters, not the directory.
bison_target(SourceProviderParser
    "${CMAKE_SOURCE_DIR}/unoidl/source/sourceprovider-parser.y"
    "${UNOIDL_GENERATED_DIR}/sourceprovider-parser.cxx"
    DEFINES_FILE "${UNOIDL_GENERATED_DIR}/sourceprovider-parser.hxx")

flex_target(SourceProviderScanner
    "${CMAKE_SOURCE_DIR}/unoidl/source/sourceprovider-scanner.l"
    "${UNOIDL_GENERATED_DIR}/sourceprovider-scanner.cxx")

set(UNOIDL_SOURCES
    unoidl/source/sourcefileprovider
    unoidl/source/sourcetreeprovider
    unoidl/source/unoidl
    unoidl/source/unoidlprovider)

# Library_unoidl.mk: "drop legacy provider for fuzzing" --
#   ifeq (,$(filter FUZZERS,$(BUILD_TYPE)))
#   $(eval $(call gb_Library_add_exception_objects,unoidl, unoidl/source/legacyprovider))
#   endif
# That $(filter FUZZERS,$(BUILD_TYPE)) inside ifeq(,...) is a *negative*
# match: the block runs (legacyprovider gets compiled in) whenever FUZZERS is
# NOT in BUILD_TYPE, i.e. legacyprovider ships on every normal build and is
# excluded only for OSS-Fuzz-style builds. ENABLE_FUZZERS is sal's own local
# option() for that same gbuild FUZZERS BUILD_TYPE flag (sal/CMakeLists.txt);
# re-declaring it here with the identical name/type/default is the
# established precedent for this exact kind of module-local BUILD_TYPE
# filter (CMake option() re-declaration is idempotent), and it stays local
# rather than moving to cmake/LibreStudioConfigHeaders.cmake because -- same
# as sal's own copy -- there is no generated header to substitute it into.
option(ENABLE_FUZZERS
    "Build for OSS-Fuzz style fuzzing (matches gbuild's FUZZERS BUILD_TYPE flag)" OFF)
if (NOT ENABLE_FUZZERS)
    list(APPEND UNOIDL_SOURCES unoidl/source/legacyprovider)
endif ()

librestudio_expand_cxx(UNOIDL_SOURCES)

add_library(unoidl SHARED
    ${UNOIDL_SOURCES}
    ${BISON_SourceProviderParser_OUTPUTS}
    ${FLEX_SourceProviderScanner_OUTPUTS})

target_include_directories(unoidl PRIVATE
    "${CMAKE_SOURCE_DIR}/unoidl/source"
    "${UNOIDL_GENERATED_DIR}")

target_compile_definitions(unoidl PRIVATE LO_DLLIMPLEMENTATION_UNOIDL)
target_link_libraries(unoidl PRIVATE reg sal salhelper)

librestudio_set_library_output_name(unoidl unoidl OOO)

# --- tools -------------------------------------------------------------------

# unoidl-write turns a tree of .idl into an .rdb; every generated C++ binding in
# the build descends from its output.  unoidl-check validates an .rdb against a
# reference, unoidl-read dumps one — both are useful for checking the port.
#
# Module_unoidl.mk gates unoidl-write on
#   $(if $(or $(filter ODK,$(BUILD_TYPE)),$(call gb_not,$(CROSS_COMPILING))), Executable_unoidl-write)
# i.e. "ODK, OR not cross-compiling". This build never cross-compiles
# (CROSS_COMPILING is never true here -- see gbuild's own $(call
# gb_not,$(CROSS_COMPILING)) evaluating unconditionally true), which makes
# the right-hand side of the `or` always true regardless of ODK, so the whole
# condition is unconditionally true. unoidl-write is therefore built
# unconditionally here too, ODK or not -- and it has to be: unoidl-write is
# also what generates the unoidl-check_test fixtures below, and every other
# module in this port ultimately depends on unoidl-write's output (the .rdb
# registries the generated C++ bindings come from).

# Executable_unoidl-write.mk / Executable_unoidl-check.mk / Executable_unoidl-
# read.mk each have, in their gb_Executable_use_libraries call:
#   $(if $(filter TRUE,$(DISABLE_DYNLOADING)),reg)
#   $(if $(filter TRUE,$(DISABLE_DYNLOADING)),store)
# i.e. reg/store are linked directly only when DISABLE_DYNLOADING=TRUE.
# Per configure.ac (~line 3555), DISABLE_DYNLOADING is only ever set to TRUE
# when enable_dynamic_loading resolves to "no" (forced for iOS/Android/
# Emscripten, or an explicit --disable-dynamic-loading); on this Linux
# desktop port dynamic loading is enabled by default and no cmake/*.cmake
# option in this port ever sets/forces it off. So DISABLE_DYNLOADING is never
# TRUE here, and the three $(if ...) conditionals above correctly resolve to
# "don't add reg/store directly" -- they come in transitively via unoidl's
# own target_link_libraries(unoidl PRIVATE reg sal salhelper) above. This is
# a considered no-op, not an oversight: no reg/store lines are added to
# unoidl-write/unoidl-check/unoidl-read below.
add_executable(unoidl-write "${CMAKE_SOURCE_DIR}/unoidl/source/unoidl-write.cxx")
target_link_libraries(unoidl-write PRIVATE unoidl salhelper sal)

# Module_unoidl.mk: $(if $(call gb_not,$(CROSS_COMPILING)), Executable_unoidl-check)
# -- unconditional here for the same "never cross-compiling" reason as
# unoidl-write above; unlike unoidl-write/-read it isn't gated on ODK at all
# upstream.
add_executable(unoidl-check "${CMAKE_SOURCE_DIR}/unoidl/source/unoidl-check.cxx")
target_link_libraries(unoidl-check PRIVATE unoidl salhelper sal)

# Module_unoidl.mk: $(if $(filter ODK,$(BUILD_TYPE)), Executable_unoidl-read) --
# purely an ODK tool (dumps an .rdb registry for inspection), unlike
# unoidl-write/-check above which the rest of the build needs regardless of
# ODK. See cmake/LibreStudioConfigHeaders.cmake for why ENABLE_ODK defaults
# OFF (a bare ./configure without --enable-odk leaves BUILD_TYPE without ODK
# in this project's own configure.ac).
if (ENABLE_ODK)
    add_executable(unoidl-read "${CMAKE_SOURCE_DIR}/unoidl/source/unoidl-read.cxx")
    target_link_libraries(unoidl-read PRIVATE unoidl salhelper sal)
endif ()

if (ENABLE_ODK)
    set(_unoidl_check_test_install_targets unoidl-read)
else ()
    set(_unoidl_check_test_install_targets)
endif ()

install(TARGETS unoidl unoidl-write unoidl-check ${_unoidl_check_test_install_targets}
    LIBRARY DESTINATION program
    RUNTIME DESTINATION program)

# --- unoidl-check_test (Module_unoidl.mk's CustomTarget_unoidl-check_test,
#     ODK-gated the same way) -----------------------------------------------
#
# CustomTarget_unoidl-check_test.mk builds 8 fixture .idl files (unoidl/qa/
# unoidl-check/*.idl) into .rdb registries via unoidl-write, then runs
# unoidl-check 32 times (with/without --ignore-unpublished, {psa,usa}.rdb as
# reference against each of the 8 registries) asserting a hardcoded
# pass/fail outcome per combination -- a real regression test for
# unoidl-check's publish-compatibility logic, producing no artifact anything
# else in the build consumes. That shape -- pass/fail assertions, no
# downstream consumer -- is exactly what CTest is for, so this is
# add_test() rather than a plain always-run add_custom_target(): registering
# it as a test lets `ctest` (and any future `make check`-equivalent target)
# pick it up the same way gb_Module_add_check_targets did, instead of
# silently running on every build.
#
# See unoidl-check_test.cmake for the full transcription of the .mk's 32
# assertions; this just wires the two executables and the qa/ fixture
# directory in.
if (ENABLE_ODK)
    add_test(NAME unoidl_check_test
        COMMAND "${CMAKE_COMMAND}"
            "-DUNOIDL_WRITE=$<TARGET_FILE:unoidl-write>"
            "-DUNOIDL_CHECK=$<TARGET_FILE:unoidl-check>"
            "-DQA_DIR=${CMAKE_CURRENT_SOURCE_DIR}/qa/unoidl-check"
            "-DOUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/unoidl-check_test"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/unoidl-check_test.cmake")
endif ()
