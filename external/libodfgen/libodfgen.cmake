# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for libodfgen, part of the writerperfect-family externals
# -- see external/librevenge/librevenge.cmake's own header comment for the
# shared shape. Invoked by librestudio_pkgconfig() only when no system
# libodfgen-0.1 was found.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_odfgen)
    message(STATUS "libodfgen: no system package found -- building libodfgen 0.1.8 from source")

    librestudio_pkgconfig(NAME REVENGE MODULES librevenge-0.0 VERSION 0.0.1 DRIVER librevenge)

    # download.lst's own ODFGEN_TARBALL/ODFGEN_SHA256SUM (ODFGEN_VERSION_
    # MICRO=8).
    librestudio_fetch_tarball(libodfgen libodfgen-0.1.8.tar.xz
        55200027fd46623b9bdddd38d275e7452d1b0ff8aeddcad6f9ae6dc25f610625)

    # UnpackedTarball_libodfgen.mk's own patch list, default patchlevel 0.
    # rpath.patch conditional on librevenge having been vendored;
    # libodfgen-bundled-soname.patch.0's own non-MACOSX/non-WNT guard
    # always taken here.
    set(_libodfgen_patches
        ellipticalarc.patch
        0001-const-up-libodfgen.patch.1
        0001-Missing-include.patch.1
        libodfgen-bundled-soname.patch.0)
    if (NOT LIBRESTUDIO_REVENGE_FOUND)
        list(APPEND _libodfgen_patches rpath.patch)
    endif ()
    librestudio_apply_patches(libodfgen "${libodfgen_SOURCE_DIR}" "${_librestudio_driver_dir}" 0 ${_libodfgen_patches})
    unset(_libodfgen_patches)

    # ExternalProject_libodfgen.mk's own recipe: --with-pic, shared-only,
    # --disable-debug (ENABLE_DEBUG unset), --disable-werror/--disable-
    # weffc, --without-docs. XML_CFLAGS/XML_LIBS from this port's own
    # root-level find_package(LibXml2 REQUIRED) (CMakeLists.txt:825).
    # -DLIBREVENGE_VISIBILITY only needed in ITS OWN CPPFLAGS (a raw
    # ./configure environment string, not a CMake target property) when
    # librevenge was vendored -- the imported "revenge" target's own
    # INTERFACE_COMPILE_DEFINITIONS only reaches CMake-native consumers,
    # not this separate autotools invocation.
    set(_libodfgen_cppflags "")
    set(_libodfgen_ldflags "")
    set(_libodfgen_extra_depends "")
    if (NOT LIBRESTUDIO_REVENGE_FOUND)
        set(_libodfgen_cppflags "-DLIBREVENGE_VISIBILITY")
        set(_libodfgen_ldflags "LDFLAGS=-L${LIBRESTUDIO_REVENGE_LIBDIR} -Wl,-rpath,${LIBRESTUDIO_REVENGE_LIBDIR}")
        if (TARGET librevenge_ext)
            set(_libodfgen_extra_depends librevenge_ext)
        endif ()
    endif ()

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    ExternalProject_Add(libodfgen_ext
        SOURCE_DIR "${libodfgen_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${libodfgen_SOURCE_DIR}/configure"
            --with-pic --enable-shared --disable-static --disable-debug
            --disable-werror --disable-weffc --without-docs
            "CPPFLAGS=${_libodfgen_cppflags}"
            "XML_CFLAGS=-I${LIBXML2_INCLUDE_DIRS}"
            "XML_LIBS=${LIBXML2_LIBRARIES}"
            ${_libodfgen_ldflags}
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        DEPENDS ${_libodfgen_extra_depends}
        BUILD_BYPRODUCTS
            "${libodfgen_SOURCE_DIR}/src/.libs/libodfgen-0.1.so"
        COMMENT "Building libodfgen (autotools: configure && make)")
    unset(_libodfgen_cppflags)
    unset(_libodfgen_ldflags)
    unset(_libodfgen_extra_depends)

    # RepositoryExternal.mk's own !SYSTEM_ODFGEN, non-MSC gb_LinkTarget__
    # use_odfgen: note src/.libs, NOT src/lib/.libs (libodfgen's own
    # top-level Makefile.am builds directly at src/, unlike librevenge/
    # libwpd's own src/lib/ subdirectory layout).
    add_library(odfgen SHARED IMPORTED GLOBAL)
    set_target_properties(odfgen PROPERTIES
        IMPORTED_LOCATION "${libodfgen_SOURCE_DIR}/src/.libs/libodfgen-0.1.so"
        INTERFACE_INCLUDE_DIRECTORIES "${libodfgen_SOURCE_DIR}/inc")
    target_link_libraries(odfgen INTERFACE ${LIBRESTUDIO_REVENGE_TARGET})
    add_dependencies(odfgen libodfgen_ext)

    librestudio_vendor_set_target(ODFGEN odfgen)
endfunction()
