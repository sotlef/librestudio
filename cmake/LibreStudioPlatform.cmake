# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Platform and compiler setup shared by every module.
#
# Mirrors what gbuild's solenv/gbuild/platform/*.mk set up per module: the CPPU
# environment tag, the RTL_OS/RTL_ARCH strings the source code stringifies, and
# the base definitions that sal and everything above it assume.

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR
        "The CMake build currently only supports Linux; CMAKE_SYSTEM_NAME is "
        "${CMAKE_SYSTEM_NAME}. Porting follows the same shape as "
        "solenv/gbuild/platform/, but has not been done yet.")
endif ()

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Only 64-bit targets are supported for now.")
endif ()

# The UNO loader tag for this ABI, as recorded in .component files and used to
# pick the C++ bridge library.  See solenv/gbuild/platform/com_GCC_defs.mk.
set(LIBRESTUDIO_CPPU_ENV "gcc3")
set(LIBRESTUDIO_RTL_OS "Linux")
set(LIBRESTUDIO_RTL_ARCH "x86_64")

# UNO libraries are versioned with the UDK major version; RTVERLIBS and
# UNOVERLIBS in Repository.mk get this appended (libuno_sal.so.3).
set(LIBRESTUDIO_UDK_MAJOR 3)

# Build artefacts are laid out like an installed LibreOffice, because the C++
# code assumes it: config_folders.h pins LIBO_*_FOLDER to "program", and
# cppu::getUnoIniUri() in cppuhelper/source/paths.cxx looks for unorc next to
# the library it is linked into.
set(LIBRESTUDIO_PROGRAM_DIR "${CMAKE_BINARY_DIR}/instdir/program")
set(LIBRESTUDIO_SHARE_DIR "${CMAKE_BINARY_DIR}/instdir/share")
set(LIBRESTUDIO_REGISTRY_DIR "${CMAKE_BINARY_DIR}/instdir/share/registry")

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/instdir" CACHE PATH
        "Install prefix" FORCE)
endif ()

foreach (_dir "${LIBRESTUDIO_PROGRAM_DIR}" "${LIBRESTUDIO_SHARE_DIR}")
    file(MAKE_DIRECTORY "${_dir}")
endforeach ()

# Match gbuild: libraries and executables all land in one directory, so that
# dlopen-by-name and $ORIGIN expansion behave the same at build and run time.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${LIBRESTUDIO_PROGRAM_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${LIBRESTUDIO_PROGRAM_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif ()

# The source tree is 7M lines of upstream code; building it with the warnings of
# a fresh project is not useful.  gbuild compiles upstream with the project's
# own warning policy, which we do not attempt to reproduce here.
add_compile_options(-fPIC -fno-strict-aliasing)

# gbuild links shared libraries with -Wl,-z,defs
# (solenv/gbuild/platform/linux.mk), which turns "forgot to link a library" from
# a runtime crash into a build error.  That is especially valuable while porting.
add_link_options(-Wl,-z,defs -Wl,-z,noexecstack -Wl,--as-needed)

# Definitions every translation unit sees, matching gbuild's gb_OSDEFS and the
# per-library -D flags in Library_*.mk.
add_compile_definitions(
    CPPU_ENV=${LIBRESTUDIO_CPPU_ENV}
    RTL_OS="${LIBRESTUDIO_RTL_OS}"
    RTL_ARCH="${LIBRESTUDIO_RTL_ARCH}"
    LIBO_INTERNAL_ONLY
    LINUX
    UNX
    SRCDIR="${CMAKE_SOURCE_DIR}"
)

# sal/osl/unx/log.cxx and friends normalise paths against the source root, and
# several tools embed it.  Quoted separately because of the spaces-hostile CI
# setups gbuild warns about; CMake handles the escaping.
include_directories(
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_BINARY_DIR}/config_host"
)

# gbuild names objects without an extension and relative to the source root
# (gb_Library_add_exception_objects,sal, sal/rtl/string).  Transcription into
# CMake is far easier to check against the .mk when the names are kept in that
# spelling, so source lists are written the same way and expanded here — into
# absolute paths, since CMake would otherwise resolve them against the module
# directory rather than the tree root.
macro(librestudio_expand_cxx _list)
    set(_expanded "")
    foreach (_src IN LISTS ${_list})
        if (IS_ABSOLUTE "${_src}")
            list(APPEND _expanded "${_src}")
        else ()
            list(APPEND _expanded "${CMAKE_SOURCE_DIR}/${_src}.cxx")
        endif ()
    endforeach ()
    set(${_list} "${_expanded}")
    unset(_expanded)
endmacro()

# A helper for the library naming rules in solenv/gbuild/platform/unxgcc.mk.
#
#   UNO  -> libuno_<name>.so.<UDK_MAJOR>          (sal, cppu)
#   RT   -> libuno_<name>gcc3.so.<UDK_MAJOR>      (salhelper, cppuhelper)
#   OOO  -> lib<name>lo.so                        (everything else)
#
# RepositoryFixes.mk redirects every RTVERLIBS entry — cppuhelper, salhelper,
# purpenvhelper — to the libuno_ prefix on GCC, so the UNO runtime libraries all
# share it.
#
# The name matters beyond aesthetics: .component files reference the runtime
# filename through vnd.sun.star.expand, and the UNO service manager dlopens it.
function(librestudio_set_library_output_name target name layer)
    if (layer STREQUAL "UNO")
        set_target_properties(${target} PROPERTIES
            OUTPUT_NAME "uno_${name}"
            VERSION "${LIBRESTUDIO_UDK_MAJOR}"
            SOVERSION "${LIBRESTUDIO_UDK_MAJOR}")
    elseif (layer STREQUAL "RT")
        set_target_properties(${target} PROPERTIES
            OUTPUT_NAME "uno_${name}${LIBRESTUDIO_CPPU_ENV}"
            VERSION "${LIBRESTUDIO_UDK_MAJOR}"
            SOVERSION "${LIBRESTUDIO_UDK_MAJOR}")
    elseif (layer STREQUAL "OOO")
        set_target_properties(${target} PROPERTIES OUTPUT_NAME "${name}lo")
    elseif (layer STREQUAL "PLAIN")
        set_target_properties(${target} PROPERTIES OUTPUT_NAME "${name}")
    else ()
        message(FATAL_ERROR "Unknown library layer '${layer}' for target ${target}")
    endif ()
endfunction()
