# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for mythes, invoked by librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake, called from lingucomponent/CMakeLists.txt --
# the only real consumer, via lnth) only when no system mythes was found.
#
# Same shape as external/hunspell/hunspell.cmake: Module_mythes.mk
# registers ExternalProject_mythes on every non-MSC platform (transcribed
# from external/mythes/ExternalProject_mythes.mk, read in full); the
# StaticLibrary_mythes.mk direct-compile path is MSC-only and not
# reproduced.
#
# _librestudio_driver_dir: see external/zlib/zlib.cmake's own header
# comment for why this is captured at file top level, outside the function.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_mythes)
    message(STATUS "mythes: no system package found -- building mythes 1.2.6 from source")

    # download.lst's own MYTHES_TARBALL/MYTHES_SHA256SUM, verified directly
    # (grep -n mythes download.lst).
    librestudio_fetch_tarball(mythes mythes-1.2.6.tar.xz
        97bd2ba5738aebdba1bc31f30f1be42f1404386c94105ad5990d839a1311c8a5)

    # UnpackedTarball_mythes.mk: no patches (grepped external/mythes/ -- no
    # .patch files exist there at all).

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    # Options transcribed literally from ExternalProject_mythes.mk's own
    # gb_ExternalProject_run,build recipe: --disable-shared, --disable-
    # werror, --with-pic; no "cd" (this one builds at the unpacked tree's
    # own top level, unlike hunspell/libnumbertext).
    ExternalProject_Add(mythes_ext
        SOURCE_DIR "${mythes_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${mythes_SOURCE_DIR}/configure"
            --disable-shared --disable-werror --with-pic
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${mythes_SOURCE_DIR}/.libs/libmythes-1.2.a"
        COMMENT "Building mythes (autotools: configure && make)")

    # ExternalProject_mythes.mk's own gb_ExternalProject_use_external,
    # mythes,hunspell (guarded by !ENABLE_WASM_STRIP_HUNSPELL, which this
    # port never sets -- always applies): build-ordering only, mythes does
    # not itself link against hunspell -- reproduced as an add_dependencies
    # edge on the ExternalProject step, only when hunspell was ALSO
    # vendored (a system hunspell needs no such ordering, and LIBRESTUDIO_
    # HUNSPELL_TARGET may not even be a real ExternalProject step name in
    # that case). By lingucomponent/CMakeLists.txt's own call order, HUNSPELL
    # is always resolved (for ulingu) before MYTHES is (for lnth), so this
    # check is safe to make right here.
    if (NOT LIBRESTUDIO_HUNSPELL_FOUND AND TARGET hunspell_ext)
        add_dependencies(mythes_ext hunspell_ext)
    endif ()

    # RepositoryExternal.mk's own !SYSTEM_MYTHES gb_LinkTarget__use_mythes
    # (640-659): no -D define (unlike hunspell's -DHUNSPELL_STATIC), include
    # dir is the unpacked tree's own top level (mythes.hxx lives there, not
    # under a subdirectory), link against the built static archive directly.
    add_library(mythes STATIC IMPORTED GLOBAL)
    set_target_properties(mythes PROPERTIES
        IMPORTED_LOCATION "${mythes_SOURCE_DIR}/.libs/libmythes-1.2.a"
        INTERFACE_INCLUDE_DIRECTORIES "${mythes_SOURCE_DIR}")
    add_dependencies(mythes mythes_ext)

    librestudio_vendor_set_target(MYTHES mythes)
endfunction()
