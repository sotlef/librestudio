# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sal — the system abstraction layer: rtl (strings, containers, crypto, locale),
# osl (files, processes, threads, sockets) and the text encoding converters.
#
# Source lists are transcribed from Library_sal.mk, which is the specification
# for what belongs in this library. Unlike an earlier version of this file,
# the WNT, macOS, iOS, Android, EMSCRIPTEN and SPARC branches ARE now
# structurally ported below (behind the matching if()/elseif() on WIN32,
# APPLE, CMAKE_SYSTEM_NAME STREQUAL "iOS"/"Android"/"Emscripten", and
# CMAKE_SYSTEM_PROCESSOR for SPARC) -- this project's standing rule is that
# nothing is silently excluded, even where it cannot be built or verified on
# this Linux machine. Every branch that cannot actually be exercised here is
# marked "unverified/best-effort" in its own comment rather than presented as
# a confident, tested fact; see cmake/LibreStudioPlatform.cmake (not modified
# by this file) for the shared LIBRESTUDIO_RTL_OS/LIBRESTUDIO_RTL_ARCH/
# LIBRESTUDIO_CPPU_ENV/WIN32/APPLE/CMAKE_SYSTEM_NAME plumbing this file reuses
# rather than re-deriving.

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

# Unix platform layer (Library_sal.mk:156-217, the `ifneq ($(OS),WNT)` block).
# Four files are deliberately NOT in this list, because Library_sal.mk itself
# branches on them individually rather than listing them unconditionally --
# they are added back in by the platform-conditional blocks further down,
# right before add_library(sal ...):
#   - sal/osl/unx/interlck: exception object (.cxx) everywhere except SPARC/
#     SPARC64, where it's replaced by hand-written assembly (.mk:209-217).
#   - sal/osl/unx/uunxapi and sal/osl/unx/system: exception objects (.cxx)
#     everywhere except macOS, where they are compiled as the .mm siblings
#     that already exist on disk (sal/osl/unx/uunxapi.mm, sal/osl/unx/
#     system.mm -- each just `#include`s its own .cxx, confirmed by reading
#     both files) via gb_Library_add_objcxxobjects (.mk:187-197).
#   - sal/osl/unx/osxlocale: only added under macOS or iOS (.mk:199-203,
#     `ifneq ($(filter MACOSX iOS,$(OS)),)`) -- absent on every other Unix,
#     Linux included, so it cannot live in this always-on list either.
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
    sal/osl/unx/tempfile
    sal/osl/unx/thread
    sal/osl/unx/time)

# Windows platform layer (Library_sal.mk:219-247, the `else # $(OS) == WNT`
# arm of the same if/else). Entirely disjoint from SAL_UNX_SOURCES above --
# gbuild replaces the whole Unix list with this one, it does not merge them.
# Every one of these 23 basenames was checked to actually exist on disk as
# sal/osl/w32/<name>.cxx (`ls sal/osl/w32/*.cxx`), so the *list* is verified;
# what is NOT verified is that this port actually compiles/links them, since
# this machine has no MSVC toolchain (no cl.exe/ml64.exe/Windows SDK -- see
# this project's own Windows/MSVC research) to try it against. Note
# sal/osl/w32/backtrace.cxx is the Windows equivalent of the Unix-only
# sal/osl/unx/backtrace.c cobject added separately below -- it is a regular
# exception object here, already included in this list, not a second special
# case.
set(SAL_W32_SOURCES
    sal/osl/w32/backtrace
    sal/osl/w32/conditn
    sal/osl/w32/dllentry
    sal/osl/w32/file
    sal/osl/w32/file_dirvol
    sal/osl/w32/file_error
    sal/osl/w32/file_url
    sal/osl/w32/interlck
    sal/osl/w32/memory
    sal/osl/w32/module
    sal/osl/w32/mutex
    sal/osl/w32/nlsupport
    sal/osl/w32/pipe
    sal/osl/w32/process
    sal/osl/w32/procimpl
    sal/osl/w32/random
    sal/osl/w32/salinit
    sal/osl/w32/security
    sal/osl/w32/signal
    sal/osl/w32/socket
    sal/osl/w32/tempfile
    sal/osl/w32/thread
    sal/osl/w32/time)

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

# --- lo-bootstrap --------------------------------------------------------

# Library_lo-bootstrap.mk: sal itself only links against this small helper
# library (Library_sal.mk:39-43, gb_Library_use_libraries) on Android and
# Emscripten. Module_sal.mk's own target list -- `$(if $(filter $(OS),
# ANDROID EMSCRIPTEN),Library_lo-bootstrap)` -- confirms the *target itself*
# is only ever built under those two OSes, so the whole add_library() below
# (not just its sources) is gated the same way, matching gbuild.
if (CMAKE_SYSTEM_NAME STREQUAL "Android" OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(LO_BOOTSTRAP_SOURCES "")

    if (CMAKE_SYSTEM_NAME STREQUAL "Android")
        # Library_lo-bootstrap.mk:13-26. sal/android/libreofficekit-jni.c is a
        # real Android-NDK JNI bridge (<jni.h>, Android framework headers)
        # this build cannot compile or verify: there is no Android NDK/SDK on
        # this machine. Both files are confirmed to exist on disk
        # (sal/android/libreofficekit-jni.c, sal/android/lo-bootstrap.c), but
        # this branch has never actually been built here -- structurally
        # present, unverified/best-effort, not silently dropped.
        list(APPEND LO_BOOTSTRAP_SOURCES
            "${CMAKE_SOURCE_DIR}/sal/android/libreofficekit-jni.c"
            "${CMAKE_SOURCE_DIR}/sal/android/lo-bootstrap.c")
    endif ()
    if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        # Library_lo-bootstrap.mk:29-33.
        list(APPEND LO_BOOTSTRAP_SOURCES
            "${CMAKE_SOURCE_DIR}/sal/emscripten/lo-bootstrap.c")
    endif ()

    add_library(lo-bootstrap SHARED ${LO_BOOTSTRAP_SOURCES})
    target_include_directories(lo-bootstrap PRIVATE
        "${CMAKE_SOURCE_DIR}/sal/inc")

    if (CMAKE_SYSTEM_NAME STREQUAL "Android")
        # Library_lo-bootstrap.mk:16-21: gb_Library_disable_standard_system_
        # libs ("We explicitly *don't* want gb_STDLIBS to be linked here") plus
        # -llog -landroid. gbuild's gb_STDLIBS is its own curated default
        # link set (solenv/gbuild/platform/unxgcc.mk); this CMake port never
        # links a gbuild-style STDLIBS set onto any target to begin with, so
        # there is no equivalent flag here to turn off -- noted as
        # unverified/best-effort (this whole branch has no NDK to test
        # against) rather than modelled with an invented CMake mechanism.
        target_link_libraries(lo-bootstrap PRIVATE log android)
    endif ()
endif ()

# --- libsal ------------------------------------------------------------------

librestudio_expand_cxx(SAL_COMMON_SOURCES)
librestudio_expand_cxx(SAL_UNX_SOURCES)
librestudio_expand_cxx(SAL_TEXTENC_SOURCES)
if (WIN32)
    librestudio_expand_cxx(SAL_W32_SOURCES)
endif ()

set(SAL_INCLUDE_DIRS
    "${CMAKE_SOURCE_DIR}/sal/inc")

# Library_sal.mk:156-247, the `ifneq ($(OS),WNT) ... else ... endif` block:
# assembles the OS-specific half of sal's source list (everything besides
# SAL_COMMON_SOURCES/SAL_TEXTENC_SOURCES, which are platform-independent).
set(SAL_PLATFORM_SOURCES "")

if (WIN32)
    # Library_sal.mk:219-247: replaces the whole Unix source list outright
    # (gbuild does not merge the two). Unverified/best-effort like the rest
    # of the Windows branch in this file -- no MSVC toolchain here to confirm
    # it actually compiles/links (see the SAL_W32_SOURCES comment above).
    list(APPEND SAL_PLATFORM_SOURCES ${SAL_W32_SOURCES})
else ()
    # Library_sal.mk:157-184 (the always-on Unix exception objects) plus
    # :204-208 (sal/osl/unx/backtrace.c, a cobject, added for every non-WNT
    # OS -- this Linux build's existing, already-correct behaviour).
    list(APPEND SAL_PLATFORM_SOURCES
        ${SAL_UNX_SOURCES}
        "${CMAKE_SOURCE_DIR}/sal/osl/unx/backtrace.c")

    # Library_sal.mk:186-197: uunxapi/system compile as Objective-C++ (.mm)
    # specifically on macOS, plain C++ (.cxx) exception objects everywhere
    # else (Linux included -- this is the pre-existing, unchanged behaviour
    # for this build). The .mm siblings (sal/osl/unx/uunxapi.mm, sal/osl/unx/
    # system.mm) already exist on disk and each just `#include`s its own
    # .cxx (confirmed by reading both), so compiling the real .mm files --
    # which CMake recognises as Objective-C++ automatically on Apple
    # platforms -- is sufficient; no set_source_files_properties(...
    # LANGUAGE OBJCXX) trick is needed. Unverified/best-effort: no macOS SDK
    # here to confirm the .mm compile path actually works.
    if (APPLE)
        list(APPEND SAL_PLATFORM_SOURCES
            "${CMAKE_SOURCE_DIR}/sal/osl/unx/uunxapi.mm"
            "${CMAKE_SOURCE_DIR}/sal/osl/unx/system.mm")
    else ()
        list(APPEND SAL_PLATFORM_SOURCES
            "${CMAKE_SOURCE_DIR}/sal/osl/unx/uunxapi.cxx"
            "${CMAKE_SOURCE_DIR}/sal/osl/unx/system.cxx")
    endif ()

    # Library_sal.mk:199-203: osxlocale is macOS- and iOS-only, absent from
    # every other Unix (Linux included -- unchanged here).
    if (APPLE OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
        list(APPEND SAL_PLATFORM_SOURCES
            "${CMAKE_SOURCE_DIR}/sal/osl/unx/osxlocale.cxx")
    endif ()

    # Library_sal.mk:209-217: interlck is a normal exception object
    # everywhere except SPARC/SPARC64, where it is replaced by hand-written
    # assembly (sal/osl/unx/asm/interlck_sparc.s, confirmed to exist on
    # disk). This build's CPU is x86_64 (LIBRESTUDIO_RTL_ARCH in
    # cmake/LibreStudioPlatform.cmake), so this branch never actually
    # executes here -- implemented structurally correct, but genuinely
    # unverified/unverifiable on this machine: no SPARC hardware/toolchain,
    # and enable_language(ASM) + this .s file have never been exercised.
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "^[Ss][Pp][Aa][Rr][Cc](64)?$")
        enable_language(ASM)
        list(APPEND SAL_PLATFORM_SOURCES
            "${CMAKE_SOURCE_DIR}/sal/osl/unx/asm/interlck_sparc.s")
    else ()
        list(APPEND SAL_PLATFORM_SOURCES
            "${CMAKE_SOURCE_DIR}/sal/osl/unx/interlck.cxx")
    endif ()
endif ()

# On this Linux build, SAL_PLATFORM_SOURCES now resolves to exactly
# SAL_UNX_SOURCES + backtrace.c + uunxapi.cxx + system.cxx + interlck.cxx --
# the same set of translation units add_library(sal ...) compiled before this
# platform branching was added (list order differs, which does not affect
# what gets linked into the shared library).
add_library(sal SHARED
    ${SAL_COMMON_SOURCES}
    ${SAL_PLATFORM_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(sal PRIVATE
        "${CMAKE_SOURCE_DIR}/sal/inc/pch/precompiled_sal.hxx")
    target_compile_definitions(sal PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(sal PRIVATE ${SAL_INCLUDE_DIRS})

target_compile_definitions(sal PRIVATE
    SAL_DLLIMPLEMENTATION
    COND_LIB_SAL_TEXTENC
    SYSTEM_ZLIB
    ZLIB_CONST
    # Library_sal.mk:33-35 also has -DRTL_OS/-DRTL_ARCH/-DSRCDIR explicitly
    # for sal. These three are already provided to every target in the tree
    # (sal included) via the directory-scope add_compile_definitions() call
    # in cmake/LibreStudioPlatform.cmake (included from the root
    # CMakeLists.txt before add_subdirectory(sal)), using the very same
    # LIBRESTUDIO_RTL_OS/LIBRESTUDIO_RTL_ARCH values -- so repeating them
    # here is a harmless, value-identical duplicate, not a functional gap.
    # Kept anyway so this file stays self-documenting against Library_sal.mk's
    # own explicit per-library add_defs, and stays correct even if that
    # global default were ever narrowed later to not cover every module.
    RTL_OS="${LIBRESTUDIO_RTL_OS}"
    RTL_ARCH="${LIBRESTUDIO_RTL_ARCH}"
    SRCDIR="${CMAKE_SOURCE_DIR}")

# Library_sal.mk:29-31 -- $(if $(filter iOS,$(OS)),-DNO_CHILD_PROCESSES).
# Structurally present; inert on this Linux build (CMAKE_SYSTEM_NAME is never
# "iOS" here).
if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_compile_definitions(sal PRIVATE NO_CHILD_PROCESSES)
endif ()

# Library_sal.mk:26 -- $(if $(filter FUZZERS,$(BUILD_TYPE)),
# -DFORCE_DEFAULT_SIGNAL): an OSS-Fuzz-style build disables sal's own signal
# handler installation so the fuzzing harness's own handler is the one that
# runs. Sal-specific (nothing else reads it), so it lives here rather than in
# cmake/LibreStudioConfigHeaders.cmake -- there is no generated header to
# substitute it into, just this one compile definition.
option(ENABLE_FUZZERS
    "Build for OSS-Fuzz style fuzzing (matches gbuild's FUZZERS BUILD_TYPE flag)" OFF)
if (ENABLE_FUZZERS)
    target_compile_definitions(sal PRIVATE FORCE_DEFAULT_SIGNAL)
endif ()

target_include_directories(sal PRIVATE
    "${DRAGONBOX_INCLUDE_DIR}"
    "${FAST_FLOAT_INCLUDE_DIR}")

target_link_libraries(sal PRIVATE
    ZLIB::ZLIB
    ${CMAKE_DL_LIBS}
    Threads::Threads)

# Library_sal.mk:39-43 use_libraries: lo-bootstrap only on Android/Emscripten
# (see the lo-bootstrap target above, gated the same way).
if (CMAKE_SYSTEM_NAME STREQUAL "Android" OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_link_libraries(sal PRIVATE lo-bootstrap)
endif ()

# Library_sal.mk:65-78 add_libs. -ldl is already covered above by CMake's own
# portable ${CMAKE_DL_LIBS} (left as-is, not duplicated with a literal -ldl);
# the rest is real per-OS branching, typed out below. $(BACKTRACE_LIBS) is
# deliberately NOT added anywhere in this port: HAVE_FEATURE_BACKTRACE is
# hardcoded to 0 in cmake/LibreStudioConfigHeaders.cmake (the backtrace
# feature is off by default here, matching upstream's own default), and this
# port has no CMake equivalent yet of configure.ac's BACKTRACE_LIBS/
# BACKTRACE_CFLAGS AC_ARG_VAR probing (Library_sal.mk:22's own
# $(BACKTRACE_CFLAGS) -I flag on gb_Library_set_include is, for the same
# reason, likewise not reproduced here). This is a real, separate,
# honestly-flagged gap in this port, not something silently dropped while
# pretending it was handled.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "GNU/kFreeBSD")
    # Library_sal.mk:66-69 -- $(if $(filter LINUX,$(OS)),-ldl -lrt): -ldl is
    # already ${CMAKE_DL_LIBS} above, so only -lrt is new here. kFreeBSD is
    # included in this branch too because configure.ac sets OS=LINUX for it
    # (not a distinct KFREEBSD tag) -- see the "kFreeBSD is the interesting
    # one" comment in cmake/LibreStudioPlatform.cmake -- so
    # $(filter LINUX,$(OS)) genuinely also matches a kFreeBSD build upstream.
    # CMAKE_SYSTEM_NAME "GNU/kFreeBSD" is itself flagged unverified/
    # best-effort in that same file; this branch inherits that same caveat.
    # This restores the previously-unconditional "rt" link to being correctly
    # Linux-scoped (it was wrongly applied to every platform before this
    # change; on this Linux build it is still linked, unchanged).
    target_link_libraries(sal PRIVATE rt)
elseif (CMAKE_SYSTEM_NAME STREQUAL "SunOS")
    # Library_sal.mk:70-73 -- $(if $(filter SOLARIS,$(OS)),-lnsl -lsocket).
    # Unverified/best-effort: CMAKE_SYSTEM_NAME "SunOS" for Solaris and the
    # fact that this tree has no working UNO C++ ABI bridge for Solaris at
    # all are both already flagged in cmake/LibreStudioPlatform.cmake; this
    # branch inherits that same caveat.
    target_link_libraries(sal PRIVATE nsl socket)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Haiku")
    # Library_sal.mk:74-76 -- $(if $(filter HAIKU,$(OS)),-lnetwork).
    target_link_libraries(sal PRIVATE network)
endif ()

# Library_sal.mk:52-63 use_system_win32_libs -- only meaningful under WIN32.
# Unverified/best-effort like the rest of the Windows branch in this file: no
# MSVC toolchain here to confirm these library names actually resolve/link.
if (WIN32)
    target_link_libraries(sal PRIVATE
        advapi32
        comdlg32
        dbghelp
        mpr
        ole32
        shell32
        user32
        userenv
        wer
        ws2_32)
endif ()

# Library_sal.mk:80-86 -- ifeq ($(OS),MACOSX): gb_Library_use_system_darwin_
# frameworks. No existing precedent for Darwin framework linkage was found
# anywhere else in this tree (grepped every CMakeLists.txt for "-framework "
# and find_library(...): the other modules that mention Darwin frameworks in
# a comment, e.g. external/clew and jvmfwk, explain why they drop that branch
# rather than port it), so this uses the standard, idiomatic CMake mechanism
# for linking a macOS framework (target_link_libraries with
# "-framework <Name>") rather than inventing something new. Checked before
# the plain iOS-vs-macOS APPLE test below is even relevant: Library_sal.mk's
# `ifeq ($(OS),MACOSX)` is macOS-only, not iOS, so this is also excluded when
# CMAKE_SYSTEM_NAME is "iOS" (APPLE is TRUE for both) -- mirroring the same
# "iOS checked before generic APPLE" ordering cmake/LibreStudioPlatform.cmake
# already uses for LIBRESTUDIO_CPPU_ENV/RTL_OS. Unverified/best-effort: no
# macOS SDK on this machine to actually build this branch.
if (APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_link_libraries(sal PRIVATE
        "-framework CoreFoundation"
        "-framework Foundation")

    # Library_sal.mk:84 -- $(if $(ENABLE_MACOSX_SANDBOX),Security). Sal-
    # specific option, matching this file's own ENABLE_FUZZERS/ENABLE_CIPHER_
    # OPENSSL_BACKEND convention of a plain local option() (there is no
    # generated header to substitute this into, only a link decision).
    # Independent from, and does not touch, the pre-existing, unrelated
    # HAVE_FEATURE_MACOSX_SANDBOX=0 hardcoded default in cmake/config/
    # config_features.h.in; wiring that generated define up to this option is
    # separate, not-yet-done work, in the same category as the many other
    # not-yet-wired ENABLE_* toggles already noted elsewhere in this file.
    option(ENABLE_MACOSX_SANDBOX
        "Build for the macOS App Sandbox (matches gbuild's ENABLE_MACOSX_SANDBOX)" OFF)
    if (ENABLE_MACOSX_SANDBOX)
        target_link_libraries(sal PRIVATE "-framework Security")
    endif ()
endif ()

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

# Executable_osl_process_child.mk says explicitly it "is required only by
# sal_osl_process test" (sal/qa/osl/process/osl_process.cxx, ctor of
# Test_osl_executeProcess -- it builds suExecutableFileURL as
# getExecutablePath() + "/osl_process_child"): the shipped sal library itself
# never spawns it; this is a CppunitTest helper only, not a runtime
# dependency of osl. Kept as an unconditional module target here (not folded
# into the CppunitTest section below) because Module_sal.mk itself adds it
# unconditionally once $(BUILD_TYPE) contains DESKTOP and $(OS) isn't
# EMSCRIPTEN -- this port's established precedent (see e.g.
# svl/CMakeLists.txt, shell/CMakeLists.txt, desktop/CMakeLists.txt) is to
# treat DESKTOP as the always-true baseline for this Linux desktop build, and
# EMSCRIPTEN is never this build's OS.
add_executable(osl_process_child
    "${CMAKE_SOURCE_DIR}/sal/qa/osl/process/osl_process_child.cxx")
target_include_directories(osl_process_child PRIVATE ${SAL_INCLUDE_DIRS})
target_link_libraries(osl_process_child PRIVATE sal)

# osl_execProc_test_batch, the same CppunitTest's same source file
# (osl_process.cxx, ~line 387), spawns this script directly by path, next to
# osl_process_child -- Executable_osl_process_child.mk delivers it there via
# gb_Deliver_deliver. Both osl_process_child and every CppunitTest's shared
# library land in the same LIBRESTUDIO_PROGRAM_DIR (see
# CMAKE_RUNTIME_OUTPUT_DIRECTORY / CMAKE_LIBRARY_OUTPUT_DIRECTORY in
# cmake/LibreStudioPlatform.cmake -- "Match gbuild: libraries and executables
# all land in one directory"), so copying it there once is enough for the
# test's suCWD + "/batch.sh" (or ".bat" on Windows) lookup to find it.
if (WIN32)
    set(SAL_OSL_PROCESS_BATCH_FILE "batch.bat")
else ()
    set(SAL_OSL_PROCESS_BATCH_FILE "batch.sh")
endif ()

add_custom_command(
    OUTPUT "${LIBRESTUDIO_PROGRAM_DIR}/${SAL_OSL_PROCESS_BATCH_FILE}"
    COMMAND "${CMAKE_COMMAND}" -E copy
            "${CMAKE_SOURCE_DIR}/sal/qa/osl/process/${SAL_OSL_PROCESS_BATCH_FILE}"
            "${LIBRESTUDIO_PROGRAM_DIR}/${SAL_OSL_PROCESS_BATCH_FILE}"
    DEPENDS "${CMAKE_SOURCE_DIR}/sal/qa/osl/process/${SAL_OSL_PROCESS_BATCH_FILE}"
    COMMENT "Deliver sal/qa/osl/process/${SAL_OSL_PROCESS_BATCH_FILE}"
    VERBATIM)
add_custom_target(sal_osl_process_batch ALL
    DEPENDS "${LIBRESTUDIO_PROGRAM_DIR}/${SAL_OSL_PROCESS_BATCH_FILE}")
add_dependencies(osl_process_child sal_osl_process_batch)

# --- cppunittester ------------------------------------------------------------

# The one runner shared by every CppunitTest target across the whole tree
# (see cmake/LibreStudioTest.cmake's header comment for the full mechanism).
# Executable_cppunittester.mk's externals are boost_headers
# (boost/algorithm/string.hpp, used to split CPPUNIT_TEST_NAME) and cppunit
# (CppUnit::PlugInManager, TestRunner, ...). Boost::boost is already resolved
# by the time sal is reached (root CMakeLists.txt, find_package(Boost ...)
# before add_subdirectory(sal)); cppunit is resolved the same way unotest/
# CMakeLists.txt and test/CMakeLists.txt do it (pkg_check_modules ...
# IMPORTED_TARGET cppunit) -- each directory has to call it again, since an
# IMPORTED_TARGET from pkg_check_modules is only visible in the directory it
# was created in and below (see test/CMakeLists.txt's own comment on this).
#
# MSVC-specific bits of Executable_cppunittester.mk (linking dbghelp when
# MSVC_USE_DEBUG_RUNTIME is set, /STACK:10000000 under $(COM)=MSC) are not
# modelled here: this port has no MSVC toolchain to build or verify against
# (see this project's own Windows/MSVC research), so guessing at the
# corresponding CMake would be undocumented, untested behaviour rather than a
# faithful port. Left as a known gap for whoever eventually brings up the
# Windows/MSVC side, rather than silently pretended-away.
pkg_check_modules(CPPUNIT REQUIRED IMPORTED_TARGET cppunit)

add_executable(cppunittester
    "${CMAKE_SOURCE_DIR}/sal/cppunittester/cppunittester.cxx")
target_include_directories(cppunittester PRIVATE ${SAL_INCLUDE_DIRS})
target_link_libraries(cppunittester PRIVATE
    sal
    PkgConfig::CPPUNIT
    Boost::boost)

# --- CppunitTest targets -------------------------------------------------------
#
# sal's 8 CppunitTest_*.mk files, ported via librestudio_add_cppunit_test()
# (cmake/LibreStudioTest.cmake -- read its header comment first for the
# shared rationale: shared-library-per-test, the cppunittester runner above,
# why no protector plug-ins are implemented yet, and the minimal
# UserInstallation handling). Each of the 8 was checked individually for
# gb_CppunitTest_use_ure / gb_CppunitTest_use_vcl* calls -- none of them make
# either call, confirming no protector plug-in is needed at this layer.
#
# CompilerTest_sal_rtl_oustring (Module_sal.mk, gated on $(COM_IS_CLANG) and
# NOT $(COMPILER_EXTERNAL_TOOL)/$(COMPILER_PLUGIN_TOOL)) is a Clang-plugin
# compile-time check, not a CppunitTest -- architecturally a static-analysis
# pass over other translation units at compile time, not a runtime test.  It
# needs a Clang plugin build mechanism (compiling and loading a
# libclang-based plugin into the compiler itself) this port does not have at
# all, so it is out of scope here and deliberately not ported by this
# function or anything else in this file.
#
# The "valgrind" external CppunitTest_sal_osl.mk uses (gb_LinkTarget__use_
# valgrind, RepositoryExternal.mk) only adds -DHAVE_VALGRIND_HEADERS and a
# -I flag when $(ENABLE_VALGRIND)=TRUE, which is not this build's default and
# has no CMake option yet in this port (same category as the many other
# not-yet-wired ENABLE_* toggles across the tree) -- so it is a no-op here,
# matching upstream's own default, not a silent gap.

# CppunitTest_Module_DLL.mk: "This is not really a test. Only a dynamic
# library which is loaded by sal_osl_module unit test library." --
# sal/qa/osl/module/osl_Module.cxx's getDllURL() dlopens it directly next to
# itself, by the exact filename librestudio_add_cppunit_test() already
# produces (see that function's OUTPUT_NAME comment).
#
# gb_CppunitTest_set_visibility_default(Module_DLL) is not modelled: it
# counteracts gbuild's own -fvisibility=hidden default, but nothing in this
# CMake port sets -fvisibility=hidden anywhere (grepped for -fvisibility
# across every CMakeLists.txt/*.cmake file in the tree: no hits), so there is
# nothing to counteract here -- symbols are already default-visibility.
librestudio_add_cppunit_test(Module_DLL
    SOURCES
        sal/qa/osl/module/osl_Module_DLL
    LIBRARIES sal)

# CppunitTest_sal_osl_security.mk
librestudio_add_cppunit_test(sal_osl_security
    SOURCES
        sal/qa/osl/security/osl_Security
    LIBRARIES sal)

# CppunitTest_sal_osl.mk. Two build-order-only (not link) dependencies:
#   - Module_DLL, dlopened at run time by the osl_Module test above;
#   - osl_process_child, spawned at run time by the osl_process test (and its
#     batch-file sibling target, transitively, via add_dependencies above).
# sal/qa/osl/file/osl_File.cxx genuinely uses tools/urlobj.hxx's
# INetURLObject, hence the real (not build-order-only) "tl" library
# dependency, exactly as CppunitTest_sal_osl.mk's own use_libraries says --
# verified via `grep -n "add_library(tl " tools/CMakeLists.txt`. tools/ is
# add_subdirectory()'d well after sal/ in the root CMakeLists.txt, but CMake
# resolves target_link_libraries() link-item target names at generate time,
# not at the time of this call, so linking a not-yet-defined target here is
# fine as long as it exists by the end of the configure step (confirmed
# against a standalone throwaway CMake project before relying on it here).
librestudio_add_cppunit_test(sal_osl
    SOURCES
        sal/qa/osl/condition/osl_Condition
        sal/qa/osl/file/osl_File
        sal/qa/osl/file/osl_old_test_file
        sal/qa/osl/file/test_cpy_wrt_file
        sal/qa/osl/getsystempathfromfileurl/test-getsystempathfromfileurl
        sal/qa/osl/module/osl_Module
        sal/qa/osl/mutex/osl_Mutex
        sal/qa/osl/pipe/osl_Pipe
        sal/qa/osl/process/osl_process
        sal/qa/osl/process/osl_Thread
        sal/qa/osl/setthreadname/test-setthreadname
        sal/qa/osl/socket
    LIBRARIES sal tl
    DEPENDS Module_DLL osl_process_child)

# CppunitTest_sal_osl_file_details.mk. Unlike every other test here, this one
# uses gb_CppunitTest_use_library_objects(sal_osl_file_details,sal) rather
# than gb_CppunitTest_use_libraries: sal/qa/osl/file/forbidden.cxx calls
# ::isForbidden() (declared in sal/inc/setallowedpaths.hxx, defined in
# sal/osl/unx/file.cxx) and includes "../../../osl/unx/file_impl.hxx"
# directly -- both are internal to sal, not part of its public ABI (not
# listed in sal/util/sal.map), so they are not reachable by linking against
# the sal *library*. gbuild's use_library_objects reuses sal's already-
# compiled .o files directly inside the CppunitTest binary instead of linking
# libsal.so; this port gets the same effect (the internal symbols end up
# present in this test's own binary) by recompiling the same source lists
# (SAL_COMMON_SOURCES, SAL_PLATFORM_SOURCES -- exactly what add_library(sal
# ...) above compiles, now that SAL_PLATFORM_SOURCES carries the same
# per-platform branching as the production target, see above) directly into
# this MODULE, rather than by restructuring the already-working production
# `sal` target into a shared OBJECT library, which would be a materially
# larger and riskier change than this one CppunitTest justifies. This also
# explains why CppunitTest_sal_osl_file_details.mk repeats "-ldl -lrt" in its
# own add_libs instead of just linking libsal.so (which already carries those
# transitively): it is not linking against sal at all, it is rebuilding it.
# SAL_COMMON_SOURCES includes sal/rtl/math, which needs DRAGONBOX_INCLUDE_DIR
# / FAST_FLOAT_INCLUDE_DIR the same way add_library(sal ...) above does --
# recompiling "all of sal" means recompiling that too, so it needs the same
# include dirs sal itself is given for it, not just sal/inc.
#
# LIBRARIES below still hardcodes "rt" unconditionally (pre-existing, not
# introduced by this change). CppunitTest_sal_osl_file_details.mk's own
# add_libs actually has the exact same $(if $(filter LINUX,$(OS)),-ldl -lrt)
# / SOLARIS -lnsl -lsocket / HAIKU -lnetwork branching as Library_sal.mk's
# add_libs does (verified by reading that .mk directly, not assumed), plus
# the same MACOSX darwin-frameworks and WIN32 system-libs blocks as sal
# itself -- so in principle this test deserves the identical treatment just
# applied to the `sal` target above. This task's scope, per its own brief, is
# specifically sal's own library target ("which .cxx/.c/.mm/.asm files
# actually go into the sal target, and which link libraries/frameworks it
# needs"); extending every CppunitTest in this file to the same platform
# abstraction is real, additional work left undone here on purpose, not
# silently pretended to already match the .mk -- this comment exists so that
# gap is visible rather than hidden behind an inaccurate description of what
# the .mk does.
librestudio_add_cppunit_test(sal_osl_file_details
    SOURCES
        ${SAL_COMMON_SOURCES}
        ${SAL_PLATFORM_SOURCES}
        sal/qa/osl/file/forbidden
    LIBRARIES ZLIB::ZLIB ${CMAKE_DL_LIBS} rt Threads::Threads
    INCLUDE ${SAL_INCLUDE_DIRS} "${DRAGONBOX_INCLUDE_DIR}" "${FAST_FLOAT_INCLUDE_DIR}"
    DEFINITIONS SAL_DLLIMPLEMENTATION COND_LIB_SAL_TEXTENC SYSTEM_ZLIB ZLIB_CONST)

# CppunitTest_sal_rtl.mk. Options mirror gbuild's own $(ENABLE_CIPHER_OPENSSL_
# BACKEND) / $(DISABLE_NAN_TESTS) toggles 1:1 (same reasoning as ENABLE_FUZZERS
# above: sal-specific, no generated header involved, so a plain local option()
# here rather than anything in cmake/LibreStudioConfigHeaders.cmake).
option(ENABLE_CIPHER_OPENSSL_BACKEND
    "OpenSSL-backed rtl::Cipher implementation (matches gbuild's ENABLE_CIPHER_OPENSSL_BACKEND)" OFF)
option(DISABLE_NAN_TESTS
    "Skip sal_rtl's test_payloadNaN (matches gbuild's DISABLE_NAN_TESTS, for architectures without full IEEE754 NaN payload support)" OFF)

# Library_sal.mk:249-255 -- ifeq ($(ENABLE_CIPHER_OPENSSL_BACKEND),TRUE): this
# applies to the *main* sal library itself, not just the sal_rtl CppunitTest
# below (whose own SAL_RTL_TEST_DEFS use of the same option, a few lines
# down, is separate and additional -- the test needs the define on its own
# translation units too since it is a fresh CppunitTest binary, not merely a
# consumer of libsal.so). pkg_check_modules(...IMPORTED_TARGET openssl) here
# mirrors the same externals/openssl resolution comphelper/CMakeLists.txt and
# svl/CMakeLists.txt already use for their own OPENSSL_BACKEND branches
# (grepped for precedent before adding this). openssl_headers is not a
# separate pkg-config module upstream provides -- the single "openssl"
# module already supplies both headers and libs, exactly as those two
# existing call sites already assume.
if (ENABLE_CIPHER_OPENSSL_BACKEND)
    target_compile_definitions(sal PRIVATE LIBO_CIPHER_OPENSSL_BACKEND)
    pkg_check_modules(OPENSSL REQUIRED IMPORTED_TARGET openssl)
    target_link_libraries(sal PRIVATE PkgConfig::OPENSSL)
endif ()

set(SAL_RTL_TEST_DEFS "")
if (ENABLE_CIPHER_OPENSSL_BACKEND)
    list(APPEND SAL_RTL_TEST_DEFS LIBO_CIPHER_OPENSSL_BACKEND)
endif ()
if (DISABLE_NAN_TESTS)
    list(APPEND SAL_RTL_TEST_DEFS DISABLE_NAN_TESTS)
endif ()

# sal_textenc dependency: "The test uses O(U)String capabilities that dlopen
# this lib" (CppunitTest_sal_rtl.mk) -- build-order-only, matching gbuild's
# own plain target-level prerequisite there (not a use_libraries call).
librestudio_add_cppunit_test(sal_rtl
    SOURCES
        sal/qa/ByteSequence/ByteSequence
        sal/qa/OStringBuffer/rtl_OStringBuffer
        sal/qa/rtl/alloc/rtl_alloc
        sal/qa/rtl/bootstrap/expand
        sal/qa/rtl/cipher/rtl_cipher
        sal/qa/rtl/crc32/rtl_crc32
        sal/qa/rtl/digest/rtl_digest
        sal/qa/rtl/doublelock/rtl_doublelocking
        sal/qa/rtl/locale/rtl_locale
        sal/qa/rtl/math/test-rtl-math
        sal/qa/rtl/math/test-std-math
        sal/qa/rtl/ostring/rtl_str
        sal/qa/rtl/oustring/rtl_ustr
        sal/qa/rtl/oustringbuffer/test_oustringbuffer_appendchar
        sal/qa/rtl/oustringbuffer/test_oustringbuffer_appenduninitialized
        sal/qa/rtl/oustringbuffer/test_oustringbuffer_assign
        sal/qa/rtl/oustringbuffer/test_oustringbuffer_tostring
        sal/qa/rtl/oustringbuffer/test_oustringbuffer_utf32
        sal/qa/rtl/oustring/rtl_OUString2
        sal/qa/rtl/process/rtl_Process
        sal/qa/rtl/random/rtl_random
        sal/qa/rtl/ref/rtl_ref
        sal/qa/rtl/strings/nonconstarray
        sal/qa/rtl/strings/test_strings_replace
        sal/qa/rtl/strings/test_strings_defaultstringview
        sal/qa/rtl/strings/test_ostring
        sal/qa/rtl/strings/test_ostring_concat
        sal/qa/rtl/strings/test_ostring_stringliterals
        sal/qa/rtl/strings/test_ostringbuffer
        sal/qa/rtl/strings/test_oustring_compare
        sal/qa/rtl/strings/test_oustring_concat
        sal/qa/rtl/strings/test_oustring_convert
        sal/qa/rtl/strings/test_oustring_endswith
        sal/qa/rtl/strings/test_oustring_startswith
        sal/qa/rtl/strings/test_oustring_stringliterals
        sal/qa/rtl/strings/test_strings_toint
        sal/qa/rtl/strings/test_strings_valuex
        sal/qa/rtl/textenc/rtl_tencinfo
        sal/qa/rtl/textenc/rtl_textcvt
        sal/qa/rtl/uri/rtl_Uri
        sal/qa/rtl/uri/rtl_testuri
        sal/qa/rtl/uuid/rtl_Uuid
    LIBRARIES sal
    INCLUDE "${CMAKE_SOURCE_DIR}/sal/qa/inc"
    DEFINITIONS ${SAL_RTL_TEST_DEFS}
    DEPENDS sal_textenc)

# CppunitTest_sal_types.mk
librestudio_add_cppunit_test(sal_types
    SOURCES
        sal/qa/sal/test_types
    LIBRARIES sal)

# CppunitTest_sal_comtools.mk / CppunitTest_sal_retry_if_failed.mk: both
# $(filter WNT,$(OS))-gated in Module_sal.mk (Windows-only COM-interop and
# RetryIfFailed helper tests). Kept structurally present, behind if (WIN32),
# per this port's policy of not silently dropping Windows-only gbuild targets
# -- see this project's own Windows/MSVC research on why they cannot actually
# be built or exercised on this machine (no cl.exe/ml64.exe/SDK). Inert on
# this Linux build: the if (WIN32) block below never executes here.
if (WIN32)
    # ole32: gb_CppunitTest_use_system_win32_libs(sal_comtools, ole32).
    librestudio_add_cppunit_test(sal_comtools
        SOURCES
            sal/qa/systools/test_comtools
        LIBRARIES sal ole32)

    librestudio_add_cppunit_test(sal_retry_if_failed
        SOURCES
            sal/qa/systools/test_retry_if_failed)
endif ()

# --- install -----------------------------------------------------------------

install(TARGETS sal sal_textenc osl_process_child cppunittester
    LIBRARY DESTINATION program
    RUNTIME DESTINATION program)
