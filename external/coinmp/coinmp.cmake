# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for CoinMP, invoked by librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake, called from sc/CMakeLists.txt -- the only real
# consumer) only when no system coinmp+coinutils pair was found.
#
# A genuinely different shape from every other vendored external in this
# port: not a list of files to compile, but a real `./configure && make`
# over the whole unpacked tree at once (COIN-OR's own cross-project build:
# BuildTools/CoinUtils/Osi/Clp/Cgl/Cbc/CoinMP), transcribed from external/
# coinmp/ExternalProject_coinmp.mk's non-MSVC branch (read in full).
#
# _librestudio_driver_dir: see external/zlib/zlib.cmake's own header
# comment for why this is captured at file top level, outside the function.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_coinmp)
    message(STATUS "coinmp: no system package found -- building CoinMP 1.8.4 from source")

    # download.lst's own COINMP_TARBALL/COINMP_SHA256SUM, verified directly
    # and cross-checked against the real downloaded file's own sha256sum.
    librestudio_fetch_tarball(coinmp CoinMP-1.8.4.tgz
        3459fb0ccbdd39342744684338984ac4cc153fb0434f4cae8cf74bd67490a38d)

    # UnpackedTarball_coinmp.mk's own patch list (17 entries;
    # windows.build.patch.1 is real but `if (COM=MSC)`-gated in the .mk
    # itself and skipped here -- not a Windows build).
    # gb_UnpackedTarball_set_patchlevel,coinmp,0 is the module default;
    # every patch below with a numeric suffix overrides it per-patch, the
    # rest fall through to 0.
    librestudio_apply_patches(coinmp "${coinmp_SOURCE_DIR}" "${_librestudio_driver_dir}" 0
        no-binaries.patch.1
        werror-undef.patch.0
        coinmp-msvc-disable-sse2.patch.1
        ubsan.patch.0
        rpath.patch
        libtool.patch
        Wnon-c-typedef-for-linkage.patch
        register.patch
        configure-exit.patch
        pedantic-errors.patch
        bind2nd.patch.1
        clang-with-path.patch
        odr.patch
        const.patch.1
        const2.patch.1
        const3.patch.1)

    # gb_UnpackedTarball_update_autoconf_configs (config.guess/config.sub
    # refresh for exotic/newer host triples) is NOT reproduced -- the
    # tarball already ships pre-generated `configure` scripts for the root
    # and all 6 buildable sub-projects, so no autoreconf is needed either.
    # Skipping the config.guess/config.sub refresh is a real, narrow
    # infidelity -- honestly noted, not hidden -- but harmless on this
    # specific, thoroughly ordinary x86_64-linux-gnu host triple.
    set(_coinmp_build "${coinmp_SOURCE_DIR}")

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    # Options are upstream's Linux branch, transcribed literally
    # (ExternalProject_coinmp.mk's non-MSC else-branch): skip the sample
    # data project, silent rules, no bzlib/glpk, dependency-linking (so
    # each sub-project's .so picks up its own dependencies via DT_NEEDED
    # rather than requiring every consumer to list them all), no Fortran,
    # and an rpath of $ORIGIN so the built .so's find each other without an
    # installed prefix. Plain "make" (not CMake's own $(MAKE) recursive-make
    # token, which only means anything under the Unix Makefiles generator)
    # -- this port's own generator is ninja, and CoinMP's build is a
    # genuinely separate GNU Make invocation either way, same as
    # external/harfbuzz's own separate meson invocation.
    ExternalProject_Add(coinmp_ext
        SOURCE_DIR "${_coinmp_build}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND
            "${_coinmp_build}/configure" "COIN_SKIP_PROJECTS=Data/Sample"
                --enable-silent-rules --disable-bzlib --without-glpk
                --enable-dependency-linking F77=unavailable
                "LDFLAGS=-Wl,-z,origin -Wl,-rpath,$ORIGIN"
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${_coinmp_build}/Cbc/src/.libs/libCbc.so.3.9.10"
            "${_coinmp_build}/Cbc/src/.libs/libCbcSolver.so.3.9.10"
            "${_coinmp_build}/Cgl/src/.libs/libCgl.so.1.9.11"
            "${_coinmp_build}/Clp/src/.libs/libClp.so.1.13.12"
            "${_coinmp_build}/Clp/src/.libs/libClpSolver.so.1.13.12"
            "${_coinmp_build}/Clp/src/OsiClp/.libs/libOsiClp.so.1.13.12"
            "${_coinmp_build}/CoinMP/src/.libs/libCoinMP.so.1.8.4"
            "${_coinmp_build}/CoinUtils/src/.libs/libCoinUtils.so.3.10.15"
            "${_coinmp_build}/Osi/src/Osi/.libs/libOsi.so.1.12.10"
        COMMENT "Building CoinMP (autotools: configure && make)")

    # RepositoryExternal.mk's own bundled gb_LinkTarget__use_coinmp
    # (2626-2639): 8 directly-linked libraries (ClpSolver is packaged at
    # runtime but not itself a direct link dependency -- transcribed
    # exactly, not simplified) -- IMPORTED targets, same pattern as
    # external/harfbuzz's own fallback build. Two parallel lists, not a
    # list of "name;path" pairs -- CMake lists are themselves
    # semicolon-joined strings, so a "name;path" list element does not
    # nest: list(APPEND ... "a;b") just appends two elements, not one pair,
    # which silently misaligned an earlier draft of this loop
    # (add_dependencies() failed on a path used as a target name).
    set(_coinmp_lib_names Cbc CbcSolver Cgl Clp OsiClp CoinMP CoinUtils Osi)
    set(_coinmp_lib_paths
        "${_coinmp_build}/Cbc/src/.libs/libCbc.so.3.9.10"
        "${_coinmp_build}/Cbc/src/.libs/libCbcSolver.so.3.9.10"
        "${_coinmp_build}/Cgl/src/.libs/libCgl.so.1.9.11"
        "${_coinmp_build}/Clp/src/.libs/libClp.so.1.13.12"
        "${_coinmp_build}/Clp/src/OsiClp/.libs/libOsiClp.so.1.13.12"
        "${_coinmp_build}/CoinMP/src/.libs/libCoinMP.so.1.8.4"
        "${_coinmp_build}/CoinUtils/src/.libs/libCoinUtils.so.3.10.15"
        "${_coinmp_build}/Osi/src/Osi/.libs/libOsi.so.1.12.10")
    list(LENGTH _coinmp_lib_names _coinmp_lib_count)
    math(EXPR _coinmp_lib_last "${_coinmp_lib_count} - 1")
    set(_coinmp_targets)
    foreach (_i RANGE 0 ${_coinmp_lib_last})
        list(GET _coinmp_lib_names ${_i} _name)
        list(GET _coinmp_lib_paths ${_i} _loc)
        add_library(${_name} SHARED IMPORTED GLOBAL)
        set_target_properties(${_name} PROPERTIES
            IMPORTED_LOCATION "${_loc}"
            INTERFACE_INCLUDE_DIRECTORIES
                "${_coinmp_build}/CoinMP/src;${_coinmp_build}/CoinUtils/src")
        add_dependencies(${_name} coinmp_ext)
        list(APPEND _coinmp_targets ${_name})
        unset(_name)
        unset(_loc)
    endforeach ()
    unset(_i)
    unset(_coinmp_lib_names)
    unset(_coinmp_lib_paths)
    unset(_coinmp_lib_count)
    unset(_coinmp_lib_last)

    librestudio_vendor_set_target(COINMP "${_coinmp_targets}")
    unset(_coinmp_targets)
endfunction()
