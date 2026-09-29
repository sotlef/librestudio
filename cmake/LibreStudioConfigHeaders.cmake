# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Generation of the config_host/config_*.h headers.
#
# configure.ac produces 51 of these through autoconf's AC_CONFIG_HEADERS; the
# UNO core reads 15 of them.  Rather than emulating autoconf we run the checks
# with CMake and expand our own templates from cmake/config/, which keeps the
# build self-contained and the values explicit.
#
# The headers land in <build>/config_host, which LibreStudioPlatform.cmake puts
# on the include path — the same arrangement as gbuild's -I$(BUILDDIR)/config_host
# in solenv/gbuild/platform/com_GCC_defs.mk.

include(CheckIncludeFile)
include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)
include(CheckTypeSize)

set(LIBRESTUDIO_CONFIG_HOST_DIR "${CMAKE_BINARY_DIR}/config_host")
file(MAKE_DIRECTORY "${LIBRESTUDIO_CONFIG_HOST_DIR}")

# --- config_global.h ---------------------------------------------------------

check_include_file(syslog.h HAVE_SYSLOG_H)

check_cxx_source_compiles("
    int main() { return __has_builtin(__builtin_atomic_exchange) ? 0 : 1; }
" HAVE_GCC_BUILTIN_ATOMIC)

# This probe is upstream's, taken from configure.ac whole rather than reduced to
# "does the compiler know the keyword".  The S1 and S2 cases below are the point:
# GCC 13 compiles consteval perfectly well but rejects a consteval *constructor*
# called from an ordinary constructor's member-initialiser, and
# include/tools/color.hxx uses exactly that pattern behind this macro.  A probe
# that only tests the keyword answers yes, HAVE_CPP_CONSTEVAL is defined, and
# svtools/colorcfg.hxx then fails to compile with "'this' is not a constant
# expression".  Upstream uses AC_RUN_IFELSE, so this runs the program too: the
# S at the top verifies that the consteval constructor really did assign at
# compile time.
check_cxx_source_runs([=[
    struct S {
        consteval S() { i = 1; }
        int i = 0;
    };
    S const s;

    struct S1 {
         int a;
         consteval S1(int n): a(n) {}
    };
    struct S2 {
        S1 x;
        S2(): x(0) {}
    };

    struct S3 {
        consteval S3() {}
        union {
            int a;
            unsigned b = 0;
        };
    };
    void f() { S3(); }

    struct S4 { consteval S4() = default; };
    void f4(bool b) { b ? S4() : S4(); }

    struct S5 {
        consteval S5() { c = 0; }
        char * f() { return &c; }
        union {
            char c;
            int i;
        };
    };
    auto s5 = S5().f();

    struct S6 {
        consteval S6(char const (&lit)[2]) {
            buf[0] = lit[0];
            buf[1] = lit[1];
        }
        union {
            int x;
            char buf[2];
        };
    };
    void f6() { S6("a"); }

    int main() {
        return (s.i == 1) ? 0 : 1;
    }
]=] HAVE_CPP_CONSTEVAL)

# Fix (first real `cmake --build`, 2026-09-27): this probe used to test a
# default-constructed, EMPTY constinit std::vector -- trivially true on
# virtually any compiler (an empty vector's default constructor allocates
# nothing), so it always reported "yes" regardless of whether a *populated*
# constinit vector (the real question sc/source/core/tool/token.cxx's own
# "#if HAVE_CPP_CONSTINIT_SORTED_VECTOR" branch depends on, ScTokenArray::
# CheckForThreading's 29-element aThreadedCalcDenyList) actually works.
# GCC 13.2.1 accepted this port's old, too-lenient probe but rejected the
# real usage with "'constinit' variable ... does not have a constant
# initializer" -- same class of mistake this project's own notes already
# record for HAVE_CPP_CONSTEVAL ("a probe costs nothing if you supplied the
# variable yourself; check against configure.ac in full"). Replaced with
# upstream's own real probe, transcribed verbatim from configure.ac
# (~line 8430, "whether $CXX_BASE supports C++2a constinit sorted vectors"):
# a templated wrapper actually populated via an initializer_list and sorted,
# not an empty vector.
check_cxx_source_compiles("
    #include <algorithm>
    #include <initializer_list>
    #include <vector>
    template<typename T> class S {
    private:
        std::vector<T> v_;
    public:
        constexpr S(std::initializer_list<T> i): v_(i) { std::sort(v_.begin(), v_.end()); }
    };
    constinit S<int> s{3, 2, 1};
    int main() { return 0; }
" HAVE_CPP_CONSTINIT_SORTED_VECTOR)

check_cxx_source_compiles("
    #include <compare>
    int main() { return (1.0 <=> 2.0) < 0 ? 0 : 1; }
" HAVE_CPP_STRONG_ORDER)

set(HAVE_SYSLOG_H ${HAVE_SYSLOG_H})
set(HAVE_DLAPI 0)
set(HAVE_UNIX_DLAPI 1)
set(HAVE_GCC_WSFINAE_INCOMPLETE 0)
set(HAVE_P1155R3 0)

# --- config_typesizes.h ------------------------------------------------------

# include/sal/types.h includes sal/typesizes.h, which is a static shim in the
# tree that includes config_typesizes.h — so this header reaches every
# translation unit in the tree, not just the ones that ask for it.
check_type_size("short" SAL_TYPES_SIZEOFSHORT)
check_type_size("int" SAL_TYPES_SIZEOFINT)
check_type_size("long" SAL_TYPES_SIZEOFLONG)
check_type_size("long long" SAL_TYPES_SIZEOFLONGLONG)
check_type_size("void *" SAL_TYPES_SIZEOFPOINTER)

# Alignment of a type of a given size is that size for the ABIs we support;
# upstream hardcodes the same values in configure.ac's fallback table.
set(SAL_TYPES_ALIGNMENT2 2)
set(SAL_TYPES_ALIGNMENT4 4)
set(SAL_TYPES_ALIGNMENT8 8)

# --- config_cxxabi.h ---------------------------------------------------------

# bridges/source/cpp_uno/gcc3_* links against libstdc++'s ABI directly, so each
# entry point it uses is probed separately.
#
# This whole probe is Itanium-C++-ABI-specific (cxxabi.h, __cxxabiv1::...),
# which is what GCC/Clang/libstdc++ (and libc++) implement on Linux and macOS.
# MSVC does not implement the Itanium ABI at all -- it has its own, unrelated
# exception/RTTI ABI and no <cxxabi.h> -- so every check below would just fail
# under cl.exe rather than answer anything useful; skipping the block under
# MSVC turns that into "not attempted" instead of "attempted and failed". This
# only matters once the mscx_uno bridge (which would need its own, entirely
# different ABI probes, not these) exists to build against on Windows.
#
# The two lists of names are declared unconditionally (they are just symbol
# names, nothing platform-specific), so that the "leave it a defined 0" loop
# below still runs on MSVC and every HAVE_CXXABI_H_* macro config_cxxabi.h.in
# expects still gets a value, just 0 for all of them, instead of an
# `@HAVE_CXXABI_H_...@` left unsubstituted by configure_file().
set(_cxxabi_functions
    CXA_GET_GLOBALS
    CXA_THROW
    CXA_ALLOCATE_EXCEPTION
    CXA_CURRENT_EXCEPTION_TYPE)
set(_cxxabi_types
    CXA_EXCEPTION
    CXA_EH_GLOBALS
    CLASS_TYPE_INFO
    SI_CLASS_TYPE_INFO
    VMI_CLASS_TYPE_INFO
    BASE_CLASS_TYPE_INFO)

if (NOT MSVC)
    foreach (_sym IN LISTS _cxxabi_functions)
        string(TOLOWER "${_sym}" _lower)
        check_cxx_source_compiles("
            #include <cxxabi.h>
            int main() { (void)&__cxxabiv1::__${_lower}; return 0; }
        " HAVE_CXXABI_H_${_sym})
    endforeach ()

    foreach (_sym IN LISTS _cxxabi_types)
        string(TOLOWER "${_sym}" _lower)
        check_cxx_source_compiles("
            #include <cxxabi.h>
            int main() { (void)sizeof(__cxxabiv1::__${_lower}); return 0; }
        " HAVE_CXXABI_H_${_sym})
    endforeach ()
endif ()

foreach (_var IN LISTS _cxxabi_functions _cxxabi_types)
    if (NOT HAVE_CXXABI_H_${_var})
        set(HAVE_CXXABI_H_${_var} 0)
    endif ()
endforeach ()

# --- remaining values --------------------------------------------------------

# configmgr can read the configuration through dconf instead of through files.
# configure.ac's AC_ARG_ENABLE(dconf, ...) leaves enable_dconf unset by
# default (--disable-dconf is the only way to turn it off), and the
# PKG_CHECK_MODULES([DCONF], [dconf >= 0.40.0]) check right after treats
# "unset" the same as "try to autodetect": the backend ends up on whenever
# dconf is found and off otherwise. This build detects rather than hardcodes
# it, the same "is this library installed" rule the CUPS/DBUS/GIO switches
# below follow. dconf is not installed on this machine, so this reduces to
# off here, but a machine that has it gets the feature without a rebuild
# switch to flip.
pkg_check_modules(DCONF dconf>=0.40.0)
if (DCONF_FOUND)
    set(ENABLE_DCONF 1)
else ()
    set(ENABLE_DCONF 0)
endif ()

# --- vcl's feature switches ---------------------------------------------------
#
# A group of the config headers below answer "is this library installed" rather
# than "which branch was chosen", and configure.ac decides them from the same
# evidence: the options behind them default to on, so a system that has the
# library gets the feature.  This build detects rather than hardcodes, so that a
# machine without libcups builds a vcl without CUPS instead of one that fails to
# link.  Each header names the upstream condition it mirrors.
#
# Two of them are worth spelling out.  ENABLE_GIO is tied to a GTK back end
# upstream only because that is the configuration in which it links gio at all;
# here libvcl links gio unconditionally and compiles
# vcl/unx/generic/window/sessioninhibitor.cxx, which needs the flag to do
# anything.  ENABLE_EOT is on by default upstream for anything that is not a
# fuzzer build.

pkg_check_modules(CUPS cups)
pkg_check_modules(DBUS dbus-1>=0.60)
pkg_check_modules(DBUS_GLIB dbus-glib-1)
pkg_check_modules(GIO_FEATURE gio-2.0>=2.26)
pkg_check_modules(LIBEOT_FEATURE libeot>=0.01)
pkg_check_modules(ATSPI_SCROLL_TO atspi-2>=2.32)

foreach (_switch
        "ENABLE_CUPS;CUPS"
        "ENABLE_DBUS;DBUS"
        "DBUS_HAVE_GLIB;DBUS_GLIB"
        "ENABLE_GIO;GIO_FEATURE"
        "ENABLE_EOT;LIBEOT_FEATURE"
        "HAVE_ATSPI2_SCROLL_TO;ATSPI_SCROLL_TO")
    list(GET _switch 0 _variable)
    list(GET _switch 1 _prefix)
    if (${_prefix}_FOUND)
        set(${_variable} 1)
    else ()
        set(${_variable} 0)
    endif ()
endforeach ()

# The rest are decisions rather than detections: a build type, an option, or a
# platform this build is not.  Each is what the branch of configure.ac that
# applies to a Linux desktop build would produce.
# USING_X11 and USE_HEADLESS_CODE are what config_vclplug.h exists for; both are
# on for a Linux desktop build.  configure.ac defines USE_HEADLESS_CODE whenever
# freetype and fontconfig are used, which is exactly this case.
# Cache variables rather than plain ones, so that they show up in cmake -L and
# can be flipped to check what depends on them.  A value that only exists for
# the length of a configure run cannot be tested, and an untested condition is
# indistinguishable from a comment.
#
# The two below are derived, as configure.ac derives them: USING_X11 from the
# presence of an X11 backend, USE_HEADLESS_CODE from the build using freetype
# and fontconfig — which a Linux desktop build does, hence the name being
# misleading.  DISABLE_GUI is the one a person chooses, upstream's --disable-gui.
set(USING_X11 TRUE CACHE BOOL "configure.ac's USING_X11: this build has an X11 backend")
set(USE_HEADLESS_CODE TRUE CACHE BOOL
    "configure.ac's USE_HEADLESS_CODE: the build uses freetype and fontconfig, which is what selects the generic font handling, the printer back ends and the code the X11 plug-in shares")
option(DISABLE_GUI "configure.ac's --disable-gui: build without a toolkit" OFF)

# ENABLE_WASM_STRIP_SPLASH is a switch rather than a value: vcl/CMakeLists.txt
# tests it to decide whether the splash window is part of the library, which is
# the one condition Library_vcl.mk has in the middle of its object list.
set(ENABLE_WASM_STRIP_SPLASH FALSE CACHE BOOL
    "configure.ac's --enable-wasm-strip: leave the splash window out")

# ENABLE_GEN is what tells libvcl's plug-in loader that the X11 back end exists;
# vcl/CMakeLists.txt builds it under the same switch.  Upstream enables it for a
# Linux GUI build.
set(ENABLE_GEN TRUE CACHE BOOL "configure.ac's ENABLE_GEN: build the X11 plug-in")

# ENABLE_GTK3 is the same kind of switch, and it carries more weight than the
# name suggests: vcl/source/app/salplug.cxx puts gtk3 at the head of both the
# desktop-autodetection list and the fallback list under this macro, so with it
# off libvcl will not even try to load a GTK3 back end that is sitting next to
# it.  Upstream's default follows test_gtk3, which follows using_x11, and then
# requires gtk+-3.0 outright — a build asked for GTK3 without it fails to
# configure.  Deriving the default from the packages being present keeps that
# answer where they are and still lets the build configure where they are not;
# asking for it explicitly on a machine without them fails in pkg_check_modules
# in vcl/CMakeLists.txt, which is the same failure, later and with a better
# message.  The X11 and GUI halves of the condition are upstream's too: gtk3's
# check is reached only when test_gtk3 has followed using_x11 on, and a build
# without a toolkit has no use for a toolkit back end.
pkg_check_modules(GTK3_DETECT gtk+-3.0>=3.24 gmodule-no-export-2.0 glib-2.0>=2.38 atk>=2.36 cairo)
if (USING_X11 AND NOT DISABLE_GUI AND GTK3_DETECT_FOUND)
    set(_enable_gtk3_default TRUE)
else ()
    set(_enable_gtk3_default FALSE)
endif ()
set(ENABLE_GTK3 ${_enable_gtk3_default} CACHE BOOL
    "configure.ac's ENABLE_GTK3: build the GTK3 plug-in")

# And RandR is derived, as configure.ac derives it, from the library being
# there — a display that changes size should not need to be told about.
pkg_check_modules(XRANDR_DETECT xrandr)
if (XRANDR_DETECT_FOUND)
    set(_enable_randr_default TRUE)
else ()
    set(_enable_randr_default FALSE)
endif ()
set(ENABLE_RANDR ${_enable_randr_default} CACHE BOOL
    "configure.ac's ENABLE_RANDR: the back end notices display changes")

# The two switches below are not about what this machine has installed, but
# about which slice of the LibreOffice feature matrix this port builds.
# Upstream ties them to --enable-scripting and a BUILD_TYPE filter
# (SCRIPTING, DBCONNECTIVITY) that gbuild's Module_*.mk files use to decide
# which Library_*.mk targets even get added; this port had been resolving
# that filter silently -- build the gated target unconditionally, with a
# comment explaining why -- instead of through a real, testable switch. Both
# default on, matching configure.ac's own default and this port's actual
# behaviour so far, so flipping either off now is a real choice instead of an
# unwritten one. (Turning either off does not yet remove anything: the
# CMakeLists.txt files that consume them gate the specific target/source
# subset each flag actually controls in upstream, per module.)
option(ENABLE_SCRIPTING
    "configure.ac's --enable-scripting (BUILD_TYPE's SCRIPTING filter): build the Basic/VBA object-model libraries (basctl, scripting, vbahelper, and the sc/sw VBA object libraries)" ON)
option(ENABLE_DBCONNECTIVITY
    "configure.ac's --enable-database-connectivity (BUILD_TYPE's DBCONNECTIVITY filter): build the database-facing pieces of connectivity/dbaccess/forms/svx/sw" ON)
option(ENABLE_XMLHELP
    "configure.ac's --enable-xmlhelp (defaults on for desktop builds): compile in sfx2's interactive Help (F1) wiring, which xmlhelp's ucpchelp1 UNO service backs at runtime" ON)

# ENABLE_AVMEDIA follows the same "on by default, off only for a build this
# port does not do" shape as ENABLE_DBCONNECTIVITY/ENABLE_XMLHELP above.
# configure.ac:1648-1651 (AC_ARG_ENABLE(avmedia, ...)) defaults enable_avmedia
# to "yes" whenever --disable-avmedia is not given
# (`test "${enable_avmedia+set}" = set || enable_avmedia=yes`); the only place
# that flips it to "no" is configure.ac:3461-3462, gated on
# `$enable_wasm_strip = yes` -- a WASM-specific switch this desktop Linux
# port never sets. configure.ac:3513-3515 then appends AVMEDIA to BUILD_TYPE
# and AC_DEFINEs HAVE_FEATURE_AVMEDIA precisely when enable_avmedia = yes, so
# a bare ./configure on this platform genuinely builds the AVMEDIA-gated part
# of avmedia/Library_avmedia.mk. Default ON here matches that evidence.
option(ENABLE_AVMEDIA
    "configure.ac's --enable-avmedia (BUILD_TYPE's AVMEDIA filter, defaults on outside --enable-wasm-strip): build avmedia's media-player framework/viewer sources (mediacontrol, mediaplayer, mediatoolbox, soundhandler, mediawindow) and register avmedia.component, on top of the always-built mediaitem.cxx stub" ON)

# ENABLE_ODK is the odd one out among this group: unlike --enable-scripting
# (configure.ac:1667-1670, whose action-if-not-given literally does
# `test "${enable_scripting+set}" = set || enable_scripting=yes`) and
# --enable-database-connectivity (configure.ac:3521 tests
# `!= no`, so an unset value still takes the "yes" branch),
# libo_FUZZ_ARG_ENABLE(odk, ...) (configure.ac:1761-1764) sets no default at
# all when the flag is absent: its action-if-not-given
# (m4/libo_fuzz_configury.m4's libo_FUZZ_ARG_ENABLE) only assigns a value
# when $enable_fuzz_options = yes (random fuzz-config selection), which is
# not this build. And the later check, configure.ac:9966, is a strict
# `test "$enable_odk" = yes` -- not `!= no` -- so a bare, unset
# $enable_odk takes the "no" branch and BUILD_TYPE never gets ODK appended.
# So a plain ./configure with no explicit --enable-odk genuinely defaults
# this off, unlike SCRIPTING/DBCONNECTIVITY/XMLHELP above (this matches
# what this port already found and recorded when it scoped unodevtools out
# for the same reason). distro-configs/LibreOfficeLinux.conf -- the profile
# this port's desktop-Linux target otherwise matches -- does pass
# --enable-odk explicitly, but that is a packaging choice layered on top of
# the bare default, not the default itself, so it is not reproduced here;
# OFF is the honest bare-configure default. unoidl/CMakeLists.txt gates
# unoidl-read and the unoidl-check_test CTest test on this; the odk module
# itself would gate on it too, once ported.
option(ENABLE_ODK
    "configure.ac's --enable-odk (BUILD_TYPE's ODK filter): build the Office Development Kit tools (unoidl-read here; more once the odk module itself is ported). Off by default -- see the comment above for why this differs from ENABLE_SCRIPTING/ENABLE_DBCONNECTIVITY/ENABLE_XMLHELP's on-by-default matches of configure.ac" OFF)

# ENABLE_ONLINE_UPDATE is the same shape as ENABLE_ODK, not the ENABLE_
# SCRIPTING/ENABLE_DBCONNECTIVITY/ENABLE_XMLHELP group above: configure.ac
# (~line 14650-14666, "Test whether to enable online update service") sets
# ENABLE_ONLINE_UPDATE="TRUE" only when --enable-online-update is passed
# explicitly ("if test "$enable_online_update" = ""; then ... AC_MSG_RESULT([no])"
# -- an unset $enable_online_update takes the "no" branch, there is no
# `action-if-not-given` that defaults it to "yes" the way --enable-scripting's
# does), so a bare ./configure genuinely leaves this off. extensions/
# CMakeLists.txt gates Library_updchk/Library_updatecheckui (and documents why
# Configuration_updchk stays unported either way) on this; extensions/
# CppunitTest_deferred.cmake gates CppunitTest_extensions_test_update on it too
# (it gb_CppunitTest_use_library_objects's updchk, so it cannot exist unless
# updchk itself is built).
option(ENABLE_ONLINE_UPDATE
    "configure.ac's --enable-online-update (off unless passed explicitly -- no bare-configure default the way --enable-scripting/--enable-database-connectivity/--enable-xmlhelp have one): build extensions' update-check UNO service (updchk) and its \"new version available\" notification dialog (updatecheckui)" OFF)

# Unlike every switch above, this one's own upstream default is OFF outside
# Windows (configure.ac: enable_pch=no unless $_os=WINNT) -- so defaulting
# it off here is not a simplification of this port's own, it is what a bare
# ./configure already does on this platform. pch/CMakeLists.txt and the
# per-target target_precompile_headers(...) calls scattered across the tree
# (wherever a module's own Library_*.mk called gb_*_set_precompiled_header
# or gb_LinkTarget_use_common_precompiled_header) are all gated on it, so
# flipping it on is the only way any of that machinery does anything.
option(ENABLE_PCH
    "configure.ac's --enable-pch (off by default except on Windows): compile translation units against a precompiled header, per-module where one exists, the shared pch/precompiled_system.hxx otherwise" OFF)

# These four are narrower: each gates one optional source block in exactly
# one or two already-ported libraries (sc's solver, sd's remote control),
# tied to one external each. ENABLE_AVAHI follows the same detect-a-library
# shape as ENABLE_GTK3/ENABLE_RANDR above; ENABLE_SDREMOTE_BLUETOOTH reuses
# DBUS_GLIB_FOUND from the pkg_check_modules call earlier in this file rather
# than probing dbus-glib-1 twice. ENABLE_SDREMOTE has no library to detect --
# it is upstream's own on-by-default UI feature, present regardless of what
# is installed -- so it is a plain default like ENABLE_GEN.
#
# ENABLE_COINMP is really libo_FUZZ_ARG_ENABLE(coinmp, --disable-coinmp,
# ...) (configure.ac:1690) -- a "disable" switch, so the real default is ON
# on a normal Linux desktop build regardless of what pkg-config finds (only
# a handful of unrelated platform blocks -- Windows/aarch64, Haiku,
# WASM-strip -- set enable_coinmp=no; none apply here). 2026-09-28: no
# system `coinmp`+`coinutils` pair exists on this platform at all
# (`apt-cache search coinmp` -- empty), but sc/CMakeLists.txt's own "solver"
# section now vendors external/coinmp/'s download.lst tarball when
# COINMP_DETECT_FOUND is false -- so TRUE unconditionally is the honest
# default now, matching configure.ac, not the detected-package value.
# COINMP_DETECT itself is still real and still read there, to choose system
# vs. bundled -- fixed here to check coinmp AND coinutils together, matching
# configure.ac's own combined PKG_CHECK_MODULES(COINMP, coinmp coinutils)
# (configure.ac:12405) exactly, rather than coinmp alone as before.
pkg_check_modules(AVAHI_DETECT avahi-client)
set(ENABLE_AVAHI ${AVAHI_DETECT_FOUND} CACHE BOOL
    "configure.ac's ENABLE_AVAHI: build sd's Avahi remote-control discovery backend")

pkg_check_modules(COINMP_DETECT coinmp coinutils)
set(ENABLE_COINMP TRUE CACHE BOOL
    "configure.ac's --enable-coinmp: build sc solver's CoinMP-based solvers, from system coinmp+coinutils if both present (COINMP_DETECT_FOUND), the download.lst tarball otherwise")

set(ENABLE_SDREMOTE TRUE CACHE BOOL
    "configure.ac's ENABLE_SDREMOTE (on by default): build Impress's remote-control server")

if (DBUS_GLIB_FOUND)
    set(_enable_sdremote_bluetooth_default TRUE)
else ()
    set(_enable_sdremote_bluetooth_default FALSE)
endif ()
set(ENABLE_SDREMOTE_BLUETOOTH ${_enable_sdremote_bluetooth_default} CACHE BOOL
    "configure.ac's ENABLE_SDREMOTE_BLUETOOTH: build the Bluetooth transport for sd's remote control")

# 2026-09-29, found via a real ctest run: configure.ac:2372-2373/5114-5125 --
# unset unless --enable-embindtest-uno is passed explicitly, OR auto-on when
# $_os = Emscripten AND ENABLE_DBGUTIL = TRUE (neither applies to this port's
# bare-configure Linux baseline) -- so bare OFF here matches upstream's own
# real default exactly, not a guess. Module_jsuno.mk itself only adds
# CppunitTest_jsuno_testuno to its check targets `$(if $(ENABLE_EMBINDTEST_
# UNO), ...)` -- so on a bare configure, upstream never even RUNS that test,
# despite CppunitTest_jsuno_testuno.mk unconditionally use_components-ing
# unotest/source/embindtest/embindtest. This was previously mis-scoped in
# this port as "embindtest is Emscripten-only, so leave jsuno_testuno
# registered anyway, expect it to fail" -- confirmed wrong by reading
# Module_jsuno.mk directly: embindtest is a perfectly portable native Library
# (Library_embindtest.mk: plain cppu/cppuhelper/sal/salhelper/tl/vcl, no
# Emscripten-specific build machinery at all), just off by default like any
# other opt-in feature -- and the test that needs it is équally gated.
option(ENABLE_EMBINDTEST_UNO
    "configure.ac's --enable-embindtest-uno (bare-configure default OFF unless $_os=Emscripten and ENABLE_DBGUTIL, neither true here): build unotest's embindtest UNO component and run jsuno_testuno, which needs it"
    OFF)


# The header values are worked out separately, and have to be 0 or 1 rather than
# TRUE or FALSE: they go into a #define that the preprocessor compares with #if,
# and `#if TRUE` is false — TRUE being an identifier it has never heard of.  The
# same shape as HAVE_FEATURE_JAVA above.
foreach (_switch USING_X11 USE_HEADLESS_CODE ENABLE_WASM_STRIP_SPLASH ENABLE_GEN
        ENABLE_GTK3 ENABLE_SCRIPTING ENABLE_DBCONNECTIVITY ENABLE_XMLHELP
        ENABLE_AVAHI ENABLE_AVMEDIA)
    if (${_switch})
        set(${_switch}_CONFIG 1)
    else ()
        set(${_switch}_CONFIG 0)
    endif ()
endforeach ()

set(ENABLE_CAIRO_CANVAS 1)      # cairo is linked into libvcl through
                                # USE_HEADLESS_CODE, and upstream's option is on
                                # by default when it is available
set(ENABLE_CAIRO_RGBA 0)        # Android's non-standard cairo pixel order
set(ENABLE_CPDB 0)              # --enable-cpdb, off by default
set(HAVE_FEATURE_DESKTOP 1)     # a desktop build: not iOS, not Android, not fuzzers

# Upstream ties this to a BUILD_TYPE of OPENCL, which this DESKTOP build
# never has -- but opencl/ itself (Library_opencl, Executable_opencltest)
# and its one dependency, the self-contained, already-vendored external/clew
# (a small dlopen-based OpenCL loader shim, no network fetch, no external
# deps beyond dlfcn.h), are both real, buildable targets on this platform.
# Rather than leave this permanently tied to a BUILD_TYPE value this port
# does not otherwise track, ENABLE_OPENCL is a real switch -- default ON,
# since opencl/clew are genuinely built here, unlike the other BUILD_TYPE
# values (SCRIPTING/DBCONNECTIVITY/XMLHELP) that mirror configure.ac's own
# upstream default. Flipping it off would need sc/CMakeLists.txt's own
# opencl-sourced block gated the same way (see sc/CMakeLists.txt).
option(ENABLE_OPENCL
    "Build opencl/clew: not part of this port's DESKTOP BUILD_TYPE upstream, but a real, locally-buildable target here" ON)
if (ENABLE_OPENCL)
    set(HAVE_FEATURE_OPENCL 1)
else ()
    set(HAVE_FEATURE_OPENCL 0)
endif ()
set(HAVE_MORE_FONTS 1)          # --with-fonts, yes by default
set(HAVE_LIBCPP 0)              # this build uses libstdc++
set(MPL_HAVE_SUBSET 0)          # --enable-mpl-subset, off by default
set(USE_AFDKO_PROGRAMS 0)       # BUILD_TYPE is DESKTOP
# The rest of what a wasm build may trim.  None of it is trimmed here, and none
# of it is tested by the build system, so they are values rather than switches.
set(ENABLE_WASM_STRIP_ACCESSIBILITY 0)
set(ENABLE_WASM_STRIP_CHART 0)
set(ENABLE_WASM_STRIP_EXTRA 0)
set(ENABLE_WASM_STRIP_HUNSPELL 0)
set(ENABLE_WASM_STRIP_PINGUSER 0)
set(ENABLE_WASM_STRIP_PREMULTIPLY 0)
set(ENABLE_WASM_STRIP_RECENT 0)
set(ENABLE_WASM_STRIP_RECOVERYUI 0)

# Runtime optimisations change sal's threading and reference-counting internals.
# Upstream leaves them off unless --enable-runtime-optimizations is given.
set(ENABLE_RUNTIME_OPTIMIZATIONS 0)

# Text encoding tables cover every locale; --with-lang=ALL is upstream's default
# and what the tables in sal/textenc/tables.cxx expect.
set(WITH_LOCALE_ALL 1)

set(HAVE_FEATURE_MULTIUSER_ENVIRONMENT 1)
set(HAVE_FEATURE_BACKTRACE 0)

# Upstream defines HAVE_FEATURE_UI unless --disable-gui was given, and this
# build has the GUI.  The two values are not cosmetic: with the flag off,
# vcl/inc/vcl/opengl/OpenGLWrapper.hxx does not declare
# OpenGLWrapper::getBufferSwapCounter(), while OpenGLContext.cxx goes on
# defining it, and the file stops compiling.
set(HAVE_FEATURE_UI 1)

# HAVE_FEATURE_OPENGL needs libGL, which configure.ac checks for by looking for
# glBegin.  There is nothing to link — vcl reaches the GL entry points through
# epoxy, which is what epoxy is for — so the library is only looked for, and the
# flag follows whether it is there.
find_library(GL_LIBRARY NAMES GL)
if (GL_LIBRARY)
    set(HAVE_FEATURE_OPENGL 1)
else ()
    set(HAVE_FEATURE_OPENGL 0)
endif ()

# liblangtag needed an "inline" workaround before 0.5.5 (configure.ac probes for
# it with PKG_CHECK_EXISTS).
if (LIBLANGTAG_VERSION VERSION_LESS 0.5.5)
    set(LIBLANGTAG_INLINE_FIX 1)
else ()
    set(LIBLANGTAG_INLINE_FIX 0)
endif ()

set(LIBO_VERSION_MAJOR 27)
set(LIBO_VERSION_MINOR 2)
set(LIBO_VERSION_MICRO 0)
set(LIBO_VERSION_PATCH 0)
set(LIBO_THIS_YEAR 2026)

if (LIBRESTUDIO_ENABLE_JAVA)
    set(HAVE_FEATURE_JAVA 1)
else ()
    set(HAVE_FEATURE_JAVA 0)
endif ()

if (LIBRESTUDIO_ENABLE_GPGME)
    set(HAVE_FEATURE_GPGME 1)
else ()
    set(HAVE_FEATURE_GPGME 0)
endif ()

if (LIBRESTUDIO_TLS_BACKEND STREQUAL "NSS")
    set(USE_TLS_NSS 1)
    set(USE_TLS_OPENSSL 0)
    set(USE_CRYPTO_NSS 1)
    set(HAVE_FEATURE_NSS 1)
    # Left unset on purpose: config_crypto.h.in turns this into `#undef`, and
    # systools/opensslinit.hxx keys off whether the macro exists at all.
    unset(SYSTEM_OPENSSL)
else ()
    set(USE_TLS_NSS 0)
    set(USE_TLS_OPENSSL 1)
    set(USE_CRYPTO_NSS 0)
    set(HAVE_FEATURE_NSS 0)
    set(SYSTEM_OPENSSL 1)
endif ()

# box2d ships no pkg-config file, only a CMake package-config (found via
# find_package(box2d REQUIRED) in the root CMakeLists.txt, before this file is
# included) -- its own Config.cmake sets box2d_VERSION_MAJOR/_MINOR from the
# installed version, which slideshow's box2dtools.hxx needs at the
# preprocessor level to pick its 2.x vs. 3.x API branch.
set(BOX2D_MAJOR ${box2d_VERSION_MAJOR})
set(BOX2D_MINOR ${box2d_VERSION_MINOR})

# --- expansion ---------------------------------------------------------------

# check_include_file and check_cxx_source_compiles leave their result variable
# empty on failure (and cache the emptiness), which would expand to a bare
# "#define HAVE_X " in the generated header.  Everything that reaches a template
# as a numeric define goes through here first.
foreach (_flag
        HAVE_SYSLOG_H
        HAVE_GCC_BUILTIN_ATOMIC
        HAVE_CPP_CONSTEVAL
        HAVE_CPP_CONSTINIT_SORTED_VECTOR
        HAVE_CPP_STRONG_ORDER)
    if (${_flag})
        set(${_flag} 1)
    else ()
        set(${_flag} 0)
    endif ()
endforeach ()

foreach (_size
        SAL_TYPES_SIZEOFSHORT
        SAL_TYPES_SIZEOFINT
        SAL_TYPES_SIZEOFLONG
        SAL_TYPES_SIZEOFLONGLONG
        SAL_TYPES_SIZEOFPOINTER)
    if (NOT ${_size})
        message(FATAL_ERROR
            "check_type_size could not determine ${_size}; the compiler probe "
            "failed, which usually means the toolchain is not usable.")
    endif ()
endforeach ()


set(_config_headers
    config_global
    config_features
    config_extensions
    config_collab
    config_folders
    config_options
    config_options_calc
    config_lgpl
    config_buildid
    config_buildconfig
    config_python
    config_poppler
    config_locales
    config_probes
    config_typesizes
    config_cxxabi
    config_version
    config_java
    config_fuzzers
    config_langs
    config_oauth2
    config_oox
    config_gpgme
    config_crypto
    config_dconf
    config_liblangtag
    config_emscripten
    config_vclplug
    config_afdko
    config_atspi
    config_box2d
    config_cairo_canvas
    config_cairo_rgba
    config_cpdb
    config_cups
    config_dbus
    config_eot
    config_feature_desktop
    config_feature_opencl
    config_firebird
    config_fonts
    config_gio
    config_libcxx
    config_mpl
    config_skia
    config_validation
    config_vendor
    config_wasm_strip
    config_zxing)

foreach (_header IN LISTS _config_headers)
    configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/config/${_header}.h.in"
        "${LIBRESTUDIO_CONFIG_HOST_DIR}/${_header}.h"
        @ONLY)
endforeach ()
