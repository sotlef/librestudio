# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# cli_ure -- the CLI (.NET/C++-CLI) language binding for UNO: a set of
# managed (C#) and mixed native/managed (C++/CLI, "/clr") assemblies that let
# .NET code talk to a running UNO environment, plus climaker, the tool that
# generates a CLI assembly of C# type stubs from a UNOIDL type registry (the
# .NET analogue of javamaker/cppumaker). This is a second, completely
# separate compiler toolchain (csc.exe, al.exe, MSVC's /clr C++ extension,
# the .NET Framework SDK) on top of the ordinary C++ toolchain every other
# module in this port uses -- and, per Module_cli_ure.mk below, upstream
# itself only ever builds it with MSVC on Windows.
#
# Module_cli_ure.mk, read in full:
#
#   $(eval $(call gb_Module_Module,cli_ure))
#   ifeq ($(ENABLE_CLI),TRUE)
#   ifeq ($(COM),MSC)
#   ifneq ($(CPUNAME)_$(CROSS_COMPILING),AARCH64_TRUE)
#   $(eval $(call gb_Module_add_targets,cli_ure,\
#       CliLibrary_cli_basetypes \
#       CliLibrary_cli_ure \
#       CliNativeLibrary_cli_cppuhelper \
#       CliUnoApi_cli_uretypes \
#       CustomTarget_cli_ure_assemblies \
#       Executable_climaker \
#       Library_cli_cppuhelper_native \
#       Library_cli_uno \
#       Package_cli_basetypes_copy \
#   ))
#   endif
#   endif
#   endif
#
# Three independent gates, all of which must hold: ENABLE_CLI=TRUE (see
# configure.ac lines ~7565-7647: on Windows this defaults to checking for
# csc.exe/al.exe/mscoree.lib and erroring out if `--enable-cli` was passed
# but they are missing -- there is no equivalent detection anywhere in this
# configure-less CMake port, so ENABLE_CLI is declared as a plain option
# below, defaulting OFF rather than upstream's real on-Windows default of
# ON, until real find_program(csc)/find_program(al)/mscoree.lib detection is
# added); COM=MSC (MSVC specifically -- no mingw-w64 branch exists upstream
# either, /clr and the CLI toolchain are MSVC-only); and not
# (CPUNAME=AARCH64 AND cross-compiling) (no ARM64 CLI toolchain support).
# Real gate below: if (WIN32 AND MSVC AND ENABLE_CLI).
#
# Kept structurally present per this port's policy of not silently dropping
# platform-only gbuild targets (cpputools/CMakeLists.txt's sp2bv is the
# canonical precedent). Inert on this Linux build, and MSVC/.NET-Framework-
# toolchain-dependent even on a real Windows build of this port, so
# unverifiable on this machine either way.
if (WIN32 AND MSVC)
    option(ENABLE_CLI
        "Build the CLI (.NET) UNO language binding (cli_basetypes/cli_ure/cli_cppuhelper/cli_uno + climaker) -- needs csc.exe, al.exe and the .NET Framework SDK, none of which this port detects automatically (see configure.ac's own real csc.exe/al.exe/mscoree.lib checks, ~line 7565)"
        OFF)
endif ()

if (WIN32 AND MSVC AND ENABLE_CLI)
    # Real version numbers transcribed from cli_ure/version/version.txt
    # (included by every one of the .mk files below via
    # "include $(SRCDIR)/cli_ure/version/version.txt").
    set(CLI_URETYPES_NEW_VERSION    "1.0.9.0")
    set(CLI_URETYPES_POLICY_VERSION "9.0.0.0")
    set(CLI_BASETYPES_NEW_VERSION    "1.0.20.0")
    set(CLI_BASETYPES_POLICY_VERSION "20.0.0.0")
    set(CLI_URE_NEW_VERSION    "1.0.23.0")
    set(CLI_URE_POLICY_VERSION "23.0.0.0")
    set(CLI_CPPUHELPER_NEW_VERSION    "1.0.23.0")
    set(CLI_CPPUHELPER_POLICY_VERSION "23.0.0.0")

    set(CLI_URE_SNK "${CMAKE_SOURCE_DIR}/cli_ure/source/cliuno.snk")

    find_program(CSC_COMPILER csc.exe csc)
    find_program(AL_TOOL al.exe al)

    # -------------------------------------------------------------------
    # CustomTarget_cli_ure_assemblies.mk -- three trivial @TOKEN@ ->
    # value substitutions (upstream: `sed -e "s/@X@/$(X)/g" $< > $@`,
    # CMake equivalent: configure_file(... @ONLY), and the real source
    # files already use exactly "@CLI_..._NEW_VERSION@" markers, verified
    # by reading all three: cli_ure/source/basetypes/assembly.cs,
    # cli_ure/source/native/assembly.cxx, cli_ure/source/ure/assembly.cs).
    # -------------------------------------------------------------------
    set(CLI_URE_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    configure_file(
        "${CMAKE_SOURCE_DIR}/cli_ure/source/basetypes/assembly.cs"
        "${CLI_URE_GEN_DIR}/basetypes/assembly.cs" @ONLY)
    configure_file(
        "${CMAKE_SOURCE_DIR}/cli_ure/source/native/assembly.cxx"
        "${CLI_URE_GEN_DIR}/native/assembly.cxx" @ONLY)
    configure_file(
        "${CMAKE_SOURCE_DIR}/cli_ure/source/ure/assembly.cs"
        "${CLI_URE_GEN_DIR}/ure/assembly.cs" @ONLY)

    # Deliberately NOT reproduced: gb_CliConfigTarget (solenv/gbuild/
    # CliLibrary.mk's clipatchconfig.pl-driven substitution of the bare
    # (non-"@"-wrapped) CLI_*_OLD_VERSION/CLI_*_NEW_VERSION tokens inside
    # cli_basetypes_config/cli_ure_config/cli_cppuhelper_config/
    # cli_uretypes_config) and the whole gb_CliAssemblyTarget "al.exe
    # -link:<config> -version:<policy>" publisher-policy-assembly step
    # those configs feed (see solenv/gbuild/CliAssembly.mk). Both exist
    # purely so a .NET application can be redirected, via machine/app
    # .config, from an old cli_basetypes/cli_ure/cli_cppuhelper/cli_uretypes
    # version to this one at *load* time -- side-by-side assembly
    # versioning, not something needed for cli_basetypes.dll etc. to build
    # or run standalone. Left as a follow-up, same spirit as the al.exe
    # policy-assembly omission documented for cli_uretypes below.

    # -------------------------------------------------------------------
    # CliLibrary_cli_basetypes.mk -- "cli_basetypes": System.Reflection
    # attributes (Any/BoundAttribute/... 8 .cs files) plus the generated
    # assembly.cs above, compiled to cli_basetypes.dll with csc.exe and
    # strong-named with cliuno.snk.
    # -------------------------------------------------------------------
    set(CLI_BASETYPES_DLL "${CMAKE_CURRENT_BINARY_DIR}/cli_basetypes.dll")
    add_custom_command(
        OUTPUT "${CLI_BASETYPES_DLL}"
        COMMAND "${CSC_COMPILER}" /nologo /target:library /out:"${CLI_BASETYPES_DLL}"
                /keyfile:"${CLI_URE_SNK}"
                "${CMAKE_SOURCE_DIR}/cli_ure/source/basetypes/uno/Any.cs"
                "${CMAKE_SOURCE_DIR}/cli_ure/source/basetypes/uno/BoundAttribute.cs"
                "${CMAKE_SOURCE_DIR}/cli_ure/source/basetypes/uno/ExceptionAttribute.cs"
                "${CMAKE_SOURCE_DIR}/cli_ure/source/basetypes/uno/OnewayAttribute.cs"
                "${CMAKE_SOURCE_DIR}/cli_ure/source/basetypes/uno/ParameterizedTypeAttribute.cs"
                "${CMAKE_SOURCE_DIR}/cli_ure/source/basetypes/uno/PolymorphicType.cs"
                "${CMAKE_SOURCE_DIR}/cli_ure/source/basetypes/uno/TypeArgumentsAttribute.cs"
                "${CMAKE_SOURCE_DIR}/cli_ure/source/basetypes/uno/TypeParametersAttribute.cs"
                "${CLI_URE_GEN_DIR}/basetypes/assembly.cs"
        DEPENDS
            "${CMAKE_SOURCE_DIR}/cli_ure/source/basetypes/uno/Any.cs"
            "${CLI_URE_GEN_DIR}/basetypes/assembly.cs"
            "${CLI_URE_SNK}"
        COMMENT "csc: cli_basetypes.dll"
        VERBATIM)
    add_custom_target(cli_basetypes ALL DEPENDS "${CLI_BASETYPES_DLL}")

    # -------------------------------------------------------------------
    # CliUnoApi_cli_uretypes.mk -- "cli_uretypes": NOT compiled from C#
    # sources at all. gb_CliUnoApi_wrap_api,cli_uretypes,udkapi feeds the
    # merged udkapi UnoApiTarget .rdb straight into climaker itself
    # (solenv/gbuild/CliUnoApi.mk's gb_CliUnoApi__command):
    #
    #   climaker --out cli_uretypes.dll --assembly-company "LibreOffice" \
    #       --assembly-description "..." --assembly-version <ver> \
    #       --keyfile cliuno.snk <udkapi.rdb>
    #
    # i.e. climaker (built below) *is* the compiler for this assembly --
    # it reads UNOIDL type descriptions and emits CLI (C#-equivalent IL)
    # type stubs directly, the same role javamaker/cppumaker play for
    # Java/C++. Needs climaker and the udkapi UnoApiTarget rdb from this
    # port's own UNO-API machinery (cmake/LibreStudioUnoApi.cmake).
    # -------------------------------------------------------------------
    set(CLI_URETYPES_DLL "${CMAKE_CURRENT_BINARY_DIR}/cli_uretypes.dll")
    add_custom_command(
        OUTPUT "${CLI_URETYPES_DLL}"
        COMMAND climaker --out "${CLI_URETYPES_DLL}"
                --assembly-company "LibreOffice"
                --assembly-description "This assembly contains metadata for the LibreOffice API."
                --assembly-version "${CLI_URETYPES_NEW_VERSION}"
                --keyfile "${CLI_URE_SNK}"
                "$<TARGET_PROPERTY:udkapi_rdb,RDB_FILE>"
        DEPENDS climaker udkapi_rdb "${CLI_URE_SNK}"
        COMMENT "climaker: cli_uretypes.dll (udkapi)"
        VERBATIM)
    add_custom_target(cli_uretypes ALL DEPENDS "${CLI_URETYPES_DLL}")
    # NOTE: "udkapi_rdb"/its RDB_FILE property is this file's own assumed
    # name for whatever target cmake/LibreStudioUnoApi.cmake exposes as the
    # merged udkapi UnoApiTarget .rdb -- not verified against that file as
    # part of this module's own assignment (out of scope: LibreStudioUnoApi.cmake
    # belongs to no single module). Whoever wires cli_ure into the real
    # add_subdirectory graph should confirm the actual target/property name
    # and adjust this DEPENDS/COMMAND accordingly.
    #
    # Deliberately NOT reproduced: the al.exe publisher-policy assembly
    # (policy.1.0.cli_uretypes, CLI_URETYPES_POLICY_VERSION="9.0.0.0" above)
    # -- see the CustomTarget_cli_ure_assemblies note further up for why.

    # -------------------------------------------------------------------
    # CliLibrary_cli_ure.mk -- "cli_ure": 4 .cs files (DisposeGuard/
    # WeakAdapter/WeakBase/WeakComponentBase) plus generated assembly.cs,
    # referencing cli_uretypes.dll (gb_CliLibrary_use_assemblies).
    # -------------------------------------------------------------------
    set(CLI_URE_DLL "${CMAKE_CURRENT_BINARY_DIR}/cli_ure.dll")
    add_custom_command(
        OUTPUT "${CLI_URE_DLL}"
        COMMAND "${CSC_COMPILER}" /nologo /target:library /out:"${CLI_URE_DLL}"
                /keyfile:"${CLI_URE_SNK}"
                /reference:"${CLI_URETYPES_DLL}"
                "${CMAKE_SOURCE_DIR}/cli_ure/source/ure/uno/util/DisposeGuard.cs"
                "${CMAKE_SOURCE_DIR}/cli_ure/source/ure/uno/util/WeakAdapter.cs"
                "${CMAKE_SOURCE_DIR}/cli_ure/source/ure/uno/util/WeakBase.cs"
                "${CMAKE_SOURCE_DIR}/cli_ure/source/ure/uno/util/WeakComponentBase.cs"
                "${CLI_URE_GEN_DIR}/ure/assembly.cs"
        DEPENDS
            "${CMAKE_SOURCE_DIR}/cli_ure/source/ure/uno/util/DisposeGuard.cs"
            "${CLI_URE_GEN_DIR}/ure/assembly.cs"
            "${CLI_URE_SNK}" "${CLI_URETYPES_DLL}" cli_uretypes
        COMMENT "csc: cli_ure.dll"
        VERBATIM)
    add_custom_target(cli_ure_assembly ALL DEPENDS "${CLI_URE_DLL}")

    # -------------------------------------------------------------------
    # CliNativeLibrary_cli_cppuhelper.mk -- despite the "CliNativeLibrary"
    # class name this is a real, ordinary-looking C++/CLI DLL (mixed
    # native+managed code, MSVC's /clr), NOT csc-compiled: 2 hand-written
    # sources (native_bootstrap.cxx, path.cxx) plus the generated
    # assembly.cxx, all through add_cxxclrobjects. gb_Library_use_
    # internal_bootstrap_api,cli_cppuhelper,udkapi -> normal udkapi headers.
    # -------------------------------------------------------------------
    set(CLI_CPPUHELPER_SOURCES
        "${CMAKE_SOURCE_DIR}/cli_ure/source/native/native_bootstrap.cxx"
        "${CMAKE_SOURCE_DIR}/cli_ure/source/native/path.cxx"
        "${CLI_URE_GEN_DIR}/native/assembly.cxx")

    add_library(cli_cppuhelper SHARED ${CLI_CPPUHELPER_SOURCES})
    target_compile_options(cli_cppuhelper PRIVATE /clr)
    # -delayload:<cppuhelper/cppu/sal filename> x3, per Library_cli_
    # cppuhelper_native.mk -- MSVC's own /DELAYLOAD linker switch, the
    # direct equivalent of CMake's LINK_FLAGS for a per-DLL delay-load list.
    set_target_properties(cli_cppuhelper PROPERTIES
        LINK_FLAGS "/DELAYLOAD:$<TARGET_FILE_NAME:cppuhelper> /DELAYLOAD:$<TARGET_FILE_NAME:cppu> /DELAYLOAD:$<TARGET_FILE_NAME:sal> /ignore:4248")

    target_include_directories(cli_cppuhelper PRIVATE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(cli_cppuhelper udkapi_normal_headers)

    target_link_libraries(cli_cppuhelper PRIVATE cppu cppuhelper sal advapi32 delayimp mscoree msvcmrt)

    # -------------------------------------------------------------------
    # Library_cli_cppuhelper_native.mk -- confusingly-named-relative-to-
    # the-above-target sibling: gb_Library_Library,cli_cppuhelper_native
    # is a *second*, plain (non-/clr) native wrapper library that simply
    # links the real cppu/sal/cppuhelper C++ libraries so upstream's own
    # CliAssembly machinery has a native import lib to re-export from --
    # verified: Library_cli_cppuhelper_native.mk has add_ldflags(-ignore:4248,
    # -keyfile:cliuno.snk, -delayload:x3) and use_libraries(cppu cppuhelper
    # sal), but ZERO source files (no add_exception_objects/add_cxxobjects/
    # add_cxxclrobjects call anywhere in the file) -- it exists only to
    # carry those extra link flags on top of a link step that otherwise
    # links nothing of its own. Since the CMake "cli_cppuhelper" target two
    # sections up already carries the equivalent LINK_FLAGS/link-libraries
    # directly (there being no separate CMake concept of a link-flags-only,
    # sourceless target worth splitting out), this port folds the two
    # together rather than reproducing an empty-sources CMake target.
    # -------------------------------------------------------------------

    # -------------------------------------------------------------------
    # Executable_climaker.mk -- climaker: the UNOIDL -> CLI type-stub
    # generator used above by cli_uretypes. 2 /clr-compiled sources
    # (climaker_app.cxx, climaker_emit.cxx).
    # -------------------------------------------------------------------
    add_executable(climaker
        "${CMAKE_SOURCE_DIR}/cli_ure/source/climaker/climaker_app.cxx"
        "${CMAKE_SOURCE_DIR}/cli_ure/source/climaker/climaker_emit.cxx")
    target_compile_options(climaker PRIVATE /clr /wd4715)
    set_target_properties(climaker PROPERTIES LINK_FLAGS "/ignore:4248")

    target_include_directories(climaker PRIVATE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(climaker udkapi_normal_headers)

    target_link_libraries(climaker PRIVATE cppu cppuhelper sal salhelper unoidl mscoree msvcmrt)

    # `$(call gb_Executable_get_headers_target,climaker) : $(call
    # gb_CliLibrary_get_target,cli_basetypes)` -- an order-only build
    # dependency (climaker needs cli_basetypes.dll present, e.g. for its
    # own qa/climaker test resources, before its own header/build stage
    # runs; it does not link against it).
    add_dependencies(climaker cli_basetypes)

    # -------------------------------------------------------------------
    # Library_cli_uno.mk -- "cli_uno": the actual UNO<->CLI bridge
    # (cli_bridge/cli_data/cli_environment/cli_proxy/cli_uno.cxx, all
    # /clr). gb_Library_use_udk_api -> normal udkapi headers.
    # `$(call gb_Library_get_headers_target,cli_uno) :| cli_ure cli_uretypes`
    # is again an order-only dependency (this native/managed mix
    # references those two managed assemblies by CLI metadata, not by a
    # native link line -- there is no `use_libraries`/`use_assemblies` call
    # for them in this .mk, only that bare header-stage ordering rule).
    # -------------------------------------------------------------------
    add_library(cli_uno SHARED
        "${CMAKE_SOURCE_DIR}/cli_ure/source/uno_bridge/cli_bridge.cxx"
        "${CMAKE_SOURCE_DIR}/cli_ure/source/uno_bridge/cli_data.cxx"
        "${CMAKE_SOURCE_DIR}/cli_ure/source/uno_bridge/cli_environment.cxx"
        "${CMAKE_SOURCE_DIR}/cli_ure/source/uno_bridge/cli_proxy.cxx"
        "${CMAKE_SOURCE_DIR}/cli_ure/source/uno_bridge/cli_uno.cxx")
    target_compile_options(cli_uno PRIVATE /clr)
    set_target_properties(cli_uno PROPERTIES LINK_FLAGS "/ignore:4248")

    target_include_directories(cli_uno PRIVATE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(cli_uno udkapi_normal_headers)

    target_link_libraries(cli_uno PRIVATE cppu sal mscoree msvcmrt)
    add_dependencies(cli_uno cli_ure_assembly cli_uretypes)

    install(TARGETS cli_cppuhelper cli_uno climaker
        RUNTIME DESTINATION program
        LIBRARY DESTINATION program)
    install(FILES "${CLI_BASETYPES_DLL}" "${CLI_URE_DLL}" "${CLI_URETYPES_DLL}"
        DESTINATION program)

    # -------------------------------------------------------------------
    # Package_cli_basetypes_copy.mk -- a deliberate duplicate copy of
    # cli_basetypes.dll into the ODK SDK's own bin/ directory ("to work
    # around CLR DLL finding brain damage", per the .mk's own comment).
    # Only meaningful once the ODK is actually being assembled -- gated
    # here on this port's existing, real ENABLE_ODK option
    # (cmake/LibreStudioConfigHeaders.cmake), the same switch odk-consuming
    # code elsewhere in this port already keys off.
    # -------------------------------------------------------------------
    if (ENABLE_ODK)
        install(FILES "${CLI_BASETYPES_DLL}" DESTINATION sdk/bin)
    endif ()
endif ()
