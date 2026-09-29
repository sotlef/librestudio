# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libmspub, part of the writerperfect-family externals
# -- static-only half, see external/libabw/libabw.cmake's own header
# comment. Invoked by librestudio_pkgconfig() only when no system
# libmspub-0.1 was found.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_mspub)
    message(STATUS "libmspub: no system package found -- building libmspub 0.1.5 from source")

    librestudio_pkgconfig(NAME REVENGE MODULES librevenge-0.0 VERSION 0.0.1 DRIVER librevenge)
    librestudio_find_package(NAME ZLIB TARGET_VAR _libmspub_zlib_target)

    librestudio_fetch_tarball(libmspub libmspub-0.1.5.tar.xz
        3671095f5a10bee8a755052a30576952c5b16d8b0f2ba9f2fb998338c18cb119)

    # UnpackedTarball_libmspub.mk's own 1 patch (a missing #include fix,
    # upstreamed as gerrit.libreoffice.org/#/c/73814/), default patchlevel 0.
    librestudio_apply_patches(libmspub "${libmspub_SOURCE_DIR}" "${_librestudio_driver_dir}" 0
        stdint.patch)

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    # ExternalProject_libmspub.mk's own recipe: --with-pic, static-only,
    # --without-docs, --disable-tools, --disable-debug (ENABLE_DEBUG
    # unset), --disable-werror, --disable-weffc. Its own HAVE_LIBCPP-
    # conditional extra CPPFLAGS entry (-D_LIBCPP_ENABLE_CXX17_REMOVED_
    # UNARY_BINARY_FUNCTION) never applies here -- HAVE_LIBCPP is hardcoded
    # to 0 in this port (cmake/LibreStudioConfigHeaders.cmake: "this build
    # uses libstdc++"), same as external/clucene/clucene.cmake's own
    # identical HAVE_LIBCPP branch.
    ExternalProject_Add(libmspub_ext
        SOURCE_DIR "${libmspub_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libmspub_SOURCE_DIR}/configure"
            --with-pic --enable-static --disable-shared --without-docs
            --disable-tools --disable-debug --disable-werror --disable-weffc
            "CPPFLAGS=-I${librevenge_SOURCE_DIR}/inc"
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${libmspub_SOURCE_DIR}/src/lib/.libs/libmspub-0.1.a"
        COMMENT "Building libmspub (autotools: configure && make)")

    add_library(mspub STATIC IMPORTED GLOBAL)
    set_target_properties(mspub PROPERTIES
        IMPORTED_LOCATION "${libmspub_SOURCE_DIR}/src/lib/.libs/libmspub-0.1.a"
        INTERFACE_INCLUDE_DIRECTORIES "${libmspub_SOURCE_DIR}/inc")
    target_link_libraries(mspub INTERFACE ${LIBRESTUDIO_REVENGE_TARGET} ${_libmspub_zlib_target})
    add_dependencies(mspub libmspub_ext)

    librestudio_vendor_set_target(MSPUB mspub)
endfunction()
