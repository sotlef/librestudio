# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# reportdesign (module reportdesign) — the "Report Builder": rpt
# (Library_rpt.mk) is the report document model itself (the UNO
# ReportDefinition/Section/Group/... component API under core/api, the Draw
# page/SdrObject-based layout engine under core/sdr, and the ODF report
# filter under filter/xml); rptui (Library_rptui.mk) is the report design
# editor UI built on top of it (the property-browser inspection handlers,
# the design view/controller, and its dialogs). rptui links rpt, so rpt is
# declared first below. Source lists, includes and library sets are
# transcribed from each library's Library_*.mk.
#
# Neither library declares a private UNO API (no InternalUnoApi_reportdesign
# .mk exists) -- both call plain gb_Library_use_sdk_api, so they only need
# the normal-variant udkapi/offapi headers, wired the same way
# forms/CMakeLists.txt does it.

# --- rpt --------------------------------------------------------------------

set(RPT_SOURCES
    reportdesign/source/core/api/FixedLine
    reportdesign/source/core/api/FixedText
    reportdesign/source/core/api/FormatCondition
    reportdesign/source/core/api/FormattedField
    reportdesign/source/core/api/Function
    reportdesign/source/core/api/Functions
    reportdesign/source/core/api/Group
    reportdesign/source/core/api/Groups
    reportdesign/source/core/api/ImageControl
    reportdesign/source/core/api/ReportComponent
    reportdesign/source/core/api/ReportControlModel
    reportdesign/source/core/api/ReportDefinition
    reportdesign/source/core/api/ReportEngineJFree
    reportdesign/source/core/api/ReportVisitor
    reportdesign/source/core/api/Section
    reportdesign/source/core/api/services
    reportdesign/source/core/api/Shape
    reportdesign/source/core/api/Tools
    reportdesign/source/core/misc/conditionalexpression
    reportdesign/source/core/misc/conditionupdater
    reportdesign/source/core/misc/reportformula
    reportdesign/source/core/resource/core_resource
    reportdesign/source/core/sdr/formatnormalizer
    reportdesign/source/core/sdr/PropertyForward
    reportdesign/source/core/sdr/ReportDrawPage
    reportdesign/source/core/sdr/ReportUndoFactory
    reportdesign/source/core/sdr/RptModel
    reportdesign/source/core/sdr/RptObject
    reportdesign/source/core/sdr/RptObjectListener
    reportdesign/source/core/sdr/RptPage
    reportdesign/source/core/sdr/UndoActions
    reportdesign/source/core/sdr/UndoEnv
    reportdesign/source/filter/xml/dbloader2
    reportdesign/source/filter/xml/xmlAutoStyle
    reportdesign/source/filter/xml/xmlCell
    reportdesign/source/filter/xml/xmlColumn
    reportdesign/source/filter/xml/xmlComponent
    reportdesign/source/filter/xml/xmlCondPrtExpr
    reportdesign/source/filter/xml/xmlControlProperty
    reportdesign/source/filter/xml/xmlExport
    reportdesign/source/filter/xml/xmlExport2
    reportdesign/source/filter/xml/xmlExportDocumentHandler
    reportdesign/source/filter/xml/xmlfilter
    reportdesign/source/filter/xml/xmlFixedContent
    reportdesign/source/filter/xml/xmlFormatCondition
    reportdesign/source/filter/xml/xmlFormattedField
    reportdesign/source/filter/xml/xmlFunction
    reportdesign/source/filter/xml/xmlGroup
    reportdesign/source/filter/xml/xmlHelper
    reportdesign/source/filter/xml/xmlImage
    reportdesign/source/filter/xml/xmlImportDocumentHandler
    reportdesign/source/filter/xml/xmlMasterFields
    reportdesign/source/filter/xml/xmlPropertyHandler
    reportdesign/source/filter/xml/xmlReport
    reportdesign/source/filter/xml/xmlReportElement
    reportdesign/source/filter/xml/xmlReportElementBase
    reportdesign/source/filter/xml/xmlSection
    reportdesign/source/filter/xml/xmlStyleImport
    reportdesign/source/filter/xml/xmlSubDocument
    reportdesign/source/filter/xml/xmlTable
)

librestudio_expand_cxx(RPT_SOURCES)
add_library(rpt SHARED ${RPT_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(rpt PRIVATE
        "${CMAKE_SOURCE_DIR}/reportdesign/inc/pch/precompiled_rpt.hxx")
    target_compile_definitions(rpt PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(rpt PRIVATE
    "${CMAKE_SOURCE_DIR}/reportdesign/inc"
    "${CMAKE_SOURCE_DIR}/reportdesign/source/inc"
    "${CMAKE_SOURCE_DIR}/reportdesign/source/core/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(rpt officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(rpt PRIVATE REPORTDESIGN_DLLIMPLEMENTATION)

target_link_libraries(rpt PRIVATE
    comphelper cppu cppuhelper dbtools dbu editeng fwk i18nlangtag sal
    salhelper sax sfx svl svt svxcore svx tk tl utl vcl xo
    Boost::boost)

librestudio_add_component(rpt
    COMPONENT "${CMAKE_SOURCE_DIR}/reportdesign/util/rpt.component"
    LIBRARY   rpt
    LAYER     OOO)

librestudio_set_library_output_name(rpt rpt OOO)

install(TARGETS rpt LIBRARY DESTINATION program)

# --- rptui --------------------------------------------------------------

set(RPTUI_SOURCES
    reportdesign/source/ui/dlg/AddField
    reportdesign/source/ui/dlg/CondFormat
    reportdesign/source/ui/dlg/Condition
    reportdesign/source/ui/dlg/DateTime
    reportdesign/source/ui/dlg/dlgpage
    reportdesign/source/ui/dlg/Formula
    reportdesign/source/ui/dlg/GroupExchange
    reportdesign/source/ui/dlg/GroupsSorting
    reportdesign/source/ui/dlg/Navigator
    reportdesign/source/ui/dlg/PageNumber
    reportdesign/source/ui/inspection/DataProviderHandler
    reportdesign/source/ui/inspection/DefaultInspection
    reportdesign/source/ui/inspection/GeometryHandler
    reportdesign/source/ui/inspection/metadata
    reportdesign/source/ui/inspection/ReportComponentHandler
    reportdesign/source/ui/misc/ColorListener
    reportdesign/source/ui/misc/FunctionHelper
    reportdesign/source/ui/misc/RptUndo
    reportdesign/source/ui/misc/statusbarcontroller
    reportdesign/source/ui/misc/UITools
    reportdesign/source/ui/report/DesignView
    reportdesign/source/ui/report/dlgedclip
    reportdesign/source/ui/report/dlgedfac
    reportdesign/source/ui/report/dlgedfunc
    reportdesign/source/ui/report/EndMarker
    reportdesign/source/ui/report/FixedTextColor
    reportdesign/source/ui/report/FormattedFieldBeautifier
    reportdesign/source/ui/report/propbrw
    reportdesign/source/ui/report/ReportController
    reportdesign/source/ui/report/ReportControllerObserver
    reportdesign/source/ui/report/ReportSection
    reportdesign/source/ui/report/ReportWindow
    reportdesign/source/ui/report/ScrollHelper
    reportdesign/source/ui/report/SectionView
    reportdesign/source/ui/report/SectionWindow
    reportdesign/source/ui/report/StartMarker
    reportdesign/source/ui/report/ViewsWindow
)

librestudio_expand_cxx(RPTUI_SOURCES)
add_library(rptui SHARED ${RPTUI_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(rptui PRIVATE
        "${CMAKE_SOURCE_DIR}/reportdesign/inc/pch/precompiled_rptui.hxx")
    target_compile_definitions(rptui PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(rptui PRIVATE
    "${CMAKE_SOURCE_DIR}/reportdesign/inc"
    "${CMAKE_SOURCE_DIR}/reportdesign/source/inc"
    "${CMAKE_SOURCE_DIR}/reportdesign/source/ui/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(rptui udkapi_normal_headers offapi_normal_headers)

target_link_libraries(rptui PRIVATE
    comphelper cppu cppuhelper dbtools dbu editeng for forui fwk i18nlangtag
    rpt sal salhelper sfx sot svl svt svxcore svx tk tl utl vcl basegfx
    Boost::boost)

librestudio_add_component(rptui
    COMPONENT "${CMAKE_SOURCE_DIR}/reportdesign/util/rptui.component"
    LIBRARY   rptui
    LAYER     OOO)

librestudio_set_library_output_name(rptui rptui OOO)

install(TARGETS rptui LIBRARY DESTINATION program)

# --- UIConfig_dbreport ---------------------------------------------------
#
# UIConfig_dbreport.mk: gb_UIConfig_UIConfig(modules/dbreport) -- 1
# menubarfiles, 1 popupmenufiles, 1 statusbarfiles, 15 toolbarfiles and 12
# uifiles, all under reportdesign/uiconfig/dbreport/, packaged into
# share/config/soffice.cfg/modules/dbreport/{menubar,popupmenu,statusbar,
# toolbar,ui} -- same gb_UIConfig_DIR_FOR-derived install(FILES ...)
# mechanism as UIConfig_svt (svtools/CMakeLists.txt), extended to the four
# extra file categories the same way basctl/CMakeLists.txt's own UIConfig_
# basicide block just did. Unlike UIConfig_basicide, this one is NOT gated:
# Module_reportdesign.mk's gb_Module_add_targets call lists Library_rpt,
# Library_rptui and UIConfig_dbreport together in one unconditional block (no
# surrounding BUILD_TYPE/OS filter at all), so it is unconditional here too.
#
# CppunitTest_reportdesign_dialogs_test.mk's own gb_CppunitTest_use_uiconfigs(
# reportdesign_dialogs_test,modules/dbreport) call (reportdesign/CppunitTest_
# deferred.cmake) needs a real build-time ALL target to depend on, the same
# reason UIConfig_basicide grew "basicide_uiconfig" just above -- introduced
# here as "dbreport_uiconfig" (this UIConfig's own gbuild leaf name;
# "modules/dbreport" itself is not a legal CMake target name).
set(REPORTDESIGN_UICONFIG_MENUBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/menubar/menubar.xml"
)
set(REPORTDESIGN_UICONFIG_POPUPMENU_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/popupmenu/report.xml"
)
set(REPORTDESIGN_UICONFIG_STATUSBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/statusbar/statusbar.xml"
)
set(REPORTDESIGN_UICONFIG_TOOLBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/toolbar/alignmentbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/toolbar/arrowshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/toolbar/basicshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/toolbar/calloutshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/toolbar/distributebar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/toolbar/drawbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/toolbar/flowchartshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/toolbar/Formatting.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/toolbar/reportcontrols.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/toolbar/resizebar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/toolbar/sectionalignmentbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/toolbar/sectionshrinkbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/toolbar/starshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/toolbar/symbolshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/toolbar/toolbar.xml"
)
set(REPORTDESIGN_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/ui/backgrounddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/ui/chardialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/ui/condformatdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/ui/conditionwin.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/ui/datetimedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/ui/floatingfield.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/ui/floatingnavigator.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/ui/floatingsort.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/ui/groupsortmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/ui/navigatormenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/ui/pagedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/dbreport/ui/pagenumberdialog.ui"
)

install(FILES ${REPORTDESIGN_UICONFIG_MENUBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/dbreport/menubar)
install(FILES ${REPORTDESIGN_UICONFIG_POPUPMENU_FILES}
    DESTINATION share/config/soffice.cfg/modules/dbreport/popupmenu)
install(FILES ${REPORTDESIGN_UICONFIG_STATUSBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/dbreport/statusbar)
install(FILES ${REPORTDESIGN_UICONFIG_TOOLBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/dbreport/toolbar)
install(FILES ${REPORTDESIGN_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/modules/dbreport/ui)

# Real build-time mirror of the install() rules above, alongside them (not
# instead of them) -- same shape as basctl/CMakeLists.txt's own basicide_
# uiconfig block just introduced; see that file's comment for the full
# reasoning this is not repeated here.
set(_dbreport_uiconfig_outputs "")
foreach (_dbreport_uiconfig_pair
        "REPORTDESIGN_UICONFIG_MENUBAR_FILES:menubar"
        "REPORTDESIGN_UICONFIG_POPUPMENU_FILES:popupmenu"
        "REPORTDESIGN_UICONFIG_STATUSBAR_FILES:statusbar"
        "REPORTDESIGN_UICONFIG_TOOLBAR_FILES:toolbar"
        "REPORTDESIGN_UICONFIG_UI_FILES:ui")
    string(REPLACE ":" ";" _dbreport_uiconfig_pair "${_dbreport_uiconfig_pair}")
    list(GET _dbreport_uiconfig_pair 0 _dbreport_uiconfig_var)
    list(GET _dbreport_uiconfig_pair 1 _dbreport_uiconfig_category)
    foreach (_dbreport_uiconfig_src IN LISTS ${_dbreport_uiconfig_var})
        get_filename_component(_dbreport_uiconfig_name "${_dbreport_uiconfig_src}" NAME)
        set(_dbreport_uiconfig_dest
            "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/dbreport/${_dbreport_uiconfig_category}/${_dbreport_uiconfig_name}")
        add_custom_command(
            OUTPUT "${_dbreport_uiconfig_dest}"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/dbreport/${_dbreport_uiconfig_category}"
            COMMAND ${CMAKE_COMMAND} -E copy "${_dbreport_uiconfig_src}" "${_dbreport_uiconfig_dest}"
            DEPENDS "${_dbreport_uiconfig_src}"
            COMMENT "Copying ${_dbreport_uiconfig_name} into build-tree soffice.cfg/modules/dbreport/${_dbreport_uiconfig_category}"
            VERBATIM)
        list(APPEND _dbreport_uiconfig_outputs "${_dbreport_uiconfig_dest}")
    endforeach ()
endforeach ()
unset(_dbreport_uiconfig_pair)
unset(_dbreport_uiconfig_var)
unset(_dbreport_uiconfig_category)
unset(_dbreport_uiconfig_src)
unset(_dbreport_uiconfig_name)
unset(_dbreport_uiconfig_dest)

add_custom_target(dbreport_uiconfig ALL DEPENDS ${_dbreport_uiconfig_outputs})
unset(_dbreport_uiconfig_outputs)

# AllLangMoTarget_rpt (l10n .po -> .mo): not ported, matching this port's
# established blanket exclusion of every AllLangMoTarget_* (see e.g. uui/
# CMakeLists.txt, desktop/CMakeLists.txt's own notes on their own module's
# AllLangMoTarget).

# --- CppunitTest_reportdesign_basic_test: documented gap, not an oversight --
#
# Module_reportdesign.mk gates this test behind "ifeq ($(ENABLE_JAVA),TRUE)"
# -- unlike ENABLE_SCRIPTING/ENABLE_DBCONNECTIVITY/ENABLE_XMLHELP, this port's
# own LIBRESTUDIO_ENABLE_JAVA option (root CMakeLists.txt) defaults OFF (it
# "Requires a JDK", not assumed present), so a bare configure of this port
# does not build this test either, matching upstream's own bare-configure
# default (ENABLE_JAVA is not one of configure.ac's "yes unless disabled"
# options). Even with LIBRESTUDIO_ENABLE_JAVA turned on, the test would still
# not build here: CppunitTest_reportdesign_basic_test.mk calls gb_CppunitTest_
# use_jars(reportdesign_basic_test, sdbc_hsqldb), and connectivity/
# CMakeLists.txt's own header comment already documents Jar_sdbc_hsqldb (and
# every other SDBC driver besides calc/dbase/flat) as out of scope for this
# port -- there is no sdbc_hsqldb JAR target here to depend on. Left
# unwired for both reasons, the same documented-gap posture as connectivity's
# own Configuration_calc/dbase/flat notes and this module's own AllLangMoTarget_
# rpt note just above.
