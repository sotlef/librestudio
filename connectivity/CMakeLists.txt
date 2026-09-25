# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# connectivity — only dbtools is built here: the shared SQL-parsing and
# database-helper library that svx (fmcomp/gridcell, form/sqlparserclient,
# form/fmundo, ...) links against unconditionally in upstream, because
# HAVE_FEATURE_DBCONNECTIVITY defaults to enabled on desktop Linux (it is only
# forced off under --enable-wasm-strip -- see configure.ac). The actual SDBC
# drivers (ado, calc, dbase, evoab, firebird_sdbc, hsqldb, jdbc, macab1,
# macabdrv1, mozbootstrap, mysqlc, mysql_jdbc, odbc, postgresql-sdbc(-impl),
# sdbc2, writer) are out of scope for this port and are not built.
#
# Source list, includes and library set are transcribed from
# Library_dbtools.mk; dbtools is registered in the OOO layer, matching
# Repository.mk's gb_Helper_register_libraries_for_install(OOOLIBS,ooo,...).

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
