# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libepubgen, part of the writerperfect-family
# externals -- static-only half, see external/libabw/libabw.cmake's own
# header comment. Invoked by librestudio_pkgconfig() only when no system
# libepubgen-0.1 was found.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_epubgen)
    message(STATUS "libepubgen: no system package found -- building libepubgen 0.1.1 from source")

    librestudio_pkgconfig(NAME REVENGE MODULES librevenge-0.0 VERSION 0.0.1 DRIVER librevenge)

    librestudio_fetch_tarball(libepubgen libepubgen-0.1.1.tar.xz
        03e084b994cbeffc8c3dd13303b2cb805f44d8f2c3b79f7690d7e3fc7f6215ad)

    # UnpackedTarball_libepubgen.mk's own 2 patches, default patchlevel 0
    # (tdf-119539.patch.1 carries its own ".1" suffix).
    librestudio_apply_patches(libepubgen "${libepubgen_SOURCE_DIR}" "${_librestudio_driver_dir}" 0
        tdf-120491.patch
        tdf-119539.patch.1)

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    # ExternalProject_libepubgen.mk's own recipe: --with-pic, static-only,
    # --without-docs, --disable-debug (ENABLE_DEBUG unset), --disable-
    # werror. Its own `ifneq ($(ENABLE_WASM_STRIP_EPUB),TRUE)` consumer-side
    # gate is never TRUE in this port (not a variable this port tracks), so
    # always taken.
    ExternalProject_Add(libepubgen_ext
        SOURCE_DIR "${libepubgen_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libepubgen_SOURCE_DIR}/configure"
            --with-pic --enable-static --disable-shared --without-docs
            --disable-debug --disable-werror
            "CPPFLAGS=-I${librevenge_SOURCE_DIR}/inc"
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${libepubgen_SOURCE_DIR}/src/lib/.libs/libepubgen-0.1.a"
        COMMENT "Building libepubgen (autotools: configure && make)")

    add_library(epubgen STATIC IMPORTED GLOBAL)
    set_target_properties(epubgen PROPERTIES
        IMPORTED_LOCATION "${libepubgen_SOURCE_DIR}/src/lib/.libs/libepubgen-0.1.a"
        INTERFACE_INCLUDE_DIRECTORIES "${libepubgen_SOURCE_DIR}/inc")
    target_link_libraries(epubgen INTERFACE ${LIBRESTUDIO_REVENGE_TARGET})
    add_dependencies(epubgen libepubgen_ext)

    librestudio_vendor_set_target(EPUBGEN epubgen)
endfunction()
