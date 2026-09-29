# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libwpg, part of the writerperfect-family externals --
# see external/librevenge/librevenge.cmake's own header comment for the
# shared shape. Invoked by librestudio_pkgconfig() only when no system
# libwpg-0.3 was found.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_wpg)
    message(STATUS "libwpg: no system package found -- building libwpg 0.3.4 from source")

    librestudio_pkgconfig(NAME REVENGE MODULES librevenge-0.0 VERSION 0.0.1 DRIVER librevenge)
    librestudio_pkgconfig(NAME WPD MODULES libwpd-0.10 DRIVER libwpd)

    # download.lst's own WPG_TARBALL/WPG_SHA256SUM (WPG_VERSION_MICRO=4).
    librestudio_fetch_tarball(libwpg libwpg-0.3.4.tar.xz
        b55fda9440d1e070630eb2487d8b8697cf412c214a27caee9df69cec7c004de3)

    # UnpackedTarball_libwpg.mk's own patches: unlike libwpd/libodfgen,
    # rpath.patch here is applied UNCONDITIONALLY (not gated on
    # $(SYSTEM_REVENGE) in the real .mk -- re-checked directly). Default
    # patchlevel 0; libwpg-bundled-soname.patch.0's own non-MACOSX/non-WNT/
    # non-iOS guard always taken here.
    librestudio_apply_patches(libwpg "${libwpg_SOURCE_DIR}" "${_librestudio_driver_dir}" 0
        rpath.patch
        0001-const-up-mimeTypesMap.patch.1
        libwpg-bundled-soname.patch.0)

    # ExternalProject_libwpg.mk's own recipe: --with-pic, shared-only,
    # --without-docs, --disable-tools, --disable-debug, --disable-werror.
    # Real -L/-rpath added for whichever of revenge/wpd were vendored
    # (upstream's own unconditional-on-Linux $ORIGIN flag is meaningless in
    # this port's separate-per-external _deps/ layout -- see external/
    # librevenge/librevenge.cmake's own header comment).
    set(_libwpg_ldflags "LDFLAGS=")
    set(_libwpg_extra_depends "")
    if (NOT LIBRESTUDIO_REVENGE_FOUND)
        string(APPEND _libwpg_ldflags " -L${LIBRESTUDIO_REVENGE_LIBDIR} -Wl,-rpath,${LIBRESTUDIO_REVENGE_LIBDIR}")
        if (TARGET librevenge_ext)
            list(APPEND _libwpg_extra_depends librevenge_ext)
        endif ()
    endif ()
    if (NOT LIBRESTUDIO_WPD_FOUND)
        string(APPEND _libwpg_ldflags " -L${LIBRESTUDIO_WPD_LIBDIR} -Wl,-rpath,${LIBRESTUDIO_WPD_LIBDIR}")
        if (TARGET libwpd_ext)
            list(APPEND _libwpg_extra_depends libwpd_ext)
        endif ()
    endif ()

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    ExternalProject_Add(libwpg_ext
        SOURCE_DIR "${libwpg_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libwpg_SOURCE_DIR}/configure"
            --with-pic --enable-shared --disable-static --without-docs
            --disable-tools --disable-debug --disable-werror
            "${_libwpg_ldflags}"
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        DEPENDS ${_libwpg_extra_depends}
        BUILD_BYPRODUCTS
            "${libwpg_SOURCE_DIR}/src/lib/.libs/libwpg-0.3.so"
        COMMENT "Building libwpg (autotools: configure && make)")
    unset(_libwpg_ldflags)
    unset(_libwpg_extra_depends)

    add_library(wpg SHARED IMPORTED GLOBAL)
    set_target_properties(wpg PROPERTIES
        IMPORTED_LOCATION "${libwpg_SOURCE_DIR}/src/lib/.libs/libwpg-0.3.so"
        INTERFACE_INCLUDE_DIRECTORIES "${libwpg_SOURCE_DIR}/inc")
    target_link_libraries(wpg INTERFACE ${LIBRESTUDIO_REVENGE_TARGET} ${LIBRESTUDIO_WPD_TARGET})
    add_dependencies(wpg libwpg_ext)

    librestudio_vendor_set_target(WPG wpg)
endfunction()
