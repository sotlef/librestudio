# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for highway, called DIRECTLY by external/libjxl/
# libjxl.cmake (its only real consumer) -- NOT invoked through
# librestudio_pkgconfig()/librestudio_find_package(), same reasoning as
# external/brotli/brotli.cmake's own header comment: configure.ac never
# probes a system highway/libhwy package at all, and RepositoryExternal.mk's
# gb_LinkTarget__use_highway is gated only by `ifneq ($(filter
# HIGHWAY,$(BUILD_TYPE)),)`, never by a SYSTEM_HIGHWAY switch -- always
# built from source when libjxl needs it, so this driver is a plain,
# unconditionally-vendoring function.
#
# The smallest driver in this port: StaticLibrary_highway.mk (read in
# full) compiles exactly 3 files -- highway's OWN runtime-dispatch target
# detection (hwy/targets.cc), a timer and an abort helper -- NOT the whole
# library's SIMD-specialized algorithm headers (those are header-only,
# consumed directly by libjxl's own .cc files via #include <hwy/...>, no
# separate compilation unit of their own in this reduced upstream-chosen
# source list). No patches (UnpackedTarball_highway.mk has no
# gb_UnpackedTarball_add_patches call at all).
function(librestudio_vendor_highway)
    if (TARGET highway)
        return()
    endif ()
    message(STATUS "highway: building highway 1.4.0 from source (no system option upstream)")

    # download.lst's own HIGHWAY_TARBALL/HIGHWAY_SHA256SUM, verified
    # directly (grep -n highway download.lst).
    librestudio_fetch_tarball(highway highway-1.4.0.tar.gz
        36f672ab48ddb3c8555e9e89e16fe400cd7d16c6eb455a1a3d0c146a63ababdc)

    add_library(highway STATIC
        "${highway_SOURCE_DIR}/hwy/timer.cc"
        "${highway_SOURCE_DIR}/hwy/targets.cc"
        "${highway_SOURCE_DIR}/hwy/abort.cc")
    # StaticLibrary_highway.mk's own gb_StaticLibrary_set_include: just the
    # unpacked tree's own top level (hwy/*.h are consumed as -I<top>/hwy/...
    # by libjxl's own sources, same convention as every other vendored
    # header-plus-few-.cc-files external here).
    target_include_directories(highway PUBLIC "${highway_SOURCE_DIR}")
    # gb_StaticLibrary_set_warnings_disabled,highway
    target_compile_options(highway PRIVATE -w)
    set_target_properties(highway PROPERTIES POSITION_INDEPENDENT_CODE ON)

    librestudio_vendor_set_target(HIGHWAY highway)
endfunction()
