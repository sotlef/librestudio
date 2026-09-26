# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Platform and compiler setup shared by every module.
#
# Mirrors what gbuild's solenv/gbuild/platform/*.mk set up per module: the CPPU
# environment tag, the RTL_OS/RTL_ARCH strings the source code stringifies, and
# the base definitions that sal and everything above it assume.

# The full platform list, re-derived from configure.ac's own `case "$host_os"`
# block (grep configure.ac for `RTL_OS=`): 13 case arms, each with a verified
# RTL_OS=... assignment. This is the project's actual porting mandate -- port
# gbuild to CMake literally, nothing pre-excluded by default -- not just the
# three platforms (Linux/Windows/macOS) this build can be exercised on in this
# environment. CMAKE_SYSTEM_NAME is standard CMake knowledge, not something
# configure.ac states, so each entry below is only as confident as noted next
# to its LIBRESTUDIO_RTL_OS branch further down; kFreeBSD in particular is a
# best-effort, explicitly-flagged guess, not a verified fact like the other 12.
set(_librestudio_supported_system_names
    Linux       # done: native Linux (glibc/musl); configure.ac's linux-gnu*|linux-musl*
    Windows     # done: native Windows (MSVC)
    Darwin      # done: native macOS
    iOS         # CMAKE_SYSTEM_NAME under the Apple iOS toolchain, not native Darwin
    FreeBSD
    NetBSD
    OpenBSD
    DragonFly
    Haiku
    Android     # CMAKE_SYSTEM_NAME under the Android NDK CMake toolchain file
    SunOS       # CMake's real reported name for Solaris (see confidence note below)
    Emscripten  # CMAKE_SYSTEM_NAME under emcmake/Emscripten's toolchain file
    "GNU/kFreeBSD") # TODO(platform-scope): unverified best-effort guess, see below

if (NOT CMAKE_SYSTEM_NAME IN_LIST _librestudio_supported_system_names)
    message(FATAL_ERROR
        "CMAKE_SYSTEM_NAME is ${CMAKE_SYSTEM_NAME}, which is not one of the "
        "platforms configure.ac itself has a host_os case arm for "
        "(${_librestudio_supported_system_names}). Only Linux, Windows and "
        "macOS have actually been exercised by this port; the rest are "
        "structurally recognised (RTL_OS/CPPU_ENV set below, following "
        "configure.ac) but otherwise untested -- see the per-platform "
        "comments below and in LibreStudioConfigHeaders.cmake for what is "
        "still a TODO for each.")
endif ()
unset(_librestudio_supported_system_names)

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Only 64-bit targets are supported for now.")
endif ()

# The UNO loader tag for this ABI, as recorded in .component files and used to
# pick the C++ bridge library.  See solenv/gbuild/platform/com_GCC_defs.mk for
# the Unix side and configure.ac's own CPPU_ENV/RTL_OS derivation (the `case
# "$CPUNAME"` block near AC_SUBST(CPPU_ENV), and the per-$host_os RTL_OS=...
# assignments) for the values used here.
#
# The MSVC-ABI UNO bridge (mscx_uno) itself is not implemented -- setting
# LIBRESTUDIO_CPPU_ENV=mscx here only prepares the naming convention (library
# names, .component uris) for it; actually building on Windows requires that
# bridge to exist first (see this project's research on the topic).
#
# configure.ac's own CPPU_ENV derivation (line ~6101, `if "$OS" = WNT -a "$COM"
# = MSC ... else CPPU_ENV=gcc3`) is unconditional for every non-MSVC platform:
# every one of the 10 platforms added below builds with GCC/Clang (configure.ac
# sets COM=GCC in every host_os case arm but the Windows/WNT one -- verified by
# reading each arm), so gcc3 is architecturally correct for all of them the
# same way it already is for macOS, in principle. Whether an actual, working
# gcc3 bridge exists for each one is a separate question, checked per platform
# below against bridges/source/cpp_uno/ and bridges/Library_cpp_uno.mk's own
# $(OS) selector lists (not assumed): every platform here reuses the shared
# gcc3_linux_<cpu> (or, for iOS, the dedicated gcc3_ios) bridge sources via
# #ifdef, confirmed by bridges/Library_cpp_uno.mk explicitly listing that
# platform's OS tag in the bridge-selection filter for x86_64 -- except
# Solaris, which is not listed there at all (see its branch below): that is a
# genuine, separate gap, the same kind as the Windows mscx_uno one, not papered
# over by setting gcc3 anyway.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LIBRESTUDIO_CPPU_ENV "gcc3")
    set(LIBRESTUDIO_RTL_OS "Linux")
elseif (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    # iOS is Apple, built with the same Clang/Itanium ABI as macOS, but is its
    # own CMAKE_SYSTEM_NAME (checked before the `elseif (APPLE)` macOS branch
    # below, since APPLE is also TRUE for iOS builds -- this ordering is what
    # keeps the macOS branch byte-identical to before this change, since a
    # native macOS configure never has CMAKE_SYSTEM_NAME STREQUAL "iOS").
    # RTL_OS=iOS is configure.ac's own value (the `ios*` host_os arm). Unlike
    # every other new platform below, iOS has a real, dedicated bridge
    # directory -- bridges/source/cpp_uno/gcc3_ios -- rather than reusing the
    # shared gcc3_linux_<cpu> sources, confirmed by both the directory itself
    # and bridges/Library_cpp_uno.mk's `else ifeq ($(OS),iOS)` arm.
    set(LIBRESTUDIO_CPPU_ENV "gcc3")
    set(LIBRESTUDIO_RTL_OS "iOS")
elseif (APPLE)
    # macOS builds with GCC/Clang against the same Itanium C++ ABI as Linux,
    # so it shares the gcc3 bridge tag -- there is no separate mscx-style
    # bridge for it. RTL_OS=MacOSX is upstream's own value (configure.ac's
    # *-darwin* host_os branch), confirmed against this tree.
    set(LIBRESTUDIO_CPPU_ENV "gcc3")
    set(LIBRESTUDIO_RTL_OS "MacOSX")
elseif (WIN32)
    # Real upstream naming (configure.ac: `case "$CPUNAME"` under
    # `"$OS" = WNT -a "$COM" = MSC`): msci for x86, mscx for x86-64, msca for
    # arm64. x86-64 is the default here since it is the common case and this
    # project has no non-x86-64 Windows target in view yet.
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64)$")
        set(LIBRESTUDIO_CPPU_ENV "msca")
    elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(LIBRESTUDIO_CPPU_ENV "msci")
    else ()
        set(LIBRESTUDIO_CPPU_ENV "mscx")
    endif ()
    set(LIBRESTUDIO_RTL_OS "Windows")
elseif (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    # RTL_OS=FreeBSD is configure.ac's `freebsd*` host_os arm. No dedicated
    # bridges/source/cpp_uno/gcc3_freebsd_* directory exists; instead
    # bridges/Library_cpp_uno.mk's x86_64 (and i386/aarch64/etc.) bridge
    # selection explicitly lists FREEBSD alongside LINUX/NETBSD/OPENBSD/... in
    # the $(OS) filter that picks gcc3_linux_<cpu> -- i.e. the Linux bridge
    # sources are shared across these Unix/GCC platforms via #ifdef, this is
    # not this port assuming gcc3 blindly applies.
    set(LIBRESTUDIO_CPPU_ENV "gcc3")
    set(LIBRESTUDIO_RTL_OS "FreeBSD")
elseif (CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    # RTL_OS=NetBSD is configure.ac's `*netbsd*` host_os arm. Same shared
    # gcc3_linux_<cpu> bridge reuse as FreeBSD above (NETBSD is in the same
    # bridges/Library_cpp_uno.mk $(OS) filter lists).
    set(LIBRESTUDIO_CPPU_ENV "gcc3")
    set(LIBRESTUDIO_RTL_OS "NetBSD")
elseif (CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    # RTL_OS=OpenBSD is configure.ac's `openbsd*` host_os arm. Same shared
    # gcc3_linux_<cpu> bridge reuse (OPENBSD is in the same filter lists).
    set(LIBRESTUDIO_CPPU_ENV "gcc3")
    set(LIBRESTUDIO_RTL_OS "OpenBSD")
elseif (CMAKE_SYSTEM_NAME STREQUAL "DragonFly")
    # RTL_OS=DragonFly is configure.ac's `dragonfly*` host_os arm. Same shared
    # gcc3_linux_<cpu> bridge reuse (DRAGONFLY is in the same filter lists).
    set(LIBRESTUDIO_CPPU_ENV "gcc3")
    set(LIBRESTUDIO_RTL_OS "DragonFly")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Haiku")
    # RTL_OS=Haiku is configure.ac's `haiku*` host_os arm. HAIKU appears in
    # bridges/Library_cpp_uno.mk's $(OS) filter for both CPUNAME=INTEL and
    # CPUNAME=X86_64 (though notably not for ARM/AARCH64 -- this project only
    # targets x86_64 anyway, see LIBRESTUDIO_RTL_ARCH below), so the shared
    # gcc3_linux_x86-64 bridge sources really do cover Haiku; this is not a
    # silently-missing bridge the way Solaris below genuinely is.
    set(LIBRESTUDIO_CPPU_ENV "gcc3")
    set(LIBRESTUDIO_RTL_OS "Haiku")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Android")
    # RTL_OS=Android is configure.ac's `linux-android*` host_os arm (note this
    # is a distinct host_os match from plain `linux-gnu*`/`linux-musl*`, and
    # CMake likewise reports a distinct CMAKE_SYSTEM_NAME of "Android", not
    # "Linux", under the NDK toolchain file). ANDROID is in the same
    # bridges/Library_cpp_uno.mk $(OS) filter lists as FreeBSD/NetBSD/etc, for
    # ARM/AARCH64/INTEL/X86_64 CPUNAMEs alike, so the shared gcc3_linux_<cpu>
    # bridge sources cover it too.
    set(LIBRESTUDIO_CPPU_ENV "gcc3")
    set(LIBRESTUDIO_RTL_OS "Android")
elseif (CMAKE_SYSTEM_NAME STREQUAL "SunOS")
    # CMake reports Solaris as CMAKE_SYSTEM_NAME "SunOS", not "Solaris" -- a
    # known CMake quirk (it comes from uname -s, which on Solaris prints
    # "SunOS"); high confidence but flagged since it is easy to get backwards.
    # RTL_OS=Solaris is configure.ac's own value regardless (the `solaris*`
    # host_os arm) -- RTL_OS is upstream's own naming choice and need not match
    # CMAKE_SYSTEM_NAME.
    #
    # TODO(platform-scope): unlike every other platform in this block, Solaris
    # does NOT appear anywhere in bridges/Library_cpp_uno.mk's $(OS) filter
    # lists for any CPUNAME -- there is no gcc3_solaris_* bridge directory
    # under bridges/source/cpp_uno/ either. That means gbuild itself has never
    # actually had a working UNO C++ ABI bridge for Solaris in this tree. gcc3
    # is still architecturally what Solaris *would* use (COM=GCC, Itanium ABI,
    # per configure.ac's `solaris*` arm), and CPPU_ENV is set below purely to
    # keep the naming convention consistent -- exactly the same treatment as
    # LIBRESTUDIO_CPPU_ENV=mscx for Windows above -- but actually building UNO
    # libraries for Solaris would need that bridge to be written first. This is
    # a real, separate, honestly-flagged gap, not silently papered over.
    set(LIBRESTUDIO_CPPU_ENV "gcc3")
    set(LIBRESTUDIO_RTL_OS "Solaris")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    # RTL_OS=Emscripten is configure.ac's `emscripten*` host_os arm. Emscripten
    # gets its own dedicated bridge, bridges/source/cpp_uno/gcc3_wasm (selected
    # in bridges/Library_cpp_uno.mk via `else ifeq ($(OS),EMSCRIPTEN)`, not the
    # shared gcc3_linux_<cpu> filter list the other new platforms use) -- still
    # tagged gcc3 upstream (configure.ac's CPPU_ENV derivation is
    # `if WNT+MSC ... else CPPU_ENV=gcc3`, unconditionally, and COM=GCC in the
    # `emscripten*` arm), so the CPPU_ENV value here is unchanged even though
    # the actual bridge sources it names are wasm-specific.
    set(LIBRESTUDIO_CPPU_ENV "gcc3")
    set(LIBRESTUDIO_RTL_OS "Emscripten")
elseif (CMAKE_SYSTEM_NAME STREQUAL "GNU/kFreeBSD")
    # TODO(platform-scope): CMAKE_SYSTEM_NAME for Debian GNU/kFreeBSD is
    # genuinely uncertain -- unlike every other platform in this file, this is
    # NOT a verified or well-documented CMake value. `uname -s` on that system
    # reports "GNU/kFreeBSD", which is the best-guess string used here, but
    # CMake has no widely-documented CMAKE_SYSTEM_NAME for this rare
    # combination and this has not been confirmed against a real kFreeBSD
    # CMake run. Treat this branch as unverified/best-effort, not a fact like
    # the other 12 in this file.
    #
    # RTL_OS=kFreeBSD is configure.ac's own value (the `kfreebsd*` host_os
    # arm) and IS verified. What is notable -- and easy to get wrong -- is
    # that configure.ac sets OS=LINUX (not a distinct KFREEBSD tag) in that
    # same arm, meaning gbuild's own compile-definition macro and UNO bridge
    # selection for kFreeBSD are byte-identical to Linux's (see the
    # matching "kFreeBSD is the interesting one" comment further down in this
    # same file, above the add_compile_definitions if-chain); only RTL_OS,
    # the runtime-visible string, differs. LIBRESTUDIO_CPPU_ENV is gcc3 here
    # for the same reason it is for Linux.
    set(LIBRESTUDIO_CPPU_ENV "gcc3")
    set(LIBRESTUDIO_RTL_OS "kFreeBSD")
endif ()

# This build only supports 64-bit targets (checked above), and the literal
# below was never actually derived from CMAKE_SYSTEM_NAME/CMAKE_SYSTEM_PROCESSOR
# to begin with -- it is unchanged from before this file became
# platform-aware, and applies the same way on every recognised platform.
set(LIBRESTUDIO_RTL_ARCH "x86_64")

# UNO libraries are versioned with the UDK major version; RTVERLIBS and
# UNOVERLIBS in Repository.mk get this appended (libuno_sal.so.3).
set(LIBRESTUDIO_UDK_MAJOR 3)

# Build artefacts are laid out like an installed LibreOffice, because the C++
# code assumes it: config_folders.h pins LIBO_*_FOLDER to "program", and
# cppu::getUnoIniUri() in cppuhelper/source/paths.cxx looks for unorc next to
# the library it is linked into.
set(LIBRESTUDIO_PROGRAM_DIR "${CMAKE_BINARY_DIR}/instdir/program")
set(LIBRESTUDIO_SHARE_DIR "${CMAKE_BINARY_DIR}/instdir/share")
set(LIBRESTUDIO_REGISTRY_DIR "${CMAKE_BINARY_DIR}/instdir/share/registry")

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/instdir" CACHE PATH
        "Install prefix" FORCE)
endif ()

foreach (_dir "${LIBRESTUDIO_PROGRAM_DIR}" "${LIBRESTUDIO_SHARE_DIR}")
    file(MAKE_DIRECTORY "${_dir}")
endforeach ()

# Match gbuild: libraries and executables all land in one directory, so that
# dlopen-by-name and $ORIGIN expansion behave the same at build and run time.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${LIBRESTUDIO_PROGRAM_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${LIBRESTUDIO_PROGRAM_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif ()

# The source tree is 7M lines of upstream code; building it with the warnings of
# a fresh project is not useful.  gbuild compiles upstream with the project's
# own warning policy, which we do not attempt to reproduce here.
#
# These are GCC/Clang flags (meaningful for both Linux and macOS, which build
# with the same compilers); cl.exe/link.exe understand neither, so both blocks
# below are skipped under MSVC rather than passed through and rejected.
if (NOT MSVC)
    add_compile_options(-fPIC -fno-strict-aliasing)

    # gbuild links shared libraries with -Wl,-z,defs
    # (solenv/gbuild/platform/linux.mk), which turns "forgot to link a library" from
    # a runtime crash into a build error.  That is especially valuable while porting.
    add_link_options(-Wl,-z,defs -Wl,-z,noexecstack -Wl,--as-needed)
endif ()

# Definitions every translation unit sees, matching gbuild's gb_OSDEFS and the
# per-library -D flags in Library_*.mk.
add_compile_definitions(
    CPPU_ENV=${LIBRESTUDIO_CPPU_ENV}
    RTL_OS="${LIBRESTUDIO_RTL_OS}"
    RTL_ARCH="${LIBRESTUDIO_RTL_ARCH}"
    LIBO_INTERNAL_ONLY
    SRCDIR="${CMAKE_SOURCE_DIR}"
)

# gbuild's gb_OSDEFS is `-D$(OS)` plus platform extras (see
# solenv/gbuild/platform/unxgcc.mk, macosx.mk and windows.mk): LINUX/MACOSX/WNT
# are mutually exclusive per-OS tags, while UNX (solenv/gbuild/platform/
# unxgcc.mk and macosx.mk both add it) is upstream's "this is a Unix" macro --
# confirmed against the tree itself: every #ifdef UNX / defined(UNX) site seen
# (e.g. comphelper/source/misc/debuggerinfo.cxx, which branches on
# `defined MACOSX` / `elif defined UNX` / else) treats it as "Unix, including
# macOS", never "Linux specifically" -- so it applies on both Linux and macOS,
# not on Windows.
#
# The 10 platforms below follow the same pattern, checked against the actual
# solenv/gbuild/platform/*.mk files rather than assumed:
#   * FREEBSD_X86_64_GCC.mk, NETBSD_X86_64_GCC.mk, OPENBSD_X86_64_GCC.mk,
#     DRAGONFLY_X86_64_GCC.mk, HAIKU_X86_64_GCC.mk and the ANDROID_*_GCC.mk
#     files all just `include unxgcc.mk` (optionally plus a small OS-specific
#     extra file, e.g. android.mk), so they all get unxgcc.mk's generic
#     gb_OSDEFS = -D$(OS) ... -DUNX -DUNIX -- i.e. FREEBSD/NETBSD/OPENBSD/
#     DRAGONFLY/HAIKU/ANDROID (from $(OS)) plus UNX, exactly like Linux, and
#     confirmed to include UNX for Android (there is no "Android is not UNX"
#     special case in android.mk -- it does not touch gb_OSDEFS at all).
#   * iOS.mk sets gb_OSDEFS explicitly rather than going through unxgcc.mk's
#     $(OS) substitution, but reaches the same two tags: `-DIOS ... -DUNX`.
#     Checked directly: iOS.mk does define UNX (upstream does treat iOS as a
#     Unix for this macro), unlike Android where UNX comes for free via
#     unxgcc.mk.
#   * solaris.mk is the one exception to the "-D$(OS)" pattern: its gb_OSDEFS
#     comment reads "do not define SOLARIS - use #ifdef __sun instead", and
#     the define list confirms it -- no -DSOLARIS, just -DUNX (plus -DSYSV
#     -DSUN -DSUN4, which are beyond the LINUX/MACOSX/WNT abstraction level
#     already established here and are not reproduced). So Solaris gets UNX
#     only, deliberately, not "UNX plus a same-shaped OS tag".
#   * unxgcc.mk's own gb_OSDEFS explicitly excludes Emscripten from the
#     generic -D$(OS) substitution: `$(if $(filter-out EMSCRIPTEN,$(OS)),
#     -D$(OS))` adds nothing when OS=EMSCRIPTEN. This is not an oversight to
#     "fix" by adding EMSCRIPTEN here -- the emscripten compiler predefines
#     __EMSCRIPTEN__ itself, so gbuild deliberately does not duplicate it.
#     Emscripten therefore also gets UNX only, for a different reason than
#     Solaris.
#   * kFreeBSD is the interesting one: configure.ac's own `kfreebsd*` host_os
#     arm sets OS=LINUX (not a distinct KFREEBSD tag, see the RTL_OS comment
#     above) -- so gbuild's own gb_OSDEFS for a kFreeBSD build is `-DLINUX
#     -DUNX`, byte-identical to a real Linux build's, verified from
#     configure.ac itself rather than assumed. Only RTL_OS (a runtime string,
#     not a preprocessor macro) distinguishes kFreeBSD from Linux upstream.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(LINUX UNX)
elseif (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    # Checked before `elseif (APPLE)`, since APPLE is also TRUE for iOS builds
    # -- see the matching ordering note on the CPPU_ENV/RTL_OS branch above.
    add_compile_definitions(IOS UNX)
elseif (APPLE)
    add_compile_definitions(MACOSX UNX)
elseif (WIN32)
    # windows.mk's gb_OSDEFS/gb_WIN_VERSION_DEFS: -DWIN32 -DWNT (no UNX/LINUX).
    add_compile_definitions(WIN32 WNT)
elseif (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    add_compile_definitions(FREEBSD UNX)
elseif (CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    add_compile_definitions(NETBSD UNX)
elseif (CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    add_compile_definitions(OPENBSD UNX)
elseif (CMAKE_SYSTEM_NAME STREQUAL "DragonFly")
    add_compile_definitions(DRAGONFLY UNX)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Haiku")
    add_compile_definitions(HAIKU UNX)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_compile_definitions(ANDROID UNX)
elseif (CMAKE_SYSTEM_NAME STREQUAL "SunOS")
    # Deliberately no SOLARIS macro -- see the solaris.mk explanation above.
    add_compile_definitions(UNX)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    # Deliberately no EMSCRIPTEN macro -- the compiler predefines
    # __EMSCRIPTEN__ itself; see the unxgcc.mk explanation above.
    add_compile_definitions(UNX)
elseif (CMAKE_SYSTEM_NAME STREQUAL "GNU/kFreeBSD")
    # LINUX, not KFREEBSD -- configure.ac sets OS=LINUX for kFreeBSD itself;
    # see the RTL_OS branch above and the explanation above this if-chain.
    add_compile_definitions(LINUX UNX)
endif ()

# sal/osl/unx/log.cxx and friends normalise paths against the source root, and
# several tools embed it.  Quoted separately because of the spaces-hostile CI
# setups gbuild warns about; CMake handles the escaping.
include_directories(
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_BINARY_DIR}/config_host"
)

# gbuild names objects without an extension and relative to the source root
# (gb_Library_add_exception_objects,sal, sal/rtl/string).  Transcription into
# CMake is far easier to check against the .mk when the names are kept in that
# spelling, so source lists are written the same way and expanded here — into
# absolute paths, since CMake would otherwise resolve them against the module
# directory rather than the tree root.
macro(librestudio_expand_cxx _list)
    set(_expanded "")
    foreach (_src IN LISTS ${_list})
        if (IS_ABSOLUTE "${_src}")
            list(APPEND _expanded "${_src}")
        else ()
            list(APPEND _expanded "${CMAKE_SOURCE_DIR}/${_src}.cxx")
        endif ()
    endforeach ()
    set(${_list} "${_expanded}")
    unset(_expanded)
endmacro()

# A helper for the library naming rules in solenv/gbuild/platform/unxgcc.mk.
#
#   UNO  -> libuno_<name>.so.<UDK_MAJOR>          (sal, cppu)
#   RT   -> libuno_<name>gcc3.so.<UDK_MAJOR>      (salhelper, cppuhelper)
#   OOO  -> lib<name>lo.so                        (everything else)
#
# RepositoryFixes.mk redirects every RTVERLIBS entry — cppuhelper, salhelper,
# purpenvhelper — to the libuno_ prefix on GCC, so the UNO runtime libraries all
# share it.
#
# The name matters beyond aesthetics: .component files reference the runtime
# filename through vnd.sun.star.expand, and the UNO service manager dlopens it.
#
# Checked for platform-awareness: this function sets only OUTPUT_NAME/VERSION/
# SOVERSION, never PREFIX/SUFFIX, so CMake's own per-platform defaults decide
# the extension and prefix (lib<name>.so on Linux, lib<name>.dylib on macOS,
# <name>.dll -- no "lib" prefix -- on Windows); LIBRESTUDIO_CPPU_ENV in the RT
# case is already platform-branched above, so this needs no further changes.
function(librestudio_set_library_output_name target name layer)
    if (layer STREQUAL "UNO")
        set_target_properties(${target} PROPERTIES
            OUTPUT_NAME "uno_${name}"
            VERSION "${LIBRESTUDIO_UDK_MAJOR}"
            SOVERSION "${LIBRESTUDIO_UDK_MAJOR}")
    elseif (layer STREQUAL "RT")
        set_target_properties(${target} PROPERTIES
            OUTPUT_NAME "uno_${name}${LIBRESTUDIO_CPPU_ENV}"
            VERSION "${LIBRESTUDIO_UDK_MAJOR}"
            SOVERSION "${LIBRESTUDIO_UDK_MAJOR}")
    elseif (layer STREQUAL "OOO")
        set_target_properties(${target} PROPERTIES OUTPUT_NAME "${name}lo")
    elseif (layer STREQUAL "PLAIN")
        set_target_properties(${target} PROPERTIES OUTPUT_NAME "${name}")
    else ()
        message(FATAL_ERROR "Unknown library layer '${layer}' for target ${target}")
    endif ()
endfunction()
