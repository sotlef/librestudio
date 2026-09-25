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
#     OUTPUT_DIR <dir>)
#
# Produces <OUTPUT_DIR>/<SDI-basename>.hxx (the header) and
# <OUTPUT_DIR>/<SDI-basename> (the binary slot map). Sets <target>_HEADER and
# <target>_INCLUDE_DIR (== OUTPUT_DIR, for the #include <foo.hxx> consumers use).
function(librestudio_add_sdi_target target)
    cmake_parse_arguments(_sdi "" "SDI;EXPORTS;OUTPUT_DIR" "" ${ARGN})

    if (NOT _sdi_SDI OR NOT _sdi_EXPORTS OR NOT _sdi_OUTPUT_DIR)
        message(FATAL_ERROR
            "librestudio_add_sdi_target(${target}): SDI, EXPORTS and OUTPUT_DIR are required")
    endif ()

    get_filename_component(_sdi_dir "${_sdi_SDI}" DIRECTORY)
    get_filename_component(_sdi_name "${_sdi_SDI}" NAME_WE)

    set(_map "${_sdi_OUTPUT_DIR}/${_sdi_name}")
    set(_header "${_map}.hxx")

    file(GLOB _sdi_includes CONFIGURE_DEPENDS "${_sdi_dir}/*.sdi")

    add_custom_command(
        OUTPUT "${_map}" "${_header}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_sdi_OUTPUT_DIR}"
        COMMAND "$<TARGET_FILE:svidl>" -quiet
                "-I${CMAKE_SOURCE_DIR}/include" "-I${_sdi_dir}"
                "-fs${_header}" "-fx${_sdi_EXPORTS}" "-fm${_map}"
                "${_sdi_SDI}"
        WORKING_DIRECTORY "${_sdi_dir}"
        DEPENDS svidl "${_sdi_SDI}" "${_sdi_EXPORTS}" ${_sdi_includes}
        COMMENT "svidl: ${_sdi_name}"
        VERBATIM)

    add_custom_target(${target} DEPENDS "${_map}" "${_header}")

    set(${target}_HEADER "${_header}" PARENT_SCOPE)
    set(${target}_INCLUDE_DIR "${_sdi_OUTPUT_DIR}" PARENT_SCOPE)
endfunction ()
