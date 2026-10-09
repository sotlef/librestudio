# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Skia (Library_skia.mk), vendored from download.lst's tarball; vcl's Windows
# back end draws through it.  Only built when ENABLE_SKIA is on, which is
# the default on Windows.
#
# Skia only supports being compiled by clang, so unlike everything else in the
# tree it cannot go through the project's cl.exe: it is configured and built
# as a separate CMake project (external/skia/CMakeLists.skia.in) by clang-cl,
# the way harfbuzz is built by meson, and comes back as skialo.dll plus its
# import library.  The source list is skia_sources.cmake, the evaluation of
# Library_skia.mk's conditionals for Windows with the Vulkan back end.

set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_skia)
    find_program(LIBRESTUDIO_CLANG_CL clang-cl
        HINTS "$ENV{VCINSTALLDIR}/Tools/Llvm/x64/bin"
        REQUIRED)

    librestudio_fetch_tarball(skia skia-m149-9bb16a741cdbcbde46a0d4caf8ac73f4d07a89e2.tar.xz
        08458913d3076a7c1c0a1550f472b3aa5df811b2f08c7d24e65fb4d2ca3f14f4)

    # UnpackedTarball_skia.mk: patches in this order (a numeric suffix is the
    # patch level, the rest are -p1).
    librestudio_apply_patches(skia "${skia_SOURCE_DIR}" "${_librestudio_driver_dir}" 1
        fix-pch.patch.1
        make-api-visible.patch.1
        no-trace-resources-on-exit.patch.1
        fix-alpha-difference-copy.patch.1
        share-grcontext.patch.1
        clang-attributes-warning.patch.1
        fontconfig-get-typeface.patch.1
        windows-do-not-modify-logfont.patch.0
        windows-force-unicode-api.patch.0
        fix-without-gl.patch.1
        windows-typeface-directwrite.patch.1
        windows-raster-surface-no-copies.patch.1
        swap-buffers-rect.patch.1
        ubsan.patch.1
        fix-warnings.patch.1
        windows-libraries-system32.patch.1
        allow-no-es2restrictions.patch.1
        vk_mem_alloc.patch.1
        macosmetal.patch.1
        redefinition-of-op.patch.0
        0001-Added-missing-include-cstdio.patch
        fix-SkDebugf-link-error.patch.1
        ubsan-missing-typeinfo.patch.1
        help-msvc-analyzer.patch
        windows-define-conflict.patch.1
        0002-loong64-Honor-existing-LASX-LSX-settings.patch
        0003-loong64-Fix-missing-rounding-in-loong64-scaled_mult-.patch
        0004-loong64-Fix-the-remaining-implicit-vector-casts.patch
        msvc-unknown-attributes.patch.1
        fix-semaphore-include.patch.1
        fix-xputimage-depth.patch.1)

    # The post_action: skcms ships .cc files that the .mk compiles as .cpp.
    foreach (_cc IN ITEMS
            modules/skcms/skcms
            modules/skcms/src/skcms_TransformBaseline
            modules/skcms/src/skcms_TransformHsw
            modules/skcms/src/skcms_TransformSkx)
        if (EXISTS "${skia_SOURCE_DIR}/${_cc}.cc")
            file(RENAME "${skia_SOURCE_DIR}/${_cc}.cc" "${skia_SOURCE_DIR}/${_cc}.cpp")
        endif ()
    endforeach ()
    unset(_cc)

    # Library_skia.mk use_externals(zlib libjpeg libpng): their include
    # directories, whichever way they were resolved, plus libpng/zlib to link.
    set(SKIA_EXTERNAL_INCLUDES "")
    set(SKIA_EXTERNAL_LIBS "")
    foreach (_ext IN ITEMS ZLIB JPEG PNG)
        set(_t "${LIBRESTUDIO_${_ext}_TARGET}")
        if (TARGET ${_t})
            get_target_property(_inc ${_t} INTERFACE_INCLUDE_DIRECTORIES)
            if (_inc)
                list(APPEND SKIA_EXTERNAL_INCLUDES ${_inc})
            endif ()
            get_target_property(_type ${_t} TYPE)
            get_target_property(_loc ${_t} IMPORTED_IMPLIB_RELEASE)
            if (NOT _loc)
                get_target_property(_loc ${_t} IMPORTED_IMPLIB)
            endif ()
            if (NOT _loc)
                get_target_property(_loc ${_t} IMPORTED_LOCATION_RELEASE)
            endif ()
            if (_loc)
                list(APPEND SKIA_EXTERNAL_LIBS "${_loc}")
            endif ()
        endif ()
    endforeach ()
    unset(_ext)
    list(REMOVE_DUPLICATES SKIA_EXTERNAL_INCLUDES)

    set(_skia_build "${CMAKE_BINARY_DIR}/skia-build")
    file(MAKE_DIRECTORY "${_skia_build}")
    configure_file("${_librestudio_driver_dir}/CMakeLists.skia.in"
        "${_skia_build}/src/CMakeLists.txt" @ONLY)

    include(ExternalProject)
    ExternalProject_Add(skia_ext
        SOURCE_DIR "${_skia_build}/src"
        BINARY_DIR "${_skia_build}/bin"
        CMAKE_GENERATOR "Ninja"
        CMAKE_ARGS
            -DCMAKE_C_COMPILER=${LIBRESTUDIO_CLANG_CL}
            -DCMAKE_CXX_COMPILER=${LIBRESTUDIO_CLANG_CL}
            -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${_skia_build}/bin/skialo.dll"
            "${_skia_build}/bin/skialo.lib"
        # Skia's glue code (external/skia/source) links sal.
        DEPENDS sal)

    # Programs run from instdir/program: put the DLL there once built.
    add_custom_command(TARGET skia_ext POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_skia_build}/bin/skialo.dll" "${LIBRESTUDIO_PROGRAM_DIR}/skialo.dll")

    add_library(skia SHARED IMPORTED GLOBAL)
    set_target_properties(skia PROPERTIES
        IMPORTED_LOCATION "${_skia_build}/bin/skialo.dll"
        IMPORTED_IMPLIB "${_skia_build}/bin/skialo.lib"
        # gb_LinkTarget__use_skia (RepositoryExternal.mk)
        INTERFACE_INCLUDE_DIRECTORIES
            "${skia_SOURCE_DIR}/include/core;${skia_SOURCE_DIR}/include/effects;${skia_SOURCE_DIR}/include/gpu;${skia_SOURCE_DIR}/include/config;${skia_SOURCE_DIR}/include/ports;${skia_SOURCE_DIR}/include/private;${skia_SOURCE_DIR}/include/third_party/vulkan;${skia_SOURCE_DIR}/tools/gpu;${skia_SOURCE_DIR};${CMAKE_SOURCE_DIR}/external/skia/inc"
        INTERFACE_COMPILE_DEFINITIONS "SK_USER_CONFIG_HEADER=<config_skia.h>;SKIA_DLL")
    add_dependencies(skia skia_ext)

    librestudio_vendor_set_target(SKIA skia)
endfunction()
