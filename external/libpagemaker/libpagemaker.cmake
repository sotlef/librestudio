# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libpagemaker, part of the writerperfect-family
# externals -- static-only half, see external/libabw/libabw.cmake's own
# header comment. Invoked by librestudio_pkgconfig() only when no system
# libpagemaker-0.0 was found.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_pagemaker)
    message(STATUS "libpagemaker: no system package found -- building libpagemaker 0.0.4 from source")

    librestudio_pkgconfig(NAME REVENGE MODULES librevenge-0.0 VERSION 0.0.1 DRIVER librevenge)

    librestudio_fetch_tarball(libpagemaker libpagemaker-0.0.4.tar.xz
        66adacd705a7d19895e08eac46d1e851332adf2e736c566bef1164e7a442519d)

    # UnpackedTarball_libpagemaker.mk's own single patch, at its own
    # explicit ".1" suffix level.
    librestudio_apply_patches(libpagemaker "${libpagemaker_SOURCE_DIR}" "${_librestudio_driver_dir}" 1
        includes.patch.1)

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    # ExternalProject_libpagemaker.mk's own recipe: --with-pic, static-only,
    # --without-docs, --disable-tools, --disable-debug (its own literal
    # --disable-debug), --disable-werror, --disable-weffc.
    ExternalProject_Add(libpagemaker_ext
        SOURCE_DIR "${libpagemaker_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libpagemaker_SOURCE_DIR}/configure"
            --with-pic --enable-static --disable-shared --without-docs
            --disable-tools --disable-debug --disable-werror --disable-weffc
            "CPPFLAGS=-I${librevenge_SOURCE_DIR}/inc"
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${libpagemaker_SOURCE_DIR}/src/lib/.libs/libpagemaker-0.0.a"
        COMMENT "Building libpagemaker (autotools: configure && make)")

    add_library(pagemaker STATIC IMPORTED GLOBAL)
    set_target_properties(pagemaker PROPERTIES
        IMPORTED_LOCATION "${libpagemaker_SOURCE_DIR}/src/lib/.libs/libpagemaker-0.0.a"
        INTERFACE_INCLUDE_DIRECTORIES "${libpagemaker_SOURCE_DIR}/inc")
    target_link_libraries(pagemaker INTERFACE ${LIBRESTUDIO_REVENGE_TARGET})
    add_dependencies(pagemaker libpagemaker_ext)

    librestudio_vendor_set_target(PAGEMAKER pagemaker)
endfunction()
