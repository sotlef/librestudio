# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libqxp, part of the writerperfect-family externals --
# static-only half, see external/libabw/libabw.cmake's own header comment.
# Invoked by librestudio_pkgconfig() only when no system libqxp-0.0 was
# found.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_qxp)
    message(STATUS "libqxp: no system package found -- building libqxp 0.0.3 from source")

    librestudio_pkgconfig(NAME REVENGE MODULES librevenge-0.0 VERSION 0.0.1 DRIVER librevenge)

    librestudio_fetch_tarball(libqxp libqxp-0.0.3.tar.xz
        4687a9cc96c32d7406e5072c4da150fca696c563e5cde62b024f82fa53d32332)

    # UnpackedTarball_libqxp.mk's own single patch (Android-named but not
    # OS=ANDROID-gated in the real .mk -- applied unconditionally, same
    # "transcribe don't guess" treatment as other misleadingly-named
    # patches in this port), at its own explicit ".1" suffix level.
    librestudio_apply_patches(libqxp "${libqxp_SOURCE_DIR}" "${_librestudio_driver_dir}" 1
        android-workaround.patch.1)

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    # ExternalProject_libqxp.mk's own recipe: --with-pic, static-only,
    # --without-docs, --disable-tests, --disable-tools, --disable-debug
    # (its own literal --disable-debug), --disable-werror, --disable-weffc,
    # --disable-dependency-tracking (gb_FULLDEPS unset).
    ExternalProject_Add(libqxp_ext
        SOURCE_DIR "${libqxp_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libqxp_SOURCE_DIR}/configure"
            --with-pic --enable-static --disable-shared --without-docs
            --disable-tests --disable-tools --disable-debug --disable-werror
            --disable-weffc --disable-dependency-tracking
            "CPPFLAGS=-I${librevenge_SOURCE_DIR}/inc"
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${libqxp_SOURCE_DIR}/src/lib/.libs/libqxp-0.0.a"
        COMMENT "Building libqxp (autotools: configure && make)")

    add_library(qxp STATIC IMPORTED GLOBAL)
    set_target_properties(qxp PROPERTIES
        IMPORTED_LOCATION "${libqxp_SOURCE_DIR}/src/lib/.libs/libqxp-0.0.a"
        INTERFACE_INCLUDE_DIRECTORIES "${libqxp_SOURCE_DIR}/inc")
    target_link_libraries(qxp INTERFACE ${LIBRESTUDIO_REVENGE_TARGET})
    add_dependencies(qxp libqxp_ext)

    librestudio_vendor_set_target(QXP qxp)
endfunction()
