# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Building UNO type registries (.rdb) and the C++ bindings generated from them.
#
# These two steps are what everything above sal depends on, and they are
# deliberately split, because the tools that perform them can only be built
# before any binding exists:
#
#   .idl tree --unoidl-write--> .rdb --cppumaker--> com/sun/star/**/*.hpp
#
# Upstream's equivalent is solenv/gbuild/UnoApiTarget.mk; the corresponding
# CMake invocations are documented against each function below.

# librestudio_uno_api_rdb(<name>
#     IDL_ROOT <dir>          # directory whose subdirectories are the type trees
#     OUTPUT   <file.rdb>
#     [EXCLUDE <subdir>...]   # subdirectories to keep out of the scan
#     [DEP_RDBS <rdb>...])    # registries this one extends
#
# Creates target <name>_rdb and sets <name>_RDB to the produced file.
function(librestudio_uno_api_rdb name)
    cmake_parse_arguments(_rdb "" "IDL_ROOT;OUTPUT" "EXCLUDE;DEP_RDBS" ${ARGN})

    if (NOT _rdb_IDL_ROOT OR NOT _rdb_OUTPUT)
        message(FATAL_ERROR "librestudio_uno_api_rdb(${name}): IDL_ROOT and OUTPUT are required")
    endif ()
    if (NOT _rdb_EXCLUDE)
        set(_rdb_EXCLUDE type_reference)
    endif ()

    # unoidl-write walks a directory tree and rejects any file that declares more
    # than one entity — which is exactly what the type_reference dumps are: they
    # exist for unoidl-check, not as sources.  Everything else is staged as-is,
    # because a type's fully qualified name is built from its path relative to
    # IDL_ROOT, so the tree shape has to be preserved.  offapi keeps its types
    # under both com/ and org/, which is why this walks the subdirectories rather
    # than assuming a single root.
    set(_staging "${CMAKE_CURRENT_BINARY_DIR}/${name}-idl")

    file(GLOB _root_entries LIST_DIRECTORIES true "${_rdb_IDL_ROOT}/*")
    set(_stage_commands)
    set(_idl_files)
    foreach (_entry IN LISTS _root_entries)
        if (NOT IS_DIRECTORY "${_entry}")
            continue ()
        endif ()
        get_filename_component(_entry_name "${_entry}" NAME)
        if (_entry_name IN_LIST _rdb_EXCLUDE)
            continue ()
        endif ()
        list(APPEND _stage_commands
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${_entry}" "${_staging}/${_entry_name}")
        file(GLOB_RECURSE _entry_idls CONFIGURE_DEPENDS "${_entry}/*.idl")
        list(APPEND _idl_files ${_entry_idls})
    endforeach ()

    if (NOT _stage_commands)
        message(FATAL_ERROR
            "librestudio_uno_api_rdb(${name}): no .idl directories found under ${_rdb_IDL_ROOT}")
    endif ()

    add_custom_command(
        OUTPUT "${_rdb_OUTPUT}"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${_staging}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_staging}"
        ${_stage_commands}
        COMMAND "$<TARGET_FILE:unoidl-write>" ${_rdb_DEP_RDBS} "${_staging}" "${_rdb_OUTPUT}"
        DEPENDS unoidl-write ${_idl_files} ${_rdb_DEP_RDBS}
        COMMENT "unoidl-write: ${name}.rdb"
        VERBATIM)

    add_custom_target(${name}_rdb DEPENDS "${_rdb_OUTPUT}")
    # CACHE INTERNAL rather than PARENT_SCOPE: these describe a module that other,
    # sibling modules consume, and PARENT_SCOPE from inside a function only
    # reaches the calling directory.
    set(${name}_RDB "${_rdb_OUTPUT}" CACHE INTERNAL "${name} type registry")
endfunction ()

# librestudio_uno_api_headers(<name>
#     RDB     <file.rdb>
#     OUTPUT_DIR <dir>
#     VARIANT normal|comprehensive|bootstrap
#     [EXTRA_RDBS <rdb>...])
#
# Runs cppumaker over <rdb> and sets <name>_HEADERS_TARGET and
# <name>_HEADERS_INCLUDE_DIR.
#
# The variant selects how much type information is inlined into the headers and
# is not a free choice per consumer: upstream's select_variant maps
# gb_*_use_udk_api to normal (-L), gb_*_use_internal_comprehensive_api to
# comprehensive (-C), and stoc/bootstrap to the bootstrap variant (no flag).
function(librestudio_uno_api_headers name)
    cmake_parse_arguments(_hdr "" "RDB;OUTPUT_DIR;VARIANT" "EXTRA_RDBS" ${ARGN})

    if (NOT _hdr_RDB OR NOT _hdr_OUTPUT_DIR OR NOT _hdr_VARIANT)
        message(FATAL_ERROR
            "librestudio_uno_api_headers(${name}): RDB, OUTPUT_DIR and VARIANT are required")
    endif ()

    if (_hdr_VARIANT STREQUAL "normal")
        set(_variant_flag "-L")
    elseif (_hdr_VARIANT STREQUAL "comprehensive")
        set(_variant_flag "-C")
    elseif (_hdr_VARIANT STREQUAL "bootstrap")
        set(_variant_flag "")
    else ()
        message(FATAL_ERROR
            "librestudio_uno_api_headers(${name}): unknown VARIANT '${_hdr_VARIANT}'")
    endif ()

    set(_extra_flags "")
    foreach (_extra IN LISTS _hdr_EXTRA_RDBS)
        list(APPEND _extra_flags "-X${_extra}")
    endforeach ()

    # cppumaker writes one file per entity, so the stamp is what the build graph
    # tracks.  -Gc makes cppumaker leave a file's timestamp alone when its
    # content did not change (codemaker/source/codemaker/global.cxx), without
    # which every rebuild would look like a change to every header.
    set(_stamp "${_hdr_OUTPUT_DIR}/.generated")

    add_custom_command(
        OUTPUT "${_stamp}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_hdr_OUTPUT_DIR}"
        COMMAND "$<TARGET_FILE:cppumaker>" -Gc ${_variant_flag}
                "-O${_hdr_OUTPUT_DIR}" "${_hdr_RDB}" ${_extra_flags}
        COMMAND ${CMAKE_COMMAND} -E touch "${_stamp}"
        DEPENDS cppumaker "${_hdr_RDB}" ${_hdr_EXTRA_RDBS}
        COMMENT "cppumaker: ${name} (${_hdr_VARIANT})"
        VERBATIM)

    add_custom_target(${name}_headers DEPENDS "${_stamp}")

    set(${name}_HEADERS_TARGET "${name}_headers" CACHE INTERNAL "${name} headers target")
    set(${name}_HEADERS_INCLUDE_DIR "${_hdr_OUTPUT_DIR}" CACHE INTERNAL
        "${name} generated C++ headers")
endfunction ()
