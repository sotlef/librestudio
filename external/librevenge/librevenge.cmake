# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for librevenge, invoked by librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake, called from writerperfect/CMakeLists.txt --
# the only real consumer, directly and transitively as every other
# writerperfect-family format library's own base dependency) only when no
# sufficiently new system librevenge-0.0 was found.
#
# The base of an 18-library family (librevenge/libwpd/libodfgen/libwpg/
# libwps/libmwaw/libetonyek/libstaroffice/libabw/libebook/libepubgen/libcdr/
# libfreehand/libmspub/libpagemaker/libqxp/libvisio/libzmf), all following
# the same shape: Module_<name>.mk registers ExternalProject_<name> (a real
# `./configure && make`, NOT a gbuild direct-compile) on every non-MSC
# platform. librevenge/libwpd/libodfgen/libwpg/libwps/libmwaw/libetonyek/
# libstaroffice additionally register an MSC-only StaticLibrary_<name>
# (not reproduced, same as every other MSC-only branch in this port) and
# build SHARED (`--enable-shared --disable-static`, since this port has no
# DISABLE_DYNLOADING equivalent -- grepped, never set anywhere in this
# tree, so upstream's own `$(if $(DISABLE_DYNLOADING), ..., --enable-shared
# --disable-static)` always resolves to the shared arm here); the other 10
# have no MSC branch at all and build STATIC unconditionally
# (--enable-static --disable-shared, no DISABLE_DYNLOADING check in their
# own recipes at all). See each driver's own header comment for which
# category it is in.
#
# boost_headers (needed by several of these -- see each driver's own
# use_externals list, transcribed from its own ExternalProject_<name>.mk)
# is NOT passed as an explicit -I flag anywhere in this family's drivers:
# Boost is a real system package here (this port's own root CMakeLists.txt
# already does `find_package(Boost 1.69 REQUIRED COMPONENTS locale)`), and
# Boost_INCLUDE_DIR resolved to plain /usr/include -- the C++ compiler's
# own default search path already covers it, so upstream's own
# $(BOOST_CPPFLAGS) would itself expand to a no-op -I/usr/include on this
# exact machine. Same reasoning applies to icu-uc (pkg-config --cflags
# icu-uc is empty here) -- only libxml-2.0/liblangtag actually need an
# explicit -I (verified directly, not assumed) and get one where relevant.
#
# _librestudio_driver_dir: see external/zlib/zlib.cmake's own header
# comment for why this is captured at file top level, outside the function.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_librevenge)
    message(STATUS "librevenge: no system package found -- building librevenge 0.0.6 from source")

    # download.lst's own REVENGE_TARBALL/REVENGE_SHA256SUM (REVENGE_VERSION_
    # MICRO=6), verified directly (grep -n revenge download.lst).
    librestudio_fetch_tarball(librevenge librevenge-0.0.6.tar.bz2
        52a65e904d255dbdd97a8b7bb28d6574e14f999eb01416aff004502406d0904d)

    # UnpackedTarball_librevenge.mk's own 2 patches, at the module's own
    # explicit default patchlevel 0 (librevenge-bundled-soname.patch.0
    # carries its own ".0" suffix anyway, same value): rpath.patch always
    # applies, librevenge-bundled-soname.patch.0's own `ifneq(OS,MACOSX)
    # ifneq(OS,WNT)` guard is always taken (this port is Linux-only).
    librestudio_apply_patches(librevenge "${librevenge_SOURCE_DIR}" "${_librestudio_driver_dir}" 0
        rpath.patch
        librevenge-bundled-soname.patch.0)

    find_program(MAKE_EXECUTABLE make REQUIRED)
    include(ExternalProject)
    # ExternalProject_librevenge.mk's own recipe, transcribed literally
    # (non-MACOSX arm): --with-pic, shared-only (see this file's own header
    # comment), --disable-debug (ENABLE_DEBUG unset in this port),
    # --disable-werror/--disable-weffc/--disable-streams/--disable-
    # generators/--without-docs.
    ExternalProject_Add(librevenge_ext
        SOURCE_DIR "${librevenge_SOURCE_DIR}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "${librevenge_SOURCE_DIR}/configure"
            --with-pic --enable-shared --disable-static --disable-debug
            --disable-werror --disable-weffc --disable-streams
            --disable-generators --without-docs
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${librevenge_SOURCE_DIR}/src/lib/.libs/librevenge-0.0.so"
            "${librevenge_SOURCE_DIR}/src/lib/.libs/librevenge-stream-0.0.so"
        COMMENT "Building librevenge (autotools: configure && make)")

    # configure.ac's own REVENGE_CFLAGS_internal/REVENGE_LIBS_internal
    # (10419-10424, read directly): include dir is the unpacked tree's own
    # top-level inc/, link against the libtool-created unversioned-suffix
    # symlink (-lrevenge-0.0 resolves to librevenge-0.0.so inside .libs/,
    # the real file libtool builds for uninstalled linking -- same
    # reasoning as external/lcms2/lcms2.cmake's own liblcms2.so handling,
    # except there the exact versioned filename was known and used
    # directly; here the unversioned linker-name symlink is used since no
    # single real .so.X.Y.Z filename was given anywhere in the read
    # sources, and it is the literal target of -lrevenge-0.0 either way).
    # -DLIBREVENGE_VISIBILITY is added by RepositoryExternal.mk's own
    # CONSUMER-side gb_LinkTarget__use_revenge (not by librevenge's own
    # StaticLibrary/ExternalProject build) -- PUBLIC here so it propagates
    # the same way.
    #
    # librevenge-stream-0.0 is a second library librevenge's own build
    # produces (RVNGInputStream helpers) -- StaticLibrary_revenge.mk's own
    # MSC-only branch links both, and several downstream format libraries
    # (e.g. libwpg) need RVNGString/RVNGInputStream symbols that live in
    # librevenge-0.0 itself, not the stream helper -- but the stream
    # helper is part of the same source tree's own build and is captured
    # here as a BUILD_BYPRODUCTS entry for completeness even though no
    # downstream driver links it separately (none of the RepositoryExternal.
    # mk blocks read for this family reference "-lrevenge-stream" at all).
    add_library(revenge SHARED IMPORTED GLOBAL)
    set_target_properties(revenge PROPERTIES
        IMPORTED_LOCATION "${librevenge_SOURCE_DIR}/src/lib/.libs/librevenge-0.0.so"
        INTERFACE_INCLUDE_DIRECTORIES "${librevenge_SOURCE_DIR}/inc"
        INTERFACE_COMPILE_DEFINITIONS "LIBREVENGE_VISIBILITY")
    add_dependencies(revenge librevenge_ext)

    # Real absolute path to librevenge's own .libs/ directory, exported so
    # every dependent driver in this family (libwpd, libodfgen, libwpg,
    # libwps, libmwaw, libetonyek, libstaroffice, and all 10 static ones)
    # can add it to ITS OWN CONFIGURE_COMMAND's LDFLAGS
    # (-Wl,-rpath,<this path>) when librevenge was vendored -- reproducing
    # upstream's own `$(if $(SYSTEM_REVENGE),,'LDFLAGS=-Wl,-rpath,$ORIGIN')`
    # convention, except with a REAL absolute path instead of $ORIGIN:
    # upstream's $ORIGIN assumes every vendored .so ends up co-installed in
    # the same final instdir/program/ directory, which is not true of this
    # port's own separate-per-external _deps/<name>/<name>-<version>/
    # layout (nothing is installed at all during this vendoring pass) --
    # each dependent library's own `./configure`-generated test/link steps
    # need to find librevenge.so at a real, absolute location instead.
    set(LIBRESTUDIO_REVENGE_LIBDIR "${librevenge_SOURCE_DIR}/src/lib/.libs" CACHE INTERNAL "" FORCE)

    librestudio_vendor_set_target(REVENGE revenge)
endfunction()
