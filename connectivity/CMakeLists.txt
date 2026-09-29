# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# connectivity — dbtools (unconditional) plus the three file-based SDBC
# drivers, calc/dbase/flat (gated behind ENABLE_DBCONNECTIVITY, see below).
#
# dbtools is the shared SQL-parsing and database-helper library that svx
# (fmcomp/gridcell, form/sqlparserclient, form/fmundo, ...) links against
# unconditionally in upstream, because HAVE_FEATURE_DBCONNECTIVITY defaults to
# enabled on desktop Linux (it is only forced off under --enable-wasm-strip --
# see configure.ac). Source list, includes and library set are transcribed
# from Library_dbtools.mk; dbtools is registered in the OOO layer, matching
# Repository.mk's gb_Helper_register_libraries_for_install(OOOLIBS,ooo,...).
#
# calc/dbase/flat are upstream's simplest SDBC drivers: file-based, no ODBC/
# JDBC/network client library, each one a thin CTable/CConnection/CDriver (or
# D-/E- prefixed equivalent) subclass of the shared "file" driver framework in
# Library_file.mk (connectivity/source/drivers/file, .../component). Library_
# file itself is not named in Module_connectivity.mk's Module_add_targets list
# quoted below because it ships no .component of its own -- it is a link-time
# prerequisite of calc/dbase/flat (and of every other SDBC driver upstream),
# so it is ported here alongside them, matching how dbtools is transcribed:
# one add_library() per Library_*.mk, sources/use_libraries/componentfile
# copied verbatim, OOO layer per Repository.mk's same OOOLIBS registration
# (see connectivity/Library_calc.mk, Library_dbase.mk, Library_flat.mk,
# Library_file.mk).
#
# 2026-09-29: odbc, sdbc2, mysql_jdbc, writer, dbpool2 and mozbootstrap are
# NOW ALSO ported (see the dedicated block below, right after flat) --
# these six used to be listed here as a real, not-yet-attempted gap
# (re-verified 2026-09-28 against the live .mk after an earlier version of
# this comment mischaracterized it as "out of scope"; see [[feedback_
# gbuild_full_replacement_scope]] in project memory for why that framing
# is wrong and should not be repeated). None of them needed a new external
# -- see that block's own header comment for the odbc_headers/officecfg
# specifics.
#
# The rest of Module_connectivity.mk's own driver list remains genuinely
# unbuilt, for real platform/dependency reasons, not a settled scope
# decision:
#   - ado: gated `ifeq ($(OS),WNT)` in the real .mk -- ADO is a Windows COM
#     API, genuinely unreachable on this Linux port, not merely unattempted.
#   - macab1/macabdrv1: gated `ifeq ($(OS),MACOSX)` -- same kind of real
#     platform limitation, not a choice.
#   - hsqldb/jdbc: 2026-09-29, ALSO now ported (own blocks further down,
#     right after Jar_sdbc_hsqldb) -- the Java-UNO bridge itself (bridges/
#     CMakeLists.txt's own java_uno library+jar) was already ported and
#     build-verified earlier; what was actually still missing was these
#     two C++-side consumers, not the bridge. Both gated `if (LIBRESTUDIO_
#     ENABLE_JAVA)`, same as Jar_sdbc_hsqldb -- and that flag is currently
#     OFF in this build's own cache (not a code gap, a configuration
#     choice left for the user to flip).
#   - evoab (Configuration_evoab/Library_evoab): gated `$(enable_evolution2)`
#     in configure.ac, defaulting to "no" -- needs an explicit --enable flag
#     plus GNOME gobject-2.0/evolution-data-server, neither detected by this
#     port's own dependency probing.
#   - mysqlc (ENABLE_MARIADBC in the .mk, "mariadb_sdbc" in configure.ac):
#     configure.ac defaults this to TRUE (on) whenever not explicitly
#     disabled -- so unlike evoab/firebird/postgresql below, this one is
#     "on by default upstream" and this port not building it is a real,
#     not-yet-attempted gap, blocked on actually wiring the MariaDB/MySQL
#     Connector-C client library into a consumer (external/mariadb-
#     connector-c/ already has a real StaticLibrary_mariadb-connector-c.mk
#     in this tree, just never plugged into this module).
#   - firebird_sdbc: gated `$(enable_firebird_sdbc)` in configure.ac (several
#     branches -- system vs. bundled Firebird), not evaluated in depth here;
#     needs either a system Firebird client library or upstream's bundled
#     Firebird external, neither vendored in this port.
#   - postgresql-sdbc(-impl): gated behind an explicit PostgreSQL enable
#     path in configure.ac that probes for libpq-fe.h/libpq; not detected/
#     linked anywhere in this port.
# None of the above has been attempted in this pass; each would need its own
# fresh investigation (which of these can build with libraries already
# vendored here vs. which need a new external) before being ported for real.
#
# Configuration_calc/dbase/flat.mk (connectivity/registry/<driver>/.../
# Drivers.xcu) are NOT ported -- documented gap, not an oversight. Each one is
# gb_Configuration_add_spool_modules + add_localized_datas: the spool call
# reads the shared Drivers.xcu with a "module" filter (gb_XcuDataSource_for_
# XcuModuleTarget's string surgery: "Drivers-calc.xcu" resolves to Drivers.xcu
# read with -module calc) into an intermediate Drivers-<driver>.xcu that by
# itself is packed into no registry at all. What actually folds that
# intermediate file into a real .xcd is postprocess/CustomTarget_registry.mk
# (dbase/flat's outputs feed FILES_main -> main.xcd; calc's feeds FILES_calc
# -> a separate calc.xcd, gated the same "ifeq ($(gb_Side),host)" way as
# writer's). postprocess/ has no CMakeLists.txt in this tree at all (root
# CMakeLists.txt never add_subdirectory()s it) -- officecfg/CMakeLists.txt's
# own main.xcd assembly (see the registry section further down in that file)
# replicates only the parts of postprocess's FILES_main list that come from
# officecfg's own registry data, not the DBCONNECTIVITY block that pulls in
# connectivity/registry/*. Wiring these three Configuration_* targets in
# would mean either reaching into officecfg/CMakeLists.txt's already-defined
# main.xcd custom command from here (the wrong direction: connectivity would
# be reopening a sibling module's finished target) or porting postprocess/
# CustomTarget_registry.mk's own multi-.xcd packing wholesale -- a
# significantly larger undertaking than three driver libraries, and one with
# no consumer yet even if built (main.xcd growing a DataAccess/Drivers node
# has no effect until something reads it, e.g. dbaccess). Left unbuilt rather
# than producing an intermediate file with nowhere to go, the same call
# officecfg/CMakeLists.txt itself makes for the eight filter-config spool
# files it cannot build either (see its own "cannot be built here" note).
# Configuration_hsqldb.mk/Configuration_jdbc.mk (added along with Library_
# hsqldb/Library_jdbc, 2026-09-29) are the exact same class of gap, not
# ported for the same postprocess/CustomTarget_registry.mk reason.

find_package(BISON 3.0 REQUIRED)
find_package(FLEX 2.6 REQUIRED)

set(CONNECTIVITY_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

# T_YACCFLAGS/T_LEXFLAGS from Library_dbtools.mk (gb_Library_add_grammars /
# gb_Library_add_scanners overrides), confirmed against the real
# gb_YaccTarget_YaccTarget/gb_LexTarget_LexTarget invocations in
# solenv/gbuild/LinkTarget.mk. sqlflex.l includes "sqlbison.hxx" by that exact
# name, so the parser's header must land there -- bison_target()'s
# DEFINES_FILE takes care of that, and the flex/bison dependency below makes
# sure the header exists before the scanner is compiled.
bison_target(SqlBisonParser
    "${CMAKE_SOURCE_DIR}/connectivity/source/parse/sqlbison.y"
    "${CONNECTIVITY_GENERATED_DIR}/sqlbison.cxx"
    DEFINES_FILE "${CONNECTIVITY_GENERATED_DIR}/sqlbison.hxx"
    COMPILE_FLAGS "-l -p SQLyy -b sql")

flex_target(SqlFlexScanner
    "${CMAKE_SOURCE_DIR}/connectivity/source/parse/sqlflex.l"
    "${CONNECTIVITY_GENERATED_DIR}/sqlflex.cxx"
    COMPILE_FLAGS "-i -8 -PSQLyy -L")

add_flex_bison_dependency(SqlFlexScanner SqlBisonParser)

set(DBTOOLS_SOURCES
    connectivity/source/commontools/AutoRetrievingBase
    connectivity/source/commontools/BlobHelper
    connectivity/source/commontools/CommonTools
    connectivity/source/commontools/ConnectionWrapper
    connectivity/source/commontools/DateConversion
    connectivity/source/commontools/DriversConfig
    connectivity/source/commontools/FDatabaseMetaDataResultSet
    connectivity/source/commontools/FDatabaseMetaDataResultSetMetaData
    connectivity/source/commontools/FValue
    connectivity/source/commontools/ParameterSubstitution
    connectivity/source/commontools/RowFunctionParser
    connectivity/source/commontools/TColumnsHelper
    connectivity/source/commontools/TConnection
    connectivity/source/commontools/TDatabaseMetaDataBase
    connectivity/source/commontools/TIndex
    connectivity/source/commontools/TIndexColumns
    connectivity/source/commontools/TIndexes
    connectivity/source/commontools/TKey
    connectivity/source/commontools/TKeyColumns
    connectivity/source/commontools/TKeys
    connectivity/source/commontools/TPrivilegesResultSet
    connectivity/source/commontools/TSkipDeletedSet
    connectivity/source/commontools/TSortIndex
    connectivity/source/commontools/TTableHelper
    connectivity/source/commontools/conncleanup
    connectivity/source/commontools/dbcharset
    connectivity/source/commontools/dbconversion
    connectivity/source/commontools/dbexception
    connectivity/source/commontools/dbmetadata
    connectivity/source/commontools/dbtools
    connectivity/source/commontools/dbtools2
    connectivity/source/commontools/filtermanager
    connectivity/source/commontools/formattedcolumnvalue
    connectivity/source/commontools/parameters
    connectivity/source/commontools/paramwrapper
    connectivity/source/commontools/predicateinput
    connectivity/source/commontools/propertyids
    connectivity/source/commontools/sqlerror
    connectivity/source/commontools/statementcomposer
    connectivity/source/commontools/warningscontainer
    connectivity/source/parse/PColumn
    connectivity/source/parse/internalnode
    connectivity/source/parse/sqliterator
    connectivity/source/parse/sqlnode
    connectivity/source/resource/sharedresources
    connectivity/source/sdbcx/VCatalog
    connectivity/source/sdbcx/VCollection
    connectivity/source/sdbcx/VColumn
    connectivity/source/sdbcx/VDescriptor
    connectivity/source/sdbcx/VGroup
    connectivity/source/sdbcx/VIndex
    connectivity/source/sdbcx/VIndexColumn
    connectivity/source/sdbcx/VKey
    connectivity/source/sdbcx/VKeyColumn
    connectivity/source/sdbcx/VTable
    connectivity/source/sdbcx/VUser
    connectivity/source/sdbcx/VView
)

librestudio_expand_cxx(DBTOOLS_SOURCES)

add_library(dbtools SHARED
    ${DBTOOLS_SOURCES}
    ${BISON_SqlBisonParser_OUTPUTS}
    ${FLEX_SqlFlexScanner_OUTPUTS})

if (ENABLE_PCH)
    target_precompile_headers(dbtools PRIVATE
        "${CMAKE_SOURCE_DIR}/connectivity/inc/pch/precompiled_dbtools.hxx")
    target_compile_definitions(dbtools PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(dbtools PRIVATE
    "${CMAKE_SOURCE_DIR}/connectivity/inc"
    "${CMAKE_SOURCE_DIR}/connectivity/source/inc"
    "${CONNECTIVITY_GENERATED_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(dbtools udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(dbtools PRIVATE OOO_DLLIMPLEMENTATION_DBTOOLS)

target_link_libraries(dbtools PRIVATE
    cppu cppuhelper sal salhelper
    utl tl comphelper i18nlangtag svt vcl
    Boost::boost)

# $(if $(ENABLE_JAVA), jvmaccess) from Library_dbtools.mk.
if (LIBRESTUDIO_ENABLE_JAVA)
    target_link_libraries(dbtools PRIVATE jvmaccess)
endif ()

# ifeq ($(filter-out NETBSD MACOSX,$(OS)),) -- true only for NETBSD/MACOSX;
# on Linux this block does not apply, so ucbhelper is deliberately omitted.

librestudio_add_component(dbtools
    COMPONENT "${CMAKE_SOURCE_DIR}/connectivity/source/dbtools/dbtools.component"
    LIBRARY   dbtools
    LAYER     OOO)

librestudio_set_library_output_name(dbtools dbtools OOO)

install(TARGETS dbtools LIBRARY DESTINATION program)

# --- sdbc_hsqldb (Jar_sdbc_hsqldb.mk) -----------------------------------------
#
# Module_connectivity.mk: `ifneq ($(ENABLE_JAVA),)` around Jar_sdbc_hsqldb
# (plus Configuration_hsqldb/Configuration_jdbc, both pure Configuration_*.mk
# -- registry defaults, no compiled code, not ported here) -- the small
# Java-side glue the embedded HSQLDB JDBC driver needs on top of the bundled
# hsqldb.jar itself (root CMakeLists.txt's own hsqldb external, next to
# java_websocket). reportdesign_basic_test (reportdesign/CppunitTest_
# deferred.cmake) is this port's first, and so far only, consumer
# (gb_CppunitTest_use_jars(reportdesign_basic_test,sdbc_hsqldb)).
#
# Jar_sdbc_hsqldb.mk, re-checked directly: use_externals(hsqldb) ->
# CLASSPATH HSQLDB_JAR (root CMakeLists.txt); add_manifest_classpath
# `$(if $(filter MACOSX,$(OS)),../../Frameworks/,..)` -> plain ".." on this
# non-MACOSX Linux port; set_packageroot(com) + add_packagedir(org) -> one
# PACKAGEROOT of both "com" and "org" (see cmake/LibreStudioJavaJar.cmake's
# own header comment on why these two are equivalent here: FileSystem
# RuntimeException.java is compiled by the SAME javac invocation as the
# other 8 sources, landing in this jar's own classdir under org/hsqldb/lib/,
# not a separately pre-built directory the way genuine PACKAGEDIRS uses
# are); add_sourcefiles lists all 9 sources below (org/hsqldb/lib/File
# SystemRuntimeException.java is one of the 9, not appended separately --
# re-verified against the live .mk, not assumed). None of the 8 com.sun.star.
# sdbcx.comp.hsqldb.*.java sources actually import or reference any
# com.sun.star.* UNO API type despite their package name (grepped directly)
# -- only java.io/java.net and org.hsqldb.lib -- so, unlike ridljar/bean,
# this jar needs no UNO-types jar on its classpath at all.
if (LIBRESTUDIO_ENABLE_JAVA)
    librestudio_add_uno_jar(sdbc_hsqldb
        PACKAGEROOT com org
        MANIFEST_CLASSPATH ".."
        CLASSPATH "${HSQLDB_JAR}"
        DEPENDS hsqldb_ext "${HSQLDB_JAR}"
        SOURCES
            connectivity/org/hsqldb/lib/FileSystemRuntimeException
            connectivity/com/sun/star/sdbcx/comp/hsqldb/NativeInputStreamHelper
            connectivity/com/sun/star/sdbcx/comp/hsqldb/NativeLibraries
            connectivity/com/sun/star/sdbcx/comp/hsqldb/NativeOutputStreamHelper
            connectivity/com/sun/star/sdbcx/comp/hsqldb/NativeStorageAccess
            connectivity/com/sun/star/sdbcx/comp/hsqldb/StorageAccess
            connectivity/com/sun/star/sdbcx/comp/hsqldb/StorageFileAccess
            connectivity/com/sun/star/sdbcx/comp/hsqldb/StorageNativeInputStream
            connectivity/com/sun/star/sdbcx/comp/hsqldb/StorageNativeOutputStream)

    # ExternalPackage-equivalent: Jar_sdbc_hsqldb.mk's own jar lands in
    # $(LIBO_SHARE_JAVA_FOLDER), i.e. program/classes here, same as
    # hsqldb.jar itself.
    install(FILES "${sdbc_hsqldb_JAR}" DESTINATION program/classes)
endif ()

# --- hsqldb (Library_hsqldb.mk) ------------------------------------------------
#
# 2026-09-29: the C++ side of the embedded HSQLDB SDBC driver (HDriver et
# al.) -- the Java-UNO BRIDGE itself (bridges/CMakeLists.txt's own
# java_uno library+jar) was already ported and build-verified earlier;
# what was actually still missing was this consumer. use_externals(boost_
# headers hsqldb): RepositoryExternal.mk's own gb_LinkTarget__use_hsqldb
# (read in full) -- the !SYSTEM_HSQLDB (vendored) branch's macro body is
# EMPTY, no -D flags at all; only the SYSTEM_HSQLDB branch adds -DSYSTEM_
# HSQLDB/-DHSQLDB_JAR="file://...". This port never detects a system
# hsqldb at all (root CMakeLists.txt always calls librestudio_fetch_
# tarball(hsqldb ...) unconditionally, no system probe) -- confirmed by
# reading connectivity/source/drivers/hsqldb/HDriver.cxx's own
# `#ifdef SYSTEM_HSQLDB` guard directly: since that branch is never
# compiled here, no extra define is added, matching the real empty macro
# exactly (not an oversight). jvmfwk is the only new-to-this-file link
# dependency, already a real, already-ported target (jvmfwk/CMakeLists.txt).
if (LIBRESTUDIO_ENABLE_JAVA)
    set(HSQLDB_LIB_SOURCES
        connectivity/source/drivers/hsqldb/HCatalog
        connectivity/source/drivers/hsqldb/HColumns
        connectivity/source/drivers/hsqldb/HConnection
        connectivity/source/drivers/hsqldb/HDriver
        connectivity/source/drivers/hsqldb/HStorageAccess
        connectivity/source/drivers/hsqldb/HStorageMap
        connectivity/source/drivers/hsqldb/HTable
        connectivity/source/drivers/hsqldb/HTables
        connectivity/source/drivers/hsqldb/HTerminateListener
        connectivity/source/drivers/hsqldb/HTools
        connectivity/source/drivers/hsqldb/HUser
        connectivity/source/drivers/hsqldb/HUsers
        connectivity/source/drivers/hsqldb/HView
        connectivity/source/drivers/hsqldb/HViews
        connectivity/source/drivers/hsqldb/StorageFileAccess
        connectivity/source/drivers/hsqldb/StorageNativeInputStream
        connectivity/source/drivers/hsqldb/StorageNativeOutputStream
        connectivity/source/drivers/hsqldb/accesslog
    )

    librestudio_expand_cxx(HSQLDB_LIB_SOURCES)

    add_library(hsqldb SHARED ${HSQLDB_LIB_SOURCES})

    target_include_directories(hsqldb PRIVATE
        "${CMAKE_SOURCE_DIR}/connectivity/inc"
        "${CMAKE_SOURCE_DIR}/connectivity/source/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(hsqldb udkapi_normal_headers offapi_normal_headers)

    target_link_libraries(hsqldb PRIVATE
        comphelper cppu cppuhelper dbtools jvmfwk sal salhelper tl utl vcl
        i18nlangtag
        Boost::boost)

    librestudio_add_component(hsqldb
        COMPONENT "${CMAKE_SOURCE_DIR}/connectivity/source/drivers/hsqldb/hsqldb.component"
        LIBRARY   hsqldb
        LAYER     OOO)

    librestudio_set_library_output_name(hsqldb hsqldb OOO)

    install(TARGETS hsqldb LIBRARY DESTINATION program)
endif ()

# --- jdbc (Library_jdbc.mk) -----------------------------------------------------
#
# 2026-09-29: the generic (non-HSQLDB-specific) JDBC bridge SDBC driver --
# wraps an arbitrary JDBC driver class through the Java-UNO bridge
# (jvmaccess -> JNI env access, already a real, already-ported target).
# use_common_precompiled_header is a gbuild PCH convenience, not
# reproduced (this port has no PCH-sharing mechanism, same as every other
# library here). The real .mk's own trailing order-only rule --
# `$(call gb_Library_get_target,jdbc) :| $(call gb_Library_get_target,
# affine_uno_uno)` -- is a build-ordering hint for jdbc's own unit tests
# (affine_uno_uno, cppu/CMakeLists.txt, already a real target), not a link
# dependency; reproduced as an order-only add_dependencies() below since
# it costs nothing and matches upstream exactly, even though this port
# does not yet build any jdbc-dependent test that would need the ordering.
if (LIBRESTUDIO_ENABLE_JAVA)
    set(JDBC_SOURCES
        connectivity/source/drivers/jdbc/Array
        connectivity/source/drivers/jdbc/Blob
        connectivity/source/drivers/jdbc/Boolean
        connectivity/source/drivers/jdbc/CallableStatement
        connectivity/source/drivers/jdbc/Class
        connectivity/source/drivers/jdbc/Clob
        connectivity/source/drivers/jdbc/ConnectionLog
        connectivity/source/drivers/jdbc/DatabaseMetaData
        connectivity/source/drivers/jdbc/Date
        connectivity/source/drivers/jdbc/DriverPropertyInfo
        connectivity/source/drivers/jdbc/Exception
        connectivity/source/drivers/jdbc/InputStream
        connectivity/source/drivers/jdbc/JConnection
        connectivity/source/drivers/jdbc/JDriver
        connectivity/source/drivers/jdbc/JStatement
        connectivity/source/drivers/jdbc/Object
        connectivity/source/drivers/jdbc/PreparedStatement
        connectivity/source/drivers/jdbc/Reader
        connectivity/source/drivers/jdbc/Ref
        connectivity/source/drivers/jdbc/ResultSet
        connectivity/source/drivers/jdbc/ResultSetMetaData
        connectivity/source/drivers/jdbc/SQLException
        connectivity/source/drivers/jdbc/SQLWarning
        connectivity/source/drivers/jdbc/String
        connectivity/source/drivers/jdbc/Throwable
        connectivity/source/drivers/jdbc/Timestamp
        connectivity/source/drivers/jdbc/JBigDecimal
        connectivity/source/drivers/jdbc/tools
        connectivity/source/drivers/jdbc/ContextClassLoader
    )

    librestudio_expand_cxx(JDBC_SOURCES)

    add_library(jdbc SHARED ${JDBC_SOURCES})

    target_include_directories(jdbc PRIVATE
        "${CMAKE_SOURCE_DIR}/connectivity/inc"
        "${CMAKE_SOURCE_DIR}/connectivity/source/inc"
        "${CONNECTIVITY_GENERATED_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(jdbc udkapi_normal_headers offapi_normal_headers affine_uno_uno)

    target_link_libraries(jdbc PRIVATE
        cppu cppuhelper sal salhelper jvmaccess dbtools tl utl jvmfwk comphelper
        Boost::boost)

    librestudio_add_component(jdbc
        COMPONENT "${CMAKE_SOURCE_DIR}/connectivity/source/drivers/jdbc/jdbc.component"
        LIBRARY   jdbc
        LAYER     OOO)

    librestudio_set_library_output_name(jdbc jdbc OOO)

    install(TARGETS jdbc LIBRARY DESTINATION program)
endif ()


# --- ENABLE_DBCONNECTIVITY: calc, dbase, flat, and their "file" prerequisite ---
#
# Module_connectivity.mk gates Library_calc/_dbase/_flat (and every other SDBC
# driver) behind "ifneq (,$(filter DBCONNECTIVITY,$(BUILD_TYPE)))"; ENABLE_
# DBCONNECTIVITY (cmake/LibreStudioConfigHeaders.cmake) is this port's
# stand-in for that BUILD_TYPE filter and defaults ON, matching upstream
# configure.ac's own DBCONNECTIVITY default on desktop Linux. dbtools above
# stays outside this if() because Module_connectivity.mk lists it
# unconditionally, ahead of and outside the DBCONNECTIVITY block.

if (ENABLE_DBCONNECTIVITY)

# file: the shared file-based driver framework (connectivity/source/drivers/
# component + .../file) that calc/dbase/flat each subclass (CTable/FTable,
# CConnection/FConnection, CDriver/FDriver, ...). Not named in Module_
# connectivity.mk's own DBCONNECTIVITY target list quoted for calc/dbase/flat
# below because it ships no .component of its own -- gb_Library_use_libraries
# pulls it in as a link-only prerequisite, so it is ported here the same way,
# transcribed from Library_file.mk.

set(FILE_SOURCES
    connectivity/source/drivers/component/CColumns
    connectivity/source/drivers/component/CDatabaseMetaData
    connectivity/source/drivers/component/CPreparedStatement
    connectivity/source/drivers/component/CResultSet
    connectivity/source/drivers/component/CStatement
    connectivity/source/drivers/component/CTable
    connectivity/source/drivers/file/FCatalog
    connectivity/source/drivers/file/FColumns
    connectivity/source/drivers/file/FConnection
    connectivity/source/drivers/file/FDatabaseMetaData
    connectivity/source/drivers/file/FDateFunctions
    connectivity/source/drivers/file/FDriver
    connectivity/source/drivers/file/FNoException
    connectivity/source/drivers/file/FNumericFunctions
    connectivity/source/drivers/file/FPreparedStatement
    connectivity/source/drivers/file/FResultSet
    connectivity/source/drivers/file/FResultSetMetaData
    connectivity/source/drivers/file/FStatement
    connectivity/source/drivers/file/FStringFunctions
    connectivity/source/drivers/file/FTable
    connectivity/source/drivers/file/FTables
    connectivity/source/drivers/file/fanalyzer
    connectivity/source/drivers/file/fcode
    connectivity/source/drivers/file/fcomp
    connectivity/source/drivers/file/quotedstring
)

librestudio_expand_cxx(FILE_SOURCES)

add_library(file SHARED ${FILE_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(file PRIVATE
        "${CMAKE_SOURCE_DIR}/connectivity/inc/pch/precompiled_file.hxx")
    target_compile_definitions(file PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(file PRIVATE
    "${CMAKE_SOURCE_DIR}/connectivity/inc"
    "${CMAKE_SOURCE_DIR}/connectivity/source/inc"
    "${CONNECTIVITY_GENERATED_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(file udkapi_normal_headers offapi_normal_headers)

# gb_Library_add_defs,file,-DOOO_DLLIMPLEMENTATION_FILE from Library_file.mk.
target_compile_definitions(file PRIVATE OOO_DLLIMPLEMENTATION_FILE)

target_link_libraries(file PRIVATE
    cppu cppuhelper sal salhelper
    svl tl ucbhelper dbtools utl comphelper
    Boost::boost)

librestudio_set_library_output_name(file file OOO)

# Library_file.mk has no gb_Library_set_componentfile call -- file is a
# link-time base for calc/dbase/flat, not itself a UNO component, so there is
# no librestudio_add_component() call here (compare dbtools/calc/dbase/flat,
# each of which is one).

install(TARGETS file LIBRARY DESTINATION program)


# calc: connectivity/Library_calc.mk, connectivity/source/drivers/calc.

set(CALC_SOURCES
    connectivity/source/drivers/calc/CDatabaseMetaData
    connectivity/source/drivers/calc/CCatalog
    connectivity/source/drivers/calc/CTable
    connectivity/source/drivers/calc/CTables
    connectivity/source/drivers/calc/CConnection
    connectivity/source/drivers/calc/CDriver
)

librestudio_expand_cxx(CALC_SOURCES)

add_library(calc SHARED ${CALC_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(calc PRIVATE
        "${CMAKE_SOURCE_DIR}/connectivity/inc/pch/precompiled_calc.hxx")
    target_compile_definitions(calc PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(calc PRIVATE
    "${CMAKE_SOURCE_DIR}/connectivity/inc"
    "${CMAKE_SOURCE_DIR}/connectivity/source/inc"
    "${CONNECTIVITY_GENERATED_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(calc udkapi_normal_headers offapi_normal_headers)

target_link_libraries(calc PRIVATE
    cppu cppuhelper sal salhelper
    svl tl utl dbtools file comphelper
    Boost::boost)

librestudio_add_component(calc
    COMPONENT "${CMAKE_SOURCE_DIR}/connectivity/source/drivers/calc/calc.component"
    LIBRARY   calc
    LAYER     OOO)

librestudio_set_library_output_name(calc calc OOO)

install(TARGETS calc LIBRARY DESTINATION program)


# dbase: connectivity/Library_dbase.mk, connectivity/source/drivers/dbase.

set(DBASE_SOURCES
    connectivity/source/drivers/dbase/DResultSet
    connectivity/source/drivers/dbase/DStatement
    connectivity/source/drivers/dbase/DPreparedStatement
    connectivity/source/drivers/dbase/dindexnode
    connectivity/source/drivers/dbase/DIndexIter
    connectivity/source/drivers/dbase/DDatabaseMetaData
    connectivity/source/drivers/dbase/DCatalog
    connectivity/source/drivers/dbase/DColumns
    connectivity/source/drivers/dbase/DIndexColumns
    connectivity/source/drivers/dbase/DIndex
    connectivity/source/drivers/dbase/DIndexes
    connectivity/source/drivers/dbase/DTables
    connectivity/source/drivers/dbase/DConnection
    connectivity/source/drivers/dbase/DDriver
    connectivity/source/drivers/dbase/DTable
)

librestudio_expand_cxx(DBASE_SOURCES)

add_library(dbase SHARED ${DBASE_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(dbase PRIVATE
        "${CMAKE_SOURCE_DIR}/connectivity/inc/pch/precompiled_dbase.hxx")
    target_compile_definitions(dbase PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(dbase PRIVATE
    "${CMAKE_SOURCE_DIR}/connectivity/inc"
    "${CMAKE_SOURCE_DIR}/connectivity/source/inc"
    "${CONNECTIVITY_GENERATED_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(dbase udkapi_normal_headers offapi_normal_headers)

target_link_libraries(dbase PRIVATE
    cppu cppuhelper sal salhelper
    svl tl ucbhelper dbtools file utl comphelper svt
    Boost::boost)

librestudio_add_component(dbase
    COMPONENT "${CMAKE_SOURCE_DIR}/connectivity/source/drivers/dbase/dbase.component"
    LIBRARY   dbase
    LAYER     OOO)

librestudio_set_library_output_name(dbase dbase OOO)

install(TARGETS dbase LIBRARY DESTINATION program)


# flat: connectivity/Library_flat.mk, connectivity/source/drivers/flat.

set(FLAT_SOURCES
    connectivity/source/drivers/flat/EResultSet
    connectivity/source/drivers/flat/EStatement
    connectivity/source/drivers/flat/EPreparedStatement
    connectivity/source/drivers/flat/ETable
    connectivity/source/drivers/flat/EDatabaseMetaData
    connectivity/source/drivers/flat/ECatalog
    connectivity/source/drivers/flat/EColumns
    connectivity/source/drivers/flat/ETables
    connectivity/source/drivers/flat/EConnection
    connectivity/source/drivers/flat/EDriver
)

librestudio_expand_cxx(FLAT_SOURCES)

add_library(flat SHARED ${FLAT_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(flat PRIVATE
        "${CMAKE_SOURCE_DIR}/connectivity/inc/pch/precompiled_flat.hxx")
    target_compile_definitions(flat PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(flat PRIVATE
    "${CMAKE_SOURCE_DIR}/connectivity/inc"
    "${CMAKE_SOURCE_DIR}/connectivity/source/inc"
    "${CONNECTIVITY_GENERATED_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(flat udkapi_normal_headers offapi_normal_headers)

target_link_libraries(flat PRIVATE
    cppu cppuhelper sal salhelper
    tl svl utl i18nlangtag dbtools file comphelper
    Boost::boost)

librestudio_add_component(flat
    COMPONENT "${CMAKE_SOURCE_DIR}/connectivity/source/drivers/flat/flat.component"
    LIBRARY   flat
    LAYER     OOO)

librestudio_set_library_output_name(flat flat OOO)

install(TARGETS flat LIBRARY DESTINATION program)

# --- 2026-09-29: remaining DBCONNECTIVITY libraries -----------------------------
#
# odbc, sdbc2, mysql_jdbc, writer, dbpool2, mozbootstrap: Module_
# connectivity.mk's own DBCONNECTIVITY block also lists these five
# unconditionally (odbc additionally gated off only for ANDROID/iOS, not
# Linux) plus mozbootstrap, nested one level deeper behind `ifneq (,$(filter
# DESKTOP,$(BUILD_TYPE)))` -- a real, always-true condition on this port,
# same as DBCONNECTIVITY itself (cmake/LibreStudioConfigHeaders.cmake's own
# USE_AFDKO_PROGRAMS comment: "BUILD_TYPE is DESKTOP", an established fact
# here, not a guess), so mozbootstrap is placed in this same if()-block
# rather than a separate one. This CMakeLists.txt's own header comment
# previously listed all six as "genuinely missing production libraries...
# not attempted in this pass" -- corrected here now that they are.
#
# None of the six needed a new external: boost_headers (all), officecfg/
# registry custom headers (sdbc2 only, same OFFICECFG_INCLUDE_DIR pattern
# as filterconfig/pdffilter elsewhere in this port), and odbc_headers
# (odbc only) -- RepositoryExternal.mk's own gb_LinkTarget__use_odbc_
# headers (read in full): a real system-or-vendor CHOICE, but even the
# vendored branch is a bare -I onto external/unixODBC/inc/odbc (4 header
# files -- sql.h/sqlext.h/sqltypes.h/sqlucode.h -- already checked into
# this repo, no tarball fetch or build step at all). No system unixODBC-
# devel package is installed on this machine (find_path() for sql.h finds
# nothing anywhere on the default search path, confirmed), so the bundled
# path is what actually resolves here. Library_odbc.mk itself never links
# a real ODBC runtime library either (no libodbc in its own use_externals/
# use_libraries) -- connectivity/source/drivers/odbc/ODriver.cxx loads the
# system's real ODBC driver manager via osl::Module at RUNTIME (dlopen),
# confirmed by reading it, so no link-time ODBC library dependency exists
# to resolve regardless.
find_path(ODBC_HEADERS_INCLUDE_DIR sql.h)
if (ODBC_HEADERS_INCLUDE_DIR)
    set(ODBC_HEADERS_DEFS SYSTEM_ODBC_HEADERS)
else ()
    set(ODBC_HEADERS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/unixODBC/inc/odbc")
    set(ODBC_HEADERS_DEFS "")
endif ()

# dbpool2: connectivity/Library_dbpool2.mk, connectivity/source/cpool. The
# generic SDBC connection-pooling wrapper (OConnectionPool service) any
# driver can opt into -- no use_external at all beyond boost_headers is
# NOT even declared (re-checked: Library_dbpool2.mk has no gb_Library_
# use_external/use_externals call whatsoever), so Boost::boost is
# deliberately omitted here, unlike every other library in this file.

set(DBPOOL2_SOURCES
    connectivity/source/cpool/ZConnectionWrapper
    connectivity/source/cpool/ZDriverWrapper
    connectivity/source/cpool/ZPooledConnection
    connectivity/source/cpool/ZConnectionPool
    connectivity/source/cpool/ZPoolCollection
)

librestudio_expand_cxx(DBPOOL2_SOURCES)

add_library(dbpool2 SHARED ${DBPOOL2_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(dbpool2 PRIVATE
        "${CMAKE_SOURCE_DIR}/connectivity/inc/pch/precompiled_dbpool2.hxx")
    target_compile_definitions(dbpool2 PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(dbpool2 PRIVATE
    "${CMAKE_SOURCE_DIR}/connectivity/inc"
    "${CMAKE_SOURCE_DIR}/connectivity/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(dbpool2 udkapi_normal_headers offapi_normal_headers)

target_link_libraries(dbpool2 PRIVATE
    comphelper cppu cppuhelper dbtools sal salhelper tl)

librestudio_add_component(dbpool2
    COMPONENT "${CMAKE_SOURCE_DIR}/connectivity/source/cpool/dbpool2.component"
    LIBRARY   dbpool2
    LAYER     OOO)

librestudio_set_library_output_name(dbpool2 dbpool2 OOO)

install(TARGETS dbpool2 LIBRARY DESTINATION program)


# odbc: connectivity/Library_odbc.mk, connectivity/source/drivers/odbc.

set(ODBC_SOURCES
    connectivity/source/drivers/odbc/OPreparedStatement
    connectivity/source/drivers/odbc/OStatement
    connectivity/source/drivers/odbc/OResultSetMetaData
    connectivity/source/drivers/odbc/OResultSet
    connectivity/source/drivers/odbc/OTools
    connectivity/source/drivers/odbc/ODatabaseMetaDataResultSet
    connectivity/source/drivers/odbc/ODatabaseMetaData
    connectivity/source/drivers/odbc/ODriver
    connectivity/source/drivers/odbc/OConnection
)

librestudio_expand_cxx(ODBC_SOURCES)

add_library(odbc SHARED ${ODBC_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(odbc PRIVATE
        "${CMAKE_SOURCE_DIR}/connectivity/inc/pch/precompiled_odbc.hxx")
    target_compile_definitions(odbc PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(odbc PRIVATE
    "${CMAKE_SOURCE_DIR}/connectivity/inc"
    "${CMAKE_SOURCE_DIR}/connectivity/source/inc"
    "${CONNECTIVITY_GENERATED_DIR}"
    "${ODBC_HEADERS_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(odbc udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(odbc PRIVATE
    OOO_DLLIMPLEMENTATION_ODBCBASE
    ${ODBC_HEADERS_DEFS})

target_link_libraries(odbc PRIVATE
    cppu cppuhelper comphelper dbtools sal salhelper utl
    Boost::boost)

librestudio_add_component(odbc
    COMPONENT "${CMAKE_SOURCE_DIR}/connectivity/source/drivers/odbc/odbc.component"
    LIBRARY   odbc
    LAYER     OOO)

librestudio_set_library_output_name(odbc odbc OOO)

install(TARGETS odbc LIBRARY DESTINATION program)


# mysql_jdbc: connectivity/Library_mysql_jdbc.mk, connectivity/source/
# drivers/mysql_jdbc. Despite the name, this is NOT a Java/JDBC bridge --
# it is the generic "Y"-prefixed catalog/schema/table/view/column
# reflection layer LibreOffice wraps around an ALREADY-connected native or
# JDBC MySQL/MariaDB connection to give it full SDBC catalog awareness;
# the actual connection itself is established elsewhere (a JDBC driver
# loaded at runtime, or -- for the also-unported mysqlc, ENABLE_MARIADBC --
# a native Connector-C link this port has not vendored). No ENABLE_JAVA
# gate at all in the real .mk (unlike hsqldb/jdbc above), confirmed
# directly -- it is unconditional in the DBCONNECTIVITY block.

set(MYSQL_JDBC_SOURCES
    connectivity/source/drivers/mysql_jdbc/YDriver
    connectivity/source/drivers/mysql_jdbc/YTables
    connectivity/source/drivers/mysql_jdbc/YTable
    connectivity/source/drivers/mysql_jdbc/YViews
    connectivity/source/drivers/mysql_jdbc/YCatalog
    connectivity/source/drivers/mysql_jdbc/YColumns
    connectivity/source/drivers/mysql_jdbc/YUser
    connectivity/source/drivers/mysql_jdbc/YUsers
)

librestudio_expand_cxx(MYSQL_JDBC_SOURCES)

add_library(mysql_jdbc SHARED ${MYSQL_JDBC_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(mysql_jdbc PRIVATE
        "${CMAKE_SOURCE_DIR}/connectivity/inc/pch/precompiled_mysql_jdbc.hxx")
    target_compile_definitions(mysql_jdbc PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(mysql_jdbc PRIVATE
    "${CMAKE_SOURCE_DIR}/connectivity/inc"
    "${CMAKE_SOURCE_DIR}/connectivity/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(mysql_jdbc udkapi_normal_headers offapi_normal_headers)

target_link_libraries(mysql_jdbc PRIVATE
    cppu cppuhelper sal salhelper utl dbtools comphelper
    Boost::boost)

librestudio_add_component(mysql_jdbc
    COMPONENT "${CMAKE_SOURCE_DIR}/connectivity/source/drivers/mysql_jdbc/mysql_jdbc.component"
    LIBRARY   mysql_jdbc
    LAYER     OOO)

librestudio_set_library_output_name(mysql_jdbc mysql_jdbc OOO)

install(TARGETS mysql_jdbc LIBRARY DESTINATION program)


# sdbc2: connectivity/Library_sdbc2.mk, connectivity/source/manager. The
# SDBC DriverManager service (mdrivermanager.cxx) that dbaccess and every
# other SDBC consumer asks for a connection through. use_custom_headers
# officecfg/registry -> OFFICECFG_INCLUDE_DIR + officecfg_headers, same
# pattern as filter/CMakeLists.txt's own filterconfig/pdffilter.

set(SDBC2_SOURCES
    connectivity/source/manager/mdrivermanager
)

librestudio_expand_cxx(SDBC2_SOURCES)

add_library(sdbc2 SHARED ${SDBC2_SOURCES})

target_include_directories(sdbc2 PRIVATE
    "${CMAKE_SOURCE_DIR}/connectivity/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(sdbc2 officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(sdbc2 PRIVATE
    cppu cppuhelper comphelper dbtools utl sal tl
    Boost::boost)

librestudio_add_component(sdbc2
    COMPONENT "${CMAKE_SOURCE_DIR}/connectivity/source/manager/sdbc2.component"
    LIBRARY   sdbc2
    LAYER     OOO)

librestudio_set_library_output_name(sdbc2 sdbc2 OOO)

install(TARGETS sdbc2 LIBRARY DESTINATION program)


# writer: connectivity/Library_writer.mk, connectivity/source/drivers/
# writer. The OpenDocument text ("Writer" documents as read-only tables)
# SDBC driver -- links "file" (built above), the shared file-driver
# framework calc/dbase/flat also subclass.

set(WRITER_SOURCES
    connectivity/source/drivers/writer/WCatalog
    connectivity/source/drivers/writer/WConnection
    connectivity/source/drivers/writer/WDatabaseMetaData
    connectivity/source/drivers/writer/WDriver
    connectivity/source/drivers/writer/WTable
    connectivity/source/drivers/writer/WTables
)

librestudio_expand_cxx(WRITER_SOURCES)

add_library(writer SHARED ${WRITER_SOURCES})

target_include_directories(writer PRIVATE
    "${CMAKE_SOURCE_DIR}/connectivity/inc"
    "${CMAKE_SOURCE_DIR}/connectivity/source/inc"
    "${CONNECTIVITY_GENERATED_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(writer udkapi_normal_headers offapi_normal_headers)

target_link_libraries(writer PRIVATE
    cppu cppuhelper svl tl utl sal salhelper dbtools file comphelper
    Boost::boost)

librestudio_add_component(writer
    COMPONENT "${CMAKE_SOURCE_DIR}/connectivity/source/drivers/writer/writer.component"
    LIBRARY   writer
    LAYER     OOO)

librestudio_set_library_output_name(writer writer OOO)

install(TARGETS writer LIBRARY DESTINATION program)


# mozbootstrap: connectivity/Library_mozbootstrap.mk, connectivity/source/
# drivers/mozab/bootstrap. Locates an existing Mozilla/Thunderbird address-
# book profile on disk (INI-file parsing, profile discovery) for the
# read-only Mozilla/LDAP address-book SDBC driver (Library_mozab/
# mozabdrv -- neither of which is ported; mozbootstrap alone ships its own
# .component and is listed as a standalone Module_connectivity.mk target).

set(MOZBOOTSTRAP_SOURCES
    connectivity/source/drivers/mozab/bootstrap/MMozillaBootstrap
    connectivity/source/drivers/mozab/bootstrap/MNSFolders
    connectivity/source/drivers/mozab/bootstrap/MNSINIParser
    connectivity/source/drivers/mozab/bootstrap/MNSProfileDiscover
)

librestudio_expand_cxx(MOZBOOTSTRAP_SOURCES)

add_library(mozbootstrap SHARED ${MOZBOOTSTRAP_SOURCES})

target_include_directories(mozbootstrap PRIVATE
    "${CMAKE_SOURCE_DIR}/connectivity/source/drivers/mozab"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(mozbootstrap udkapi_normal_headers offapi_normal_headers)

target_link_libraries(mozbootstrap PRIVATE
    comphelper cppu cppuhelper sal)

librestudio_add_component(mozbootstrap
    COMPONENT "${CMAKE_SOURCE_DIR}/connectivity/source/drivers/mozab/bootstrap/mozbootstrap.component"
    LIBRARY   mozbootstrap
    LAYER     OOO)

librestudio_set_library_output_name(mozbootstrap mozbootstrap OOO)

install(TARGETS mozbootstrap LIBRARY DESTINATION program)

endif (ENABLE_DBCONNECTIVITY)

# --- CppunitTests --------------------------------------------------------------
#
# connectivity has 5 CppunitTest_*.mk files. connectivity_commontools and
# connectivity_sharedresources need only "dbtools" (built unconditionally
# above, outside the ENABLE_DBCONNECTIVITY block, so this section is placed
# after that endif deliberately, not gated by it) plus configmgr/i18npool
# components; both need deferral purely for use_configuration's own
# LIBRESTUDIO_TEST_UNITTEST_DIR requirement (test/CMakeLists.txt runs well
# after this module). connectivity_ado/connectivity_firebird_test/
# connectivity_mysql_test are NOT ported -- see connectivity/CppunitTest_
# deferred.cmake's own header comment for why: each needs exactly one of the
# driver libraries (ado, firebird_sdbc, mysqlc) this module's own header
# comment above already documents as not yet ported, for its own specific
# technical reason (ado: Windows-only COM API; firebird_sdbc/mysqlc: need an
# external client library this port has not vendored) -- not a scope
# decision, just not attempted in this pass.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/connectivity/CppunitTest_deferred.cmake")
