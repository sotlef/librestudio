# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# package — the zip and ODF package layer.  package2 reads and writes the
# zip containers and the manifest inside them; xstor wraps an OLE2 or zip
# container behind the storage interfaces.  Source lists from
# Library_package2.mk and Library_xstor.mk.

# zlib, resolved via librestudio_find_package() (cmake/LibreStudioTarballs.cmake).
librestudio_find_package(NAME ZLIB)

# --- argon2 -------------------------------------------------------------------
#
# package/source/zipapi/ZipFile.cxx verifies the argon2id hashes that protect a
# password-protected ODF package, and argon2 is not in this tree.  The tarball
# ships a Makefile that upstream drives as an external project; here the sources
# are compiled directly, which keeps the build in one place and lets CMake track
# the objects.
#
# The tarball carries two interchangeable cores and its Makefile picks between
# them by architecture — src/opt.c on x86-64, src/ref.c everywhere else, since
# it passes OPTTARGET=forcefail off x86-64 and lets the probe fail.  Same
# choice here.

set(ARGON2_SRC "${argon2_SOURCE_DIR}/src")
if (LIBRESTUDIO_RTL_ARCH STREQUAL "x86_64")
    set(ARGON2_CORE "${ARGON2_SRC}/opt.c")
else ()
    set(ARGON2_CORE "${ARGON2_SRC}/ref.c")
endif ()

add_library(argon2 STATIC
    "${ARGON2_SRC}/argon2.c"
    "${ARGON2_SRC}/blake2/blake2b.c"
    "${ARGON2_SRC}/core.c"
    "${ARGON2_SRC}/encoding.c"
    "${ARGON2_SRC}/thread.c"
    "${ARGON2_CORE}")

target_include_directories(argon2 PUBLIC
    "${argon2_SOURCE_DIR}/include" "${ARGON2_SRC}")
target_link_libraries(argon2 PUBLIC Threads::Threads)
# gb_Library_set_warnings_disabled, as for zxcvbn-c: not our source.
target_compile_options(argon2 PRIVATE -w)
set_target_properties(argon2 PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- package2 -----------------------------------------------------------------

set(PACKAGE2_SOURCES
    package/source/manifest/ManifestExport
    package/source/manifest/ManifestImport
    package/source/manifest/ManifestReader
    package/source/manifest/ManifestWriter
    package/source/zipapi/blowfishcontext
    package/source/zipapi/ByteChucker
    package/source/zipapi/ByteGrabber
    package/source/zipapi/CRC32
    package/source/zipapi/Deflater
    package/source/zipapi/InflaterBytesZlib
    package/source/zipapi/InflaterBytesZstd
    package/source/zipapi/InflateZlib
    package/source/zipapi/sha1context
    package/source/zipapi/ThreadedDeflater
    package/source/zipapi/XBufferedThreadedStream
    package/source/zipapi/XUnbufferedStream
    package/source/zipapi/ZipEnumeration
    package/source/zipapi/ZipFile
    package/source/zipapi/ZipOutputEntry
    package/source/zipapi/ZipOutputStream
    package/source/zippackage/wrapstreamforshare
    package/source/zippackage/zipfileaccess
    package/source/zippackage/ZipPackageBuffer
    package/source/zippackage/ZipPackage
    package/source/zippackage/ZipPackageEntry
    package/source/zippackage/ZipPackageFolder
    package/source/zippackage/ZipPackageFolderEnumeration
    package/source/zippackage/ZipPackageSink
    package/source/zippackage/ZipPackageStream
)

librestudio_expand_cxx(PACKAGE2_SOURCES)
add_library(package2 SHARED ${PACKAGE2_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(package2 PRIVATE
        "${CMAKE_SOURCE_DIR}/package/inc/pch/precompiled_package2.hxx")
    target_compile_definitions(package2 PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(package2 PRIVATE
    "${CMAKE_SOURCE_DIR}/package/inc"
    "${ZSTD_INCLUDE_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(package2
    officecfg_headers udkapi_normal_headers offapi_normal_headers)

# SYSTEM_ZLIB and ZLIB_CONST are what gb_LinkTarget__use_zlib adds for a system
# zlib, and they are not optional here: without ZLIB_CONST, zlib declares
# next_in as a plain Bytef* and Deflater.cxx and the two inflaters do not
# compile.  sal and tl carry the same pair.
target_compile_definitions(package2 PRIVATE
    DLLIMPLEMENTATION_PACKAGE SYSTEM_ZLIB ZLIB_CONST)
target_link_libraries(package2 PRIVATE
    argon2
    comphelper cppu cppuhelper sal salhelper sax tl ucbhelper utl
    ${LIBRESTUDIO_ZLIB_TARGET} "${ZSTD_LIBRARY}")

librestudio_add_component(package2
    COMPONENT "${CMAKE_SOURCE_DIR}/package/util/package2.component"
    LIBRARY   package2
    LAYER     OOO)

librestudio_set_library_output_name(package2 package2 OOO)
install(TARGETS package2 LIBRARY DESTINATION program)

# --- xstor --------------------------------------------------------------------

set(XSTOR_SOURCES
    package/source/xstor/disposelistener
    package/source/xstor/ocompinstream
    package/source/xstor/ohierarchyholder
    package/source/xstor/oseekinstream
    package/source/xstor/owriteablestream
    package/source/xstor/selfterminatefilestream
    package/source/xstor/switchpersistencestream
    package/source/xstor/xfactory
    package/source/xstor/xstorage
)

librestudio_expand_cxx(XSTOR_SOURCES)
add_library(xstor SHARED ${XSTOR_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(xstor PRIVATE
        "${CMAKE_SOURCE_DIR}/package/inc/pch/precompiled_xstor.hxx")
    target_compile_definitions(xstor PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(xstor PRIVATE
    "${CMAKE_SOURCE_DIR}/package/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(xstor udkapi_normal_headers offapi_normal_headers)

target_link_libraries(xstor PRIVATE
    comphelper cppu cppuhelper sal salhelper tl utl
    Boost::boost)

librestudio_add_component(xstor
    COMPONENT "${CMAKE_SOURCE_DIR}/package/source/xstor/xstor.component"
    LIBRARY   xstor
    LAYER     OOO)

librestudio_set_library_output_name(xstor xstor OOO)
install(TARGETS xstor LIBRARY DESTINATION program)

# --- Package_dtd ---------------------------------------------------------------
#
# Package_dtd.mk: the single DTD the ODF manifest is validated against,
# packaged verbatim into share/dtd/officedocument/1_0 -- the same
# gb_Package_add_file destination and plain install(FILES ...) convention
# already established for framework/CMakeLists.txt's and xmlscript/
# CMakeLists.txt's own Package_dtd.mk ports (see either for the
# LIBO_SHARE_FOLDER derivation; not repeated here). This was the one target
# from Module_package.mk not yet ported (Library_package2/Library_xstor were
# already wired above; verified against the live Module_package.mk, which
# lists exactly these three targets and nothing else).
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/dtd/Manifest.dtd"
    DESTINATION share/dtd/officedocument/1_0)

# CppunitTest_package2_test.mk: registered in package/CppunitTest_
# deferred.cmake, NOT here, include()'d from the root CMakeLists.txt only
# after ucb is add_subdirectory()'d (use_components on ucb1/ucpfile1), well
# after package's own position. See that file for the full per-test detail
# and its include() call site in the root CMakeLists.txt for why it sits
# where it does.

# package/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/package/CppunitTest_deferred.cmake")
