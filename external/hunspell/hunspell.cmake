# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for hunspell, invoked by librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake, called from lingucomponent/CMakeLists.txt --
# the only real consumer, via ulingu) only when no system hunspell was
# found.
#
# Module_hunspell.mk registers StaticLibrary_hunspell only on the MSC
# (Windows) branch; on every other platform -- this port's own target --
# it registers ExternalProject_hunspell instead, a genuine `./configure &&
# cd src/hunspell && make` over the whole unpacked tree (transcribed from
# external/hunspell/ExternalProject_hunspell.mk, read in full), same shape
# as external/coinmp/coinmp.cmake and external/poppler/poppler.cmake's own
# ExternalProject_Add use. StaticLibrary_hunspell.mk (the MSC-only direct-
# compile path) is not reproduced, same as every other Windows-only branch
# in this port.
#
# _librestudio_driver_dir: see external/zlib/zlib.cmake's own header
# comment for why this is captured at file top level, outside the function.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_hunspell)
    message(STATUS "hunspell: no system package found -- building hunspell 1.7.3 from source")

    # download.lst's own HUNSPELL_TARBALL/HUNSPELL_SHA256SUM, verified
    # directly (grep -n hunspell download.lst).
    librestudio_fetch_tarball(hunspell hunspell-1.7.3.tar.gz
        433274dac0619cb00c2e18b43a3dd3a9d50da5b5613fa9b5c21781e35dd76bc1)

    # UnpackedTarball_hunspell.mk's own single patch, at the module's
    # default patchlevel 1 (no gb_UnpackedTarball_set_patchlevel call for
    # hunspell anywhere in the tree -- grepped -- and solenv/gbuild/
    # UnpackedTarball.mk's own gb_UnpackedTarball_PATCHLEVEL_DEFAULT is 1).
    librestudio_apply_patches(hunspell "${hunspell_SOURCE_DIR}" "${_librestudio_driver_dir}" 1
        fix_morphological_analysis_for_compound_words.patch)

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    # Options transcribed literally from ExternalProject_hunspell.mk's own
    # gb_ExternalProject_run,build recipe: --disable-shared (only a static
    # archive is wanted here), --disable-nls, --with-pic. The recipe itself
    # then does "cd src/hunspell && $(MAKE)" -- reproduced here as `make -C
    # src/hunspell` rather than a shell `cd &&` pair, since ExternalProject_
    # Add's BUILD_COMMAND is a single command line with no per-step
    # WORKING_DIRECTORY of its own (same reasoning as external/libnumbertext/
    # libnumbertext.cmake's own BUILD_COMMAND below).
    ExternalProject_Add(hunspell_ext
        SOURCE_DIR "${hunspell_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${hunspell_SOURCE_DIR}/configure"
            --disable-shared --disable-nls --with-pic
        BUILD_COMMAND "${MAKE_EXECUTABLE}" -C "${hunspell_SOURCE_DIR}/src/hunspell"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${hunspell_SOURCE_DIR}/src/hunspell/.libs/libhunspell-1.7.a"
        COMMENT "Building hunspell (autotools: configure && make)")

    # RepositoryExternal.mk's own !SYSTEM_HUNSPELL gb_LinkTarget__use_hunspell
    # (734-777): -DHUNSPELL_STATIC, include dir is src/hunspell itself (the
    # unpacked tree's own headers, not an installed prefix), link against
    # the built static archive directly.
    add_library(hunspell STATIC IMPORTED GLOBAL)
    set_target_properties(hunspell PROPERTIES
        IMPORTED_LOCATION "${hunspell_SOURCE_DIR}/src/hunspell/.libs/libhunspell-1.7.a"
        INTERFACE_INCLUDE_DIRECTORIES "${hunspell_SOURCE_DIR}/src/hunspell"
        INTERFACE_COMPILE_DEFINITIONS "HUNSPELL_STATIC")
    add_dependencies(hunspell hunspell_ext)

    librestudio_vendor_set_target(HUNSPELL hunspell)
endfunction()
