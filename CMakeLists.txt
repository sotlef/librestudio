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
include(LibreStudioUnoApi)
include(LibreStudioJavaJar)
include(LibreStudioComponent)
include(LibreStudioTest)
include(LibreStudioZip)
include(LibreStudioUnorc)
include(LibreStudioSdiTarget)

# createcomponent.xslt and packcomponents.xslt from solenv/bin are driven by
# xsltproc, both here and by the officecfg schemas.
find_program(XSLTPROC_EXECUTABLE xsltproc REQUIRED)

# Third-party libraries that upstream keeps as tarballs rather than vendoring.
# Most are header-only.  Prefer a system copy — configure.ac does the same
# behind --with-system-dragonbox / --with-system-fast_float — and only download
# when there is none.
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

# mdds is header-only as well; svl's grid printer uses it.  configure.ac prefers
# the system module, which installs itself under a versioned directory, so both
# layouts are probed before falling back to the tarball.
find_path(MDDS_INCLUDE_DIR mdds/multi_type_vector/types.hpp
    PATHS /usr/include /usr/local/include /usr/include/mdds-3.0)
if (NOT MDDS_INCLUDE_DIR)
    librestudio_fetch_tarball(mdds mdds-3.2.1.tar.xz
        673f5bb94612dbba581fc92b99b5e5dd1a53e29496a5dbc936432f6b0687c112)
    set(MDDS_INCLUDE_DIR "${mdds_SOURCE_DIR}/include")
endif ()

# zxcvbn-c is the one external here that is compiled rather than only included:
# svl builds it into itself, and the rules for that are next to their only user.
# There is no system module for it — configure.ac has none either.
librestudio_fetch_tarball(zxcvbn_c zxcvbn-c-2.6.tar.gz
    11e39f6776f9c82c68b2acb94336e32697d4ab6cdb4ac16f9583ccbdd735113a)

# argon2 hashes the passwords of password-protected ODF packages.  Its tarball
# ships a Makefile — that is what upstream drives — but this build compiles the
# sources itself, next to the only library that uses them.
librestudio_fetch_tarball(argon2 phc-winner-argon2-20190702.tar.gz
    daf972a89577f8772602bf2eb38b6a3dd3d922bf5724d45e7f9589b5e830442c)

# quickjs is the JavaScript engine jsuno embeds (the JavaScript scripting
# provider); LibreOffice vendors a pre-amalgamated single-file build of it and
# there is no system package to prefer, so like zxcvbn-c and argon2 above it is
# always fetched here and compiled next to its only consumer (jsuno).
librestudio_fetch_tarball(quickjs quickjs-0.17.0-amalgam.zip
    866bad1e2f148b6f8144a4a3d4c3f8a0754495a6920fe56a7dd698cf0ad27f65)

find_package(ZLIB REQUIRED)

# package2 compresses zip entries with zstd as well as with zlib.  On this
# distribution libzstd ships no pkg-config file, so its two halves are found
# directly rather than through pkg_check_modules.
find_path(ZSTD_INCLUDE_DIR zstd.h)
find_library(ZSTD_LIBRARY NAMES zstd)
if (NOT ZSTD_INCLUDE_DIR OR NOT ZSTD_LIBRARY)
    message(FATAL_ERROR "zstd is required by package2 but was not found")
endif ()
find_package(Boost 1.69 REQUIRED COMPONENTS locale)
find_package(Threads REQUIRED)
find_package(ICU 66 REQUIRED COMPONENTS uc i18n)
find_package(PkgConfig REQUIRED)

# JNI headers/libraries and the JDK toolchain (javac, jar) for the optional
# Java part of the UNO core -- jvmfwk, jvmaccess, stoc's javaloader/javavm,
# unoil, ridljar, jurt, javaunohelper, bean -- plus the already-ported
# connectivity/dbtools and vcl/vclplug_* blocks that link jvmaccess behind
# this same option. (javamaker/codemaker_java in codemaker/CMakeLists.txt are
# NOT part of this: they are plain C++ code generators built unconditionally,
# needing neither JNI nor this option -- see the comment there.) Every
# add_subdirectory for the Java-only modules above is itself wrapped in
# `if (LIBRESTUDIO_ENABLE_JAVA)` further down, so this is the only place JNI
# and the JDK need to be looked up.
if (LIBRESTUDIO_ENABLE_JAVA)
    find_package(JNI REQUIRED)
    find_package(Java REQUIRED COMPONENTS Development)
endif ()

# --- java_websocket -------------------------------------------------------
#
# The one Java dependency in the real UNO bridge that is not just a JDK
# feature: ridljar's libreoffice.jar links java_websocket.jar for the
# WebSocket URP transport (com/sun/star/lib/connections/websocket/*,
# gb_Jar_use_externals,libreoffice,java_websocket in Jar_libreoffice.mk).
# Upstream never looks for a system copy -- external/java_websocket always
# vendors and ant-builds it -- so this does the same: fetch the tarball named
# in download.lst, apply the patches UnpackedTarball_java_websocket.mk lists
# (in its order, all at the default patch level 1 -- that file has no
# gb_UnpackedTarball_set_patchlevel call), then run the "jar" ant target that
# ant-build.patch's build.xml adds (javac's src/main/java into build/classes,
# then jars it as dist/java_websocket.jar). ExternalProject_java_websocket.mk's
# -Dant.build.javac.source/-target and -Dbuild.label flags are skipped: the
# patched build.xml (see ant-build.patch above) never references those
# properties, so passing them would be a no-op; -Dant.tstamp.now (reproducible
# builds) and ICECREAM_RUN are likewise skipped as build-environment-specific
# knobs this port does not otherwise implement (same reasoning as skipping
# JVM_ONE_PATH_CHECK in jvmfwk/CMakeLists.txt).
if (LIBRESTUDIO_ENABLE_JAVA)
    find_program(ANT_EXECUTABLE ant REQUIRED)
    find_package(Patch REQUIRED)

    set(JAVA_WEBSOCKET_SHA256 f3ce29efde338707241d47ab4784b19a1e3658bfa0564b5bc5f80c27e5118728)
    librestudio_fetch_tarball(java_websocket Java-WebSocket-1.6.0.tar.gz "${JAVA_WEBSOCKET_SHA256}")

    set(JAVA_WEBSOCKET_PATCHES
        ant-build.patch
        no-slf4j.patch
        0001-cid-1546192-SIC-Inner-class-could-be-made-static.patch
        0001-cid-1545614-UR-Uninitialized-read-of-field-in-constr.patch
        0002-cid-1545284-UR-Uninitialized-read-of-field-in-constr.patch
        0001-cid-1546292-DLS-Dead-local-store.patch
        0001-cid-1545227-Dm-Dubious-method-used.patch
        0001-cid-1545515-Dm-Dubious-method-used.patch
        0001-cid-1546264-Dm-Dubious-method-used.patch
        0001-cid-1546341-Resource-leak-on-an-exceptional-path.patch
        0001-cid-1545249-Bad-bit-shift-operation.patch
        0001-cid-1546344-Dm-Dubious-method-used.patch
        java-websocket-jar-timestamps.patch.1)

    # Applied once, at configure time, right where librestudio_fetch_tarball
    # unpacks -- mirrors that function's own .unpacked-<sha> stamp so a cached,
    # already-patched tree from a previous configure is not re-patched (and
    # does not fail with "already applied") on a later cmake run.
    set(JAVA_WEBSOCKET_PATCHED_STAMP "${java_websocket_SOURCE_DIR}/.patched")
    if (NOT EXISTS "${JAVA_WEBSOCKET_PATCHED_STAMP}")
        foreach (_patch IN LISTS JAVA_WEBSOCKET_PATCHES)
            execute_process(
                COMMAND "${Patch_EXECUTABLE}" -p1 -i
                        "${CMAKE_SOURCE_DIR}/external/java_websocket/patches/${_patch}"
                WORKING_DIRECTORY "${java_websocket_SOURCE_DIR}"
                RESULT_VARIABLE _patch_result)
            if (NOT _patch_result EQUAL 0)
                # A partially-patched tree must never be left behind for the next
                # configure to find: librestudio_fetch_tarball's own
                # .unpacked-<sha256> stamp (cmake/LibreStudioTarballs.cmake) lives
                # one directory up from java_websocket_SOURCE_DIR (in
                # CMAKE_BINARY_DIR/_deps/java_websocket, the extraction root that
                # function derives SOURCE_DIR from) and would otherwise survive
                # this FATAL_ERROR, making a later cmake run skip re-extraction and
                # restart this loop from patch #1 against the same broken tree.
                # Wiping the whole extraction root forces a clean re-extract (from
                # the already-cached tarball, so no re-download) on the next run.
                file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/_deps/java_websocket")
                message(FATAL_ERROR
                    "Failed to apply ${_patch} to java_websocket; source tree reset, re-run cmake to retry")
            endif ()
        endforeach ()
        file(TOUCH "${JAVA_WEBSOCKET_PATCHED_STAMP}")
    endif ()

    # JAVA_HOME=$(JAVA_HOME_FOR_BUILD) in the upstream CustomTarget, minus
    # cross-compiling (not supported by this port): strip bin/javac twice to
    # get the JDK home find_package(Java) resolved above.
    get_filename_component(JAVA_WEBSOCKET_JAVA_HOME "${Java_JAVAC_EXECUTABLE}" DIRECTORY)
    get_filename_component(JAVA_WEBSOCKET_JAVA_HOME "${JAVA_WEBSOCKET_JAVA_HOME}" DIRECTORY)

    include(ExternalProject)
    ExternalProject_Add(java_websocket_ext
        SOURCE_DIR "${java_websocket_SOURCE_DIR}"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND "${CMAKE_COMMAND}" -E env "JAVA_HOME=${JAVA_WEBSOCKET_JAVA_HOME}"
                "${ANT_EXECUTABLE}" -q -f build.xml jar
        BUILD_IN_SOURCE TRUE
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS "${java_websocket_SOURCE_DIR}/dist/java_websocket.jar")

    set(JAVA_WEBSOCKET_JAR "${java_websocket_SOURCE_DIR}/dist/java_websocket.jar")

    # ExternalPackage_java_websocket.mk installs dist/java_websocket.jar to
    # $(LIBO_SHARE_JAVA_FOLDER)/java_websocket.jar -- "program/classes" on
    # this (non-MACOSX/non-WNT) Linux port (configure.ac's else branch).
    install(FILES "${JAVA_WEBSOCKET_JAR}" DESTINATION program/classes)
endif ()

# liblangtag is used directly by i18nlangtag; it pulls libxml2 in through its
# own pkg-config file, which is why the UNO core links libxml2 without using it.
pkg_check_modules(LIBLANGTAG REQUIRED IMPORTED_TARGET liblangtag>=0.4.0)

# tl parses XML with libxml2 directly (tools/source/xml/XmlWalker.cxx).
find_package(LibXml2 REQUIRED)

# sax wraps expat as well as libxml2.
find_package(EXPAT REQUIRED)

# svl's URI and WebDAV helpers speak HTTP through libcurl.
find_package(CURL REQUIRED)

# helpcompiler transforms help XHP through XSLT (libxslt) and builds/searches
# the full-text help index with CLucene (libclucene-core).
pkg_check_modules(LIBXSLT REQUIRED IMPORTED_TARGET libxslt)
pkg_check_modules(LIBCLUCENE_CORE REQUIRED IMPORTED_TARGET libclucene-core)

# slideshow's box2d-based physics transitions. box2d ships a CMake
# package-config (no pkg-config .pc file), and exports an imported target
# rather than variables, so this is find_package rather than
# pkg_check_modules like everything else in this section.
find_package(box2d REQUIRED)

# --- harfbuzz -----------------------------------------------------------------
#
# vcl is the first module that needs harfbuzz, and it needs a recent one:
# hb_ot_fetch_bits() and the HB_OT_BITS_TAG_* constants that vcl/source/font/*
# uses to read the OS/2 and head table bits do not exist before 14.3, and the
# calls are not guarded.  configure.ac names the same floor
# (harfbuzz_required_version=14.3.0) and falls back to bundling the tarball.
#
# The system here is 12.1.0, so this build bundles.  Harfbuzz is a meson
# project, so meson builds it even though CMake is building everything else —
# which is also what upstream does.  Two static libraries come out of it, and
# they are handed to the rest of the build as imported targets, so that the
# consumers say `harfbuzz` and do not have to know how it was made.

find_program(MESON_EXECUTABLE meson REQUIRED)

# Prefer a system harfbuzz that is new enough, the way configure.ac does; the
# tarball is the fallback, not the rule.
pkg_check_modules(HARFBUZZ_SYSTEM harfbuzz>=14.3.0 harfbuzz-subset>=14.3.0)
if (HARFBUZZ_SYSTEM_FOUND)
    set(LIBRESTUDIO_HARFBUZZ_TARGETS PkgConfig::HARFBUZZ_SYSTEM)
else ()
    librestudio_fetch_tarball(harfbuzz harfbuzz-14.5.0.tar.xz
        b7132e148358a45185c9feafd049dbaf243649d3c44414b3534d9c95d18592b9)

    set(HARFBUZZ_SRC "${harfbuzz_SOURCE_DIR}/src")
    set(HARFBUZZ_BUILD "${CMAKE_BINARY_DIR}/harfbuzz-build")

    include(ExternalProject)
    # The options are upstream's: static, subset enabled, ICU built in, and
    # graphite2 from the system; tests and utilities are left out, and
    # auto_features=disabled keeps meson from switching on anything else it
    # happens to find.  -fPIC because these objects end up inside a shared
    # library; PYTHONWARNINGS= because a warning from the interpreter prints as
    # a meson error.
    ExternalProject_Add(harfbuzz_ext
        SOURCE_DIR "${harfbuzz_SOURCE_DIR}"
        BINARY_DIR "${HARFBUZZ_BUILD}"
        CONFIGURE_COMMAND
            "${CMAKE_COMMAND}" -E env "PYTHONWARNINGS="
            "${MESON_EXECUTABLE}" setup --wrap-mode nofallback
                --buildtype release
                -Ddefault_library=static
                -Dauto_features=disabled
                -Dcpp_std=c++20
                -Dtests=disabled
                -Dutilities=disabled
                -Dsubset=enabled
                -Draster=disabled
                -Dvector=disabled
                -Dicu=enabled
                -Dicu_builtin=true
                -Dgraphite2=enabled
                "-Dc_args=-fPIC"
                "-Dcpp_args=-fPIC"
                "${harfbuzz_SOURCE_DIR}"
        BUILD_COMMAND "${MESON_EXECUTABLE}" compile -C "${HARFBUZZ_BUILD}" libs
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "${HARFBUZZ_BUILD}/src/libharfbuzz.a"
            "${HARFBUZZ_BUILD}/src/libharfbuzz-subset.a"
        COMMENT "Building harfbuzz")

    set(LIBRESTUDIO_HARFBUZZ_TARGETS harfbuzz harfbuzz_subset)
    foreach (_hb harfbuzz harfbuzz-subset)
        string(REPLACE "-" "_" _target "${_hb}")
        add_library(${_target} STATIC IMPORTED GLOBAL)
        set_target_properties(${_target} PROPERTIES
            IMPORTED_LOCATION "${HARFBUZZ_BUILD}/src/lib${_hb}.a"
            # The headers are in the unpacked tarball, so they exist as soon as
            # the fetch above has run; only the archives wait for meson.
            INTERFACE_INCLUDE_DIRECTORIES "${HARFBUZZ_SRC}")
        # add_dependencies on an imported target is allowed and does reach the
        # consumers, which is what orders meson before the link.
        add_dependencies(${_target} harfbuzz_ext)
    endforeach ()
endif ()

# --- configuration headers ----------------------------------------------------
#
# Generated once everything above has been found, because several of their
# values are decided by what was found: config_cups.h, config_dbus.h and
# config_gio.h say whether libcups, libdbus and gio are there, and they are
# what vcl/ switches its printing and session handling on.
include(LibreStudioConfigHeaders)

# --- modules -----------------------------------------------------------------
#
# The order is the bootstrap order, not the order the modules appear in the
# source tree: the IDL toolchain in unoidl/ and codemaker/ has to exist before
# any module that consumes generated com/sun/star headers can be compiled.

# pch's precompiled_system donor target (see pch/CMakeLists.txt) has to
# exist before any target elsewhere in the tree calls
# target_precompile_headers(... REUSE_FROM precompiled_system) -- the
# earliest such consumer is sax, so this has to sit above it. It only needs
# Boost::boost, already resolved by this point.
add_subdirectory(pch)

add_subdirectory(sal)
add_subdirectory(salhelper)
add_subdirectory(store)
add_subdirectory(registry)
add_subdirectory(unoidl)
add_subdirectory(codemaker)
add_subdirectory(udkapi)
add_subdirectory(cppu)
add_subdirectory(offapi)

# o3tl's own production target is header-only and IDE-visibility-only (see
# o3tl/CMakeLists.txt) -- by itself it would have no bootstrap ordering
# constraint, nothing depends on it and it depends on nothing. But
# o3tl/CMakeLists.txt now also declares o3tl_tests (CppunitTest_o3tl_tests.mk),
# which needs the cppunittester target (sal/CMakeLists.txt) and the plain
# udkapi_normal/offapi_normal headers targets (gb_CppunitTest_use_sdk_api) --
# both already defined above by this point -- so this add_subdirectory has to
# sit after sal and offapi, not before them.
add_subdirectory(o3tl)

# oovbaapi (VBA-compatibility types) only needs udkapi/offapi's .rdb files,
# same as offapi only needs udkapi's -- declaring it here keeps it with its
# sibling UnoApi modules, well ahead of basic (sb), its first consumer.
add_subdirectory(oovbaapi)
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

# jvmfwk (JRE discovery/bootstrap) and jvmaccess (the JNI attach/AttachGuard
# wrapper built on top of it) are the two lowest-level pieces of the real
# Java/UNO bridge -- both need only cppu, cppuhelper, sal, salhelper (all
# already declared above), and everything further up the Java stack (stoc's
# javaloader/javavm, jurt, javaunohelper, bean) links one or both of them.
# Gated the same way as every other Java-only module in this port.
if (LIBRESTUDIO_ENABLE_JAVA)
    add_subdirectory(jvmfwk)
    add_subdirectory(jvmaccess)
endif ()

# unoil generates the offapi Java stub sources (via javamaker, from codemaker
# above) that ridljar's libreoffice.jar packages directly -- it needs only
# offapi/udkapi's *_RDB variables (both already set by add_subdirectory(udkapi)
# and add_subdirectory(offapi) above) and the javamaker executable, not
# jvmfwk/jvmaccess. Gated the same as the rest of the Java/UNO bridge.
if (LIBRESTUDIO_ENABLE_JAVA)
    add_subdirectory(unoil)
endif ()

# ridljar builds the real UNO Java bridge classes (libreoffice.jar) plus the
# tiny unoloader.jar bootstrap jar. It needs its own javamaker customtarget
# (udkapi only) in addition to unoil's offapi one above, and the
# java_websocket.jar external fetched/built near the top of this file
# (JAVA_WEBSOCKET_JAR), so it must come after both.
if (LIBRESTUDIO_ENABLE_JAVA)
    add_subdirectory(ridljar)
endif ()

# jurt: jpipe, the native JNI pipe-transport library ridljar's PipeConnection.java
# loads via System.loadLibrary. Only needs sal + JNI, not anything above, but
# kept here to match this port's Java/UNO bridge module ordering.
if (LIBRESTUDIO_ENABLE_JAVA)
    add_subdirectory(jurt)
endif ()

# javaunohelper: juh/juhx, the native JNI helper libraries that
# com.sun.star.comp.helper.Bootstrap/SharedLibraryLoader dlopen to bootstrap a
# UNO ComponentContext from Java. juhx needs jvmaccess (built above, alongside
# jvmfwk) and udkapi's comprehensive header variant (built above, in
# udkapi/CMakeLists.txt), not anything from jurt/ridljar.
if (LIBRESTUDIO_ENABLE_JAVA)
    add_subdirectory(javaunohelper)
endif ()

# bean: officebean.jar + libofficebean.so, the last module in this port's
# Java/UNO bridge. officebean.jar's compile classpath needs ridljar's
# libreoffice.jar (libreoffice_JAR/libreoffice_jar, set above); the native
# library needs only JNI/AWT, nothing from jurt/javaunohelper/jvmaccess.
#
# Module_bean.mk: ifneq ($(ENABLE_JAVA),) / ifneq ($(OS),MACOSX) / ifneq
# ($(OS),ANDROID) around gb_Module_add_targets(bean, Jar_officebean
# Library_officebean). The OS exclusions are not just build-system noise --
# bean/native/ is a JNI/AWT peer bridge upstream never ports to macOS or
# Android, so this port must not attempt it there either.
if (LIBRESTUDIO_ENABLE_JAVA AND (NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin") AND (NOT CMAKE_SYSTEM_NAME STREQUAL "Android"))
    add_subdirectory(bean)
endif ()

add_subdirectory(basegfx)
add_subdirectory(tools)
add_subdirectory(unotools)
# idl (svidl only) needs nothing beyond comphelper/tl/sal + the SDK API
# headers, all of which exist by this point; it is a build-time host tool
# (compiles sfx2's *.sdi slot definitions), not a library other modules link.
add_subdirectory(idl)

# l10ntools: 10 host-build tools (l10n text processing -- helpex/idxdict/
# ulfex/cfgex/xrmex/localize/pocheck/propex/treex + the shared transex
# static lib). Nothing in this port currently calls them (their real
# consumers -- HelpTarget/Configuration/Dictionary/Extension l10n machinery
# -- aren't ported), same standalone-until-needed posture idl itself had
# before sfx2. Only needs sal, already available.
add_subdirectory(l10ntools)

# testtools: the UNO bridge test harness (bridgetest client/server +
# cppobj/constructors UNO services), gated in upstream on
# !CROSS_COMPILING (true here). Needs the new RDB-keyed
# librestudio_add_component()/librestudio_finalize_components() (a private
# uno_services.rdb, never installed) and librestudio_uno_api_rdb_file()
# (bridgetest.idl/XPerformanceTest.idl are loose files, not directories) --
# both added to cmake/LibreStudioComponent.cmake / cmake/LibreStudioUnoApi.cmake
# this wave. Needs unoidl-write and udkapi's RDB, both already available.
add_subdirectory(testtools)

add_subdirectory(sot)
add_subdirectory(svl)
add_subdirectory(configmgr)
add_subdirectory(package)
add_subdirectory(xmlscript)
add_subdirectory(docmodel)
add_subdirectory(vcl)
add_subdirectory(toolkit)
add_subdirectory(svtools)
add_subdirectory(canvas)
add_subdirectory(cppcanvas)
# drawinglayer comes after canvas/cppcanvas: drawinglayercore (built here)
# needs nothing from them, but the full drawinglayer library, defined in the
# same CMakeLists.txt, links svt, tk, canvastools and cppcanvas -- declaring
# it here keeps the subdirectory list reading in dependency order even though
# CMake itself does not require it (target_link_libraries resolves target
# names against the whole project, not just what came before it).
add_subdirectory(drawinglayer)

# chart2 (chart2api only) -> xmloff (xo) -> unoxml: xo links chart2api, and
# unoxml links xo, so the three have to be declared in that order.
add_subdirectory(chart2)
add_subdirectory(xmloff)
add_subdirectory(unoxml)

# linguistic (lng) only needs xo (xmloff) beyond what's declared well above it
# -- placed here, next to its one same-tier dependency, well ahead of editeng,
# its first consumer.
add_subdirectory(linguistic)

# framework (fwk) sits on the same tier as toolkit/svtools/vcl, its heaviest
# dependencies -- nothing here depends on chart2/xmloff/unoxml, it is simply
# declared after them because sfx2, its own first consumer, comes later still.
add_subdirectory(framework)

# basic (sb) needs xmlscript (declared much earlier) and oovbaapi (declared
# next to udkapi/offapi above); it has no dependency on fwk, but is declared
# after it since sfx2, the first consumer of both, needs both declared first.
add_subdirectory(basic)

# sfx2 (sfx) is the application-framework tier: it links fwk, sb, drawinglayer,
# docmodel and svidl's compiled slot headers (idl/) -- everything above it has
# to be declared first.
add_subdirectory(sfx2)

# editeng is the shared rich-text editing engine that svx and the app modules
# build on; it needs sfx (declared just above) and lng (declared with xmloff)
# -- everything else it links (xo, drawinglayer, svt, tk, sot, svl, ...) was
# already declared well before either of those.
add_subdirectory(editeng)

# formula (for/forui) is the formula-calculation engine shared by Calc and any
# other formula-capable document; forui needs sfx (declared just above), for
# needs only libraries declared long before this point.
add_subdirectory(formula)

# avmedia (ENABLE_AVMEDIA defaults ON -- see avmedia/CMakeLists.txt's own
# comment for the configure.ac evidence) builds the media-player framework/
# viewer sources plus the five per-backend libraries, each behind its own
# flag -- it only needs libraries declared long before this point, but
# svxcore below links it unconditionally regardless of ENABLE_AVMEDIA (it
# always needs mediaitem.cxx for the media item itself).
add_subdirectory(avmedia)

# connectivity (dbtools only -- see connectivity/CMakeLists.txt) needs utl, tl,
# comphelper, i18nlangtag, svt and vcl, all declared well before this point.
# svxcore/svx link it unconditionally in upstream, since
# HAVE_FEATURE_DBCONNECTIVITY defaults to enabled on desktop Linux.
add_subdirectory(connectivity)

# svx (svxcore + svx) is the drawing/form/gallery engine and its UI-facing
# wrapper; it needs avmedia and editeng (both declared just above) plus sb,
# fwk, sfx and dbtools (all declared earlier).
add_subdirectory(svx)

# emfio, svgio, UnoControls (ctl), animations (animcore) and eventattacher
# (evtatt) are independent of the svx tier above -- each needs only libraries
# (drawinglayer, toolkit, comphelper, cppu, cppuhelper, sal, ...) declared
# long before this point.
add_subdirectory(emfio)
add_subdirectory(svgio)
add_subdirectory(UnoControls)
add_subdirectory(animations)
add_subdirectory(eventattacher)

# forms (frm) is the UNO form-component model; needs dbtools, editeng, sfx,
# svxcore (all declared above). filter (msfilter only, scoped subset) is the
# shared MS-binary-format helper library; needs svxcore, editeng, sfx, xo, sb
# (all declared above).
add_subdirectory(forms)
add_subdirectory(filter)

# scaddins (analysis/date/pricing Calc add-ins) only needs comphelper, cppu,
# cppuhelper, i18nlangtag, sal, utl (all declared above) plus its own private
# UNO API compiled from udkapi/offapi's .rdb files, so it can go anywhere
# after those. scripting (scriptframe/singleprov/protocolhandler/
# stringresource/vbaevents) needs framework, sfx2, toolkit, vcl, basic (sb)
# and filter (msfilter) -- all declared above. ucb (the UCB content-provider
# libraries) needs comphelper, framework, sfx2, svl, ucbhelper, vcl (all
# declared above) plus its own directory-scoped gio-2.0 pkg-config lookup.
# dbaccess needs dbtools (connectivity), svx, sfx2, svtools, xmloff (xo),
# basic (sb) (all declared above) plus a private reproduction of
# connectivity's sqlbison.y parse step for its own generated header.
#
# jsuno (the QuickJS-based JavaScript-to-UNO bridge) only needs comphelper,
# cppu, cppuhelper, sal (all declared above) plus the vendored quickjs
# external it builds for itself, so it goes right before scripting, whose
# jsprov script provider links it.
add_subdirectory(scaddins)
add_subdirectory(jsuno)
add_subdirectory(scripting)
add_subdirectory(ucb)

# Deferred CppunitTest registrations, continued: drawinglayer_processors and
# drawinglayer_border (both CppunitTest_drawinglayer_*.mk) need use_components
# on ucb's ucb1/ucpfile1 (both tests) and framework's fwk (drawinglayer_
# processors only) -- both add_subdirectory()'d only just above/well before
# here (framework at line 517, ucb right above) -- on top of configmgr's and
# i18npool's components, already available at drawinglayer's own position
# (line 501, well before framework/ucb). ucb is the later-binding constraint
# of the two tests' dependencies, so both are registered here, right after
# it, rather than splitting drawinglayer_border off earlier (its own
# dependencies would already be satisfied right after framework, but keeping
# both of drawinglayer's deferred tests in one file with one include() call
# site is simpler, and registering drawinglayer_border a little later than
# strictly necessary costs nothing here) -- same reasoning svl/
# CppunitTest_deferred.cmake's own include() site already applies to
# svl_urihelper/svl_qa_cppunit. See drawinglayer/CppunitTest_deferred.cmake
# for the full per-test detail, including the canvas::vclcanvas/
# canvas::Factory components drawinglayer_processors needs but this port has
# not ported yet, and the pointer comment at the end of drawinglayer/
# CMakeLists.txt for the short version.
include("${CMAKE_SOURCE_DIR}/drawinglayer/CppunitTest_deferred.cmake")

# Deferred CppunitTest registration: package2_test (CppunitTest_package2_
# test.mk) needs use_components on ucb's ucb1/ucpfile1, just add_subdirectory
# ()'d right above -- configmgr/sax/package's own components are already
# available at package's own position (line 493), well before this point.
# See package/CppunitTest_deferred.cmake for the full per-test detail.
include("${CMAKE_SOURCE_DIR}/package/CppunitTest_deferred.cmake")

add_subdirectory(dbaccess)

# vbahelper (vbahelper/msforms) needs msfilter (filter), sb (basic), sfx2,
# svxcore/svx, toolkit, svtools, vcl (all declared above) plus the oovbaapi
# normal-variant headers, same pattern as scripting's vbaevents.
add_subdirectory(vbahelper)

# embeddedobj (embobj/emboleobj) only needs comphelper, cppu, cppuhelper,
# i18nlangtag, sal, svl, svt, tl, utl, vcl (all declared above); emboleobj's
# gb_Library_use_libraries(emboleobj,embobj) is entirely inside an
# ifeq($(OS),WNT) block in Library_emboleobj.mk, so it links no embobj here.
add_subdirectory(embeddedobj)

# reportdesign (rpt/rptui) needs dbtools (connectivity), dbu (dbaccess),
# editeng, framework (fwk), sax, sfx2, svl/svt/svxcore/svx, toolkit (tk), tl,
# utl, vcl, xmloff (xo), formula (for/forui), sot, basegfx, i18nlangtag,
# salhelper (all declared above) -- unblocked now that dbaccess and formula
# are ported. No private UNO API needed.
add_subdirectory(reportdesign)

# extensions (pcr/abp/bib/dbp/log/updatefeed/updchk/updatecheckui -- a scoped
# subset; scn/ldapbe2 need SANE/OpenLDAP dev headers not present in this
# environment, WinUserInfoBe/oleautobridge/so_activex/OOoSpotlightImporter
# are Windows/macOS-only) needs svx/svxcore, editeng, sfx2, svtools, toolkit,
# vcl, svl, unotools, tools, connectivity (dbtools), i18nlangtag, comphelper,
# cppuhelper/cppu/sal/salhelper, sot, officecfg (all declared above) plus
# CURL::libcurl for updchk.
add_subdirectory(extensions)

# oox (OOXML import/export helper library used by writerfilter/sc/sd/sw and,
# below, by starmath) needs basegfx, comphelper, cppu, cppuhelper, docmodel,
# drawinglayer, editeng, i18nlangtag, msfilter (filter), sal, sax, sfx2, svl,
# svt, svx/svxcore, sot, tl, utl, vcl, xmlscript, xo (all declared above) plus
# its own TLS-backend pkg-config lookup (NSS or OpenSSL, mirroring
# comphelper's/svl's switch).
add_subdirectory(oox)

# basctl (the Basic IDE: object browser, code editor, dialog editor) needs
# basegfx, comphelper, cppu, cppuhelper, editeng, framework (fwk), sal,
# i18nlangtag, sb (basic), sfx2, sot, svl/svt/svxcore/svx, toolkit (tk), tl,
# utl, ucbhelper, vcl, xmlscript (all declared above). No private UNO API.
add_subdirectory(basctl)

# cui (the "common UI" dialog library: Tools > Options pages, customize
# dialogs, character map, hyperlink, gallery, spell-check, thesaurus,
# drawing-object tabpages) needs basctl (just declared above) plus basegfx,
# comphelper, cppu, cppuhelper, docmodel, drawinglayer, editeng, framework
# (fwk), i18nlangtag, i18nutil, linguistic (lng), sal, salhelper, sax, sb,
# sfx2, sot, svl/svt/svxcore/svx, toolkit (tk), tl, ucbhelper, utl, vcl (all
# declared above) plus ICU::uc and LibXml2::LibXml2.
add_subdirectory(cui)

# starmath (sm/smd: the Math formula editor) needs oox (just declared above)
# plus comphelper, cppu, cppuhelper, editeng, framework (fwk), i18nutil,
# i18nlangtag, msfilter (filter), sal, salhelper, sax, sfx2, sot, svl/svt/
# svxcore/svx, toolkit (tk), tl, utl, vcl, xmloff (xo) (all declared above).
add_subdirectory(starmath)

# unotest and test are the CppunitTest fixture-helper libraries (bootstrap a
# UNO context, VCL/office bootstrap, XML/HTML diffing, ...) that later
# modules' own unit tests will build against — not something any production
# library here links.  unotest needs comphelper, cppu, cppuhelper, sal, sb,
# tl, utl, vcl; test additionally needs unotest itself plus basegfx, sfx2
# (sfx), svtools (svt), drawinglayer/drawinglayercore (all declared above),
# and pulls in cppunit as an external dependency for the first time in this
# port.
add_subdirectory(unotest)
add_subdirectory(test)

# --- group 2, wave 1 (2026-09-26): small/independent modules -----------------
#
# fpicker, hwpfilter, lotuswordpro, remotebridges, uui need only comphelper,
# cppu, cppuhelper, i18nlangtag, sal, salhelper, sfx2, sot, svl, svtools (svt),
# svx/svxcore, tk (toolkit), tl, ucbhelper, utl, vcl, xmloff (xo) plus ICU and
# Boost (all declared above). All fully ported this wave, no deferred targets.
add_subdirectory(fpicker)
add_subdirectory(hwpfilter)
add_subdirectory(lotuswordpro)
add_subdirectory(remotebridges)
add_subdirectory(uui)

# Deferred CppunitTest registrations: librestudio_add_cppunit_test()'s
# COMPONENTS parameter (cmake/LibreStudioTest.cmake) resolves each name
# through a GLOBAL property that a component's owning module's own
# librestudio_add_component() call sets (cmake/LibreStudioComponent.cmake) --
# read eagerly via get_property(GLOBAL ...), so it only sees whatever has
# been set by the exact point it runs. comphelper_test needs i18npool's
# component, and i18npool is add_subdirectory()'d AFTER comphelper above
# (i18npool links comphelper, so the natural producer-before-consumer order
# runs comphelper first); sax_parser/sax_xmlimport need configmgr's,
# framework's, i18npool's, sfx2's, ucb's, and uui's components, all of which
# are add_subdirectory()'d AFTER sax above, for the same reason (sax is
# linked by many modules that need to be available early); configmgr_unit
# needs unoxml's and ucb's components (both add_subdirectory()'d AFTER
# configmgr above, for the same "linked by many earlier modules" reason) plus
# test_fixtures/its protector plug-ins (add_subdirectory(test), also after).
# Defining these tests at comphelper's/sax's/configmgr's own natural position
# would hit those properties before they are set and abort configure with
# librestudio_add_cppunit_test()'s own FATAL_ERROR -- reordering comphelper's/
# sax's/configmgr's own add_subdirectory() instead would be the much larger,
# riskier change, given how much of the tree already depends on each being
# available early. include()'ing each test's registration here, once every
# module it references has already run, is the smaller, safer fix; see
# comphelper/CppunitTest_deferred.cmake, sax/CppunitTest_deferred.cmake and
# configmgr/CppunitTest_deferred.cmake themselves for the per-test detail.
# (scaddins_analysis and xmlscript_cppunit need the same kind of deferral for
# an additional reason -- the production services.rdb, use_rdb(services) --
# so they are include()'d much further down, right after program_services_rdb
# is defined; see that include() call site instead of here.)
include("${CMAKE_SOURCE_DIR}/comphelper/CppunitTest_deferred.cmake")
include("${CMAKE_SOURCE_DIR}/sax/CppunitTest_deferred.cmake")
include("${CMAKE_SOURCE_DIR}/configmgr/CppunitTest_deferred.cmake")

# group 2, wave 2 (2026-09-26): the five modules wave 1 left blocked on
# missing -devel packages (libxslt/clucene, hunspell/mythes/libexttextcat/
# libnumbertext/hyphen, box2d, librevenge/libwpd/libwps/libodfgen plus the
# wider writerperfect format-library set, libxmlsec1(-nss)) -- all installed
# this session, all five now fully ported.
add_subdirectory(helpcompiler)
add_subdirectory(lingucomponent)
add_subdirectory(slideshow)
add_subdirectory(writerperfect)
add_subdirectory(xmlsecurity)

# Deferred CppunitTest registrations, continued: svl_urihelper and
# svl_qa_cppunit (both CppunitTest_svl_*.mk) need use_components on ucb's
# ucb1/ucpfile1 (svl_urihelper) and configmgr's/framework's/lingucomponent's
# own components (svl_qa_cppunit), plus both need use_ure's protector
# plug-ins from unotest -- all add_subdirectory()'d after svl's own, above.
# lingucomponent (just add_subdirectory()'d above) is the later-binding
# constraint of the two tests' dependencies, so both are registered together
# here, right after it, rather than splitting svl_urihelper off to the
# earlier comphelper/sax/configmgr deferral point above (its own
# dependencies would already be satisfied there, but keeping both of svl's
# deferred tests in one file with one include() call site is simpler, and
# registering svl_urihelper a little later than strictly necessary costs
# nothing here). See svl/CppunitTest_deferred.cmake for the full per-test
# detail, and the pointer comment in svl/CMakeLists.txt for the short
# version.
include("${CMAKE_SOURCE_DIR}/svl/CppunitTest_deferred.cmake")

# toolkit/CppunitTest_deferred.cmake's include() used to sit here (right
# after svl's own, above): toolkit_toolkit (CppunitTest_toolkit.mk) needs
# use_components on framework's fwk (517), sfx2's sfx (527) and ucb's
# ucb1/ucpfile1 (592) -- all add_subdirectory()'d after toolkit's own natural
# position (491) -- plus use_configuration's dependency on test (682), also
# after toolkit, and none of that needs program_services_rdb. But that file
# now also registers toolkit_a11y (CppunitTest_toolkit_a11y.mk), whose
# use_rdb(services) does need program_services_rdb -- so the whole include()
# moved down to right after that target is defined (see below, alongside
# scaddins_analysis/xmlscript_cppunit), rather than splitting the two
# toolkit tests across two files. See toolkit/CppunitTest_deferred.cmake's
# own header comment for the full detail.

# sdext: only PresentationMinimizer and pdfimport are ported. xpdfimport (the
# separate poppler-based helper executable pdfimport pipes to) is deferred —
# this machine has no poppler pkg-config/headers, only poppler-data.
add_subdirectory(sdext)

# pyuno needs python3-embed (present); fully ported (pyuno, pythonloader,
# pyuno_wrapper).
add_subdirectory(pyuno)

# shell: desktopbe1, localebe1, losessioninstall (behind ENABLE_GIO, which is
# genuinely tracked in LibreStudioConfigHeaders.cmake), shell_xmlparser,
# syssh, uri-encode, cmdmail are ported. kf5be1 is deferred — no ENABLE_KF5
# switch exists in this port's config surface and no Qt5/KF5 pkg-config is
# present on this machine.
add_subdirectory(shell)

# sd: sd, sdui and sdd are all ported (textconversiondlgs/icg blockers
# resolved, then Library_sd/Library_sdui written in full).
# external/clew (a small, fully self-contained, already-vendored OpenCL
# dynamic-loading shim -- one .c file, no network fetch, no deps beyond
# dlfcn.h) and opencl (Library_opencl, Executable_opencltest) are not part
# of this port's DESKTOP BUILD_TYPE upstream, but ENABLE_OPENCL (default ON,
# cmake/LibreStudioConfigHeaders.cmake) turns them on unconditionally here --
# both are real, locally-buildable targets. Must come before sc, which links
# opencl+clew for its own sc/source/core/opencl/* block once this exists.
add_subdirectory(external/clew)
add_subdirectory(opencl)

add_subdirectory(sd)

# sc: sc, scfilt, scui, solver, vbaobj and scd are all ported
# (textconversiondlgs/md4c/orcus blockers resolved, then Library_sc and its
# satellites written in full).
add_subdirectory(sc)

# sw: sw, swui, msword, vbaswobj and swd are all ported (textconversiondlgs/
# md4c blockers resolved, then Library_sw and its satellites written in
# full). sw_writerfilter remains separately deferred — its CustomTarget_source
# (OOXML factory generator) needs its own CMake design.
add_subdirectory(sw)

# cppuhelper is added last: it consumes the comprehensive header variants of
# udkapi, offapi and its own private registry, all of which have to be declared
# before it.

# group 3 (2026-09-26): the two modules that depend on group 2 -- xmlhelp
# needs helpcompiler's helplinker, desktop needs sc+sw (only indirectly, via
# nothing desktop itself links, but sc/sw being finished is what unblocked
# treating the whole dependency graph as closed). Both fully ported. desktop
# must be added before librestudio_finalize_components(services) below --
# its 7 .component files need to make it into services.rdb.
add_subdirectory(xmlhelp)
add_subdirectory(desktop)

# libreofficekit: the GTK tiled-rendering demo (libreofficekitgtk,
# gtktiledviewer) and a benchmark tool (tilebench). The LOK C API itself is
# already in desktop's sofficeapp; these consume it via dlopen/dlsym at
# runtime (LibreOfficeKitInit.h), not a CMake link dependency -- placed
# after desktop only because LOK_PATH is meant to point at the directory
# sofficeapp installs to, not because anything here links it.
add_subdirectory(libreofficekit)

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

# Deferred CppunitTest registration: officecfg_accelerator_key_checker_test
# (CppunitTest_officecfg_accelerator_key_checker_test.mk) calls
# gb_CppunitTest_use_rdb(name,services) -- see scaddins/CppunitTest_
# deferred.cmake's header comment for what this port makes of that (no
# COMPONENTS, a DEPENDS on the program_services_rdb target just defined
# above, and cppunittester's UNO bootstrap falling back to unorc's own ini
# default for UNO_SERVICES) -- so it cannot be registered before this exact
# point, the first place program_services_rdb exists as a real target. Its
# other dependencies (officecfg itself, subsequenttest/test_fixtures/the
# protector plug-ins via test, unotest) are all add_subdirectory()'d long
# before this point already, so program_services_rdb is the binding
# constraint, not those. See officecfg/CppunitTest_deferred.cmake for the
# full per-test detail, including why this could not simply be registered at
# officecfg/CMakeLists.txt's own position (390) instead.
include("${CMAKE_SOURCE_DIR}/officecfg/CppunitTest_deferred.cmake")

# Deferred CppunitTest registrations, continued: scaddins_analysis
# (CppunitTest_scaddins_analysis.mk) and xmlscript_cppunit
# (CppunitTest_xmlscript_cppunit.mk) both call gb_CppunitTest_use_rdb(name,
# services) -- see scaddins/CppunitTest_deferred.cmake's header comment for
# what this port makes of that (no COMPONENTS, a DEPENDS on the
# program_services_rdb target just defined above, and cppunittester's UNO
# bootstrap falling back to unorc's own ini default for UNO_SERVICES) -- so
# neither can be registered before this exact point, the first place
# program_services_rdb exists as a real target. Both also need framework/
# test (xmlscript_cppunit) or test alone (scaddins_analysis), add_subdirectory
# ()'d long before this point already, so program_services_rdb is the binding
# constraint for both, not those.
include("${CMAKE_SOURCE_DIR}/scaddins/CppunitTest_deferred.cmake")
include("${CMAKE_SOURCE_DIR}/xmlscript/CppunitTest_deferred.cmake")

# Deferred CppunitTest registration: toolkit_toolkit (CppunitTest_toolkit.mk)
# and toolkit_a11y (CppunitTest_toolkit_a11y.mk), toolkit/CppunitTest_
# deferred.cmake's combined include() call site. toolkit_toolkit itself does
# not need program_services_rdb (it uses explicit COMPONENTS instead of
# use_rdb -- see that file's own comment on it) and could have been
# registered as early as right after svl's own deferred include above (its
# framework/sfx2/ucb/test dependencies are all satisfied there already), but
# toolkit_a11y -- now wired into the same file -- does call
# gb_CppunitTest_use_rdb(toolkit_a11y,services), which needs the
# program_services_rdb target just defined above. Since both tests share one
# file and one include() call site, the whole include() sits here, at the
# later-binding test's constraint, rather than splitting the file in two;
# see toolkit/CppunitTest_deferred.cmake's own header comment for the full
# detail.
include("${CMAKE_SOURCE_DIR}/toolkit/CppunitTest_deferred.cmake")

# vcl_app_test, vcl_bitmapfilter_test, vcl_bitmap_test, vcl_filter_igif,
# vcl_graphic_test, vcl_idle_task, vcl_map, vcl_type_serializer_test: the
# first sub-wave of vcl's CppunitTest_*.mk files that need use_ure. All eight
# call gb_CppunitTest_use_rdb(name, services) same as scaddins_analysis/
# xmlscript_cppunit just above, so they need the same program_services_rdb
# ordering and are include()'d from the same point; see vcl/
# CppunitTest_deferred.cmake's own header comment for the per-test detail,
# including why they could not be registered at vcl's own add_subdirectory()
# position either. vcl_localize_digits/vcl_unit_conversion (no use_ure at
# all) and vcl_pdfium_library_test (deliberately not wired -- see that same
# file) are not part of this include().
include("${CMAKE_SOURCE_DIR}/vcl/CppunitTest_deferred.cmake")

# Deferred CppunitTest registration: svtools_graphic (CppunitTest_svtools_
# graphic.mk) calls use_rdb(services) same as scaddins_analysis/xmlscript_
# cppunit/vcl's own eight tests above, so it needs the same program_
# services_rdb ordering, plus its use_libraries(...,sw,...) needs sw's own
# add_subdirectory() (line 809, already satisfied well before this point) to
# have run for that link to resolve as a real target dependency rather than a
# bare linker flag. See svtools/CppunitTest_deferred.cmake for the full
# per-test detail, including why svtools_dialogs_test (a screenshot target)
# is not wired at all, and svtools/CMakeLists.txt's own comment for the
# pointer to CppunitTest_svtools_html (wired directly there, no deferral
# needed).
include("${CMAKE_SOURCE_DIR}/svtools/CppunitTest_deferred.cmake")

# Deferred CppunitTest registrations: svx_unit, svx_gallery_test and svx_core
# (all three of CppunitTest_svx_unit.mk/CppunitTest_svx_gallery_test.mk/
# CppunitTest_svx_core.mk) call use_rdb(services) same as scaddins_analysis/
# xmlscript_cppunit/vcl's/svtools_graphic's own tests above, so they need the
# same program_services_rdb ordering, plus unotest/test_fixtures/
# subsequenttest (test (688) and unotest (687) are both add_subdirectory()'d
# after svx's own position (560), but program_services_rdb is the later-
# binding constraint for all three anyway). See svx/CppunitTest_deferred.cmake
# for the full per-test detail, including why svx_dialogs_test (a screenshot
# target) is not wired at all -- and why UIConfig_svx now existing does not
# change that verdict.
include("${CMAKE_SOURCE_DIR}/svx/CppunitTest_deferred.cmake")

# Deferred CppunitTest registrations: sfx2_misc, sfx2_classification,
# sfx2_doc and sfx2_autoredaction (CppunitTest_sfx2_misc.mk/CppunitTest_
# sfx2_classification.mk/CppunitTest_sfx2_doc.mk/CppunitTest_sfx2_
# autoredaction.mk) all call use_rdb(services) same as svx_unit/svx_
# gallery_test/svx_core just above, so they need the same program_
# services_rdb ordering, plus unotest/test_fixtures/subsequenttest (test,
# unotest -- both add_subdirectory()'d after sfx2's own position, but
# program_services_rdb is the later-binding constraint for all four
# anyway) and, via CONFIGURATION, test/CMakeLists.txt's own LIBRESTUDIO_
# TEST_UNITTEST_DIR. See sfx2/CppunitTest_deferred.cmake for the full
# per-test detail, including why sfx2_view is not wired at all (a second,
# independent blocker beyond subsequenttest: use_executable(xpdfimport),
# which this port has never ported) and the sfx2_classification/
# sfx2_classification_package target-name-collision fix it documents.
include("${CMAKE_SOURCE_DIR}/sfx2/CppunitTest_deferred.cmake")

# Deferred CppunitTest registration: all 5 of framework's CppunitTest_*.mk
# files (framework_dispatch, framework_loadenv, framework_CheckXTitle,
# framework_accelerators, framework_services) call use_rdb(services) same as
# scaddins_analysis/xmlscript_cppunit/vcl's own eight tests above, so they
# need the same program_services_rdb ordering, plus use_configuration's own
# LIBRESTUDIO_TEST_UNITTEST_DIR (test/CMakeLists.txt, add_subdirectory()'d
# well before this point too). See framework/CppunitTest_deferred.cmake for
# the full per-test detail, including framework_services' if (NOT APPLE)
# guard (Module_framework.mk's own ifneq($(OS),MACOSX), "Not sure why this is
# not stable on macOS").
include("${CMAKE_SOURCE_DIR}/framework/CppunitTest_deferred.cmake")

# Deferred CppunitTest registration: linguistic_restprotocol (CppunitTest_
# linguistic_restprotocol.mk) calls use_rdb(services) same as scaddins_
# analysis/xmlscript_cppunit/vcl's/framework's own tests above, so it needs
# the same program_services_rdb ordering, plus use_configuration's own
# LIBRESTUDIO_TEST_UNITTEST_DIR (test/CMakeLists.txt, add_subdirectory()'d
# well before this point too). See linguistic/CppunitTest_deferred.cmake for
# the full per-test detail, including the Module_linguistic.mk `ifeq
# ($(OS),LINUX)` gate this test sits behind upstream.
include("${CMAKE_SOURCE_DIR}/linguistic/CppunitTest_deferred.cmake")

# Deferred CppunitTest registration: forms_unit (CppunitTest_forms_unit.mk)
# calls use_rdb(services) same as linguistic_restprotocol just above, so it
# needs the same program_services_rdb ordering, plus use_configuration's own
# LIBRESTUDIO_TEST_UNITTEST_DIR. See forms/CppunitTest_deferred.cmake for the
# full per-test detail.
include("${CMAKE_SOURCE_DIR}/forms/CppunitTest_deferred.cmake")

# Deferred CppunitTest registration: reportdesign_dialogs_test
# (CppunitTest_reportdesign_dialogs_test.mk, a screenshot target) calls
# use_rdb(services) + use_vcl_non_headless_with_windows + use_uiconfigs same
# as basctl_dialogs_test/svtools_dialogs_test/svx_dialogs_test/vcl_
# dialogs_test's own NONHEADLESS+UICONFIG wiring, so it needs the same
# program_services_rdb ordering plus use_configuration's own LIBRESTUDIO_
# TEST_UNITTEST_DIR. See reportdesign/CppunitTest_deferred.cmake for the full
# per-test detail, including why reportdesign_basic_test (the module's other
# CppunitTest) is not wired at all.
include("${CMAKE_SOURCE_DIR}/reportdesign/CppunitTest_deferred.cmake")

# Deferred CppunitTest registrations: extensions_bibliography
# (CppunitTest_extensions_bibliography.mk) and extensions_test_update
# (CppunitTest_extensions_test_update.mk, gated on ENABLE_ONLINE_UPDATE) both
# call use_rdb(services)/need program_services_rdb the same way, plus
# extensions_test_update's use_configuration needs LIBRESTUDIO_TEST_
# UNITTEST_DIR too. See extensions/CppunitTest_deferred.cmake for the full
# per-test detail, including extensions_test_update's use_library_objects
# (updchk) handling.
include("${CMAKE_SOURCE_DIR}/extensions/CppunitTest_deferred.cmake")

# Deferred CppunitTest registration: basctl_dialogs_test (CppunitTest_basctl_
# dialogs_test.mk, a screenshot target) -- same NONHEADLESS+UICONFIG
# treatment as reportdesign_dialogs_test just above, needing the same
# program_services_rdb + LIBRESTUDIO_TEST_UNITTEST_DIR ordering. See basctl/
# CppunitTest_deferred.cmake for the full per-test detail.
include("${CMAKE_SOURCE_DIR}/basctl/CppunitTest_deferred.cmake")

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

# basic (sb) instantiates real UNO objects that speak ooo.vba.* interfaces at
# VBA-macro runtime, so the service manager's reflection needs this registry
# the same way it needs offapi's for com.sun.star interfaces.
add_custom_command(
    OUTPUT "${LIBRESTUDIO_PROGRAM_DIR}/types/oovbaapi.rdb"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${LIBRESTUDIO_PROGRAM_DIR}/types"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${oovbaapi_RDB}" "${LIBRESTUDIO_PROGRAM_DIR}/types/oovbaapi.rdb"
    DEPENDS "${oovbaapi_RDB}" oovbaapi_rdb
    COMMENT "Installing types/oovbaapi.rdb"
    VERBATIM)
add_custom_target(program_oovbaapi_rdb ALL
    DEPENDS "${LIBRESTUDIO_PROGRAM_DIR}/types/oovbaapi.rdb")

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
