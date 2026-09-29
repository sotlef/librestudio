# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Compiling *.sdi (sfx2's slot/dispatch definitions) into the C++ header
# consumers #include and the binary slot map other .sdi files SlotIdFile()
# against. Upstream's equivalent is solenv/gbuild/SdiTarget.mk; svidl (idl/)
# is the compiler.

# librestudio_add_sdi_target(<target>
#     SDI        <file.sdi>   # the module's top-level slot file, e.g.
#                              # sfx2/sdi/sfxslots.sdi -- it #includes the
#                              # other .sdi files in its own directory, so
#                              # svidl runs with that directory as its cwd
#     EXPORTS    <file.sdi>   # the slot ID master list, e.g. sfx2/sdi/sfx.sdi
#     OUTPUT_DIR <dir>
#     [INCLUDES  <dir>...])   # extra -I search dirs, for a module's .sdi
#                              # files that #include another module's (e.g.
#                              # svx/sdi/svxslots.sdi includes sfxitems.sdi
#                              # from sfx2/sdi -- gb_SdiTarget_set_include's
#                              # equivalent for svx)
#
# Produces <OUTPUT_DIR>/<SDI-basename>.hxx (the header) and
# <OUTPUT_DIR>/<SDI-basename> (the binary slot map). Sets <target>_HEADER and
# <target>_INCLUDE_DIR (== OUTPUT_DIR, for the #include <foo.hxx> consumers use).
function(librestudio_add_sdi_target target)
    cmake_parse_arguments(_sdi "" "SDI;EXPORTS;OUTPUT_DIR" "INCLUDES" ${ARGN})

    if (NOT _sdi_SDI OR NOT _sdi_EXPORTS OR NOT _sdi_OUTPUT_DIR)
        message(FATAL_ERROR
            "librestudio_add_sdi_target(${target}): SDI, EXPORTS and OUTPUT_DIR are required")
    endif ()

    get_filename_component(_sdi_dir "${_sdi_SDI}" DIRECTORY)
    get_filename_component(_sdi_name "${_sdi_SDI}" NAME_WE)

    set(_map "${_sdi_OUTPUT_DIR}/${_sdi_name}")
    set(_header "${_map}.hxx")

    file(GLOB _sdi_includes CONFIGURE_DEPENDS "${_sdi_dir}/*.sdi")

    set(_extra_i_flags "")
    foreach (_inc ${_sdi_INCLUDES})
        list(APPEND _extra_i_flags "-I${_inc}")
        file(GLOB _more_includes CONFIGURE_DEPENDS "${_inc}/*.sdi")
        list(APPEND _sdi_includes ${_more_includes})
    endforeach ()

    add_custom_command(
        OUTPUT "${_map}" "${_header}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_sdi_OUTPUT_DIR}"
        COMMAND "$<TARGET_FILE:svidl>" -quiet
                "-I${CMAKE_SOURCE_DIR}/include" "-I${_sdi_dir}" ${_extra_i_flags}
                "-fs${_header}" "-fx${_sdi_EXPORTS}" "-fm${_map}"
                "${_sdi_SDI}"
        WORKING_DIRECTORY "${_sdi_dir}"
        DEPENDS svidl "${_sdi_SDI}" "${_sdi_EXPORTS}" ${_sdi_includes}
        COMMENT "svidl: ${_sdi_name}"
        VERBATIM)

    add_custom_target(${target} DEPENDS "${_map}" "${_header}")

    # 2026-09-29, found via the real remote build: a plain PARENT_SCOPE set()
    # only reaches the directory scope that directly CALLS this function
    # (sd/CMakeLists.txt's own scope, in sd's case) -- it does not reach a
    # *_deferred.cmake file, which include()s back into the ROOT directory's
    # own separate scope. sd/CppunitTest_deferred.cmake's sd_uimpress target
    # references ${sd_sdslots_INCLUDE_DIR} in its own INCLUDE list, but that
    # variable was empty there (cross-scope, same visibility gap already
    # documented repeatedly in this tree for PkgConfig::CPPUNIT and friends),
    # so `#include <sdslots.hxx>` failed with a real "No such file or
    # directory" on the remote test build. CACHE INTERNAL makes both
    # variables visible everywhere, the same fix already used throughout this
    # port for exactly this class of cross-directory-scope problem.
    set(${target}_HEADER "${_header}" CACHE INTERNAL "" FORCE)
    set(${target}_INCLUDE_DIR "${_sdi_OUTPUT_DIR}" CACHE INTERNAL "" FORCE)
endfunction ()
