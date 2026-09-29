# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libcdr, part of the writerperfect-family externals --
# static-only half, see external/libabw/libabw.cmake's own header comment.
# Invoked by librestudio_pkgconfig() only when no system libcdr-0.1 was
# found.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_cdr)
    message(STATUS "libcdr: no system package found -- building libcdr 0.1.9 from source")

    librestudio_pkgconfig(NAME REVENGE MODULES librevenge-0.0 VERSION 0.0.1 DRIVER librevenge)
    librestudio_find_package(NAME ZLIB TARGET_VAR _libcdr_zlib_target)
    librestudio_pkgconfig(NAME LCMS2 MODULES lcms2 TARGET_VAR _libcdr_lcms2_target)

    librestudio_fetch_tarball(libcdr libcdr-0.1.9.tar.xz
        f7bb6abdd7f226820f288a93dd8d07759833c0250d9e202af90f9b312c4665a3)

    # UnpackedTarball_libcdr.mk's own 1 patch (Windows-named but not
    # COM=MSC-gated in the real .mk, same "transcribe don't guess"
    # treatment as several sibling drivers' own Windows-named patches),
    # default patchlevel 0.
    librestudio_apply_patches(libcdr "${libcdr_SOURCE_DIR}" "${_librestudio_driver_dir}" 0
        libcdr-visibility-win.patch)

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    # ExternalProject_libcdr.mk's own recipe: --with-pic, static-only,
    # --without-docs, --disable-tools, --disable-debug (ENABLE_DEBUG
    # unset), --disable-werror, --disable-weffc, --disable-dependency-
    # tracking (gb_FULLDEPS unset).
    ExternalProject_Add(libcdr_ext
        SOURCE_DIR "${libcdr_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libcdr_SOURCE_DIR}/configure"
            --with-pic --enable-static --disable-shared --without-docs
            --disable-tools --disable-debug --disable-werror --disable-weffc
            --disable-dependency-tracking
            "CPPFLAGS=-I${librevenge_SOURCE_DIR}/inc"
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${libcdr_SOURCE_DIR}/src/lib/.libs/libcdr-0.1.a"
        COMMENT "Building libcdr (autotools: configure && make)")

    add_library(cdr STATIC IMPORTED GLOBAL)
    set_target_properties(cdr PROPERTIES
        IMPORTED_LOCATION "${libcdr_SOURCE_DIR}/src/lib/.libs/libcdr-0.1.a"
        INTERFACE_INCLUDE_DIRECTORIES "${libcdr_SOURCE_DIR}/inc")
    target_link_libraries(cdr INTERFACE ${LIBRESTUDIO_REVENGE_TARGET} ${_libcdr_zlib_target} ${_libcdr_lcms2_target})
    add_dependencies(cdr libcdr_ext)

    librestudio_vendor_set_target(CDR cdr)
endfunction()
