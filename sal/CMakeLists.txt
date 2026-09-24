# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sal — the system abstraction layer: rtl (strings, containers, crypto, locale),
# osl (files, processes, threads, sockets) and the text encoding converters.
#
# Source lists are transcribed from Library_sal.mk, which is the specification
# for what belongs in this library; the WNT, macOS, iOS, Android, EMSCRIPTEN and
# SPARC branches there are not ported.

# --- sources -----------------------------------------------------------------

# sal/osl/all, sal/rtl and the platform-independent text encoders: always built.
# See gb_Library_add_exception_objects in sal/Library_sal.mk.
set(SAL_COMMON_SOURCES
    sal/osl/all/compat
    sal/osl/all/debugbase
    sal/osl/all/filepath
    sal/osl/all/loadmodulerelative
    sal/osl/all/log
    sal/osl/all/mutexshared
    sal/osl/all/signalshared
    sal/osl/all/threadshared
    sal/osl/all/utility

    sal/rtl/alloc_arena
    sal/rtl/alloc_cache
    sal/rtl/alloc_fini
    sal/rtl/alloc_global
    sal/rtl/bootstrap
    sal/rtl/byteseq
    sal/rtl/cipher
    sal/rtl/cmdargs
    sal/rtl/crc
    sal/rtl/digest
    sal/rtl/hash
    sal/rtl/locale
    sal/rtl/math
    sal/rtl/preinit
    sal/rtl/random
    sal/rtl/rtl_process
    sal/rtl/strbuf
    sal/rtl/strimp
    sal/rtl/string
    sal/rtl/unload
    sal/rtl/uri
    sal/rtl/ustrbuf
    sal/rtl/ustring
    sal/rtl/uuid

    sal/textenc/converter
    sal/textenc/convertsimple
    sal/textenc/handleundefinedunicodetotextchar
    sal/textenc/tcvtutf8
    sal/textenc/tencinfo
    sal/textenc/textcvt
    sal/textenc/textenc
    sal/textenc/unichars)

# Unix platform layer.  sal/osl/unx/interlck is an exception object everywhere
# except SPARC, where it is hand-written assembly in osl/unx/asm.
set(SAL_UNX_SOURCES
    sal/osl/unx/backtraceapi
    sal/osl/unx/conditn
    sal/osl/unx/file
    sal/osl/unx/file_error_transl
    sal/osl/unx/file_misc
    sal/osl/unx/file_path_helper
    sal/osl/unx/file_stat
    sal/osl/unx/file_url
    sal/osl/unx/file_volume
    sal/osl/unx/interlck
    sal/osl/unx/memory
    sal/osl/unx/module
    sal/osl/unx/mutex
    sal/osl/unx/nlsupport
    sal/osl/unx/pipe
    sal/osl/unx/process
    sal/osl/unx/process_impl
    sal/osl/unx/random
    sal/osl/unx/readwrite_helper
    sal/osl/unx/salinit
    sal/osl/unx/security
    sal/osl/unx/signal
    sal/osl/unx/socket
    sal/osl/unx/soffice
    sal/osl/unx/system
    sal/osl/unx/tempfile
    sal/osl/unx/thread
    sal/osl/unx/time
    sal/osl/unx/uunxapi)

# The legacy encodings live in a dlopen'd plug-in, not in libsal: textenc.cxx
# only compiles its loader when COND_LIB_SAL_TEXTENC is defined, and aborts at
# run time if the library cannot be found.
set(SAL_TEXTENC_SOURCES
    sal/textenc/context
    sal/textenc/convertbig5hkscs
    sal/textenc/converteuctw
    sal/textenc/convertgb18030
    sal/textenc/convertisciidevangari
    sal/textenc/convertiso2022cn
    sal/textenc/convertiso2022jp
    sal/textenc/convertiso2022kr
    sal/textenc/convertsinglebytetobmpunicode
    sal/textenc/tables
    sal/textenc/tcvtbyte
    sal/textenc/tcvtmb
    sal/textenc/tcvtutf7)

# --- libsal ------------------------------------------------------------------

librestudio_expand_cxx(SAL_COMMON_SOURCES)
librestudio_expand_cxx(SAL_UNX_SOURCES)
librestudio_expand_cxx(SAL_TEXTENC_SOURCES)

set(SAL_INCLUDE_DIRS
    "${CMAKE_SOURCE_DIR}/sal/inc")

add_library(sal SHARED
    ${SAL_COMMON_SOURCES}
    ${SAL_UNX_SOURCES}
    "${CMAKE_SOURCE_DIR}/sal/osl/unx/backtrace.c")

target_include_directories(sal PRIVATE ${SAL_INCLUDE_DIRS})

target_compile_definitions(sal PRIVATE
    SAL_DLLIMPLEMENTATION
    COND_LIB_SAL_TEXTENC
    SYSTEM_ZLIB
    ZLIB_CONST)

target_include_directories(sal PRIVATE
    "${DRAGONBOX_INCLUDE_DIR}"
    "${FAST_FLOAT_INCLUDE_DIR}")

target_link_libraries(sal PRIVATE
    ZLIB::ZLIB
    ${CMAKE_DL_LIBS}
    rt
    Threads::Threads)

# sal/util/sal.map pins sal's exported symbols and gives them their UNO ABI
# versions; dropping it would expose every internal helper and break the ABI
# contract the rest of the tree is built against.
target_link_options(sal PRIVATE
    "-Wl,--version-script=${CMAKE_SOURCE_DIR}/sal/util/sal.map")

librestudio_set_library_output_name(sal sal UNO)

# --- libsal_textenclo --------------------------------------------------------

add_library(sal_textenc SHARED ${SAL_TEXTENC_SOURCES})
target_include_directories(sal_textenc PRIVATE ${SAL_INCLUDE_DIRS})
target_link_libraries(sal_textenc PRIVATE sal)
librestudio_set_library_output_name(sal_textenc sal_textenc OOO)

# --- osl_process_child -------------------------------------------------------

# osl_executeProcess on Unix spawns this helper to start the child with the
# right signal mask and descriptor setup.  Upstream keeps it under qa/ because
# the test suite exercises it too, but the shipped osl depends on it.
add_executable(osl_process_child
    "${CMAKE_SOURCE_DIR}/sal/qa/osl/process/osl_process_child.cxx")
target_include_directories(osl_process_child PRIVATE ${SAL_INCLUDE_DIRS})
target_link_libraries(osl_process_child PRIVATE sal)

# --- install -----------------------------------------------------------------

install(TARGETS sal sal_textenc osl_process_child
    LIBRARY DESTINATION program
    RUNTIME DESTINATION program)
