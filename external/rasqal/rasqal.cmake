# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for rasqal, invoked by librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake, called from external/redland/redland.cmake --
# the only real recursive caller, since rasqal has no consumer outside
# external/) only when no sufficiently new system rasqal was found.
function(librestudio_vendor_rasqal)
    message(STATUS "rasqal: no system package found (or vendoring forced) -- building rasqal 0.9.33 from source")

    # Own dependency resolved lazily, right here, instead of relying on any
    # guaranteed external order. DRIVER omitted -- MODULES raptor2 is a
    # single clean element, auto-detected as external/raptor2/raptor2.cmake.
    librestudio_pkgconfig(
        MODULES raptor2
        VERSION "2.0.7"
        FORCE_VENDOR_VAR LIBRESTUDIO_FORCE_VENDOR_RAPTOR2
        TARGET_VAR _rasqal_raptor2_target
        FOUND_VAR _rasqal_raptor2_found)

    # raptor2's own found-branch .pc write (formerly in the now-deleted
    # external/raptor2/CMakeLists.txt): raptor2 has no real top-level
    # consumer of its own anymore, so whichever recursive caller resolves
    # it first is responsible for writing its synthetic .pc, needed by
    # rasqal's own configure.ac (its hard-coded `$PKG_CONFIG raptor2
    # --modversion` recheck bypasses env-var overrides, reading
    # PKG_CONFIG_PATH directly instead). A cheap, side-effect-free re-probe
    # (pkg-config queries never mutate anything) -- correct whether this
    # particular call resolved raptor2 fresh or got it from the memoized
    # cache (FOUND_VAR is still accurate either way, but the memoized path
    # does not re-run pkg_check_modules, so libdir/includedir/version are
    # re-queried independently here rather than assumed still fresh).
    if (_rasqal_raptor2_found)
        pkg_check_modules(RAPTOR2_SYS QUIET "raptor2>=2.0.7")
        pkg_get_variable(_rasqal_raptor2_sys_libdir raptor2 libdir)
        pkg_get_variable(_rasqal_raptor2_sys_includedir raptor2 includedir)
        librestudio_write_uninstalled_pc(raptor2 "${LIBRESTUDIO_RDF_PKGCONFIG_DIR}"
            "${_rasqal_raptor2_sys_libdir}" "${_rasqal_raptor2_sys_includedir}"
            "${RAPTOR2_SYS_VERSION}" "" "")
    endif ()

    set(_rasqal_tarball_sha256 "6924c9ac6570bd241a9669f83b467c728a322470bf34f4b2da4f69492ccfd97c")
    set(_rasqal_cache "${LIBRESTUDIO_TARBALL_CACHE}/rasqal-0.9.33.tar.gz")
    file(MAKE_DIRECTORY "${LIBRESTUDIO_TARBALL_CACHE}")
    if (NOT EXISTS "${_rasqal_cache}")
        message(STATUS "Downloading rasqal-0.9.33.tar.gz")
        file(DOWNLOAD "https://download.librdf.org/source/rasqal-0.9.33.tar.gz"
             "${_rasqal_cache}.tmp"
             EXPECTED_HASH SHA256=${_rasqal_tarball_sha256}
             TLS_VERIFY ON
             STATUS _rasqal_dl_status)
        list(GET _rasqal_dl_status 0 _rasqal_dl_code)
        if (NOT _rasqal_dl_code EQUAL 0)
            list(GET _rasqal_dl_status 1 _rasqal_dl_msg)
            file(REMOVE "${_rasqal_cache}.tmp")
            message(FATAL_ERROR "Failed to download rasqal-0.9.33.tar.gz: ${_rasqal_dl_msg}")
        endif ()
        file(RENAME "${_rasqal_cache}.tmp" "${_rasqal_cache}")
    endif ()

    set(_rasqal_dest "${CMAKE_BINARY_DIR}/_deps/rasqal")
    set(_rasqal_stamp "${_rasqal_dest}/.unpacked-${_rasqal_tarball_sha256}")
    if (NOT EXISTS "${_rasqal_stamp}")
        file(REMOVE_RECURSE "${_rasqal_dest}")
        file(MAKE_DIRECTORY "${_rasqal_dest}")
        file(ARCHIVE_EXTRACT INPUT "${_rasqal_cache}" DESTINATION "${_rasqal_dest}")
        file(TOUCH "${_rasqal_stamp}")
    endif ()
    set(rasqal_SOURCE_DIR "${_rasqal_dest}/rasqal-0.9.33")

    include(ProcessorCount)
    ProcessorCount(_rasqal_nproc)
    if (_rasqal_nproc EQUAL 0)
        set(_rasqal_nproc 1)
    endif ()

    # rasqal 0.9.33 needed no source patch of its own (built cleanly against
    # this system's libxml2/gmp/mhash) -- manually verified end-to-end
    # before writing this. configure.ac's own hard `$PKG_CONFIG raptor2
    # --modversion` recheck (see external/raptor2/raptor2.cmake's own header
    # comment) is why PKG_CONFIG_PATH, not RAPTOR2_CFLAGS/_LIBS env
    # overrides, is used here.
    include(ExternalProject)
    ExternalProject_Add(rasqal_ext
        SOURCE_DIR "${rasqal_SOURCE_DIR}"
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E env
                "PKG_CONFIG_PATH=${LIBRESTUDIO_RDF_PKGCONFIG_DIR}"
            "${rasqal_SOURCE_DIR}/configure"
                --enable-shared --disable-static --disable-gtk-doc
        BUILD_COMMAND make -j${_rasqal_nproc}
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS "${rasqal_SOURCE_DIR}/src/.libs/librasqal.so"
        COMMENT "Building rasqal (autotools)")
    add_dependencies(rasqal_ext ${_rasqal_raptor2_target})

    set(RASQAL_INCLUDE_DIR "${rasqal_SOURCE_DIR}/src")
    set(_rasqal_lib_dir "${rasqal_SOURCE_DIR}/src/.libs")

    set(_rasqal_soname_file "${_rasqal_lib_dir}/librasqal.so.3")
    set(_rasqal_devname_file "${_rasqal_lib_dir}/librasqal.so")
    set(_rasqal_soname_dest "${LIBRESTUDIO_PROGRAM_DIR}/librasqal.so.3")
    set(_rasqal_devname_dest "${LIBRESTUDIO_PROGRAM_DIR}/librasqal.so")
    add_custom_command(
        OUTPUT "${_rasqal_soname_dest}" "${_rasqal_devname_dest}"
        COMMAND ${CMAKE_COMMAND} -E copy "${_rasqal_soname_file}" "${_rasqal_soname_dest}"
        COMMAND ${CMAKE_COMMAND} -E copy "${_rasqal_devname_file}" "${_rasqal_devname_dest}"
        DEPENDS rasqal_ext
        COMMENT "Copying librasqal.so(.3) into ${LIBRESTUDIO_PROGRAM_DIR}"
        VERBATIM)
    add_custom_target(rasqal_copy ALL DEPENDS "${_rasqal_soname_dest}" "${_rasqal_devname_dest}")

    add_library(rasqal SHARED IMPORTED GLOBAL)
    set_target_properties(rasqal PROPERTIES
        IMPORTED_LOCATION "${_rasqal_devname_dest}"
        INTERFACE_INCLUDE_DIRECTORIES "${RASQAL_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${_rasqal_raptor2_target}")
    add_dependencies(rasqal rasqal_copy)

    install(FILES "${_rasqal_soname_dest}" "${_rasqal_devname_dest}" DESTINATION program)

    librestudio_write_uninstalled_pc(rasqal "${LIBRESTUDIO_RDF_PKGCONFIG_DIR}"
        "${LIBRESTUDIO_PROGRAM_DIR}" "${RASQAL_INCLUDE_DIR}"
        "0.9.33" "" raptor2)

    librestudio_vendor_set_target(RASQAL rasqal)
endfunction()
