# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libvisio, part of the writerperfect-family externals
# -- static-only half, see external/libabw/libabw.cmake's own header
# comment. Invoked by librestudio_pkgconfig() only when no system
# libvisio-0.1 was found.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_visio)
    message(STATUS "libvisio: no system package found -- building libvisio 0.1.11 from source")

    librestudio_pkgconfig(NAME REVENGE MODULES librevenge-0.0 VERSION 0.0.1 DRIVER librevenge)

    librestudio_fetch_tarball(libvisio libvisio-0.1.11.tar.xz
        2a6efd40b6d9dbcb70fba3be53112366882ba97b57151df3698dfa478c8d8dd3)

    # UnpackedTarball_libvisio.mk: no patches at all.

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    # ExternalProject_libvisio.mk's own recipe: --with-pic, static-only,
    # --without-docs, --disable-tests, --disable-tools, --disable-debug
    # (ENABLE_DEBUG unset), --disable-werror, --disable-dependency-tracking
    # (gb_FULLDEPS unset). XML_CFLAGS/XML_LIBS are NOT passed explicitly in
    # the real recipe (unlike libodfgen/libetonyek/libebook's own) --
    # libxml2's own headers are already on this system's default include
    # path (/usr/include/libxml2 IS the standard location, but libxml2.h
    # itself is included as <libxml/...> -- confirmed the real recipe
    # relies on gbuild's own global aggregation the same way librevenge's
    # own -I does, per external/libabw/libabw.cmake's own header comment),
    # so libxml2's own -I is added here for the same reason librevenge's is.
    ExternalProject_Add(libvisio_ext
        SOURCE_DIR "${libvisio_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libvisio_SOURCE_DIR}/configure"
            --with-pic --enable-static --disable-shared --without-docs
            --disable-tests --disable-tools --disable-debug --disable-werror
            --disable-dependency-tracking
            "CPPFLAGS=-I${librevenge_SOURCE_DIR}/inc -I${LIBXML2_INCLUDE_DIRS}"
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${libvisio_SOURCE_DIR}/src/lib/.libs/libvisio-0.1.a"
        COMMENT "Building libvisio (autotools: configure && make)")

    add_library(visio STATIC IMPORTED GLOBAL)
    set_target_properties(visio PROPERTIES
        IMPORTED_LOCATION "${libvisio_SOURCE_DIR}/src/lib/.libs/libvisio-0.1.a"
        INTERFACE_INCLUDE_DIRECTORIES "${libvisio_SOURCE_DIR}/inc")
    target_link_libraries(visio INTERFACE ${LIBRESTUDIO_REVENGE_TARGET})
    add_dependencies(visio libvisio_ext)

    librestudio_vendor_set_target(VISIO visio)
endfunction()
