# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# officecfg — the configuration schema.
#
# Only the C++ side is built here so far: officecfg/registry/cppheader.xsl turns
# each .xcs schema into a typed header (officecfg::Office::Common::Save::…),
# which is what comphelper's embeddedobjectcontainer.cxx reads configuration
# through.  Upstream's equivalent is officecfg/CustomTarget_registry.mk.
#
# The .xcu side of this module (packing the schemas into the registry) belongs
# with the component work and is not built yet.

find_program(XSLTPROC_EXECUTABLE xsltproc REQUIRED)

set(OFFICECFG_SCHEMA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/registry/schema/org/openoffice")
set(OFFICECFG_CPPHEADER_XSL "${CMAKE_CURRENT_SOURCE_DIR}/registry/cppheader.xsl")
set(OFFICECFG_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/registry")
set(OFFICECFG_HEADER_ROOT "${OFFICECFG_INCLUDE_DIR}/officecfg")

file(GLOB_RECURSE OFFICECFG_XCS_FILES
    RELATIVE "${OFFICECFG_SCHEMA_ROOT}" "${OFFICECFG_SCHEMA_ROOT}/*.xcs")

set(OFFICECFG_HEADERS)
foreach (_xcs IN LISTS OFFICECFG_XCS_FILES)
    string(REGEX REPLACE "\\.xcs$" "" _base "${_xcs}")

    # cppheader.xsl namespaces the generated C++ by the schema's path segments:
    # Office/Common.xcs becomes officecfg::Office::Common, and the stylesheet is
    # told each segment through its own nsN parameter.
    string(REPLACE "/" ";" _segments "${_base}")
    set(_ns_params)
    set(_index 1)
    foreach (_segment IN LISTS _segments)
        list(APPEND _ns_params "--stringparam" "ns${_index}" "${_segment}")
        math(EXPR _index "${_index} + 1")
    endforeach ()

    get_filename_component(_out_dir "${OFFICECFG_HEADER_ROOT}/${_base}" DIRECTORY)
    set(_out "${OFFICECFG_HEADER_ROOT}/${_base}.hxx")

    add_custom_command(
        OUTPUT "${_out}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_out_dir}"
        COMMAND "${XSLTPROC_EXECUTABLE}" --nonet ${_ns_params}
                -o "${_out}" "${OFFICECFG_CPPHEADER_XSL}" "${OFFICECFG_SCHEMA_ROOT}/${_xcs}"
        DEPENDS "${OFFICECFG_SCHEMA_ROOT}/${_xcs}" "${OFFICECFG_CPPHEADER_XSL}"
        COMMENT "cppheader.xsl: officecfg/${_base}.hxx"
        VERBATIM)

    list(APPEND OFFICECFG_HEADERS "${_out}")
endforeach ()

add_custom_target(officecfg_headers DEPENDS ${OFFICECFG_HEADERS})

set(OFFICECFG_INCLUDE_DIR "${OFFICECFG_INCLUDE_DIR}" CACHE INTERNAL
    "officecfg generated configuration headers")
