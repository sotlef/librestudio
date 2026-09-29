# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# dbaccess (module dbaccess) -- five libraries, in dependency order:
#  - dbahsql: the HSQLDB-to-Firebird migration parser (schema/table/column
#    DDL parsing plus binary row import) used when upgrading an embedded
#    database; no internal dbaccess dependency.
#  - dba:     the core database-document/rowset/query engine (UNO services
#    for DataSource, DatabaseContext, RowSet, QueryComposer, ...); depends
#    on dbahsql.
#  - dbu:     the database application UI (Base's app window, query/table/
#    relation designers, wizards and dialogs); depends on dba.
#  - dbaxml:  the ODB (.odb) XML settings filter; depends on dba.
#  - sdbt:    small SDB connection-tools helper services (ConnectionTools,
#    object-name/table-name quoting); depends on dba.
# Source lists, includes, defines and componentfiles are transcribed from
# Library_dbahsql.mk, Library_dba.mk, Library_dbu.mk, Library_dbaxml.mk and
# Library_sdbt.mk. The precompiled headers (precompiled_dbahsql/dba/dbu/
# dbaxml/sdbt) are not ported, matching every other module in this port.
#
# dbahsql, dba and dbu each ask (gb_Library_set_include) for
# -I$(WORKDIR)/YaccTarget/connectivity/source/parse -- the directory bison
# writes sqlbison.hxx into from connectivity/source/parse/sqlbison.y, via
# connectivity/CMakeLists.txt's bison_target(SqlBisonParser...).
# dbaccess/source/core/api/{RowSetCache,SingleSelectQueryComposer}.cxx (dba)
# and dbaccess/source/ui/querydesign/{QueryDesignView,SelectionBrowseBox}.cxx
# (dbu) '#include <sqlbison.hxx>' directly; dbahsql's own sources do not, but
# the -I is carried over here for fidelity with the .mk. connectivity's own
# CONNECTIVITY_GENERATED_DIR is a plain directory-scoped variable, not a
# cache/global one, so it cannot be read back from here; rather than reach
# into connectivity's build directory by reconstructing its path, the same
# bison_target() invocation against the same .y file is repeated below --
# the same "regenerate the one artifact this module needs instead of poking
# at another module's build tree" approach already used by
# svx/CMakeLists.txt for oox/source/token/tokens.hxx. Only the DEFINES_FILE
# (the header) is consumed here; the generated parser .cxx is deliberately
# never added to any add_library in this file, since it is already compiled
# once into dbtools by connectivity/CMakeLists.txt -- linking against
# dbtools still happens normally through the ordinary use_libraries lists.
#
# dbu's `gb_Library_use_externals,dbu,odbc_headers` is not reproduced: the
# only place it would matter, dbaccess/source/ui/dlg/odbcconfig.cxx, guards
# its real ODBC usage (`#include <connectivity/odbc.hxx>`) behind
# HAVE_ODBC_SUPPORT, which config_features.h.in never defines in this port
# (connectivity's own Library_odbc is not built here either -- a real,
# not-yet-attempted gap, not a scope decision, re-corrected 2026-09-28 in
# connectivity/CMakeLists.txt's own header comment after an earlier version
# mischaracterized it -- see [[feedback_gbuild_full_replacement_scope]]),
# so that branch never compiles and no ODBC headers are ever needed here
# either way.
#
# dbu's `ifeq ($(COM),MSC)` (-DADO_SUPPRESS_MESSAGE) and `ifeq ($(OS),WNT)`
# (ole32/oleaut32/uuid, dbaccess/source/ui/dlg/adodatalinks) blocks are
# Windows-only and excluded, same as every other module in this Linux-only
# port.

find_package(BISON 3.0 REQUIRED)

set(DBACCESS_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

bison_target(DbaccessSqlBisonParser
    "${CMAKE_SOURCE_DIR}/connectivity/source/parse/sqlbison.y"
    "${DBACCESS_GENERATED_DIR}/sqlbison.cxx"
    DEFINES_FILE "${DBACCESS_GENERATED_DIR}/sqlbison.hxx"
    COMPILE_FLAGS "-l -p SQLyy -b sql")

add_custom_target(dbaccess_sqlbison_header
    DEPENDS "${DBACCESS_GENERATED_DIR}/sqlbison.hxx")

# ---------------------------------------------------------------------------
# dbahsql -- Library_dbahsql.mk
# ---------------------------------------------------------------------------

set(DBAHSQL_SOURCES
    dbaccess/source/filter/hsqldb/hsqlimport
    dbaccess/source/filter/hsqldb/parseschema
    dbaccess/source/filter/hsqldb/alterparser
    dbaccess/source/filter/hsqldb/createparser
    dbaccess/source/filter/hsqldb/columndef
    dbaccess/source/filter/hsqldb/fbalterparser
    dbaccess/source/filter/hsqldb/fbcreateparser
    dbaccess/source/filter/hsqldb/rowinputbinary
    dbaccess/source/filter/hsqldb/hsqlbinarynode
    dbaccess/source/filter/hsqldb/utils
)

librestudio_expand_cxx(DBAHSQL_SOURCES)
add_library(dbahsql SHARED ${DBAHSQL_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(dbahsql PRIVATE
        "${CMAKE_SOURCE_DIR}/dbaccess/inc/pch/precompiled_dbahsql.hxx")
    target_compile_definitions(dbahsql PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(dbahsql PRIVATE
    "${DBACCESS_GENERATED_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(dbahsql
    officecfg_headers udkapi_normal_headers offapi_normal_headers
    dbaccess_sqlbison_header)

target_link_libraries(dbahsql PRIVATE
    comphelper cppu cppuhelper sal salhelper dbtools ucbhelper utl tl)

# No gb_Library_set_componentfile in Library_dbahsql.mk -- dbahsql is an
# internal helper library with no UNO services of its own.

librestudio_set_library_output_name(dbahsql dbahsql OOO)

install(TARGETS dbahsql LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# dba -- Library_dba.mk
# ---------------------------------------------------------------------------

set(DBA_SOURCES
    dbaccess/source/core/api/BookmarkSet
    dbaccess/source/core/api/CacheSet
    dbaccess/source/core/api/callablestatement
    dbaccess/source/core/api/CIndexes
    dbaccess/source/core/api/column
    dbaccess/source/core/api/columnsettings
    dbaccess/source/core/api/CRowSetColumn
    dbaccess/source/core/api/CRowSetDataColumn
    dbaccess/source/core/api/datacolumn
    dbaccess/source/core/api/datasettings
    dbaccess/source/core/api/definitioncolumn
    dbaccess/source/core/api/FilteredContainer
    dbaccess/source/core/api/HelperCollections
    dbaccess/source/core/api/KeySet
    dbaccess/source/core/api/OptimisticSet
    dbaccess/source/core/api/preparedstatement
    dbaccess/source/core/api/PrivateRow
    dbaccess/source/core/api/query
    dbaccess/source/core/api/querycomposer
    dbaccess/source/core/api/querycontainer
    dbaccess/source/core/api/querydescriptor
    dbaccess/source/core/api/resultcolumn
    dbaccess/source/core/api/resultset
    dbaccess/source/core/api/RowSet
    dbaccess/source/core/api/RowSetBase
    dbaccess/source/core/api/RowSetCache
    dbaccess/source/core/api/RowSetCacheIterator
    dbaccess/source/core/api/SingleSelectQueryComposer
    dbaccess/source/core/api/statement
    dbaccess/source/core/api/StaticSet
    dbaccess/source/core/api/table
    dbaccess/source/core/api/tablecontainer
    dbaccess/source/core/api/TableDeco
    dbaccess/source/core/api/View
    dbaccess/source/core/api/viewcontainer
    dbaccess/source/core/api/WrappedResultSet
    dbaccess/source/core/dataaccess/bookmarkcontainer
    dbaccess/source/core/dataaccess/commandcontainer
    dbaccess/source/core/dataaccess/commanddefinition
    dbaccess/source/core/dataaccess/ComponentDefinition
    dbaccess/source/core/dataaccess/connection
    dbaccess/source/core/dataaccess/ContentHelper
    dbaccess/source/core/dataaccess/dataaccessdescriptor
    dbaccess/source/core/dataaccess/databasecontext
    dbaccess/source/core/dataaccess/databasedocument
    dbaccess/source/core/dataaccess/databaseregistrations
    dbaccess/source/core/dataaccess/datasource
    dbaccess/source/core/dataaccess/definitioncontainer
    dbaccess/source/core/dataaccess/documentcontainer
    dbaccess/source/core/dataaccess/documentdefinition
    dbaccess/source/core/dataaccess/documenteventexecutor
    dbaccess/source/core/dataaccess/documenteventnotifier
    dbaccess/source/core/dataaccess/documentevents
    dbaccess/source/core/dataaccess/intercept
    dbaccess/source/core/dataaccess/ModelImpl
    dbaccess/source/core/dataaccess/myucp_datasupplier
    dbaccess/source/core/dataaccess/myucp_resultset
    dbaccess/source/core/dataaccess/SharedConnection
    dbaccess/source/core/misc/apitools
    dbaccess/source/core/misc/ContainerMediator
    dbaccess/source/core/misc/DatabaseDataProvider
    dbaccess/source/core/misc/dsntypes
    dbaccess/source/core/misc/objectnameapproval
    dbaccess/source/core/misc/migrwarndlg
    dbaccess/source/core/misc/PropertyForward
    dbaccess/source/core/misc/sdbcoretools
    dbaccess/source/core/misc/veto
    dbaccess/source/core/recovery/dbdocrecovery
    dbaccess/source/core/recovery/settingsimport
    dbaccess/source/core/recovery/storagestream
    dbaccess/source/core/recovery/storagetextstream
    dbaccess/source/core/recovery/storagexmlstream
    dbaccess/source/core/recovery/subcomponentloader
    dbaccess/source/core/recovery/subcomponentrecovery
    dbaccess/source/core/resource/core_resource
)

librestudio_expand_cxx(DBA_SOURCES)
add_library(dba SHARED ${DBA_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(dba PRIVATE
        "${CMAKE_SOURCE_DIR}/dbaccess/inc/pch/precompiled_dba.hxx")
    target_compile_definitions(dba PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(dba PRIVATE
    "${CMAKE_SOURCE_DIR}/dbaccess/inc"
    "${CMAKE_SOURCE_DIR}/dbaccess/source/inc"
    "${CMAKE_SOURCE_DIR}/dbaccess/source/core/inc"
    "${CMAKE_SOURCE_DIR}/dbaccess/source/filter/hsqldb"
    "${DBACCESS_GENERATED_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(dba
    officecfg_headers udkapi_normal_headers offapi_normal_headers
    dbaccess_sqlbison_header)

target_compile_definitions(dba PRIVATE OOO_DLLIMPLEMENTATION_DBA)

target_link_libraries(dba PRIVATE
    comphelper cppu cppuhelper dbahsql dbtools fwk i18nlangtag sal salhelper
    sax sb sfx svl svt svxcore tl ucbhelper utl vcl xo
    Boost::boost)

librestudio_add_component(dba
    COMPONENT "${CMAKE_SOURCE_DIR}/dbaccess/util/dba.component"
    LIBRARY   dba
    LAYER     OOO)

librestudio_set_library_output_name(dba dba OOO)

install(TARGETS dba LIBRARY DESTINATION program)

# dbu, dbaxml and sdbt are the three DBCONNECTIVITY-gated targets in
# Module_dbaccess.mk's "ifneq (,$(filter DBCONNECTIVITY,$(BUILD_TYPE)))"
# block (dbahsql and dba above are added unconditionally, outside that
# filter, and are left untouched).

if (ENABLE_DBCONNECTIVITY)

# ---------------------------------------------------------------------------
# dbu -- Library_dbu.mk
# ---------------------------------------------------------------------------

set(DBU_SOURCES
    dbaccess/source/ui/app/AppController
    dbaccess/source/ui/app/AppControllerDnD
    dbaccess/source/ui/app/AppControllerGen
    dbaccess/source/ui/app/AppDetailPageHelper
    dbaccess/source/ui/app/AppDetailView
    dbaccess/source/ui/app/AppIconControl
    dbaccess/source/ui/app/AppSwapWindow
    dbaccess/source/ui/app/AppTitleWindow
    dbaccess/source/ui/app/AppView
    dbaccess/source/ui/app/ChildWindow
    dbaccess/source/ui/app/DocumentInfoPreview
    dbaccess/source/ui/app/subcomponentmanager
    dbaccess/source/ui/app/templwin
    dbaccess/source/ui/browser/AsynchronousLink
    dbaccess/source/ui/browser/brwctrlr
    dbaccess/source/ui/browser/brwview
    dbaccess/source/ui/browser/dataview
    dbaccess/source/ui/browser/dbexchange
    dbaccess/source/ui/browser/dbloader
    dbaccess/source/ui/browser/dbtreemodel
    dbaccess/source/ui/browser/dsbrowserDnD
    dbaccess/source/ui/browser/dsEntriesNoExp
    dbaccess/source/ui/browser/exsrcbrw
    dbaccess/source/ui/browser/formadapter
    dbaccess/source/ui/browser/genericcontroller
    dbaccess/source/ui/browser/sbagrid
    dbaccess/source/ui/browser/sbamultiplex
    dbaccess/source/ui/browser/unodatbr
    dbaccess/source/ui/control/charsetlistbox
    dbaccess/source/ui/control/ColumnControlWindow
    dbaccess/source/ui/control/curledit
    dbaccess/source/ui/control/dbtreelistbox
    dbaccess/source/ui/control/FieldControls
    dbaccess/source/ui/control/FieldDescControl
    dbaccess/source/ui/control/opendoccontrols
    dbaccess/source/ui/control/RelationControl
    dbaccess/source/ui/control/sqledit
    dbaccess/source/ui/control/SqlNameEdit
    dbaccess/source/ui/control/TableGrantCtrl
    dbaccess/source/ui/control/tabletree
    dbaccess/source/ui/control/undosqledit
    dbaccess/source/ui/dlg/admincontrols
    dbaccess/source/ui/dlg/adminpages
    dbaccess/source/ui/dlg/adtabdlg
    dbaccess/source/ui/dlg/advancedsettings
    dbaccess/source/ui/dlg/CollectionView
    dbaccess/source/ui/dlg/ConnectionHelper
    dbaccess/source/ui/dlg/ConnectionPage
    dbaccess/source/ui/dlg/ConnectionPageSetup
    dbaccess/source/ui/dlg/dbadmin
    dbaccess/source/ui/dlg/DbAdminImpl
    dbaccess/source/ui/dlg/dbfindex
    dbaccess/source/ui/dlg/DBSetupConnectionPages
    dbaccess/source/ui/dlg/dbwiz
    dbaccess/source/ui/dlg/dbwizsetup
    dbaccess/source/ui/dlg/detailpages
    dbaccess/source/ui/dlg/directsql
    dbaccess/source/ui/dlg/dlgattr
    dbaccess/source/ui/dlg/dlgsave
    dbaccess/source/ui/dlg/dlgsize
    dbaccess/source/ui/dlg/dsselect
    dbaccess/source/ui/dlg/finteraction
    dbaccess/source/ui/dlg/generalpage
    dbaccess/source/ui/dlg/indexdialog
    dbaccess/source/ui/dlg/indexfieldscontrol
    dbaccess/source/ui/dlg/odbcconfig
    dbaccess/source/ui/dlg/optionalboolitem
    dbaccess/source/ui/dlg/paramdialog
    dbaccess/source/ui/dlg/queryfilter
    dbaccess/source/ui/dlg/queryorder
    dbaccess/source/ui/dlg/QueryPropertiesDialog
    dbaccess/source/ui/dlg/RelationDlg
    dbaccess/source/ui/dlg/sqlmessage
    dbaccess/source/ui/dlg/tablespage
    dbaccess/source/ui/dlg/TablesSingleDlg
    dbaccess/source/ui/dlg/TextConnectionHelper
    dbaccess/source/ui/dlg/textconnectionsettings
    dbaccess/source/ui/dlg/UserAdmin
    dbaccess/source/ui/dlg/UserAdminDlg
    dbaccess/source/ui/misc/asyncmodaldialog
    dbaccess/source/ui/misc/charsets
    dbaccess/source/ui/misc/controllerframe
    dbaccess/source/ui/misc/databaseobjectview
    dbaccess/source/ui/misc/datasourceconnector
    dbaccess/source/ui/misc/dbaundomanager
    dbaccess/source/ui/misc/dbsubcomponentcontroller
    dbaccess/source/ui/misc/defaultobjectnamecheck
    dbaccess/source/ui/misc/DExport
    dbaccess/source/ui/misc/dsmeta
    dbaccess/source/ui/misc/HtmlReader
    dbaccess/source/ui/misc/imageprovider
    dbaccess/source/ui/misc/indexcollection
    dbaccess/source/ui/misc/linkeddocuments
    dbaccess/source/ui/misc/propertystorage
    dbaccess/source/ui/misc/RowSetDrop
    dbaccess/source/ui/misc/RtfReader
    dbaccess/source/ui/misc/singledoccontroller
    dbaccess/source/ui/misc/stringlistitem
    dbaccess/source/ui/misc/TableCopyHelper
    dbaccess/source/ui/misc/TokenWriter
    dbaccess/source/ui/misc/UITools
    dbaccess/source/ui/misc/WColumnSelect
    dbaccess/source/ui/misc/WCopyTable
    dbaccess/source/ui/misc/WCPage
    dbaccess/source/ui/misc/WExtendPages
    dbaccess/source/ui/misc/WNameMatch
    dbaccess/source/ui/misc/WTypeSelect
    dbaccess/source/ui/querydesign/ConnectionLine
    dbaccess/source/ui/querydesign/ConnectionLineAccess
    dbaccess/source/ui/querydesign/ConnectionLineData
    dbaccess/source/ui/querydesign/JAccess
    dbaccess/source/ui/querydesign/JoinController
    dbaccess/source/ui/querydesign/JoinDesignView
    dbaccess/source/ui/querydesign/JoinExchange
    dbaccess/source/ui/querydesign/JoinTableView
    dbaccess/source/ui/querydesign/limitboxcontroller
    dbaccess/source/ui/querydesign/QTableConnection
    dbaccess/source/ui/querydesign/QTableConnectionData
    dbaccess/source/ui/querydesign/QTableWindow
    dbaccess/source/ui/querydesign/QTableWindowData
    dbaccess/source/ui/querydesign/querycontainerwindow
    dbaccess/source/ui/querydesign/querycontroller
    dbaccess/source/ui/querydesign/QueryDesignView
    dbaccess/source/ui/querydesign/querydlg
    dbaccess/source/ui/querydesign/QueryMoveTabWinUndoAct
    dbaccess/source/ui/querydesign/QueryTabConnUndoAction
    dbaccess/source/ui/querydesign/QueryTableView
    dbaccess/source/ui/querydesign/QueryTabWinUndoAct
    dbaccess/source/ui/querydesign/QueryTextView
    dbaccess/source/ui/querydesign/QueryViewSwitch
    dbaccess/source/ui/querydesign/SelectionBrowseBox
    dbaccess/source/ui/querydesign/TableConnection
    dbaccess/source/ui/querydesign/TableConnectionData
    dbaccess/source/ui/querydesign/TableFieldDescription
    dbaccess/source/ui/querydesign/TableFieldInfo
    dbaccess/source/ui/querydesign/TableWindow
    dbaccess/source/ui/querydesign/TableWindowAccess
    dbaccess/source/ui/querydesign/TableWindowData
    dbaccess/source/ui/querydesign/TableWindowListBox
    dbaccess/source/ui/querydesign/TableWindowTitle
    dbaccess/source/ui/relationdesign/RelationController
    dbaccess/source/ui/relationdesign/RelationDesignView
    dbaccess/source/ui/relationdesign/RelationTableView
    dbaccess/source/ui/relationdesign/RTableConnection
    dbaccess/source/ui/relationdesign/RTableConnectionData
    dbaccess/source/ui/tabledesign/FieldDescriptions
    dbaccess/source/ui/tabledesign/TableController
    dbaccess/source/ui/tabledesign/TableDesignControl
    dbaccess/source/ui/tabledesign/TableDesignHelpBar
    dbaccess/source/ui/tabledesign/TableDesignView
    dbaccess/source/ui/tabledesign/TableFieldControl
    dbaccess/source/ui/tabledesign/TableFieldDescWin
    dbaccess/source/ui/tabledesign/TableRow
    dbaccess/source/ui/tabledesign/TableRowExchange
    dbaccess/source/ui/tabledesign/TableUndo
    dbaccess/source/ui/tabledesign/TEditControl
    dbaccess/source/ui/uno/admindlg
    dbaccess/source/ui/uno/AdvancedSettingsDlg
    dbaccess/source/ui/uno/ColumnControl
    dbaccess/source/ui/uno/ColumnModel
    dbaccess/source/ui/uno/ColumnPeer
    dbaccess/source/ui/uno/composerdialogs
    dbaccess/source/ui/uno/copytablewizard
    dbaccess/source/ui/uno/dbinteraction
    dbaccess/source/ui/uno/DBTypeWizDlg
    dbaccess/source/ui/uno/DBTypeWizDlgSetup
    dbaccess/source/ui/uno/TableFilterDlg
    dbaccess/source/ui/uno/textconnectionsettings_uno
    dbaccess/source/ui/uno/unoadmin
    dbaccess/source/ui/uno/unoDirectSql
    dbaccess/source/ui/uno/unosqlmessage
    dbaccess/source/ui/uno/UserSettingsDlg
)
# dbaccess/source/ui/dlg/adodatalinks (ifeq ($(OS),WNT)) is Windows-only and
# excluded.

librestudio_expand_cxx(DBU_SOURCES)
add_library(dbu SHARED ${DBU_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(dbu PRIVATE
        "${CMAKE_SOURCE_DIR}/dbaccess/inc/pch/precompiled_dbu.hxx")
    target_compile_definitions(dbu PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(dbu PRIVATE
    "${CMAKE_SOURCE_DIR}/dbaccess/inc"
    "${CMAKE_SOURCE_DIR}/dbaccess/source/inc"
    "${CMAKE_SOURCE_DIR}/dbaccess/source/ui/inc"
    "${DBACCESS_GENERATED_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    # odbcconfig.cxx pulls in include/connectivity/odbc.hxx, which expects
    # <odbc/sqlext.h> etc. from the bundled external/unixODBC headers (the
    # gb_Library_use_externals,odbc,odbc_headers external) -- the system
    # unixODBC package here has no headers subpackage installed, and upstream
    # only takes the system path on macOS (SYSTEM_ODBC_HEADERS), so the
    # bundled copy is what this port uses on Linux too.
    "${CMAKE_SOURCE_DIR}/external/unixODBC/inc")
add_dependencies(dbu
    officecfg_headers udkapi_normal_headers offapi_normal_headers
    dbaccess_sqlbison_header)

target_compile_definitions(dbu PRIVATE DBACCESS_DLLIMPLEMENTATION)

target_link_libraries(dbu PRIVATE
    comphelper cppu cppuhelper dba dbtools editeng fwk sal salhelper
    i18nlangtag sfx sot svl svt svxcore svx tk tl ucbhelper utl vcl
    Boost::boost)
# ifeq ($(COM),MSC): -DADO_SUPPRESS_MESSAGE -- Windows-only, not reproduced.
# ifeq ($(OS),WNT): ole32/oleaut32/uuid -- Windows-only, not reproduced.

librestudio_add_component(dbu
    COMPONENT "${CMAKE_SOURCE_DIR}/dbaccess/util/dbu.component"
    LIBRARY   dbu
    LAYER     OOO)

librestudio_set_library_output_name(dbu dbu OOO)

install(TARGETS dbu LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# dbaxml -- Library_dbaxml.mk
# ---------------------------------------------------------------------------

set(DBAXML_SOURCES
    dbaccess/source/filter/xml/dbloader2
    dbaccess/source/filter/xml/xmlAutoStyle
    dbaccess/source/filter/xml/xmlColumn
    dbaccess/source/filter/xml/xmlComponent
    dbaccess/source/filter/xml/xmlConnectionData
    dbaccess/source/filter/xml/xmlConnectionResource
    dbaccess/source/filter/xml/xmlDatabase
    dbaccess/source/filter/xml/xmlDatabaseDescription
    dbaccess/source/filter/xml/xmlDataSource
    dbaccess/source/filter/xml/xmlDataSourceInfo
    dbaccess/source/filter/xml/xmlDataSourceSetting
    dbaccess/source/filter/xml/xmlDataSourceSettings
    dbaccess/source/filter/xml/xmlDocuments
    dbaccess/source/filter/xml/xmlExport
    dbaccess/source/filter/xml/xmlFileBasedDatabase
    dbaccess/source/filter/xml/xmlfilter
    dbaccess/source/filter/xml/xmlHelper
    dbaccess/source/filter/xml/xmlHierarchyCollection
    dbaccess/source/filter/xml/xmlLogin
    dbaccess/source/filter/xml/xmlQuery
    dbaccess/source/filter/xml/xmlServerDatabase
    dbaccess/source/filter/xml/xmlStyleImport
    dbaccess/source/filter/xml/xmlTable
    dbaccess/source/filter/xml/xmlTableFilterList
    dbaccess/source/filter/xml/xmlTableFilterPattern
)

librestudio_expand_cxx(DBAXML_SOURCES)
add_library(dbaxml SHARED ${DBAXML_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(dbaxml PRIVATE
        "${CMAKE_SOURCE_DIR}/dbaccess/inc/pch/precompiled_dbaxml.hxx")
    target_compile_definitions(dbaxml PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(dbaxml PRIVATE
    "${CMAKE_SOURCE_DIR}/dbaccess/inc"
    "${CMAKE_SOURCE_DIR}/dbaccess/source/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(dbaxml officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(dbaxml PRIVATE
    comphelper cppu cppuhelper dba dbtools sal salhelper sax sfx svl tk tl
    utl vcl xo
    Boost::boost)

librestudio_add_component(dbaxml
    COMPONENT "${CMAKE_SOURCE_DIR}/dbaccess/source/filter/xml/dbaxml.component"
    LIBRARY   dbaxml
    LAYER     OOO)

librestudio_set_library_output_name(dbaxml dbaxml OOO)

install(TARGETS dbaxml LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# sdbt -- Library_sdbt.mk
# ---------------------------------------------------------------------------

set(SDBT_SOURCES
    dbaccess/source/sdbtools/connection/connectiontools
    dbaccess/source/sdbtools/connection/datasourcemetadata
    dbaccess/source/sdbtools/connection/objectnames
    dbaccess/source/sdbtools/connection/tablename
)

librestudio_expand_cxx(SDBT_SOURCES)
add_library(sdbt SHARED ${SDBT_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(sdbt PRIVATE
        "${CMAKE_SOURCE_DIR}/dbaccess/inc/pch/precompiled_sdbt.hxx")
    target_compile_definitions(sdbt PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(sdbt PRIVATE
    "${CMAKE_SOURCE_DIR}/dbaccess/inc"
    "${CMAKE_SOURCE_DIR}/dbaccess/source/inc"
    "${CMAKE_SOURCE_DIR}/dbaccess/source/sdbtools/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(sdbt officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(sdbt PRIVATE
    comphelper cppu cppuhelper dba dbtools sal salhelper tl utl
    Boost::boost)

librestudio_add_component(sdbt
    COMPONENT "${CMAKE_SOURCE_DIR}/dbaccess/util/sdbt.component"
    LIBRARY   sdbt
    LAYER     OOO)

librestudio_set_library_output_name(sdbt sdbt OOO)

install(TARGETS sdbt LIBRARY DESTINATION program)

# --- UIConfig_dbaccess ------------------------------------------------------
#
# dbaccess/UIConfig_dbaccess.mk: gb_UIConfig_UIConfig(dbaccess) -- 82
# uifiles, unconditional, single category (unlike most other UIConfig_* ports
# in this tree, this one has no menubar/popupmenu/statusbar/toolbar files at
# all -- re-checked directly, the .mk really only calls gb_UIConfig_add_uifiles)
# -- packaged into share/config/soffice.cfg/dbaccess/ui, same install()+
# build-tree-copy-loop+ALL-target pattern as writerperfect_uiconfig (also
# single-category). All files verified present on disk. CppunitTest_dbaccess_
# dialogs_test.mk's own use_uiconfigs(dbaccess_dialogs_test, dbaccess) needs a
# real build-time ALL target for this -- introduced here as "dbaccess_
# uiconfig" (this port's own <module>_uiconfig convention).
set(DBACCESS_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/admindialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/advancedsettingsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/appborderwindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/appdetailwindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/applycolpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/appswapwindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/authentificationpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/autocharsetpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/choosedatasourcedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/colcontrolbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/collectionviewdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/colwidthdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/connectionpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/copytablepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dbaseindexdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dbasepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dbtreelist.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dbwizconnectionpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dbwizmysqlintropage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dbwizmysqlnativepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dbwizspreadsheetpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dbwiztextpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/deleteallrowsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/designsavemodifieddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/detailwindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/directsqldialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/emptypage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/fielddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/fielddescpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/fielddescpanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/finalpagewizard.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/generalpagedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/generalpagewizard.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/generalspecialjdbcdetailspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/generatedvaluespage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/indexdesigndialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/jdbcconnectionpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/joindialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/jointablemenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/joinviewmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/keymenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/ldapconnectionpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/ldappage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/limitbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/mysqlnativepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/mysqlnativesettings.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/namematchingpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/odbcpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/parametersdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/password.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querycolmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/queryfilterdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/queryfuncmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querypropertiesdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/queryview.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/relationdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/rowheightdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/saveindexdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/savedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/savemodifieddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/specialjdbcconnectionpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/postgrespage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/specialsettingspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sortdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sqlexception.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tableborderwindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tabledesignrowmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tabledesignsavemodifieddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tablelistbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tablesfilterdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tablesfilterpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tablesjoindialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tabletitle.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/taskwindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/textconnectionsettings.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/textpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/titlewindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/typeselectpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/useradmindialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/useradminpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/userdetailspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/migrwarndlg.ui"
)

install(FILES ${DBACCESS_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/dbaccess/ui)

set(_dba_uiconfig_outputs "")
foreach (_dba_uiconfig_src IN LISTS DBACCESS_UICONFIG_UI_FILES)
    get_filename_component(_dba_uiconfig_name "${_dba_uiconfig_src}" NAME)
    set(_dba_uiconfig_dest
        "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/dbaccess/ui/${_dba_uiconfig_name}")
    add_custom_command(
        OUTPUT "${_dba_uiconfig_dest}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/dbaccess/ui"
        COMMAND ${CMAKE_COMMAND} -E copy "${_dba_uiconfig_src}" "${_dba_uiconfig_dest}"
        DEPENDS "${_dba_uiconfig_src}"
        COMMENT "Copying ${_dba_uiconfig_name} into build-tree soffice.cfg/dbaccess/ui")
    list(APPEND _dba_uiconfig_outputs "${_dba_uiconfig_dest}")
endforeach ()
unset(_dba_uiconfig_src)
unset(_dba_uiconfig_name)
unset(_dba_uiconfig_dest)

add_custom_target(dbaccess_uiconfig ALL DEPENDS ${_dba_uiconfig_outputs})
unset(_dba_uiconfig_outputs)

# NOT ported here: UIConfig_dbapp/UIConfig_dbbrowser/UIConfig_dbquery/
# UIConfig_dbrelation/UIConfig_dbtable/UIConfig_dbtdata -- Module_dbaccess.mk
# lists 7 UIConfig_*.mk targets total, but no CppunitTest this port is wiring
# needs any of the other 6 (only CppunitTest_dbaccess_dialogs_test's own
# use_uiconfigs(dbaccess) call does, hence only UIConfig_dbaccess above is
# ported in this pass) -- a real, separate packaging gap for dbaccess's own
# running UI, left honestly unaddressed rather than silently implied done.

# --- CppunitTests --------------------------------------------------------------
#
# dbaccess has 11 CppunitTest_*.mk files, all re-checked directly. 5 are
# wireable and wired to dbaccess/CppunitTest_deferred.cmake: dbaccess_
# dialogs_test (needs program_services_rdb), and dbaccess_dialog_save/
# empty_stdlib_save/nolib_save/hsqlschema_import (needed only use_
# configuration -- test/CMakeLists.txt's own LIBRESTUDIO_TEST_UNITTEST_DIR --
# and, until 2026-09-28, "unoxml/source/rdf/unordf", which needed external
# RDF libraries -- librdf/redland/raptor2/rasqal -- this port had never
# vendored or detected; all three are now vendored for real, see unoxml/
# CMakeLists.txt's own "--- Library_unordf ---" section and root
# CMakeLists.txt's own "--- raptor2/rasqal/redland ---" sections). The other
# 6 are NOT ported here, for two distinct, real, not-yet-attempted reasons:
#   - dbaccess_hsqldb_test and dbaccess_RowSetClones both use_components
#     "connectivity/source/drivers/hsqldb/hsqldb" and "connectivity/source/
#     drivers/jdbc/jdbc" -- both gated `ifneq ($(ENABLE_JAVA),)` in Module_
#     connectivity.mk. 2026-09-29: the components themselves (Library_
#     hsqldb/Library_jdbc) are now real, ported components (connectivity/
#     CMakeLists.txt's own dedicated blocks) -- the Java-UNO bridge itself
#     (bridges/CMakeLists.txt's own java_uno) was never actually the gap
#     either, it was already ported and build-verified earlier. What
#     remains blocking both tests here is purely LIBRESTUDIO_ENABLE_JAVA
#     being OFF in this build's own cache (a configuration choice left for
#     the user, not a missing-code gap) plus, for dbaccess_hsqldb_test
#     specifically, use_jars(sdbc_hsqldb) -- see the next bullet.
#     dbaccess_migration (re-checked against Module_dbaccess.mk's own gating)
#     is gated `ifeq ($(ENABLE_FIREBIRD_SDBC),TRUE)` AND `$(if $(ENABLE_JAVA),
#     ...)` -- double-blocked, same firebird_sdbc gap as the bullet below,
#     not primarily the Java bridge.
#   - dbaccess_CRMDatabase_test needs use_jars(sdbc_hsqldb) --
#     gb_CppunitTest_use_jars, a mechanism librestudio_add_cppunit_test()
#     has no parameter for yet (distinct from PACKAGES/COMPONENTS -- it
#     puts a jar on the test's own classpath). connectivity/CMakeLists.txt
#     does build a real "sdbc_hsqldb_jar" target via librestudio_add_uno_jar(),
#     but only under LIBRESTUDIO_ENABLE_JAVA (default OFF), so even a JARS
#     parameter would need that flag on to have anything to point at.
#   - dbaccess_embeddeddb_performancetest and dbaccess_firebird_test both
#     use_libraries "firebird_sdbc" directly -- connectivity's own Firebird
#     SDBC driver, gated `ifeq ($(ENABLE_FIREBIRD_SDBC),TRUE)` and not built
#     in this port (see connectivity/CMakeLists.txt's own header comment).
# None of the above forced through with an invented mechanism -- each a
# real, addressable, not-yet-attempted gap, left honestly unwired.
#
# CppunitTest_dbaccess_dialogs_test.mk (a gb_CppunitTest_CppunitScreenShot
# screenshot target), re-checked directly: 1 source (dbaccess/qa/unit/
# dbaccess-dialogs-test), use_sdk_api (twice, no-op second time), set_include
# (-Idbaccess/source/inc -Idbaccess/inc $$(INCLUDE)), use_libraries (basegfx
# comphelper cppu cppuhelper drawinglayer editeng i18nlangtag i18nutil
# msfilter oox sal salhelper sax sfx sot svl svt test tl tk ucbhelper unotest
# utl vcl xo -- "test" -> test_fixtures), use_external(boost_headers),
# use_ure + use_vcl_non_headless_with_windows -> 3 protector pairs +
# NONHEADLESS, use_rdb(services) -> DEPENDS program_services_rdb (no
# COMPONENTS), use_configuration -> CONFIGURATION, use_uiconfigs(dbaccess)
# -> UICONFIG dbaccess (this file's own new "dbaccess_uiconfig" ALL target,
# ported alongside this wiring).
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/dbaccess/CppunitTest_deferred.cmake")

endif (ENABLE_DBCONNECTIVITY)
