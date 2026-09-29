# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for zlib, invoked by librestudio_find_package() (cmake/
# LibreStudioTarballs.cmake, called from whichever real consumer -- vcl,
# comphelper, sal, ... -- first needs zlib) only when no sufficiently new
# system zlib was found.
#
# _librestudio_driver_dir captured here, at this file's own top level (NOT
# inside the function below): CMAKE_CURRENT_LIST_DIR correctly reflects
# this file's own directory only WHILE include() is actively processing it
# -- by the time librestudio_vendor_zlib() is actually CALLED (from
# cmake/LibreStudioTarballs.cmake's own _librestudio_resolve_or_vendor(),
# in whichever real consumer's directory scope triggered resolution), a
# reference to CMAKE_CURRENT_LIST_DIR INSIDE the function body would
# instead reflect that caller's own file, not this one (verified directly
# with a throwaway `cmake -P` reproduction before relying on this pattern).
# A plain, un-PARENT_SCOPEd set() here is still visible inside the function
# when it runs later, via CMake's dynamic (call-stack) variable scoping.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_zlib)
    message(STATUS "zlib: no system package found -- building zlib 1.3.2 from source")

    # download.lst's own ZLIB_TARBALL/ZLIB_SHA256SUM, verified directly and
    # cross-checked against the real downloaded file's own sha256sum.
    librestudio_fetch_tarball(zlib zlib-1.3.2.tar.xz
        d7a0654783a4da529d1bb793b7ad9c3318020af77667bcae35f95d0e42a792f3)

    # UnpackedTarball_zlib.mk's own patch list (patchlevel 0, both dry-run
    # verified against the real fetched tarball at that level before
    # writing this loop -- level 1 fails both).
    librestudio_apply_patches(zlib "${zlib_SOURCE_DIR}" "${_librestudio_driver_dir}" 0
        missinginclude.patch werror-undef.patch)

    # StaticLibrary_zlib.mk's own source list, transcribed verbatim
    # (re-verified against the real 1.3.2 tarball: all 14 files exist, 0
    # missing).
    set(_zlib_sources
        adler32
        compress
        crc32
        deflate
        gzclose
        gzlib
        gzread
        gzwrite
        inffast
        inflate
        inftrees
        trees
        uncompr
        zutil)

    set(_zlib_abs_sources "")
    foreach (_f IN LISTS _zlib_sources)
        list(APPEND _zlib_abs_sources "${zlib_SOURCE_DIR}/${_f}.c")
    endforeach ()
    unset(_f)

    add_library(zlib STATIC ${_zlib_abs_sources})
    target_include_directories(zlib PUBLIC "${zlib_SOURCE_DIR}")
    # gb_StaticLibrary_set_warnings_disabled,zlib
    target_compile_options(zlib PRIVATE -w)
    # ENABLE_DEBUG's -DZLIB_DEBUG (StaticLibrary_zlib.mk:35-37): this port
    # has no ENABLE_DEBUG flag anywhere (grepped), so this branch of the
    # .mk never applies here -- not reproduced, matching how every other
    # undefined-flag branch in this port is already treated.

    librestudio_vendor_set_target(ZLIB zlib)
endfunction()
