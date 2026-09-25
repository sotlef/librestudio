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

check_cxx_source_compiles("
    #include <vector>
    constinit static std::vector<int> v;
    int main() { return (int)v.size(); }
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

foreach (_var IN LISTS _cxxabi_functions _cxxabi_types)
    if (NOT HAVE_CXXABI_H_${_var})
        set(HAVE_CXXABI_H_${_var} 0)
    endif ()
endforeach ()

# --- remaining values --------------------------------------------------------

# configmgr can read the configuration through dconf instead of through files.
# Upstream turns that on when libdconf is found; there is no option for it here
# yet, and dconf is not installed on this machine either.
set(ENABLE_DCONF 0)

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


# The header values are worked out separately, and have to be 0 or 1 rather than
# TRUE or FALSE: they go into a #define that the preprocessor compares with #if,
# and `#if TRUE` is false — TRUE being an identifier it has never heard of.  The
# same shape as HAVE_FEATURE_JAVA above.
foreach (_switch USING_X11 USE_HEADLESS_CODE ENABLE_WASM_STRIP_SPLASH ENABLE_GEN)
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
set(HAVE_FEATURE_OPENCL 0)      # BUILD_TYPE is DESKTOP
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
    config_folders
    config_options
    config_locales
    config_probes
    config_typesizes
    config_cxxabi
    config_version
    config_java
    config_fuzzers
    config_oox
    config_gpgme
    config_crypto
    config_dconf
    config_liblangtag
    config_emscripten
    config_vclplug
    config_afdko
    config_atspi
    config_cairo_canvas
    config_cairo_rgba
    config_cpdb
    config_cups
    config_dbus
    config_eot
    config_feature_desktop
    config_feature_opencl
    config_fonts
    config_gio
    config_libcxx
    config_mpl
    config_skia
    config_wasm_strip)

foreach (_header IN LISTS _config_headers)
    configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/config/${_header}.h.in"
        "${LIBRESTUDIO_CONFIG_HOST_DIR}/${_header}.h"
        @ONLY)
endforeach ()
