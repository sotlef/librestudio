# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# net_ure -- the .NET language binding for UNO.
#
# Module_net_ure.mk, read in full: its whole gb_Module_add_targets call is
# gated `ifeq ($(ENABLE_DOTNET)_$(filter ODK,$(BUILD_TYPE)),TRUE_ODK)` -- it
# needs BOTH a detected, sufficiently new dotnet SDK AND the ODK BUILD_TYPE
# filter. ENABLE_ODK already defaults OFF on this port (see
# cmake/LibreStudioConfigHeaders.cmake's own long comment on why a bare
# ./configure never appends ODK to BUILD_TYPE), so this whole module is
# inert-by-default here regardless of dotnet detection -- matching upstream's
# real behaviour exactly (a bare ./configure with no --enable-odk never
# builds net_ure either, even with a perfectly good dotnet SDK installed).
#
# configure.ac:5938-5991's own detection logic (--with-dotnet, defaulting to
# probing $PATH for a `dotnet`/`dotnet.exe`, checking `dotnet --version`'s
# major component is >= 8, silently disabling -- not erroring -- if missing
# or too old) is reproduced below with find_program()/execute_process(),
# following the same "real switch, real detection" precedent as
# cmake/LibreStudioConfigHeaders.cmake's ENABLE_AVAHI/ENABLE_COINMP
# (pkg_check_modules-detected CACHE BOOL, not a plain option()).
find_program(LIBRESTUDIO_DOTNET_EXECUTABLE NAMES dotnet)
set(_net_ure_dotnet_ok FALSE)
if (LIBRESTUDIO_DOTNET_EXECUTABLE)
    execute_process(
        COMMAND "${LIBRESTUDIO_DOTNET_EXECUTABLE}" --version
        OUTPUT_VARIABLE _net_ure_dotnet_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _net_ure_dotnet_version_rc)
    if (_net_ure_dotnet_version_rc EQUAL 0)
        string(REGEX MATCH "^[0-9]+" _net_ure_dotnet_major "${_net_ure_dotnet_version}")
        if (_net_ure_dotnet_major AND _net_ure_dotnet_major GREATER_EQUAL 8)
            set(_net_ure_dotnet_ok TRUE)
        endif ()
    endif ()
endif ()
set(ENABLE_DOTNET ${_net_ure_dotnet_ok} CACHE BOOL
    "configure.ac's --with-dotnet auto-detection: a `dotnet` executable with SDK major version >= 8 was found on PATH")
unset(_net_ure_dotnet_ok)

if (ENABLE_DOTNET AND ENABLE_ODK)
    # --- Library_net_bootstrap ------------------------------------------------
    #
    # A plain, hand-written C++ library (one .cxx file) with no netmaker
    # codegen dependency at all -- gb_Library_use_udk_api + cppu/cppuhelper/
    # sal/salhelper only. Genuinely portable and buildable independent of the
    # rest of this module, so it is wired for real (unlike the netmaker-
    # dependent pieces below).
    add_library(net_bootstrap SHARED
        "${CMAKE_CURRENT_SOURCE_DIR}/source/bootstrap/bootstrap.cxx")
    target_include_directories(net_bootstrap PRIVATE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(net_bootstrap udkapi_normal_headers)
    target_link_libraries(net_bootstrap PRIVATE cppu cppuhelper sal salhelper)
    librestudio_set_library_output_name(net_bootstrap net_bootstrap OOO)
    install(TARGETS net_bootstrap LIBRARY DESTINATION program)

    # --- DotnetLibrary_net_basetypes ------------------------------------------
    #
    # DotnetLibrary_net_basetypes.mk: 6 hand-written C# sources (Any.cs,
    # BoundAttribute.cs, IQueryInterface.cs, RaisesAttribute.cs,
    # UnoException.cs, UnoGeneratedAttribute.cs) -- no codegen dependency
    # either, so this too is genuinely buildable without netmaker.
    # solenv/gbuild/DotnetLibrary.mk's own gb_DotnetLibrary_DotnetLibrary
    # generates a minimal SDK-style .csproj (TargetFramework netstandard2.0,
    # AssemblyName = target name) and runs `dotnet build <proj> -c Release
    # -o <workdir>` -- reproduced directly below via file(WRITE) + a real
    # add_custom_command/add_custom_target(ALL), rather than trying to
    # shoehorn a hand-rolled MSBuild project through CMake's own (C++/C-only)
    # target model.
    set(_net_basetypes_dir "${CMAKE_BINARY_DIR}/net_ure/net_basetypes")
    set(_net_basetypes_proj "${_net_basetypes_dir}/net_basetypes.csproj")
    set(_net_basetypes_sources
        "${CMAKE_CURRENT_SOURCE_DIR}/source/basetypes/Any.cs"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/basetypes/BoundAttribute.cs"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/basetypes/IQueryInterface.cs"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/basetypes/RaisesAttribute.cs"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/basetypes/UnoException.cs"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/basetypes/UnoGeneratedAttribute.cs")
    file(MAKE_DIRECTORY "${_net_basetypes_dir}")
    file(WRITE "${_net_basetypes_proj}" "<Project Sdk=\"Microsoft.NET.Sdk\">
  <PropertyGroup>
    <TargetFramework>netstandard2.0</TargetFramework>
    <AssemblyName>net_basetypes</AssemblyName>
    <Version>0.1.0</Version>
    <Company>LibreOffice</Company>
    <Description>Base datatypes for the .NET language UNO binding.</Description>
  </PropertyGroup>
  <ItemGroup>
")
    foreach (_src IN LISTS _net_basetypes_sources)
        file(APPEND "${_net_basetypes_proj}" "    <Compile Include=\"${_src}\"/>\n")
    endforeach ()
    file(APPEND "${_net_basetypes_proj}" "  </ItemGroup>
</Project>
")
    add_custom_command(
        OUTPUT "${_net_basetypes_dir}/net_basetypes.dll"
        COMMAND "${LIBRESTUDIO_DOTNET_EXECUTABLE}" build "${_net_basetypes_proj}"
                -c Release -o "${_net_basetypes_dir}"
        DEPENDS ${_net_basetypes_sources} "${_net_basetypes_proj}"
        WORKING_DIRECTORY "${_net_basetypes_dir}"
        COMMENT "net_ure: dotnet build net_basetypes.csproj")
    add_custom_target(net_basetypes ALL DEPENDS "${_net_basetypes_dir}/net_basetypes.dll")
    install(FILES "${_net_basetypes_dir}/net_basetypes.dll" DESTINATION program)

    # --- everything else in this module: blocked on codemaker's netmaker -----
    #
    # CustomTarget_net_uretypes.mk and CustomTarget_net_oootypes.mk both
    # invoke codemaker's `netmaker` executable (against udkapi and
    # udkapi+offapi respectively) to generate the C# UNO type bindings that
    # DotnetLibrary_net_uretypes/net_oootypes then compile; DotnetLibrary_
    # net_bridge is the real UNO<->CLR bridge built on top of all three.
    # codemaker/Executable_netmaker.mk exists in the source tree, but
    # codemaker/CMakeLists.txt says outright: "netmaker, pythonmaker and
    # rustmaker are for language bindings the core does not need. Their
    # sources are left in place and can be added the same way as
    # cppumaker/javamaker." -- i.e. netmaker is NOT wired in this port yet,
    # the same real, upstream-acknowledged prerequisite gap rust_uno/
    # CMakeLists.txt documents for rustmaker. CustomTarget_net_nuget_pkg.mk
    # (a NuGet package around net_bridge) and every net_ure Package_*.mk are
    # downstream of net_bridge and therefore blocked the same way.
    #
    # dotnet itself IS present and sufficiently new on this development
    # machine (verified: `dotnet --version` -> 9.0.111), so -- exactly like
    # rust_uno's cargo/rustc -- the blocker is the missing netmaker
    # prerequisite, not toolchain availability.
    message(WARNING
        "net_ure: ENABLE_DOTNET and ENABLE_ODK are both set, but codemaker's "
        "own netmaker executable (Executable_netmaker.mk) is not wired in "
        "codemaker/CMakeLists.txt yet -- CustomTarget_net_uretypes/"
        "net_oootypes' codegen step, and therefore DotnetLibrary_net_uretypes/"
        "net_oootypes/net_bridge and every net_ure Package_*, cannot "
        "actually build until that prerequisite lands. Only net_bootstrap "
        "and net_basetypes (both free of any netmaker dependency) are built "
        "by this CMakeLists.txt.")
endif ()
