# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libstaroffice, part of the writerperfect-family
# externals -- see external/librevenge/librevenge.cmake's own header
# comment for the shared shape. Invoked by librestudio_pkgconfig() only
# when no sufficiently new (>= 0.0.7) system libstaroffice-0.0 was found.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_staroffice)
    message(STATUS "libstaroffice: no system package found -- building libstaroffice 0.0.8 from source")

    librestudio_pkgconfig(NAME REVENGE MODULES librevenge-0.0 VERSION 0.0.1 DRIVER librevenge)

    # download.lst's own STAROFFICE_TARBALL/STAROFFICE_SHA256SUM
    # (STAROFFICE_VERSION_MICRO=8).
    librestudio_fetch_tarball(libstaroffice libstaroffice-0.0.8.tar.xz
        bbca92882e96c55bd1527670866d6bc7529ccb12786da55a2398b60a063378da)

    set(_libstaroffice_patches libstaroffice-bundled-soname.patch.0)
    if (NOT LIBRESTUDIO_REVENGE_FOUND)
        list(APPEND _libstaroffice_patches rpath.patch)
    endif ()
    librestudio_apply_patches(libstaroffice "${libstaroffice_SOURCE_DIR}" "${_librestudio_driver_dir}" 0 ${_libstaroffice_patches})
    unset(_libstaroffice_patches)

    # ExternalProject_libstaroffice.mk's own recipe: --with-pic, shared-
    # only, --with-sharedptr=c++11, --without-docs, --disable-tools,
    # --disable-zip, --disable-debug (ENABLE_DEBUG unset), --disable-werror.
    # LDFLAGS rpath only when librevenge was vendored.
    set(_libstaroffice_ldflags "")
    set(_libstaroffice_extra_depends "")
    if (NOT LIBRESTUDIO_REVENGE_FOUND)
        set(_libstaroffice_ldflags "LDFLAGS=-L${LIBRESTUDIO_REVENGE_LIBDIR} -Wl,-rpath,${LIBRESTUDIO_REVENGE_LIBDIR}")
        if (TARGET librevenge_ext)
            set(_libstaroffice_extra_depends librevenge_ext)
        endif ()
    endif ()

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    ExternalProject_Add(libstaroffice_ext
        SOURCE_DIR "${libstaroffice_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libstaroffice_SOURCE_DIR}/configure"
            --with-pic --enable-shared --disable-static
            --with-sharedptr=c++11 --without-docs --disable-tools
            --disable-zip --disable-debug --disable-werror
            ${_libstaroffice_ldflags}
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        DEPENDS ${_libstaroffice_extra_depends}
        BUILD_BYPRODUCTS
            "${libstaroffice_SOURCE_DIR}/src/lib/.libs/libstaroffice-0.0.so"
        COMMENT "Building libstaroffice (autotools: configure && make)")
    unset(_libstaroffice_ldflags)
    unset(_libstaroffice_extra_depends)

    add_library(staroffice SHARED IMPORTED GLOBAL)
    set_target_properties(staroffice PROPERTIES
        IMPORTED_LOCATION "${libstaroffice_SOURCE_DIR}/src/lib/.libs/libstaroffice-0.0.so"
        INTERFACE_INCLUDE_DIRECTORIES "${libstaroffice_SOURCE_DIR}/inc")
    target_link_libraries(staroffice INTERFACE ${LIBRESTUDIO_REVENGE_TARGET})
    add_dependencies(staroffice libstaroffice_ext)

    librestudio_vendor_set_target(STAROFFICE staroffice)
endfunction()
