# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for orcus, invoked by librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake, called from sc/CMakeLists.txt -- the only real
# consumer) only when no sufficiently new system orcus (+ spreadsheet-model)
# pair was found (or LIBRESTUDIO_FORCE_VENDOR_ORCUS asks for it
# unconditionally). See the rest of this dependency's own rationale (why
# autotools ExternalProject_Add, what was manually verified end-to-end
# before writing this) in sc/CMakeLists.txt's own comment.
function(librestudio_vendor_orcus)
    message(STATUS "orcus: no system package >= 0.21.0 found (or vendoring forced) -- building orcus 0.21.0 from source")

    # Own dependency resolved lazily, right here, instead of relying on any
    # guaranteed external order -- same demonstration as external/libpng/
    # libpng.cmake's own recursive librestudio_find_package(NAME ZLIB ...)
    # call. DRIVER given explicitly (short name "libixion"): MODULES
    # libixion-0.20 has a hyphen and a dot, so it does not qualify as an
    # auto-detection candidate at all.
    librestudio_pkgconfig(
        NAME IXION
        MODULES libixion-0.20
        VERSION "0.20.0"
        FORCE_VENDOR_VAR LIBRESTUDIO_FORCE_VENDOR_IXION
        DRIVER libixion
        TARGET_VAR _orcus_ixion_target
        FOUND_VAR _orcus_ixion_found)

    # Normalized MDDS_CFLAGS/LIBIXION_CFLAGS/LIBIXION_LIBS-style values for
    # orcus's own ./configure below, covering both of libixion's branches
    # uniformly. When system ixion was used, the fix is a second, cheap,
    # deterministic QUIET re-probe of the exact same module right below
    # (pkg-config has no side effects) rather than threading every
    # individual Cflags/Libs value through more CACHE INTERNAL plumbing for
    # values only this one file needs. When ixion was vendored, it has no
    # .pc file at all yet anyway (autoconf's ./configure runs long before
    # this port's own build step would ever produce one), so its include
    # dir/output dir (LIBRESTUDIO_IXION_VENDORED_INCLUDE_DIR/_LIB_DIR,
    # external/libixion/libixion.cmake's own export) are passed directly
    # instead.
    if (NOT _orcus_ixion_found)
        set(_ixion_cflags_for_orcus "-I${LIBRESTUDIO_IXION_VENDORED_INCLUDE_DIR}")
        set(_ixion_libs_for_orcus "-L${LIBRESTUDIO_IXION_VENDORED_LIB_DIR} -lixion-0.20")
        set(_ixion_runtime_dir_for_orcus "${LIBRESTUDIO_IXION_VENDORED_LIB_DIR}")
    else ()
        pkg_check_modules(IXION_FLAGS QUIET "libixion-0.20>=0.20.0")
        string(REPLACE ";" " " _ixion_cflags_for_orcus "${IXION_FLAGS_CFLAGS}")
        string(REPLACE ";" " " _ixion_libs_for_orcus "${IXION_FLAGS_LDFLAGS}")
        list(GET IXION_FLAGS_LIBRARY_DIRS 0 _ixion_runtime_dir_for_orcus)
    endif ()

    librestudio_fetch_tarball(orcus liborcus-0.21.0.tar.xz
        1c6e473d6b8a63d61d0e6874a1762d6f0e0e78338d763e48ac93126ecde19b37)

    include(ProcessorCount)
    ProcessorCount(_orcus_nproc)
    if (_orcus_nproc EQUAL 0)
        set(_orcus_nproc 1)
    endif ()

    include(ExternalProject)
    ExternalProject_Add(orcus_ext
        SOURCE_DIR "${orcus_SOURCE_DIR}"
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND
            "${CMAKE_COMMAND}" -E env
                "MDDS_CFLAGS=-I${MDDS_INCLUDE_DIR}"
                "MDDS_LIBS= "
                "LIBIXION_CFLAGS=${_ixion_cflags_for_orcus}"
                "LIBIXION_LIBS=${_ixion_libs_for_orcus}"
            "${orcus_SOURCE_DIR}/configure"
                "--prefix=${orcus_SOURCE_DIR}/librestudio-install"
                --enable-shared --disable-static
                --disable-debug
                --without-tools --without-benchmark --without-doc-example
                --disable-python
                --disable-werror
                --without-parquet-filter
        BUILD_COMMAND "${CMAKE_COMMAND}" -E env "LD_LIBRARY_PATH=${_ixion_runtime_dir_for_orcus}" make -j${_orcus_nproc}
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${orcus_SOURCE_DIR}/src/liborcus/.libs/liborcus-0.21.so"
            "${orcus_SOURCE_DIR}/src/parser/.libs/liborcus-parser-0.21.so"
            "${orcus_SOURCE_DIR}/src/mso/.libs/liborcus-mso-0.21.so"
            "${orcus_SOURCE_DIR}/src/spreadsheet/.libs/liborcus-spreadsheet-model-0.21.so"
        COMMENT "Building orcus (autotools)")
    add_dependencies(orcus_ext ${_orcus_ixion_target})

    set(ORCUS_INCLUDE_DIR "${orcus_SOURCE_DIR}/include")

    # Autotools/libtool leaves the real .so files under each component's own
    # src/*/.libs/ -- unlike ixion (a real CMake project, so
    # CMAKE_RUNTIME_OUTPUT_DIRECTORY already redirected it into
    # LIBRESTUDIO_PROGRAM_DIR automatically), nothing here does that
    # relocation for us. install(FILES ...) alone would not be enough: it
    # only runs at `cmake --install` time, not `cmake --build` -- every
    # consumer here (sc/scfilt, and anything loading these at runtime via
    # the shared LIBRESTUDIO_PROGRAM_DIR/rpath convention every other
    # library in this port already uses) needs the real copy to exist as
    # soon as `cmake --build` finishes, so a real build-time
    # add_custom_command + ALL target does the copy instead, and the
    # IMPORTED targets below point at the copied location, not the original
    # .libs/ one.
    set(_orcus_built_libs
        "${orcus_SOURCE_DIR}/src/liborcus/.libs/liborcus-0.21.so"
        "${orcus_SOURCE_DIR}/src/parser/.libs/liborcus-parser-0.21.so"
        "${orcus_SOURCE_DIR}/src/mso/.libs/liborcus-mso-0.21.so"
        "${orcus_SOURCE_DIR}/src/spreadsheet/.libs/liborcus-spreadsheet-model-0.21.so")
    # 2026-09-29, found via a real ctest run (sc_basic_types_test):
    # "${_orcus_lib}" here is libtool's own unversioned linker-name symlink
    # (.libs/lib*.so -> lib*.so.0.0.0) -- `cmake -E copy` follows it and
    # writes a plain regular file named lib*.so, with no lib*.so.0 anywhere
    # in LIBRESTUDIO_PROGRAM_DIR. But every one of these 4 .so's own embedded
    # SONAME is lib*.so.0 (confirmed directly via `readelf -d`), and it is
    # THAT name (not the unversioned one) that every consumer's own compiled-
    # in DT_NEEDED entry records and the dynamic loader searches for at
    # runtime -- "liborcus-parser-0.21.so.0: cannot open shared object file"
    # was real, reproduced live, not a hypothetical. Fixed by also creating
    # the *.so.0 symlink libtool/a real `make install` would have created,
    # right alongside the unversioned copy every consumer here still links
    # against by its exact IMPORTED_LOCATION path.
    set(_orcus_program_libs "")
    foreach (_orcus_lib IN LISTS _orcus_built_libs)
        get_filename_component(_orcus_lib_name "${_orcus_lib}" NAME)
        set(_orcus_dest "${LIBRESTUDIO_PROGRAM_DIR}/${_orcus_lib_name}")
        set(_orcus_soname_dest "${_orcus_dest}.0")
        add_custom_command(
            OUTPUT "${_orcus_dest}" "${_orcus_soname_dest}"
            COMMAND ${CMAKE_COMMAND} -E copy "${_orcus_lib}" "${_orcus_dest}"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                    "${_orcus_lib_name}" "${_orcus_soname_dest}"
            DEPENDS orcus_ext
            COMMENT "Copying ${_orcus_lib_name} (+ .so.0 SONAME symlink) into ${LIBRESTUDIO_PROGRAM_DIR}"
            VERBATIM)
        list(APPEND _orcus_program_libs "${_orcus_dest}" "${_orcus_soname_dest}")
    endforeach ()
    add_custom_target(orcus_copy ALL DEPENDS ${_orcus_program_libs})

    add_library(orcus_parser SHARED IMPORTED GLOBAL)
    set_target_properties(orcus_parser PROPERTIES
        IMPORTED_LOCATION "${LIBRESTUDIO_PROGRAM_DIR}/liborcus-parser-0.21.so"
        INTERFACE_INCLUDE_DIRECTORIES "${ORCUS_INCLUDE_DIR}")
    add_dependencies(orcus_parser orcus_copy)

    add_library(orcus SHARED IMPORTED GLOBAL)
    set_target_properties(orcus PROPERTIES
        IMPORTED_LOCATION "${LIBRESTUDIO_PROGRAM_DIR}/liborcus-0.21.so"
        INTERFACE_INCLUDE_DIRECTORIES "${ORCUS_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES orcus_parser)
    add_dependencies(orcus orcus_copy)

    add_library(orcus_mso SHARED IMPORTED GLOBAL)
    set_target_properties(orcus_mso PROPERTIES
        IMPORTED_LOCATION "${LIBRESTUDIO_PROGRAM_DIR}/liborcus-mso-0.21.so"
        INTERFACE_INCLUDE_DIRECTORIES "${ORCUS_INCLUDE_DIR}")
    add_dependencies(orcus_mso orcus_copy)

    # readelf -d on the built .so confirms its real DT_NEEDED set: orcus,
    # orcus_parser and (this port's own vendored or system) ixion -- wired
    # as INTERFACE_LINK_LIBRARIES here so consuming targets (sc/scfilt) get
    # the whole chain and correct link/RPATH ordering automatically, not
    # just the one library they name directly.
    add_library(orcus_spreadsheet_model SHARED IMPORTED GLOBAL)
    set_target_properties(orcus_spreadsheet_model PROPERTIES
        IMPORTED_LOCATION "${LIBRESTUDIO_PROGRAM_DIR}/liborcus-spreadsheet-model-0.21.so"
        INTERFACE_INCLUDE_DIRECTORIES "${ORCUS_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "orcus;orcus_parser;${_orcus_ixion_target}")
    add_dependencies(orcus_spreadsheet_model orcus_copy)

    install(FILES ${_orcus_program_libs} DESTINATION program)

    librestudio_vendor_set_target(ORCUS orcus)
    set(LIBRESTUDIO_ORCUS_SPREADSHEET_TARGET orcus_spreadsheet_model CACHE INTERNAL "" FORCE)
endfunction()
