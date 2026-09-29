# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libpng, invoked by librestudio_find_package() (cmake/
# LibreStudioTarballs.cmake, called from sdext/desktop/vcl/writerperfect's
# own CMakeLists.txt -- whichever runs first) only when no sufficiently new
# system libpng was found.
#
# _librestudio_driver_dir: see external/zlib/zlib.cmake's own header
# comment for why this is captured at file top level, outside the function
# -- used below both for configure_file()'s own source and (implicitly,
# since ZLIB's own DRIVER is omitted) NOT for the recursive ZLIB call,
# which relies on auto-detection instead (NAME=ZLIB -> candidate "zlib" ->
# external/zlib/zlib.cmake exists).
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_libpng)
    message(STATUS "libpng: no system package found -- building libpng 1.6.58 from source")

    # Own dependency resolved lazily, right here, instead of relying on any
    # guaranteed external order (there is none anymore) -- a real
    # demonstration that the memoized librestudio_find_package() cache makes
    # order stop mattering for correctness: whichever of ZLIB/PNG's own real
    # consumers happens to resolve ZLIB first, this call returns instantly
    # from cache; if none has yet, this call resolves it right now instead.
    librestudio_find_package(NAME ZLIB TARGET_VAR _libpng_zlib_target)

    # download.lst's own LIBPNG_TARBALL/LIBPNG_SHA256SUM, verified directly
    # and cross-checked against the real downloaded file's own sha256sum.
    librestudio_fetch_tarball(libpng libpng-1.6.58.tar.xz
        28eb403f51f0f7405249132cecfe82ea5c0ef97f1b32c5a65828814ae0d34775)

    # UnpackedTarball_libpng.mk: no patches, just one pre-built config
    # header copied to the unpacked tree's root (upstream normally
    # generates this via its own build scripts; this port, like every
    # other vendored external here, ships the already-resolved result
    # directly instead of reproducing that generator).
    configure_file(
        "${_librestudio_driver_dir}/configs/pnglibconf.h"
        "${libpng_SOURCE_DIR}/pnglibconf.h"
        COPYONLY)

    # StaticLibrary_libpng.mk's own source list, transcribed verbatim
    # (re-verified against the real 1.6.58 tarball: all files exist, 0
    # missing). The ARM/POWERPC branches are not reproduced -- this port
    # only derives LIBRESTUDIO_CPUNAME for X86_64 targets so far (see
    # cmake/LibreStudioPlatform.cmake's own header comment on the 64-bit
    # gate); only the X86_64/INTEL branch is real and reachable here.
    set(_libpng_sources
        png
        pngerror
        pngget
        pngmem
        pngpread
        pngread
        pngrio
        pngrtran
        pngrutil
        pngset
        pngtrans
        pngwio
        pngwrite
        pngwtran
        pngwutil)
    if (LIBRESTUDIO_CPUNAME STREQUAL "X86_64" OR LIBRESTUDIO_CPUNAME STREQUAL "INTEL")
        list(APPEND _libpng_sources
            intel/intel_init
            intel/filter_sse2_intrinsics)
    endif ()

    set(_libpng_abs_sources "")
    foreach (_f IN LISTS _libpng_sources)
        list(APPEND _libpng_abs_sources "${libpng_SOURCE_DIR}/${_f}.c")
    endforeach ()
    unset(_f)

    add_library(libpng STATIC ${_libpng_abs_sources})
    target_include_directories(libpng PUBLIC "${libpng_SOURCE_DIR}")
    # gb_StaticLibrary_set_warnings_disabled,libpng
    target_compile_options(libpng PRIVATE -w)
    if (LIBRESTUDIO_CPUNAME STREQUAL "X86_64" OR LIBRESTUDIO_CPUNAME STREQUAL "INTEL")
        target_compile_definitions(libpng PRIVATE PNG_INTEL_SSE_OPT)
    endif ()
    target_link_libraries(libpng PUBLIC ${_libpng_zlib_target})

    librestudio_vendor_set_target(PNG libpng)
endfunction()
