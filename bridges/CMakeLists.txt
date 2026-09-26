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
# (bridges/source/cpp_uno/gcc3_linux_x86-64/abi.cxx, rtti.cxx), it unwinds
# through a call that the compiler must not assume cannot throw, and part of it
# is hand-written assembly.  Upstream therefore compiles it with a specific set
# of per-file flags (Library_cpp_uno.mk); reproducing them is not optional —
# a uniform flag set produces a bridge that crashes rather than one that fails
# to build.
#
# Only the C++ bridge is built.  The .NET bridge (net_uno) is behind
# ENABLE_DOTNET upstream and is not enabled here.
#
# Latent gap: the Java UNO bridge is not ported either. Upstream's
# Module_bridges.mk builds Jar_java_uno, Library_java_uno, and (macOS-only)
# Package_jnilib_java_uno under `ifneq ($(ENABLE_JAVA),)`; none of the three
# exist anywhere in this tree. This is currently masked because
# LIBRESTUDIO_ENABLE_JAVA defaults OFF, but jurt, javaunohelper, and bean
# (see their CMakeLists.txt) already build under that flag and are Java-UNO-
# bridge *consumers* -- if LIBRESTUDIO_ENABLE_JAVA is ever turned ON, they
# would build without the actual bridge library (Library_java_uno) they need
# at runtime. This must be addressed before Java support can be considered
# complete.

# Upstream names this by the CPPU environment: $(CPPU_ENV)_uno, i.e. gcc3_uno.
set(BRIDGE_DIR "${CMAKE_SOURCE_DIR}/bridges/source/cpp_uno/gcc3_linux_x86-64")

# Flags for the whole library (gb_Library_add_cxxflags in the .mk): the frame
# pointer is needed to walk the stack during unwinding, strict aliasing
# miscompiles the ABI poking, LTO breaks it outright, and AVX registers are not
# part of the UNO calling convention.
set(BRIDGES_CXXFLAGS
    -fno-omit-frame-pointer
    -fno-strict-aliasing
    -fno-lto)

# gb_COMPILERNOOPTFLAGS from solenv/gbuild/platform/com_GCC_defs.mk.
set(BRIDGES_NOOPT_FLAGS -O0 -Wp,-U_FORTIFY_SOURCE -fstrict-aliasing -fstrict-overflow)

# bridges_NON_CALL_EXCEPTIONS_FLAGS: callvirtualmethod must be able to catch a
# signal raised by the call it makes, which only works if the compiler is told
# the call can throw.
set(BRIDGES_NON_CALL_EXCEPTIONS_FLAGS -fnon-call-exceptions)

add_library(gcc3_uno SHARED
    # bridge_exception_objects
    "${BRIDGE_DIR}/abi.cxx"
    "${BRIDGE_DIR}/cpp2uno.cxx"
    "${BRIDGE_DIR}/except.cxx"
    "${BRIDGE_DIR}/rtti.cxx"
    "${BRIDGE_DIR}/uno2cpp.cxx"
    # bridges/source/cpp_uno/shared
    "${CMAKE_SOURCE_DIR}/bridges/source/cpp_uno/shared/bridge.cxx"
    "${CMAKE_SOURCE_DIR}/bridges/source/cpp_uno/shared/component.cxx"
    "${CMAKE_SOURCE_DIR}/bridges/source/cpp_uno/shared/types.cxx"
    "${CMAKE_SOURCE_DIR}/bridges/source/cpp_uno/shared/unointerfaceproxy.cxx"
    "${CMAKE_SOURCE_DIR}/bridges/source/cpp_uno/shared/vtablefactory.cxx"
    "${CMAKE_SOURCE_DIR}/bridges/source/cpp_uno/shared/vtables.cxx"
    # bridge_asm_objects
    "${BRIDGE_DIR}/call.s")

# bridge_noncallexception_noopt_objects
set_source_files_properties("${BRIDGE_DIR}/callvirtualmethod.cxx" PROPERTIES
    COMPILE_OPTIONS "${BRIDGES_NOOPT_FLAGS};${BRIDGES_NON_CALL_EXCEPTIONS_FLAGS}")
target_sources(gcc3_uno PRIVATE "${BRIDGE_DIR}/callvirtualmethod.cxx")

# A plain C++ object, but compiled without optimisation and with exceptions
# enabled (gb_Library_add_cxxobjects with gb_COMPILERNOOPTFLAGS and
# gb_LinkTarget_EXCEPTIONFLAGS).
set_source_files_properties("${CMAKE_SOURCE_DIR}/bridges/source/cpp_uno/shared/cppinterfaceproxy.cxx"
    PROPERTIES COMPILE_OPTIONS "${BRIDGES_NOOPT_FLAGS}")
target_sources(gcc3_uno PRIVATE
    "${CMAKE_SOURCE_DIR}/bridges/source/cpp_uno/shared/cppinterfaceproxy.cxx")

# gb_Library_use_internal_comprehensive_api: the bridge inlines the type
# information it marshals, rather than resolving it through a typelib.
target_include_directories(gcc3_uno PRIVATE
    "${CMAKE_SOURCE_DIR}/bridges/inc"
    "${udkapi_comprehensive_HEADERS_INCLUDE_DIR}")
add_dependencies(gcc3_uno udkapi_comprehensive_headers)

target_compile_options(gcc3_uno PRIVATE ${BRIDGES_CXXFLAGS})

target_link_libraries(gcc3_uno PRIVATE cppu sal ${CMAKE_DL_LIBS})

# Registered as a PLAINLIBS_URE plug-in upstream, hence libgcc3_uno.so with no
# version suffix.
librestudio_set_library_output_name(gcc3_uno gcc3_uno PLAIN)

install(TARGETS gcc3_uno LIBRARY DESTINATION program)

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
