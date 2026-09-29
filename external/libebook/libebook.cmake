# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libebook, part of the writerperfect-family externals
# -- static-only half, see external/libabw/libabw.cmake's own header
# comment for that half's shared shape and rationale. Invoked by
# librestudio_pkgconfig() only when no sufficiently new (>= 0.1.2) system
# libe-book-0.1 was found. Note the real soname is "libe-book", not
# "libebook" -- RepositoryExternal.mk's own gb_LinkTarget__use_ebook names
# it explicitly (confirmed, not guessed).
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_ebook)
    message(STATUS "libebook: no system package found -- building libebook 0.1.4 from source")

    librestudio_pkgconfig(NAME REVENGE MODULES librevenge-0.0 VERSION 0.0.1 DRIVER librevenge)
    librestudio_find_package(NAME ZLIB TARGET_VAR _libebook_zlib_target)

    librestudio_fetch_tarball(libebook libe-book-0.1.4.tar.xz
        104b7e791b6632745898e9b6a0037b7540235771a4d8c3bde2c764466ad912f1)

    # UnpackedTarball_libebook.mk: no patches at all.

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    # ExternalProject_libebook.mk's own recipe: --with-pic, static-only,
    # --without-docs, --without-tools, --disable-tests, --disable-debug
    # (ENABLE_DEBUG unset), --disable-werror, --disable-weffc. LANGTAG_
    # CFLAGS/LANGTAG_LIBS and XML_CFLAGS/XML_LIBS from this port's own
    # root-level resolutions (see external/libetonyek/libetonyek.cmake's
    # own header comment); librevenge's own -I added explicitly (see
    # external/libabw/libabw.cmake's own header comment on why).
    ExternalProject_Add(libebook_ext
        SOURCE_DIR "${libebook_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libebook_SOURCE_DIR}/configure"
            --with-pic --enable-static --disable-shared --without-docs
            --without-tools --disable-tests --disable-debug --disable-werror
            --disable-weffc
            "CPPFLAGS=-I${librevenge_SOURCE_DIR}/inc"
            "LANGTAG_CFLAGS=${LIBLANGTAG_CFLAGS}"
            "LANGTAG_LIBS=${LIBLANGTAG_LIBS}"
            "XML_CFLAGS=-I${LIBXML2_INCLUDE_DIRS}"
            "XML_LIBS=${LIBXML2_LIBRARIES}"
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${libebook_SOURCE_DIR}/src/lib/.libs/libe-book-0.1.a"
        COMMENT "Building libebook (autotools: configure && make)")

    add_library(ebook STATIC IMPORTED GLOBAL)
    set_target_properties(ebook PROPERTIES
        IMPORTED_LOCATION "${libebook_SOURCE_DIR}/src/lib/.libs/libe-book-0.1.a"
        INTERFACE_INCLUDE_DIRECTORIES "${libebook_SOURCE_DIR}/inc")
    target_link_libraries(ebook INTERFACE ${LIBRESTUDIO_REVENGE_TARGET} ${_libebook_zlib_target})
    add_dependencies(ebook libebook_ext)

    librestudio_vendor_set_target(EBOOK ebook)
endfunction()
