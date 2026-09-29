# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Fetching of third-party header-only dependencies.
#
# dragonbox, fast_float and frozen are not vendored in the source tree: upstream
# ships them as tarballs listed in download.lst and unpacks them into workdir/.
# We do the same, minus the gbuild machinery — file(DOWNLOAD) checks the recorded
# SHA256, file(ARCHIVE_EXTRACT) unpacks once at configure time.
#
# Tarballs are cached between build trees in LIBRESTUDIO_TARBALL_CACHE so that
# reconfiguring or building a second tree does not hit the network again.

set(LIBRESTUDIO_TARBALL_URL "https://dev-www.libreoffice.org/src"
    CACHE STRING "Base URL for third-party source tarballs")

set(LIBRESTUDIO_TARBALL_CACHE "${CMAKE_BINARY_DIR}/_tarballs"
    CACHE PATH "Directory where downloaded tarballs are cached")

# librestudio_fetch_tarball(<name> <tarball> <sha256>)
#
# Downloads <tarball> from LIBRESTUDIO_TARBALL_URL unless a copy with a matching
# SHA256 is already cached, unpacks it into <build>/_deps/<name>, and sets
# <name>_SOURCE_DIR in the caller's scope to the tarball's top-level directory
# (or to the extraction directory itself when the tarball has no single
# top-level directory).
function(librestudio_fetch_tarball name tarball sha256)
    set(_cache_file "${LIBRESTUDIO_TARBALL_CACHE}/${tarball}")
    set(_dest "${CMAKE_BINARY_DIR}/_deps/${name}")
    set(_stamp "${_dest}/.unpacked-${sha256}")

    file(MAKE_DIRECTORY "${LIBRESTUDIO_TARBALL_CACHE}")

    if (NOT EXISTS "${_cache_file}")
        message(STATUS "Downloading ${tarball}")
        file(DOWNLOAD "${LIBRESTUDIO_TARBALL_URL}/${tarball}" "${_cache_file}.tmp"
             EXPECTED_HASH SHA256=${sha256}
             TLS_VERIFY ON
             STATUS _dl_status)
        list(GET _dl_status 0 _dl_code)
        if (NOT _dl_code EQUAL 0)
            list(GET _dl_status 1 _dl_msg)
            file(REMOVE "${_cache_file}.tmp")
            message(FATAL_ERROR
                "Failed to download ${LIBRESTUDIO_TARBALL_URL}/${tarball}: ${_dl_msg}")
        endif ()
        file(RENAME "${_cache_file}.tmp" "${_cache_file}")
    endif ()

    if (NOT EXISTS "${_stamp}")
        file(REMOVE_RECURSE "${_dest}")
        file(MAKE_DIRECTORY "${_dest}")
        file(ARCHIVE_EXTRACT INPUT "${_cache_file}" DESTINATION "${_dest}")
        file(TOUCH "${_stamp}")
    endif ()

    # A well-formed source tarball contains exactly one top-level directory.
    # Fall back to the extraction root if that is not the case.
    file(GLOB _entries RELATIVE "${_dest}" "${_dest}/*")
    list(FILTER _entries EXCLUDE REGEX "^\\.unpacked-")
    list(LENGTH _entries _count)
    if (_count EQUAL 1 AND IS_DIRECTORY "${_dest}/${_entries}")
        set(_source_dir "${_dest}/${_entries}")
    else ()
        set(_source_dir "${_dest}")
    endif ()

    set(${name}_SOURCE_DIR "${_source_dir}" PARENT_SCOPE)
endfunction()

# raptor2/rasqal/redland (external/raptor2, external/rasqal, external/
# redland) each depend on the previous, and rasqal's own configure.ac
# bypasses the usual "skip pkg-config if <PREFIX>_CFLAGS/_LIBS are pre-set"
# convention with a hard-coded `$PKG_CONFIG raptor2 --modversion` recheck
# (configure.ac:428) -- so every one of the three, whichever branch (system
# or vendored) resolves it, writes a synthetic, uninstalled .pc file into
# this one shared scratch directory, always put first on PKG_CONFIG_PATH
# for every subsequent configure step in the chain. Works uniformly for
# both branches (a system package's synthetic .pc just mirrors its own real
# pkg-config Cflags/Libs) and sidesteps needing to special-case rasqal's
# own bypass at all.
#
# Defined here (this file is include()'d once from the root CMakeLists.txt,
# early), not inside any of external/raptor2, external/rasqal or external/
# redland's own CMakeLists.txt, because those three are siblings under
# external/'s own add_subdirectory() chain: a function defined inside one
# add_subdirectory()'d directory is only visible to directories nested
# further inside it, never to a sibling.
set(LIBRESTUDIO_RDF_PKGCONFIG_DIR "${CMAKE_BINARY_DIR}/_deps/rdf-pkgconfig"
    CACHE INTERNAL "Shared uninstalled-.pc scratch directory for the raptor2/rasqal/redland vendoring chain")

# Same reasoning as LIBRESTUDIO_RDF_PKGCONFIG_DIR above, for graphite2:
# external/graphite/graphite.cmake (found-or-vendored) and external/
# harfbuzz/harfbuzz.cmake (its own meson CONFIGURE_COMMAND) both need this
# path, from independent call sites with no guaranteed order between them
# -- declared here, in the one file loaded once, unconditionally, before
# any real consumer or driver runs.
set(LIBRESTUDIO_GRAPHITE2_PKGCONFIG_DIR "${CMAKE_BINARY_DIR}/_deps/graphite-pkgconfig"
    CACHE INTERNAL "Scratch directory for a synthetic graphite2.pc, read by external/harfbuzz's own meson setup via PKG_CONFIG_PATH")

# Force-vendor overrides for the externals that support one -- declared
# here, not inside whichever consumer happens to resolve them first, for
# the same reason: no consumer is a privileged "first" one anymore, and an
# option() needs a single, always-reached declaration site to be visible in
# ccmake/cmake-gui regardless of which real consumer actually triggers
# resolution.
option(LIBRESTUDIO_FORCE_VENDOR_IXION
    "Always build libixion from its own upstream source, even if a sufficiently new system package exists"
    OFF)
option(LIBRESTUDIO_FORCE_VENDOR_ORCUS
    "Always build orcus from download.lst's own pinned tarball, even if a sufficiently new system package exists"
    OFF)
option(LIBRESTUDIO_FORCE_VENDOR_RAPTOR2
    "Always build raptor2 from its own upstream source, even if a sufficiently new system package exists"
    OFF)
option(LIBRESTUDIO_FORCE_VENDOR_RASQAL
    "Always build rasqal from its own upstream source, even if a sufficiently new system package exists"
    OFF)
option(LIBRESTUDIO_FORCE_VENDOR_REDLAND
    "Always build redland/librdf from its own upstream source, even if a sufficiently new system package exists"
    OFF)
option(LIBRESTUDIO_FORCE_VENDOR_LIBJXL
    "Always build libjxl (and its own brotli/highway prerequisites) from source, even if a sufficiently new system libjxl+libjxl_threads pair exists"
    OFF)
option(LIBRESTUDIO_FORCE_VENDOR_CLUCENE
    "Always build clucene from its own upstream source, even if a sufficiently new system libclucene-core exists"
    OFF)
option(LIBRESTUDIO_FORCE_VENDOR_LIBJPEG_TURBO
    "Always build libjpeg-turbo from its own upstream source, even if a sufficiently new system libjpeg exists"
    OFF)

# librestudio_write_uninstalled_pc(<name> <dir> <libdir> <includedir>
#     <version> <libs_private> <requires>)
#
# <dir>: which scratch directory to write <name>.pc into -- callers outside
# the raptor2/rasqal/redland chain (e.g. external/graphite/CMakeLists.txt,
# for external/harfbuzz's own meson dependency('graphite2', ...) lookup)
# pass their own, so this one function serves every vendored external that
# needs a downstream *.pc consumer, not only the RDF chain it was
# originally written for.
function(librestudio_write_uninstalled_pc name dir libdir includedir version libs_private requires)
    file(MAKE_DIRECTORY "${dir}")
    file(WRITE "${dir}/${name}.pc"
"libdir=${libdir}
includedir=${includedir}

Name: ${name}
Description: LibreStudio uninstalled ${name}.pc (vendored or system, resolved at configure time)
Version: ${version}
Libs: -L\${libdir} -l${name}
Libs.private: ${libs_private}
Cflags: -I\${includedir}
Requires: ${requires}
")
endfunction()

# librestudio_apply_patches(<name> <source_dir> <patch_dir> <default_level> <patch1> [<patch2> ...])
#
# Extracted, one-for-one, from the identical loop that used to be
# duplicated across external/zlib, external/graphite, external/poppler and
# external/coinmp's own CMakeLists.txt (the only four of the twelve
# system-or-vendor externals that actually apply patches through the real
# `patch` utility -- external/libixion and external/raptor2 each do their
# own lightweight file(READ)/string(REPLACE)/file(WRITE) integration patch
# instead, kept inline in their own drivers; external/rasqal/redland/md4c
# need no patches at all).
#
# A patch filename's own numeric suffix (e.g. "foo.patch.1") overrides
# <default_level> for that one patch; everything else falls through to
# <default_level>. Patches are read from <patch_dir> -- an EXPLICIT
# parameter, not (as an earlier draft of this function had it)
# CMAKE_CURRENT_SOURCE_DIR: since drivers are now include()'d from
# whichever real consumer's own directory-scope first requests the
# dependency (comphelper, vcl, sc, ...), CMAKE_CURRENT_SOURCE_DIR at the
# time this function actually runs is THAT CONSUMER's own directory, not
# the driver file's own external/<name>/ -- confirmed directly via a
# throwaway `cmake -P` script before writing this (CMAKE_CURRENT_SOURCE_DIR
# is a directory-scope property, changed only by add_subdirectory(), never
# by include()). Each of the 4 real callers (external/zlib/zlib.cmake,
# external/graphite/graphite.cmake, external/poppler/poppler.cmake,
# external/coinmp/coinmp.cmake) captures its own directory once, at its own
# top level (before its function(librestudio_vendor_<name>) definition) via
# `set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")` -- CMAKE_
# CURRENT_LIST_DIR correctly reflects the included file's own directory
# WHILE include() is running it, and that value stays correctly visible
# inside the driver's own function body afterwards (verified: CMake
# resolves unqualified variable reads/writes through the dynamic call-stack
# of the CURRENT invocation, not the lexical scope of the file where a
# function happens to be textually defined; a nested driver-to-driver call
# that re-captures the same variable name in ITS OWN function invocation
# does not clobber the caller's already-captured value, since each function
# call gets its own local binding for a plain set() -- confirmed with a
# 3-level nested `cmake -P` reproduction of this exact include()+
# cmake_language(CALL...) shape before relying on it here), and passes that
# as <patch_dir>.
#
# A ".patched" stamp file in <source_dir> prevents re-applying on every
# reconfigure; on failure, <source_dir>'s parent "_deps/<name>" tree is
# wiped so a re-run starts from a clean re-fetch instead of a half-patched
# tree.
function(librestudio_apply_patches name source_dir patch_dir default_level)
    set(_patches ${ARGN})
    find_package(Patch REQUIRED)
    set(_stamp "${source_dir}/.patched")
    if (NOT EXISTS "${_stamp}")
        foreach (_patch IN LISTS _patches)
            get_filename_component(_patch_suffix "${_patch}" LAST_EXT)
            string(REPLACE "." "" _patch_suffix "${_patch_suffix}")
            if (_patch_suffix MATCHES "^[0-9]+$")
                set(_patch_level "${_patch_suffix}")
            else ()
                set(_patch_level "${default_level}")
            endif ()
            execute_process(
                COMMAND "${Patch_EXECUTABLE}" -f -s "-p${_patch_level}" --fuzz=0 -i
                        "${patch_dir}/${_patch}"
                WORKING_DIRECTORY "${source_dir}"
                RESULT_VARIABLE _patch_result)
            unset(_patch_suffix)
            unset(_patch_level)
            if (NOT _patch_result EQUAL 0)
                file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/_deps/${name}")
                message(FATAL_ERROR
                    "Failed to apply ${_patch} to ${name}; source tree reset, re-run cmake to retry")
            endif ()
        endforeach ()
        unset(_patch)
        file(TOUCH "${_stamp}")
    endif ()
endfunction()

# --- librestudio_pkgconfig() / librestudio_find_package() -----------------
#
# Shared "system-or-vendor" dependency resolution, called directly from
# whichever real consumer module (vcl, sc, comphelper, ...) first needs a
# given external dependency -- there is no dedicated external/<name>/
# CMakeLists.txt "entry point" file anymore (an earlier version of this
# design had one per dependency; removed once it became clear the lazy,
# memoized resolution below already makes the FIRST real caller, whichever
# one that happens to be, a perfectly good entry point on its own, and a
# do-nothing stub file added no value). external/<name>/ directories still
# exist (patches, driver .cmake files, configs/), just without their own
# CMakeLists.txt or add_subdirectory() registration.
#
# Probes the system via pkg-config or CMake's own find_package() (one
# function per mechanism -- deliberately NOT one function with a
# mechanism-selecting parameter, since that would let a caller accidentally
# pass both PKGCONFIG- and FIND_PACKAGE-style parameters at once with no
# structural error), and when nothing suitable is found (or FORCE_VENDOR_VAR
# asks for it unconditionally), include() a driver file and invoke its
# librestudio_vendor_<name-lower>() function, which is responsible for
# fetching, patching, building, and setting LIBRESTUDIO_<NAME>_TARGET (and
# any sibling CACHE INTERNAL variables it needs -- some drivers produce
# more than one target, e.g. CoinMP's 8 IMPORTED targets or harfbuzz's
# harfbuzz+harfbuzz_subset pair) itself.
#
# libjpeg-turbo (2026-09-29): plugged in, the last of the original 12
# "researched but not yet plugged in" externals. Real gbuild StaticLibrary
# direct-compile (58 portable C files, matching NASM/SIMD asm on x86 when a
# real nasm/nasmw/yasm is found -- neither is on this machine, so only the
# portable-C arm is build-verified here) -- see external/libjpeg-turbo/
# libjpeg-turbo.cmake's own header comment for the full story, including
# why this is librestudio_find_package() (PACKAGE JPEG), not
# librestudio_pkgconfig() (configure.ac probes it via plain header+lib
# checks, not pkg-config), and the 3 real consumers converted from a bare
# find_package(JPEG REQUIRED) (vcl, sdext, external/poppler/poppler.cmake's
# own internal need).
#
# No externals from the ORIGINAL 12-item "researched but not yet plugged
# in" list remained as of 2026-09-29 (see immediately above) -- but a
# separate, much larger family was found and plugged in the same day, see
# the writerperfect-family note further down this comment block (it was
# never part of that original 12; writerperfect's own bare `pkg_check_
# modules(... REQUIRED ...)` calls for these 18 libraries were simply not
# looked at until a later pass that same day).
#
# writerperfect-family (2026-09-29, later the same day): librevenge/
# libwpd/libodfgen/libwpg/libwps/libmwaw/libetonyek/libstaroffice (shared-
# lib half, real MSC-vs-non-MSC split like hunspell) and libabw/libebook/
# libepubgen/libcdr/libfreehand/libmspub/libpagemaker/libqxp/libvisio/
# libzmf (static-only half, no MSC branch at all) -- 18 libraries total,
# all real autotools ExternalProject builds, all converted from
# writerperfect/CMakeLists.txt's own bare `pkg_check_modules(...
# REQUIRED ...)` calls (which never enforced configure.ac's own version
# floors at all) to librestudio_pkgconfig(). librevenge is the shared base
# dependency of the other 17. VERSION floors now match configure.ac's own
# libo_CHECK_SYSTEM_MODULE/libo_PKG_VERSION calls exactly -- on this
# machine, libwps's system package (0.4.12) falls one micro version short
# of the 0.4.14 floor upstream itself requires, so libwps genuinely vendors
# here (not force-vendored), a real, non-forced validation of the pattern.
# See external/librevenge/librevenge.cmake's own header comment for the
# full family-wide rationale (shared-vs-static split, boost/icu default-
# path reasoning, real-absolute-path-vs-$ORIGIN rpath substitution for
# cross-library linking within the vendored branch).
#
# clucene (2026-09-29): plugged in -- CMakeLists.txt's own root-level
# LIBCLUCENE_CORE resolution (xmlhelp + helpcompiler are the real
# consumers) now goes through external/clucene/clucene.cmake. Turned out to
# be a real gbuild StaticLibrary direct-compile (178 files, one combined
# core+shared+contribs-lib archive), not an autotools build -- the "21 real
# patches" was accurate (22 on Linux, all mechanical -p0/-p1 application,
# no autoreconf needed). Along the way, corrected a real bug in
# helpcompiler/CMakeLists.txt: it hardcoded -DSYSTEM_CLUCENE unconditionally
# (written before clucene had any vendoring fallback), which would have
# been wrong on the vendored branch -- HelpIndexer.cxx's own `#ifndef
# SYSTEM_CLUCENE` guards a call to a method that only exists in the
# patched/vendored fork, not in a real system package. Now conditioned on
# LIBRESTUDIO_CLUCENE_FOUND, matching RepositoryExternal.mk's own
# gb_LinkTarget__use_clucene exactly.
#
# libjxl/brotli/highway (2026-09-29): plugged in -- vcl/CMakeLists.txt (via
# libjxl) is the only real consumer. All three turned out to be genuine
# gbuild StaticLibrary direct-compiles (like libpng/graphite/md4c), not
# autotools ExternalProject builds -- the earlier "highway has no package
# on this machine" note was about a SYSTEM highway package, which was never
# relevant: neither brotli nor highway has a system-package option
# upstream at all (no SYSTEM_BROTLI/SYSTEM_HIGHWAY anywhere in configure.ac
# or RepositoryExternal.mk), so external/brotli/brotli.cmake and external/
# highway/highway.cmake are called DIRECTLY by external/libjxl/libjxl.cmake
# (its only consumer) rather than through librestudio_pkgconfig() -- see
# each driver's own header comment. Along the way, corrected vcl/
# CMakeLists.txt's own invented split of libjxl into two separate
# pkg_check_modules() probes (JXL + JXL_THREADS) back into the single
# combined NAME LIBJXL / MODULES "libjxl libjxl_threads" call configure.ac
# and Library_vcl.mk actually use upstream.
#
# lcms2/libeot/libwebp (2026-09-29): plugged in -- vcl/CMakeLists.txt (all
# three) and writerperfect/CMakeLists.txt (lcms2 only) are the real
# consumers, via external/lcms2/lcms2.cmake, external/libeot/libeot.cmake
# and external/libwebp/libwebp.cmake. lcms2 is a real shared-library
# ExternalProject (matching configure.ac's own LCMS2_LIBS_internal, a
# -llcms2 link flag, not a static archive); libeot and libwebp are static
# (libwebp's driver sets a 2-element LIBRESTUDIO_LIBWEBP_TARGET: webp +
# sharpyuv, same shape as CoinMP's own multi-target list). None of the
# three's own runtime-install steps (ExternalPackage_lcms2.mk's .so
# packaging) are reproduced -- pure runtime packaging, out of scope here.
#
# hunspell/mythes/libexttextcat/libnumbertext (2026-09-29): plugged in --
# lingucomponent/CMakeLists.txt is the real (and only) consumer of all
# four, via external/hunspell/hunspell.cmake, external/mythes/mythes.cmake,
# external/libexttextcat/libexttextcat.cmake and external/libnumbertext/
# libnumbertext.cmake. All four are genuine `./configure && make` autotools
# ExternalProject builds (same shape as coinmp/poppler), not gbuild-style
# direct-compiles -- Module_<name>.mk registers ExternalProject_<name> on
# every non-MSC platform, StaticLibrary_<name>.mk is MSC-only and not
# reproduced. The langclass/*.lm (libexttextcat) and data/*.sor
# (libnumbertext) runtime data packages are NOT reproduced -- pure data-
# packaging, out of scope for this compile-time vendoring pass, same
# boundary already drawn around poppler_data (which WAS small enough to
# reproduce, 4 whole directories, unlike these two's ~200 individually-
# named files) -- see each driver's own header comment.
#
# Both functions memoize through LIBRESTUDIO_<NAME>_TARGET/LIBRESTUDIO_
# <NAME>_FOUND (CACHE INTERNAL, so every consumer across the tree can read
# them as plain variables) GUARDED by a GLOBAL PROPERTY, not by the CACHE
# variable's own presence: a CACHE variable persists in CMakeCache.txt
# across separate `cmake` invocations, but the actual target OBJECTS
# (find_package()'s ZLIB::ZLIB, pkg_check_modules()'s PkgConfig::FOO, or a
# vendor driver's own add_library()) do not -- they are recreated fresh by
# every single configure run regardless of what CMakeCache.txt remembers.
# Treating "the CACHE variable already has a value" as "already resolved"
# would, on a plain reconfigure of an existing build tree, skip calling
# find_package()/pkg_check_modules() again in the new process and leave
# every consumer linking to a target that was never actually (re)created --
# a real bug caught by exactly that reconfigure scenario while writing this.
# A GLOBAL PROPERTY does not persist across `cmake` invocations either (like
# a plain variable), so it correctly says "resolved" only within the
# current configure run: a second call for the same NAME later in the SAME
# run (whether from another external/<name>/CMakeLists.txt or from a driver
# resolving its own transitive dependency, e.g. libpng's driver calling
# librestudio_find_package(NAME ZLIB ...) itself) returns instantly instead
# of re-probing or re-vendoring -- which is what makes the whole external/
# CMakeLists.txt add_subdirectory() order stop mattering for correctness
# (see that file's own header comment) -- while a brand new run always
# re-resolves at least once, as it must.

# _librestudio_dependency_cached(<name> <target_var> <found_var> <cached_var>)
#
# If LIBRESTUDIO_<NAME>_TARGET was already resolved earlier in THIS
# configure run (LIBRESTUDIO_<NAME>_RESOLVED global property set -- see
# above for why this is a global property, not the CACHE variable's own
# presence), fills <target_var>/<found_var> in the CALLING function's own
# scope (PARENT_SCOPE -- one level up from here, i.e. the calling public
# function) and sets <cached_var> to TRUE; otherwise only sets <cached_var>
# to FALSE. The calling public function decides whether to return() early
# itself -- a CMake function cannot force its caller to return.
function(_librestudio_dependency_cached name target_var found_var cached_var)
    get_property(_resolved GLOBAL PROPERTY LIBRESTUDIO_${name}_RESOLVED SET)
    if (_resolved)
        set(${target_var} "${LIBRESTUDIO_${name}_TARGET}" PARENT_SCOPE)
        set(${found_var} "${LIBRESTUDIO_${name}_FOUND}" PARENT_SCOPE)
        set(${cached_var} TRUE PARENT_SCOPE)
    else ()
        set(${cached_var} FALSE PARENT_SCOPE)
    endif ()
endfunction()

# _librestudio_resolve_or_vendor(<name> <found> <target> <force_vendor_var>
#     <driver> <target_var> <found_var>)
#
# Common tail of both public functions, called AFTER each has done its own
# mechanism-specific detection:
#  - if <found> is true AND <force_vendor_var> (an option() name, possibly
#    empty) is not set to TRUE -- stores <target> into
#    LIBRESTUDIO_<NAME>_TARGET (CACHE INTERNAL) and LIBRESTUDIO_<NAME>_FOUND
#    to TRUE;
#  - otherwise -- include(<driver>) + cmake_language(CALL
#    librestudio_vendor_<name-lower>) and requires the driver to have set
#    LIBRESTUDIO_<NAME>_TARGET itself (not via a return value -- some
#    drivers produce several targets/CACHE variables from one call), then
#    sets LIBRESTUDIO_<NAME>_FOUND to FALSE;
#  - either way, fills <target_var>/<found_var> in the calling public
#    function's own PARENT_SCOPE, which forwards them on with its own.
#
# FORCE on every CACHE INTERNAL set() below (both here and required of every
# driver's own LIBRESTUDIO_<NAME>_TARGET set() call): by the time this
# function runs, _librestudio_dependency_cached() has already established
# that NAME was not yet resolved in THIS configure run (see that function's
# own header comment on why that check is a GLOBAL PROPERTY, not the CACHE
# variable's presence) -- so any value already sitting in CMakeCache.txt for
# LIBRESTUDIO_<NAME>_TARGET is necessarily stale, left over from a previous
# run, and must be overwritten, not silently kept (plain set(... CACHE ...)
# without FORCE is a no-op when the variable already exists in the cache
# file, regardless of value -- the same class of bug this port already found
# once with a duplicate, silently-ignored ENABLE_COINMP declaration).
function(_librestudio_resolve_or_vendor name found target force_vendor_var driver target_var found_var)
    set(_do_vendor FALSE)
    if (NOT found)
        set(_do_vendor TRUE)
    elseif (force_vendor_var)
        # Two nested if()s, not a single "force_vendor_var AND ${${force_vendor_var}}"
        # expression: when force_vendor_var is empty (no FORCE_VENDOR_VAR was
        # given), ${force_vendor_var} expands to zero tokens rather than an
        # empty-string token, which malforms the surrounding if()'s argument
        # list -- reproduced directly while writing this. force_vendor_var
        # here holds an option()'s NAME (a string); ${force_vendor_var}
        # substitutes that name in, and if()'s own auto-dereference of a bare
        # identifier then reads that named option's actual value.
        if (${force_vendor_var})
            set(_do_vendor TRUE)
        endif ()
    endif ()

    if (NOT _do_vendor)
        string(TOLOWER "${name}" _name_lower_msg)
        message(STATUS "${_name_lower_msg}: using system package")
        unset(_name_lower_msg)
        set(LIBRESTUDIO_${name}_TARGET "${target}" CACHE INTERNAL "" FORCE)
        set(LIBRESTUDIO_${name}_FOUND TRUE CACHE INTERNAL "" FORCE)
    else ()
        if (NOT driver)
            message(FATAL_ERROR
                "librestudio: ${name} needs vendoring, but no DRIVER path could be determined -- "
                "auto-detection found neither external/<name>/<name>.cmake nor "
                "external/lib<name>/lib<name>.cmake for it (or NAME/MODULES did not qualify for "
                "auto-detection at all, e.g. more than one MODULES entry). Pass DRIVER <short-name> "
                "explicitly.")
        endif ()
        include("${driver}")
        string(TOLOWER "${name}" _name_lower)
        cmake_language(CALL "librestudio_vendor_${_name_lower}")
        unset(_name_lower)
        get_property(_vendor_set GLOBAL PROPERTY LIBRESTUDIO_${name}_VENDOR_TARGET_SET SET)
        if (NOT _vendor_set)
            message(FATAL_ERROR
                "librestudio: driver ${driver} did not set LIBRESTUDIO_${name}_TARGET via librestudio_vendor_set_target()")
        endif ()
        set(LIBRESTUDIO_${name}_FOUND FALSE CACHE INTERNAL "" FORCE)
    endif ()

    # 2026-09-29, found via the real remote build: RepositoryExternal.mk's
    # own gb_LinkTarget__use_zlib / gb_LinkTarget__use_zlib_multiarch define
    # -DZLIB_CONST for EVERY zlib consumer UNCONDITIONALLY, on both the
    # SYSTEM_ZLIB and internal/vendored branches alike (RepositoryExternal.mk
    # lines ~440-457) -- this port's router never reproduced that anywhere.
    # Without it, zlib.h's z_stream::next_in stays plain `Bytef*` (non-const)
    # instead of `const Bytef*`, and any real consumer assigning a `const
    # unsigned char*` into it (e.g. sdext's pnghelper.cxx:113, `aStream.
    # next_in = i_pBuf;`) is a genuine C++ error ("invalid conversion from
    # const to non-const pointer"), reproduced live on the remote test
    # machine. Applied here, once, to whichever real target
    # LIBRESTUDIO_ZLIB_TARGET resolved to (system ZLIB::ZLIB or the vendored
    # "zlib" STATIC target) -- both are ordinary CMake targets, so
    # INTERFACE_COMPILE_DEFINITIONS propagates to every consumer the same way
    # RepositoryExternal.mk's per-use macro call does, without having to
    # touch each of zlib's many consumers individually.
    if (name STREQUAL "ZLIB")
        set_property(TARGET "${LIBRESTUDIO_ZLIB_TARGET}" APPEND PROPERTY
            INTERFACE_COMPILE_DEFINITIONS ZLIB_CONST)
    endif ()

    set_property(GLOBAL PROPERTY LIBRESTUDIO_${name}_RESOLVED TRUE)
    set(${target_var} "${LIBRESTUDIO_${name}_TARGET}" PARENT_SCOPE)
    set(${found_var} "${LIBRESTUDIO_${name}_FOUND}" PARENT_SCOPE)
endfunction()

# librestudio_vendor_set_target(<name> <target>)
#
# Every librestudio_vendor_<name-lower>() driver MUST call this (instead of
# a bare set(LIBRESTUDIO_<NAME>_TARGET ... CACHE INTERNAL "")) to export its
# result -- it applies the same FORCE-with-justification as the found-branch
# above (a stale value from a previous run must not survive), and also
# raises the LIBRESTUDIO_<NAME>_VENDOR_TARGET_SET global-property flag
# _librestudio_resolve_or_vendor() checks right after invoking the driver.
# A driver producing several targets from one call (CoinMP's 8 IMPORTED
# targets, harfbuzz's harfbuzz+harfbuzz_subset pair) calls this once for its
# own primary LIBRESTUDIO_<NAME>_TARGET and sets any sibling CACHE INTERNAL
# variables directly (also with FORCE, for the same reason).
function(librestudio_vendor_set_target name target)
    set(LIBRESTUDIO_${name}_TARGET "${target}" CACHE INTERNAL "" FORCE)
    set_property(GLOBAL PROPERTY LIBRESTUDIO_${name}_VENDOR_TARGET_SET TRUE)
endfunction()

# _librestudio_compute_driver_path(<name> <driver_short> <candidate> <out_var>)
#
# Resolves DRIVER for both public functions. By this port's own convention
# every driver lives at external/<short-name>/<short-name>.cmake (a real,
# always-true convention here, verified against all 12 -- see each
# driver's own file), so DRIVER itself is a SHORT NAME, not a full path:
#  - <driver_short> given (caller passed DRIVER explicitly) -> expands
#    directly to "${CMAKE_SOURCE_DIR}/external/<driver_short>/<driver_short>.cmake",
#    no further guessing (the caller already committed to an exact name).
#  - <driver_short> empty, <candidate> given -> tries, via a REAL EXISTS
#    check against the actual filesystem (not a hand-maintained lookup
#    table -- an earlier draft of this design tried exactly that and
#    already had a wrong entry for ORCUS before it was ever implemented,
#    the same "don't guess, verify against the real source" lesson this
#    whole port is built on), "external/<candidate>/<candidate>.cmake"
#    then "external/lib<candidate>/lib<candidate>.cmake" (this port has
#    real directories both with and without a "lib" prefix -- libpng,
#    libixion -- and without -- zlib, md4c, raptor2, ...). Returns the
#    first that exists.
#  - Neither given, or neither candidate path exists -> <out_var> empty;
#    _librestudio_resolve_or_vendor() turns this into a FATAL_ERROR, but
#    only if vendoring actually ends up being needed (a found-branch never
#    looks at this at all).
function(_librestudio_compute_driver_path name driver_short candidate out_var)
    if (driver_short)
        set(${out_var} "${CMAKE_SOURCE_DIR}/external/${driver_short}/${driver_short}.cmake" PARENT_SCOPE)
        return()
    endif ()
    if (NOT candidate)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif ()
    set(_candidate_path "${CMAKE_SOURCE_DIR}/external/${candidate}/${candidate}.cmake")
    if (EXISTS "${_candidate_path}")
        set(${out_var} "${_candidate_path}" PARENT_SCOPE)
        return()
    endif ()
    set(_lib_candidate_path "${CMAKE_SOURCE_DIR}/external/lib${candidate}/lib${candidate}.cmake")
    if (EXISTS "${_lib_candidate_path}")
        set(${out_var} "${_lib_candidate_path}" PARENT_SCOPE)
        return()
    endif ()
    set(${out_var} "" PARENT_SCOPE)
endfunction()

# librestudio_pkgconfig(
#     [NAME <logical name>]                 # -> LIBRESTUDIO_<NAME>_TARGET;
#                                            # optional, see the two-way
#                                            # default rule below
#     MODULES [<module1> [<module2>...]]    # bare keyword (no values) ->
#                                            # TOLOWER(NAME) (requires NAME);
#                                            # explicit names used as-is (AND,
#                                            # e.g. poppler+poppler-cpp)
#     [VERSION <version>]
#     [FORCE_VENDOR_VAR <option() name>]
#     [DRIVER <short driver name>]           # e.g. "libpng", not a path --
#                                             # see _librestudio_compute_driver_path()'s
#                                             # own header comment for the
#                                             # auto-detection rule when omitted
#     [TARGET_VAR <var>]                     # optional -- omit when the
#                                             # caller only needs the
#                                             # resolution to happen and will
#                                             # read LIBRESTUDIO_<NAME>_TARGET
#                                             # (CACHE INTERNAL, set either
#                                             # way) directly instead; pass it
#                                             # when the caller uses the value
#                                             # immediately (e.g. a driver
#                                             # resolving its own recursive
#                                             # dependency)
#     [FOUND_VAR <var>]
# )
#
# NAME<->MODULES default rule (verified against all 12 real pkg-config
# module names this port probes -- no silent default was applied where it
# would have produced a WRONG module/name, only where the derivation is
# provably exact):
#  - NAME given, MODULES bare -> MODULES = TOLOWER(NAME).
#  - NAME omitted, MODULES is exactly one element matching [A-Za-z0-9_]+
#    (no hyphen/dot/etc) -> NAME = TOUPPER(that module). Correct for
#    md4c->MD4C, harfbuzz->HARFBUZZ, raptor2->RAPTOR2, rasqal->RASQAL,
#    redland->REDLAND, graphite2->GRAPHITE2 (a deliberate rename from this
#    port's own earlier GRAPHITE, not an exception -- see external/graphite/
#    graphite.cmake). Does not apply to md4c-html/libixion-0.20/
#    liborcus-0.21 (hyphen/dot) or poppler+poppler-cpp/coinmp+coinutils
#    (more than one module) -- NAME stays required there.
#  - Neither given, or NAME omitted with MODULES not matching the rule
#    above -> FATAL_ERROR (explicit failure, never a silent wrong guess).
#  - Both given explicitly -> both used as-is, no derivation.
function(librestudio_pkgconfig)
    set(_one_value NAME VERSION FORCE_VENDOR_VAR DRIVER TARGET_VAR FOUND_VAR)
    set(_multi_value MODULES)
    cmake_parse_arguments(_lp "" "${_one_value}" "${_multi_value}" ${ARGN})

    if (NOT DEFINED _lp_MODULES)
        message(FATAL_ERROR
            "librestudio_pkgconfig: MODULES is required (a bare MODULES with no values opts into the TOLOWER(NAME) default)")
    endif ()

    list(LENGTH _lp_MODULES _lp_modules_count)
    if (NOT _lp_NAME AND _lp_modules_count EQUAL 1)
        list(GET _lp_MODULES 0 _lp_single_module)
        if (_lp_single_module MATCHES "^[A-Za-z0-9_]+$")
            string(TOUPPER "${_lp_single_module}" _lp_NAME)
        endif ()
        unset(_lp_single_module)
    endif ()
    if (NOT _lp_NAME)
        message(FATAL_ERROR
            "librestudio_pkgconfig: NAME is required unless MODULES is exactly one element with no special characters")
    endif ()
    if (_lp_modules_count EQUAL 0)
        string(TOLOWER "${_lp_NAME}" _lp_MODULES)
    endif ()

    _librestudio_dependency_cached("${_lp_NAME}" _lp_cached_target _lp_cached_found _lp_is_cached)
    if (_lp_is_cached)
        if (_lp_TARGET_VAR)
            set(${_lp_TARGET_VAR} "${_lp_cached_target}" PARENT_SCOPE)
        endif ()
        if (_lp_FOUND_VAR)
            set(${_lp_FOUND_VAR} "${_lp_cached_found}" PARENT_SCOPE)
        endif ()
        return()
    endif ()

    set(_lp_pkg_specs "")
    foreach (_m IN LISTS _lp_MODULES)
        if (_lp_VERSION)
            list(APPEND _lp_pkg_specs "${_m}>=${_lp_VERSION}")
        else ()
            list(APPEND _lp_pkg_specs "${_m}")
        endif ()
    endforeach ()
    unset(_m)

    # QUIET (not REQUIRED): not finding a system package is the expected,
    # handled path into vendoring below, not a configure-time error.
    # IMPORTED_TARGET GLOBAL still creates PkgConfig::<NAME> when found,
    # visible outside this directory scope -- same combined probe+target
    # shape external/harfbuzz's own system-detection call already
    # established.
    pkg_check_modules(${_lp_NAME} QUIET IMPORTED_TARGET GLOBAL ${_lp_pkg_specs})

    # Driver-path candidate for auto-detection: the FINAL (post-default)
    # MODULES, not NAME -- see _librestudio_compute_driver_path()'s own
    # header comment for why NAME is deliberately not used here (it would
    # have silently produced a wrong guess for ORCUS, whose NAME happens to
    # match its real directory only by coincidence, unlike MD4C/RAPTOR2/
    # RASQAL/REDLAND/GRAPHITE2 where NAME really was derived from this same
    # single module in the first place). Recomputed fresh here (not reusing
    # _lp_modules_count from above) because MODULES may have just received
    # its own TOLOWER(NAME) default a few lines up.
    list(LENGTH _lp_MODULES _lp_final_modules_count)
    set(_lp_driver_candidate "")
    if (_lp_final_modules_count EQUAL 1)
        list(GET _lp_MODULES 0 _lp_driver_candidate)
        if (NOT _lp_driver_candidate MATCHES "^[A-Za-z0-9_]+$")
            set(_lp_driver_candidate "")
        endif ()
    endif ()
    unset(_lp_final_modules_count)
    _librestudio_compute_driver_path("${_lp_NAME}" "${_lp_DRIVER}" "${_lp_driver_candidate}" _lp_driver_path)
    unset(_lp_driver_candidate)

    _librestudio_resolve_or_vendor("${_lp_NAME}" "${${_lp_NAME}_FOUND}" "PkgConfig::${_lp_NAME}"
        "${_lp_FORCE_VENDOR_VAR}" "${_lp_driver_path}" _lp_out_target _lp_out_found)
    unset(_lp_driver_path)

    if (_lp_TARGET_VAR)
        set(${_lp_TARGET_VAR} "${_lp_out_target}" PARENT_SCOPE)
    endif ()
    if (_lp_FOUND_VAR)
        set(${_lp_FOUND_VAR} "${_lp_out_found}" PARENT_SCOPE)
    endif ()
endfunction()

# librestudio_find_package(
#     NAME <logical name>
#     [PACKAGE <CMake package name>]   # defaults to NAME itself -- for
#                                       # zlib/libpng the real CMake Find
#                                       # module name already IS "ZLIB"/
#                                       # "PNG" literally, not a guess
#     [PACKAGE_TARGET <target>]        # defaults to <PACKAGE>::<PACKAGE>
#     [VERSION <version>]
#     [FORCE_VENDOR_VAR <option() name>]
#     [DRIVER <short driver name>]     # e.g. "libpng", not a path -- see
#                                       # _librestudio_compute_driver_path()'s
#                                       # own header comment; when omitted,
#                                       # defaults to TOLOWER(NAME) (no
#                                       # MODULES here to derive a candidate
#                                       # from instead, unlike librestudio_pkgconfig())
#     [TARGET_VAR <var>]                 # optional -- omit when the caller
#                                         # only needs the resolution to
#                                         # happen and will read
#                                         # LIBRESTUDIO_<NAME>_TARGET (CACHE
#                                         # INTERNAL, set either way) directly
#                                         # instead
#     [FOUND_VAR <var>]
# )
function(librestudio_find_package)
    set(_one_value NAME PACKAGE PACKAGE_TARGET VERSION FORCE_VENDOR_VAR DRIVER TARGET_VAR FOUND_VAR)
    cmake_parse_arguments(_lf "" "${_one_value}" "" ${ARGN})

    if (NOT _lf_NAME)
        message(FATAL_ERROR "librestudio_find_package: NAME is required")
    endif ()
    if (NOT _lf_PACKAGE)
        set(_lf_PACKAGE "${_lf_NAME}")
    endif ()
    if (NOT _lf_PACKAGE_TARGET)
        set(_lf_PACKAGE_TARGET "${_lf_PACKAGE}::${_lf_PACKAGE}")
    endif ()

    _librestudio_dependency_cached("${_lf_NAME}" _lf_cached_target _lf_cached_found _lf_is_cached)
    if (_lf_is_cached)
        if (_lf_TARGET_VAR)
            set(${_lf_TARGET_VAR} "${_lf_cached_target}" PARENT_SCOPE)
        endif ()
        if (_lf_FOUND_VAR)
            set(${_lf_FOUND_VAR} "${_lf_cached_found}" PARENT_SCOPE)
        endif ()
        return()
    endif ()

    # QUIET, not REQUIRED: same reasoning as librestudio_pkgconfig() above.
    # GLOBAL: CMake >= 3.24 (this port requires newer already), promotes the
    # imported target to global visibility -- without it a plain
    # find_package() target is only visible in this directory scope and its
    # nested subdirectories, not to a cousin consumer module.
    if (_lf_VERSION)
        find_package(${_lf_PACKAGE} ${_lf_VERSION} QUIET GLOBAL)
    else ()
        find_package(${_lf_PACKAGE} QUIET GLOBAL)
    endif ()

    string(TOLOWER "${_lf_NAME}" _lf_driver_candidate)
    _librestudio_compute_driver_path("${_lf_NAME}" "${_lf_DRIVER}" "${_lf_driver_candidate}" _lf_driver_path)
    unset(_lf_driver_candidate)

    _librestudio_resolve_or_vendor("${_lf_NAME}" "${${_lf_PACKAGE}_FOUND}" "${_lf_PACKAGE_TARGET}"
        "${_lf_FORCE_VENDOR_VAR}" "${_lf_driver_path}" _lf_out_target _lf_out_found)
    unset(_lf_driver_path)

    if (_lf_TARGET_VAR)
        set(${_lf_TARGET_VAR} "${_lf_out_target}" PARENT_SCOPE)
    endif ()
    if (_lf_FOUND_VAR)
        set(${_lf_FOUND_VAR} "${_lf_out_found}" PARENT_SCOPE)
    endif ()
endfunction()
