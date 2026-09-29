# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libabw, part of the writerperfect-family externals --
# see external/librevenge/librevenge.cmake's own header comment for the
# family's shared shape. Unlike the 8 "core" libraries (revenge/wpd/odfgen/
# wpg/wps/mwaw/etonyek/staroffice), libabw has no MSC branch at all and
# always builds STATIC (--enable-static --disable-shared, unconditionally
# in its own real .mk -- no DISABLE_DYNLOADING check even exists for this
# half of the family). A static archive needs no runtime rpath (there is
# no dynamic link step at .a-creation time), so none of this half's
# drivers reproduce the shared-half's LDFLAGS-rpath-chaining logic.
#
# Its own real recipe (ExternalProject_libabw.mk, read in full) passes NO
# explicit REVENGE_CFLAGS/-I flag for librevenge at all -- gbuild's own
# global $(CPPFLAGS)/SOLARINC aggregation (every already-resolved
# external's own include path, folded automatically into one global
# variable every ExternalProject_run inherits) supplies it invisibly
# upstream. This port has no such global aggregator, so librevenge's own
# real include dir is added explicitly here instead -- the FUNCTIONAL
# equivalent of what gbuild's plumbing does silently, not a literal
# transcription of the .mk's own (deceptively bare-looking) CPPFLAGS line.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_abw)
    message(STATUS "libabw: no system package found -- building libabw 0.1.4 from source")

    librestudio_pkgconfig(NAME REVENGE MODULES librevenge-0.0 VERSION 0.0.1 DRIVER librevenge)
    librestudio_find_package(NAME ZLIB TARGET_VAR _libabw_zlib_target)

    librestudio_fetch_tarball(libabw libabw-0.1.4.tar.xz
        fa2685a3440da6e03a66a778480d93cb95f6064e4541e58e37397680760fd6a0)

    # UnpackedTarball_libabw.mk: no patches at all (grepped -- 0 .patch
    # files in external/libabw/).

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    # ExternalProject_libabw.mk's own recipe: --with-pic, static-only,
    # --without-docs, --disable-tools, --disable-debug (ENABLE_DEBUG
    # unset), --disable-werror, --disable-dependency-tracking (gb_FULLDEPS
    # unset in this port).
    ExternalProject_Add(libabw_ext
        SOURCE_DIR "${libabw_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libabw_SOURCE_DIR}/configure"
            --with-pic --enable-static --disable-shared --without-docs
            --disable-tools --disable-debug --disable-werror
            --disable-dependency-tracking
            "CPPFLAGS=-I${librevenge_SOURCE_DIR}/inc"
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${libabw_SOURCE_DIR}/src/lib/.libs/libabw-0.1.a"
        COMMENT "Building libabw (autotools: configure && make)")

    add_library(abw STATIC IMPORTED GLOBAL)
    set_target_properties(abw PROPERTIES
        IMPORTED_LOCATION "${libabw_SOURCE_DIR}/src/lib/.libs/libabw-0.1.a"
        INTERFACE_INCLUDE_DIRECTORIES "${libabw_SOURCE_DIR}/inc")
    target_link_libraries(abw INTERFACE ${LIBRESTUDIO_REVENGE_TARGET} ${_libabw_zlib_target})
    add_dependencies(abw libabw_ext)

    librestudio_vendor_set_target(ABW abw)
endfunction()
