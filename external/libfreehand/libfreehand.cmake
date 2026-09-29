# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libfreehand, part of the writerperfect-family
# externals -- static-only half, see external/libabw/libabw.cmake's own
# header comment. Invoked by librestudio_pkgconfig() only when no system
# libfreehand-0.1 was found.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_freehand)
    message(STATUS "libfreehand: no system package found -- building libfreehand 0.1.4 from source")

    librestudio_pkgconfig(NAME REVENGE MODULES librevenge-0.0 VERSION 0.0.1 DRIVER librevenge)
    librestudio_find_package(NAME ZLIB TARGET_VAR _libfreehand_zlib_target)
    librestudio_pkgconfig(NAME LCMS2 MODULES lcms2 TARGET_VAR _libfreehand_lcms2_target)

    librestudio_fetch_tarball(libfreehand libfreehand-0.1.4.tar.xz
        350b10d24a76d7e8c8ae98b74c2d432a2c8ddec08935d09856d20b695a35e600)

    # UnpackedTarball_libfreehand.mk: no patches at all.

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    # ExternalProject_libfreehand.mk's own recipe: --with-pic, static-only,
    # --without-docs, --disable-tests, --disable-tools, --disable-debug
    # (its own literal --disable-debug, no ENABLE_DEBUG conditional here
    # unlike most siblings), --disable-werror, --disable-weffc.
    ExternalProject_Add(libfreehand_ext
        SOURCE_DIR "${libfreehand_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libfreehand_SOURCE_DIR}/configure"
            --with-pic --enable-static --disable-shared --without-docs
            --disable-tests --disable-tools --disable-debug --disable-werror
            --disable-weffc
            "CPPFLAGS=-I${librevenge_SOURCE_DIR}/inc"
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${libfreehand_SOURCE_DIR}/src/lib/.libs/libfreehand-0.1.a"
        COMMENT "Building libfreehand (autotools: configure && make)")

    add_library(freehand STATIC IMPORTED GLOBAL)
    set_target_properties(freehand PROPERTIES
        IMPORTED_LOCATION "${libfreehand_SOURCE_DIR}/src/lib/.libs/libfreehand-0.1.a"
        INTERFACE_INCLUDE_DIRECTORIES "${libfreehand_SOURCE_DIR}/inc")
    target_link_libraries(freehand INTERFACE ${LIBRESTUDIO_REVENGE_TARGET} ${_libfreehand_zlib_target} ${_libfreehand_lcms2_target})
    add_dependencies(freehand libfreehand_ext)

    librestudio_vendor_set_target(FREEHAND freehand)
endfunction()
