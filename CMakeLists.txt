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
include(LibreStudioPackage)
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

# zlib itself: resolved lazily by whichever real consumer needs it first
# (comphelper/hwpfilter/package/sal/tools/shell/writerperfect/sdext, each
# calling librestudio_find_package(NAME ZLIB ...) -- cmake/
# LibreStudioTarballs.cmake) into LIBRESTUDIO_ZLIB_TARGET -- no dedicated
# entry point here anymore.

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
#
# --- java_websocket ---------------------------------------------------------
#
# The one Java dependency in the real UNO bridge that is not just a JDK
# feature: ridljar's libreoffice.jar links java_websocket.jar for the
# WebSocket URP transport (com/sun/star/lib/connections/websocket/*,
# gb_Jar_use_externals,libreoffice,java_websocket in Jar_libreoffice.mk).
# Upstream never looks for a system copy -- external/java_websocket always
# vendors it -- so this does the same: fetch the tarball named in
# download.lst, apply the patches UnpackedTarball_java_websocket.mk lists (in
# order, all at the default patch level 1).
#
# 2026-09-27: built with librestudio_add_uno_jar() (cmake/LibreStudioJavaJar.
# cmake) instead of upstream's own ant invocation. Verified directly (fetched
# and read the real, patched build.xml -- see ant-build.patch below): that
# build.xml is not upstream's own, it is LibreOffice's OWN fabrication (the
# upstream project ships no build.xml of its own at all), and its entire
# content is exactly `javac src/main/java -> build/classes` (excluding one
# file) then `jar` -- precisely what librestudio_add_uno_jar() already does
# natively, no ant needed. This also means `ant` was, in effect, never a real
# requirement for java_websocket at all -- only hsqldb (below) still is.
if (LIBRESTUDIO_ENABLE_JAVA)
    find_package(JNI REQUIRED)
    find_package(Java REQUIRED COMPONENTS Development)
    find_package(Patch REQUIRED)

    set(JAVA_WEBSOCKET_SHA256 f3ce29efde338707241d47ab4784b19a1e3658bfa0564b5bc5f80c27e5118728)
    librestudio_fetch_tarball(java_websocket Java-WebSocket-1.6.0.tar.gz "${JAVA_WEBSOCKET_SHA256}")

    # ant-build.patch itself (LibreOffice's own fabricated build.xml) is
    # skipped from this list -- there is nothing left for it to patch now
    # that this port never invokes ant/reads build.xml at all; every other
    # patch here is a real upstream source fix, still needed.
    set(JAVA_WEBSOCKET_PATCHES
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
        0001-cid-1546344-Dm-Dubious-method-used.patch)
    # java-websocket-jar-timestamps.patch.1 (a reproducible-build timestamp
    # tweak to the now-unused build.xml itself) is dropped for the same
    # reason ant-build.patch is.

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

    # PerMessageDeflateExtension.java is ant-build.patch's own <exclude> --
    # reproduced directly rather than inherited from the now-unused build.xml.
    file(GLOB_RECURSE JAVA_WEBSOCKET_SOURCES "${java_websocket_SOURCE_DIR}/src/main/java/*.java")
    list(FILTER JAVA_WEBSOCKET_SOURCES EXCLUDE REGEX "PerMessageDeflateExtension\\.java$")

    librestudio_add_uno_jar(java_websocket
        PACKAGEROOT org
        SOURCES ${JAVA_WEBSOCKET_SOURCES})

    # ExternalPackage_java_websocket.mk installs dist/java_websocket.jar to
    # $(LIBO_SHARE_JAVA_FOLDER)/java_websocket.jar -- "program/classes" on
    # this (non-MACOSX/non-WNT) Linux port (configure.ac's else branch).
    install(FILES "${java_websocket_JAR}" DESTINATION program/classes)

    # --- hsqldb -----------------------------------------------------------
    #
    # connectivity/Jar_sdbc_hsqldb.mk's own external/qa dependency: the
    # bundled HSQLDB 1.8.0 database engine, java-side JDBC driver for
    # connectivity's embedded-database "HSQLDB" driver (needed by
    # reportdesign_basic_test). Not tracked as a system-package alternative
    # in this port (matches upstream's own !SYSTEM_HSQLDB default). external/
    # hsqldb's own 4 .mk files, re-checked directly: UnpackedTarball_hsqldb.mk
    # fetches $(HSQLDB_TARBALL) (download.lst), fixes CRLF line endings on 3
    # files the patches below touch, then applies 13 patches at patchlevel 2
    # (a purely-numeric filename suffix, e.g. applet.patch.0, overrides that
    # per-patch -- solenv/gbuild/UnpackedTarball.mk's own `case` logic,
    # re-derived directly against a real download+patch run: a blanket -p2
    # made applet.patch.0 fail before this was found); ExternalPackage_
    # hsqldb.mk installs the resulting lib/hsqldb.jar to program/classes.
    #
    # 2026-09-27: unlike java_websocket, hsqldb's OWN upstream build.xml (not
    # a LibreOffice fabrication this time -- a real, large, 1180-line ant
    # project fetched and read directly) does genuine work ant alone was
    # doing: `ant jar` depends on target "hsqldb" -> "classes14,util,swing" ->
    # ... -> "switchtojdk16" -> "switchtojdk14" -> "switches" -> "codeswitcher",
    # which compiles and RUNS org/hsqldb/util/CodeSwitcher.java, a tool that
    # MUTATES specific .java sources in place, toggling comment-delimited
    # +LABEL/-LABEL code blocks to select JDK-version-specific code paths
    # (this 2008-era codebase's own portability mechanism, predating the
    # "one .class per javac -source/-target" approach). Re-derived from the
    # live build.xml which switchtojdk* branches actually fire on any modern
    # JDK: javaversion6's `<available classname="java.net.IDN"/>` probe
    # (IDN exists since Java 6) is satisfied unconditionally today, setting
    # ant.java.iscjava16=true and -- via each javaversionN target's own
    # `unless="ant.java.iscjava<N+2>"` guard -- permanently skipping the
    # iscjava12/iscjava14 probes and their switchtojdk13/switchtojdk14
    # branches (their own bodies additionally gated `if="ant.java.iscjava..`
    # on a property that consequently never gets set). Net effect,
    # independent of which specific modern JDK builds this port: exactly two
    # CodeSwitcher invocations ever run -- "switchtojdk11" (unconditional
    # upstream) then "switchtojdk16" -- reproduced as exactly two java
    # invocations in cmake/hsqldb-build.sh.in below, with every intermediate,
    # unreachable-on-a-modern-JDK branch simply omitted rather than ported.
    #
    # The remaining store/lib/classes/classes14/util(/swing) compile stages
    # are various narrow <include>/<exclude> filesets over the SAME org/
    # hsqldb source tree, compiled into one shared classdir -- rather than
    # mirror each narrow fileset exactly (classes14's own reachability is
    # genuinely ambiguous the same way: `if="ant.java.iscjava14"`, a property
    # this session's own analysis above shows is never set on a modern JDK
    # either, so upstream's *own* ant build may or may not still compile
    # those 6 files today -- not a question this port's CMake needs to
    # answer identically to resolve it correctly), this port instead compiles
    # the whole org/hsqldb source tree in one javac invocation (file(GLOB_
    # RECURSE) below, discovered fresh after the patch step, since the
    # tarball is fetched/patched at configure time and the files already
    # exist on disk by this point) -- a safe superset of whatever the various
    # staged filesets would produce between them, since nothing in this tree
    # depends on any file NOT being compiled. Excluded for real, specific
    # reasons (not narrowed arbitrarily): org/hsqldb/test/** (needs junit.jar,
    # never vendored, and confirmed -- grepped -- referenced by nothing
    # outside itself); org/hsqldb/sample/** (demo/example code, excluded from
    # every real packaging target in the live build.xml); and the Swing-based
    # util classes (*Swing.java, TableSorter.java -- this port's own
    # equivalent of upstream's own -Dnoswing=true flag, already passed by the
    # ant invocation this replaces).
    #
    # download.lst's own HSQLDB_TARBALL/HSQLDB_SHA256SUM values, verified
    # directly (`grep -n hsqldb download.lst`).
    set(HSQLDB_TARBALL "17410483b5b5f267aa18b7e00b65e6e0-hsqldb_1_8_0.zip")
    set(HSQLDB_SHA256SUM "d30b13f4ba2e3b6a2d4f020c0dee0a9fb9fc6fbcc2d561f36b78da4bf3802370")
    librestudio_fetch_tarball(hsqldb "${HSQLDB_TARBALL}" "${HSQLDB_SHA256SUM}")

    set(HSQLDB_EOL_FIX_FILES
        build/build.xml
        src/org/hsqldb/Library.java
        src/org/hsqldb/persist/HsqlDatabaseProperties.java)
    foreach (_eol_file IN LISTS HSQLDB_EOL_FIX_FILES)
        set(_eol_path "${hsqldb_SOURCE_DIR}/${_eol_file}")
        if (EXISTS "${_eol_path}")
            file(READ "${_eol_path}" _eol_content)
            string(REPLACE "\r\n" "\n" _eol_content "${_eol_content}")
            file(WRITE "${_eol_path}" "${_eol_content}")
        endif ()
        unset(_eol_path)
        unset(_eol_content)
    endforeach ()

    set(HSQLDB_PATCHES
        i96823.patch
        i97032.patch
        i103528.patch
        i104901.patch
        fdo36824.patch
        limit_as_table_alias.patch
        hsqldb-runFinalizersOnExit.patch
        jdbc-4.1.patch
        multipleResultSets.patch
        disable-dump-script.patch
        abstract.patch
        applet.patch.0
        hsqldb-jar-timestamps.patch.1)

    set(HSQLDB_PATCHED_STAMP "${hsqldb_SOURCE_DIR}/.patched")
    if (NOT EXISTS "${HSQLDB_PATCHED_STAMP}")
        foreach (_patch IN LISTS HSQLDB_PATCHES)
            # solenv/gbuild/UnpackedTarball.mk's own patch-application loop
            # (`s=$${p##*.}; case "$$s" in [0-9]) pl="$$s"; ;; esac`): a
            # purely-numeric filename SUFFIX overrides the module's default
            # patch level (2, gb_UnpackedTarball_set_patchlevel(hsqldb,2))
            # for that one patch -- applet.patch.0 (level 0) and hsqldb-
            # jar-timestamps.patch.1 (level 1) both need this; discovered
            # directly when a blanket -p2 made applet.patch.0 fail to find
            # its own target file. Every other patch here keeps a plain
            # ".patch" extension (non-numeric), so falls through to the
            # default level 2.
            get_filename_component(_patch_suffix "${_patch}" LAST_EXT)
            string(REPLACE "." "" _patch_suffix "${_patch_suffix}")
            if (_patch_suffix MATCHES "^[0-9]+$")
                set(_patch_level "${_patch_suffix}")
            else ()
                set(_patch_level "2")
            endif ()
            execute_process(
                COMMAND "${Patch_EXECUTABLE}" -f -s "-p${_patch_level}" --fuzz=0 -i
                        "${CMAKE_SOURCE_DIR}/external/hsqldb/patches/${_patch}"
                WORKING_DIRECTORY "${hsqldb_SOURCE_DIR}"
                RESULT_VARIABLE _patch_result)
            unset(_patch_suffix)
            unset(_patch_level)
            if (NOT _patch_result EQUAL 0)
                # Same reasoning as java_websocket's identical guard above:
                # wipe the whole extraction root so the next configure
                # re-extracts cleanly (from the already-cached tarball, no
                # re-download) instead of restarting this loop against an
                # already-half-patched tree.
                file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/_deps/hsqldb")
                message(FATAL_ERROR
                    "Failed to apply ${_patch} to hsqldb; source tree reset, re-run cmake to retry")
            endif ()
        endforeach ()
        file(TOUCH "${HSQLDB_PATCHED_STAMP}")
    endif ()

    set(HSQLDB_SRC_DIR "${hsqldb_SOURCE_DIR}/src")
    set(HSQLDB_CLASSDIR "${CMAKE_BINARY_DIR}/hsqldb-classes")
    set(HSQLDB_CODESWITCHER_CLASSDIR "${CMAKE_BINARY_DIR}/hsqldb-codeswitcher-classes")
    set(HSQLDB_CODESWITCHER_SRC "${HSQLDB_SRC_DIR}/org/hsqldb/util/CodeSwitcher.java")
    set(HSQLDB_SWITCHED_STAMP "${hsqldb_SOURCE_DIR}/.switched")
    set(HSQLDB_SERVLET_JAR "${hsqldb_SOURCE_DIR}/lib/servlet.jar")
    set(HSQLDB_JAR_DIR "${hsqldb_SOURCE_DIR}/lib")
    set(HSQLDB_JAR "${HSQLDB_JAR_DIR}/hsqldb.jar")

    file(GLOB_RECURSE HSQLDB_MAIN_SOURCES "${HSQLDB_SRC_DIR}/org/hsqldb/*.java")
    list(FILTER HSQLDB_MAIN_SOURCES EXCLUDE REGEX "/org/hsqldb/test/")
    list(FILTER HSQLDB_MAIN_SOURCES EXCLUDE REGEX "/org/hsqldb/sample/")
    list(FILTER HSQLDB_MAIN_SOURCES EXCLUDE REGEX "/org/hsqldb/util/[A-Za-z]*Swing\\.java$")
    list(FILTER HSQLDB_MAIN_SOURCES EXCLUDE REGEX "/org/hsqldb/util/TableSorter\\.java$")

    # A response file, not a bare command-line argument list: 240+ absolute
    # paths risk exceeding a real exec argv/shell line-length limit, the same
    # reason solenv/gbuild/JavaClassSet.mk's own gb_JavaClassSet__command
    # always routes its own (potentially huge) per-target source list through
    # a RESPONSEFILE rather than the command line directly.
    set(HSQLDB_MAIN_SOURCES_RSP "${CMAKE_BINARY_DIR}/hsqldb-main-sources.rsp")
    set(_hsqldb_rsp_content "")
    foreach (_src IN LISTS HSQLDB_MAIN_SOURCES)
        string(APPEND _hsqldb_rsp_content "\"${_src}\"\n")
    endforeach ()
    file(GENERATE OUTPUT "${HSQLDB_MAIN_SOURCES_RSP}" CONTENT "${_hsqldb_rsp_content}")
    unset(_hsqldb_rsp_content)

    # The exact file lists from build.xml's own "switchtojdk11"/"switchtojdk16"
    # targets (org/hsqldb/-relative paths there, expanded to absolute paths
    # here); duplicate entries within switchtojdk16's own upstream list
    # (RowInputTextLog.java and SimpleLog.java each appear twice there) are
    # kept as-is -- CodeSwitcher re-processing the same already-switched file
    # a second time in the same invocation is a harmless no-op, and this
    # keeps the list a literal transcription rather than a silently "cleaned
    # up" one.
    set(HSQLDB_SWITCH11_FILES
        lib/java/JavaSystem.java lib/HsqlTimer.java jdbc/jdbcStatement.java
        persist/LockFile.java persist/Logger.java util/DatabaseManager.java
        util/ConnectionDialogCommon.java)
    set(HSQLDB_SWITCH16_FILES
        lib/java/JavaSystem.java lib/HsqlTimer.java jdbc/jdbcBlob.java
        jdbc/jdbcDatabaseMetaData.java jdbc/jdbcDataSource.java
        jdbc/jdbcCallableStatement.java jdbc/jdbcClob.java jdbc/jdbcConnection.java
        jdbc/jdbcParameterMetaData.java jdbc/jdbcPreparedStatement.java
        jdbc/jdbcResultSet.java jdbc/jdbcResultSetMetaData.java
        jdbc/jdbcStatement.java persist/LockFile.java persist/Logger.java
        persist/ScaledRAFile.java rowio/RowInputTextLog.java
        util/DatabaseManager.java util/ConnectionDialogCommon.java
        lib/SimpleLog.java rowio/RowInputTextLog.java lib/SimpleLog.java
        rowio/RowInputTextLog.java HsqlDateTime.java)

    set(HSQLDB_SWITCH11_ARGS "")
    foreach (_f IN LISTS HSQLDB_SWITCH11_FILES)
        string(APPEND HSQLDB_SWITCH11_ARGS "\"${HSQLDB_SRC_DIR}/org/hsqldb/${_f}\" ")
    endforeach ()
    set(HSQLDB_SWITCH16_ARGS "")
    foreach (_f IN LISTS HSQLDB_SWITCH16_FILES)
        string(APPEND HSQLDB_SWITCH16_ARGS "\"${HSQLDB_SRC_DIR}/org/hsqldb/${_f}\" ")
    endforeach ()

    # Resource files build.xml's own "classes"/"util" <copy> steps put into
    # the classdir alongside the compiled .class files (see this section's
    # own header comment, point 4) -- discovered now, at configure time,
    # since the hsqldb source tree already exists on disk by this point
    # (fetched+patched above in this same configure run), same timing
    # BEANSHELL_MAIN_SOURCES et al. below already rely on.
    file(GLOB_RECURSE HSQLDB_RESOURCE_FILES
        RELATIVE "${HSQLDB_SRC_DIR}"
        "${HSQLDB_SRC_DIR}/org/hsqldb/*.properties"
        "${HSQLDB_SRC_DIR}/org/hsqldb/*.text")
    file(GLOB HSQLDB_UTIL_ICON_FILES
        "${HSQLDB_SRC_DIR}/org/hsqldb/util/*.gif"
        "${HSQLDB_SRC_DIR}/org/hsqldb/util/*.png")

    librestudio_cmake_quote(_q_javac "${Java_JAVAC_EXECUTABLE}")
    librestudio_cmake_quote(_q_java "${Java_JAVA_EXECUTABLE}")
    librestudio_cmake_quote(_q_jar "${Java_JAR_EXECUTABLE}")
    librestudio_cmake_quote(_q_codeswitcher_classdir "${HSQLDB_CODESWITCHER_CLASSDIR}")
    librestudio_cmake_quote(_q_codeswitcher_src "${HSQLDB_CODESWITCHER_SRC}")
    librestudio_cmake_quote(_q_src_dir "${HSQLDB_SRC_DIR}")
    librestudio_cmake_quote(_q_switched_stamp "${HSQLDB_SWITCHED_STAMP}")
    librestudio_cmake_quote(_q_classdir "${HSQLDB_CLASSDIR}")
    librestudio_cmake_quote(_q_servlet_jar "${HSQLDB_SERVLET_JAR}")
    librestudio_cmake_quote(_q_rsp "@${HSQLDB_MAIN_SOURCES_RSP}")
    librestudio_cmake_quote(_q_jar_dir "${HSQLDB_JAR_DIR}")
    librestudio_cmake_quote(_q_hsqldb_jar "${HSQLDB_JAR}")

    set(_hsqldb_switch11_quoted "")
    foreach (_f IN LISTS HSQLDB_SWITCH11_FILES)
        librestudio_cmake_quote(_q_f "${HSQLDB_SRC_DIR}/org/hsqldb/${_f}")
        string(APPEND _hsqldb_switch11_quoted " ${_q_f}")
    endforeach ()
    set(_hsqldb_switch16_quoted "")
    foreach (_f IN LISTS HSQLDB_SWITCH16_FILES)
        librestudio_cmake_quote(_q_f "${HSQLDB_SRC_DIR}/org/hsqldb/${_f}")
        string(APPEND _hsqldb_switch16_quoted " ${_q_f}")
    endforeach ()

    set(_hsqldb_script "# Auto-generated CMake replacement for hsqldb's own ant build.xml \"jar\"
# target -- do not edit (see this section's own header comment in root
# CMakeLists.txt for the full derivation of every step here against the
# real build.xml). A CMake script (invoked via \${CMAKE_COMMAND} -P), not a
# POSIX shell script, so this build step needs no `sh` (no MSYS/Git-Bash/WSL
# assumption on Windows) -- CMake itself is already this build's one
# unconditional dependency.

# 1. Bootstrap: compile just enough to run CodeSwitcher.
file(MAKE_DIRECTORY ${_q_codeswitcher_classdir})
execute_process(
    COMMAND ${_q_javac} -encoding utf8 -d ${_q_codeswitcher_classdir}
            -sourcepath ${_q_src_dir} ${_q_codeswitcher_src}
    RESULT_VARIABLE _r)
if (NOT _r EQUAL 0)
    message(FATAL_ERROR \"hsqldb: javac (CodeSwitcher bootstrap) failed\")
endif ()

# 2. Run CodeSwitcher -- mutates the .java sources in place, so only once
#    ever per extracted+patched source tree (see the stamp file below).
if (NOT EXISTS ${_q_switched_stamp})
    execute_process(
        COMMAND ${_q_java} -cp ${_q_codeswitcher_classdir} org.hsqldb.util.CodeSwitcher
                ${_hsqldb_switch11_quoted} +JAVA2 -JAVA2FULL -JAVA4
        RESULT_VARIABLE _r)
    if (NOT _r EQUAL 0)
        message(FATAL_ERROR \"hsqldb: CodeSwitcher (switchtojdk11) failed\")
    endif ()
    execute_process(
        COMMAND ${_q_java} -cp ${_q_codeswitcher_classdir} org.hsqldb.util.CodeSwitcher
                ${_hsqldb_switch16_quoted} +JAVA2 +JAVA2FULL +JAVA4 +JAVA6
        RESULT_VARIABLE _r)
    if (NOT _r EQUAL 0)
        message(FATAL_ERROR \"hsqldb: CodeSwitcher (switchtojdk16) failed\")
    endif ()
    file(TOUCH ${_q_switched_stamp})
endif ()

# 3. Main compile.
file(MAKE_DIRECTORY ${_q_classdir})
execute_process(
    COMMAND ${_q_javac} -encoding utf8 -d ${_q_classdir}
            -classpath ${_q_servlet_jar} -sourcepath ${_q_src_dir} ${_q_rsp}
    RESULT_VARIABLE _r)
if (NOT _r EQUAL 0)
    message(FATAL_ERROR \"hsqldb: javac (main compile) failed\")
endif ()

# 4. Resources.
")

    foreach (_res IN LISTS HSQLDB_RESOURCE_FILES)
        librestudio_cmake_quote(_q_res_src "${HSQLDB_SRC_DIR}/${_res}")
        get_filename_component(_res_dir "${_res}" DIRECTORY)
        librestudio_cmake_quote(_q_res_destdir "${HSQLDB_CLASSDIR}/${_res_dir}")
        string(APPEND _hsqldb_script "file(MAKE_DIRECTORY ${_q_res_destdir})\nfile(COPY ${_q_res_src} DESTINATION ${_q_res_destdir})\n")
    endforeach ()
    unset(_res_dir)

    if (HSQLDB_UTIL_ICON_FILES)
        librestudio_cmake_quote(_q_util_destdir "${HSQLDB_CLASSDIR}/org/hsqldb/util")
        string(APPEND _hsqldb_script "file(MAKE_DIRECTORY ${_q_util_destdir})\n")
        foreach (_icon IN LISTS HSQLDB_UTIL_ICON_FILES)
            librestudio_cmake_quote(_q_icon "${_icon}")
            string(APPEND _hsqldb_script "file(COPY ${_q_icon} DESTINATION ${_q_util_destdir})\n")
        endforeach ()
    endif ()

    string(APPEND _hsqldb_script "
# 5. Jar it up.
file(MAKE_DIRECTORY ${_q_jar_dir})
execute_process(
    COMMAND ${_q_jar} -c -f ${_q_hsqldb_jar} org
    WORKING_DIRECTORY ${_q_classdir}
    RESULT_VARIABLE _r)
if (NOT _r EQUAL 0)
    message(FATAL_ERROR \"hsqldb: jar failed\")
endif ()
")

    set(_hsqldb_build_script "${CMAKE_BINARY_DIR}/hsqldb-build.cmake")
    file(GENERATE OUTPUT "${_hsqldb_build_script}" CONTENT "${_hsqldb_script}")

    add_custom_command(
        OUTPUT "${HSQLDB_JAR}"
        COMMAND ${CMAKE_COMMAND} -P "${_hsqldb_build_script}"
        DEPENDS ${HSQLDB_MAIN_SOURCES} "${HSQLDB_MAIN_SOURCES_RSP}"
                "${_hsqldb_build_script}"
        COMMENT "Building hsqldb.jar"
        VERBATIM)
    add_custom_target(hsqldb_ext ALL DEPENDS "${HSQLDB_JAR}")

    install(FILES "${HSQLDB_JAR}" DESTINATION program/classes)

    # --- beanshell (needed by scripting_beanshell + scripting's own optional
    # Jar_ScriptProviderForBeanShell.mk) -------------------------------------
    #
    # external/beanshell's own 4 .mk files, re-checked directly:
    # UnpackedTarball_beanshell.mk fetches $(BSH_TARBALL) (download.lst,
    # confirmed tracked there: BSH_TARBALL=bsh-2.1.1-src.zip, real first-
    # party tarball, unlike raptor2/rasqal/redland below), fixes CRLF line
    # endings on engine/src/bsh/TestBshScriptEngine.java (needed before
    # beanshell-invoke.patch applies cleanly -- re-verified empirically: the
    # patch fails 3/3 hunks against the unfixed CRLF file, succeeds against
    # the LF-normalized one), then applies 5 patches at patchlevel 2 (a
    # purely-numeric filename suffix overrides that default per-patch, same
    # solenv/gbuild/UnpackedTarball.mk convention hsqldb's own patch loop
    # above already reproduces -- java9.patch.0/applet.patch.0 need level 0,
    # beanshell-jar-timestamps.patch.1 needs level 1). ExternalPackage_
    # beanshell.mk installs the result to $(LIBO_SHARE_JAVA_FOLDER)/bsh.jar
    # (i.e. program/classes/bsh.jar on this platform).
    #
    # Like java_websocket above (and unlike hsqldb): re-verified directly
    # that BeanShell's own real upstream build.xml's "jarall" target (the
    # default target ExternalProject_beanshell.mk's own `ant -f build.xml`
    # invocation runs) does no source-mutating work comparable to hsqldb's
    # CodeSwitcher -- it is a plain javac compile (excluding a handful of
    # named files/packages) plus a few resource copies plus `jar`. The one
    # real wrinkle: BeanShell's parser is normally generated from bsh.jjt via
    # jjtree+javacc (bundled in lib/javacc.jar) -- but this port does NOT run
    # jjtree/javacc at all: manually verified that (a) bsh.jj and every
    # javacc-generated .java file (Parser.java, ParserTokenManager.java,
    # etc.) are already checked into this release tarball (a working,
    # ready-to-build state, the exact same reason autotools projects ship a
    # pre-built ./configure -- so downstream packagers need not install
    # jjtree/javacc themselves either), and (b) none of the 5 patches above
    # touches bsh.jjt or bsh.jj, so the checked-in generated files remain
    # fully consistent with the grammar -- regenerating them would produce
    # byte-identical output to what is already there, exactly what ant's own
    # `<uptodate>` checks (checkjjt/checkjj targets) would themselves skip
    # regenerating given unrelated, non-grammar patches. Also unlike
    # hsqldb: no external jar (lib/*.jar, all bundled in the tarball itself)
    # is needed on javac's own classpath at all -- manually verified end-to-
    # end (compiled with zero -classpath entries, zero errors) -- because
    # every source file this port actually compiles only needs javax.script,
    # part of the JDK itself since Java 6, not BeanShell's own bundled
    # javaxscript.jar shim for pre-6 JDKs.
    #
    # Excludes (build.xml's own composed ${excludes} property, the parts not
    # commented out there -- re-verified directly, none of the commented-out
    # ones apply): exclude-bsf (bsh/util/BeanShellBSFEngine.java,
    # TestBshBSF.java -- the latter actually lives under bsf/src/, not
    # src/bsh/util/, confirmed by find), exclude-servlet (bsh/servlet/*),
    # excludes-legacy (bsh/JThis.java), plus 4 hardcoded AWT applet demo
    # classes never packed into jarall either. test-src-dir (BeanShell's own
    # internal test suite, needs junit/hamcrest, never part of any real
    # packaging target) is skipped entirely rather than compiled-then-
    # excluded, since nothing packages it either way.
    #
    # Prefer a system bsh.jar first, matching upstream's own --with-system-
    # beanshell option (configure.ac: default path /usr/share/java/bsh.jar) --
    # same "prefer system, fall back to a real vendored build" shape as
    # every other external in this file, with an explicit force-vendor
    # override for consistency (even though, unlike a shared library, a
    # missing/mismatched system bsh.jar cannot silently break ABI the way a
    # too-old system .so could -- there is no version floor to enforce here,
    # any bsh.jar exposing the plain bsh.Interpreter API upstream's own
    # ScriptProviderForBeanShell.java uses will do).
    option(LIBRESTUDIO_FORCE_VENDOR_BEANSHELL
        "Always build BeanShell from its own upstream source, even if a system bsh.jar exists"
        OFF)

    set(_beanshell_use_system FALSE)
    if (NOT LIBRESTUDIO_FORCE_VENDOR_BEANSHELL)
        find_file(BEANSHELL_SYSTEM_JAR NAMES bsh.jar PATHS /usr/share/java)
        if (BEANSHELL_SYSTEM_JAR)
            set(_beanshell_use_system TRUE)
        endif ()
    endif ()

    if (_beanshell_use_system)
        message(STATUS "beanshell: using system bsh.jar (${BEANSHELL_SYSTEM_JAR})")
        set(BEANSHELL_JAR "${BEANSHELL_SYSTEM_JAR}")
    else ()
        message(STATUS "beanshell: no system bsh.jar found (or vendoring forced) -- building BeanShell 2.1.1 from source")

        set(BSH_SHA256SUM "2248387ceaa319840434a3547a8b2fec12f95a8418ee039ce5ff5726053a139c")
        librestudio_fetch_tarball(beanshell "bsh-2.1.1-src.zip" "${BSH_SHA256SUM}")

        set(_beanshell_eol_path "${beanshell_SOURCE_DIR}/engine/src/bsh/TestBshScriptEngine.java")
        if (EXISTS "${_beanshell_eol_path}")
            file(READ "${_beanshell_eol_path}" _beanshell_eol_content)
            string(REPLACE "\r\n" "\n" _beanshell_eol_content "${_beanshell_eol_content}")
            file(WRITE "${_beanshell_eol_path}" "${_beanshell_eol_content}")
        endif ()
        unset(_beanshell_eol_path)
        unset(_beanshell_eol_content)

        set(BEANSHELL_PATCHES
            bsh-2.0b1-src.patch
            beanshell-invoke.patch
            java9.patch.0
            applet.patch.0
            beanshell-jar-timestamps.patch.1)

        set(BEANSHELL_PATCHED_STAMP "${beanshell_SOURCE_DIR}/.patched")
        if (NOT EXISTS "${BEANSHELL_PATCHED_STAMP}")
            foreach (_patch IN LISTS BEANSHELL_PATCHES)
                get_filename_component(_patch_suffix "${_patch}" LAST_EXT)
                string(REPLACE "." "" _patch_suffix "${_patch_suffix}")
                if (_patch_suffix MATCHES "^[0-9]+$")
                    set(_patch_level "${_patch_suffix}")
                else ()
                    set(_patch_level "2")
                endif ()
                execute_process(
                    COMMAND "${Patch_EXECUTABLE}" -f -s "-p${_patch_level}" --fuzz=0 -i
                            "${CMAKE_SOURCE_DIR}/external/beanshell/${_patch}"
                    WORKING_DIRECTORY "${beanshell_SOURCE_DIR}"
                    RESULT_VARIABLE _patch_result)
                unset(_patch_suffix)
                unset(_patch_level)
                if (NOT _patch_result EQUAL 0)
                    file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/_deps/beanshell")
                    message(FATAL_ERROR
                        "Failed to apply ${_patch} to beanshell; source tree reset, re-run cmake to retry")
                endif ()
            endforeach ()
            file(TOUCH "${BEANSHELL_PATCHED_STAMP}")
        endif ()

        set(BEANSHELL_SRC_DIR "${beanshell_SOURCE_DIR}/src")
        set(BEANSHELL_CLASSDIR "${CMAKE_BINARY_DIR}/beanshell-classes")
        set(BEANSHELL_JAR_DIR "${beanshell_SOURCE_DIR}/dist")
        set(BEANSHELL_JAR "${BEANSHELL_JAR_DIR}/bsh-2.1.1.jar")
        set(BEANSHELL_LIB_CLASSPATH "")
        set(BEANSHELL_ENGINE_CONFIG "${beanshell_SOURCE_DIR}/engine/config/javax.script.ScriptEngineFactory")

        file(GLOB_RECURSE BEANSHELL_MAIN_SOURCES
            "${beanshell_SOURCE_DIR}/src/*.java"
            "${beanshell_SOURCE_DIR}/bsf/src/*.java"
            "${beanshell_SOURCE_DIR}/engine/src/*.java")
        list(FILTER BEANSHELL_MAIN_SOURCES EXCLUDE REGEX "/bsh/servlet/")
        list(FILTER BEANSHELL_MAIN_SOURCES EXCLUDE REGEX "/TestBshBSF\\.java$")
        list(FILTER BEANSHELL_MAIN_SOURCES EXCLUDE REGEX "/BeanShellBSFEngine\\.java$")
        list(FILTER BEANSHELL_MAIN_SOURCES EXCLUDE REGEX "/bsh/JThis\\.java$")
        list(FILTER BEANSHELL_MAIN_SOURCES EXCLUDE REGEX "/bsh/util/AWTDemoApplet\\.java$")
        list(FILTER BEANSHELL_MAIN_SOURCES EXCLUDE REGEX "/bsh/util/AWTRemoteApplet\\.java$")
        list(FILTER BEANSHELL_MAIN_SOURCES EXCLUDE REGEX "/bsh/util/JDemoApplet\\.java$")
        list(FILTER BEANSHELL_MAIN_SOURCES EXCLUDE REGEX "/bsh/util/JRemoteApplet\\.java$")

        set(BEANSHELL_MAIN_SOURCES_RSP "${CMAKE_BINARY_DIR}/beanshell-main-sources.rsp")
        set(_beanshell_rsp_content "")
        foreach (_src IN LISTS BEANSHELL_MAIN_SOURCES)
            string(APPEND _beanshell_rsp_content "\"${_src}\"\n")
        endforeach ()
        file(GENERATE OUTPUT "${BEANSHELL_MAIN_SOURCES_RSP}" CONTENT "${_beanshell_rsp_content}")
        unset(_beanshell_rsp_content)

        # bsh/commands/*.bsh + bsh/util/lib/* -- the resource files
        # build.xml's own "classes" target <copy>s alongside the compiled
        # .class files (see this section's own header comment, point 2/3).
        # Discovered now, at configure time, same timing HSQLDB_RESOURCE_FILES
        # above already relies on (the source tree already exists on disk by
        # this point, fetched+patched above in this same configure run).
        file(GLOB BEANSHELL_COMMANDS_FILES "${BEANSHELL_SRC_DIR}/bsh/commands/*.bsh")
        file(GLOB BEANSHELL_UTIL_LIB_FILES "${BEANSHELL_SRC_DIR}/bsh/util/lib/*")

        librestudio_cmake_quote(_q_beanshell_javac "${Java_JAVAC_EXECUTABLE}")
        librestudio_cmake_quote(_q_beanshell_jar "${Java_JAR_EXECUTABLE}")
        librestudio_cmake_quote(_q_beanshell_classdir "${BEANSHELL_CLASSDIR}")
        librestudio_cmake_quote(_q_beanshell_cp "${BEANSHELL_LIB_CLASSPATH}")
        librestudio_cmake_quote(_q_beanshell_rsp "@${BEANSHELL_MAIN_SOURCES_RSP}")
        librestudio_cmake_quote(_q_beanshell_commands_destdir "${BEANSHELL_CLASSDIR}/bsh/commands")
        librestudio_cmake_quote(_q_beanshell_util_lib_destdir "${BEANSHELL_CLASSDIR}/bsh/util/lib")
        librestudio_cmake_quote(_q_beanshell_services_destdir "${BEANSHELL_CLASSDIR}/META-INF/services")
        librestudio_cmake_quote(_q_beanshell_engine_config "${BEANSHELL_ENGINE_CONFIG}")
        librestudio_cmake_quote(_q_beanshell_manifest_tmp "${BEANSHELL_CLASSDIR}/MANIFEST.MF.tmp")
        librestudio_cmake_quote(_q_beanshell_jar_dir "${BEANSHELL_JAR_DIR}")
        librestudio_cmake_quote(_q_beanshell_jar_file "${BEANSHELL_JAR}")

        set(_beanshell_script "# Auto-generated CMake replacement for BeanShell's own upstream
# build.xml \"jarall\" target -- do not edit (see this section's own header
# comment in root CMakeLists.txt for the full derivation of every step
# here against the real build.xml). A CMake script (invoked via
# \${CMAKE_COMMAND} -P), not a POSIX shell script, so this build step needs
# no `sh` (no MSYS/Git-Bash/WSL assumption on Windows) -- CMake itself is
# already this build's one unconditional dependency.

# 1. Main compile.
file(MAKE_DIRECTORY ${_q_beanshell_classdir})
execute_process(
    COMMAND ${_q_beanshell_javac} -encoding utf8 -d ${_q_beanshell_classdir}
            -classpath ${_q_beanshell_cp} ${_q_beanshell_rsp}
    RESULT_VARIABLE _r)
if (NOT _r EQUAL 0)
    message(FATAL_ERROR \"beanshell: javac (main compile) failed\")
endif ()

# 2. Resources: bsh/commands/*.bsh + bsh/util/lib/*.
file(MAKE_DIRECTORY ${_q_beanshell_commands_destdir})
")

        foreach (_f IN LISTS BEANSHELL_COMMANDS_FILES)
            librestudio_cmake_quote(_q_bf "${_f}")
            string(APPEND _beanshell_script "file(COPY ${_q_bf} DESTINATION ${_q_beanshell_commands_destdir})\n")
        endforeach ()

        string(APPEND _beanshell_script "file(MAKE_DIRECTORY ${_q_beanshell_util_lib_destdir})\n")
        foreach (_f IN LISTS BEANSHELL_UTIL_LIB_FILES)
            librestudio_cmake_quote(_q_bf "${_f}")
            string(APPEND _beanshell_script "file(COPY ${_q_bf} DESTINATION ${_q_beanshell_util_lib_destdir})\n")
        endforeach ()
        unset(_q_bf)

        string(APPEND _beanshell_script "
# 3. The javax.script (JSR 223) engine factory registration.
file(MAKE_DIRECTORY ${_q_beanshell_services_destdir})
file(COPY ${_q_beanshell_engine_config} DESTINATION ${_q_beanshell_services_destdir})

# 4. Jar it up.
file(MAKE_DIRECTORY ${_q_beanshell_jar_dir})
file(WRITE ${_q_beanshell_manifest_tmp} \"Manifest-Version: 1.0
\")
execute_process(
    COMMAND ${_q_beanshell_jar} -c -f ${_q_beanshell_jar_file} -m MANIFEST.MF.tmp bsh
    WORKING_DIRECTORY ${_q_beanshell_classdir}
    RESULT_VARIABLE _r)
if (NOT _r EQUAL 0)
    message(FATAL_ERROR \"beanshell: jar failed\")
endif ()
")

        set(_beanshell_build_script "${CMAKE_BINARY_DIR}/beanshell-build.cmake")
        file(GENERATE OUTPUT "${_beanshell_build_script}" CONTENT "${_beanshell_script}")

        add_custom_command(
            OUTPUT "${BEANSHELL_JAR}"
            COMMAND ${CMAKE_COMMAND} -P "${_beanshell_build_script}"
            DEPENDS ${BEANSHELL_MAIN_SOURCES} "${BEANSHELL_MAIN_SOURCES_RSP}"
                    ${BEANSHELL_COMMANDS_FILES} ${BEANSHELL_UTIL_LIB_FILES}
                    "${BEANSHELL_ENGINE_CONFIG}" "${_beanshell_build_script}"
            COMMENT "Building bsh-2.1.1.jar"
            VERBATIM)
        add_custom_target(beanshell_ext ALL DEPENDS "${BEANSHELL_JAR}")
    endif ()

    install(FILES "${BEANSHELL_JAR}" DESTINATION program/classes RENAME bsh.jar)
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

# clucene (2026-09-29, system-or-vendor refactor): resolved via
# librestudio_pkgconfig() into LIBRESTUDIO_CLUCENE_TARGET -- real vendoring
# fallback in external/clucene/clucene.cmake (a single combined library,
# core+shared+contribs-lib, unlike a real system install's 2-.so split --
# see that driver's own header comment and helpcompiler/CMakeLists.txt's
# own FOUND-branch handling of the "contribs-lib" split). DRIVER given
# explicitly: "libclucene-core" contains a hyphen, so it never qualifies
# for this router's own single-module auto-detect candidate rule.
librestudio_pkgconfig(NAME CLUCENE MODULES libclucene-core DRIVER clucene
    FORCE_VENDOR_VAR LIBRESTUDIO_FORCE_VENDOR_CLUCENE)

# slideshow's box2d-based physics transitions. box2d ships a CMake
# package-config (no pkg-config .pc file), and exports an imported target
# rather than variables, so this is find_package rather than
# pkg_check_modules like everything else in this section.
find_package(box2d REQUIRED)

# --- configuration headers ----------------------------------------------------
#
# Generated once everything above has been found, because several of their
# values are decided by what was found: config_cups.h, config_dbus.h and
# config_gio.h say whether libcups, libdbus and gio are there, and they are
# what vcl/ switches its printing and session handling on.
include(LibreStudioConfigHeaders)

# external/ (zlib, libpng, md4c, graphite, harfbuzz, libixion, orcus,
# raptor2, rasqal, redland, poppler, coinmp): the "prefer a sufficiently new
# system package, fall back to a real vendored build" externals -- no
# add_subdirectory() router here anymore. Each is resolved lazily and
# memoized (librestudio_pkgconfig()/librestudio_find_package(), cmake/
# LibreStudioTarballs.cmake) directly from whichever real consumer module
# first needs it (vcl/sc/sdext/unoxml/comphelper/...); external/<name>/
# directories still hold each one's own <name>.cmake driver, patches and
# config files, just without a CMakeLists.txt or add_subdirectory()
# registration of their own. ENABLE_COINMP/COINMP_DETECT_FOUND (read by
# sc/CMakeLists.txt's own CoinMP resolution) still need
# LibreStudioConfigHeaders included first, which is why this comment (and
# every consumer module below) still comes after it.

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

# unodevtools (Executable_uno-skeletonmaker) needs only sal/salhelper/unoidl
# plus codemaker's static libraries, all declared immediately above; inert by
# default (ENABLE_ODK is OFF), same gate as unoidl-read above.
add_subdirectory(unodevtools)

# rust_uno declares a self-contained ENABLE_RUST_UNO option and defines no
# targets at all yet (see its own CMakeLists.txt) -- no ordering constraint,
# placed here purely to stay near its sibling experimental-language-binding
# module net_ure (added further below, after cppuhelper).
add_subdirectory(rust_uno)

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
# --- verified-empty-on-this-platform / no-router-needed modules --------------
#
# android, ios, static: each module's own Module_*.mk gates every one of its
# real targets behind a platform/BUILD_TYPE check this port's own
# configuration never satisfies (OS=ANDROID, OS=iOS, EMSCRIPTEN respectively)
# -- verified directly against each live .mk, not assumed out of scope; see
# each module's own CMakeLists.txt comment for the full reasoning. uitest:
# UITest_* targets only, the same already-established "no runner exists" gap
# as JunitTest/PythonTest elsewhere. No dependencies either way (only
# CMAKE_SYSTEM_NAME checks or nothing at all), so these four can sit
# anywhere; grouped here, right before o3tl, as this port's own "genuinely
# no-op right now" cluster. (external/ used to belong in this list too --
# not anymore: it is a real system-or-vendor router now, add_subdirectory()'d
# once, earlier, right after include(LibreStudioConfigHeaders) above, see
# that call site's own comment.)
add_subdirectory(android)
add_subdirectory(ios)
add_subdirectory(static)
add_subdirectory(uitest)

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

# net_ure (net_bootstrap needs cppu/cppuhelper/sal/salhelper, all declared
# above) -- inert by default (ENABLE_ODK is OFF; ENABLE_DOTNET auto-detects
# but the module's own gate needs both).
add_subdirectory(net_ure)

# The whole Java/UNO bridge, one add_subdirectory() per module in real
# dependency order (each comment explains why THAT module sits after the
# previous one, not why the LIBRESTUDIO_ENABLE_JAVA gate is repeated --
# nothing unconditional ever ran between these six blocks, so they are one
# if() now instead of six identical ones):
#
#   jvmfwk (JRE discovery/bootstrap) and jvmaccess (the JNI attach/
#   AttachGuard wrapper built on top of it) are the two lowest-level pieces
#   -- both need only cppu, cppuhelper, sal, salhelper (all already declared
#   above), and everything further up the Java stack (stoc's javaloader/
#   javavm, jurt, javaunohelper, bean) links one or both of them.
#
#   unoil generates the offapi Java stub sources (via javamaker, from
#   codemaker above) that ridljar's libreoffice.jar packages directly -- it
#   needs only offapi/udkapi's *_RDB variables (both already set by
#   add_subdirectory(udkapi) and add_subdirectory(offapi) above) and the
#   javamaker executable, not jvmfwk/jvmaccess.
#
#   ridljar builds the real UNO Java bridge classes (libreoffice.jar) plus
#   the tiny unoloader.jar bootstrap jar. It needs its own javamaker
#   customtarget (udkapi only) in addition to unoil's offapi one above, and
#   the java_websocket.jar external fetched/built near the top of this file
#   (JAVA_WEBSOCKET_JAR), so it must come after both.
#
#   jurt: jpipe, the native JNI pipe-transport library ridljar's
#   PipeConnection.java loads via System.loadLibrary. Only needs sal + JNI,
#   not anything above, but kept here to match this port's Java/UNO bridge
#   module ordering.
#
#   javaunohelper: juh/juhx, the native JNI helper libraries that
#   com.sun.star.comp.helper.Bootstrap/SharedLibraryLoader dlopen to
#   bootstrap a UNO ComponentContext from Java. juhx needs jvmaccess (built
#   above, alongside jvmfwk) and udkapi's comprehensive header variant
#   (built above, in udkapi/CMakeLists.txt), not anything from jurt/ridljar.
#
#   bean: officebean.jar + libofficebean.so, the last module in this port's
#   Java/UNO bridge. officebean.jar's compile classpath needs ridljar's
#   libreoffice.jar (libreoffice_JAR/libreoffice_jar, set above); the native
#   library needs only JNI/AWT, nothing from jurt/javaunohelper/jvmaccess.
#   Module_bean.mk: ifneq ($(ENABLE_JAVA),) / ifneq ($(OS),MACOSX) / ifneq
#   ($(OS),ANDROID) around gb_Module_add_targets(bean, Jar_officebean
#   Library_officebean). The OS exclusions are not just build-system noise
#   -- bean/native/ is a JNI/AWT peer bridge upstream never ports to macOS
#   or Android, so this port must not attempt it there either -- hence its
#   own nested condition, a real superset of the outer gate, not a
#   duplicate of it.
if (LIBRESTUDIO_ENABLE_JAVA)
    add_subdirectory(jvmfwk)
    add_subdirectory(jvmaccess)
    add_subdirectory(unoil)
    add_subdirectory(ridljar)
    add_subdirectory(jurt)
    add_subdirectory(javaunohelper)
    if ((NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin") AND (NOT CMAKE_SYSTEM_NAME STREQUAL "Android"))
        add_subdirectory(bean)
    endif ()
endif ()

# qadevOOo/nlpsolver both self-gate their real (LIBRESTUDIO_ENABLE_JAVA +
# librestudio_add_uno_jar()) targets internally; librestudio_add_uno_jar()
# itself is defined by cmake/LibreStudioJavaJar.cmake, include()'d near the
# top of this file, so no add_subdirectory() ordering constraint from it.
# Placed here to stay with the rest of this port's Java/UNO bridge cluster.
add_subdirectory(qadevOOo)
add_subdirectory(nlpsolver)

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

# librelogo is pure data (a Python macro + one .properties file, install()
# rules gated on ENABLE_SCRIPTING) -- no target dependency on anything else in
# the tree, placed here purely to stay near its fellow Basic/scripting-slice
# modules.
add_subdirectory(librelogo)

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

# Deferred CppunitTest registrations (2026-09-27, superseding the old "find
# the latest-binding module and hand-place include() after it here" scheme):
# librestudio_add_cppunit_test()'s COMPONENTS parameter resolves each name
# through a GLOBAL property (cmake/LibreStudioTest.cmake), read eagerly via
# get_property(GLOBAL ...) at CMake *script*-processing time -- CMake's
# target/build graph resolves dependencies regardless of declaration order,
# but this is plain sequential script execution, so a get_property() genuinely
# only sees whatever has been set() by the exact point it runs. A module whose
# CppunitTest needs a component (or CONFIGURATION's LIBRESTUDIO_TEST_UNITTEST_
# DIR, or program_services_rdb) from a module add_subdirectory()'d LATER than
# itself keeps that one librestudio_add_cppunit_test() call in its own
# <module>/CppunitTest_deferred.cmake, and its own <module>/CMakeLists.txt
# schedules that file with cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}"
# CALL include ...) -- which CMake runs only after every add_subdirectory() in
# the whole tree has completed, regardless of which one is actually the
# latest-binding dependency. No per-module include() bookkeeping lives here
# anymore; see each module's own CppunitTest_deferred.cmake for its specific
# ordering constraint, and cmake/LibreStudioTest.cmake's header comment for
# the DEFER mechanism itself.

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

# code: "LibreStudio Code", a new VCL-native IDE app -- not a port of any
# upstream gbuild module (original code, no Module_code.mk to transcribe, so
# it has no row in PORTING_STATUS.md). Needs comphelper, cppu, cppuhelper,
# sal, svtools, tl, ucbhelper, vcl (all declared above) plus the vclmain
# static lib vcl/CMakeLists.txt defines for its own workbench programs.
add_subdirectory(code)

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

# apple_remote/cli_ure/embedserv/winaccessibility/setup_native are all
# platform-gated to a toolchain this Linux dev machine doesn't have (macOS,
# or Windows+MSVC) -- every real target inside each is wrapped in its own
# if (APPLE ...)/if (WIN32 ...) guard and is therefore inert here, but
# embedserv/winaccessibility do call librestudio_add_component() on the
# platforms where they're real, so -- to stay correct for a future
# cross-compiled Windows build, not just this Linux one -- they are placed
# before librestudio_finalize_components(services) below, same as every
# other component-registering module.
add_subdirectory(apple_remote)
add_subdirectory(cli_ure)
add_subdirectory(embedserv)
add_subdirectory(winaccessibility)
add_subdirectory(setup_native)

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
#
# 2026-09-29: this add_dependencies() call alone is NOT sufficient -- kept
# here for the aggregate i18npool_localedata phony target's own ordering, but
# it does not reach the per-file add_custom_command() edges that are that
# target's own DEPENDS (separate ninja build edges, no edge between them and
# whatever the phony node itself additionally depends on). Confirmed via a
# real remote build: most locales' saxparser invocations raced ahead of
# program_oovbaapi_rdb and failed. The real fix -- the same targets (plus
# program_types_rdb/program_offapi_rdb, also read via unorc's UNO_TYPES) --
# is now on each individual add_custom_command() in i18npool/CMakeLists.txt's
# own per-locale loop; see that file's header comment on the DEPENDS line.
add_dependencies(i18npool_localedata program_services_rdb sax)

librestudio_generate_unorc("${LIBRESTUDIO_PROGRAM_DIR}/unorc")

# --- remaining leaf modules (no ordering constraint on anything above) -------
#
# swext needs ridljar's libreoffice/libreoffice_jar target (declared far
# above, well before this point). odk needs cppuhelper's findsofficepath
# static lib (also declared far above). Every other module here is either
# pure install()/data (extras, readlicense_oo, sysui, ure, postprocess,
# wizards) or a documentation-only stub with zero real targets on this
# platform/config (reportbuilder, scp2, solenv, soltools, instsetoo_native) --
# none of them are linked against by, or link against, anything else in this
# tree, so they are grouped here rather than threaded individually through
# the dependency-ordered graph above.
add_subdirectory(swext)
add_subdirectory(odk)
add_subdirectory(extras)
add_subdirectory(readlicense_oo)
add_subdirectory(sysui)
add_subdirectory(ure)
add_subdirectory(postprocess)
add_subdirectory(wizards)
add_subdirectory(reportbuilder)
add_subdirectory(scp2)
add_subdirectory(solenv)
add_subdirectory(soltools)
add_subdirectory(instsetoo_native)

# --- smoke test --------------------------------------------------------------

add_subdirectory(cpputools)
add_subdirectory(smoketest)
