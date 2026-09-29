# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libwebp, invoked by librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake, called from vcl/CMakeLists.txt -- the only
# real consumer) only when no system libwebp was found.
#
# A genuine two-library case, like external/coinmp/coinmp.cmake:
# RepositoryExternal.mk's own !SYSTEM_LIBWEBP gb_LinkTarget__use_libwebp
# (2887-2911, non-WNT arm) links BOTH `-L.../libwebp/src/.libs -lwebp` AND
# `-L.../libwebp/sharpyuv/.libs -lsharpyuv` -- libwebp's own build split
# the YUV<->sRGB colour-conversion helper into its own sub-library some
# releases back, transcribed here as two IMPORTED STATIC targets rather
# than one.
#
# _librestudio_driver_dir: see external/zlib/zlib.cmake's own header
# comment for why this is captured at file top level, outside the function.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_libwebp)
    message(STATUS "libwebp: no system package found -- building libwebp 1.6.0 from source")

    # download.lst's own LIBWEBP_TARBALL/LIBWEBP_SHA256SUM, verified
    # directly (grep -n libwebp download.lst).
    librestudio_fetch_tarball(libwebp libwebp-1.6.0.tar.gz
        e4ab7009bf0629fd11982d4c2aa83964cf244cffba7347ecd39019a9e38c4564)

    # UnpackedTarball_libwebp.mk's own single patch (Makefile.vc.patch) is
    # COM=MSC-only in spirit (a Windows nmake makefile) but, like lcms2's
    # own Windows-named patch, not actually gated in the .mk -- applied
    # unconditionally at the module's own explicit patchlevel 0
    # (gb_UnpackedTarball_set_patchlevel,libwebp,0). Harmless here: the
    # patch only touches Makefile.vc, a file this Linux/autoconf build
    # path never reads.
    librestudio_apply_patches(libwebp "${libwebp_SOURCE_DIR}" "${_librestudio_driver_dir}" 0
        Makefile.vc.patch)

    # ExternalProject_libwebp.mk's own non-MSC branch calls
    # gb_ExternalProject_use_autoconf,libwebp,build -- a genuine no-op on
    # the GCC/Linux platform class (solenv/gbuild/platform/com_GCC_class.mk:
    # `gb_ExternalProject_use_autoconf :=`, confirmed by reading it; only
    # solenv/gbuild/platform/com_MSC_class.mk gives it a real body), so no
    # autoreconf step is needed -- the tarball already ships a pre-generated
    # `configure`, same as every other vendored external here.
    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    # Options transcribed literally from the recipe's own non-MSC branch:
    # static-only build (--enable-static --disable-shared --with-pic), and
    # every optional codec/backend this port's own libwebp does not need
    # disabled (gl/sdl/png/jpeg/tiff/gif/wic) -- avoids pulling in
    # find-at-configure-time dependencies this driver does not resolve
    # itself. No "cd" -- builds at the unpacked tree's own top level,
    # which recurses into sharpyuv/ on its own (a real libwebp Makefile.am
    # SUBDIRS entry).
    ExternalProject_Add(libwebp_ext
        SOURCE_DIR "${libwebp_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libwebp_SOURCE_DIR}/configure"
            --enable-static --with-pic --disable-shared
            --disable-gl --disable-sdl --disable-png --disable-jpeg
            --disable-tiff --disable-gif --disable-wic --enable-silent-rules
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${libwebp_SOURCE_DIR}/src/.libs/libwebp.a"
            "${libwebp_SOURCE_DIR}/sharpyuv/.libs/libsharpyuv.a"
        COMMENT "Building libwebp (autotools: configure && make)")

    # Include dir is the unpacked tree's own top-level src/ (webp/*.h
    # headers live there); both archives share it.
    add_library(webp STATIC IMPORTED GLOBAL)
    set_target_properties(webp PROPERTIES
        IMPORTED_LOCATION "${libwebp_SOURCE_DIR}/src/.libs/libwebp.a"
        INTERFACE_INCLUDE_DIRECTORIES "${libwebp_SOURCE_DIR}/src")
    add_dependencies(webp libwebp_ext)

    add_library(sharpyuv STATIC IMPORTED GLOBAL)
    set_target_properties(sharpyuv PROPERTIES
        IMPORTED_LOCATION "${libwebp_SOURCE_DIR}/sharpyuv/.libs/libsharpyuv.a")
    add_dependencies(sharpyuv libwebp_ext)

    librestudio_vendor_set_target(LIBWEBP "webp;sharpyuv")
endfunction()
