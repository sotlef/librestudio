# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for graphite2, invoked by librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake, called from vcl/CMakeLists.txt -- the only
# real consumer, which also owns the GRAPHITE2-before-HARFBUZZ ordering
# requirement) only when no sufficiently new system graphite2 was found.
# LIBRESTUDIO_GRAPHITE2_PKGCONFIG_DIR itself lives in cmake/
# LibreStudioTarballs.cmake (shared, needed by this file and by
# external/harfbuzz/harfbuzz.cmake independently).
#
# _librestudio_driver_dir: see external/zlib/zlib.cmake's own header
# comment for why this is captured at file top level, outside the function.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_graphite2)
    message(STATUS "graphite2: no system package found -- building graphite2 1.3.15 from source")

    # download.lst's own GRAPHITE_TARBALL/GRAPHITE_SHA256SUM, verified
    # directly and cross-checked against the real downloaded file's own
    # sha256sum.
    librestudio_fetch_tarball(graphite graphite2-minimal-1.3.15.tgz
        10cfc69a6f859ce875b9708d170b8222eba76cdb89b6dbb4ee1e375bc7242409)

    # UnpackedTarball_graphite.mk's own patch (patchlevel 0, dry-run
    # verified against the real fetched tarball at that level before
    # writing this).
    librestudio_apply_patches(graphite "${graphite_SOURCE_DIR}" "${_librestudio_driver_dir}" 0 ubsan.patch)

    # StaticLibrary_graphite.mk's own source list, transcribed verbatim
    # (re-verified against the real 1.3.15 tarball: all files exist, 0
    # missing). direct_machine.cpp is gbuild's own GCC branch
    # ($(if $(filter GCC,$(COM)),direct,call)_machine) -- this port is
    # always GCC (gcc3_uno, established project-wide), so call_machine.cpp
    # is never the right choice and is not reproduced.
    set(_graphite_sources
        direct_machine
        gr_char_info
        gr_face
        gr_features
        gr_font
        gr_logging
        gr_segment
        gr_slot
        json
        CmapCache
        Code
        Collider
        Decompressor
        Face
        FeatureMap
        FileFace
        Font
        GlyphCache
        GlyphFace
        Intervals
        Justifier
        NameTable
        Pass
        Position
        Segment
        Silf
        Slot
        Sparse
        TtfUtil
        UtfCodec)

    set(_graphite_abs_sources "")
    foreach (_f IN LISTS _graphite_sources)
        list(APPEND _graphite_abs_sources "${graphite_SOURCE_DIR}/src/${_f}.cpp")
    endforeach ()
    unset(_f)

    add_library(graphite STATIC ${_graphite_abs_sources})
    target_include_directories(graphite PUBLIC
        "${graphite_SOURCE_DIR}/include")
    target_include_directories(graphite PRIVATE
        "${graphite_SOURCE_DIR}/src")
    target_compile_definitions(graphite PRIVATE
        GRAPHITE2_NTRACING
        GRAPHITE2_STATIC)
    # gb_StaticLibrary_set_warnings_disabled,graphite
    target_compile_options(graphite PRIVATE -w)
    # ifeq ($(COM),GCC) ifneq ($(COM_IS_CLANG),TRUE): this port's compiler
    # is always real GCC (gcc3_uno, established project-wide), never Clang.
    target_compile_options(graphite PRIVATE -fpermissive)
    # The CMake target is named "graphite" (matching the module directory),
    # but the synthetic graphite2.pc written next expects to link
    # "-lgraphite2" (its real upstream library name) -- OUTPUT_NAME makes
    # the produced archive libgraphite2.a instead of the default
    # libgraphite.a, so the two agree.
    set_target_properties(graphite PROPERTIES OUTPUT_NAME graphite2)

    # librestudio_write_uninstalled_pc() (cmake/LibreStudioTarballs.cmake,
    # the same mechanism already used for raptor2/rasqal/redland): so
    # external/harfbuzz's own meson dependency('graphite2', ...) can find
    # this vendored build via PKG_CONFIG_PATH. CMAKE_ARCHIVE_OUTPUT_
    # DIRECTORY (cmake/LibreStudioPlatform.cmake) redirects every static
    # library in this build into one shared "lib" directory -- a plain,
    # configure-time-known path, not a generator expression, so it can be
    # written into the .pc file directly.
    librestudio_write_uninstalled_pc(graphite2 "${LIBRESTUDIO_GRAPHITE2_PKGCONFIG_DIR}"
        "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}" "${graphite_SOURCE_DIR}/include"
        "1.3.15" "" "")

    librestudio_vendor_set_target(GRAPHITE2 graphite)
endfunction()
