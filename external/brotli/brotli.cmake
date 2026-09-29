# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for brotli, called DIRECTLY by external/libjxl/libjxl.cmake
# (its only real consumer) -- NOT invoked through librestudio_pkgconfig()/
# librestudio_find_package() like every other driver in this port.
# configure.ac never probes a system brotli package at all (grepped: no
# SYSTEM_BROTLI, no with_system_brotli, no PKG_CHECK_MODULES(BROTLI...)
# anywhere), and RepositoryExternal.mk's own gb_LinkTarget__use_brotli is
# gated only by `ifneq ($(filter BROTLI,$(BUILD_TYPE)),)` -- BROTLI is
# unconditionally added to BUILD_TYPE whenever libjxl itself needs
# building (external/libjxl/Makefile's own BUILD_TYPE addition, transitively
# reached only through libjxl), never behind a SYSTEM_BROTLI switch. There
# is nothing to system-or-vendor resolve here: brotli is ALWAYS built from
# source, so this driver is just a plain function, called unconditionally.
#
# A genuine gbuild StaticLibrary direct-compile (Library_clucene.mk's own
# shape, not an ExternalProject autotools build) -- transcribed from
# external/brotli/StaticLibrary_brotli.mk, read in full: 34 plain .c files
# across c/common, c/enc, c/dec, no patches at all (UnpackedTarball_brotli.mk
# has no gb_UnpackedTarball_add_patches call whatsoever).
function(librestudio_vendor_brotli)
    if (TARGET brotli)
        return()
    endif ()
    message(STATUS "brotli: building brotli 1.2.0 from source (no system option upstream)")

    # download.lst's own BROTLI_TARBALL/BROTLI_SHA256SUM, verified directly
    # (grep -n brotli download.lst).
    librestudio_fetch_tarball(brotli brotli-1.2.0.tar.gz
        816c96e8e8f193b40151dad7e8ff37b1221d019dbcb9c35cd3fadbfe6477dfec)

    set(_brotli_common_sources
        constants
        context
        dictionary
        platform
        shared_dictionary
        transform)
    set(_brotli_enc_sources
        backward_references
        backward_references_hq
        bit_cost
        block_splitter
        brotli_bit_stream
        cluster
        command
        compound_dictionary
        compress_fragment
        compress_fragment_two_pass
        dictionary_hash
        encode
        encoder_dict
        entropy_encode
        fast_log
        histogram
        literal_cost
        memory
        metablock
        static_dict
        static_dict_lut
        static_init
        utf8_util)
    set(_brotli_dec_sources
        bit_reader
        decode
        huffman
        prefix
        state
        static_init)

    set(_brotli_sources "")
    foreach (_f IN LISTS _brotli_common_sources)
        list(APPEND _brotli_sources "${brotli_SOURCE_DIR}/c/common/${_f}.c")
    endforeach ()
    foreach (_f IN LISTS _brotli_enc_sources)
        list(APPEND _brotli_sources "${brotli_SOURCE_DIR}/c/enc/${_f}.c")
    endforeach ()
    foreach (_f IN LISTS _brotli_dec_sources)
        list(APPEND _brotli_sources "${brotli_SOURCE_DIR}/c/dec/${_f}.c")
    endforeach ()
    unset(_f)

    add_library(brotli STATIC ${_brotli_sources})
    # StaticLibrary_brotli.mk's own gb_StaticLibrary_set_include: the
    # unpacked tree's own top level (internal headers) plus c/include
    # (the public <brotli/*.h> headers).
    target_include_directories(brotli PUBLIC
        "${brotli_SOURCE_DIR}"
        "${brotli_SOURCE_DIR}/c/include")
    # gb_StaticLibrary_set_warnings_disabled,brotli
    target_compile_options(brotli PRIVATE -w)
    set_target_properties(brotli PROPERTIES POSITION_INDEPENDENT_CODE ON)

    librestudio_vendor_set_target(BROTLI brotli)
endfunction()
