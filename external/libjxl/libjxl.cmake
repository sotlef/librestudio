# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libjxl, invoked by librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake, called from vcl/CMakeLists.txt -- the only
# real consumer, vcl/source/filter/jxl/reader.cxx) only when no
# sufficiently new system libjxl+libjxl_threads pair was found.
#
# configure.ac's own check (libo_CHECK_SYSTEM_MODULE([libjxl],[LIBJXL],
# [libjxl libjxl_threads]), grepped and read directly) probes BOTH pkg-config
# modules together into ONE combined LIBJXL_CFLAGS/LIBJXL_LIBS pair, and
# Library_vcl.mk's own gb_Library_use_externals list has a single "libjxl"
# entry, not two -- there is no separate "libjxl_threads" external anywhere
# in RepositoryExternal.mk at all. vcl/CMakeLists.txt's own PRE-2026-09-29
# code had invented a second, separate JXL_THREADS pkg_check_modules() call
# that does not mirror this upstream structure (functionally harmless on
# the system branch, since pkg-config resolves either way, but structurally
# wrong for a system-or-vendor router with one driver per logical
# dependency) -- corrected there to a single NAME LIBJXL / MODULES
# "libjxl libjxl_threads" call, matching upstream exactly.
#
# StaticLibrary_libjxl.mk (read in full) compiles BOTH libjxl's own
# codec sources AND the one libjxl_threads source (lib/threads/
# resizable_parallel_runner.cc) into ONE static archive -- so on this
# vendored branch there really is only one target, same as upstream's own
# combined-into-one-.mk structure.
#
# Needs external/brotli/brotli.cmake and external/highway/highway.cmake
# (own real Library_vcl.mk/RepositoryExternal.mk dependencies -- see
# StaticLibrary_libjxl.mk's own gb_StaticLibrary_use_externals list) --
# called DIRECTLY here, not through librestudio_pkgconfig()/
# librestudio_find_package(), since neither has a system-package option
# upstream at all (see each driver's own header comment). lcms2 IS a
# real system-or-vendor dependency (StaticLibrary_libjxl.mk's third
# gb_StaticLibrary_use_externals entry) and is resolved the normal way,
# lazily, exactly like external/poppler/poppler.cmake's own recursive
# ZLIB resolution -- whichever of vcl/writerperfect/libjxl happens to
# resolve LCMS2 first does the real work, the other two calls return
# instantly from cache.
#
# _librestudio_driver_dir: see external/zlib/zlib.cmake's own header
# comment for why this is captured at file top level, outside the function.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_libjxl)
    message(STATUS "libjxl: no system package found -- building libjxl 0.11.2 from source")

    include("${CMAKE_SOURCE_DIR}/external/brotli/brotli.cmake")
    librestudio_vendor_brotli()
    include("${CMAKE_SOURCE_DIR}/external/highway/highway.cmake")
    librestudio_vendor_highway()
    librestudio_pkgconfig(NAME LCMS2 MODULES lcms2 TARGET_VAR _libjxl_lcms2_target)

    # download.lst's own LIBJXL_TARBALL/LIBJXL_SHA256SUM, verified directly
    # (grep -n libjxl download.lst).
    librestudio_fetch_tarball(libjxl libjxl-0.11.2.tar.gz
        ab38928f7f6248e2a98cc184956021acb927b16a0dee71b4d260dc040a4320ea)

    # UnpackedTarball_libjxl.mk's own gb_UnpackedTarball_set_exclude_pattern
    # (tools/benchmark/metrics, symlinks a Windows tar cannot handle) is not
    # reproduced -- CMake's own file(ARCHIVE_EXTRACT) on Linux has no such
    # limitation, and the source list below never touches that directory
    # anyway. windows-compile-directives.patch.0 is COM=MSC-gated in the
    # real .mk and not applied here (not a Windows build).
    #
    # UnpackedTarball_libjxl.mk's own gb_UnpackedTarball_add_files: 4 small
    # pre-generated export/version headers copied into lib/include/jxl/ --
    # upstream libjxl's own CMake build normally generates these via
    # configure_file()-equivalent macro expansion from source; LibreOffice
    # ships the already-resolved result directly instead (same convention
    # this port already followed for external/libpng/libpng.cmake's own
    # pnglibconf.h).
    foreach (_hdr jxl_cms_export.h jxl_export.h jxl_threads_export.h version.h)
        configure_file(
            "${_librestudio_driver_dir}/${_hdr}"
            "${libjxl_SOURCE_DIR}/lib/include/jxl/${_hdr}"
            COPYONLY)
    endforeach ()
    unset(_hdr)

    # StaticLibrary_libjxl.mk's own source list, transcribed verbatim
    # (re-verified against the real 0.11.2 tarball: all 60 files exist, 0
    # missing) -- a deliberately reduced subset of libjxl's full upstream
    # tree (no per-SIMD-target recompiled render/dec variants, no jpeg
    # transcoding, no CLI tools), matching upstream LibreOffice's own
    # already-made scope decision, not re-derived here.
    set(_libjxl_sources
        lib/jxl/ac_strategy
        lib/jxl/alpha
        lib/jxl/ans_common
        lib/jxl/blending
        lib/jxl/box_content_decoder
        lib/jxl/chroma_from_luma
        lib/jxl/coeff_order
        lib/jxl/color_encoding_internal
        lib/jxl/compressed_dc
        lib/jxl/dec_ans
        lib/jxl/dec_cache
        lib/jxl/dec_context_map
        lib/jxl/dec_frame
        lib/jxl/dec_group
        lib/jxl/dec_group_border
        lib/jxl/dec_huffman
        lib/jxl/dec_modular
        lib/jxl/dec_noise
        lib/jxl/dec_patch_dictionary
        lib/jxl/dec_xyb
        lib/jxl/decode
        lib/jxl/encode
        lib/jxl/entropy_coder
        lib/jxl/epf
        lib/jxl/fields
        lib/jxl/frame_header
        lib/jxl/headers
        lib/jxl/huffman_table
        lib/jxl/icc_codec
        lib/jxl/icc_codec_common
        lib/jxl/image
        lib/jxl/image_bundle
        lib/jxl/image_metadata
        lib/jxl/loop_filter
        lib/jxl/memory_manager_internal
        lib/jxl/modular/encoding/dec_ma
        lib/jxl/modular/encoding/encoding
        lib/jxl/modular/modular_image
        lib/jxl/modular/transform/palette
        lib/jxl/modular/transform/rct
        lib/jxl/modular/transform/squeeze
        lib/jxl/modular/transform/transform
        lib/jxl/opsin_params
        lib/jxl/passes_state
        lib/jxl/quant_weights
        lib/jxl/quantizer
        lib/jxl/render_pipeline/low_memory_render_pipeline
        lib/jxl/render_pipeline/render_pipeline
        lib/jxl/render_pipeline/simple_render_pipeline
        lib/jxl/render_pipeline/stage_blending
        lib/jxl/render_pipeline/stage_chroma_upsampling
        lib/jxl/render_pipeline/stage_cms
        lib/jxl/render_pipeline/stage_epf
        lib/jxl/render_pipeline/stage_from_linear
        lib/jxl/render_pipeline/stage_gaborish
        lib/jxl/render_pipeline/stage_noise
        lib/jxl/render_pipeline/stage_patches
        lib/jxl/render_pipeline/stage_splines
        lib/jxl/render_pipeline/stage_spot
        lib/jxl/render_pipeline/stage_to_linear
        lib/jxl/render_pipeline/stage_tone_mapping
        lib/jxl/render_pipeline/stage_upsampling
        lib/jxl/render_pipeline/stage_write
        lib/jxl/render_pipeline/stage_xyb
        lib/jxl/render_pipeline/stage_ycbcr
        lib/jxl/simd_util
        lib/jxl/splines
        lib/jxl/toc
        lib/threads/resizable_parallel_runner)

    set(_libjxl_abs_sources "")
    foreach (_f IN LISTS _libjxl_sources)
        list(APPEND _libjxl_abs_sources "${libjxl_SOURCE_DIR}/${_f}.cc")
    endforeach ()
    unset(_f)

    add_library(libjxl STATIC ${_libjxl_abs_sources})
    # StaticLibrary_libjxl.mk's own gb_StaticLibrary_set_include: PRIVATE
    # top-level (internal #include "lib/jxl/..."-style paths used across
    # libjxl's own sources) + PUBLIC lib/include (the public <jxl/...>
    # headers, including the 4 copied-in ones above) -- PUBLIC because
    # RepositoryExternal.mk's own consumer-side gb_LinkTarget__use_libjxl
    # sets this SAME -I onto vcl directly (vcl/source/filter/jxl/reader.cxx
    # includes public jxl/*.h headers itself).
    target_include_directories(libjxl PRIVATE "${libjxl_SOURCE_DIR}")
    target_include_directories(libjxl PUBLIC "${libjxl_SOURCE_DIR}/lib/include")
    # gb_StaticLibrary_set_warnings_disabled,libjxl
    target_compile_options(libjxl PRIVATE -w)
    set_target_properties(libjxl PROPERTIES POSITION_INDEPENDENT_CODE ON)

    # StaticLibrary_libjxl.mk's own gb_StaticLibrary_add_defs has FOUR
    # entries; RepositoryExternal.mk's own consumer-side gb_LinkTarget__
    # use_libjxl re-applies only THREE of them onto vcl directly (omitting
    # JPEGXL_ENABLE_TRANSCODE_JPEG=OFF, which only libjxl's own compilation
    # needs) -- PRIVATE vs PUBLIC below reproduces that exact split.
    target_compile_definitions(libjxl PRIVATE JPEGXL_ENABLE_TRANSCODE_JPEG=OFF)
    target_compile_definitions(libjxl PUBLIC
        JXL_INTERNAL_LIBRARY_BUILD
        JXL_THREADS_INTERNAL_LIBRARY_BUILD
        jxl_cms_EXPORTS)

    # PRIVATE, not PUBLIC: brotli's/highway's own INTERFACE_INCLUDE_
    # DIRECTORIES are needed by libjxl's OWN sources (which #include
    # <brotli/...>/<hwy/...> directly) but vcl/source/filter/jxl/reader.cxx
    # never does -- RepositoryExternal.mk's own gb_LinkTarget_use_externals,
    # (1),highway brotli lcms2) on the consumer side is upstream's blanket
    # "chain every use_external through" convention, reproduced here via
    # CMake's own transitive static-library LINK propagation (a consumer of
    # libjxl still gets brotli/highway pulled onto its final link line
    # through this PRIVATE target_link_libraries, since a static archive's
    # own link dependencies always propagate transitively regardless of
    # PRIVATE/PUBLIC -- only COMPILE usage requirements like include dirs
    # stay scoped by PRIVATE) without redundantly exposing their headers to
    # a consumer that never includes them. lcms2 IS relevant to consumers
    # too (vcl already links ${LIBRESTUDIO_LCMS2_TARGET} directly, from the
    # earlier lcms2/libeot/libwebp pass), so no separate PUBLIC re-export is
    # needed for it either.
    target_link_libraries(libjxl PRIVATE brotli highway ${_libjxl_lcms2_target})

    librestudio_vendor_set_target(LIBJXL libjxl)
endfunction()
