# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libixion, invoked by librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake), called recursively from external/orcus/
# orcus.cmake -- the only real caller, since libixion has no consumer
# outside external/ -- only when no sufficiently new system libixion was
# found (or LIBRESTUDIO_FORCE_VENDOR_IXION asks for it unconditionally).
#
# Unlike every other vendored external in this port, this one is a real
# add_subdirectory() of a third-party CMake project, not a hand-assembled
# add_library()/ExternalProject_Add() -- so it exports LIBRESTUDIO_IXION_
# VENDORED_INCLUDE_DIR/_LIB_DIR too (only meaningful on this branch), which
# external/orcus/orcus.cmake's own driver needs to build MDDS_CFLAGS/
# LIBIXION_CFLAGS/LIBIXION_LIBS-style values for orcus's own ./configure
# when ixion itself was vendored (it has no .pc file yet at that point).
function(librestudio_vendor_ixion)
    message(STATUS "libixion: no system package >= 0.20.0 found (or vendoring forced) -- building ixion 0.20.0 from source")

    set(_ixion_tarball_sha256 "4a6c2c480ad40b706ecf459dfca03f39351e12b48911c7c4803b75c823a1bcb1")
    set(_ixion_cache "${LIBRESTUDIO_TARBALL_CACHE}/ixion-0.20.0.tar.gz")
    file(MAKE_DIRECTORY "${LIBRESTUDIO_TARBALL_CACHE}")
    if (NOT EXISTS "${_ixion_cache}")
        message(STATUS "Downloading ixion-0.20.0.tar.gz")
        file(DOWNLOAD "https://gitlab.com/ixion/ixion/-/archive/0.20.0/ixion-0.20.0.tar.gz"
             "${_ixion_cache}.tmp"
             EXPECTED_HASH SHA256=${_ixion_tarball_sha256}
             TLS_VERIFY ON
             STATUS _ixion_dl_status)
        list(GET _ixion_dl_status 0 _ixion_dl_code)
        if (NOT _ixion_dl_code EQUAL 0)
            list(GET _ixion_dl_status 1 _ixion_dl_msg)
            file(REMOVE "${_ixion_cache}.tmp")
            message(FATAL_ERROR "Failed to download ixion-0.20.0.tar.gz: ${_ixion_dl_msg}")
        endif ()
        file(RENAME "${_ixion_cache}.tmp" "${_ixion_cache}")
    endif ()

    set(_ixion_dest "${CMAKE_BINARY_DIR}/_deps/ixion")
    set(_ixion_stamp "${_ixion_dest}/.unpacked-${_ixion_tarball_sha256}")
    if (NOT EXISTS "${_ixion_stamp}")
        file(REMOVE_RECURSE "${_ixion_dest}")
        file(MAKE_DIRECTORY "${_ixion_dest}")
        file(ARCHIVE_EXTRACT INPUT "${_ixion_cache}" DESTINATION "${_ixion_dest}")
        file(TOUCH "${_ixion_stamp}")
    endif ()
    set(ixion_SOURCE_DIR "${_ixion_dest}/ixion-0.20.0")

    # ixion's own src/CMakeLists.txt unconditionally add_subdirectory()s
    # python/ (needs a real Python3 "Development.Module" component, not
    # guaranteed present here) and test/ (its own mini test harness) --
    # neither is needed to produce libixion-0.20.so itself. Patched out of
    # this vendored copy once at unpack time: ixion is a brand-new external
    # dependency this port itself is introducing, not part of LibreOffice's
    # own source tree (which this project never patches), so a light
    # integration patch to a freshly-fetched third-party build script is not
    # the same category of change as editing LibreOffice sources.
    if (NOT EXISTS "${ixion_SOURCE_DIR}/.librestudio-patched")
        file(READ "${ixion_SOURCE_DIR}/src/CMakeLists.txt" _ixion_src_cmake)
        string(REPLACE "add_subdirectory(python)"
            "# add_subdirectory(python) -- removed by LibreStudio, needs a Python3 Development.Module component this port does not guarantee"
            _ixion_src_cmake "${_ixion_src_cmake}")
        string(REPLACE "add_subdirectory(test)"
            "# add_subdirectory(test) -- removed by LibreStudio, not needed to produce the library itself"
            _ixion_src_cmake "${_ixion_src_cmake}")
        file(WRITE "${ixion_SOURCE_DIR}/src/CMakeLists.txt" "${_ixion_src_cmake}")
        file(WRITE "${ixion_SOURCE_DIR}/.librestudio-patched" "")
    endif ()

    set(MDDS_INCLUDEDIR "${MDDS_INCLUDE_DIR}" CACHE STRING "" FORCE)
    set(BUILD_VULKAN OFF CACHE BOOL "" FORCE)
    add_subdirectory("${ixion_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/_deps/ixion-build" EXCLUDE_FROM_ALL)

    set(LIBRESTUDIO_IXION_VENDORED_INCLUDE_DIR "${ixion_SOURCE_DIR}/include" CACHE INTERNAL "" FORCE)
    # CMAKE_RUNTIME_OUTPUT_DIRECTORY/CMAKE_LIBRARY_OUTPUT_DIRECTORY
    # (LibreStudioPlatform.cmake) redirect every shared library in this whole
    # build -- ixion-0.20 included, even though add_subdirectory()'d from a
    # _deps/ subtree -- into LIBRESTUDIO_PROGRAM_DIR, not the per-target
    # binary dir a plain CMake project would default to. Verified directly:
    # the built .so lands at ${LIBRESTUDIO_PROGRAM_DIR}/libixion-0.20.so, not
    # under _deps/ixion-build/ as a naive guess would assume.
    set(LIBRESTUDIO_IXION_VENDORED_LIB_DIR "${LIBRESTUDIO_PROGRAM_DIR}" CACHE INTERNAL "" FORCE)

    librestudio_vendor_set_target(IXION ixion-0.20)
endfunction()
