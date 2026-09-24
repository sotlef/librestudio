# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Fetching of third-party header-only dependencies.
#
# dragonbox, fast_float and frozen are not vendored in the source tree: upstream
# ships them as tarballs listed in download.lst and unpacks them into workdir/.
# We do the same, minus the gbuild machinery — file(DOWNLOAD) checks the recorded
# SHA256, file(ARCHIVE_EXTRACT) unpacks once at configure time.
#
# Tarballs are cached between build trees in LIBRESTUDIO_TARBALL_CACHE so that
# reconfiguring or building a second tree does not hit the network again.

set(LIBRESTUDIO_TARBALL_URL "https://dev-www.libreoffice.org/src"
    CACHE STRING "Base URL for third-party source tarballs")

set(LIBRESTUDIO_TARBALL_CACHE "${CMAKE_BINARY_DIR}/_tarballs"
    CACHE PATH "Directory where downloaded tarballs are cached")

# librestudio_fetch_tarball(<name> <tarball> <sha256>)
#
# Downloads <tarball> from LIBRESTUDIO_TARBALL_URL unless a copy with a matching
# SHA256 is already cached, unpacks it into <build>/_deps/<name>, and sets
# <name>_SOURCE_DIR in the caller's scope to the tarball's top-level directory
# (or to the extraction directory itself when the tarball has no single
# top-level directory).
function(librestudio_fetch_tarball name tarball sha256)
    set(_cache_file "${LIBRESTUDIO_TARBALL_CACHE}/${tarball}")
    set(_dest "${CMAKE_BINARY_DIR}/_deps/${name}")
    set(_stamp "${_dest}/.unpacked-${sha256}")

    file(MAKE_DIRECTORY "${LIBRESTUDIO_TARBALL_CACHE}")

    if (NOT EXISTS "${_cache_file}")
        message(STATUS "Downloading ${tarball}")
        file(DOWNLOAD "${LIBRESTUDIO_TARBALL_URL}/${tarball}" "${_cache_file}.tmp"
             EXPECTED_HASH SHA256=${sha256}
             TLS_VERIFY ON
             STATUS _dl_status)
        list(GET _dl_status 0 _dl_code)
        if (NOT _dl_code EQUAL 0)
            list(GET _dl_status 1 _dl_msg)
            file(REMOVE "${_cache_file}.tmp")
            message(FATAL_ERROR
                "Failed to download ${LIBRESTUDIO_TARBALL_URL}/${tarball}: ${_dl_msg}")
        endif ()
        file(RENAME "${_cache_file}.tmp" "${_cache_file}")
    endif ()

    if (NOT EXISTS "${_stamp}")
        file(REMOVE_RECURSE "${_dest}")
        file(MAKE_DIRECTORY "${_dest}")
        file(ARCHIVE_EXTRACT INPUT "${_cache_file}" DESTINATION "${_dest}")
        file(TOUCH "${_stamp}")
    endif ()

    # A well-formed source tarball contains exactly one top-level directory.
    # Fall back to the extraction root if that is not the case.
    file(GLOB _entries RELATIVE "${_dest}" "${_dest}/*")
    list(FILTER _entries EXCLUDE REGEX "^\\.unpacked-")
    list(LENGTH _entries _count)
    if (_count EQUAL 1 AND IS_DIRECTORY "${_dest}/${_entries}")
        set(_source_dir "${_dest}/${_entries}")
    else ()
        set(_source_dir "${_dest}")
    endif ()

    set(${name}_SOURCE_DIR "${_source_dir}" PARENT_SCOPE)
endfunction()
