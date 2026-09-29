# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for harfbuzz, invoked by librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake, called from vcl/CMakeLists.txt -- the only real
# consumer, which also owns the GRAPHITE2-before-HARFBUZZ ordering
# requirement) only when no sufficiently new system harfbuzz was found (this
# port needs 14.3+, hb_ot_fetch_bits()/HB_OT_BITS_TAG_* are used unguarded
# by vcl/source/font/*).
function(librestudio_vendor_harfbuzz)
    message(STATUS "harfbuzz: no system package new enough -- building harfbuzz 14.5.0 from source")

    find_program(MESON_EXECUTABLE meson REQUIRED)

    librestudio_fetch_tarball(harfbuzz harfbuzz-14.5.0.tar.xz
        b7132e148358a45185c9feafd049dbaf243649d3c44414b3534d9c95d18592b9)

    set(HARFBUZZ_SRC "${harfbuzz_SOURCE_DIR}/src")
    set(HARFBUZZ_BUILD "${CMAKE_BINARY_DIR}/harfbuzz-build")

    include(ExternalProject)
    # The options are upstream's: static, subset enabled, ICU built in, and
    # graphite2 from the system; tests and utilities are left out, and
    # auto_features=disabled keeps meson from switching on anything else it
    # happens to find. -fPIC because these objects end up inside a shared
    # library; PYTHONWARNINGS= because a warning from the interpreter prints
    # as a meson error.
    ExternalProject_Add(harfbuzz_ext
        SOURCE_DIR "${harfbuzz_SOURCE_DIR}"
        BINARY_DIR "${HARFBUZZ_BUILD}"
        CONFIGURE_COMMAND
            # PKG_CONFIG_PATH: -Dgraphite2=enabled below makes meson's own
            # dependency('graphite2', ...) probe pkg-config for it.
            # LIBRESTUDIO_GRAPHITE2_PKGCONFIG_DIR (cmake/LibreStudioTarballs.cmake)
            # holds a real graphite2.pc only when graphite2 was vendored --
            # vcl/CMakeLists.txt's own librestudio_pkgconfig(NAME GRAPHITE2 ...)
            # call, right before its own HARFBUZZ resolution, guarantees
            # graphite2 has already been resolved (and its .pc, if any,
            # already written) by the time this CONFIGURE_COMMAND actually
            # runs. Prepended,
            # not replacing $ENV{PKG_CONFIG_PATH}, so meson still finds
            # everything else (ICU, ...) via the normal system path; on the
            # system-graphite2 branch the directory is simply empty and
            # pkg-config skips it, falling through to the real system
            # graphite2.pc on the inherited path.
            "${CMAKE_COMMAND}" -E env "PYTHONWARNINGS="
                "PKG_CONFIG_PATH=${LIBRESTUDIO_GRAPHITE2_PKGCONFIG_DIR}:$ENV{PKG_CONFIG_PATH}"
            "${MESON_EXECUTABLE}" setup --wrap-mode nofallback
                --buildtype release
                -Ddefault_library=static
                -Dauto_features=disabled
                -Dcpp_std=c++20
                -Dtests=disabled
                -Dutilities=disabled
                -Dsubset=enabled
                -Draster=disabled
                -Dvector=disabled
                -Dicu=enabled
                -Dicu_builtin=true
                -Dgraphite2=enabled
                "-Dc_args=-fPIC"
                "-Dcpp_args=-fPIC"
                "${harfbuzz_SOURCE_DIR}"
        BUILD_COMMAND "${MESON_EXECUTABLE}" compile -C "${HARFBUZZ_BUILD}" libs
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${HARFBUZZ_BUILD}/src/libharfbuzz.a"
            "${HARFBUZZ_BUILD}/src/libharfbuzz-subset.a"
        COMMENT "Building harfbuzz")

    set(_harfbuzz_targets harfbuzz harfbuzz_subset)
    foreach (_hb harfbuzz harfbuzz-subset)
        string(REPLACE "-" "_" _target "${_hb}")
        add_library(${_target} STATIC IMPORTED GLOBAL)
        set_target_properties(${_target} PROPERTIES
            IMPORTED_LOCATION "${HARFBUZZ_BUILD}/src/lib${_hb}.a"
            # The headers are in the unpacked tarball, so they exist as soon
            # as the fetch above has run; only the archives wait for meson.
            INTERFACE_INCLUDE_DIRECTORIES "${HARFBUZZ_SRC}")
        # add_dependencies on an imported target is allowed and does reach
        # the consumers, which is what orders meson before the link.
        add_dependencies(${_target} harfbuzz_ext)
        unset(_target)
    endforeach ()
    unset(_hb)

    librestudio_vendor_set_target(HARFBUZZ "${_harfbuzz_targets}")
    unset(_harfbuzz_targets)
endfunction()
