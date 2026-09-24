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
include(CheckTypeSize)

set(LIBRESTUDIO_CONFIG_HOST_DIR "${CMAKE_BINARY_DIR}/config_host")
file(MAKE_DIRECTORY "${LIBRESTUDIO_CONFIG_HOST_DIR}")

# --- config_global.h ---------------------------------------------------------

check_include_file(syslog.h HAVE_SYSLOG_H)

check_cxx_source_compiles("
    int main() { return __has_builtin(__builtin_atomic_exchange) ? 0 : 1; }
" HAVE_GCC_BUILTIN_ATOMIC)

check_cxx_source_compiles("
    consteval int f() { return 0; }
    int main() { return f(); }
" HAVE_CPP_CONSTEVAL)

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

# Runtime optimisations change sal's threading and reference-counting internals.
# Upstream leaves them off unless --enable-runtime-optimizations is given.
set(ENABLE_RUNTIME_OPTIMIZATIONS 0)

# Text encoding tables cover every locale; --with-lang=ALL is upstream's default
# and what the tables in sal/textenc/tables.cxx expect.
set(WITH_LOCALE_ALL 1)

set(HAVE_FEATURE_MULTIUSER_ENVIRONMENT 1)
set(HAVE_FEATURE_BACKTRACE 0)

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
else ()
    set(USE_TLS_NSS 0)
    set(USE_TLS_OPENSSL 1)
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
    config_liblangtag
    config_emscripten
    config_vclplug)

foreach (_header IN LISTS _config_headers)
    configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/config/${_header}.h.in"
        "${LIBRESTUDIO_CONFIG_HOST_DIR}/${_header}.h"
        @ONLY)
endforeach ()
