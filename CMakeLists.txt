# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# LibreStudio — CMake build.
#
# This builds the UNO core (sal, cppu, cppuhelper, the service manager, the C++
# bridge and their dependencies) from this tree, using the tree's own IDL
# toolchain.  It is an independent build system: it does not read, invoke or
# depend on gbuild, and the .mk files next to each CMakeLists.txt are ignored.
#
# Usage:
#   cmake -S . -B build -G Ninja
#   cmake --build build

cmake_minimum_required(VERSION 3.20)

project(LibreStudio LANGUAGES C CXX ASM)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# --- options -----------------------------------------------------------------

option(LIBRESTUDIO_ENABLE_JAVA
    "Build the Java parts of the UNO core (jvmaccess, jvmfwk, javaloader, javavm). \
Requires a JDK." OFF)

option(LIBRESTUDIO_ENABLE_GPGME
    "Build comphelper's GPGME-backed document signing support. Requires libgpgmepp." OFF)

set(LIBRESTUDIO_TLS_BACKEND "NSS" CACHE STRING
    "Crypto backend for comphelper (NSS or OPENSSL)")
set_property(CACHE LIBRESTUDIO_TLS_BACKEND PROPERTY STRINGS NSS OPENSSL)

# CTest has to be brought in at the top level: enable_testing() called from a
# subdirectory does not produce the root CTestTestfile.cmake, and ctest then
# reports "No tests were found" no matter what the subdirectories registered.
include(CTest)

# --- platform, configuration, dependencies -----------------------------------

include(LibreStudioPlatform)
include(LibreStudioTarballs)
include(LibreStudioConfigHeaders)
include(LibreStudioUnoApi)
include(LibreStudioComponent)
include(LibreStudioUnorc)

# createcomponent.xslt and packcomponents.xslt from solenv/bin are driven by
# xsltproc, both here and by the officecfg schemas.
find_program(XSLTPROC_EXECUTABLE xsltproc REQUIRED)

# Third-party header-only libraries that upstream keeps as tarballs rather than
# vendoring.  Prefer a system copy — configure.ac does the same behind
# --with-system-dragonbox / --with-system-fast_float — and only download when
# there is none.
find_path(DRAGONBOX_INCLUDE_DIR dragonbox/dragonbox.h
    PATHS /usr/include /usr/local/include /usr/include/dragonbox-1.1.3)
if (NOT DRAGONBOX_INCLUDE_DIR)
    librestudio_fetch_tarball(dragonbox dragonbox-1.1.3.tar.gz
        09d63b05e9c594ec423778ab59b7a5aa1d76fdd71d25c7048b0258c4ec9c3384)
    set(DRAGONBOX_INCLUDE_DIR "${dragonbox_SOURCE_DIR}/include")
endif ()

find_path(FAST_FLOAT_INCLUDE_DIR fast_float/fast_float.h
    PATHS /usr/include /usr/local/include)
if (NOT FAST_FLOAT_INCLUDE_DIR)
    librestudio_fetch_tarball(fast_float fast_float-8.3.0.tar.gz
        90485994d0fed61d0693e8dba32c464b0e8adf7f2e7c2efbf5bff0df5ef5b13f)
    set(FAST_FLOAT_INCLUDE_DIR "${fast_float_SOURCE_DIR}/include")
endif ()

# frozen's unordered containers are used by comphelper and the net bridge.
find_path(FROZEN_INCLUDE_DIR frozen/unordered_map.h
    PATHS /usr/include /usr/local/include)
if (NOT FROZEN_INCLUDE_DIR)
    librestudio_fetch_tarball(frozen frozen-1.2.0.tar.gz
        ed8339c017d7c5fe019ac2c642477f435278f0dc643c1d69d3f3b1e95915e823)
    set(FROZEN_INCLUDE_DIR "${frozen_SOURCE_DIR}/include")
endif ()

find_package(ZLIB REQUIRED)
find_package(Boost 1.69 REQUIRED COMPONENTS locale)
find_package(Threads REQUIRED)
find_package(ICU 66 REQUIRED COMPONENTS uc i18n)
find_package(PkgConfig REQUIRED)

# liblangtag is used directly by i18nlangtag; it pulls libxml2 in through its
# own pkg-config file, which is why the UNO core links libxml2 without using it.
pkg_check_modules(LIBLANGTAG REQUIRED IMPORTED_TARGET liblangtag>=0.4.0)

# tl parses XML with libxml2 directly (tools/source/xml/XmlWalker.cxx).
find_package(LibXml2 REQUIRED)

# sax wraps expat as well as libxml2.
find_package(EXPAT REQUIRED)

# --- modules -----------------------------------------------------------------
#
# The order is the bootstrap order, not the order the modules appear in the
# source tree: the IDL toolchain in unoidl/ and codemaker/ has to exist before
# any module that consumes generated com/sun/star headers can be compiled.

add_subdirectory(sal)
add_subdirectory(salhelper)
add_subdirectory(store)
add_subdirectory(registry)
add_subdirectory(unoidl)
add_subdirectory(codemaker)
add_subdirectory(udkapi)
add_subdirectory(cppu)
add_subdirectory(offapi)
add_subdirectory(officecfg)
add_subdirectory(i18nlangtag)
add_subdirectory(xmlreader)
add_subdirectory(io)
add_subdirectory(binaryurp)
add_subdirectory(ucbhelper)
add_subdirectory(comphelper)
add_subdirectory(stoc)
add_subdirectory(i18nutil)
add_subdirectory(sax)

add_subdirectory(i18npool)
add_subdirectory(bridges)
add_subdirectory(cppuhelper)
add_subdirectory(basegfx)
add_subdirectory(tools)
add_subdirectory(unotools)
add_subdirectory(sot)

# cppuhelper is added last: it consumes the comprehensive header variants of
# udkapi, offapi and its own private registry, all of which have to be declared
# before it.

# --- service registry and bootstrap configuration ----------------------------

# Every module has now declared its .component files, so they can be packed.
librestudio_finalize_components(services)

# The service manager finds libraries through the uri attribute baked into each
# component above, and unorc is what expands $URE_INTERNAL_LIB_DIR and
# $LO_LIB_DIR in those attributes.  Both have to sit next to the libraries.
add_custom_command(
    OUTPUT "${LIBRESTUDIO_PROGRAM_DIR}/services.rdb"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${services_RDB}" "${LIBRESTUDIO_PROGRAM_DIR}/services.rdb"
    DEPENDS "${services_RDB}" services_rdb
    COMMENT "Installing services.rdb"
    VERBATIM)
add_custom_target(program_services_rdb ALL
    DEPENDS "${LIBRESTUDIO_PROGRAM_DIR}/services.rdb")

# types.rdb holds the UDK types and stays a copy of udkapi's registry; the
# office API types live beside it in types/offapi.rdb, and unorc names both.
# That is the layout upstream installs — odk/settings/std.mk points at
# program/types/offapi.rdb, and the iOS packaging copies program/types.rdb
# across as udkapi.rdb next to it.  A single merged types.rdb is what the
# installed office ships on the desktop, but nothing in the bootstrap requires
# one file: UNO_TYPES is a list.
add_custom_command(
    OUTPUT "${LIBRESTUDIO_PROGRAM_DIR}/types.rdb"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${udkapi_RDB}" "${LIBRESTUDIO_PROGRAM_DIR}/types.rdb"
    DEPENDS "${udkapi_RDB}" udkapi_rdb
    COMMENT "Installing types.rdb"
    VERBATIM)
add_custom_target(program_types_rdb ALL
    DEPENDS "${LIBRESTUDIO_PROGRAM_DIR}/types.rdb")

add_custom_command(
    OUTPUT "${LIBRESTUDIO_PROGRAM_DIR}/types/offapi.rdb"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${LIBRESTUDIO_PROGRAM_DIR}/types"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${offapi_RDB}" "${LIBRESTUDIO_PROGRAM_DIR}/types/offapi.rdb"
    DEPENDS "${offapi_RDB}" offapi_rdb
    COMMENT "Installing types/offapi.rdb"
    VERBATIM)
add_custom_target(program_offapi_rdb ALL
    DEPENDS "${LIBRESTUDIO_PROGRAM_DIR}/types/offapi.rdb")

# saxparser runs as part of the build and resolves com.sun.star.xml.sax.Parser
# through the service manager, so the registry it reads has to be the one this
# build just produced — not the stale copy from a previous one.  That makes the
# locale data depend on the installed registry and on libsaxlo itself, neither
# of which the executable links against.
add_dependencies(i18npool_localedata program_services_rdb sax)

librestudio_generate_unorc("${LIBRESTUDIO_PROGRAM_DIR}/unorc")

# --- smoke test --------------------------------------------------------------

add_subdirectory(cpputools)
add_subdirectory(smoketest)
