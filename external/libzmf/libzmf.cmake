# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libzmf, part of the writerperfect-family externals --
# static-only half, see external/libabw/libabw.cmake's own header comment.
# Invoked by librestudio_pkgconfig() only when no system libzmf-0.0 was
# found.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_zmf)
    message(STATUS "libzmf: no system package found -- building libzmf 0.0.2 from source")

    librestudio_pkgconfig(NAME REVENGE MODULES librevenge-0.0 VERSION 0.0.1 DRIVER librevenge)
    librestudio_find_package(NAME ZLIB TARGET_VAR _libzmf_zlib_target)
    librestudio_find_package(NAME PNG TARGET_VAR _libzmf_png_target)

    librestudio_fetch_tarball(libzmf libzmf-0.0.2.tar.xz
        27051a30cb057fdb5d5de65a1f165c7153dc76e27fe62251cbb86639eb2caf22)

    # UnpackedTarball_libzmf.mk's own single patch (Android-named but not
    # OS=ANDROID-gated in the real .mk), at its own explicit ".1" suffix
    # level.
    librestudio_apply_patches(libzmf "${libzmf_SOURCE_DIR}" "${_librestudio_driver_dir}" 1
        android-workaround.patch.1)

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    # ExternalProject_libzmf.mk's own recipe: --with-pic, static-only,
    # --without-docs, --disable-tests, --disable-tools, --disable-debug
    # (its own literal --disable-debug), --disable-werror, --disable-weffc.
    ExternalProject_Add(libzmf_ext
        SOURCE_DIR "${libzmf_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libzmf_SOURCE_DIR}/configure"
            --with-pic --enable-static --disable-shared --without-docs
            --disable-tests --disable-tools --disable-debug --disable-werror
            --disable-weffc
            "CPPFLAGS=-I${librevenge_SOURCE_DIR}/inc"
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${libzmf_SOURCE_DIR}/src/lib/.libs/libzmf-0.0.a"
        COMMENT "Building libzmf (autotools: configure && make)")

    add_library(zmf STATIC IMPORTED GLOBAL)
    set_target_properties(zmf PROPERTIES
        IMPORTED_LOCATION "${libzmf_SOURCE_DIR}/src/lib/.libs/libzmf-0.0.a"
        INTERFACE_INCLUDE_DIRECTORIES "${libzmf_SOURCE_DIR}/inc")
    target_link_libraries(zmf INTERFACE ${LIBRESTUDIO_REVENGE_TARGET} ${_libzmf_zlib_target} ${_libzmf_png_target})
    add_dependencies(zmf libzmf_ext)

    librestudio_vendor_set_target(ZMF zmf)
endfunction()
