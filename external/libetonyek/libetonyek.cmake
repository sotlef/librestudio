# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libetonyek, part of the writerperfect-family
# externals -- see external/librevenge/librevenge.cmake's own header
# comment for the shared shape. Invoked by librestudio_pkgconfig() only
# when no sufficiently new (>= 0.1.10) system libetonyek-0.1 was found.
#
# glm (header-only, no gb_Module build target of its own -- like dragonbox/
# frozen/mdds, an UnpackedTarball-only external) is fetched directly here,
# the only real consumer in this tree (grepped: no other CMakeLists.txt
# references glm at all) -- mdds itself is NOT fetched here: this port's
# own root CMakeLists.txt already resolves MDDS_INCLUDE_DIR once (a system
# probe with a tarball fallback, CMakeLists.txt:91-97), reused directly.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_etonyek)
    message(STATUS "libetonyek: no system package found -- building libetonyek 0.1.13 from source")

    librestudio_pkgconfig(NAME REVENGE MODULES librevenge-0.0 VERSION 0.0.1 DRIVER librevenge)
    librestudio_find_package(NAME ZLIB TARGET_VAR _etonyek_zlib_target)

    # download.lst's own GLM_TARBALL/GLM_SHA256SUM -- header-only, no build,
    # just an -I onto the unpacked tree's own top level (glm/glm.hpp etc).
    librestudio_fetch_tarball(glm glm-1.0.3.zip
        3aa4347b8f13cba882df1c7b61a6ca910c75a875c56ec3d75d7dc9ae8eac34df)

    # download.lst's own ETONYEK_TARBALL/ETONYEK_SHA256SUM (ETONYEK_VERSION_
    # MICRO=13).
    librestudio_fetch_tarball(libetonyek libetonyek-0.1.13.tar.xz
        032b71cb597edd92a0b270b916188281bc35be55296b263f6817b29adbcb1709)

    # UnpackedTarball_libetonyek.mk's own 5 patches, default patchlevel 0
    # (win_build.patch.1/mdds3.0.patch.1 carry their own ".1" suffix).
    # rpath.patch here is applied UNCONDITIONALLY (not gated on
    # $(SYSTEM_REVENGE) -- re-checked directly, same as libwpg's own).
    set(_libetonyek_patches
        win_build.patch.1
        ubsan.patch
        rpath.patch
        enumarith.patch
        mdds3.0.patch.1)
    librestudio_apply_patches(libetonyek "${libetonyek_SOURCE_DIR}" "${_librestudio_driver_dir}" 0 ${_libetonyek_patches})
    unset(_libetonyek_patches)

    # ExternalProject_libetonyek.mk's own recipe: --with-pic, shared-only,
    # --without-docs, --disable-debug (ENABLE_DEBUG unset), --disable-tests,
    # --disable-werror, --disable-weffc, --without-tools, --with-mdds=1.0
    # (a version-floor string the configure script checks against, not a
    # path -- MDDS_INCLUDE_DIR supplies the actual location via CPPFLAGS
    # below). LANGTAG_CFLAGS/LANGTAG_LIBS and XML_CFLAGS/XML_LIBS come from
    # this port's own root-level liblangtag/libxml2 resolutions
    # (CMakeLists.txt:822 pkg_check_modules(LIBLANGTAG...), :825
    # find_package(LibXml2...)). LDFLAGS rpath unconditional on Linux in
    # the real recipe (not SYSTEM_REVENGE-gated, same as libwpg) -- real
    # path substituted for upstream's $ORIGIN only when librevenge was
    # actually vendored (a no-op otherwise).
    set(_etonyek_ldflags "")
    set(_etonyek_extra_depends "")
    if (NOT LIBRESTUDIO_REVENGE_FOUND)
        set(_etonyek_ldflags "LDFLAGS=-L${LIBRESTUDIO_REVENGE_LIBDIR} -Wl,-rpath,${LIBRESTUDIO_REVENGE_LIBDIR}")
        if (TARGET librevenge_ext)
            set(_etonyek_extra_depends librevenge_ext)
        endif ()
    endif ()

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    ExternalProject_Add(libetonyek_ext
        SOURCE_DIR "${libetonyek_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libetonyek_SOURCE_DIR}/configure"
            --with-pic --enable-shared --disable-static --without-docs
            --disable-debug --disable-tests --disable-werror --disable-weffc
            --without-tools --with-mdds=1.0
            "CPPFLAGS=-I${MDDS_INCLUDE_DIR} -I${glm_SOURCE_DIR}"
            "LANGTAG_CFLAGS=${LIBLANGTAG_CFLAGS}"
            "LANGTAG_LIBS=${LIBLANGTAG_LIBS}"
            "XML_CFLAGS=-I${LIBXML2_INCLUDE_DIRS}"
            "XML_LIBS=${LIBXML2_LIBRARIES}"
            ${_etonyek_ldflags}
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        DEPENDS ${_etonyek_extra_depends}
        BUILD_BYPRODUCTS
            "${libetonyek_SOURCE_DIR}/src/lib/.libs/libetonyek-0.1.so"
        COMMENT "Building libetonyek (autotools: configure && make)")
    unset(_etonyek_ldflags)
    unset(_etonyek_extra_depends)

    # RepositoryExternal.mk's own !SYSTEM_ETONYEK, non-MSC gb_LinkTarget__
    # use_etonyek: -DLIBETONYEK_VISIBILITY on the consumer side.
    add_library(etonyek SHARED IMPORTED GLOBAL)
    set_target_properties(etonyek PROPERTIES
        IMPORTED_LOCATION "${libetonyek_SOURCE_DIR}/src/lib/.libs/libetonyek-0.1.so"
        INTERFACE_INCLUDE_DIRECTORIES "${libetonyek_SOURCE_DIR}/inc"
        INTERFACE_COMPILE_DEFINITIONS "LIBETONYEK_VISIBILITY")
    target_link_libraries(etonyek INTERFACE ${LIBRESTUDIO_REVENGE_TARGET} ${_etonyek_zlib_target})
    add_dependencies(etonyek libetonyek_ext)

    librestudio_vendor_set_target(ETONYEK etonyek)
endfunction()
