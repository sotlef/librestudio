# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libeot, invoked by librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake, called from vcl/CMakeLists.txt -- the only
# real consumer) only when no system libeot >= 0.01 was found.
#
# Module_libeot.mk registers StaticLibrary_libeot only on the MSC (Windows)
# branch; on every other platform -- this port's own target -- it registers
# ExternalProject_libeot instead, a genuine `./configure && make libeot.la`
# (transcribed from external/libeot/ExternalProject_libeot.mk, read in
# full), same shape as external/hunspell/hunspell.cmake. StaticLibrary_
# libeot.mk (the MSC-only direct-compile path, which lists 13 individual
# .c sources and a wider include path reaching into libeot's own internal
# src/ctf, src/lzcomp, src/util) is not reproduced.
#
# _librestudio_driver_dir: see external/zlib/zlib.cmake's own header
# comment for why this is captured at file top level, outside the function.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_libeot)
    message(STATUS "libeot: no system package found -- building libeot 0.01 from source")

    # download.lst's own LIBEOT_TARBALL/LIBEOT_SHA256SUM, verified directly
    # (grep -n libeot download.lst).
    librestudio_fetch_tarball(libeot libeot-0.01.tar.bz2
        cf5091fa8e7dcdbe667335eb90a2cfdd0a3fe8f8c7c8d1ece44d9d055736a06a)

    # UnpackedTarball_libeot.mk's own 4 patches, in its own $(libeot_patches)
    # order, at the module's own explicit default patchlevel 1
    # (gb_UnpackedTarball_set_patchlevel,libeot,1). debug-no-werror.patch.0
    # and ubsan.patch.0 each carry their own ".0" suffix, overriding to
    # level 0; the other two fall through to the level-1 default.
    librestudio_apply_patches(libeot "${libeot_SOURCE_DIR}" "${_librestudio_driver_dir}" 1
        0001-remove-unused-err.h.patch
        debug-no-werror.patch.0
        ubsan.patch.0
        0001-const-up-tripletEncodings.patch)

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    # ExternalProject_libeot.mk's own recipe, transcribed literally:
    # `touch Makefile.in` first (bumps its mtime past aclocal.m4/configure.ac
    # so automake does not try to regenerate it mid-build -- this port has
    # no autoreconf toolchain wired for libeot, same as every other
    # pre-generated-configure tarball here), then `./configure --with-pic
    # --enable-static --disable-shared --disable-debug` (ENABLE_DEBUG is
    # not a variable this port tracks, so the upstream `$(if $(ENABLE_
    # DEBUG),--enable-debug,--disable-debug)` always resolves to
    # --disable-debug here), then `make libeot.la` specifically -- not a
    # bare `make`, which would also try to build libeot's own CLI test
    # tool. sh -c chains the touch and configure into one CONFIGURE_COMMAND
    # since ExternalProject_Add gives each step exactly one command line.
    ExternalProject_Add(libeot_ext
        SOURCE_DIR "${libeot_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND sh -c
            "touch Makefile.in && '${libeot_SOURCE_DIR}/configure' --with-pic --enable-static --disable-shared --disable-debug"
        BUILD_COMMAND "${MAKE_EXECUTABLE}" libeot.la
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${libeot_SOURCE_DIR}/.libs/libeot.a"
        COMMENT "Building libeot (autotools: configure && make libeot.la)")

    # RepositoryExternal.mk's own !SYSTEM_LIBEOT, non-MSC gb_LinkTarget__
    # use_libeot (3604-3628): include dir is only the unpacked tree's own
    # top-level inc/ (the public API header) -- NOT src/, src/ctf/, src/
    # lzcomp/ or src/util/, which StaticLibrary_libeot.mk's own MSC-only
    # direct-compile path needs but a consumer linking the already-built
    # archive does not.
    add_library(libeot STATIC IMPORTED GLOBAL)
    set_target_properties(libeot PROPERTIES
        IMPORTED_LOCATION "${libeot_SOURCE_DIR}/.libs/libeot.a"
        INTERFACE_INCLUDE_DIRECTORIES "${libeot_SOURCE_DIR}/inc")
    add_dependencies(libeot libeot_ext)

    librestudio_vendor_set_target(LIBEOT libeot)
endfunction()
