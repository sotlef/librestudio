# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libmwaw, part of the writerperfect-family externals
# -- see external/librevenge/librevenge.cmake's own header comment for the
# shared shape. Invoked by librestudio_pkgconfig() only when no
# sufficiently new (>= 0.3.21) system libmwaw-0.3 was found.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_mwaw)
    message(STATUS "libmwaw: no system package found -- building libmwaw 0.3.23 from source")

    librestudio_pkgconfig(NAME REVENGE MODULES librevenge-0.0 VERSION 0.0.1 DRIVER librevenge)

    # download.lst's own MWAW_TARBALL/MWAW_SHA256SUM (MWAW_VERSION_MICRO=23).
    librestudio_fetch_tarball(libmwaw libmwaw-0.3.23.tar.xz
        ac3590f691a2904eb8c7dc8b757b8a29f125f592449e421459ae8fa928b399eb)

    set(_libmwaw_patches libmwaw-bundled-soname.patch.0)
    if (NOT LIBRESTUDIO_REVENGE_FOUND)
        list(APPEND _libmwaw_patches rpath.patch)
    endif ()
    librestudio_apply_patches(libmwaw "${libmwaw_SOURCE_DIR}" "${_librestudio_driver_dir}" 0 ${_libmwaw_patches})
    unset(_libmwaw_patches)

    # ExternalProject_libmwaw.mk's own recipe: `export PKG_CONFIG="true"`
    # (disables real pkg-config lookups during its own configure -- uses
    # /bin/true as a no-op stand-in, same convention several sibling
    # drivers' own recipes use as `export PKG_CONFIG=""`), --with-pic,
    # shared-only, --without-docs, --disable-tools, --disable-zip,
    # --disable-debug (ENABLE_DEBUG unset), --disable-werror. Then `cd
    # src/lib && make` -- NOT a top-level make, transcribed as `make -C
    # src/lib` (same convention as external/hunspell/hunspell.cmake's own
    # BUILD_COMMAND). PKG_CONFIG=true is an environment variable, not a
    # configure flag, so CONFIGURE_COMMAND is wrapped in sh -c (same
    # pattern as external/libeot/libeot.cmake's own touch+configure combo).
    set(_libmwaw_ldflags "")
    set(_libmwaw_extra_depends "")
    if (NOT LIBRESTUDIO_REVENGE_FOUND)
        set(_libmwaw_ldflags " LDFLAGS='-L${LIBRESTUDIO_REVENGE_LIBDIR} -Wl,-rpath,${LIBRESTUDIO_REVENGE_LIBDIR}'")
        if (TARGET librevenge_ext)
            set(_libmwaw_extra_depends librevenge_ext)
        endif ()
    endif ()

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    ExternalProject_Add(libmwaw_ext
        SOURCE_DIR "${libmwaw_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND sh -c
            "PKG_CONFIG=true '${libmwaw_SOURCE_DIR}/configure' --with-pic --enable-shared --disable-static --without-docs --disable-tools --disable-zip --disable-debug --disable-werror${_libmwaw_ldflags}"
        BUILD_COMMAND "${MAKE_EXECUTABLE}" -C "${libmwaw_SOURCE_DIR}/src/lib"
        INSTALL_COMMAND ""
        DEPENDS ${_libmwaw_extra_depends}
        BUILD_BYPRODUCTS
            "${libmwaw_SOURCE_DIR}/src/lib/.libs/libmwaw-0.3.so"
        COMMENT "Building libmwaw (autotools: configure && make)")
    unset(_libmwaw_ldflags)
    unset(_libmwaw_extra_depends)

    add_library(mwaw SHARED IMPORTED GLOBAL)
    set_target_properties(mwaw PROPERTIES
        IMPORTED_LOCATION "${libmwaw_SOURCE_DIR}/src/lib/.libs/libmwaw-0.3.so"
        INTERFACE_INCLUDE_DIRECTORIES "${libmwaw_SOURCE_DIR}/inc")
    target_link_libraries(mwaw INTERFACE ${LIBRESTUDIO_REVENGE_TARGET})
    add_dependencies(mwaw libmwaw_ext)

    librestudio_vendor_set_target(MWAW mwaw)
endfunction()
