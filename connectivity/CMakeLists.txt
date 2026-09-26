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
# All other SDBC drivers (ado, evoab, firebird_sdbc, hsqldb, jdbc, macab1,
# macabdrv1, mozbootstrap, mysqlc, mysql_jdbc, odbc, postgresql-sdbc(-impl),
# sdbc2, writer) remain out of scope for this port and are not built.
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

endif (ENABLE_DBCONNECTIVITY)
