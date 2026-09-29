# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libwpd, part of the writerperfect-family externals --
# see external/librevenge/librevenge.cmake's own header comment for the
# shared shape (real `./configure && make`, shared-lib category since
# DISABLE_DYNLOADING is never set in this port). Invoked by
# librestudio_pkgconfig() only when no system libwpd-0.10 was found.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_wpd)
    message(STATUS "libwpd: no system package found -- building libwpd 0.10.3 from source")

    librestudio_pkgconfig(NAME REVENGE MODULES librevenge-0.0 VERSION 0.0.1 DRIVER librevenge)

    # download.lst's own WPD_TARBALL/WPD_SHA256SUM (WPD_VERSION_MICRO=3).
    librestudio_fetch_tarball(libwpd libwpd-0.10.3.tar.xz
        2465b0b662fdc5d4e3bebcdc9a79027713fb629ca2bff04a3c9251fdec42dd09)

    # UnpackedTarball_libwpd.mk's own patch list, at the module's own
    # explicit default patchlevel 0: rpath.patch is CONDITIONAL on
    # $(SYSTEM_REVENGE) being empty (i.e. only added when librevenge itself
    # was vendored) -- reproduced via LIBRESTUDIO_REVENGE_FOUND, set by
    # librevenge's own driver right above. libwpd-bundled-soname.patch.0's
    # own `ifneq(OS,MACOSX) ifneq(OS,WNT) ifneq(OS,iOS)` guard is always
    # taken on this Linux-only port.
    set(_libwpd_patches
        libwpd-vs2013.patch.1
        tdf153034_3_WrongGreekCharactersWP5Import.patch
        include.patch
        0001-const-up-tibetanMap1-to-move-out-of-data-section.patch.1
        0001-const-up-various-arrays.patch.1
        libwpd-bundled-soname.patch.0)
    if (NOT LIBRESTUDIO_REVENGE_FOUND)
        list(APPEND _libwpd_patches rpath.patch)
    endif ()
    librestudio_apply_patches(libwpd "${libwpd_SOURCE_DIR}" "${_librestudio_driver_dir}" 0 ${_libwpd_patches})
    unset(_libwpd_patches)

    # ExternalProject_libwpd.mk's own recipe: --with-pic, shared-only,
    # --without-docs, --disable-tools, --disable-debug, --disable-werror.
    # LDFLAGS rpath only added when librevenge was vendored (see external/
    # librevenge/librevenge.cmake's own header comment on why a real
    # absolute path replaces upstream's own $ORIGIN here).
    set(_libwpd_ldflags "")
    if (NOT LIBRESTUDIO_REVENGE_FOUND)
        set(_libwpd_ldflags "LDFLAGS=-L${LIBRESTUDIO_REVENGE_LIBDIR} -Wl,-rpath,${LIBRESTUDIO_REVENGE_LIBDIR}")
    endif ()

    # Build-ordering dependency on librevenge's own ExternalProject step,
    # only when it was ALSO vendored (a system librevenge needs no such
    # ordering, and the librevenge_ext target only exists on the vendored
    # branch) -- same conditional pattern as external/mythes/mythes.cmake's
    # own hunspell_ext dependency.
    set(_libwpd_extra_depends "")
    if (NOT LIBRESTUDIO_REVENGE_FOUND AND TARGET librevenge_ext)
        set(_libwpd_extra_depends librevenge_ext)
    endif ()

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    ExternalProject_Add(libwpd_ext
        SOURCE_DIR "${libwpd_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libwpd_SOURCE_DIR}/configure"
            --with-pic --enable-shared --disable-static --without-docs
            --disable-tools --disable-debug --disable-werror
            ${_libwpd_ldflags}
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        DEPENDS ${_libwpd_extra_depends}
        BUILD_BYPRODUCTS
            "${libwpd_SOURCE_DIR}/src/lib/.libs/libwpd-0.10.so"
        COMMENT "Building libwpd (autotools: configure && make)")
    unset(_libwpd_ldflags)
    unset(_libwpd_extra_depends)

    # configure.ac's own WPD_CFLAGS_internal/WPD_LIBS_internal (10431-10436).
    add_library(wpd SHARED IMPORTED GLOBAL)
    set_target_properties(wpd PROPERTIES
        IMPORTED_LOCATION "${libwpd_SOURCE_DIR}/src/lib/.libs/libwpd-0.10.so"
        INTERFACE_INCLUDE_DIRECTORIES "${libwpd_SOURCE_DIR}/inc")
    target_link_libraries(wpd INTERFACE ${LIBRESTUDIO_REVENGE_TARGET})
    add_dependencies(wpd libwpd_ext)

    set(LIBRESTUDIO_WPD_LIBDIR "${libwpd_SOURCE_DIR}/src/lib/.libs" CACHE INTERNAL "" FORCE)

    librestudio_vendor_set_target(WPD wpd)
endfunction()
