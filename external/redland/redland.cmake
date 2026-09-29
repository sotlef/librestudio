# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for redland/librdf, invoked by librestudio_pkgconfig()
# (cmake/LibreStudioTarballs.cmake, called from unoxml/CMakeLists.txt --
# the only real consumer) only when no sufficiently new system redland was
# found.
function(librestudio_vendor_redland)
    message(STATUS "redland: no system package found (or vendoring forced) -- building redland 1.0.17 from source")

    # Own dependencies resolved lazily, right here, instead of relying on
    # any guaranteed external order. DRIVER omitted for both -- MODULES
    # raptor2/rasqal are each a single clean element, auto-detected as
    # external/raptor2/raptor2.cmake and external/rasqal/rasqal.cmake.
    librestudio_pkgconfig(
        MODULES raptor2
        VERSION "2.0.7"
        FORCE_VENDOR_VAR LIBRESTUDIO_FORCE_VENDOR_RAPTOR2
        TARGET_VAR _redland_raptor2_target
        FOUND_VAR _redland_raptor2_found)
    # See external/rasqal/rasqal.cmake's own identical block for why this
    # re-probe (rather than reading pkg_check_modules's own result
    # variables from inside librestudio_pkgconfig()'s function scope, which
    # they never escape) is needed, and why it is safe to run twice (here
    # and, redundantly but harmlessly, again from within rasqal's own
    # vendor function below if rasqal itself also needs vendoring) --
    # redland's OWN ./configure needs raptor2.pc to exist on
    # LIBRESTUDIO_RDF_PKGCONFIG_DIR regardless of whether rasqal itself
    # ends up being vendored or found.
    if (_redland_raptor2_found)
        pkg_check_modules(RAPTOR2_SYS QUIET "raptor2>=2.0.7")
        pkg_get_variable(_redland_raptor2_sys_libdir raptor2 libdir)
        pkg_get_variable(_redland_raptor2_sys_includedir raptor2 includedir)
        librestudio_write_uninstalled_pc(raptor2 "${LIBRESTUDIO_RDF_PKGCONFIG_DIR}"
            "${_redland_raptor2_sys_libdir}" "${_redland_raptor2_sys_includedir}"
            "${RAPTOR2_SYS_VERSION}" "" "")
    endif ()

    librestudio_pkgconfig(
        MODULES rasqal
        FORCE_VENDOR_VAR LIBRESTUDIO_FORCE_VENDOR_RASQAL
        TARGET_VAR _redland_rasqal_target
        FOUND_VAR _redland_rasqal_found)
    # rasqal's own found-branch .pc write (formerly in the now-deleted
    # external/rasqal/CMakeLists.txt) -- rasqal has no real top-level
    # consumer of its own either, needed here for the same reason as
    # raptor2's own write just above.
    if (_redland_rasqal_found)
        pkg_check_modules(RASQAL_SYS QUIET rasqal)
        pkg_get_variable(_redland_rasqal_sys_libdir rasqal libdir)
        pkg_get_variable(_redland_rasqal_sys_includedir rasqal includedir)
        librestudio_write_uninstalled_pc(rasqal "${LIBRESTUDIO_RDF_PKGCONFIG_DIR}"
            "${_redland_rasqal_sys_libdir}" "${_redland_rasqal_sys_includedir}"
            "${RASQAL_SYS_VERSION}" "" raptor2)
    endif ()

    set(_redland_tarball_sha256 "de1847f7b59021c16bdc72abb4d8e2d9187cd6124d69156f3326dd34ee043681")
    set(_redland_cache "${LIBRESTUDIO_TARBALL_CACHE}/redland-1.0.17.tar.gz")
    file(MAKE_DIRECTORY "${LIBRESTUDIO_TARBALL_CACHE}")
    if (NOT EXISTS "${_redland_cache}")
        message(STATUS "Downloading redland-1.0.17.tar.gz")
        file(DOWNLOAD "https://download.librdf.org/source/redland-1.0.17.tar.gz"
             "${_redland_cache}.tmp"
             EXPECTED_HASH SHA256=${_redland_tarball_sha256}
             TLS_VERIFY ON
             STATUS _redland_dl_status)
        list(GET _redland_dl_status 0 _redland_dl_code)
        if (NOT _redland_dl_code EQUAL 0)
            list(GET _redland_dl_status 1 _redland_dl_msg)
            file(REMOVE "${_redland_cache}.tmp")
            message(FATAL_ERROR "Failed to download redland-1.0.17.tar.gz: ${_redland_dl_msg}")
        endif ()
        file(RENAME "${_redland_cache}.tmp" "${_redland_cache}")
    endif ()

    set(_redland_dest "${CMAKE_BINARY_DIR}/_deps/redland")
    set(_redland_stamp "${_redland_dest}/.unpacked-${_redland_tarball_sha256}")
    if (NOT EXISTS "${_redland_stamp}")
        file(REMOVE_RECURSE "${_redland_dest}")
        file(MAKE_DIRECTORY "${_redland_dest}")
        file(ARCHIVE_EXTRACT INPUT "${_redland_cache}" DESTINATION "${_redland_dest}")
        file(TOUCH "${_redland_stamp}")
    endif ()
    set(redland_SOURCE_DIR "${_redland_dest}/redland-1.0.17")

    include(ProcessorCount)
    ProcessorCount(_redland_nproc)
    if (_redland_nproc EQUAL 0)
        set(_redland_nproc 1)
    endif ()

    # redland 1.0.17 needed no source patch of its own -- manually verified
    # end-to-end before writing this. --with-{bdb,mysql,postgresql,sqlite,
    # virtuoso,threestore}=no and --with-threads=no keep this to the storage
    # backends and threading mode unoxml's own CBlankNode/CURI/CLiteral/
    # librdf_repository wrapper classes actually need (in-memory triple
    # stores only, single-threaded UNO calls). --disable-modular links the
    # storage backends into librdf itself instead of separate dlopen()'d
    # plugin .so's (this port's own "no dlopen-based plugin loading"
    # convention).
    include(ExternalProject)
    ExternalProject_Add(redland_ext
        SOURCE_DIR "${redland_SOURCE_DIR}"
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E env
                "PKG_CONFIG_PATH=${LIBRESTUDIO_RDF_PKGCONFIG_DIR}"
            "${redland_SOURCE_DIR}/configure"
                --enable-shared --disable-static --disable-gtk-doc
                --with-bdb=no --with-mysql=no --with-postgresql=no
                --with-sqlite=no --with-virtuoso=no --with-threestore=no
                --with-threads=no --disable-modular
        BUILD_COMMAND make -j${_redland_nproc}
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS "${redland_SOURCE_DIR}/src/.libs/librdf.so"
        COMMENT "Building redland/librdf (autotools)")
    add_dependencies(redland_ext ${_redland_raptor2_target} ${_redland_rasqal_target})

    set(REDLAND_INCLUDE_DIR "${redland_SOURCE_DIR}/src")
    set(_redland_lib_dir "${redland_SOURCE_DIR}/src/.libs")

    set(_redland_soname_file "${_redland_lib_dir}/librdf.so.0")
    set(_redland_devname_file "${_redland_lib_dir}/librdf.so")
    set(_redland_soname_dest "${LIBRESTUDIO_PROGRAM_DIR}/librdf.so.0")
    set(_redland_devname_dest "${LIBRESTUDIO_PROGRAM_DIR}/librdf.so")
    add_custom_command(
        OUTPUT "${_redland_soname_dest}" "${_redland_devname_dest}"
        COMMAND ${CMAKE_COMMAND} -E copy "${_redland_soname_file}" "${_redland_soname_dest}"
        COMMAND ${CMAKE_COMMAND} -E copy "${_redland_devname_file}" "${_redland_devname_dest}"
        DEPENDS redland_ext
        COMMENT "Copying librdf.so(.0) into ${LIBRESTUDIO_PROGRAM_DIR}"
        VERBATIM)
    add_custom_target(redland_copy ALL DEPENDS "${_redland_soname_dest}" "${_redland_devname_dest}")

    add_library(redland SHARED IMPORTED GLOBAL)
    set_target_properties(redland PROPERTIES
        IMPORTED_LOCATION "${_redland_devname_dest}"
        INTERFACE_INCLUDE_DIRECTORIES "${REDLAND_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${_redland_rasqal_target};${_redland_raptor2_target}")
    add_dependencies(redland redland_copy)

    install(FILES "${_redland_soname_dest}" "${_redland_devname_dest}" DESTINATION program)

    librestudio_vendor_set_target(REDLAND redland)
endfunction()
