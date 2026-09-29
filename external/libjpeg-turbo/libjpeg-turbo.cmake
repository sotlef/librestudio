# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libjpeg-turbo, invoked by librestudio_find_package()
# (cmake/LibreStudioTarballs.cmake, PACKAGE JPEG / PACKAGE_TARGET
# JPEG::JPEG) only when no system libjpeg was found. Real consumers: vcl/
# CMakeLists.txt, sdext/CMakeLists.txt (xpdfimport) and external/poppler/
# poppler.cmake's own internal JPEG dependency -- all three previously
# called plain `find_package(JPEG REQUIRED)` directly (this port's own
# earlier note: "libjpeg-turbo itself is not yet one of this port's
# vendored externals... stays a plain system find_package() for now") --
# converted to librestudio_find_package(), 2026-09-29.
#
# configure.ac itself never uses pkg-config for this one (grepped: plain
# AC_CHECK_HEADER(jpeglib.h)/AC_CHECK_LIB(jpeg, jpeg_resync_to_restart, ...)
# -- exactly what CMake's own FindJPEG.cmake module already does), which is
# why this is librestudio_find_package() (PACKAGE JPEG), not
# librestudio_pkgconfig() -- same reasoning as external/zlib/external/libpng.
#
# A genuine gbuild StaticLibrary direct-compile (like libpng/graphite/md4c/
# libjxl/clucene), NOT an autotools ExternalProject -- transcribed from
# external/libjpeg-turbo/StaticLibrary_libjpeg-turbo.mk, read in full: 58
# portable C files unconditionally, PLUS an x86 SIMD arm (NASM assembly)
# gated the same way upstream gates it -- `ifneq ($(NASM),)`, i.e. only
# when a real nasm/nasmw/yasm executable is found (configure.ac:10201's own
# AC_CHECK_PROGS list) -- reproduced here as find_program(... nasm nasmw
# yasm). Neither nasm, nasmw nor yasm is installed on this machine, so only
# the portable-C arm has actually been build-tested (see PORTING_STATUS.md/
# memory for that run); the NASM arm is structurally transcribed (same
# "correct but not build-verified on this host" status this port already
# accepted for bridges/Library_cpp_uno.mk's own 21-branch CPU table).
#
# Driver function is librestudio_vendor_jpeg(), not ..._libjpeg_turbo(): the
# NAME passed to librestudio_find_package() is JPEG (matching CMake's own
# FindJPEG module and its JPEG::JPEG target, since configure.ac's own probe
# for this one is a plain AC_CHECK_HEADER/AC_CHECK_LIB pair, not pkg-config
# -- see below), and _librestudio_resolve_or_vendor() always calls
# librestudio_vendor_<TOLOWER(NAME)>() -- the DRIVER file's own path is a
# separate, independently-chosen short name (here "libjpeg-turbo", passed
# explicitly since neither "jpeg" nor "libjpeg" is this directory's real
# name).
# _librestudio_driver_dir: see external/zlib/zlib.cmake's own header
# comment for why this is captured at file top level, outside the function.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_jpeg)
    message(STATUS "libjpeg-turbo: no system package found -- building libjpeg-turbo 3.1.4.1 from source")

    # download.lst's own LIBJPEG_TURBO_TARBALL/LIBJPEG_TURBO_SHA256SUM,
    # verified directly (grep -n libjpeg_turbo download.lst).
    librestudio_fetch_tarball(libjpeg_turbo libjpeg-turbo-3.1.4.1.tar.gz
        ecae8008e2cc9ade2f2c1bb9d5e6d4fb73e7c433866a056bd82980741571a022)

    # UnpackedTarball_libjpeg-turbo.mk's own 2 patches, at the module's own
    # explicit default patchlevel 0 (gb_UnpackedTarball_set_patchlevel,
    # libjpeg-turbo,0) -- both patches address "src/..." paths directly
    # with no a//b/ prefix, consistent with level 0.
    librestudio_apply_patches(libjpeg_turbo "${libjpeg_turbo_SOURCE_DIR}" "${_librestudio_driver_dir}" 0
        include.patch
        undefined_references.patch)

    # UnpackedTarball_libjpeg-turbo.mk's own 3 gb_UnpackedTarball_add_file
    # calls: upstream's own CMake build normally generates jconfig.h/
    # jconfigint.h/jversion.h via configure_file()-equivalent substitution
    # from jconfig.h.in etc; the .mk's own comment records the exact
    # one-time `cmake -DENABLE_STATIC:BOOL=ON -DENABLE_SHARED:BOOL=NO
    # -DWITH_JAVA:BOOL=OFF -DWITH_TURBOJPEG:BOOL=OFF -DWITH_SIMD:BOOL=ON`
    # invocation LibreOffice used to produce the checked-in result (plus
    # manual tweaking) -- reproduced here as a direct file copy, same
    # convention as external/libpng/libpng.cmake's own pnglibconf.h.
    foreach (_hdr jconfigint.h jconfig.h jversion.h)
        configure_file(
            "${_librestudio_driver_dir}/${_hdr}"
            "${libjpeg_turbo_SOURCE_DIR}/src/${_hdr}"
            COPYONLY)
    endforeach ()
    unset(_hdr)

    # StaticLibrary_libjpeg-turbo.mk's own -DSIZEOF_SIZE_T=$(SIZEOF_SIZE_T)
    # -- gbuild's own value comes from configure.ac's ac_cv_sizeof_size_t
    # (autoconf's AC_CHECK_SIZEOF); CMake's own CheckTypeSize module is the
    # exact equivalent, not a reinvention.
    include(CheckTypeSize)
    check_type_size("size_t" LIBJPEG_TURBO_SIZEOF_SIZE_T)

    set(_libjpeg_turbo_sources
        jaricom jcapimin jcapistd jcarith jccoefct jccolor jcdctmgr jcdiffct
        jchuff jcicc jcinit jclhuff jclossls jcmainct jcmarker jcmaster
        jcomapi jcparam jcphuff jcprepct jcsample jctrans jdapimin jdapistd
        jdarith jdatadst jdatasrc jdcoefct jdcolor jddctmgr jddiffct jdhuff
        jdicc jdinput jdlhuff jdlossls jdmainct jdmarker jdmaster jdmerge
        jdphuff jdpostct jdsample jdtrans jerror jfdctflt jfdctfst jfdctint
        jidctflt jidctfst jidctint jidctred jmemmgr jmemnobs jpeg_nbits
        jquant1 jquant2 jutils)
    set(_libjpeg_turbo_abs_sources "")
    foreach (_f IN LISTS _libjpeg_turbo_sources)
        list(APPEND _libjpeg_turbo_abs_sources "${libjpeg_turbo_SOURCE_DIR}/src/${_f}.c")
    endforeach ()
    unset(_f)

    # StaticLibrary_libjpeg-turbo.mk's own `ifneq ($(NASM),)` gate --
    # configure.ac:10201's own AC_CHECK_PROGS(NASM, [nasm nasmw yasm]), so
    # any of the three qualifies, not just "nasm" literally.
    find_program(LIBJPEG_TURBO_NASM_EXECUTABLE NAMES nasm nasmw yasm)
    set(_libjpeg_turbo_simd FALSE)
    if (LIBJPEG_TURBO_NASM_EXECUTABLE AND
        (LIBRESTUDIO_CPUNAME STREQUAL "X86_64" OR LIBRESTUDIO_CPUNAME STREQUAL "INTEL"))
        set(_libjpeg_turbo_simd TRUE)
    endif ()

    if (_libjpeg_turbo_simd)
        enable_language(ASM_NASM)
        if (LIBRESTUDIO_CPUNAME STREQUAL "X86_64")
            set(_libjpeg_turbo_simd_dir "simd/x86_64")
            list(APPEND _libjpeg_turbo_abs_sources "${libjpeg_turbo_SOURCE_DIR}/${_libjpeg_turbo_simd_dir}/jsimd.c")
            set(_libjpeg_turbo_asm_names
                jsimdcpu jfdctflt-sse jccolor-sse2 jcgray-sse2 jchuff-sse2
                jcphuff-sse2 jcsample-sse2 jdcolor-sse2 jdmerge-sse2
                jdsample-sse2 jfdctfst-sse2 jfdctint-sse2 jidctflt-sse2
                jidctfst-sse2 jidctint-sse2 jidctred-sse2 jquantf-sse2
                jquanti-sse2 jccolor-avx2 jcgray-avx2 jcsample-avx2
                jdcolor-avx2 jdmerge-avx2 jdsample-avx2 jfdctint-avx2
                jidctint-avx2 jquanti-avx2)
        else ()
            set(_libjpeg_turbo_simd_dir "simd/i386")
            list(APPEND _libjpeg_turbo_abs_sources "${libjpeg_turbo_SOURCE_DIR}/${_libjpeg_turbo_simd_dir}/jsimd.c")
            set(_libjpeg_turbo_asm_names
                jsimdcpu jfdctflt-3dn jidctflt-3dn jquant-3dn jccolor-mmx
                jcgray-mmx jcsample-mmx jdcolor-mmx jdmerge-mmx jdsample-mmx
                jfdctfst-mmx jfdctint-mmx jidctfst-mmx jidctint-mmx
                jidctred-mmx jquant-mmx jfdctflt-sse jidctflt-sse
                jquant-sse jccolor-sse2 jcgray-sse2 jchuff-sse2
                jcphuff-sse2 jcsample-sse2 jdcolor-sse2 jdmerge-sse2
                jdsample-sse2 jfdctfst-sse2 jfdctint-sse2 jidctflt-sse2
                jidctfst-sse2 jidctint-sse2 jidctred-sse2 jquantf-sse2
                jquanti-sse2 jccolor-avx2 jcgray-avx2 jcsample-avx2
                jdcolor-avx2 jdmerge-avx2 jdsample-avx2 jfdctint-avx2
                jidctint-avx2 jquanti-avx2)
        endif ()
        set(_libjpeg_turbo_asm_sources "")
        foreach (_f IN LISTS _libjpeg_turbo_asm_names)
            list(APPEND _libjpeg_turbo_asm_sources
                "${libjpeg_turbo_SOURCE_DIR}/${_libjpeg_turbo_simd_dir}/${_f}.asm")
        endforeach ()
        unset(_f)
        list(APPEND _libjpeg_turbo_abs_sources ${_libjpeg_turbo_asm_sources})
    endif ()

    add_library(libjpeg_turbo STATIC ${_libjpeg_turbo_abs_sources})
    # StaticLibrary_libjpeg-turbo.mk's own gb_StaticLibrary_set_include:
    # the unpacked tree's own top level (src/jconfig.h etc, plus src/ and
    # simd/ headers reachable relative to it).
    target_include_directories(libjpeg_turbo PUBLIC "${libjpeg_turbo_SOURCE_DIR}")
    # gb_StaticLibrary_set_warnings_disabled,libjpeg-turbo
    target_compile_options(libjpeg_turbo PRIVATE -w)
    set_target_properties(libjpeg_turbo PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_compile_definitions(libjpeg_turbo PRIVATE
        SIZEOF_SIZE_T=${LIBJPEG_TURBO_SIZEOF_SIZE_T})
    # StaticLibrary_libjpeg-turbo.mk's own `ifeq ($(COM),GCC) ifneq
    # ($(ENABLE_OPTIMIZED),) -O3` -- this port has no ENABLE_OPTIMIZED
    # equivalent (a debug-vs-release distinction not yet modeled, same
    # honestly-surfaced gap as OSL_DEBUG_LEVEL in cmake/
    # LibreStudioPlatform.cmake); CMAKE_BUILD_TYPE's own Release/RelWithDebInfo
    # -O flags already cover the same intent without a separate flag here.

    if (_libjpeg_turbo_simd)
        target_compile_definitions(libjpeg_turbo PRIVATE WITH_SIMD)
        set_source_files_properties(${_libjpeg_turbo_asm_sources} PROPERTIES
            COMPILE_OPTIONS
                "-I${libjpeg_turbo_SOURCE_DIR}/simd/nasm/;-I${libjpeg_turbo_SOURCE_DIR}/${_libjpeg_turbo_simd_dir}/")
    endif ()

    librestudio_vendor_set_target(JPEG libjpeg_turbo)
endfunction()
