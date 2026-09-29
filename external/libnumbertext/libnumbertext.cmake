# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libnumbertext, invoked by librestudio_pkgconfig()
# (cmake/LibreStudioTarballs.cmake, called from lingucomponent/CMakeLists.txt
# -- the only real consumer, via numbertext) only when no system
# libnumbertext >= 1.0.6 was found.
#
# Same shape as external/hunspell/hunspell.cmake: Module_libnumbertext.mk
# registers ExternalProject_libnumbertext on every non-MSC platform
# (transcribed from external/libnumbertext/ExternalProject_libnumbertext.mk,
# read in full); StaticLibrary_libnumbertext.mk (MSC-only) is not
# reproduced. ExternalPackage_numbertext.mk (the data/*.sor runtime
# conversion-rule data files, installed to LIBO_SHARE_FOLDER/numbertext) is
# a pure data-packaging concern, deliberately NOT reproduced here -- same
# boundary drawn in external/libexttextcat/libexttextcat.cmake's own header
# comment for its sibling fingerprint data.
#
# _librestudio_driver_dir: see external/zlib/zlib.cmake's own header
# comment for why this is captured at file top level, outside the function.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_libnumbertext)
    message(STATUS "libnumbertext: no system package found -- building libnumbertext 1.0.11 from source")

    # download.lst's own LIBNUMBERTEXT_TARBALL/LIBNUMBERTEXT_SHA256SUM,
    # verified directly (grep -n libnumbertext download.lst).
    librestudio_fetch_tarball(libnumbertext libnumbertext-1.0.11.tar.xz
        5dcb4db3b2340f81f601ce86d8d76b69e34d70f84f804192c901e4b7f84d5fb0)

    # UnpackedTarball_libnumbertext.mk's own 3 patches, all applied
    # unconditionally regardless of platform (not COM=MSC-gated in the .mk,
    # even though two of the three names read as Windows-specific -- re-
    # checked directly, not guessed). deprecated.patch.0 carries its own
    # ".0" numeric suffix (level 0); the other two fall through to the
    # module's default patchlevel 1 (no gb_UnpackedTarball_set_patchlevel
    # call for libnumbertext anywhere in the tree -- grepped -- so solenv/
    # gbuild/UnpackedTarball.mk's own gb_UnpackedTarball_PATCHLEVEL_DEFAULT
    # of 1 applies).
    librestudio_apply_patches(libnumbertext "${libnumbertext_SOURCE_DIR}" "${_librestudio_driver_dir}" 1
        MSVCNonBMPBug.patch
        WinUnicodePath.patch
        deprecated.patch.0)

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    # Options transcribed literally from ExternalProject_libnumbertext.mk's
    # own gb_ExternalProject_run,build recipe: --disable-shared, --with-pic,
    # plus the verbose/werror conditionals resolved to their non-verbose,
    # non-werror defaults (same as external/libexttextcat/libexttextcat.cmake).
    # The recipe itself then does "cd src && $(MAKE)" -- reproduced as
    # `make -C src`, same reasoning as external/hunspell/hunspell.cmake's
    # own BUILD_COMMAND.
    ExternalProject_Add(libnumbertext_ext
        SOURCE_DIR "${libnumbertext_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libnumbertext_SOURCE_DIR}/configure"
            --disable-shared --with-pic --enable-silent-rules --disable-werror
        BUILD_COMMAND "${MAKE_EXECUTABLE}" -C "${libnumbertext_SOURCE_DIR}/src"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${libnumbertext_SOURCE_DIR}/src/.libs/libnumbertext-1.0.a"
        COMMENT "Building libnumbertext (autotools: configure && make)")

    # RepositoryExternal.mk's own !SYSTEM_LIBNUMBERTEXT gb_LinkTarget__use_
    # libnumbertext (1021-1049): no -D define on this branch (unlike the
    # SYSTEM branch, which adds -DSYSTEM_LIBNUMBERTEXT) -- but numbertext.cxx
    # does not itself branch on that macro at all (grepped: 0 references),
    # so lingucomponent/CMakeLists.txt's own unconditional -DSYSTEM_
    # LIBNUMBERTEXT stays harmless either way, unlike libexttextcat's own
    # guesslang.cxx (see external/libexttextcat/libexttextcat.cmake's own
    # header comment for why THAT one does matter).
    add_library(libnumbertext STATIC IMPORTED GLOBAL)
    set_target_properties(libnumbertext PROPERTIES
        IMPORTED_LOCATION "${libnumbertext_SOURCE_DIR}/src/.libs/libnumbertext-1.0.a"
        INTERFACE_INCLUDE_DIRECTORIES "${libnumbertext_SOURCE_DIR}/src")
    add_dependencies(libnumbertext libnumbertext_ext)

    librestudio_vendor_set_target(LIBNUMBERTEXT libnumbertext)
endfunction()
