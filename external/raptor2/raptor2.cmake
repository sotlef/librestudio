# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for raptor2, invoked by librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake), called recursively from external/rasqal/
# rasqal.cmake and external/redland/redland.cmake -- raptor2 has no real
# consumer outside external/, so there is no external/raptor2/CMakeLists.txt
# at all (unlike zlib/md4c/graphite/etc, which still have one real
# top-level entry point). Upstream LibreOffice's own use_externals(unordf,
# ... librdf redland_headers raptor_headers rasqal_headers ...) is a "find a
# system package or fail" external -- this fork's download.lst does not
# vendor any of raptor2/rasqal/redland, so this port fetches its own copies
# from librdf.org's own release mirror when no sufficiently new system
# package exists.
#
# rasqal's own configure.ac needs raptor2 (its own hard-coded `$PKG_CONFIG
# raptor2 --modversion` version recheck at configure.ac:428 bypasses the
# usual "skip pkg-config if <PREFIX>_CFLAGS/_LIBS are pre-set" convention),
# and redland's own configure.ac needs both. Rather than fight that
# per-library inconsistency, every one of the three -- whichever branch
# (system or vendored) resolves it -- writes a synthetic, uninstalled .pc
# file into one shared scratch directory (librestudio_write_uninstalled_pc(),
# cmake/LibreStudioTarballs.cmake), always put first on PKG_CONFIG_PATH for
# every subsequent configure step in this chain.
function(librestudio_vendor_raptor2)
    message(STATUS "raptor2: no system package >= 2.0.7 found (or vendoring forced) -- building raptor2 2.0.16 from source")

    set(_raptor2_tarball_sha256 "089db78d7ac982354bdbf39d973baf09581e6904ac4c92a98c5caadb3de44680")
    set(_raptor2_cache "${LIBRESTUDIO_TARBALL_CACHE}/raptor2-2.0.16.tar.gz")
    file(MAKE_DIRECTORY "${LIBRESTUDIO_TARBALL_CACHE}")
    if (NOT EXISTS "${_raptor2_cache}")
        message(STATUS "Downloading raptor2-2.0.16.tar.gz")
        file(DOWNLOAD "https://download.librdf.org/source/raptor2-2.0.16.tar.gz"
             "${_raptor2_cache}.tmp"
             EXPECTED_HASH SHA256=${_raptor2_tarball_sha256}
             TLS_VERIFY ON
             STATUS _raptor2_dl_status)
        list(GET _raptor2_dl_status 0 _raptor2_dl_code)
        if (NOT _raptor2_dl_code EQUAL 0)
            list(GET _raptor2_dl_status 1 _raptor2_dl_msg)
            file(REMOVE "${_raptor2_cache}.tmp")
            message(FATAL_ERROR "Failed to download raptor2-2.0.16.tar.gz: ${_raptor2_dl_msg}")
        endif ()
        file(RENAME "${_raptor2_cache}.tmp" "${_raptor2_cache}")
    endif ()

    set(_raptor2_dest "${CMAKE_BINARY_DIR}/_deps/raptor2")
    set(_raptor2_stamp "${_raptor2_dest}/.unpacked-${_raptor2_tarball_sha256}")
    if (NOT EXISTS "${_raptor2_stamp}")
        file(REMOVE_RECURSE "${_raptor2_dest}")
        file(MAKE_DIRECTORY "${_raptor2_dest}")
        file(ARCHIVE_EXTRACT INPUT "${_raptor2_cache}" DESTINATION "${_raptor2_dest}")
        file(TOUCH "${_raptor2_stamp}")
    endif ()
    set(raptor2_SOURCE_DIR "${_raptor2_dest}/raptor2-2.0.16")

    # raptor2 2.0.16 (released 2023) accesses xmlEntity's own "checked" field
    # directly (src/raptor_libxml.c) -- a field libxml2 >= 2.12 removed from
    # the public struct, breaking the build against this system's libxml2
    # 2.12.10. Real, reproduced compile error (not hypothetical). LibreStudio
    # integration patch, applied once at fetch time to this vendored copy
    # only -- a hand-written file(READ)/string(REPLACE)/file(WRITE), not the
    # `patch` utility (librestudio_apply_patches() does not apply here, same
    # reasoning as external/libixion's own integration patch): a freshly-
    # fetched third-party source this project is introducing, not a
    # LibreOffice source this project never patches.
    if (NOT EXISTS "${raptor2_SOURCE_DIR}/.librestudio-patched")
        file(READ "${raptor2_SOURCE_DIR}/src/raptor_libxml.c" _raptor2_libxml_c)
        string(REPLACE
"#if LIBXML_VERSION >= 20627
    /* Checked field was released in 2.6.27 on 2006-10-25
     * http://git.gnome.org/browse/libxml2/commit/?id=a37a6ad91a61d168ecc4b29263def3363fff4da6
     *
     */

    /* Mark this entity as having been checked - never do this again */
    if(!ret->checked)
      ret->checked = 1;
#endif"
"/* xmlEntity->checked cache-guard removed -- LibreStudio integration patch,
 * see this port's own external/raptor2/raptor2.cmake header comment. */"
            _raptor2_libxml_c "${_raptor2_libxml_c}")
        file(WRITE "${raptor2_SOURCE_DIR}/src/raptor_libxml.c" "${_raptor2_libxml_c}")
        file(WRITE "${raptor2_SOURCE_DIR}/.librestudio-patched" "")
    endif ()

    include(ProcessorCount)
    ProcessorCount(_raptor2_nproc)
    if (_raptor2_nproc EQUAL 0)
        set(_raptor2_nproc 1)
    endif ()

    include(ExternalProject)
    ExternalProject_Add(raptor2_ext
        SOURCE_DIR "${raptor2_SOURCE_DIR}"
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND "${raptor2_SOURCE_DIR}/configure"
            --enable-shared --disable-static --disable-gtk-doc
            --without-yajl --with-www=curl
        BUILD_COMMAND make -j${_raptor2_nproc}
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS "${raptor2_SOURCE_DIR}/src/.libs/libraptor2.so"
        COMMENT "Building raptor2 (autotools)")

    set(RAPTOR2_INCLUDE_DIR "${raptor2_SOURCE_DIR}/src")
    set(_raptor2_lib_dir "${raptor2_SOURCE_DIR}/src/.libs")

    # Autotools/libtool leaves the real .so under src/.libs/ -- same
    # relocation-into-LIBRESTUDIO_PROGRAM_DIR dance as orcus (external/orcus/
    # orcus.cmake), and for the same reason. Both the SONAME-versioned file
    # (libraptor2.so.0, what a consumer's own DT_NEEDED actually names at
    # link time) and the unversioned dev-symlink target (libraptor2.so, what
    # -lraptor2 resolves to at link time) are copied -- copying only one
    # name risks a real dlopen/DT_NEEDED mismatch at runtime between the two.
    set(_raptor2_soname_file "${_raptor2_lib_dir}/libraptor2.so.0")
    set(_raptor2_devname_file "${_raptor2_lib_dir}/libraptor2.so")
    set(_raptor2_soname_dest "${LIBRESTUDIO_PROGRAM_DIR}/libraptor2.so.0")
    set(_raptor2_devname_dest "${LIBRESTUDIO_PROGRAM_DIR}/libraptor2.so")
    add_custom_command(
        OUTPUT "${_raptor2_soname_dest}" "${_raptor2_devname_dest}"
        COMMAND ${CMAKE_COMMAND} -E copy "${_raptor2_soname_file}" "${_raptor2_soname_dest}"
        COMMAND ${CMAKE_COMMAND} -E copy "${_raptor2_devname_file}" "${_raptor2_devname_dest}"
        DEPENDS raptor2_ext
        COMMENT "Copying libraptor2.so(.0) into ${LIBRESTUDIO_PROGRAM_DIR}"
        VERBATIM)
    add_custom_target(raptor2_copy ALL DEPENDS "${_raptor2_soname_dest}" "${_raptor2_devname_dest}")

    add_library(raptor2 SHARED IMPORTED GLOBAL)
    set_target_properties(raptor2 PROPERTIES
        IMPORTED_LOCATION "${_raptor2_devname_dest}"
        INTERFACE_INCLUDE_DIRECTORIES "${RAPTOR2_INCLUDE_DIR}")
    add_dependencies(raptor2 raptor2_copy)

    install(FILES "${_raptor2_soname_dest}" "${_raptor2_devname_dest}" DESTINATION program)

    librestudio_write_uninstalled_pc(raptor2 "${LIBRESTUDIO_RDF_PKGCONFIG_DIR}"
        "${LIBRESTUDIO_PROGRAM_DIR}" "${RAPTOR2_INCLUDE_DIR}"
        "2.0.16" "-lcurl -lxml2 -lxslt -lm" "")

    librestudio_vendor_set_target(RAPTOR2 raptor2)
endfunction()
