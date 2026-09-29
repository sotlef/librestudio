# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sc (LibreOffice Calc) -- Module_sc.mk lists five libraries plus one
# SCRIPTING-gated library:
#   Library_sc      (sc)      -- the Calc core + UI + UNO API implementation
#   Library_scd     (scd)     -- the tiny format-detection service (Excel/
#                                 Calc file-type sniffing), used by the
#                                 document-loader dialog. No dependency on
#                                 Library_sc.
#   Library_scfilt  (scfilt)  -- import/export filters (Excel binary/OOXML,
#                                 Lotus, dBase/dif/rtf/html, orcus). Depends
#                                 on sc.
#   Library_scui    (scui)    -- dialog implementations (ScAbstractDialogFactory
#                                 target). Depends on sc.
#   Library_solver  (solver)  -- Solver/SwarmSolver UNO components. Depends
#                                 on sc.
#   Library_vbaobj  (vbaobj)  -- the Calc VBA object model, gated in
#                                 Module_sc.mk behind
#                                 `ifneq ($(filter SCRIPTING,$(BUILD_TYPE)),)`.
#                                 Depends on sc and vbahelper (vbahelper is
#                                 already ported -- vbahelper/CMakeLists.txt --
#                                 with all of ITS OWN dependencies satisfied).
#                                 Now gated by `if (ENABLE_SCRIPTING)` below,
#                                 matching upstream; ENABLE_SCRIPTING defaults
#                                 ON (cmake/LibreStudioConfigHeaders.cmake), so
#                                 vbaobj is still built by default.
#
# Status (2026-09-26): all five targets below are ported. One exclusion
# remains, plus two gates now flipped ON:
#  * OPENCL (sc's 10-file sc/source/core/opencl/* block, and the clew/opencl
#    use_externals/use_libraries entries that go with it) -- gb_Helper_optional
#    guarded. Formerly excluded ("no clew port"); as of this wave, opencl/clew
#    are real targets (opencl/CMakeLists.txt, external/clew/CMakeLists.txt)
#    and ENABLE_OPENCL defaults ON, so this block is now included -- see the
#    `if (ENABLE_OPENCL)` guards around SC_SOURCES and target_link_libraries
#    below, both real, toggleable conditionals (not hardcoded).
#  * CoinMP (solver's CoinMPSolver/CoinMPModel/SlpSolver, and the coinmp
#    external) -- gb_Helper_optional(ENABLE_COINMP, ...) guarded. 2026-09-28:
#    now real, see "solver -- Library_solver.mk" below -- ENABLE_COINMP
#    defaults ON (matching configure.ac's own real default on a Linux
#    desktop build), system coinmp+coinutils preferred, external/coinmp/'s
#    download.lst tarball (a genuine autotools `./configure && make` over
#    the whole COIN-OR tree, not a file-list compile) built otherwise -- no
#    system coinmp package exists on this platform, so that is always the
#    path taken here.
#  * vbaobj's Module_sc.mk-level SCRIPTING gate -- this port does not track
#    the SCRIPTING build-type switch, so vbaobj is built unconditionally (its
#    own Library_vbaobj.mk carries no internal gate once reached).
# Also excluded per Library_sc.mk itself: the ENABLE_FORMULA_LOGGER-gated
# formulalogger.cxx and the ENABLE_DBGUTIL-gated gridwin_dbgutil.cxx, neither
# flag being enabled here.
#
# Update (2026-09-26): vbaobj is now wrapped in `if (ENABLE_SCRIPTING)` below,
# matching Module_sc.mk's own gate. ENABLE_SCRIPTING defaults ON, so this is a
# behavior-preserving change unless someone passes -DENABLE_SCRIPTING=OFF.
#
# External packages consumed (all resolved via directory-local
# pkg_check_modules(... IMPORTED_TARGET ...), per this port's convention):
#  * md4c (sc's own use_externals, not just sw's) -- LIBRESTUDIO_MD4C_TARGET/
#    LIBRESTUDIO_MD4C_HTML_TARGET (resolved right below via
#    librestudio_pkgconfig(), cmake/LibreStudioTarballs.cmake).
#  * orcus / orcus-spreadsheet-model, SYSTEM_LIBORCUS path (liborcus-devel is
#    installed) -- `liborcus-0.18` and `liborcus-spreadsheet-model-0.18`
#    (upstream API-version pc-file stamps; actual installed version is
#    0.19.2). sc itself only needs the spreadsheet-model half for
#    orcusinterface.hxx; scfilt's sc/source/filter/orcus/* needs both.
#  * ICU (icu_headers+icui18n+icuuc) -> ICU::uc + ICU::i18n (global targets
#    from the root CMakeLists.txt).
#  * libxml2 -> LibXml2::LibXml2; mdds_headers -> ${MDDS_INCLUDE_DIR}; frozen
#    -> ${FROZEN_INCLUDE_DIR} (both already-global root variables).
#  * scfilt's sc/source/filter/oox/* (OOXML import/export) needs the full oox
#    generated token/namespace/property headers tree -- ${OOX_GENERATED_DIR}
#    / oox_generated, exported by oox/CMakeLists.txt.
#
# sc itself gets its own SdiTarget (sc_sdi, from sc/sdi/scslots.sdi +
# sc/sdi/scalc.sdi) -- scui also needs its generated header on its include
# path (Library_scui.mk's own gb_Library_set_include lists
# $(WORKDIR)/SdiTarget/sc/sdi); scfilt/solver/vbaobj do not (their .mk
# set_include calls do not reference it).

# ---------------------------------------------------------------------------
# sc -- Library_sc.mk
# ---------------------------------------------------------------------------

librestudio_add_sdi_target(sc_sdi
    SDI        "${CMAKE_SOURCE_DIR}/sc/sdi/scslots.sdi"
    EXPORTS    "${CMAKE_SOURCE_DIR}/sc/sdi/scalc.sdi"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/SdiTarget/sc/sdi"
    INCLUDES   "${CMAKE_SOURCE_DIR}/sc/inc" "${CMAKE_SOURCE_DIR}/svx/sdi" "${CMAKE_SOURCE_DIR}/sfx2/sdi")

# md4c/md4c-html: resolved right here (this module is one of the two real
# consumers, alongside sw) via librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake) -- both are needed: configure.ac's own
# MD4C_LIBS (lines 10156-10161) is "-lmd4c -lmd4c-html" combined, not
# "-lmd4c" alone -- md_html() (the HTML-rendering entry point,
# sc/source/ui/docshell/impex.cxx) lives in the separate libmd4c-html.so/
# md4c-html.pc, not libmd4c.so/md4c.pc. DRIVER omitted for MD4C (MODULES
# md4c is a single clean element, auto-detected); MD4C_HTML needs it
# explicit (MODULES md4c-html has a hyphen, and shares md4c's own driver
# file, not a separate external/md4c_html/).
librestudio_pkgconfig(MODULES md4c)
librestudio_pkgconfig(
    NAME MD4C_HTML
    MODULES md4c-html
    DRIVER md4c)

# orcus/orcus-spreadsheet-model: sc/scfilt's orcus_autofilter.hxx etc. need
# orcus's spreadsheet-model API in its 0.21-version shape (import_interface_
# auto_filter.hpp, format_t::xml/json, single-arg orcus::detect()), which
# this system's real installed orcus (0.19.2) does not have -- not a
# packaging bug, a genuine version gap (this fork's own download.lst pins
# liborcus-0.21.0.tar.xz). MODULES names BOTH liborcus-0.21 and liborcus-
# spreadsheet-model-0.21 together (matching orcus's own real, API-version-
# suffixed pkg-config module names): the combined probe decides found-vs-
# vendor as ONE atomic decision (a system without spreadsheet-model support
# falls through to vendoring both) -- but the resulting PkgConfig::ORCUS
# target from that combined call is discarded below in favour of two
# genuinely separate imported targets, one per module, since scqahelper
# (further down in this file) links only the base orcus library, not
# spreadsheet-model. DRIVER explicit ("orcus"): more than one MODULES
# element, so no auto-detection candidate exists at all.
# LIBRESTUDIO_FORCE_VENDOR_ORCUS is declared once in cmake/
# LibreStudioTarballs.cmake, alongside its 4 sibling force-vendor options --
# not re-declared here.
librestudio_pkgconfig(
    NAME ORCUS
    MODULES liborcus-0.21 liborcus-spreadsheet-model-0.21
    VERSION 0.21.0
    FORCE_VENDOR_VAR LIBRESTUDIO_FORCE_VENDOR_ORCUS
    DRIVER orcus
    FOUND_VAR _orcus_found)
if (_orcus_found)
    message(STATUS "orcus: using system package (>= 0.21.0)")
    pkg_check_modules(ORCUS_SYS REQUIRED IMPORTED_TARGET GLOBAL "liborcus-0.21>=0.21.0")
    pkg_check_modules(ORCUS_SPREADSHEET_SYS REQUIRED IMPORTED_TARGET GLOBAL
        "liborcus-spreadsheet-model-0.21>=0.21.0")
    set(LIBRESTUDIO_ORCUS_TARGET PkgConfig::ORCUS_SYS CACHE INTERNAL "" FORCE)
    set(LIBRESTUDIO_ORCUS_SPREADSHEET_TARGET PkgConfig::ORCUS_SPREADSHEET_SYS CACHE INTERNAL "" FORCE)
endif ()
unset(_orcus_found)

set(SC_SOURCES
    sc/source/core/data/attarray
    sc/source/core/data/attrib
    sc/source/core/data/autonamecache
    sc/source/core/data/bcaslot
    sc/source/core/data/broadcast
    sc/source/core/data/bigrange
    sc/source/core/data/celltextattr
    sc/source/core/data/cellvalue
    sc/source/core/data/cellvalues
    sc/source/core/data/clipcontext
    sc/source/core/data/clipparam
    sc/source/core/data/colcontainer
    sc/source/core/data/column
    sc/source/core/data/column2
    sc/source/core/data/column3
    sc/source/core/data/column4
    sc/source/core/data/columniterator
    sc/source/core/data/columnset
    sc/source/core/data/columnspanset
    sc/source/core/data/compressedarray
    sc/source/core/data/colorscale
    sc/source/core/data/conditio
    sc/source/core/data/dbdocutl
    sc/source/core/data/dociter
    sc/source/core/data/docparam
    sc/source/core/data/docpool
    sc/source/core/data/documen2
    sc/source/core/data/documen3
    sc/source/core/data/documen4
    sc/source/core/data/documen5
    sc/source/core/data/documen6
    sc/source/core/data/documen7
    sc/source/core/data/documen8
    sc/source/core/data/documen9
    sc/source/core/data/document
    sc/source/core/data/document10
    sc/source/core/data/documentimport
    sc/source/core/data/documentstreamaccess
    sc/source/core/data/dpdimsave
    sc/source/core/data/dpfilteredcache
    sc/source/core/data/dpglobal
    sc/source/core/data/dpgroup
    sc/source/core/data/dpitemdata
    sc/source/core/data/dpnumgroupinfo
    sc/source/core/data/dpobject
    sc/source/core/data/dpoutput
    sc/source/core/data/dpoutputgeometry
    sc/source/core/data/dpresfilter
    sc/source/core/data/dpsave
    sc/source/core/data/dpsdbtab
    sc/source/core/data/dpshttab
    sc/source/core/data/dptabdat
    sc/source/core/data/dpcache
    sc/source/core/data/dptabres
    sc/source/core/data/dptabsrc
    sc/source/core/data/dputil
    sc/source/core/data/drawpage
    sc/source/core/data/drwlayer
    sc/source/core/data/edittextiterator
    sc/source/core/data/fillinfo
    sc/source/core/data/FilterData
    sc/source/core/data/formulacell
    sc/source/core/data/formulaiter
    sc/source/core/data/funcdesc
    sc/source/core/data/global
    sc/source/core/data/global2
    sc/source/core/data/globalx
    sc/source/core/data/grouptokenconverter
    sc/source/core/data/listenercontext
    sc/source/core/data/markarr
    sc/source/core/data/markdata
    sc/source/core/data/markmulti
    sc/source/core/data/mtvcellfunc
    sc/source/core/data/mtvelements
    sc/source/core/data/olinetab
    sc/source/core/data/pagepar
    sc/source/core/data/patattr
    sc/source/core/data/pivot2
    sc/source/core/data/PivotTableFormatOutput
    sc/source/core/data/poolcach
    sc/source/core/data/poolhelp
    sc/source/core/data/postit
    sc/source/core/data/queryevaluator
    sc/source/core/data/queryiter
    sc/source/core/data/refupdatecontext
    sc/source/core/data/rowheightcontext
    sc/source/core/data/segmenttree
    sc/source/core/data/sheetevents
    sc/source/core/data/SheetView
    sc/source/core/data/SheetViewManager
    sc/source/core/data/simpleformulacalc
    sc/source/core/data/SolverSettings
    sc/source/core/data/sortparam
    sc/source/core/data/stlpool
    sc/source/core/data/stlsheet
    sc/source/core/data/subtotalparam
    sc/source/core/data/tabbgcolor
    sc/source/core/data/TableContentCopier
    sc/source/core/data/table1
    sc/source/core/data/table2
    sc/source/core/data/table3
    sc/source/core/data/table4
    sc/source/core/data/table5
    sc/source/core/data/table6
    sc/source/core/data/table7
    sc/source/core/data/TableStyleGenerator
    sc/source/core/data/tablestyle
    sc/source/core/data/tabprotection
    sc/source/core/data/types
    sc/source/core/data/userdat
    sc/source/core/data/validat
    sc/source/core/data/pivot/PivotTableResultTraverser
    sc/source/core/data/pivot/StyleOutput
    sc/source/core/tool/addincfg
    sc/source/core/tool/addincol
    sc/source/core/tool/addinhelpid
    sc/source/core/tool/addinlis
    sc/source/core/tool/address
    sc/source/core/tool/adiasync
    sc/source/core/tool/appoptio
    sc/source/core/tool/arraysumSSE2
    sc/source/core/tool/autoform
    sc/source/core/tool/calcconfig
    sc/source/core/tool/callable
    sc/source/core/tool/callform
    sc/source/core/tool/cellform
    sc/source/core/tool/cellkeytranslator
    sc/source/core/tool/chartarr
    sc/source/core/tool/charthelper
    sc/source/core/tool/chartlis
    sc/source/core/tool/chartlock
    sc/source/core/tool/chartpos
    sc/source/core/tool/chgtrack
    sc/source/core/tool/chgviset
    sc/source/core/tool/compare
    sc/source/core/tool/compiler
    sc/source/core/tool/consoli
    sc/source/core/tool/dbdata
    sc/source/core/tool/ddelink
    sc/source/core/tool/defaultsoptions
    sc/source/core/tool/detdata
    sc/source/core/tool/detfunc
    sc/source/core/tool/docoptio
    sc/source/core/tool/doubleref
    sc/source/core/tool/editdataarray
    sc/source/core/tool/editutil
    sc/source/core/tool/formulagroup
    sc/source/core/tool/formulaopt
    sc/source/core/tool/formulaparserpool
    sc/source/core/tool/formularesult
    sc/source/core/tool/grouparealistener
    sc/source/core/tool/hints
    sc/source/core/tool/inputopt
    sc/source/core/tool/interpr1
    sc/source/core/tool/interpr2
    sc/source/core/tool/interpr3
    sc/source/core/tool/interpr4
    sc/source/core/tool/interpr5
    sc/source/core/tool/interpr6
    sc/source/core/tool/interpr7
    sc/source/core/tool/interpr8
    sc/source/core/tool/interpretercontext
    sc/source/core/tool/jumpmatrix
    sc/source/core/tool/listenerquery
    sc/source/core/tool/lookupcache
    sc/source/core/tool/math
    sc/source/core/tool/matrixoperators
    sc/source/core/tool/navicfg
    sc/source/core/tool/numformat
    sc/source/core/tool/odffmap
    sc/source/core/tool/optutil
    sc/source/core/tool/orcusxml
    sc/source/core/tool/parclass
    sc/source/core/tool/printopt
    sc/source/core/tool/prnsave
    sc/source/core/tool/progress
    sc/source/core/tool/queryentry
    sc/source/core/tool/queryparam
    sc/source/core/tool/rangecache
    sc/source/core/tool/rangelst
    sc/source/core/tool/rangenam
    sc/source/core/tool/rangeseq
    sc/source/core/tool/rangeutl
    sc/source/core/tool/rechead
    sc/source/core/tool/recursionhelper
    sc/source/core/tool/refdata
    sc/source/core/tool/reffind
    sc/source/core/tool/refhint
    sc/source/core/tool/refreshtimer
    sc/source/core/tool/reftokenhelper
    sc/source/core/tool/refupdat
    sc/source/core/tool/scmatrix
    sc/source/core/tool/scopetools
    sc/source/core/tool/sharedformula
    sc/source/core/tool/sharedstringpoolpurge
    sc/source/core/tool/stringutil
    sc/source/core/tool/stylehelper
    sc/source/core/tool/subtotal
    sc/source/core/tool/token
    sc/source/core/tool/tokenstringcontext
    sc/source/core/tool/typedstrdata
    sc/source/core/tool/unitconv
    sc/source/core/tool/userlist
    sc/source/core/tool/viewopti
    sc/source/core/tool/webservicelink
    sc/source/filter/xml/datastreamimport
    sc/source/filter/xml/SparklineGroupsExport
    sc/source/filter/xml/SparklineGroupsImportContext
    sc/source/filter/xml/XMLCalculationSettingsContext
    sc/source/filter/xml/XMLCellRangeSourceContext
    sc/source/filter/xml/XMLChangeTrackingExportHelper
    sc/source/filter/xml/XMLChangeTrackingImportHelper
    sc/source/filter/xml/XMLCodeNameProvider
    sc/source/filter/xml/XMLColumnRowGroupExport
    sc/source/filter/xml/XMLConsolidationContext
    sc/source/filter/xml/XMLConverter
    sc/source/filter/xml/XMLDDELinksContext
    sc/source/filter/xml/XMLDetectiveContext
    sc/source/filter/xml/XMLEmptyContext
    sc/source/filter/xml/XMLExportDDELinks
    sc/source/filter/xml/XMLExportDataPilot
    sc/source/filter/xml/XMLExportDatabaseRanges
    sc/source/filter/xml/XMLExportIterator
    sc/source/filter/xml/XMLExportSharedData
    sc/source/filter/xml/XMLStylesExportHelper
    sc/source/filter/xml/XMLStylesImportHelper
    sc/source/filter/xml/XMLTableHeaderFooterContext
    sc/source/filter/xml/XMLTableMasterPageExport
    sc/source/filter/xml/XMLTableShapeImportHelper
    sc/source/filter/xml/XMLTableShapeResizer
    sc/source/filter/xml/XMLTableShapesContext
    sc/source/filter/xml/XMLTableSourceContext
    sc/source/filter/xml/XMLTrackedChangesContext
    sc/source/filter/xml/cachedattraccess
    sc/source/filter/xml/celltextparacontext
    sc/source/filter/xml/editattributemap
    sc/source/filter/xml/importcontext
    sc/source/filter/xml/pivotsource
    sc/source/filter/xml/sheetdata
    sc/source/filter/xml/xmlannoi
    sc/source/filter/xml/xmlbodyi
    sc/source/filter/xml/xmlcelli
    sc/source/filter/xml/xmlcoli
    sc/source/filter/xml/xmlcondformat
    sc/source/filter/xml/xmlconti
    sc/source/filter/xml/xmlcvali
    sc/source/filter/xml/xmldpimp
    sc/source/filter/xml/xmldrani
    sc/source/filter/xml/xmlexprt
    sc/source/filter/xml/xmlexternaltabi
    sc/source/filter/xml/xmlfilti
    sc/source/filter/xml/xmlfonte
    sc/source/filter/xml/xmlimprt
    sc/source/filter/xml/xmllabri
    sc/source/filter/xml/xmlmappingi
    sc/source/filter/xml/xmlnexpi
    sc/source/filter/xml/xmlrowi
    sc/source/filter/xml/xmlsceni
    sc/source/filter/xml/xmlsorti
    sc/source/filter/xml/xmlstyle
    sc/source/filter/xml/xmlstyli
    sc/source/filter/xml/xmlsubti
    sc/source/filter/xml/xmltabi
    sc/source/filter/xml/xmltransformationi
    sc/source/filter/xml/xmlwrap
    sc/source/filter/importfilterdata
    sc/source/ui/Accessibility/AccessibilityHints
    sc/source/ui/Accessibility/AccessibleCell
    sc/source/ui/Accessibility/AccessibleCellBase
    sc/source/ui/Accessibility/AccessibleContextBase
    sc/source/ui/Accessibility/AccessibleCsvControl
    sc/source/ui/Accessibility/AccessibleDocument
    sc/source/ui/Accessibility/AccessibleDocumentBase
    sc/source/ui/Accessibility/AccessibleDocumentPagePreview
    sc/source/ui/Accessibility/AccessibleEditObject
    sc/source/ui/Accessibility/AccessiblePageHeader
    sc/source/ui/Accessibility/AccessiblePageHeaderArea
    sc/source/ui/Accessibility/AccessiblePreviewCell
    sc/source/ui/Accessibility/AccessiblePreviewHeaderCell
    sc/source/ui/Accessibility/AccessiblePreviewTable
    sc/source/ui/Accessibility/AccessibleSpreadsheet
    sc/source/ui/Accessibility/AccessibleTableBase
    sc/source/ui/Accessibility/AccessibleText
    sc/source/ui/Accessibility/DrawModelBroadcaster
    sc/source/ui/app/acctrl
    sc/source/ui/app/client
    sc/source/ui/app/drwtrans
    sc/source/ui/app/inputhdl
    sc/source/ui/app/inputwin
    sc/source/ui/app/lnktrans
    sc/source/ui/app/msgpool
    sc/source/ui/app/rfindlst
    sc/source/ui/app/scdll
    sc/source/ui/app/scmod
    sc/source/ui/app/seltrans
    sc/source/ui/app/transobj
    sc/source/ui/app/typemap
    sc/source/ui/app/uiitems
    sc/source/ui/attrdlg/scabstdlg
    sc/source/ui/cctrl/cbnumberformat
    sc/source/ui/cctrl/cbuttonw
    sc/source/ui/cctrl/checklistmenu
    sc/source/ui/cctrl/dpcontrol
    sc/source/ui/cctrl/tbzoomsliderctrl
    sc/source/ui/cctrl/SheetViewBox
    sc/source/ui/cctrl/SheetViewControl
    sc/source/ui/cctrl/TableStylesBox
    sc/source/ui/cctrl/TableStylesControl
    sc/source/ui/condformat/condformatdlg
    sc/source/ui/condformat/condformatdlgentry
    sc/source/ui/condformat/condformatdlgdata
    sc/source/ui/condformat/condformateasydlg
    sc/source/ui/condformat/condformathelper
    sc/source/ui/condformat/colorformat
    sc/source/ui/dataprovider/csvdataprovider
    sc/source/ui/dataprovider/dataprovider
    sc/source/ui/dataprovider/datatransformation
    sc/source/ui/dataprovider/htmldataprovider
    sc/source/ui/dataprovider/xmldataprovider
    sc/source/ui/dataprovider/sqldataprovider
    sc/source/ui/dbgui/asciiopt
    sc/source/ui/dbgui/consdlg
    sc/source/ui/dbgui/csvcontrol
    sc/source/ui/dbgui/csvgrid
    sc/source/ui/dbgui/csvruler
    sc/source/ui/dbgui/csvsplits
    sc/source/ui/dbgui/csvtablebox
    sc/source/ui/dbgui/dbnamdlg
    sc/source/ui/dbgui/dbtabledlg
    sc/source/ui/dbgui/filtdlg
    sc/source/ui/dbgui/foptmgr
    sc/source/ui/dbgui/imoptdlg
    sc/source/ui/dbgui/PivotCalcFieldDialog
    sc/source/ui/dbgui/PivotLayoutDialog
    sc/source/ui/dbgui/PivotLayoutTreeListBase
    sc/source/ui/dbgui/PivotLayoutTreeListData
    sc/source/ui/dbgui/PivotLayoutTreeListLabel
    sc/source/ui/dbgui/PivotLayoutTreeList
    sc/source/ui/dbgui/sfiltdlg
    sc/source/ui/dbgui/validate
    sc/source/ui/dialogs/searchresults
    sc/source/ui/dialogs/SelectSheetViewDialog
    sc/source/ui/dialogs/SparklineDialog
    sc/source/ui/dialogs/SparklineDataRangeDialog
    sc/source/ui/docshell/arealink
    sc/source/ui/docshell/autostyl
    sc/source/ui/docshell/datastream
    sc/source/ui/docshell/dbdocfun
    sc/source/ui/docshell/dbdocimp
    sc/source/ui/docshell/docfunc
    sc/source/ui/docshell/docfuncutil
    sc/source/ui/docshell/docsh
    sc/source/ui/docshell/docsh2
    sc/source/ui/docshell/docsh3
    sc/source/ui/docshell/docsh4
    sc/source/ui/docshell/docsh5
    sc/source/ui/docshell/docsh6
    sc/source/ui/docshell/docsh8
    sc/source/ui/docshell/documentlinkmgr
    sc/source/ui/docshell/DocumentModelAccessor
    sc/source/ui/docshell/editable
    sc/source/ui/docshell/externalrefmgr
    sc/source/ui/docshell/impex
    sc/source/ui/docshell/macromgr
    sc/source/ui/docshell/olinefun
    sc/source/ui/docshell/pagedata
    sc/source/ui/docshell/pntlock
    sc/source/ui/docshell/servobj
    sc/source/ui/docshell/SheetViewOperationsTester
    sc/source/ui/docshell/sizedev
    sc/source/ui/docshell/tablink
    sc/source/ui/drawfunc/chartsh
    sc/source/ui/drawfunc/drawsh
    sc/source/ui/drawfunc/drawsh2
    sc/source/ui/drawfunc/drawsh4
    sc/source/ui/drawfunc/drawsh5
    sc/source/ui/drawfunc/drformsh
    sc/source/ui/drawfunc/drtxtob
    sc/source/ui/drawfunc/drtxtob1
    sc/source/ui/drawfunc/drtxtob2
    sc/source/ui/drawfunc/fuconarc
    sc/source/ui/drawfunc/fuconcustomshape
    sc/source/ui/drawfunc/fuconpol
    sc/source/ui/drawfunc/fuconrec
    sc/source/ui/drawfunc/fuconstr
    sc/source/ui/drawfunc/fuconuno
    sc/source/ui/drawfunc/fudraw
    sc/source/ui/drawfunc/fuins1
    sc/source/ui/drawfunc/fuins2
    sc/source/ui/drawfunc/fupoor
    sc/source/ui/drawfunc/fusel
    sc/source/ui/drawfunc/fusel2
    sc/source/ui/drawfunc/futext
    sc/source/ui/drawfunc/futext2
    sc/source/ui/drawfunc/futext3
    sc/source/ui/drawfunc/graphsh
    sc/source/ui/drawfunc/mediash
    sc/source/ui/drawfunc/oleobjsh
    sc/source/ui/formdlg/dwfunctr
    sc/source/ui/formdlg/formdata
    sc/source/ui/formdlg/formula
    sc/source/ui/miscdlgs/acredlin
    sc/source/ui/miscdlgs/anyrefdg
    sc/source/ui/miscdlgs/conflictsdlg
    sc/source/ui/miscdlgs/crnrdlg
    sc/source/ui/miscdlgs/datastreamdlg
    sc/source/ui/miscdlgs/dataproviderdlg
    sc/source/ui/miscdlgs/datatableview
    sc/source/ui/miscdlgs/highred
    sc/source/ui/miscdlgs/mergecellsdialog
    sc/source/ui/miscdlgs/optsolver
    sc/source/ui/miscdlgs/protectiondlg
    sc/source/ui/miscdlgs/duplicaterecordsdlg
    sc/source/ui/miscdlgs/redcom
    sc/source/ui/miscdlgs/retypepassdlg
    sc/source/ui/miscdlgs/sharedocdlg
    sc/source/ui/miscdlgs/simpref
    sc/source/ui/miscdlgs/solveroptions
    sc/source/ui/miscdlgs/solverutil
    sc/source/ui/miscdlgs/solvrdlg
    sc/source/ui/miscdlgs/tabopdlg
    sc/source/ui/miscdlgs/warnbox
    sc/source/ui/namedlg/namedefdlg
    sc/source/ui/namedlg/namedlg
    sc/source/ui/namedlg/namemgrtable
    sc/source/ui/navipi/content
    sc/source/ui/navipi/navcitem
    sc/source/ui/navipi/navipi
    sc/source/ui/navipi/scenwnd
    sc/source/ui/operation/ApplyAttributesOperation
    sc/source/ui/operation/AutoFormatOperation
    sc/source/ui/operation/ChangeIndentOperation
    sc/source/ui/operation/ChangeSparklineGroupAttributesOperation
    sc/source/ui/operation/ChangeSparklineOperation
    sc/source/ui/operation/ClearItemsOperation
    sc/source/ui/operation/ConvertFormulaToValueOperation
    sc/source/ui/operation/CreatePivotTableOperation
    sc/source/ui/operation/ReplacePivotTableOperation
    sc/source/ui/operation/DeleteCellOperation
    sc/source/ui/operation/DeleteCellsOperation
    sc/source/ui/operation/DeleteContentOperation
    sc/source/ui/operation/DeleteSparklineGroupOperation
    sc/source/ui/operation/DeleteSparklineOperation
    sc/source/ui/operation/EnterMatrixOperation
    sc/source/ui/operation/FillAutoOperation
    sc/source/ui/operation/FillSeriesOperation
    sc/source/ui/operation/FillSimpleOperation
    sc/source/ui/operation/GroupSparklinesOperation
    sc/source/ui/operation/InsertCellsOperation
    sc/source/ui/operation/InsertNameListOperation
    sc/source/ui/operation/InsertSheetViewOperation
    sc/source/ui/operation/InsertSparklinesOperation
    sc/source/ui/operation/InsertThreadedCommentOperation
    sc/source/ui/operation/MergeCellsOperation
    sc/source/ui/operation/MoveBlockOperation
    sc/source/ui/operation/MultipleOpsOperation
    sc/source/ui/operation/Operation
    sc/source/ui/operation/OperationType
    sc/source/ui/operation/PivotTableOperation
    sc/source/ui/operation/QueryOperation
    sc/source/ui/operation/RemovePivotTableOperation
    sc/source/ui/operation/ReplaceNoteTextOperation
    sc/source/ui/operation/SetEditTextOperation
    sc/source/ui/operation/SetNoteTextOperation
    sc/source/ui/operation/SetFormulaOperation
    sc/source/ui/operation/SetNormalStringOperation
    sc/source/ui/operation/SetStringOperation
    sc/source/ui/operation/SetValueOperation
    sc/source/ui/operation/SubTotalsOperation
    sc/source/ui/operation/SortOperation
    sc/source/ui/operation/TransliterateTextOperation
    sc/source/ui/operation/UngroupSparklinesOperation
    sc/source/ui/operation/UpdatePivotTableOperation
    sc/source/ui/pagedlg/areasdlg
    sc/source/ui/pagedlg/tphfedit
    sc/source/ui/sidebar/AlignmentPropertyPanel
    sc/source/ui/sidebar/CellLineStyleControl
    sc/source/ui/sidebar/CellAppearancePropertyPanel
    sc/source/ui/sidebar/CellBorderStyleControl
    sc/source/ui/sidebar/DatabasePropertyPanel
    sc/source/ui/sidebar/NumberFormatControl
    sc/source/ui/sidebar/NumberFormatPropertyPanel
    sc/source/ui/sidebar/ScPanelFactory
    sc/source/ui/sparklines/SparklineAttributes
    sc/source/ui/sparklines/SparklineData
    sc/source/ui/sparklines/SparklineGroup
    sc/source/ui/sparklines/SparklineList
    sc/source/ui/StatisticsDialogs/AnalysisOfVarianceDialog
    sc/source/ui/StatisticsDialogs/CorrelationDialog
    sc/source/ui/StatisticsDialogs/CovarianceDialog
    sc/source/ui/StatisticsDialogs/ChiSquareTestDialog
    sc/source/ui/StatisticsDialogs/DescriptiveStatisticsDialog
    sc/source/ui/StatisticsDialogs/ExponentialSmoothingDialog
    sc/source/ui/StatisticsDialogs/FourierAnalysisDialog
    sc/source/ui/StatisticsDialogs/FTestDialog
    sc/source/ui/StatisticsDialogs/MatrixComparisonGenerator
    sc/source/ui/StatisticsDialogs/MovingAverageDialog
    sc/source/ui/StatisticsDialogs/RandomNumberGeneratorDialog
    sc/source/ui/StatisticsDialogs/RegressionDialog
    sc/source/ui/StatisticsDialogs/SamplingDialog
    sc/source/ui/StatisticsDialogs/StatisticsInputOutputDialog
    sc/source/ui/StatisticsDialogs/StatisticsTwoVariableDialog
    sc/source/ui/StatisticsDialogs/TableFillingAndNavigationTools
    sc/source/ui/StatisticsDialogs/TTestDialog
    sc/source/ui/StatisticsDialogs/ZTestDialog
    sc/source/ui/theme/ThemeColorChanger
    sc/source/ui/uitest/uiobject
    sc/source/ui/undo/areasave
    sc/source/ui/undo/refundo
    sc/source/ui/undo/target
    sc/source/ui/undo/undobase
    sc/source/ui/undo/undoblk
    sc/source/ui/undo/undoblk2
    sc/source/ui/undo/undoblk3
    sc/source/ui/undo/undocell
    sc/source/ui/undo/undocell2
    sc/source/ui/undo/undoconvert
    sc/source/ui/undo/undodat
    sc/source/ui/undo/undodraw
    sc/source/ui/undo/undoolk
    sc/source/ui/undo/undorangename
    sc/source/ui/undo/undosort
    sc/source/ui/undo/UndoSheetViewSortData
    sc/source/ui/undo/undostyl
    sc/source/ui/undo/undotab
    sc/source/ui/undo/undoutil
    sc/source/ui/undo/UndoInsertSparkline
    sc/source/ui/undo/UndoEditSparklineGroup
    sc/source/ui/undo/UndoDeleteSparkline
    sc/source/ui/undo/UndoDeleteSparklineGroup
    sc/source/ui/undo/UndoUngroupSparklines
    sc/source/ui/undo/UndoGroupSparklines
    sc/source/ui/undo/UndoEditSparkline
    sc/source/ui/undo/UndoInsertSheetView
    sc/source/ui/undo/UndoSpillPivotTable
    sc/source/ui/undo/UndoThemeChange
    sc/source/ui/unoobj/ChartRangeSelectionListener
    sc/source/ui/unoobj/addruno
    sc/source/ui/unoobj/afmtuno
    sc/source/ui/unoobj/appluno
    sc/source/ui/unoobj/celllistsource
    sc/source/ui/unoobj/cellsuno
    sc/source/ui/unoobj/cellvaluebinding
    sc/source/ui/unoobj/chart2uno
    sc/source/ui/unoobj/chartuno
    sc/source/ui/unoobj/condformatuno
    sc/source/ui/unoobj/confuno
    sc/source/ui/unoobj/convuno
    sc/source/ui/unoobj/cursuno
    sc/source/ui/unoobj/dapiuno
    sc/source/ui/unoobj/datauno
    sc/source/ui/unoobj/defltuno
    sc/source/ui/unoobj/dispuno
    sc/source/ui/unoobj/docuno
    sc/source/ui/unoobj/drdefuno
    sc/source/ui/unoobj/editsrc
    sc/source/ui/unoobj/eventuno
    sc/source/ui/unoobj/fielduno
    sc/source/ui/unoobj/filtuno
    sc/source/ui/unoobj/fmtuno
    sc/source/ui/unoobj/forbiuno
    sc/source/ui/unoobj/funcuno
    sc/source/ui/unoobj/linkuno
    sc/source/ui/unoobj/listenercalls
    sc/source/ui/unoobj/miscuno
    sc/source/ui/unoobj/nameuno
    sc/source/ui/unoobj/notesuno
    sc/source/ui/unoobj/optuno
    sc/source/ui/unoobj/pageuno
    sc/source/ui/unoobj/PivotTableDataProvider
    sc/source/ui/unoobj/PivotTableDataSource
    sc/source/ui/unoobj/PivotTableDataSequence
    sc/source/ui/unoobj/TablePivotCharts
    sc/source/ui/unoobj/TablePivotChart
    sc/source/ui/unoobj/ChartTools
    sc/source/ui/unoobj/servuno
    sc/source/ui/unoobj/shapeuno
    sc/source/ui/unoobj/solveruno
    sc/source/ui/unoobj/srchuno
    sc/source/ui/unoobj/styleuno
    sc/source/ui/unoobj/targuno
    sc/source/ui/unoobj/textuno
    sc/source/ui/unoobj/tokenuno
    sc/source/ui/unoobj/unodoc
    sc/source/ui/unoobj/unoreflist
    sc/source/ui/unoobj/viewuno
    sc/source/ui/unoobj/warnpassword
    sc/source/ui/view/auditsh
    sc/source/ui/view/cellmergeoption
    sc/source/ui/view/cellsh
    sc/source/ui/view/cellsh1
    sc/source/ui/view/cellsh2
    sc/source/ui/view/cellsh3
    sc/source/ui/view/cellsh4
    sc/source/ui/view/colrowba
    sc/source/ui/view/cliputil
    sc/source/ui/view/dbfunc
    sc/source/ui/view/dbfunc2
    sc/source/ui/view/dbfunc3
    sc/source/ui/view/dbfunc4
    sc/source/ui/view/drawutil
    sc/source/ui/view/drawvie3
    sc/source/ui/view/drawvie4
    sc/source/ui/view/drawview
    sc/source/ui/view/editsh
    sc/source/ui/view/formatsh
    sc/source/ui/view/gridmerg
    sc/source/ui/view/gridwin
    sc/source/ui/view/gridwin2
    sc/source/ui/view/gridwin3
    sc/source/ui/view/gridwin4
    sc/source/ui/view/gridwin5
    sc/source/ui/view/hdrcont
    sc/source/ui/view/hintwin
    sc/source/ui/view/imapwrap
    sc/source/ui/view/invmerge
    sc/source/ui/view/notemark
    sc/source/ui/view/olinewin
    sc/source/ui/view/output
    sc/source/ui/view/output2
    sc/source/ui/view/output3
    sc/source/ui/view/overlayobject
    sc/source/ui/view/pfuncache
    sc/source/ui/view/pgbrksh
    sc/source/ui/view/pivotsh
    sc/source/ui/view/preview
    sc/source/ui/view/prevloc
    sc/source/ui/view/prevwsh
    sc/source/ui/view/prevwsh2
    sc/source/ui/view/printfun
    sc/source/ui/view/reffact
    sc/source/ui/view/scextopt
    sc/source/ui/view/select
    sc/source/ui/view/selectionstate
    sc/source/ui/view/SparklineShell
    sc/source/ui/view/spelldialog
    sc/source/ui/view/spelleng
    sc/source/ui/view/spellcheckcontext
    sc/source/ui/view/tabcont
    sc/source/ui/view/tableshell
    sc/source/ui/view/tabsplit
    sc/source/ui/view/tabview
    sc/source/ui/view/tabview2
    sc/source/ui/view/tabview3
    sc/source/ui/view/tabview4
    sc/source/ui/view/tabview5
    sc/source/ui/view/tabvwsh
    sc/source/ui/view/tabvwsh2
    sc/source/ui/view/tabvwsh3
    sc/source/ui/view/tabvwsh4
    sc/source/ui/view/tabvwsh5
    sc/source/ui/view/tabvwsh8
    sc/source/ui/view/tabvwsh9
    sc/source/ui/view/tabvwsha
    sc/source/ui/view/tabvwshb
    sc/source/ui/view/tabvwshc
    sc/source/ui/view/tabvwshd
    sc/source/ui/view/tabvwshe
    sc/source/ui/view/tabvwshf
    sc/source/ui/view/tabvwshg
    sc/source/ui/view/tabvwshh
    sc/source/ui/view/viewdata
    sc/source/ui/view/viewfun2
    sc/source/ui/view/viewfun3
    sc/source/ui/view/viewfun4
    sc/source/ui/view/viewfun5
    sc/source/ui/view/viewfun6
    sc/source/ui/view/viewfun7
    sc/source/ui/view/viewfunc
    sc/source/ui/view/viewutil
    sc/source/ui/view/waitoff
    sc/source/ui/xmlsource/xmlsourcedlg
)

# Library_sc.mk's OPENCL-gated block (gb_Helper_optional) -- ENABLE_OPENCL
# now a real option (cmake/LibreStudioConfigHeaders.cmake, default ON),
# since opencl/clew are real, locally-buildable targets in this port as of
# this wave (see opencl/CMakeLists.txt, external/clew/CMakeLists.txt).
if (ENABLE_OPENCL)
    list(APPEND SC_SOURCES
        sc/source/core/opencl/formulagroupcl
        sc/source/core/opencl/opbase
        sc/source/core/opencl/op_financial
        sc/source/core/opencl/op_math
        sc/source/core/opencl/op_addin
        sc/source/core/opencl/op_statistical
        sc/source/core/opencl/op_array
        sc/source/core/opencl/op_logical
        sc/source/core/opencl/op_spreadsheet
        sc/source/core/opencl/utils)
endif ()

librestudio_expand_cxx(SC_SOURCES)

# Cached for reuse by sc_ucalc_test's own 15 tests (sc/CppunitTest_
# deferred.cmake), which need to recompile sc's own sources directly into
# each test binary via gb_CppunitTest_use_library_objects(sc_ucalc$(1), sc
# scqahelper) -- same CACHE INTERNAL reuse pattern already established by
# editeng/CMakeLists.txt's own EDITENG_SOURCES for editeng_core.
set(SC_SOURCES "${SC_SOURCES}" CACHE INTERNAL
    "sc library's own expanded source list, for sc_ucalc_test's use_library_objects")

add_library(sc SHARED ${SC_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(sc PRIVATE
        "${CMAKE_SOURCE_DIR}/sc/inc/pch/precompiled_sc.hxx")
    target_compile_definitions(sc PRIVATE PCH_LEVEL=2)
endif ()
add_dependencies(sc sc_sdi)

target_include_directories(sc PRIVATE
    "${CMAKE_SOURCE_DIR}/sc/source/core/inc"
    "${CMAKE_SOURCE_DIR}/sc/source/filter/inc"
    "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
    "${CMAKE_SOURCE_DIR}/sc/inc"
    "${sc_sdi_INCLUDE_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    "${MDDS_INCLUDE_DIR}")
add_dependencies(sc officecfg_headers
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers)

target_compile_definitions(sc PRIVATE
    -DSC_DLLIMPLEMENTATION
    -DSC_INFO_OSVERSION="Linux")

target_link_libraries(sc PRIVATE
    avmedia basegfx chart2api comphelper cppu cppuhelper dbtools
    drawinglayercore drawinglayer docmodel editeng for forui fwk i18nlangtag
    i18nutil sal salhelper sax sb sfx sot svl svt svx svxcore
    textconversiondlgs tk tl ucbhelper utl vbahelper vcl xo
    Boost::boost
    ICU::uc ICU::i18n
    LibXml2::LibXml2
    ${LIBRESTUDIO_MD4C_TARGET} ${LIBRESTUDIO_MD4C_HTML_TARGET}
    ${LIBRESTUDIO_ORCUS_TARGET} ${LIBRESTUDIO_ORCUS_SPREADSHEET_TARGET}
    rt)

if (ENABLE_OPENCL)
    target_link_libraries(sc PRIVATE opencl clew)
endif ()

librestudio_add_component(sc
    COMPONENT "${CMAKE_SOURCE_DIR}/sc/util/sc.component"
    LIBRARY   sc
    LAYER     OOO)

librestudio_set_library_output_name(sc sc OOO)

install(TARGETS sc LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# scfilt -- Library_scfilt.mk
# ---------------------------------------------------------------------------

set(SCFILT_SOURCES
    sc/source/filter/dif/difexp
    sc/source/filter/dif/difimp
    sc/source/filter/excel/colrowst
    sc/source/filter/excel/excdoc
    sc/source/filter/excel/excel
    sc/source/filter/excel/excform
    sc/source/filter/excel/excform8
    sc/source/filter/excel/excimp8
    sc/source/filter/excel/excrecds
    sc/source/filter/excel/exctools
    sc/source/filter/excel/expop2
    sc/source/filter/excel/fontbuff
    sc/source/filter/excel/frmbase
    sc/source/filter/excel/impop
    sc/source/filter/excel/namebuff
    sc/source/filter/excel/read
    sc/source/filter/excel/tokstack
    sc/source/filter/excel/xechart
    sc/source/filter/excel/xecontent
    sc/source/filter/excel/xedbdata
    sc/source/filter/excel/xeescher
    sc/source/filter/excel/xeextlst
    sc/source/filter/excel/xeformula
    sc/source/filter/excel/xehelper
    sc/source/filter/excel/xelink
    sc/source/filter/excel/xename
    sc/source/filter/excel/xepage
    sc/source/filter/excel/xepivot
    sc/source/filter/excel/xepivotxml
    sc/source/filter/excel/xerecord
    sc/source/filter/excel/xeroot
    sc/source/filter/excel/xestream
    sc/source/filter/excel/xestring
    sc/source/filter/excel/xestyle
    sc/source/filter/excel/xetable
    sc/source/filter/excel/xeview
    sc/source/filter/excel/xichart
    sc/source/filter/excel/xicontent
    sc/source/filter/excel/xiescher
    sc/source/filter/excel/xiformula
    sc/source/filter/excel/xihelper
    sc/source/filter/excel/xilink
    sc/source/filter/excel/xiname
    sc/source/filter/excel/xipage
    sc/source/filter/excel/xipivot
    sc/source/filter/excel/xiroot
    sc/source/filter/excel/xistream
    sc/source/filter/excel/xistring
    sc/source/filter/excel/xistyle
    sc/source/filter/excel/xiview
    sc/source/filter/excel/xladdress
    sc/source/filter/excel/xlchart
    sc/source/filter/excel/xlescher
    sc/source/filter/excel/xlformula
    sc/source/filter/excel/xlpage
    sc/source/filter/excel/xlpivot
    sc/source/filter/excel/xlroot
    sc/source/filter/excel/xlstyle
    sc/source/filter/excel/xltoolbar
    sc/source/filter/excel/xltools
    sc/source/filter/excel/xltracer
    sc/source/filter/excel/xlview
    sc/source/filter/excel/export/NamedSheetViews
    sc/source/filter/excel/export/SparklineExt
    sc/source/filter/excel/export/ExportTools
    sc/source/filter/ftools/fapihelper
    sc/source/filter/ftools/fprogressbar
    sc/source/filter/ftools/ftools
    sc/source/filter/ftools/sharedformulagroups
    sc/source/filter/html/htmlexp
    sc/source/filter/html/htmlexp2
    sc/source/filter/html/htmlimp
    sc/source/filter/html/htmlpars
    sc/source/filter/lotus/filter
    sc/source/filter/lotus/lotattr
    sc/source/filter/lotus/lotform
    sc/source/filter/lotus/lotimpop
    sc/source/filter/lotus/lotread
    sc/source/filter/lotus/lotus
    sc/source/filter/lotus/memory
    sc/source/filter/lotus/op
    sc/source/filter/lotus/optab
    sc/source/filter/lotus/tool
    sc/source/filter/qpro/qpro
    sc/source/filter/qpro/qproform
    sc/source/filter/qpro/qprostyle
    sc/source/filter/rtf/eeimpars
    sc/source/filter/rtf/expbase
    sc/source/filter/rtf/rtfexp
    sc/source/filter/rtf/rtfimp
    sc/source/filter/rtf/rtfparse
    sc/source/filter/xcl97/XclExpChangeTrack
    sc/source/filter/xcl97/XclImpChangeTrack
    sc/source/filter/xcl97/xcl97esc
    sc/source/filter/xcl97/xcl97rec
    sc/source/filter/oox/excelfilter
    sc/source/filter/oox/addressconverter
    sc/source/filter/oox/autofilterbuffer
    sc/source/filter/oox/autofiltercontext
    sc/source/filter/oox/biffhelper
    sc/source/filter/oox/chartsheetfragment
    sc/source/filter/oox/commentsbuffer
    sc/source/filter/oox/commentsfragment
    sc/source/filter/oox/personsfragment
    sc/source/filter/oox/threadedcommentsfragment
    sc/source/filter/oox/NamedSheetViewFragment
    sc/source/filter/oox/NamedSheetViewImporter
    sc/source/filter/oox/condformatbuffer
    sc/source/filter/oox/condformatcontext
    sc/source/filter/oox/connectionsbuffer
    sc/source/filter/oox/connectionsfragment
    sc/source/filter/oox/defnamesbuffer
    sc/source/filter/oox/drawingbase
    sc/source/filter/oox/drawingfragment
    sc/source/filter/oox/excelchartconverter
    sc/source/filter/oox/excelhandlers
    sc/source/filter/oox/excelvbaproject
    sc/source/filter/oox/externallinkbuffer
    sc/source/filter/oox/externallinkfragment
    sc/source/filter/oox/extlstcontext
    sc/source/filter/oox/formulabase
    sc/source/filter/oox/formulabuffer
    sc/source/filter/oox/formulaparser
    sc/source/filter/oox/numberformatsbuffer
    sc/source/filter/oox/ooxformulaparser
    sc/source/filter/oox/pagesettings
    sc/source/filter/oox/patterncache
    sc/source/filter/oox/pivotcachebuffer
    sc/source/filter/oox/pivotcachefragment
    sc/source/filter/oox/pivottablebuffer
    sc/source/filter/oox/pivottablefragment
    sc/source/filter/oox/PivotTableFormat
    sc/source/filter/oox/PivotTableFormatContext
    sc/source/filter/oox/querytablebuffer
    sc/source/filter/oox/querytablefragment
    sc/source/filter/oox/revisionfragment
    sc/source/filter/oox/richstringcontext
    sc/source/filter/oox/richstring
    sc/source/filter/oox/scenariobuffer
    sc/source/filter/oox/scenariocontext
    sc/source/filter/oox/sharedstringsbuffer
    sc/source/filter/oox/sharedstringsfragment
    sc/source/filter/oox/sheetdatabuffer
    sc/source/filter/oox/sheetdatacontext
    sc/source/filter/oox/SparklineFragment
    sc/source/filter/oox/stylesbuffer
    sc/source/filter/oox/stylesfragment
    sc/source/filter/oox/tablebuffer
    sc/source/filter/oox/tablecolumnsbuffer
    sc/source/filter/oox/tablecolumnscontext
    sc/source/filter/oox/tablefragment
    sc/source/filter/oox/themebuffer
    sc/source/filter/oox/unitconverter
    sc/source/filter/oox/viewsettings
    sc/source/filter/oox/workbookfragment
    sc/source/filter/oox/workbookhelper
    sc/source/filter/oox/workbooksettings
    sc/source/filter/oox/worksheetbuffer
    sc/source/filter/oox/worksheetfragment
    sc/source/filter/oox/worksheethelper
    sc/source/filter/oox/worksheetsettings
    sc/source/filter/orcus/autofilter
    sc/source/filter/orcus/global
    sc/source/filter/orcus/interface
    sc/source/filter/orcus/orcusfiltersimpl
    sc/source/filter/orcus/xmlcontext
    sc/source/filter/orcus/filterdetect
    sc/source/filter/orcus/utils
)

# KNOWN BLOCKER, not resolved (first real `cmake --build`, 2026-09-27):
# download.lst pins this LibreOffice checkout's own expected orcus version at
# 0.21.0 (ORCUS_TARBALL := liborcus-0.21.0.tar.xz), but the newest orcus
# package available on this system (via apt, both installed and candidate)
# is 0.19.2-alt1 -- confirmed no newer liborcus-devel candidate exists in any
# configured repository. Five of these seven sources (autofilter, interface,
# orcusfiltersimpl, xmlcontext, filterdetect -- global and utils are fine)
# fail to compile: orcus_autofilter.hxx (sc/source/filter/inc, transitively
# pulled in by orcusinterface.hxx, which nearly every file in this directory
# includes) needs <orcus/spreadsheet/import_interface_auto_filter.hpp> and
# the orcus::spreadsheet::iface::import_auto_filter_multi_values abstract
# interface it declares (ScOrcusAutoFilterMultiValues implements it) --
# neither exists in orcus 0.19.2's headers at all, not a renamed/reorganized
# header (checked orcus/spreadsheet/auto_filter.hpp, the one auto_filter-
# related header 0.19.2 does ship: no such interface class in it). This is a
# genuine added-between-0.19-and-0.21 orcus feature, not a build-script gap.
#
# NOT worked around by excluding these 5 files: ScOrcusFiltersImpl
# (orcusfiltersimpl.cxx) is instantiated by sc/source/filter/ftools/
# ftools.cxx's ScFormatFilterPluginImpl::GetOrcusFilters() (grepped: called
# from sc/source/ui/docshell/docsh.cxx, docsh2.cxx, sc/source/ui/dataprovider/
# xmldataprovider.cxx, sc/source/ui/xmlsource/xmlsourcedlg.cxx), a real,
# load-bearing part of Calc's own document-loading/external-data-source
# machinery, not an isolated leaf -- removing it would just move the
# "undefined reference" failure to ftools.cxx and everything scfilt links
# from there instead of fixing anything.
#
# Left as-is (still listed, still fails to build) rather than silently
# patched around: resolving this needs either a real orcus >= 0.21 (vendored
# tarball build, same class of infrastructure this port already built for
# harfbuzz/mdds/zxcvbn-c when the system package was too old) or a
# considered decision to stub the newer interface -- both are real, scoped
# follow-up work, not a build-script fix, and not attempted in this
# unattended overnight session.

librestudio_expand_cxx(SCFILT_SOURCES)
add_library(scfilt SHARED ${SCFILT_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(scfilt PRIVATE
        "${CMAKE_SOURCE_DIR}/sc/inc/pch/precompiled_scfilt.hxx")
    target_compile_definitions(scfilt PRIVATE PCH_LEVEL=2)
endif ()
add_dependencies(scfilt oox_generated)

target_include_directories(scfilt PRIVATE
    "${CMAKE_SOURCE_DIR}/sc/source/core/inc"
    "${CMAKE_SOURCE_DIR}/sc/source/filter/inc"
    "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
    "${CMAKE_SOURCE_DIR}/sc/inc"
    "${OOX_GENERATED_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    "${MDDS_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}")
add_dependencies(scfilt officecfg_headers
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers)

target_link_libraries(scfilt PRIVATE
    basegfx comphelper cppu cppuhelper drawinglayer docmodel editeng for
    i18nlangtag msfilter oox sal salhelper sax sb sc sfx sot svl svt svx
    svxcore tk tl ucbhelper utl vcl
    Boost::boost
    ${LIBRESTUDIO_ORCUS_TARGET} ${LIBRESTUDIO_ORCUS_SPREADSHEET_TARGET})

# Fix (first real `cmake --build`, 2026-09-27): Library_scfilt.mk links clew
# under gb_Helper_optional(OPENCL, clew) (line 37, no separate opencl link
# for this target) -- missing here, same "OPENCL BUILD_TYPE vs ENABLE_OPENCL"
# class of gap already found and fixed for vcl/scui/sofficeapp/cui.
# xetable.cxx (and others) reach <opencl/platforminfo.hxx>, a public global
# header needing <clew/clew.h> -- unreachable without linking clew directly
# (clew's own target_include_directories is PUBLIC, so this alone suffices).
if (ENABLE_OPENCL)
    target_link_libraries(scfilt PRIVATE clew)
endif ()

librestudio_add_component(scfilt
    COMPONENT "${CMAKE_SOURCE_DIR}/sc/util/scfilt.component"
    LIBRARY   scfilt
    LAYER     OOO)

librestudio_set_library_output_name(scfilt scfilt OOO)

install(TARGETS scfilt LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# scui -- Library_scui.mk
# ---------------------------------------------------------------------------

set(SCUI_SOURCES
    sc/source/ui/attrdlg/attrdlg
    sc/source/ui/attrdlg/scdlgfact
    sc/source/ui/attrdlg/scuiexp
    sc/source/ui/attrdlg/tabpages
    sc/source/ui/cctrl/editfield
    sc/source/ui/condformat/condformatmgr
    sc/source/ui/dbgui/dapidata
    sc/source/ui/dbgui/dapitype
    sc/source/ui/dbgui/dpgroupdlg
    sc/source/ui/dbgui/pfiltdlg
    sc/source/ui/dbgui/pvfundlg
    sc/source/ui/dbgui/scendlg
    sc/source/ui/dbgui/scuiasciiopt
    sc/source/ui/dbgui/scuiimoptdlg
    sc/source/ui/dbgui/sortdlg
    sc/source/ui/dbgui/sortkeydlg
    sc/source/ui/dbgui/subtdlg
    sc/source/ui/dbgui/textimportoptions
    sc/source/ui/dbgui/tpsort
    sc/source/ui/dbgui/tpsubt
    sc/source/ui/docshell/tpstat
    sc/source/ui/miscdlgs/crdlg
    sc/source/ui/miscdlgs/datafdlg
    sc/source/ui/miscdlgs/delcldlg
    sc/source/ui/miscdlgs/delcodlg
    sc/source/ui/miscdlgs/filldlg
    sc/source/ui/miscdlgs/gototabdlg
    sc/source/ui/miscdlgs/groupdlg
    sc/source/ui/miscdlgs/inscldlg
    sc/source/ui/miscdlgs/inscodlg
    sc/source/ui/miscdlgs/instbdlg
    sc/source/ui/miscdlgs/lbseldlg
    sc/source/ui/miscdlgs/linkarea
    sc/source/ui/miscdlgs/mtrindlg
    sc/source/ui/miscdlgs/mvtabdlg
    sc/source/ui/miscdlgs/namecrea
    sc/source/ui/miscdlgs/shtabdlg
    sc/source/ui/miscdlgs/strindlg
    sc/source/ui/miscdlgs/tabbgcolordlg
    sc/source/ui/miscdlgs/textdlgs
    sc/source/ui/namedlg/namepast
    sc/source/ui/optdlg/calcoptionsdlg
    sc/source/ui/optdlg/opredlin
    sc/source/ui/optdlg/tpcalc
    sc/source/ui/optdlg/tpcompatibility
    sc/source/ui/optdlg/tpformula
    sc/source/ui/optdlg/tpdefaults
    sc/source/ui/optdlg/tpprint
    sc/source/ui/optdlg/tpusrlst
    sc/source/ui/optdlg/tpview
    sc/source/ui/pagedlg/hfedtdlg
    sc/source/ui/pagedlg/scuitphfedit
    sc/source/ui/pagedlg/tphf
    sc/source/ui/pagedlg/tptable
    sc/source/ui/styleui/styledlg
)

librestudio_expand_cxx(SCUI_SOURCES)
add_library(scui SHARED ${SCUI_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(scui PRIVATE
        "${CMAKE_SOURCE_DIR}/sc/inc/pch/precompiled_scui.hxx")
    target_compile_definitions(scui PRIVATE PCH_LEVEL=2)
endif ()
add_dependencies(scui sc_sdi)

target_include_directories(scui PRIVATE
    "${CMAKE_SOURCE_DIR}/sc/source/core/inc"
    "${CMAKE_SOURCE_DIR}/sc/source/filter/inc"
    "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
    "${CMAKE_SOURCE_DIR}/sc/inc"
    "${sc_sdi_INCLUDE_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    "${MDDS_INCLUDE_DIR}")
add_dependencies(scui officecfg_headers
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers)

target_link_libraries(scui PRIVATE
    comphelper cppu cppuhelper docmodel editeng for forui i18nlangtag sal sc
    sfx sot svl svt svx svxcore tl utl vcl
    Boost::boost
    ICU::uc ICU::i18n)

# Fix (first real `cmake --build`, 2026-09-27): Library_scui.mk links both
# clew and opencl under gb_Helper_optional(OPENCL, ...) (lines 41-42, 58-59)
# -- missing here, the same "OPENCL BUILD_TYPE vs ENABLE_OPENCL" mismatch
# already found and fixed for vcl (see vcl/CMakeLists.txt's own comment on
# this exact class of gap): tpcalc.cxx includes <opencl/openclwrapper.hxx>
# (a public, global header), which itself needs <clew/clew.h> -- "fatal
# error: clew/clew.h: No such file" without this. clew's own
# target_include_directories(clew PUBLIC ...) (external/clew/CMakeLists.txt)
# propagates the include path automatically once linked directly (opencl
# links clew PRIVATE, so linking opencl alone would not be enough -- both
# are linked here, matching the real .mk's own list).
if (ENABLE_OPENCL)
    target_link_libraries(scui PRIVATE clew opencl)
endif ()

librestudio_add_component(scui
    COMPONENT "${CMAKE_SOURCE_DIR}/sc/util/scui.component"
    LIBRARY   scui
    LAYER     OOO)

librestudio_set_library_output_name(scui scui OOO)

install(TARGETS scui LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# solver -- Library_solver.mk
# ---------------------------------------------------------------------------
#
# ENABLE_COINMP and its detection (COINMP_DETECT_FOUND) are declared in
# cmake/LibreStudioConfigHeaders.cmake (included into scope from the root
# CMakeLists.txt well before this module's own add_subdirectory()) --
# sc/CppunitTest_deferred.cmake's solver test already reads ENABLE_COINMP
# too, so this reuses that one real, already-existing option. The actual
# CoinMP acquisition (system-or-vendor detection, the ~17-patch autotools
# `./configure && make` over the whole COIN-OR tree when vendored) is
# resolved right below via librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake) -- external/coinmp/coinmp.cmake's own header
# comment has the vendoring story; this file just adds the 3 CoinMP-specific
# source files and links LIBRESTUDIO_COINMP_TARGET, whichever branch
# resolved it. DRIVER explicit ("coinmp"): MODULES has 2 elements
# (coinmp+coinutils), so no auto-detection candidate exists at all.

set(SOLVER_SOURCES
    sc/source/solver/SwarmSolver
    sc/source/solver/SolverComponent
)

if (ENABLE_COINMP)
    librestudio_pkgconfig(
        NAME COINMP
        MODULES coinmp coinutils
        DRIVER coinmp)

    list(APPEND SOLVER_SOURCES
        sc/source/solver/CoinMPSolver
        sc/source/solver/CoinMPModel
        sc/source/solver/SlpSolver)
endif ()

librestudio_expand_cxx(SOLVER_SOURCES)
add_library(solver SHARED ${SOLVER_SOURCES})

target_include_directories(solver PRIVATE
    "${CMAKE_SOURCE_DIR}/sc/inc"
    "${CMAKE_SOURCE_DIR}/sc/source/solver"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(solver udkapi_normal_headers offapi_normal_headers)

target_link_libraries(solver PRIVATE
    comphelper cppu cppuhelper sal sc utl i18nlangtag
    Boost::boost)
if (ENABLE_COINMP)
    target_link_libraries(solver PRIVATE ${LIBRESTUDIO_COINMP_TARGET})
endif ()

# gb_Library_add_componentimpls,solver,$(if $(ENABLE_COINMP),coinmp)):
# solver.component marks its CoinMPSolver/SLPSolver <implementation>s
# <optional/> -- librestudio_add_component()'s own IMPLEMENTATION parameter
# (cmake/LibreStudioComponent.cmake:80-134, read in full) filters an
# optional implementation OUT of the built .component unless its name
# appears in one of the given IMPLEMENTATION files. solver.component.coinmp
# names exactly those two, so passing it when ENABLE_COINMP is on keeps both
# (they really are compiled in); when off, an empty file forces both back
# out. PRE-EXISTING BUG FOUND AND FIXED HERE: before this change this call
# passed no IMPLEMENTATION at all, which skips the filter entirely (see the
# early "if (_comp_IMPLEMENTATION)" guard) -- both optionals stayed listed
# in solver.component even with ENABLE_COINMP undefined and neither source
# file compiled in, a latent "services.rdb names a constructor symbol the
# .so does not export" bug this fix removes as a side effect.
if (ENABLE_COINMP)
    set(SC_SOLVER_IMPLEMENTATION "${CMAKE_SOURCE_DIR}/sc/source/solver/solver.component.coinmp")
else ()
    set(SC_SOLVER_IMPLEMENTATION "${CMAKE_CURRENT_BINARY_DIR}/solver.component.none")
    file(WRITE "${SC_SOLVER_IMPLEMENTATION}" "")
endif ()

librestudio_add_component(solver
    COMPONENT      "${CMAKE_SOURCE_DIR}/sc/source/solver/solver.component"
    IMPLEMENTATION "${SC_SOLVER_IMPLEMENTATION}"
    LIBRARY        solver
    LAYER          OOO)

librestudio_set_library_output_name(solver solver OOO)

install(TARGETS solver LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# vbaobj -- Library_vbaobj.mk
# ---------------------------------------------------------------------------

# Module_sc.mk: `ifneq ($(filter SCRIPTING,$(BUILD_TYPE)),)` around the whole
# Library_vbaobj target. ENABLE_SCRIPTING defaults ON.
if (ENABLE_SCRIPTING)

set(VBAOBJ_SOURCES
    sc/source/ui/vba/excelvbahelper
    sc/source/ui/vba/vbaapplication
    sc/source/ui/vba/vbaassistant
    sc/source/ui/vba/vbaaxes
    sc/source/ui/vba/vbaaxis
    sc/source/ui/vba/vbaaxistitle
    sc/source/ui/vba/vbaborders
    sc/source/ui/vba/vbacharacters
    sc/source/ui/vba/vbachart
    sc/source/ui/vba/vbachartobject
    sc/source/ui/vba/vbachartobjects
    sc/source/ui/vba/vbacharttitle
    sc/source/ui/vba/vbacomment
    sc/source/ui/vba/vbacomments
    sc/source/ui/vba/vbacondition
    sc/source/ui/vba/vbadialog
    sc/source/ui/vba/vbadialogs
    sc/source/ui/vba/vbaeventshelper
    sc/source/ui/vba/vbafiledialog
    sc/source/ui/vba/vbafiledialogitems
    sc/source/ui/vba/vbafont
    sc/source/ui/vba/vbaformat
    sc/source/ui/vba/vbaformatcondition
    sc/source/ui/vba/vbaformatconditions
    sc/source/ui/vba/vbaglobals
    sc/source/ui/vba/vbahyperlink
    sc/source/ui/vba/vbahyperlinks
    sc/source/ui/vba/vbainterior
    sc/source/ui/vba/vbalineshape
    sc/source/ui/vba/vbamenubar
    sc/source/ui/vba/vbamenubars
    sc/source/ui/vba/vbamenu
    sc/source/ui/vba/vbamenus
    sc/source/ui/vba/vbamenuitem
    sc/source/ui/vba/vbamenuitems
    sc/source/ui/vba/vbaname
    sc/source/ui/vba/vbanames
    sc/source/ui/vba/vbaoleobject
    sc/source/ui/vba/vbaoleobjects
    sc/source/ui/vba/vbaoutline
    sc/source/ui/vba/vbaovalshape
    sc/source/ui/vba/vbapagebreak
    sc/source/ui/vba/vbapagebreaks
    sc/source/ui/vba/vbapagesetup
    sc/source/ui/vba/vbapalette
    sc/source/ui/vba/vbapane
    sc/source/ui/vba/vbapivotcache
    sc/source/ui/vba/vbapivottable
    sc/source/ui/vba/vbapivottables
    sc/source/ui/vba/vbarange
    sc/source/ui/vba/vbasheetobject
    sc/source/ui/vba/vbasheetobjects
    sc/source/ui/vba/vbastyle
    sc/source/ui/vba/vbastyles
    sc/source/ui/vba/vbatextboxshape
    sc/source/ui/vba/vbatextframe
    sc/source/ui/vba/vbavalidation
    sc/source/ui/vba/vbawindow
    sc/source/ui/vba/vbawindows
    sc/source/ui/vba/vbaworkbook
    sc/source/ui/vba/vbaworkbooks
    sc/source/ui/vba/vbaworksheet
    sc/source/ui/vba/vbaworksheets
    sc/source/ui/vba/vbawsfunction
)

librestudio_expand_cxx(VBAOBJ_SOURCES)
add_library(vbaobj SHARED ${VBAOBJ_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(vbaobj PRIVATE
        "${CMAKE_SOURCE_DIR}/sc/inc/pch/precompiled_vbaobj.hxx")
    target_compile_definitions(vbaobj PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(vbaobj PRIVATE
    "${CMAKE_SOURCE_DIR}/sc/source/core/inc"
    "${CMAKE_SOURCE_DIR}/sc/source/filter/inc"
    "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
    "${CMAKE_SOURCE_DIR}/sc/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    "${MDDS_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}")
add_dependencies(vbaobj
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers)

target_link_libraries(vbaobj PRIVATE
    comphelper cppu cppuhelper editeng for i18nlangtag msfilter sal sb sc sfx
    svl svt svx svxcore tk tl utl vbahelper vcl
    Boost::boost)

librestudio_add_component(vbaobj
    COMPONENT "${CMAKE_SOURCE_DIR}/sc/util/vbaobj.component"
    LIBRARY   vbaobj
    LAYER     OOO)

librestudio_set_library_output_name(vbaobj vbaobj OOO)

install(TARGETS vbaobj LIBRARY DESTINATION program)

endif ()

# ---------------------------------------------------------------------------
# scd -- Library_scd.mk
# ---------------------------------------------------------------------------

set(SCD_SOURCES
    sc/source/ui/unoobj/scdetect
    sc/source/ui/unoobj/exceldetect
)

librestudio_expand_cxx(SCD_SOURCES)
add_library(scd SHARED ${SCD_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(scd REUSE_FROM precompiled_system)
endif ()

target_include_directories(scd PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    "${CMAKE_SOURCE_DIR}/sc/inc")
add_dependencies(scd
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers)

target_link_libraries(scd PRIVATE
    comphelper cppu cppuhelper sal sfx sot tl utl
    Boost::boost)

librestudio_add_component(scd
    COMPONENT "${CMAKE_SOURCE_DIR}/sc/util/scd.component"
    LIBRARY   scd
    LAYER     OOO)

librestudio_set_library_output_name(scd scd OOO)

install(TARGETS scd LIBRARY DESTINATION program)

# --- scqahelper (Library_scqahelper.mk) --------------------------------------
#
# Not a product library -- a shared helper (QueryHelper/Test base classes,
# tiled-rendering/view-callback test harness) that several of sc's own
# CppunitTest_*.mk files link against (sc_functions_test/sc_subsequent_test,
# plain use_libraries) or recompile directly into the test binary
# (sc_ucalc_test, use_library_objects -- see sc/CppunitTest_deferred.cmake).
# Repository.mk registers it under gb_Helper_register_libraries(
# PLAINLIBS_NONE,...), the same "PLAIN naming, never installed" group as
# subsequenttest/test/unotest/the *protector libraries -- so PLAIN naming,
# no install() call, matching test/CMakeLists.txt's own subsequenttest
# precedent exactly.
#
# use_externals: boost_headers -> Boost::boost, mdds_headers ->
# ${MDDS_INCLUDE_DIR} (matching sc's own resolution above), cppunit ->
# PkgConfig::CPPUNIT (librestudio_add_cppunit_test() already resolves this
# same way; scqahelper needs it directly too, since qahelper.cxx and
# friends use CPPUNIT_ASSERT macros themselves), libxml2 ->
# LibXml2::LibXml2, OPENCL's optional clew skipped (inert everywhere else
# in this port, ENABLE_OPENCL/OPENCL never configured here). The ifneq(
# SYSTEM_LIBORCUS) branch resolves to its "else" (orcus-parser) in this
# environment -- re-verified against sc/CMakeLists.txt's own header
# comment: the system liborcus (0.19.2) is too old, so this whole tree
# already resolves through the vendored-or-system-if-new-enough smart-
# detection block in root CMakeLists.txt, landing on the same
# ${LIBRESTUDIO_ORCUS_TARGET} sc's own library already links (not also
# ${LIBRESTUDIO_ORCUS_SPREADSHEET_TARGET} -- scqahelper's own qa/unit/
# helper/*.cxx sources need no spreadsheet-model-specific orcus interface,
# only the base parser one gb_Library_use_externals(scqahelper,orcus-
# parser) itself names).
set(SCQAHELPER_SOURCES
    sc/qa/unit/helper/qahelper
    sc/qa/unit/helper/scfiltertestbase
    sc/qa/unit/helper/sctiledrenderingtest
    sc/qa/unit/helper/sctestviewcallback
    sc/qa/unit/functions_test
)
librestudio_expand_cxx(SCQAHELPER_SOURCES)

# Cached for reuse by sc_ucalc_test's own 15 tests, same reason/pattern as
# SC_SOURCES just above.
set(SCQAHELPER_SOURCES "${SCQAHELPER_SOURCES}" CACHE INTERNAL
    "scqahelper library's own expanded source list, for sc_ucalc_test's use_library_objects")

add_library(scqahelper SHARED ${SCQAHELPER_SOURCES})

# PkgConfig::CPPUNIT is normally resolved lazily, on first use, by
# librestudio_add_cppunit_test() itself (cmake/LibreStudioTest.cmake) -- but
# that import target's own visibility (pkg_check_modules(...IMPORTED_TARGET)
# without GLOBAL) is scoped to the directory that first creates it plus its
# own subdirectories, and scqahelper is a plain add_library() here in sc's
# own directory scope, not reached through that function -- confirmed by a
# real reconfigure: `target_link_libraries(scqahelper ... PkgConfig::CPPUNIT)`
# fails with "the target was not found" without this guard, since every
# CppunitTest that would otherwise have created it first is itself deferred
# to run in the root directory's own scope, at the very end -- long after
# sc's own (non-deferred) processing point. Same guard, verbatim.
if (NOT TARGET PkgConfig::CPPUNIT)
    pkg_check_modules(CPPUNIT REQUIRED IMPORTED_TARGET cppunit)
endif ()

target_include_directories(scqahelper PRIVATE
    "${CMAKE_SOURCE_DIR}/sc/inc"
    "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    "${MDDS_INCLUDE_DIR}")
add_dependencies(scqahelper
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers)

target_compile_definitions(scqahelper PRIVATE SCQAHELPER_DLLIMPLEMENTATION)

target_link_libraries(scqahelper PRIVATE
    comphelper cppu cppuhelper docmodel editeng for sal sc sfx sot
    subsequenttest svl svt svx svxcore test_fixtures tl unotest utl
    ucbhelper vcl
    Boost::boost LibXml2::LibXml2 PkgConfig::CPPUNIT
    ${LIBRESTUDIO_ORCUS_TARGET})

# Same gap already found and fixed for scfilt just above (search this file
# for "Library_scfilt.mk links clew"): qahelper.cxx includes sc/inc/
# formulagroup.hxx, a public sc header reaching <opencl/platforminfo.hxx> ->
# <clew/clew.h> -- but sc's own link to clew (line ~889, `target_link_
# libraries(sc PRIVATE opencl clew)`) is PRIVATE, so it does not propagate
# clew's PUBLIC include dirs to scqahelper just by linking sc. Confirmed live
# on the remote test build: "fatal error: clew/clew.h: No such file or
# directory". clew's own target_include_directories is PUBLIC, so linking it
# here directly is sufficient, same fix as scfilt's.
target_link_libraries(scqahelper PRIVATE clew)

librestudio_set_library_output_name(scqahelper scqahelper PLAIN)

# --- UIConfig_scalc (gb_UIConfig_UIConfig(modules/scalc)) -------------------
#
# UIConfig_scalc.mk, re-checked directly: 1 menubar, 29 popupmenu, 1
# statusbar, 38 toolbar, 188 ui files -- same shape/pattern as simpress_
# uiconfig/sdraw_uiconfig above (different module). Every file verified
# present on disk before writing this block (python3 script, not assumed).
# Closes the "scalc_uiconfig does not exist" gap already documented at
# several sc CppunitTest call sites this session (5 tests' own UICONFIG
# omitting "scalc").
set(SC_SCALC_UICONFIG_MENUBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/menubar/menubar.xml"
)

set(SC_SCALC_UICONFIG_POPUPMENU_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/anchor.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/audit.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/cell.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/celledit.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/colheader.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/column_operations.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/conditional.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/conditional_easy.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/datatablemenu.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/draw.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/drawtext.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/form.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/formrichtext.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/formulabar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/freezepanes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/graphic.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/media.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/notebookbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/oleobject.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/pagebreak.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/pivot.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/preview.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/printranges.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/rowheader.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/row_operations.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/sheettab.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/sparkline.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/sparklinemenu.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/popupmenu/statisticsmenu.xml"
)

set(SC_SCALC_UICONFIG_STATUSBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/statusbar/statusbar.xml"
)

set(SC_SCALC_UICONFIG_TOOLBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/alignmentbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/arrowsbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/arrowshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/basicshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/calloutshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/classificationbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/colorbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/connectorsbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/datastreams.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/drawbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/drawobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/extrusionobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/findbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/flowchartshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/fontworkobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/fontworkshapetype.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/formatobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/formcontrols.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/formdesign.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/formsfilterbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/formsnavigationbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/formtextobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/fullscreenbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/graffilterbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/graphicobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/insertbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/insertcellsbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/linesbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/mediaobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/notebookbarshortcuts.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/previewbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/singlemode.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/standardbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/starshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/symbolshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/textobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/toolbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/toolbar/viewerbar.xml"
)

set(SC_SCALC_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/advancedfilterdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/allheaderfooterdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/analysisofvariancedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/autoformattable.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/autosum.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/cellprotectionpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/duplicaterecordsdlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/changesourcedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/chardialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/checkwarningdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/chisquaretestdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/colormenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/colorrowdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/colwidthdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/condformatmanager.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/conditionaleasydialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/conditionalformatdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/conditionalentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/conditionaliconset.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/conflictsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/consolidatedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/correlationdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/covariancedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/createnamesdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/dapiservicedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/databaroptions.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/datafielddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/datafieldoptionsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/dataform.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/dataformfragment.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/datastreams.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/dataproviderdlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/definedatabaserangedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/definename.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/definetablerangedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/deletecells.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/deletecolumnentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/deletecontents.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/descriptivestatisticsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/drawtemplatedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/dropmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/doubledialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/erroralerttabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/externaldata.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/exponentialsmoothingdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/filldlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/filterlist.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/filterdropdown.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/filtersubdropdown.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/footerdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/formatcellsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/formulacalculationoptions.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/fourieranalysisdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/floatingborderstyle.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/floatinglinestyle.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/functionpanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/goalseekdlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/gotosheetdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/groupdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/groupbydate.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/groupbynumber.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/headerdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/headerfootercontent.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/headerfooterdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/imoptdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/inputbar.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/inputstringdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/insertcells.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/insertname.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/insertsheet.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/integerdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/leftfooterdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/leftheaderdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/listmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/namerangesdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/notebookbar.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/notebookbar_compact.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/notebookbar_groups.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/notebookbar_groupedbar_full.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/notebookbar_groupedbar_compact.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/numberbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/managenamesdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/mergecellsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/mergecolumnentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/texttransformationentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sorttransformationentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/aggregatefunctionentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/numbertransformationentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/replacenulltransformationentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/datetimetransformationentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/findreplaceentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/deleterowentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/swaprowsentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/movecopysheet.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/movingaveragedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/multipleoperationsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/navigatorpanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/nosolutiondialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/optcalculatepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/optchangespage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/optcompatibilitypage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/optdefaultpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/optdlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/optformula.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/optimalcolwidthdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/optimalrowheightdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/optsortlists.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/pagelistmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/pagetemplatedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/pastespecial.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/paradialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/paratemplatedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/passfragment.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/pivotcalcfielddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/pivotfielddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/pivotfilterdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/pivottablelayoutdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/posbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/printareasdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/printeroptions.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/protectsheetdlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/queryrunstreamscriptdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/randomnumbergenerator.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/recalcquerydialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/regressiondialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/retypepassdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/retypepassworddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/rightfooterdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/rightheaderdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/rowheightdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/samplingdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/standardfilterdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/scenariodialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/scenariomenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/scgeneralpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/searchresults.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/selectdatasource.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/selectrange.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/selectsheetviewdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/selectsource.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sheetprintpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sharedocumentdlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sharedfirstfooterdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sharedfirstheaderdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sharedleftfooterdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sharedleftheaderdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sharedfooterdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sharedheaderdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sharedwarningdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sheetviewbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/showchangesdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/showdetaildialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/showsheetdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sidebaralignment.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sidebarnumberformat.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sidebarcellappearance.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sidebardatabase.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/simplerefdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/solverdlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/solveroptionsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/solverprogressdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/solversuccessdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sortcriteriapage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sortdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sortkey.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sortoptionspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sortwarning.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sparklinedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/sparklinedatarangedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/splitcolumnentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/subtotaldialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/subtotaloptionspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/subtotalgrppage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/statisticsinfopage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/tablestylesbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/tabcolordialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/textimportoptions.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/textimportcsv.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/tpviewpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/ttestdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/ungroupdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/validationdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/validationcriteriapage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/validationhelptabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/warnautocorrect.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/xmlsourcedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/zoombox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/scalc/ui/ztestdialog.ui"
)

install(FILES ${SC_SCALC_UICONFIG_MENUBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/scalc/menubar)
install(FILES ${SC_SCALC_UICONFIG_POPUPMENU_FILES}
    DESTINATION share/config/soffice.cfg/modules/scalc/popupmenu)
install(FILES ${SC_SCALC_UICONFIG_STATUSBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/scalc/statusbar)
install(FILES ${SC_SCALC_UICONFIG_TOOLBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/scalc/toolbar)
install(FILES ${SC_SCALC_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/modules/scalc/ui)

set(_scalc_uiconfig_outputs "")
foreach (_scalc_uiconfig_pair
        "SC_SCALC_UICONFIG_MENUBAR_FILES:menubar"
        "SC_SCALC_UICONFIG_POPUPMENU_FILES:popupmenu"
        "SC_SCALC_UICONFIG_STATUSBAR_FILES:statusbar"
        "SC_SCALC_UICONFIG_TOOLBAR_FILES:toolbar"
        "SC_SCALC_UICONFIG_UI_FILES:ui")
    string(REPLACE ":" ";" _scalc_uiconfig_pair "${_scalc_uiconfig_pair}")
    list(GET _scalc_uiconfig_pair 0 _scalc_uiconfig_var)
    list(GET _scalc_uiconfig_pair 1 _scalc_uiconfig_category)
    foreach (_scalc_uiconfig_src IN LISTS ${_scalc_uiconfig_var})
        get_filename_component(_scalc_uiconfig_name "${_scalc_uiconfig_src}" NAME)
        set(_scalc_uiconfig_dest
            "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/scalc/${_scalc_uiconfig_category}/${_scalc_uiconfig_name}")
        add_custom_command(
            OUTPUT "${_scalc_uiconfig_dest}"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/scalc/${_scalc_uiconfig_category}"
            COMMAND ${CMAKE_COMMAND} -E copy "${_scalc_uiconfig_src}" "${_scalc_uiconfig_dest}"
            DEPENDS "${_scalc_uiconfig_src}"
            COMMENT "Copying ${_scalc_uiconfig_name} into build-tree soffice.cfg/modules/scalc/${_scalc_uiconfig_category}")
        list(APPEND _scalc_uiconfig_outputs "${_scalc_uiconfig_dest}")
    endforeach ()
endforeach ()
unset(_scalc_uiconfig_pair)
unset(_scalc_uiconfig_var)
unset(_scalc_uiconfig_category)
unset(_scalc_uiconfig_src)
unset(_scalc_uiconfig_name)
unset(_scalc_uiconfig_dest)

add_custom_target(scalc_uiconfig ALL DEPENDS ${_scalc_uiconfig_outputs})
unset(_scalc_uiconfig_outputs)


# --- CppunitTests ------------------------------------------------------------
#
# 189 CppunitTest_*.mk files -- split out to sc/CppunitTest_deferred.cmake
# (deferred: needs program_services_rdb, only set much later in the root
# CMakeLists.txt, same ordering constraint as every other module's own
# CppunitTest_deferred.cmake). See that file's own header comment for the
# real structure (4 shared gbuild macros covering most of the 189, ported
# incrementally wave by wave rather than 189 independent hand transcriptions)
# and current progress.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/sc/CppunitTest_deferred.cmake")
