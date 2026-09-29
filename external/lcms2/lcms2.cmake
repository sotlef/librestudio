# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for lcms2, invoked by librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake, called from vcl/CMakeLists.txt -- the only
# real consumer) only when no system lcms2 was found.
#
# ExternalProject_lcms2.mk's own non-MSC branch (transcribed, read in
# full): `./configure --without-jpeg --without-tiff --with-pic ...
# --enable-shared --disable-static && cd src && make` -- this port has no
# DISABLE_DYNLOADING equivalent (grepped: never set anywhere in this tree),
# so the upstream `$(if $(DISABLE_DYNLOADING), --enable-static
# --disable-shared, --enable-shared --disable-static)` branch always
# resolves to the shared-library arm here, matching RepositoryExternal.mk's
# own LCMS2_LIBS_internal (-llcms2, a *shared*-library link flag, not a
# static archive path) and ExternalPackage_lcms2.mk's own non-Android/non-
# MSC packaging arm (liblcms2.so.2.0.19).
#
# _librestudio_driver_dir: see external/zlib/zlib.cmake's own header
# comment for why this is captured at file top level, outside the function.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_lcms2)
    message(STATUS "lcms2: no system package found -- building lcms2 2.19.1 from source")

    # download.lst's own LCMS2_TARBALL/LCMS2_SHA256SUM, verified directly
    # (grep -n lcms2 download.lst).
    librestudio_fetch_tarball(lcms2 lcms2-2.19.1.tar.gz
        bfc54f7bab59fbc921012014a8032e4cba4abd46db47d46b76416a8c0b2815c8)

    # UnpackedTarball_lcms2.mk's own 3 patches, at the module's own
    # explicit default patchlevel 3 (gb_UnpackedTarball_set_patchlevel,
    # lcms2,3 -- unlike every other driver in this port so far, lcms2 DOES
    # set one explicitly). lcms2-2.4-windows.patch carries no numeric
    # suffix so it falls through to that level-3 default even though its
    # name reads Windows-only -- applied unconditionally in the real .mk
    # (not COM=MSC-gated), same "transcribe, don't guess" policy as
    # external/libnumbertext/libnumbertext.cmake's own Windows-named
    # patches. The other two carry their own ".1" suffix, overriding to
    # level 1 per this port's own numeric-suffix convention.
    librestudio_apply_patches(lcms2 "${lcms2_SOURCE_DIR}" "${_librestudio_driver_dir}" 3
        lcms2-2.4-windows.patch
        0002-mark-more-const.patch.1
        0003-mark-more-const.patch.1)

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    ExternalProject_Add(lcms2_ext
        SOURCE_DIR "${lcms2_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${lcms2_SOURCE_DIR}/configure"
            --without-jpeg --without-tiff --with-pic
            --enable-shared --disable-static
        BUILD_COMMAND "${MAKE_EXECUTABLE}" -C "${lcms2_SOURCE_DIR}/src"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${lcms2_SOURCE_DIR}/src/.libs/liblcms2.so.2.0.19"
        COMMENT "Building lcms2 (autotools: configure && make)")

    # RepositoryExternal.mk's own !SYSTEM_LCMS2, non-Android
    # gb_LinkTarget__use_lcms2 (2600-2612): include dir is the unpacked
    # tree's own top-level include/, link against the built shared library
    # directly (configure.ac's own LCMS2_LIBS_internal literally names
    # "-llcms2", the unversioned libtool symlink libtool itself creates
    # next to the real liblcms2.so.2.0.19 inside .libs/ for uninstalled
    # linking -- IMPORTED_LOCATION points at the real versioned file since
    # that is the one BUILD_BYPRODUCTS above can name concretely).
    #
    # ExternalPackage_lcms2.mk's own runtime install step (copying
    # liblcms2.so.2.0.19 into LIBO_LIB_FOLDER, i.e. instdir/program/) is
    # NOT reproduced here -- pure runtime packaging, same class of
    # deliberate gap as external/libexttextcat/libexttextcat.cmake's own
    # langclass data, and consistent with how external/coinmp/coinmp.cmake
    # already left its own 8 shared libraries un-installed (IMPORTED SHARED
    # + rpath only, no install()).
    add_library(lcms2 SHARED IMPORTED GLOBAL)
    set_target_properties(lcms2 PROPERTIES
        IMPORTED_LOCATION "${lcms2_SOURCE_DIR}/src/.libs/liblcms2.so.2.0.19"
        INTERFACE_INCLUDE_DIRECTORIES "${lcms2_SOURCE_DIR}/include")
    add_dependencies(lcms2 lcms2_ext)

    librestudio_vendor_set_target(LCMS2 lcms2)
endfunction()
