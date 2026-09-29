# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libwps, part of the writerperfect-family externals --
# see external/librevenge/librevenge.cmake's own header comment for the
# shared shape. Invoked by librestudio_pkgconfig() only when no
# sufficiently new (>= 0.4.14, configure.ac's own libo_PKG_VERSION) system
# libwps-0.4 was found.
#
# On this machine specifically: the system package is 0.4.12, one micro
# version short of the 0.4.14 floor -- so wiring this VERSION constraint in
# (matching writerperfect/CMakeLists.txt's OWN pre-existing bare
# `pkg_check_modules(WPS REQUIRED IMPORTED_TARGET libwps-0.4)`, which never
# enforced this floor at all -- a real, pre-existing staleness this pass
# corrects) means this driver ACTUALLY vendors on this exact host, unlike
# every other library in this family, which all resolve to their system
# package here. A genuine, not-forced real-build validation opportunity.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_wps)
    message(STATUS "libwps: no sufficiently new system package found -- building libwps 0.4.14 from source")

    librestudio_pkgconfig(NAME REVENGE MODULES librevenge-0.0 VERSION 0.0.1 DRIVER librevenge)

    # download.lst's own WPS_TARBALL/WPS_SHA256SUM (WPS_VERSION_MICRO=14).
    librestudio_fetch_tarball(libwps libwps-0.4.14.tar.xz
        365b968e270e85a8469c6b160aa6af5619a4e6c995dbb04c1ecc1b4dd13e80de)

    # UnpackedTarball_libwps.mk's own patch list, default patchlevel 0.
    # rpath.patch.0 conditional on librevenge having been vendored.
    set(_libwps_patches
        libtool.patch.0
        enumarith.patch
        0001-const-up-some-things.patch.1
        tdf148749.patch.1
        libwps-bundled-soname.patch.0)
    if (NOT LIBRESTUDIO_REVENGE_FOUND)
        list(PREPEND _libwps_patches rpath.patch.0)
    endif ()
    librestudio_apply_patches(libwps "${libwps_SOURCE_DIR}" "${_librestudio_driver_dir}" 0 ${_libwps_patches})
    unset(_libwps_patches)

    # ExternalProject_libwps.mk's own recipe: --with-pic, shared-only,
    # --with-sharedptr=c++11, --without-docs, --disable-tools,
    # --disable-debug (ENABLE_DEBUG unset), --disable-werror. LDFLAGS rpath
    # only when librevenge was vendored (own `ifeq ($(SYSTEM_REVENGE),)`
    # guard, unlike libwpg's unconditional one).
    set(_libwps_ldflags "")
    set(_libwps_extra_depends "")
    if (NOT LIBRESTUDIO_REVENGE_FOUND)
        set(_libwps_ldflags "LDFLAGS=-L${LIBRESTUDIO_REVENGE_LIBDIR} -Wl,-rpath,${LIBRESTUDIO_REVENGE_LIBDIR}")
        if (TARGET librevenge_ext)
            set(_libwps_extra_depends librevenge_ext)
        endif ()
    endif ()

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    ExternalProject_Add(libwps_ext
        SOURCE_DIR "${libwps_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libwps_SOURCE_DIR}/configure"
            --with-pic --enable-shared --disable-static
            --with-sharedptr=c++11 --without-docs --disable-tools
            --disable-debug --disable-werror
            ${_libwps_ldflags}
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        DEPENDS ${_libwps_extra_depends}
        BUILD_BYPRODUCTS
            "${libwps_SOURCE_DIR}/src/lib/.libs/libwps-0.4.so"
        COMMENT "Building libwps (autotools: configure && make)")
    unset(_libwps_ldflags)
    unset(_libwps_extra_depends)

    add_library(wps SHARED IMPORTED GLOBAL)
    set_target_properties(wps PROPERTIES
        IMPORTED_LOCATION "${libwps_SOURCE_DIR}/src/lib/.libs/libwps-0.4.so"
        INTERFACE_INCLUDE_DIRECTORIES "${libwps_SOURCE_DIR}/inc")
    target_link_libraries(wps INTERFACE ${LIBRESTUDIO_REVENGE_TARGET})
    add_dependencies(wps libwps_ext)

    librestudio_vendor_set_target(WPS wps)
endfunction()
