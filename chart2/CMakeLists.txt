# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# chart2 — chart2api is a tiny abstract-interface library (a single
# translation unit) with no component file and no dependencies beyond the
# SDK API, existing so that other modules (xmloff) can implement pivot table
# data providers without linking the full chart2 UI/model library. That full
# library (chart2 itself) is also built, further below in this file --
# see its own header comment there for the source-list breakdown.

set(CHART2API_SOURCES
    chart2/source/api/AbstractPivotTableDataProvider
)

librestudio_expand_cxx(CHART2API_SOURCES)
add_library(chart2api SHARED ${CHART2API_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(chart2api PRIVATE
        "${CMAKE_SOURCE_DIR}/chart2/inc/pch/precompiled_chart2api.hxx")
    target_compile_definitions(chart2api PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(chart2api PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(chart2api udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(chart2api PRIVATE CHART2API_DLLIMPLEMENTATION)

librestudio_set_library_output_name(chart2api chart2api OOO)
install(TARGETS chart2api LIBRARY DESTINATION program)

# chart2 — the full chart engine: UNO service implementations (ChartModel,
# Diagram, Axis, the chart-type family, regression curves, ...), the
# controller (dialogs, sidebar panels, item-set converters, the API-facing
# chartapiwrapper), and the view layer that renders a chart onto a
# drawinglayer-backed page. Source list from Library_chart2.mk.

set(CHART2_SOURCES
    # ifneq ($(ENABLE_WASM_STRIP_ACCESSIBILITY),TRUE) -- applies, the variable
    # is unset (0) in this build.
    chart2/source/controller/accessibility/AccessibleBase
    chart2/source/controller/accessibility/AccessibleChartElement
    chart2/source/controller/accessibility/AccessibleChartShape
    chart2/source/controller/accessibility/AccessibleChartView
    chart2/source/controller/accessibility/AccessibleTextHelper
    chart2/source/controller/accessibility/AccessibleViewForwarder
    chart2/source/controller/accessibility/ChartElementFactory

    chart2/source/controller/chartapiwrapper/AreaWrapper
    chart2/source/controller/chartapiwrapper/AxisWrapper
    chart2/source/controller/chartapiwrapper/Chart2ModelContact
    chart2/source/controller/chartapiwrapper/ChartDataWrapper
    chart2/source/controller/chartapiwrapper/ChartDocumentWrapper
    chart2/source/controller/chartapiwrapper/DataSeriesPointWrapper
    chart2/source/controller/chartapiwrapper/DiagramWrapper
    chart2/source/controller/chartapiwrapper/GridWrapper
    chart2/source/controller/chartapiwrapper/LegendWrapper
    chart2/source/controller/chartapiwrapper/MinMaxLineWrapper
    chart2/source/controller/chartapiwrapper/TitleWrapper
    chart2/source/controller/chartapiwrapper/UpDownBarWrapper
    chart2/source/controller/chartapiwrapper/WallFloorWrapper
    chart2/source/controller/chartapiwrapper/WrappedAddInProperty
    chart2/source/controller/chartapiwrapper/WrappedAutomaticPositionProperties
    chart2/source/controller/chartapiwrapper/WrappedAxisAndGridExistenceProperties
    chart2/source/controller/chartapiwrapper/WrappedCharacterHeightProperty
    chart2/source/controller/chartapiwrapper/WrappedDataCaptionProperties
    chart2/source/controller/chartapiwrapper/WrappedGapwidthProperty
    chart2/source/controller/chartapiwrapper/WrappedNumberFormatProperty
    chart2/source/controller/chartapiwrapper/WrappedScaleProperty
    chart2/source/controller/chartapiwrapper/WrappedScaleTextProperties
    chart2/source/controller/chartapiwrapper/WrappedSceneProperty
    chart2/source/controller/chartapiwrapper/WrappedSeriesAreaOrLineProperty
    chart2/source/controller/chartapiwrapper/WrappedSplineProperties
    chart2/source/controller/chartapiwrapper/WrappedStatisticProperties
    chart2/source/controller/chartapiwrapper/WrappedStockProperties
    chart2/source/controller/chartapiwrapper/WrappedSymbolProperties
    chart2/source/controller/chartapiwrapper/WrappedTextRotationProperty
    chart2/source/controller/dialogs/ChangingResource
    chart2/source/controller/dialogs/ChartResourceGroupDlgs
    chart2/source/controller/dialogs/ChartResourceGroups
    chart2/source/controller/dialogs/ChartTypeDialogController
    chart2/source/controller/dialogs/DataBrowser
    chart2/source/controller/dialogs/dlg_ChartType
    chart2/source/controller/dialogs/dlg_ChartType_UNO
    chart2/source/controller/dialogs/dlg_CreationWizard
    chart2/source/controller/dialogs/dlg_CreationWizard_UNO
    chart2/source/controller/dialogs/dlg_DataEditor
    chart2/source/controller/dialogs/dlg_DataSource
    chart2/source/controller/dialogs/dlg_InsertAxis_Grid
    chart2/source/controller/dialogs/dlg_InsertDataLabel
    chart2/source/controller/dialogs/dlg_InsertDataTable
    chart2/source/controller/dialogs/dlg_InsertErrorBars
    chart2/source/controller/dialogs/dlg_InsertLegend
    chart2/source/controller/dialogs/dlg_InsertTitle
    chart2/source/controller/dialogs/dlg_NumberFormat
    chart2/source/controller/dialogs/dlg_ObjectProperties
    chart2/source/controller/dialogs/dlg_ShapeFont
    chart2/source/controller/dialogs/dlg_ShapeParagraph
    chart2/source/controller/dialogs/dlg_View3D
    chart2/source/controller/dialogs/ObjectNameProvider
    chart2/source/controller/dialogs/res_BarGeometry
    chart2/source/controller/dialogs/res_DataLabel
    chart2/source/controller/dialogs/res_DataTableProperties
    chart2/source/controller/dialogs/res_ErrorBar
    chart2/source/controller/dialogs/res_LegendPosition
    chart2/source/controller/dialogs/res_Titles
    chart2/source/controller/dialogs/res_Trendline
    chart2/source/controller/dialogs/TextDirectionListBox
    chart2/source/controller/dialogs/TitleDialogData
    chart2/source/controller/dialogs/tp_3D_SceneAppearance
    chart2/source/controller/dialogs/tp_3D_SceneGeometry
    chart2/source/controller/dialogs/tp_3D_SceneIllumination
    chart2/source/controller/dialogs/tp_AxisLabel
    chart2/source/controller/dialogs/tp_AxisPositions
    chart2/source/controller/dialogs/tp_ChartColorPalette
    chart2/source/controller/dialogs/tp_ChartType
    chart2/source/controller/dialogs/tp_DataLabel
    chart2/source/controller/dialogs/tp_DataPointOption
    chart2/source/controller/dialogs/tp_DataSource
    chart2/source/controller/dialogs/tp_DataTable
    chart2/source/controller/dialogs/tp_ErrorBars
    chart2/source/controller/dialogs/tp_LegendPosition
    chart2/source/controller/dialogs/tp_PointGeometry
    chart2/source/controller/dialogs/tp_PolarOptions
    chart2/source/controller/dialogs/tp_RangeChooser
    chart2/source/controller/dialogs/tp_Scale
    chart2/source/controller/dialogs/tp_SeriesToAxis
    chart2/source/controller/dialogs/tp_TitleRotation
    chart2/source/controller/dialogs/tp_Trendline
    chart2/source/controller/dialogs/tp_Wizard_TitlesAndObjects
    chart2/source/controller/drawinglayer/DrawViewWrapper
    chart2/source/controller/drawinglayer/ViewElementListProvider
    chart2/source/controller/itemsetwrapper/AxisItemConverter
    chart2/source/controller/itemsetwrapper/CharacterPropertyItemConverter
    chart2/source/controller/itemsetwrapper/DataPointItemConverter
    chart2/source/controller/itemsetwrapper/DataTableItemConverter
    chart2/source/controller/itemsetwrapper/ErrorBarItemConverter
    chart2/source/controller/itemsetwrapper/GraphicPropertyItemConverter
    chart2/source/controller/itemsetwrapper/ItemConverter
    chart2/source/controller/itemsetwrapper/LegendItemConverter
    chart2/source/controller/itemsetwrapper/MultipleChartConverters
    chart2/source/controller/itemsetwrapper/MultipleItemConverter
    chart2/source/controller/itemsetwrapper/RegressionCurveItemConverter
    chart2/source/controller/itemsetwrapper/RegressionEquationItemConverter
    chart2/source/controller/itemsetwrapper/SeriesOptionsItemConverter
    chart2/source/controller/itemsetwrapper/StatisticsItemConverter
    chart2/source/controller/itemsetwrapper/TextLabelItemConverter
    chart2/source/controller/itemsetwrapper/TitleItemConverter
    chart2/source/controller/main/ChartController
    chart2/source/controller/main/ChartController_EditData
    chart2/source/controller/main/ChartController_Insert
    chart2/source/controller/main/ChartController_Position
    chart2/source/controller/main/ChartController_Properties
    chart2/source/controller/main/ChartController_TextEdit
    chart2/source/controller/main/ChartController_Tools
    chart2/source/controller/main/ChartController_Window
    chart2/source/controller/main/ChartDropTargetHelper
    chart2/source/controller/main/ChartFrameloader
    chart2/source/controller/main/ChartModelClone
    chart2/source/controller/main/ChartTransferable
    chart2/source/controller/main/ChartWindow
    chart2/source/controller/main/CommandDispatchContainer
    chart2/source/controller/main/CommandDispatch
    chart2/source/controller/main/ControllerCommandDispatch
    chart2/source/controller/main/DragMethod_Base
    chart2/source/controller/main/DragMethod_PieSegment
    chart2/source/controller/main/DragMethod_RotateDiagram
    chart2/source/controller/main/DrawCommandDispatch
    chart2/source/controller/main/ElementSelector
    chart2/source/controller/main/FeatureCommandDispatchBase
    chart2/source/controller/main/ObjectHierarchy
    chart2/source/controller/main/PositionAndSizeHelper
    chart2/source/controller/main/SelectionHelper
    chart2/source/controller/main/ShapeController
    chart2/source/controller/main/StatusBarCommandDispatch
    chart2/source/controller/main/ToolbarController
    chart2/source/controller/main/UndoActions
    chart2/source/controller/main/UndoCommandDispatch
    chart2/source/controller/main/UndoGuard
    chart2/source/controller/sidebar/Chart2PanelFactory
    chart2/source/controller/sidebar/ChartAreaPanel
    chart2/source/controller/sidebar/ChartColorsPanel
    chart2/source/controller/sidebar/ChartColorPaletteControl
    chart2/source/controller/sidebar/ChartAxisPanel
    chart2/source/controller/sidebar/ChartColorWrapper
    chart2/source/controller/sidebar/ChartElementsPanel
    chart2/source/controller/sidebar/ChartErrorBarPanel
    chart2/source/controller/sidebar/ChartLinePanel
    chart2/source/controller/sidebar/ChartSeriesPanel
    chart2/source/controller/sidebar/ChartSidebarModifyListener
    chart2/source/controller/sidebar/ChartSidebarSelectionListener
    chart2/source/controller/sidebar/ChartTypePanel
    chart2/source/controller/uitest/uiobject

    # view pieces ...
    chart2/source/controller/dialogs/DataBrowserModel
    chart2/source/controller/dialogs/DialogModel
    chart2/source/controller/dialogs/RangeSelectionHelper
    chart2/source/controller/dialogs/RangeSelectionListener
    chart2/source/controller/dialogs/TimerTriggeredControllerLock
    chart2/source/view/axes/DateHelper
    chart2/source/view/axes/DateScaling
    chart2/source/view/axes/MinimumAndMaximumSupplier
    chart2/source/view/axes/ScaleAutomatism
    chart2/source/view/axes/Tickmarks
    chart2/source/view/axes/Tickmarks_Dates
    chart2/source/view/axes/Tickmarks_Equidistant
    chart2/source/view/axes/VAxisBase
    chart2/source/view/axes/VAxisOrGridBase
    chart2/source/view/axes/VAxisProperties
    chart2/source/view/axes/VCartesianAxis
    chart2/source/view/axes/VCartesianCoordinateSystem
    chart2/source/view/axes/VCartesianGrid
    chart2/source/view/axes/VCoordinateSystem
    chart2/source/view/axes/VPolarAngleAxis
    chart2/source/view/axes/VPolarAxis
    chart2/source/view/axes/VPolarCoordinateSystem
    chart2/source/view/axes/VPolarGrid
    chart2/source/view/axes/VPolarRadiusAxis
    chart2/source/view/charttypes/AreaChart
    chart2/source/view/charttypes/BarChart
    chart2/source/view/charttypes/HistogramChart
    chart2/source/view/charttypes/BarPositionHelper
    chart2/source/view/charttypes/BubbleChart
    chart2/source/view/charttypes/CandleStickChart
    chart2/source/view/charttypes/CorrelationCircleChart
    chart2/source/view/charttypes/CategoryPositionHelper
    chart2/source/view/charttypes/FunnelChart
    chart2/source/view/charttypes/NetChart
    chart2/source/view/charttypes/PieChart
    chart2/source/view/charttypes/UnsupportedChart
    chart2/source/view/charttypes/Splines
    chart2/source/view/charttypes/VSeriesPlotter
    chart2/source/view/diagram/VDiagram
    chart2/source/view/main/ChartItemPool
    chart2/source/view/main/ChartView
    chart2/source/view/main/Clipping
    chart2/source/view/main/DataPointSymbolSupplier
    chart2/source/view/main/DataTableView
    chart2/source/view/main/DrawModelWrapper
    chart2/source/view/main/LabelPositionHelper
    chart2/source/view/main/Linear3DTransformation
    chart2/source/view/main/PlotterBase
    chart2/source/view/main/PlottingPositionHelper
    chart2/source/view/main/PolarLabelPositionHelper
    chart2/source/view/main/PropertyMapper
    chart2/source/view/main/SeriesPlotterContainer
    chart2/source/view/main/ShapeFactory
    chart2/source/view/main/Stripe
    chart2/source/view/main/VDataSeries
    chart2/source/view/main/VLegend
    chart2/source/view/main/VLegendSymbolFactory
    chart2/source/view/main/VLineProperties
    chart2/source/view/main/VPolarTransformation
    chart2/source/view/main/VTitle
    chart2/source/view/main/VButton

    # model pieces ...
    chart2/source/model/filter/XMLFilter
    chart2/source/model/main/Axis
    chart2/source/model/main/BaseCoordinateSystem
    chart2/source/model/main/CartesianCoordinateSystem
    chart2/source/model/main/ChartModel
    chart2/source/model/main/ChartModel_Persistence
    chart2/source/model/main/DataPoint
    chart2/source/model/main/DataPointProperties
    chart2/source/model/main/DataSeries
    chart2/source/model/main/DataSeriesProperties
    chart2/source/model/main/DataTable
    chart2/source/model/main/Diagram
    chart2/source/model/main/FormattedString
    chart2/source/model/main/GridProperties
    chart2/source/model/main/Legend
    chart2/source/model/main/PageBackground
    chart2/source/model/main/PolarCoordinateSystem
    chart2/source/model/main/StockBar
    chart2/source/model/main/Title
    chart2/source/model/main/UndoManager
    chart2/source/model/main/Wall
    chart2/source/model/template/AreaChartType
    chart2/source/model/template/AreaChartTypeTemplate
    chart2/source/model/template/BarChartType
    chart2/source/model/template/BarChartTypeTemplate
    chart2/source/model/template/HistogramChartType
    chart2/source/model/template/HistogramChartTypeTemplate
    chart2/source/model/template/HistogramDataInterpreter
    chart2/source/model/template/HistogramCalculator
    chart2/source/model/template/BoxWhiskerChartType
    chart2/source/model/template/BoxWhiskerChartTypeTemplate
    chart2/source/model/template/BubbleChartType
    chart2/source/model/template/BubbleChartTypeTemplate
    chart2/source/model/template/BubbleDataInterpreter
    chart2/source/model/template/CandleStickChartType
    chart2/source/model/template/ChartType
    chart2/source/model/template/ChartTypeManager
    chart2/source/model/template/ChartTypeTemplate
    chart2/source/model/template/ClusteredColumnChartType
    chart2/source/model/template/ClusteredColumnChartTypeTemplate
    chart2/source/model/template/ColumnChartType
    chart2/source/model/template/ColumnLineChartTypeTemplate
    chart2/source/model/template/ColumnLineDataInterpreter
    chart2/source/model/template/CorrelationCircleChartType
    chart2/source/model/template/CorrelationCircleChartTypeTemplate
    chart2/source/model/template/CorrelationCircleDataInterpreter
    chart2/source/model/template/DataInterpreter
    chart2/source/model/template/FilledNetChartType
    chart2/source/model/template/FunnelChartType
    chart2/source/model/template/FunnelChartTypeTemplate
    chart2/source/model/template/LineChartType
    chart2/source/model/template/LineChartTypeTemplate
    chart2/source/model/template/NetChartType
    chart2/source/model/template/NetChartTypeTemplate
    chart2/source/model/template/ParetoLineChartType
    chart2/source/model/template/ParetoLineChartTypeTemplate
    chart2/source/model/template/PieChartType
    chart2/source/model/template/PieChartTypeTemplate
    chart2/source/model/template/RegionMapChartType
    chart2/source/model/template/RegionMapChartTypeTemplate
    chart2/source/model/template/ScatterChartType
    chart2/source/model/template/ScatterChartTypeTemplate
    chart2/source/model/template/SunburstChartType
    chart2/source/model/template/SunburstChartTypeTemplate
    chart2/source/model/template/StockChartTypeTemplate
    chart2/source/model/template/StockDataInterpreter
    chart2/source/model/template/TreemapChartType
    chart2/source/model/template/TreemapChartTypeTemplate
    chart2/source/model/template/WaterfallChartType
    chart2/source/model/template/WaterfallChartTypeTemplate
    chart2/source/model/template/XYDataInterpreter

    # tools pieces
    chart2/source/tools/AxisHelper
    chart2/source/tools/BaseColorScheme
    chart2/source/tools/BaseGFXHelper
    chart2/source/tools/CachedDataSequence
    chart2/source/tools/CharacterProperties
    chart2/source/tools/ChartColorPalettes
    chart2/source/tools/ChartColorPaletteHelper
    chart2/source/tools/ChartColorScheme
    chart2/source/tools/ChartTypeHelper
    chart2/source/tools/ChartViewHelper
    chart2/source/tools/CommonConverters
    chart2/source/tools/ConfigColorScheme
    chart2/source/tools/ControllerLockGuard
    chart2/source/tools/DataSeriesHelper
    chart2/source/tools/DataSource
    chart2/source/tools/DataSourceHelper
    chart2/source/tools/DiagramHelper
    chart2/source/tools/ErrorBar
    chart2/source/tools/ExplicitCategoriesProvider
    chart2/source/tools/ExponentialRegressionCurveCalculator
    chart2/source/tools/FillProperties
    chart2/source/tools/FormattedStringHelper
    chart2/source/tools/HistogramDataSequence
    chart2/source/tools/InternalData
    chart2/source/tools/InternalDataProvider
    chart2/source/tools/LabeledDataSequence
    chart2/source/tools/LegendHelper
    chart2/source/tools/LifeTime
    chart2/source/tools/LinearRegressionCurveCalculator
    chart2/source/tools/LinePropertiesHelper
    chart2/source/tools/LogarithmicRegressionCurveCalculator
    chart2/source/tools/MeanValueRegressionCurveCalculator
    chart2/source/tools/MediaDescriptorHelper
    chart2/source/tools/ModifyListenerCallBack
    chart2/source/tools/ModifyListenerHelper
    chart2/source/tools/MovingAverageRegressionCurveCalculator
    chart2/source/tools/NameContainer
    chart2/source/tools/NumberFormatterWrapper
    chart2/source/tools/ObjectIdentifier
    chart2/source/tools/OPropertySet
    chart2/source/tools/PolynomialRegressionCurveCalculator
    chart2/source/tools/PopupRequest
    chart2/source/tools/PotentialRegressionCurveCalculator
    chart2/source/tools/PropertyHelper
    chart2/source/tools/RangeHighlighter
    chart2/source/tools/ReferenceSizeProvider
    chart2/source/tools/RegressionCurveCalculator
    chart2/source/tools/RegressionCurveHelper
    chart2/source/tools/RegressionCurveModel
    chart2/source/tools/RegressionEquation
    chart2/source/tools/RelativePositionHelper
    chart2/source/tools/RelativeSizeHelper
    chart2/source/tools/ResId
    chart2/source/tools/Scaling
    chart2/source/tools/SceneProperties
    chart2/source/tools/StatisticsHelper
    chart2/source/tools/ThreeDHelper
    chart2/source/tools/TitleHelper
    chart2/source/tools/UncachedDataSequence
    chart2/source/tools/UserDefinedProperties
    chart2/source/tools/WeakListenerAdapter
    chart2/source/tools/WrappedDefaultProperty
    chart2/source/tools/WrappedDirectStateProperty
    chart2/source/tools/WrappedIgnoreProperty
    chart2/source/tools/WrappedProperty
    chart2/source/tools/WrappedPropertySet
    chart2/source/tools/XMLRangeHelper
)

librestudio_expand_cxx(CHART2_SOURCES)
add_library(chart2 SHARED ${CHART2_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(chart2 PRIVATE
        "${CMAKE_SOURCE_DIR}/chart2/inc/pch/precompiled_chart2.hxx")
    target_compile_definitions(chart2 PRIVATE PCH_LEVEL=2)
endif ()

# gb_Library_use_custom_headers(chart2, officecfg/registry) — chart2 reads
# configuration through the generated officecfg:: types.
target_include_directories(chart2 PRIVATE
    "${CMAKE_SOURCE_DIR}/chart2/source/controller/inc"
    "${CMAKE_SOURCE_DIR}/chart2/source/model/inc"
    "${CMAKE_SOURCE_DIR}/chart2/source/view/inc"
    "${CMAKE_SOURCE_DIR}/chart2/source/inc"
    "${CMAKE_SOURCE_DIR}/chart2/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}")
add_dependencies(chart2 officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(chart2 PRIVATE
    basegfx chart2api comphelper cppu cppuhelper drawinglayer docmodel editeng
    fwk sal salhelper i18nlangtag sfx sot svl svt svxcore svx tk tl ucbhelper
    utl vcl
    Boost::boost
    LibXml2::LibXml2)

# No <optional/> implementations in chart2/source/chart2.component, so no
# IMPLEMENTATION filtering is needed here (unlike svx/svxcore).
librestudio_add_component(chart2
    COMPONENT "${CMAKE_SOURCE_DIR}/chart2/source/chart2.component"
    LIBRARY   chart2
    LAYER     OOO)

librestudio_set_library_output_name(chart2 chart2 OOO)

install(TARGETS chart2 LIBRARY DESTINATION program)
# --- UIConfig_chart2 (registers internally as "modules/schart") -----------
#
# chart2/UIConfig_chart2.mk: gb_UIConfig_UIConfig(modules/schart) -- 59 files
# across 5 categories (1 menubarfiles, 2 popupmenufiles, 1 statusbarfiles, 9
# toolbarfiles, 46 uifiles), unconditional (no ifeq/ifneq gates in the live .mk
# -- verified by reading it in full), packaged into share/config/soffice.cfg/
# modules/schart/{menubar,popupmenu,statusbar,toolbar,ui} -- same multi-
# category pattern UIConfig_basicide/UIConfig_svt/UIConfig_swriter/UIConfig_
# smath already established. All 59 files verified present on disk.
# CppunitTest_chart2_dialogs_test.mk's own use_uiconfigs(chart2_dialogs_test,
# "modules/schart", svx) needs a real build-time ALL target for this module --
# introduced here as "schart_uiconfig" (this port's own bare-leaf-name
# convention for these targets -- "schart" is the .mk's own leaf name under
# "modules/", same convention starmath's "smath"/sw's "swriter" already use --
# not gbuild's own "modules/schart" path string).
set(CHART2_UICONFIG_MENUBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/menubar/menubar.xml"
)

set(CHART2_UICONFIG_POPUPMENU_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/popupmenu/draw.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/popupmenu/drawtext.xml"
)

set(CHART2_UICONFIG_STATUSBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/statusbar/statusbar.xml"
)

set(CHART2_UICONFIG_TOOLBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/toolbar/arrowshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/toolbar/basicshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/toolbar/calloutshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/toolbar/drawbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/toolbar/flowchartshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/toolbar/standardbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/toolbar/starshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/toolbar/symbolshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/toolbar/toolbar.xml"
)

set(CHART2_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/3dviewdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/attributedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/chartcolorpalettepopup.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/chardialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/chartdatadialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/charttypedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/columnfragment.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/combobox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/datarangedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dlg_DataLabel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dlg_InsertDataTable.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dlg_InsertErrorBars.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dlg_InsertLegend.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/imagefragment.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/insertaxisdlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/insertgriddlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/inserttitledlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/paradialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebaraxis.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebarcolors.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebarelements.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebarerrorbar.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebarseries.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebartype.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/smoothlinesdlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/steppedlinesdlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/titlerotationtabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tp_3D_SceneAppearance.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tp_3D_SceneGeometry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tp_3D_SceneIllumination.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tp_axisLabel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tp_AxisPositions.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tp_ChartColorPalette.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tp_ChartType.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tp_DataLabel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tp_DataPointOption.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tp_DataSource.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tp_DataTable.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tp_ErrorBars.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tp_LegendPosition.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tp_PolarOptions.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tp_RangeChooser.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tp_SeriesToAxis.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tp_Scale.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tp_Trendline.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/wizelementspage.ui"
)

install(FILES ${CHART2_UICONFIG_MENUBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/schart/menubar)
install(FILES ${CHART2_UICONFIG_POPUPMENU_FILES}
    DESTINATION share/config/soffice.cfg/modules/schart/popupmenu)
install(FILES ${CHART2_UICONFIG_STATUSBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/schart/statusbar)
install(FILES ${CHART2_UICONFIG_TOOLBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/schart/toolbar)
install(FILES ${CHART2_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/modules/schart/ui)

set(_schart_uiconfig_outputs "")
foreach (_schart_uiconfig_pair
        "CHART2_UICONFIG_MENUBAR_FILES:menubar"
        "CHART2_UICONFIG_POPUPMENU_FILES:popupmenu"
        "CHART2_UICONFIG_STATUSBAR_FILES:statusbar"
        "CHART2_UICONFIG_TOOLBAR_FILES:toolbar"
        "CHART2_UICONFIG_UI_FILES:ui")
    string(REPLACE ":" ";" _schart_uiconfig_pair "${_schart_uiconfig_pair}")
    list(GET _schart_uiconfig_pair 0 _schart_uiconfig_var)
    list(GET _schart_uiconfig_pair 1 _schart_uiconfig_category)
    foreach (_schart_uiconfig_src IN LISTS ${_schart_uiconfig_var})
        get_filename_component(_schart_uiconfig_name "${_schart_uiconfig_src}" NAME)
        set(_schart_uiconfig_dest
            "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/schart/${_schart_uiconfig_category}/${_schart_uiconfig_name}")
        add_custom_command(
            OUTPUT "${_schart_uiconfig_dest}"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/schart/${_schart_uiconfig_category}"
            COMMAND ${CMAKE_COMMAND} -E copy "${_schart_uiconfig_src}" "${_schart_uiconfig_dest}"
            DEPENDS "${_schart_uiconfig_src}"
            COMMENT "Copying ${_schart_uiconfig_name} into build-tree soffice.cfg/modules/schart/${_schart_uiconfig_category}")
        list(APPEND _schart_uiconfig_outputs "${_schart_uiconfig_dest}")
    endforeach ()
endforeach ()
unset(_schart_uiconfig_pair)
unset(_schart_uiconfig_var)
unset(_schart_uiconfig_category)
unset(_schart_uiconfig_src)
unset(_schart_uiconfig_name)
unset(_schart_uiconfig_dest)

add_custom_target(schart_uiconfig ALL DEPENDS ${_schart_uiconfig_outputs})
unset(_schart_uiconfig_outputs)

# --- CppunitTests --------------------------------------------------------------
#
# chart2 has 15 CppunitTest_*.mk files. 2 need no deferral at all and are
# wired directly below (chart2_common_functors, chart2_correlation_
# coefficient); the other 13 need program_services_rdb and/or COMPONENTS/
# DEPENDS whose owning targets sit textually after chart2's own add_
# subdirectory() position, and are deferred to chart2/CppunitTest_deferred.
# cmake: chart2_splines, chart2_uichart, chart2_dialogs_test, chart2_dump,
# chart2_xshape, chart2_export/export2/export3, chart2_import/import2,
# chart2_geometry, chart2_pivot_chart_test, chart2_trendcalculators.
#
# The last 5 (chart2_import/import2/geometry/pivot_chart_test/
# trendcalculators) were blocked, until 2026-09-28, by their own use_
# components lists (chart2/import_setup.mk, CppunitTest_chart2_geometry.mk,
# CppunitTest_chart2_pivot_chart_test.mk, CppunitTest_chart2_
# trendcalculators.mk, all re-checked directly) each naming "filter/source/
# odfflatxml/odfflatxml", "filter/source/xmlfilteradaptor/xmlfa" and
# "filter/source/xmlfilterdetect/xmlfd" -- three filter-module production
# libraries this port had not built yet (the same gap sc_filters_test's own
# COMPONENTS list already documented earlier this session). All three are
# now built (filter/CMakeLists.txt's own odfflatxml/xmlfa/xmlfd wave, same
# day, separate follow-up) -- all 5 are wired below. This was a DIFFERENT
# gap from postprocess_images (also now built -- postprocess/CMakeLists.
# txt's own "CustomTarget_images + Package_images: implemented" section):
# chart2_export/export2/export3 use ONLY use_rdb(services), no use_
# components at all, so postprocess_images alone was blocking them.
# chart2_import/import2/geometry ALSO carry the postprocess_images
# order-only dependency, but odfflatxml/xmlfa/xmlfd was their binding
# constraint, not postprocess_images. chart2_xshape is additionally gated
# `ifeq ($(WITH_FONTS), TRUE)` in Module_chart2.mk -- re-checked against
# configure.ac, WITH_FONTS defaults TRUE (only "no" under an explicit
# --without-fonts), so this was never itself a reason to exclude it (same
# "defaults-on flag this port doesn't track a switch for" reasoning already
# applied to ENABLE_SCRIPTING/ENABLE_DBCONNECTIVITY/ENABLE_XMLHELP).
#
# All 15 CppunitTest_*.mk files chart2 has are now wired -- 2 directly below,
# 13 via chart2/CppunitTest_deferred.cmake. chart2 is fully ported: 15/15.
#
# CppunitTest_chart2_common_functors.mk, re-checked directly: 1 source
# (chart2/qa/unit/common_functor_test), add_defs -DOOO_DLLIMPLEMENTATION_
# CHARTTOOLS, use_externals(boost_headers), use_libraries (cppu cppuhelper
# sal salhelper), set_include (-Ichart2/inc -Ichart2/source/inc $$(INCLUDE)),
# use_sdk_api, use_ure. NO use_vcl call in this .mk (re-verified directly --
# unlike almost every other CppunitTest in this port, this one only calls
# use_ure, not use_ure+use_vcl), so only 2 protector pairs (unobootstrap
# protector, unoexceptionprotector -- no vclbootstrapprotector, since that
# one is added only by use_vcl per gb_CppunitTest_use_vcl's own dependency,
# not by use_ure). No use_rdb/use_configuration/use_components anywhere in
# this .mk either, so this test needs no deferral at all.
librestudio_add_cppunit_test(chart2_common_functors
    SOURCES
        chart2/qa/unit/common_functor_test
    LIBRARIES
        cppu cppuhelper sal salhelper
        Boost::boost
    DEFINITIONS
        OOO_DLLIMPLEMENTATION_CHARTTOOLS
    INCLUDE
        "${CMAKE_SOURCE_DIR}/chart2/inc"
        "${CMAKE_SOURCE_DIR}/chart2/source/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector)

# CppunitTest_chart2_correlation_coefficient.mk, re-checked directly: 1
# source (chart2/qa/unit/correlation_coefficient_test), use_libraries (sal
# only), set_include (-Ichart2/inc -Ichart2/source/inc $$(INCLUDE)). No
# use_sdk_api/use_ure/use_vcl/use_rdb/use_configuration/use_components
# anywhere in this .mk -- the simplest CppunitTest in this port so far (no
# UNO bootstrap, no protectors, no udkapi/offapi include even). Needs no
# deferral.
librestudio_add_cppunit_test(chart2_correlation_coefficient
    SOURCES
        chart2/qa/unit/correlation_coefficient_test
    LIBRARIES
        sal
    INCLUDE
        "${CMAKE_SOURCE_DIR}/chart2/inc"
        "${CMAKE_SOURCE_DIR}/chart2/source/inc")

cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/chart2/CppunitTest_deferred.cmake")
