# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# codemaker — the C++ and Java binding generators.  cppumaker turns an .rdb
# type registry into the com/sun/star/**/*.hpp headers that the rest of the
# build compiles against, which makes it the hinge of the whole bootstrap.
# javamaker is its Java counterpart, used by unoil and ridljar to generate the
# com/sun/star/**/*.java stubs the Java UNO toolchain compiles against (see
# unoil/CMakeLists.txt, ridljar/CMakeLists.txt) — like cppumaker, it needs no
# JNI/JDK to run itself (unoidl, salhelper, sal only), so it and its
# codemaker_java static library are built unconditionally here, the same way
# cppumaker always is; only the modules that actually invoke it to generate
# Java stubs are gated behind LIBRESTUDIO_ENABLE_JAVA.
#
# netmaker, pythonmaker and rustmaker are for language bindings the core does
# not need.  Their sources are left in place and can be added the same way as
# cppumaker/javamaker.

# The three shared object sets the generators are built from.  Upstream keeps
# them static (StaticLibrary_codemaker.mk, StaticLibrary_codemaker_cpp.mk,
# StaticLibrary_codemaker_java.mk).
set(CODEMAKER_SOURCES
    codemaker/source/codemaker/exceptiontree
    codemaker/source/codemaker/global
    codemaker/source/codemaker/options
    codemaker/source/codemaker/typemanager
    codemaker/source/codemaker/unotype
    codemaker/source/codemaker/codemaker)

set(CODEMAKER_CPP_SOURCES
    codemaker/source/commoncpp/commoncpp)

set(CODEMAKER_JAVA_SOURCES
    codemaker/source/commonjava/commonjava)

librestudio_expand_cxx(CODEMAKER_SOURCES)
librestudio_expand_cxx(CODEMAKER_CPP_SOURCES)
librestudio_expand_cxx(CODEMAKER_JAVA_SOURCES)

add_library(codemaker STATIC ${CODEMAKER_SOURCES})
target_link_libraries(codemaker PUBLIC Boost::boost)

add_library(codemaker_cpp STATIC ${CODEMAKER_CPP_SOURCES})
target_link_libraries(codemaker_cpp PUBLIC Boost::boost)

add_library(codemaker_java STATIC ${CODEMAKER_JAVA_SOURCES})
target_link_libraries(codemaker_java PUBLIC Boost::boost)

# --- cppumaker ---------------------------------------------------------------

# Note what this does *not* link: no cppu, no cppuhelper, and therefore no
# generated com/sun/star headers.  Everything it needs is sal, salhelper and
# unoidl, so the tool can be built before any binding exists.  The
# com/sun/star includes that appear in codemaker/source/cppumaker/*.cxx are
# inside string literals — they are text cppumaker writes, not headers it reads.
add_executable(cppumaker
    "${CMAKE_SOURCE_DIR}/codemaker/source/cppumaker/cppumaker.cxx"
    "${CMAKE_SOURCE_DIR}/codemaker/source/cppumaker/cppuoptions.cxx"
    "${CMAKE_SOURCE_DIR}/codemaker/source/cppumaker/cpputype.cxx"
    "${CMAKE_SOURCE_DIR}/codemaker/source/cppumaker/dependencies.cxx"
    "${CMAKE_SOURCE_DIR}/codemaker/source/cppumaker/dumputils.cxx"
    "${CMAKE_SOURCE_DIR}/codemaker/source/cppumaker/includes.cxx")

target_include_directories(cppumaker PRIVATE
    "${CMAKE_SOURCE_DIR}/codemaker/source/cppumaker")

target_link_libraries(cppumaker PRIVATE
    codemaker_cpp
    codemaker
    unoidl
    salhelper
    sal
    Boost::boost)

install(TARGETS cppumaker RUNTIME DESTINATION program)

# --- javamaker -----------------------------------------------------------------

# Same shape as cppumaker: no cppu/cppuhelper, no JNI, nothing that needs a JDK
# to build the generator itself — only unoidl, salhelper and sal. reg/store are
# upstream's DISABLE_DYNLOADING-only extra libraries (Executable_javamaker.mk);
# DISABLE_DYNLOADING is not defined anywhere in this CMake port (confirmed by
# grep), so, like cppumaker, they are omitted.
add_executable(javamaker
    "${CMAKE_SOURCE_DIR}/codemaker/source/javamaker/classfile.cxx"
    "${CMAKE_SOURCE_DIR}/codemaker/source/javamaker/javamaker.cxx"
    "${CMAKE_SOURCE_DIR}/codemaker/source/javamaker/javaoptions.cxx"
    "${CMAKE_SOURCE_DIR}/codemaker/source/javamaker/javatype.cxx")

target_include_directories(javamaker PRIVATE
    "${CMAKE_SOURCE_DIR}/codemaker/source/javamaker")

target_link_libraries(javamaker PRIVATE
    codemaker_java
    codemaker
    unoidl
    salhelper
    sal
    Boost::boost)

install(TARGETS javamaker RUNTIME DESTINATION program)
