# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Zip: a generic CMake transcription of gbuild's Zip class
# (solenv/gbuild/Zip.mk, gb_Zip_Zip / gb_Zip_add_file), used tree-wide
# wherever a module packages a handful of source-tree files into a plain
# .zip-format archive at build time (e.g. an ODF document assembled from
# loose XML parts, as smoketest/Zip_smoketestdoc.mk does -- see
# smoketest/CMakeLists.txt for that call site). Built as a small reusable
# helper here rather than an inline add_custom_command in each caller,
# since gb_Zip_Zip is itself a generic primitive upstream, not specific to
# any one module.
#
# Upstream mechanism (solenv/gbuild/Zip.mk), for reference:
#   - gb_Zip_Zip(name, sourcedir) registers the archive's LOCATION
#     (sourcedir, since zip stores paths relative to where it is invoked)
#     and clears its FILES list.
#   - gb_Zip_add_file(name, relative-path) appends one path to FILES and
#     adds a prerequisite on sourcedir/relative-path.
#   - The actual recipe (gb_Zip_get_target rule) writes FILES to a
#     temporary response file (to dodge ARG_MAX on large file counts),
#     cds into LOCATION, and runs, in effect:
#       zip $(-q if make is silent) -@rX --filesync --must-match <output> \
#           < response-file-of-names
#     i.e. "-@" (read the member list from stdin), "r" (recurse -- moot
#     here since every entry is already a plain file, not a directory),
#     "X" (strip extra file attributes/timestamps for reproducibility),
#     plus "--filesync" (drop any existing archive member not in FILES)
#     and "--must-match" (fail if a listed member matches nothing).
#   - gb_Zip_add_commandoptions additionally lets a caller append flags;
#     every current gbuild caller (smoketest's own Zip_smoketestdoc.mk
#     included) uses this to add "-D" -- no directory entries stored in
#     the archive, confirmed by reading that file directly.
#   - If FILES ends up empty, upstream copies solenv/gbuild/empty.zip
#     instead of invoking zip at all (zip itself refuses to create an
#     archive with zero members). Not reproduced below: every caller
#     ported so far (smoketestdoc) has a non-empty, statically known FILES
#     list, so this is left as a documented gap for a future caller that
#     needs it, not silently assumed away.
#
# Simplifications made in this CMake transcription, and why each is safe
# for the file counts this port's callers actually have (a few dozen at
# most, nowhere near ARG_MAX):
#   - The member list is passed to zip directly as command-line arguments
#     rather than via "-@" + a stdin response file. Same resulting
#     archive; the response-file indirection upstream uses exists purely
#     to survive very large file counts, which is not a concern here.
#   - "--filesync"/"--must-match" (upstream's way of keeping a
#     *pre-existing* archive's membership in sync with FILES across
#     incremental rebuilds) are replaced by unconditionally deleting any
#     stale output file before re-zipping from scratch each time the
#     command reruns (CMake/Ninja already only reruns this recipe when a
#     dependency changed, via the OUTPUT/DEPENDS below) -- simpler than
#     reproducing zip's own incremental-sync flags, and equally correct
#     for a full rebuild.
#   - "-q" (quiet) is applied unconditionally, rather than only under
#     make's own "-s" silent-mode detection (there is no equivalent
#     concept to probe generically from within a CMake function) -- a
#     purely cosmetic difference in build-log verbosity.
#
# librestudio_add_zip(<name>
#     SOURCEDIR <dir>
#     OUTPUT <archive-file>
#     FILES <relative-path>...)
#
# SOURCEDIR: the directory FILES entries are resolved against, and the
#   working directory zip is invoked from (so the archive stores each
#   member under its given relative path, not an absolute one) --
#   gb_Zip_Zip's own second argument.
# OUTPUT: the archive's full output path -- gb_Zip_get_target's
#   equivalent. Parent directories are created as needed.
# FILES: paths relative to SOURCEDIR, one gb_Zip_add_file call each
#   upstream. Every entry becomes both a zip member and a build
#   dependency (SOURCEDIR/<entry> must exist before the archive can be
#   built).
#
# Creates a custom target <name> (so `cmake --build . --target <name>`
# works standalone, the same way gb_Helper_make_userfriendly_targets does
# upstream) that depends on OUTPUT; callers that need the archive as a
# prerequisite of something else should add_dependencies(... <name>) or
# depend on OUTPUT directly.
function(librestudio_add_zip name)
    cmake_parse_arguments(_az "" "SOURCEDIR;OUTPUT" "FILES" ${ARGN})

    if (NOT _az_SOURCEDIR)
        message(FATAL_ERROR "librestudio_add_zip(${name}): SOURCEDIR is required")
    endif ()
    if (NOT _az_OUTPUT)
        message(FATAL_ERROR "librestudio_add_zip(${name}): OUTPUT is required")
    endif ()
    if (NOT _az_FILES)
        message(FATAL_ERROR "librestudio_add_zip(${name}): FILES is required")
    endif ()
    if (_az_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "librestudio_add_zip(${name}): unknown arguments: ${_az_UNPARSED_ARGUMENTS}")
    endif ()

    # Guarded the same way cmake/LibreStudioTest.cmake guards its own
    # PkgConfig::CPPUNIT lookup: find_program()'s result is a CACHE
    # variable, so a second find_program() call is harmless on its own,
    # but the explicit guard keeps this function's own behaviour obvious
    # regardless of how many modules call it. XSLTPROC_EXECUTABLE
    # (officecfg/CMakeLists.txt, and the root CMakeLists.txt) is this
    # port's established style for a required external tool lookup:
    # find_program(<VAR> <name> REQUIRED), which fails configure outright
    # (with CMake's own clear error message) if the tool is missing --
    # mirrored here rather than inventing a separate hand-written
    # NOT-found message.
    if (NOT ZIP_EXECUTABLE)
        find_program(ZIP_EXECUTABLE zip REQUIRED)
    endif ()

    set(_az_deps "")
    foreach (_az_f IN LISTS _az_FILES)
        list(APPEND _az_deps "${_az_SOURCEDIR}/${_az_f}")
    endforeach ()

    get_filename_component(_az_output_dir "${_az_OUTPUT}" DIRECTORY)

    add_custom_command(
        OUTPUT "${_az_OUTPUT}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_az_output_dir}"
        COMMAND "${CMAKE_COMMAND}" -E rm -f "${_az_OUTPUT}"
        COMMAND "${ZIP_EXECUTABLE}" -q -D -X -r "${_az_OUTPUT}" ${_az_FILES}
        WORKING_DIRECTORY "${_az_SOURCEDIR}"
        DEPENDS ${_az_deps}
        COMMENT "Building zip archive ${name} (${_az_OUTPUT})"
        VERBATIM)

    add_custom_target(${name} DEPENDS "${_az_OUTPUT}")
endfunction()
