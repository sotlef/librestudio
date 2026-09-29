# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libexttextcat, invoked by librestudio_pkgconfig()
# (cmake/LibreStudioTarballs.cmake, called from lingucomponent/CMakeLists.txt
# -- the only real consumer, via guesslang) only when no system
# libexttextcat >= 3.4.1 was found.
#
# Same shape as external/hunspell/hunspell.cmake: Module_libexttextcat.mk
# registers ExternalProject_libexttextcat on every non-MSC platform
# (transcribed from external/libexttextcat/ExternalProject_libexttextcat.mk,
# read in full); StaticLibrary_libexttextcat.mk (MSC-only) is not
# reproduced. ExternalPackage_fingerprint.mk (the langclass/LM/*.lm runtime
# language-model data files, ~200 of them, installed to LIBO_SHARE_FOLDER/
# fingerprint) is a pure data-packaging concern, deliberately NOT reproduced
# here -- same "not enumerated speculatively" boundary this port already
# drew around poppler_data (external/poppler/poppler.cmake) vs. this one:
# poppler_data was 4 whole directories copied wholesale, this is ~200
# individually-named files with no shortcut, and no consumer in this tree
# currently exercises the runtime language-guessing path. guesslang links
# and would build; it would not correctly guess a language at runtime
# without this data installed.
#
# _librestudio_driver_dir: see external/zlib/zlib.cmake's own header
# comment for why this is captured at file top level, outside the function.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_libexttextcat)
    message(STATUS "libexttextcat: no system package found -- building libexttextcat 3.4.8 from source")

    # download.lst's own LIBEXTTEXTCAT_TARBALL/LIBEXTTEXTCAT_SHA256SUM,
    # verified directly (grep -n libexttextcat download.lst).
    librestudio_fetch_tarball(libexttextcat libexttextcat-3.4.8.tar.xz
        93eb89fd4fc8f565806354e100e778b3ac9a24e5fc04c24e6a83fb1e9b6c9d59)

    # UnpackedTarball_libexttextcat.mk: no patches (grepped external/
    # libexttextcat/ -- no .patch files exist there at all).

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    # Options transcribed literally from ExternalProject_libexttextcat.mk's
    # own gb_ExternalProject_run,build recipe: --disable-shared, --with-pic,
    # plus the verbose/werror conditionals resolved to their non-verbose,
    # non-werror defaults (ENABLE_WERROR is not a variable this port
    # tracks, same treatment as every other vendored external's --disable-
    # werror default). No "cd" -- builds at the unpacked tree's own top
    # level, product lands under src/.libs/.
    ExternalProject_Add(libexttextcat_ext
        SOURCE_DIR "${libexttextcat_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libexttextcat_SOURCE_DIR}/configure"
            --disable-shared --with-pic --enable-silent-rules --disable-werror
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${libexttextcat_SOURCE_DIR}/src/.libs/libexttextcat-2.0.a"
        COMMENT "Building libexttextcat (autotools: configure && make)")

    # RepositoryExternal.mk's own !SYSTEM_LIBEXTTEXTCAT gb_LinkTarget__use_
    # libexttextcat (982-1004): no -D define here (unlike the SYSTEM branch,
    # which adds -DSYSTEM_LIBEXTTEXTCAT) -- guesslang.cxx/guess.cxx/
    # simpleguesser.cxx's own `#ifdef SYSTEM_LIBEXTTEXTCAT` selects between
    # <libexttextcat/textcat.h> (a real system package's own install
    # layout, one subdirectory deep) and plain <textcat.h> (this vendored
    # tree's own flat src/ layout) -- lingucomponent/CMakeLists.txt's own
    # guesslang target must only define SYSTEM_LIBEXTTEXTCAT when this
    # driver was NOT the one that resolved LIBEXTTEXTCAT (i.e. only on the
    # librestudio_pkgconfig() found-branch), or guesslang would look for a
    # libexttextcat/ subdirectory that does not exist in this unpacked tree
    # at all.
    add_library(libexttextcat STATIC IMPORTED GLOBAL)
    set_target_properties(libexttextcat PROPERTIES
        IMPORTED_LOCATION "${libexttextcat_SOURCE_DIR}/src/.libs/libexttextcat-2.0.a"
        INTERFACE_INCLUDE_DIRECTORIES "${libexttextcat_SOURCE_DIR}/src")
    add_dependencies(libexttextcat libexttextcat_ext)

    librestudio_vendor_set_target(LIBEXTTEXTCAT libexttextcat)
endfunction()
