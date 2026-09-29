# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# bridges — the UNO C++ bridge: the generated machinery that maps C++ virtual
# calls onto the UNO binary ABI and back, plus the shared plumbing both
# directions use.
#
# This is the most delicate library in the build.  It reaches into the C++ ABI
# (each arch's own abi.cxx/rtti.cxx), it unwinds through a call that the
# compiler must not assume cannot throw, and part of it is hand-written
# assembly.  Upstream therefore compiles it with a specific set of per-file
# flags (Library_cpp_uno.mk); reproducing them is not optional — a uniform
# flag set produces a bridge that crashes rather than one that fails to build.
#
# 2026-09-28: this file used to hardcode exactly one bridge variant
# (gcc3_linux_x86-64) with no conditional structure at all. bridges/
# Library_cpp_uno.mk's own real `ifeq ($(CPUNAME),...)` chain (307 lines,
# read in full) has 21 CPU/OS arms resolving to 16 distinct bridge source
# directories, every one of which already exists in this tree (bridges/
# source/cpp_uno/gcc3_linux_arm, gcc3_linux_aarch64, gcc3_ios, gcc3_linux_
# alpha, gcc3_linux_hppa, gcc3_linux_ia64, gcc3_linux_intel, gcc3_linux_m68k,
# gcc3_linux_mips(64), gcc3_linux_loongarch64, gcc3_linux_riscv64, gcc3_linux_
# powerpc(64), gcc3_linux_s390x, gcc3_linux_sparc(64), gcc3_linux_x86-64,
# gcc3_macosx_x86-64, msvc_win32_arm64/intel/x86-64, msvc_shared) — this file
# now reproduces that whole chain, keyed off LIBRESTUDIO_CPUNAME (cmake/
# LibreStudioPlatform.cmake) and CMAKE_SYSTEM_NAME/APPLE/WIN32, exactly the
# same "real if/elseif pointing at already-checked-in sources, inert but
# structurally present on this Linux x86_64 machine" treatment already
# established for apple_remote/embedserv/winaccessibility/cpputools's own
# sp2bv. Doing this needed lifting the separate, pre-existing `if (NOT
# CMAKE_SIZEOF_VOID_P EQUAL 8) message(FATAL_ERROR ...)` gate in
# LibreStudioPlatform.cmake (a deliberate, explicit, user-approved decision
# that reaches beyond bridges alone -- see that gate's own comment for why it
# does NOT mean the rest of this ~7-million-line port has been audited for
# 32-bit correctness, only that bridge CPU selection now is).
#
# One real, deeper gap deliberately NOT implemented here: gcc3_wasm
# (CPUNAME=INTEL, OS=Emscripten) needs `wasmbridgegen`, a whole separate,
# not-yet-ported code-generator executable (bridges/Executable_
# wasmbridgegen.mk) that bridges/CustomTarget_gcc3_wasm.mk runs to produce
# its generated .cxx/.s/exports -- WASM is a fundamentally different build
# mode (no native compiler, no Emscripten SDK configured in this port at
# all), not a mechanical file-list-selection case like every other arm here.
# The branch below is real (LIBRESTUDIO_CPUNAME/CMAKE_SYSTEM_NAME actually
# match it when applicable) but its body is an explicit, loud "not
# implemented" message, not a silent no-op.
#
# Only the C++ bridge is built.  The .NET bridge (net_uno) is behind
# ENABLE_DOTNET upstream and is not enabled here.
#
# 2026-09-28: the Java UNO bridge itself (Jar_java_uno + Library_java_uno) is
# now ported -- see the `if (LIBRESTUDIO_ENABLE_JAVA)` block below and
# Jar_java_uno_deferred.cmake. Package_jnilib_java_uno remains unported,
# deliberately: it is MACOSX-only (a plain .jnilib->.so symlink upstream's own
# Module_bridges.mk gates on `$(if $(filter MACOSX,$(OS)),...)`), same
# platform-scope decision already made for this same module's own
# Library_net_uno/CustomTarget_gcc3_linux_arm's own gb_Executable (there isn't
# one; see the armhelper.o handling below)/CustomTarget_gcc3_wasm.
#
# Porting the bridge does NOT by itself wire up any of its consumers --
# that is separate, still-undone work, each already documented in its own
# owning CMakeLists.txt with its own specific reason for staying unwired for
# now (not "java_uno doesn't exist" anymore, but a real remaining blocker in
# each case):
#   - scripting_beanshell (scripting/CMakeLists.txt): the CppunitTest itself
#     calls real UNO methods on a Java-implemented object -- registering it
#     now would need the bridge to be exercised end-to-end for the first
#     time, not attempted in this pass.
#   - wizards' commonwizards/form/query/report/reportbuilder/table Jars
#     (wizards/CMakeLists.txt): pure javac-classpath consumers of
#     java_uno.jar, cheap to wire, just not done yet.
#   - nlpsolver's Jar_nlpsolver (nlpsolver/CMakeLists.txt): same as wizards'
#     Jars, not done yet.
#   - qadevOOo (qadevOOo/CMakeLists.txt): blocked on this AND, independently,
#     on the absent JUnit-runner category -- porting java_uno alone does not
#     unblock it.

# Flags for the whole library (gb_Library_add_cxxflags in the .mk): the frame
# pointer is needed to walk the stack during unwinding, strict aliasing
# miscompiles the ABI poking, LTO breaks it outright, and AVX registers are not
# part of the UNO calling convention. HAVE_GCC_AVX is a real compiler-flag
# check now (cmake/LibreStudioPlatform.cmake's own LIBRESTUDIO_HAVE_GCC_AVX),
# not assumed off.
set(BRIDGES_CXXFLAGS
    -fno-omit-frame-pointer
    -fno-strict-aliasing
    -fno-lto)
if (LIBRESTUDIO_HAVE_GCC_AVX)
    list(APPEND BRIDGES_CXXFLAGS -mno-avx)
endif ()

# gb_COMPILERNOOPTFLAGS from solenv/gbuild/platform/com_GCC_defs.mk.
set(BRIDGES_NOOPT_FLAGS -O0 -Wp,-U_FORTIFY_SOURCE -fstrict-aliasing -fstrict-overflow)

# bridges_NON_CALL_EXCEPTIONS_FLAGS: callvirtualmethod must be able to catch a
# signal raised by the call it makes, which only works if the compiler is told
# the call can throw. Library_cpp_uno.mk only sets this under `ifeq
# ($(COM),GCC)` and `ifneq ($(COM_IS_CLANG),TRUE)` -- Clang's own
# -fnon-call-exceptions support differs, so this matches real GCC (not Clang,
# not MSVC) exactly.
set(BRIDGES_NON_CALL_EXCEPTIONS_FLAGS "")
if (NOT MSVC AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(BRIDGES_NON_CALL_EXCEPTIONS_FLAGS -fnon-call-exceptions)
endif ()

# --- Bridge selection: mirrors bridges/Library_cpp_uno.mk's own
# `ifeq ($(CPUNAME),...)` chain exactly, arm for arm. Two shared OS-filter
# lists cover most arms (the exact $(filter ...) lists from the real .mk).
set(_bridges_bsd_linux_os "Android;DragonFly;FreeBSD;Linux;NetBSD;OpenBSD")
set(_bridges_bsd_linux_haiku_os "Android;DragonFly;FreeBSD;Linux;NetBSD;OpenBSD;Haiku")

set(BRIDGES_SELECTED_BRIDGE "")
set(BRIDGE_EXCEPTION_OBJECTS "")
set(BRIDGE_NOOPT_OBJECTS "")
set(BRIDGE_ASM_OBJECTS "")
set(BRIDGE_NONCALLEXCEPTION_OBJECTS "")
set(BRIDGE_NONCALLEXCEPTION_NOOPT_OBJECTS "")
set(BRIDGE_EXTRA_OFFAPI FALSE)
set(BRIDGE_ARM_HELPER FALSE)
set(BRIDGE_AARCH64_CALLVIRTUALFUNCTION FALSE)
set(BRIDGE_NOT_IMPLEMENTED "")

if (LIBRESTUDIO_CPUNAME STREQUAL "ARM")
    if (CMAKE_SYSTEM_NAME IN_LIST _bridges_bsd_linux_os)
        set(BRIDGES_SELECTED_BRIDGE "gcc3_linux_arm")
        set(BRIDGE_NOOPT_OBJECTS cpp2uno.cxx except.cxx uno2cpp.cxx)
        set(BRIDGE_ARM_HELPER TRUE)
    endif ()

elseif (LIBRESTUDIO_CPUNAME STREQUAL "AARCH64")
    if (CMAKE_SYSTEM_NAME IN_LIST _bridges_bsd_linux_os OR (APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS"))
        # Real macOS (Apple Silicon) shares this same gcc3_linux_aarch64
        # bridge, unlike X86_64 below (which has its own gcc3_macosx_x86-64
        # directory) -- confirmed directly against the real .mk's own OS
        # filter list, which names MACOSX here, not a separate arm.
        set(BRIDGES_SELECTED_BRIDGE "gcc3_linux_aarch64")
        set(BRIDGE_ASM_OBJECTS vtableslotcall.s)
        set(BRIDGE_EXCEPTION_OBJECTS abi.cxx cpp2uno.cxx uno2cpp.cxx)
        set(BRIDGE_AARCH64_CALLVIRTUALFUNCTION TRUE)
        if (APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
            set(BRIDGE_EXTRA_OFFAPI TRUE)
        endif ()
    elseif (CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(BRIDGES_SELECTED_BRIDGE "gcc3_ios")
        set(BRIDGE_NOOPT_OBJECTS cpp2uno.cxx except.cxx uno2cpp.cxx)
        set(BRIDGE_ASM_OBJECTS ios64_helper.s)
    elseif (WIN32 AND MSVC)
        set(BRIDGES_SELECTED_BRIDGE "msvc_win32_arm64")
        set(BRIDGE_EXCEPTION_OBJECTS cpp2uno.cxx uno2cpp.cxx abi.cxx)
        set(BRIDGE_NOOPT_OBJECTS except.cxx)
        set(BRIDGE_ASM_OBJECTS callvirtualfunction.S vtableslotcall.S)
    endif ()

elseif (LIBRESTUDIO_CPUNAME STREQUAL "AXP")
    if (CMAKE_SYSTEM_NAME IN_LIST _bridges_bsd_linux_os)
        set(BRIDGES_SELECTED_BRIDGE "gcc3_linux_alpha")
        set(BRIDGE_EXCEPTION_OBJECTS cpp2uno.cxx except.cxx uno2cpp.cxx)
    endif ()

elseif (LIBRESTUDIO_CPUNAME STREQUAL "HPPA")
    if (CMAKE_SYSTEM_NAME IN_LIST _bridges_bsd_linux_os)
        set(BRIDGES_SELECTED_BRIDGE "gcc3_linux_hppa")
        set(BRIDGE_NOOPT_OBJECTS call.cxx cpp2uno.cxx except.cxx uno2cpp.cxx)
    endif ()

elseif (LIBRESTUDIO_CPUNAME STREQUAL "IA64")
    if (CMAKE_SYSTEM_NAME IN_LIST _bridges_bsd_linux_os)
        set(BRIDGES_SELECTED_BRIDGE "gcc3_linux_ia64")
        set(BRIDGE_ASM_OBJECTS call.s)
        set(BRIDGE_EXCEPTION_OBJECTS except.cxx)
        set(BRIDGE_NOOPT_OBJECTS cpp2uno.cxx uno2cpp.cxx)
    endif ()

elseif (LIBRESTUDIO_CPUNAME STREQUAL "INTEL")
    if (CMAKE_SYSTEM_NAME IN_LIST _bridges_bsd_linux_haiku_os)
        set(BRIDGES_SELECTED_BRIDGE "gcc3_linux_intel")
        set(BRIDGE_ASM_OBJECTS call.s)
        set(BRIDGE_EXCEPTION_OBJECTS cpp2uno.cxx except.cxx uno2cpp.cxx)
        set(BRIDGE_NONCALLEXCEPTION_OBJECTS callvirtualmethod.cxx)
    elseif (WIN32 AND MSVC)
        set(BRIDGES_SELECTED_BRIDGE "msvc_win32_intel")
        set(BRIDGE_EXCEPTION_OBJECTS cpp2uno.cxx uno2cpp.cxx)
        set(BRIDGE_NOOPT_OBJECTS except.cxx)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        set(BRIDGE_NOT_IMPLEMENTED
            "gcc3_wasm needs wasmbridgegen (bridges/Executable_wasmbridgegen.mk), a not-yet-ported code-generator executable, plus bridges/CustomTarget_gcc3_wasm.mk's own generated .cxx/.s/exports -- a separate, deeper gap (WASM is not a native compile), not implemented here.")
    endif ()

elseif (LIBRESTUDIO_CPUNAME STREQUAL "M68K")
    if (CMAKE_SYSTEM_NAME IN_LIST _bridges_bsd_linux_os)
        set(BRIDGES_SELECTED_BRIDGE "gcc3_linux_m68k")
        set(BRIDGE_NOOPT_OBJECTS cpp2uno.cxx except.cxx uno2cpp.cxx)
    endif ()

elseif (LIBRESTUDIO_CPUNAME STREQUAL "MIPS")
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(BRIDGES_SELECTED_BRIDGE "gcc3_linux_mips")
        set(BRIDGE_NOOPT_OBJECTS cpp2uno.cxx uno2cpp.cxx)
        set(BRIDGE_EXCEPTION_OBJECTS except.cxx)
    endif ()

elseif (LIBRESTUDIO_CPUNAME STREQUAL "MIPS64")
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(BRIDGES_SELECTED_BRIDGE "gcc3_linux_mips64")
        set(BRIDGE_ASM_OBJECTS call.s)
        set(BRIDGE_NOOPT_OBJECTS cpp2uno.cxx uno2cpp.cxx)
        set(BRIDGE_EXCEPTION_OBJECTS except.cxx)
    endif ()

elseif (LIBRESTUDIO_CPUNAME STREQUAL "LOONGARCH64")
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(BRIDGES_SELECTED_BRIDGE "gcc3_linux_loongarch64")
        set(BRIDGE_ASM_OBJECTS call.s)
        set(BRIDGE_NOOPT_OBJECTS abi.cxx cpp2uno.cxx uno2cpp.cxx)
        set(BRIDGE_EXCEPTION_OBJECTS except.cxx)
    endif ()

elseif (LIBRESTUDIO_CPUNAME STREQUAL "RISCV64")
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(BRIDGES_SELECTED_BRIDGE "gcc3_linux_riscv64")
        set(BRIDGE_ASM_OBJECTS call.s)
        set(BRIDGE_NOOPT_OBJECTS abi.cxx cpp2uno.cxx uno2cpp.cxx)
        set(BRIDGE_EXCEPTION_OBJECTS except.cxx)
    endif ()

elseif (LIBRESTUDIO_CPUNAME STREQUAL "POWERPC")
    if (CMAKE_SYSTEM_NAME IN_LIST _bridges_bsd_linux_os)
        set(BRIDGES_SELECTED_BRIDGE "gcc3_linux_powerpc")
        set(BRIDGE_NOOPT_OBJECTS uno2cpp.cxx)
        set(BRIDGE_EXCEPTION_OBJECTS cpp2uno.cxx except.cxx)
    endif ()

elseif (LIBRESTUDIO_CPUNAME STREQUAL "POWERPC64")
    if (CMAKE_SYSTEM_NAME IN_LIST _bridges_bsd_linux_os)
        set(BRIDGES_SELECTED_BRIDGE "gcc3_linux_powerpc64")
        set(BRIDGE_NOOPT_OBJECTS cpp2uno.cxx uno2cpp.cxx)
        set(BRIDGE_EXCEPTION_OBJECTS except.cxx)
    endif ()

elseif (LIBRESTUDIO_CPUNAME STREQUAL "S390X")
    if (CMAKE_SYSTEM_NAME IN_LIST _bridges_bsd_linux_os)
        set(BRIDGES_SELECTED_BRIDGE "gcc3_linux_s390x")
        set(BRIDGE_EXCEPTION_OBJECTS cpp2uno.cxx except.cxx uno2cpp.cxx)
    endif ()

elseif (LIBRESTUDIO_CPUNAME STREQUAL "SPARC")
    if (CMAKE_SYSTEM_NAME IN_LIST _bridges_bsd_linux_os)
        set(BRIDGES_SELECTED_BRIDGE "gcc3_linux_sparc")
        set(BRIDGE_ASM_OBJECTS call.s)
        set(BRIDGE_NOOPT_OBJECTS except.cxx)
        set(BRIDGE_EXCEPTION_OBJECTS cpp2uno.cxx uno2cpp.cxx)
    endif ()

elseif (LIBRESTUDIO_CPUNAME STREQUAL "SPARC64" AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # bridges/Library_cpp_uno.mk special-cases this one as `$(OS)-$(CPUNAME)
    # = LINUX-SPARC64` rather than a plain CPUNAME arm -- reproduced as an
    # explicit CPUNAME+OS combination here instead.
    set(BRIDGES_SELECTED_BRIDGE "gcc3_linux_sparc64")
    set(BRIDGE_ASM_OBJECTS call.s)
    set(BRIDGE_NOOPT_OBJECTS cpp2uno.cxx uno2cpp.cxx)
    set(BRIDGE_EXCEPTION_OBJECTS except.cxx)

elseif (LIBRESTUDIO_CPUNAME STREQUAL "X86_64")
    if (CMAKE_SYSTEM_NAME IN_LIST _bridges_bsd_linux_haiku_os)
        # The one arm this file already implemented before 2026-09-28 --
        # kept byte-identical.
        set(BRIDGES_SELECTED_BRIDGE "gcc3_linux_x86-64")
        set(BRIDGE_ASM_OBJECTS call.s)
        set(BRIDGE_NONCALLEXCEPTION_NOOPT_OBJECTS callvirtualmethod.cxx)
        set(BRIDGE_EXCEPTION_OBJECTS abi.cxx cpp2uno.cxx except.cxx rtti.cxx uno2cpp.cxx)
    elseif (APPLE)
        set(BRIDGES_SELECTED_BRIDGE "gcc3_macosx_x86-64")
        set(BRIDGE_EXCEPTION_OBJECTS abi.cxx call.cxx cpp2uno.cxx except.cxx uno2cpp.cxx)
        set(BRIDGE_NONCALLEXCEPTION_NOOPT_OBJECTS callvirtualmethod.cxx)
    elseif (WIN32 AND MSVC)
        set(BRIDGES_SELECTED_BRIDGE "msvc_win32_x86-64")
        set(BRIDGE_EXCEPTION_OBJECTS cpp2uno.cxx uno2cpp.cxx)
        set(BRIDGE_NOOPT_OBJECTS except.cxx)
        set(BRIDGE_ASM_OBJECTS call.asm)
    endif ()

endif ()
unset(_bridges_bsd_linux_os)
unset(_bridges_bsd_linux_haiku_os)

if (BRIDGE_NOT_IMPLEMENTED)
    message(STATUS "bridges: gcc3_uno not built for this target -- ${BRIDGE_NOT_IMPLEMENTED}")
elseif (NOT BRIDGES_SELECTED_BRIDGE)
    message(FATAL_ERROR
        "bridges: no gcc3_uno bridge selection branch matched "
        "LIBRESTUDIO_CPUNAME=${LIBRESTUDIO_CPUNAME} on "
        "CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME} -- check bridges/"
        "Library_cpp_uno.mk's own ifeq chain for whether upstream itself has "
        "a bridge for this combination before adding one here.")
else ()

set(BRIDGE_DIR "${CMAKE_SOURCE_DIR}/bridges/source/cpp_uno/${BRIDGES_SELECTED_BRIDGE}")
set(BRIDGE_SHARED_DIR "${CMAKE_SOURCE_DIR}/bridges/source/cpp_uno/shared")

# ARM's own armhelper.o: bridges/CustomTarget_gcc3_linux_arm.mk's own real
# recipe (`$(gb_CXX) -c -o armhelper.o armhelper.S -fPIC`, read in full) --
# a raw object file gbuild attaches to the link via EXTRAOBJECTLISTS rather
# than through the normal add_asmobjects/add_exception_objects machinery
# (the "HACK" comment in the real .mk is upstream's own words for this).
# Reproduced literally: compile it the same way, then reference the
# resulting .o directly as a library source (CMake accepts a pre-built .o
# path as a source like any other).
set(BRIDGE_ARM_HELPER_OBJECTS "")
if (BRIDGE_ARM_HELPER)
    set(_bridge_armhelper_o "${CMAKE_CURRENT_BINARY_DIR}/armhelper.o")
    add_custom_command(
        OUTPUT "${_bridge_armhelper_o}"
        COMMAND "${CMAKE_CXX_COMPILER}" -c -o "${_bridge_armhelper_o}"
                "${BRIDGE_DIR}/armhelper.S" -fPIC
        DEPENDS "${BRIDGE_DIR}/armhelper.S"
        COMMENT "gcc3_linux_arm: armhelper.o"
        VERBATIM)
    list(APPEND BRIDGE_ARM_HELPER_OBJECTS "${_bridge_armhelper_o}")
    unset(_bridge_armhelper_o)
endif ()

# Every per-arch category list above holds bare file names (matching how the
# real .mk writes them too, e.g. "abi except uno2cpp") -- prepended with
# BRIDGE_DIR/ here, once, before any of them are handed to add_library/
# target_sources below.
list(TRANSFORM BRIDGE_EXCEPTION_OBJECTS PREPEND "${BRIDGE_DIR}/")
list(TRANSFORM BRIDGE_NOOPT_OBJECTS PREPEND "${BRIDGE_DIR}/")
list(TRANSFORM BRIDGE_ASM_OBJECTS PREPEND "${BRIDGE_DIR}/")
list(TRANSFORM BRIDGE_NONCALLEXCEPTION_OBJECTS PREPEND "${BRIDGE_DIR}/")
list(TRANSFORM BRIDGE_NONCALLEXCEPTION_NOOPT_OBJECTS PREPEND "${BRIDGE_DIR}/")

add_library(gcc3_uno SHARED
    # bridge_exception_objects
    ${BRIDGE_EXCEPTION_OBJECTS}
    # bridges/source/cpp_uno/shared
    "${BRIDGE_SHARED_DIR}/bridge.cxx"
    "${BRIDGE_SHARED_DIR}/component.cxx"
    "${BRIDGE_SHARED_DIR}/types.cxx"
    "${BRIDGE_SHARED_DIR}/unointerfaceproxy.cxx"
    "${BRIDGE_SHARED_DIR}/vtablefactory.cxx"
    "${BRIDGE_SHARED_DIR}/vtables.cxx"
    # bridge_asm_objects
    ${BRIDGE_ASM_OBJECTS}
    # ARM's own extra linked object (see above)
    ${BRIDGE_ARM_HELPER_OBJECTS})

# bridge_noopt_objects (gb_COMPILERNOOPTFLAGS)
foreach (_bridge_obj IN LISTS BRIDGE_NOOPT_OBJECTS)
    set_source_files_properties("${_bridge_obj}" PROPERTIES
        COMPILE_OPTIONS "${BRIDGES_NOOPT_FLAGS}")
    target_sources(gcc3_uno PRIVATE "${_bridge_obj}")
endforeach ()

# bridge_noncallexception_objects
foreach (_bridge_obj IN LISTS BRIDGE_NONCALLEXCEPTION_OBJECTS)
    set_source_files_properties("${_bridge_obj}" PROPERTIES
        COMPILE_OPTIONS "${BRIDGES_NON_CALL_EXCEPTIONS_FLAGS}")
    target_sources(gcc3_uno PRIVATE "${_bridge_obj}")
endforeach ()

# bridge_noncallexception_noopt_objects
foreach (_bridge_obj IN LISTS BRIDGE_NONCALLEXCEPTION_NOOPT_OBJECTS)
    set_source_files_properties("${_bridge_obj}" PROPERTIES
        COMPILE_OPTIONS "${BRIDGES_NOOPT_FLAGS};${BRIDGES_NON_CALL_EXCEPTIONS_FLAGS}")
    target_sources(gcc3_uno PRIVATE "${_bridge_obj}")
endforeach ()
unset(_bridge_obj)

# AARCH64's own extra exception object, callvirtualfunction.cxx, needs
# conditional flags (`ifneq ($(HAVE_GCC_STACK_CLASH_PROTECTION),)` and
# `ifneq ($(COM_IS_CLANG),)` in the real .mk) real compiler-flag detection
# now handles (cmake/LibreStudioPlatform.cmake).
if (BRIDGE_AARCH64_CALLVIRTUALFUNCTION)
    set(_bridge_aarch64_cvf_flags "")
    if (LIBRESTUDIO_HAVE_GCC_STACK_CLASH_PROTECTION)
        list(APPEND _bridge_aarch64_cvf_flags -fno-stack-clash-protection)
    endif ()
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        list(APPEND _bridge_aarch64_cvf_flags -fasynchronous-unwind-tables)
    endif ()
    if (_bridge_aarch64_cvf_flags)
        set_source_files_properties("${BRIDGE_DIR}/callvirtualfunction.cxx" PROPERTIES
            COMPILE_OPTIONS "${_bridge_aarch64_cvf_flags}")
    endif ()
    target_sources(gcc3_uno PRIVATE "${BRIDGE_DIR}/callvirtualfunction.cxx")
    unset(_bridge_aarch64_cvf_flags)
endif ()

# A plain C++ object, but compiled without optimisation and with exceptions
# enabled (gb_Library_add_cxxobjects with gb_COMPILERNOOPTFLAGS and
# gb_LinkTarget_EXCEPTIONFLAGS).
set_source_files_properties("${BRIDGE_SHARED_DIR}/cppinterfaceproxy.cxx"
    PROPERTIES COMPILE_OPTIONS "${BRIDGES_NOOPT_FLAGS}")
target_sources(gcc3_uno PRIVATE "${BRIDGE_SHARED_DIR}/cppinterfaceproxy.cxx")

# MSVC's own two extra shared files (`ifeq ($(COM),MSC)` in the real .mk,
# unconditional on top of whichever msvc_win32_* arm matched above).
if (WIN32 AND MSVC)
    target_sources(gcc3_uno PRIVATE
        "${CMAKE_SOURCE_DIR}/bridges/source/cpp_uno/msvc_shared/cpp2uno.cxx"
        "${CMAKE_SOURCE_DIR}/bridges/source/cpp_uno/msvc_shared/except.cxx")
endif ()

# gb_Library_use_internal_comprehensive_api: the bridge inlines the type
# information it marshals, rather than resolving it through a typelib.
# AARCH64+real-macOS additionally needs offapi's own comprehensive headers
# (Library_cpp_uno.mk's own `ifeq ($(OS),MACOSX) ifeq ($(CPUNAME),AARCH64)`
# nested guard) -- Apple Silicon-specific, not needed by any other arm.
target_include_directories(gcc3_uno PRIVATE
    "${CMAKE_SOURCE_DIR}/bridges/inc"
    "${udkapi_comprehensive_HEADERS_INCLUDE_DIR}")
add_dependencies(gcc3_uno udkapi_comprehensive_headers)
if (BRIDGE_EXTRA_OFFAPI)
    target_include_directories(gcc3_uno PRIVATE "${offapi_comprehensive_HEADERS_INCLUDE_DIR}")
    add_dependencies(gcc3_uno offapi_comprehensive_headers)
endif ()

target_compile_options(gcc3_uno PRIVATE ${BRIDGES_CXXFLAGS})

target_link_libraries(gcc3_uno PRIVATE cppu sal)

# `ifeq ($(filter ANDROID WNT DRAGONFLY FREEBSD NETBSD OPENBSD MACOSX iOS
# HAIKU,$(OS)),)` in the real .mk: -ldl is added everywhere EXCEPT that
# list -- i.e. only for plain Linux (and, by the same filter, Solaris, not
# reproduced elsewhere in this port either). Android/*BSD/macOS/iOS/Haiku/
# Windows all have libdl built into libc or no dlopen() at all, so none of
# them need it explicitly.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(gcc3_uno PRIVATE ${CMAKE_DL_LIBS})
endif ()

if (LIBRESTUDIO_HAVE_POSIX_FALLOCATE)
    target_compile_definitions(gcc3_uno PRIVATE HAVE_POSIX_FALLOCATE)
endif ()

# Registered as a PLAINLIBS_URE plug-in upstream, hence libgcc3_uno.so with no
# version suffix.
librestudio_set_library_output_name(gcc3_uno gcc3_uno PLAIN)

install(TARGETS gcc3_uno LIBRARY DESTINATION program)

endif ()

# Nothing links this library — it is found at run time.  cppu's loadEnv()
# (cppu/source/uno/lbenv.cxx) builds the library name from
# CPPU_CURRENT_LANGUAGE_BINDING_NAME and dlopen()s it, and every program that
# bootstraps UNO passes through there.  Two consequences worth knowing:
#
# When the library is missing, the failure is a segfault, not a link error: the
# environment comes back null, and uno_getMapping is handed a null to build a
# name out of.  Nothing is logged, because the SAL logging that would have
# complained is compiled out of a release build.  And ninja cannot see the
# dependency, so anything that runs such a program before the build is over —
# a generator invoked from add_custom_command, a tool run by hand against a
# half-built tree — needs it said out loud with add_dependencies.  The places in
# this tree where that is so say it.

# --- java_uno (Library_java_uno.mk + Jar_java_uno.mk) -----------------------
#
# The JNI-based UNO bridge: makes a Java-implemented UNO object callable as a
# real Reference<XInterface> from C++ (and vice versa), the missing piece
# stoc's javaloader/javavm (already ported) cannot supply on their own -- they
# can start a JVM and load a class into it, but not marshal calls across the
# JNI boundary into UNO's binary ABI.
if (LIBRESTUDIO_ENABLE_JAVA)
    # gb_Library_use_internal_comprehensive_api(java_uno,udkapi): same reason
    # as gcc3_uno above -- the bridge inlines type information it marshals
    # rather than resolving it through a typelib at runtime.
    #
    # No per-file flags here, unlike gcc3_uno's own abi-poking sources: the
    # .mk calls plain gb_Library_add_exception_objects (normal C++ exception
    # flags), not gb_Library_add_cxxobjects with special COMPILERNOOPTFLAGS/
    # NON_CALL_EXCEPTIONS handling -- this bridge crosses the JNI boundary
    # through ordinary calls, not the raw vtable/register poking gcc3_uno's
    # own ABI code does.
    add_library(java_uno SHARED
        "${CMAKE_SOURCE_DIR}/bridges/source/jni_uno/jni_bridge.cxx"
        "${CMAKE_SOURCE_DIR}/bridges/source/jni_uno/jni_data.cxx"
        "${CMAKE_SOURCE_DIR}/bridges/source/jni_uno/jni_info.cxx"
        "${CMAKE_SOURCE_DIR}/bridges/source/jni_uno/jni_java2uno.cxx"
        "${CMAKE_SOURCE_DIR}/bridges/source/jni_uno/jni_uno2java.cxx"
        "${CMAKE_SOURCE_DIR}/bridges/source/jni_uno/nativethreadpool.cxx")

    target_include_directories(java_uno PRIVATE
        "${CMAKE_SOURCE_DIR}/bridges/inc"
        "${udkapi_comprehensive_HEADERS_INCLUDE_DIR}"
        "${JNI_INCLUDE_DIRS}")
    add_dependencies(java_uno udkapi_comprehensive_headers)

    target_link_libraries(java_uno PRIVATE cppu jvmaccess sal salhelper)

    # Registered as a PLAINLIBS_URE library upstream (Repository.mk), same
    # group as gcc3_uno/jpipe -- confirmed against the consumer, not assumed:
    # both JNI_proxy.java and JNI_info_holder.java (bridges/source/jni_uno/
    # java/.../*.java) call NativeLibraryLoader.loadLibrary(..., "java_uno"),
    # i.e. plain System.loadLibrary("java_uno") resolving to bare
    # "libjava_uno.so" -- no "uno_" prefix, no soversion, exactly PLAIN.
    librestudio_set_library_output_name(java_uno java_uno PLAIN)

    install(TARGETS java_uno LIBRARY DESTINATION program)

    # This one library serves two, unrelated loading paths at once, neither
    # visible to ninja's own dependency graph:
    #   1. cppu's loadEnv() dlopen()s it by UNO environment name ("java"),
    #      exactly like gcc3_uno above (jni_bridge.cxx exports the same
    #      uno_initEnvironment/uno_ext_getMapping entry points) -- the same
    #      "nothing links this, needs add_dependencies said out loud"
    #      situation the comment above already explains for gcc3_uno.
    #   2. The Java side (JNI_proxy/JNI_info_holder, compiled into
    #      java_uno.jar below) loads it directly via System.loadLibrary, to
    #      resolve the native methods declared in nativethreadpool.cxx and
    #      the jni_java2uno/jni_uno2java JNI entry points.
    # A future consumer of the Java-UNO bridge (once one is wired) needs an
    # explicit add_dependencies on both this target and java_uno_jar below --
    # neither a classpath entry nor a dlopen-by-name is a dependency ninja
    # can infer on its own.

    # --- java_uno.jar (Jar_java_uno.mk) --------------------------------
    #
    # Needs ${libreoffice_JAR}, set by ridljar/CMakeLists.txt -- but
    # add_subdirectory(bridges) runs before add_subdirectory(ridljar) in the
    # root CMakeLists.txt (bridges is unconditional, ridljar is the fourth
    # step of the whole LIBRESTUDIO_ENABLE_JAVA module chain that starts
    # after this file). target_link_libraries/add_dependencies with a target
    # name would resolve fine regardless of order (CMake matches those
    # against the whole tree after every add_subdirectory() has run), but
    # ${libreoffice_JAR} is a plain variable read at configure time, and
    # would not be set yet here. Deferred the same way svl/sfx2/vcl/
    # reportdesign/scripting already defer their own *_deferred.cmake files:
    # runs once, after every add_subdirectory() in the whole tree has
    # completed, in the root directory's own scope (see that file's own
    # header comment for why every path in it is CMAKE_SOURCE_DIR-relative,
    # not CMAKE_CURRENT_SOURCE_DIR-relative).
    cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
        "${CMAKE_SOURCE_DIR}/bridges/Jar_java_uno_deferred.cmake")
endif ()
