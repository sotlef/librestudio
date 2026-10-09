# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# xmloff — only the xo library so far: the ODF import/export filter core (core/,
# draw/, forms/, meta/, script/, style/, table/, text/, xforms/, plus chart/
# since ENABLE_WASM_STRIP_CHART is not set in this build). Library_xof.mk (the
# OOo<->OASIS format transformer) is not ported: nothing else in the tree links
# it, it is only ever loaded as a UNO service at runtime, the same reasoning
# that has left mtfrenderer/canvasfactory/vclcanvas unported so far.
#
# xo's sources reach two generated headers, xmloff/token/tokens.hxx and
# tokenhash.inc, that CustomTarget_generated.mk produces from
# xmloff/source/token/tokens.txt via solenv/bin/generate-tokens.py (a Python
# script) and gperf. Neither tool has been needed by this CMake port before.

find_program(GPERF_EXECUTABLE gperf REQUIRED)
find_program(PYTHON3_EXECUTABLE NAMES python3 python REQUIRED)

# CACHE INTERNAL (not a plain set()), same reason as oox/CMakeLists.txt's own
# OOX_GENERATED_DIR: xmloff/CppunitTest_deferred.cmake's own
# librestudio_add_cppunit_test(xmloff_uxmloff ...) call (use_custom_headers
# xmloff/generated in CppunitTest_xmloff_uxmloff.mk) runs from the root
# CMakeLists.txt's own include()'d scope, not this directory's, so a plain
# set() here would not be visible there -- see that file's header comment for
# the general "this file runs in the root scope" wrinkle.
set(XMLOFF_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated" CACHE INTERNAL
    "xmloff generated token headers directory")
set(XMLOFF_TOKEN_SRC_DIR "${CMAKE_SOURCE_DIR}/xmloff/source/token")

# generate-tokens.py turns tokens.txt into three files: an .inc of "const
# sal_Int32 XML_foo = N;" declarations (sandwiched between tokens.hxx.head and
# .tail to make the installed header), a name table nothing in this port reads
# yet, and a gperf source that gb_CustomTarget_token_hash then turns into the
# perfect-hash lookup table fasttokenhandler.cxx includes as tokenhash.inc.
set(XMLOFF_TOKENS_HXX "${XMLOFF_GENERATED_DIR}/xmloff/token/tokens.hxx")
add_custom_command(
    OUTPUT "${XMLOFF_TOKENS_HXX}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${XMLOFF_GENERATED_DIR}/misc"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${XMLOFF_GENERATED_DIR}/xmloff/token"
    COMMAND "${PYTHON3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/solenv/bin/generate-tokens.py"
            "${XMLOFF_TOKEN_SRC_DIR}/tokens.txt"
            "${XMLOFF_GENERATED_DIR}/misc/tokenids.inc"
            "${XMLOFF_GENERATED_DIR}/tokennames.inc"
            "${XMLOFF_GENERATED_DIR}/misc/tokenhash.gperf"
    COMMAND ${CMAKE_COMMAND} -E cat
            "${XMLOFF_TOKEN_SRC_DIR}/tokens.hxx.head"
            "${XMLOFF_GENERATED_DIR}/misc/tokenids.inc"
            "${XMLOFF_TOKEN_SRC_DIR}/tokens.hxx.tail" > "${XMLOFF_TOKENS_HXX}"
    DEPENDS "${XMLOFF_TOKEN_SRC_DIR}/tokens.txt"
            "${XMLOFF_TOKEN_SRC_DIR}/tokens.hxx.head"
            "${XMLOFF_TOKEN_SRC_DIR}/tokens.hxx.tail"
            "${CMAKE_SOURCE_DIR}/solenv/bin/generate-tokens.py"
    COMMENT "generate-tokens.py: xmloff/token/tokens.hxx"
    VERBATIM)

# gperf's own output still carries #line directives back to the .gperf source
# and a (char*)0 that needs a trailing ", 0" to type-check as the { name,
# nToken } struct fasttokenhandler.cxx declares -- both fixed up the same way
# ComponentTarget.mk does, with a sed piped from gperf's stdout.
set(XMLOFF_TOKENHASH_INC "${XMLOFF_GENERATED_DIR}/tokenhash.inc")
add_custom_command(
    OUTPUT "${XMLOFF_TOKENHASH_INC}"
    COMMAND sh -c "'${GPERF_EXECUTABLE}' --compare-strncmp --switch=2 --readonly-tables --pic '${XMLOFF_GENERATED_DIR}/misc/tokenhash.gperf' | sed -e '/^#line/d' -e 's/(char\\*)0/(char\\*)0, 0/g' > '${XMLOFF_TOKENHASH_INC}'"
    DEPENDS "${XMLOFF_TOKENS_HXX}"
    COMMENT "gperf: xmloff/generated/tokenhash.inc"
    VERBATIM)

add_custom_target(xmloff_generated DEPENDS "${XMLOFF_TOKENS_HXX}" "${XMLOFF_TOKENHASH_INC}")

# --- libxo ---------------------------------------------------------------------

set(XO_SOURCES
    # ifneq ($(ENABLE_WASM_STRIP_CHART),TRUE) -- applies, the variable is unset.
    xmloff/source/chart/ColorPropertySet
    xmloff/source/chart/PropertyMaps
    xmloff/source/chart/SchXMLAutoStylePoolP
    xmloff/source/chart/SchXMLAxisContext
    xmloff/source/chart/SchXMLCalculationSettingsContext
    xmloff/source/chart/SchXMLChartContext
    xmloff/source/chart/SchXMLDataTableContext
    xmloff/source/chart/SchXMLEnumConverter
    xmloff/source/chart/SchXMLExport
    xmloff/source/chart/SchXMLImport
    xmloff/source/chart/SchXMLLegendContext
    xmloff/source/chart/SchXMLParagraphContext
    xmloff/source/chart/SchXMLPlotAreaContext
    xmloff/source/chart/SchXMLPropertyMappingContext
    xmloff/source/chart/SchXMLRegressionCurveObjectContext
    xmloff/source/chart/SchXMLSeries2Context
    xmloff/source/chart/SchXMLSeriesHelper
    xmloff/source/chart/SchXMLTableContext
    xmloff/source/chart/SchXMLTextListContext
    xmloff/source/chart/SchXMLTools
    xmloff/source/chart/XMLAxisPositionPropertyHdl
    xmloff/source/chart/XMLChartPropertyContext
    xmloff/source/chart/XMLChartStyleContext
    xmloff/source/chart/XMLErrorBarStylePropertyHdl
    xmloff/source/chart/XMLErrorIndicatorPropertyHdl
    xmloff/source/chart/XMLLabelSeparatorContext
    xmloff/source/chart/XMLSymbolImageContext
    xmloff/source/chart/XMLSymbolTypePropertyHdl
    xmloff/source/chart/XMLTextOrientationHdl
    xmloff/source/chart/contexts
    xmloff/source/chart/transporttypes

    xmloff/source/core/DocumentSettingsContext
    xmloff/source/core/DomBuilderContext
    xmloff/source/core/DomExport
    xmloff/source/core/fasttokenhandler
    xmloff/source/core/ProgressBarHelper
    xmloff/source/core/PropertySetMerger
    xmloff/source/core/RDFaExportHelper
    xmloff/source/core/RDFaImportHelper
    xmloff/source/core/SettingsExportHelper
    xmloff/source/core/SvXMLAttr
    xmloff/source/core/SvXMLAttrCollection
    xmloff/source/core/XMLBase64Export
    xmloff/source/core/XMLBase64ImportContext
    xmloff/source/core/XMLBasicExportFilter
    xmloff/source/core/XMLEmbeddedObjectExportFilter
    xmloff/source/core/XMLEmbeddedObjectImportContext
    xmloff/source/core/i18nmap
    xmloff/source/core/namespacemap
    xmloff/source/core/unoatrcn
    xmloff/source/core/unointerfacetouniqueidentifiermapper
    xmloff/source/core/xmlcnimp
    xmloff/source/core/xmlerror
    xmloff/source/core/xmlexp
    xmloff/source/core/xmlgrhlp
    xmloff/source/core/xmlictxt
    xmloff/source/core/xmlimp
    xmloff/source/core/xmltkmap
    xmloff/source/core/xmltoken
    xmloff/source/core/xmluconv
    xmloff/source/core/xmlmultiimagehelper

    xmloff/source/draw/EnhancedCustomShapeToken
    xmloff/source/draw/XMLGraphicsDefaultStyle
    xmloff/source/draw/XMLImageMapContext
    xmloff/source/draw/XMLImageMapExport
    xmloff/source/draw/XMLNumberStyles
    xmloff/source/draw/XMLReplacementImageContext
    xmloff/source/draw/XMLShapePropertySetContext
    xmloff/source/draw/XMLShapeStyleContext
    xmloff/source/draw/animationexport
    xmloff/source/draw/animationimport
    xmloff/source/draw/animexp
    xmloff/source/draw/animimp
    xmloff/source/draw/descriptionimp
    xmloff/source/draw/eventimp
    xmloff/source/draw/layerexp
    xmloff/source/draw/layerimp
    xmloff/source/draw/numithdl
    xmloff/source/draw/propimp0
    xmloff/source/draw/sdpropls
    xmloff/source/draw/sdxmlexp
    xmloff/source/draw/sdxmlimp
    xmloff/source/draw/shapeexport
    xmloff/source/draw/shapeimport
    xmloff/source/draw/SignatureLineContext
    xmloff/source/draw/QRCodeContext
    xmloff/source/draw/xexptran
    xmloff/source/draw/ximp3dobject
    xmloff/source/draw/ximp3dscene
    xmloff/source/draw/ximpbody
    xmloff/source/draw/ximpcustomshape
    xmloff/source/draw/ximpgrp
    xmloff/source/draw/ximplink
    xmloff/source/draw/ximpnote
    xmloff/source/draw/ximppage
    xmloff/source/draw/ximpshap
    xmloff/source/draw/ximpshow
    xmloff/source/draw/ximpstyl

    xmloff/source/forms/controlelement
    xmloff/source/forms/controlpropertyhdl
    xmloff/source/forms/controlpropertymap
    xmloff/source/forms/elementexport
    xmloff/source/forms/elementimport
    xmloff/source/forms/eventexport
    xmloff/source/forms/eventimport
    xmloff/source/forms/formattributes
    xmloff/source/forms/formcellbinding
    xmloff/source/forms/formenums
    xmloff/source/forms/formevents
    xmloff/source/forms/formlayerexport
    xmloff/source/forms/formlayerimport
    xmloff/source/forms/gridcolumnproptranslator
    xmloff/source/forms/layerexport
    xmloff/source/forms/layerimport
    xmloff/source/forms/logging
    xmloff/source/forms/officeforms
    xmloff/source/forms/propertyexport
    xmloff/source/forms/propertyimport
    xmloff/source/forms/property_meta_data
    xmloff/source/forms/valueproperties
    xmloff/source/forms/handler/form_handler_factory
    xmloff/source/forms/handler/vcl_date_handler
    xmloff/source/forms/handler/vcl_time_handler

    xmloff/source/meta/MetaExportComponent
    xmloff/source/meta/MetaImportComponent
    xmloff/source/meta/xmlmetae
    xmloff/source/meta/xmlmetai
    xmloff/source/meta/xmlversion

    xmloff/source/script/XMLEventExport
    xmloff/source/script/XMLEventImportHelper
    xmloff/source/script/XMLEventsImportContext
    xmloff/source/script/XMLScriptContextFactory
    xmloff/source/script/XMLScriptExportHandler
    xmloff/source/script/XMLStarBasicContextFactory
    xmloff/source/script/XMLStarBasicExportHandler
    xmloff/source/script/xmlbasicscript
    xmloff/source/script/xmlscripti

    xmloff/source/style/AttributeContainerHandler
    xmloff/source/style/DashStyle
    xmloff/source/style/DrawAspectHdl
    xmloff/source/style/EnumPropertyHdl
    xmloff/source/style/FillStyleContext
    xmloff/source/style/GradientStyle
    xmloff/source/style/HatchStyle
    xmloff/source/style/ImageStyle
    xmloff/source/style/MarkerStyle
    xmloff/source/style/MultiPropertySetHelper
    xmloff/source/style/NamedBoolPropertyHdl
    xmloff/source/style/PageHeaderFooterContext
    xmloff/source/style/PageMasterExportPropMapper
    xmloff/source/style/PageMasterImportContext
    xmloff/source/style/PageMasterImportPropMapper
    xmloff/source/style/PageMasterPropHdl
    xmloff/source/style/PageMasterPropHdlFactory
    xmloff/source/style/PageMasterPropMapper
    xmloff/source/style/PageMasterStyleMap
    xmloff/source/style/PagePropertySetContext
    xmloff/source/style/SinglePropertySetInfoCache
    xmloff/source/style/StyleMap
    xmloff/source/style/StylePropertiesContext
    xmloff/source/style/TransGradientStyle
    xmloff/source/style/WordWrapPropertyHdl
    xmloff/source/style/XMLBackgroundImageContext
    xmloff/source/style/XMLBackgroundImageExport
    xmloff/source/style/XMLBitmapLogicalSizePropertyHandler
    xmloff/source/style/XMLBitmapRepeatOffsetPropertyHandler
    xmloff/source/style/XMLRtlGutterPropertyHandler
    xmloff/source/style/XMLClipPropertyHandler
    xmloff/source/style/XMLConstantsPropertyHandler
    xmloff/source/style/XMLElementPropertyContext
    xmloff/source/style/XMLFillBitmapSizePropertyHandler
    xmloff/source/style/XMLFontAutoStylePool
    xmloff/source/style/XMLFontStylesContext
    xmloff/source/style/XMLFootnoteSeparatorExport
    xmloff/source/style/XMLFootnoteSeparatorImport
    xmloff/source/style/XMLIsPercentagePropertyHandler
    xmloff/source/style/XMLPageExport
    xmloff/source/style/XMLPercentOrMeasurePropertyHandler
    xmloff/source/style/XMLRectangleMembersHandler
    xmloff/source/style/XMLThemeContext
    xmloff/source/style/adjushdl
    xmloff/source/style/backhdl
    xmloff/source/style/bordrhdl
    xmloff/source/style/breakhdl
    xmloff/source/style/cdouthdl
    xmloff/source/style/chrhghdl
    xmloff/source/style/chrlohdl
    xmloff/source/style/csmaphdl
    xmloff/source/style/durationhdl
    xmloff/source/style/escphdl
    xmloff/source/style/fonthdl
    xmloff/source/style/impastpl
    xmloff/source/style/kernihdl
    xmloff/source/style/lspachdl
    xmloff/source/style/numehelp
    xmloff/source/style/opaquhdl
    xmloff/source/style/postuhdl
    xmloff/source/style/prhdlfac
    xmloff/source/style/prstylei
    xmloff/source/style/prstylecond
    xmloff/source/style/shadwhdl
    xmloff/source/style/shdwdhdl
    xmloff/source/style/styleexp
    xmloff/source/style/tabsthdl
    xmloff/source/style/undlihdl
    xmloff/source/style/weighhdl
    xmloff/source/style/xmlaustp
    xmloff/source/style/xmlbahdl
    xmloff/source/style/xmlexppr
    xmloff/source/style/xmlimppr
    xmloff/source/style/xmlnume
    xmloff/source/style/xmlnumfe
    xmloff/source/style/xmlnumfi
    xmloff/source/style/xmlnumi
    xmloff/source/style/xmlprcon
    xmloff/source/style/xmlprhdl
    xmloff/source/style/xmlprmap
    xmloff/source/style/xmlstyle
    xmloff/source/style/xmltabe
    xmloff/source/style/xmltabi

    xmloff/source/table/XMLTableExport
    xmloff/source/table/XMLTableImport

    xmloff/source/text/XMLAutoMarkFileContext
    xmloff/source/text/XMLAutoTextContainerEventImport
    xmloff/source/text/XMLAutoTextEventExport
    xmloff/source/text/XMLAutoTextEventImport
    xmloff/source/text/XMLCalculationSettingsContext
    xmloff/source/text/XMLChangeElementImportContext
    xmloff/source/text/XMLChangeImportContext
    xmloff/source/text/XMLChangeInfoContext
    xmloff/source/text/XMLChangedRegionImportContext
    xmloff/source/text/XMLComplexColorContext
    xmloff/source/text/XMLComplexColorExport
    xmloff/source/text/XMLFootnoteBodyImportContext
    xmloff/source/text/XMLFootnoteConfigurationImportContext
    xmloff/source/text/XMLFootnoteImportContext
    xmloff/source/text/XMLIndexAlphabeticalSourceContext
    xmloff/source/text/XMLIndexBibliographyConfigurationContext
    xmloff/source/text/XMLIndexBibliographyEntryContext
    xmloff/source/text/XMLIndexBibliographySourceContext
    xmloff/source/text/XMLIndexBodyContext
    xmloff/source/text/XMLIndexChapterInfoEntryContext
    xmloff/source/text/XMLIndexIllustrationSourceContext
    xmloff/source/text/XMLIndexMarkExport
    xmloff/source/text/XMLIndexObjectSourceContext
    xmloff/source/text/XMLIndexSimpleEntryContext
    xmloff/source/text/XMLIndexSourceBaseContext
    xmloff/source/text/XMLIndexSpanEntryContext
    xmloff/source/text/XMLIndexTOCContext
    xmloff/source/text/XMLIndexTOCSourceContext
    xmloff/source/text/XMLIndexTOCStylesContext
    xmloff/source/text/XMLIndexTabStopEntryContext
    xmloff/source/text/XMLIndexTableSourceContext
    xmloff/source/text/XMLIndexTemplateContext
    xmloff/source/text/XMLIndexTitleTemplateContext
    xmloff/source/text/XMLIndexUserSourceContext
    xmloff/source/text/XMLLineNumberingExport
    xmloff/source/text/XMLLineNumberingImportContext
    xmloff/source/text/XMLLineNumberingSeparatorImportContext
    xmloff/source/text/XMLPropertyBackpatcher
    xmloff/source/text/XMLRedlineExport
    xmloff/source/text/XMLSectionExport
    xmloff/source/text/XMLSectionFootnoteConfigExport
    xmloff/source/text/XMLSectionFootnoteConfigImport
    xmloff/source/text/XMLSectionImportContext
    xmloff/source/text/XMLSectionSourceDDEImportContext
    xmloff/source/text/XMLSectionSourceImportContext
    xmloff/source/text/XMLStringBufferImportContext
    xmloff/source/text/XMLTextCharStyleNamesElementExport
    xmloff/source/text/XMLTextColumnsContext
    xmloff/source/text/XMLTextColumnsExport
    xmloff/source/text/XMLTextFrameContext
    xmloff/source/text/XMLTextFrameHyperlinkContext
    xmloff/source/text/XMLTextHeaderFooterContext
    xmloff/source/text/XMLTextListAutoStylePool
    xmloff/source/text/XMLTextListBlockContext
    xmloff/source/text/XMLTextListItemContext
    xmloff/source/text/XMLTextMarkImportContext
    xmloff/source/text/XMLTextMasterPageContext
    xmloff/source/text/XMLTextMasterPageExport
    xmloff/source/text/XMLTextMasterStylesContext
    xmloff/source/text/XMLTextNumRuleInfo
    xmloff/source/text/XMLTextPropertySetContext
    xmloff/source/text/XMLTextShapeImportHelper
    xmloff/source/text/XMLTextShapeStyleContext
    xmloff/source/text/XMLTextTableContext
    xmloff/source/text/XMLTrackedChangesImportContext
    xmloff/source/text/xmllinebreakcontext
    xmloff/source/text/txtdrope
    xmloff/source/text/txtdropi
    xmloff/source/text/txtexppr
    xmloff/source/text/txtflde
    xmloff/source/text/txtfldi
    xmloff/source/text/txtftne
    xmloff/source/text/txtimp
    xmloff/source/text/txtimppr
    xmloff/source/text/txtlists
    xmloff/source/text/txtparae
    xmloff/source/text/txtparai
    xmloff/source/text/txtprhdl
    xmloff/source/text/txtprmap
    xmloff/source/text/txtsecte
    xmloff/source/text/txtstyle
    xmloff/source/text/txtstyli
    xmloff/source/text/txtvfldi
    xmloff/source/text/xmlcontentcontrolcontext

    xmloff/source/xforms/SchemaContext
    xmloff/source/xforms/SchemaRestrictionContext
    xmloff/source/xforms/SchemaSimpleTypeContext
    xmloff/source/xforms/TokenContext
    xmloff/source/xforms/XFormsBindContext
    xmloff/source/xforms/XFormsInstanceContext
    xmloff/source/xforms/XFormsModelContext
    xmloff/source/xforms/XFormsSubmissionContext
    xmloff/source/xforms/xformsapi
    xmloff/source/xforms/xformsexport
    xmloff/source/xforms/xformsimport
)

librestudio_expand_cxx(XO_SOURCES)
add_library(xo SHARED ${XO_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(xo PRIVATE
        "${CMAKE_SOURCE_DIR}/xmloff/inc/pch/precompiled_xo.hxx")
    target_compile_definitions(xo PRIVATE PCH_LEVEL=2)
endif ()
add_dependencies(xo xmloff_generated)

target_include_directories(xo PRIVATE
    "${CMAKE_SOURCE_DIR}/xmloff/inc"
    "${XMLOFF_GENERATED_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(xo officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(xo PRIVATE XMLOFF_DLLIMPLEMENTATION)
target_link_libraries(xo PRIVATE
    basegfx chart2api comphelper cppu cppuhelper docmodel i18nlangtag sal
    salhelper sax svl tl utl vcl
    Boost::boost)

# The component's implementations are split across four files -- one per
# gb_Library_add_componentimpl(s) call in Library_xo.mk -- because chart,
# impress and writer are each individually strippable upstream
# (ENABLE_WASM_STRIP_CHART, ENABLE_WASM_STRIP_BASIC_DRAW_MATH_IMPRESS); draw
# has no such flag and is always built. Neither flag is set in this build, so
# all four apply and librestudio_add_component() concatenates them, the same
# as gbuild's COMPONENTIMPL accumulation.
librestudio_add_component(xo
    COMPONENT      "${CMAKE_SOURCE_DIR}/xmloff/util/xo.component"
    IMPLEMENTATION "${CMAKE_SOURCE_DIR}/xmloff/util/xo.component.chart"
                   "${CMAKE_SOURCE_DIR}/xmloff/util/xo.component.draw"
                   "${CMAKE_SOURCE_DIR}/xmloff/util/xo.component.impress"
                   "${CMAKE_SOURCE_DIR}/xmloff/util/xo.component.writer"
    LIBRARY        xo
    LAYER          OOO)

librestudio_set_library_output_name(xo xo OOO)
install(TARGETS xo LIBRARY DESTINATION program)

# --- what is not here ---------------------------------------------------------
#
# Library_xof.mk (xmloff/source/transform/, the legacy OOo<->OASIS
# transformer) is a leaf: nothing in the tree links it, it is only ever
# resolved as a UNO service by name, so it stays unported along with the other
# service-only libraries skipped so far (mtfrenderer, canvasfactory,
# vclcanvas).
#
# Module_xmloff.mk's 5 CppunitTest check targets (xmloff_uxmloff, xmloff_draw,
# xmloff_forms, xmloff_style, xmloff_text -- MERGELIBS is never defined
# anywhere in this port, so xmloff_uxmloff's own "$(if $(MERGELIBS),,...)"
# guard resolves to "include it") are now all ported too, but not in this
# file: every one of them needs either program_services_rdb or
# ucb/unoxml/test, none of which exist yet at this file's own
# add_subdirectory() position (xmloff sits well before ucb, unoxml, test and
# the root CMakeLists.txt's final services.rdb packaging) -- see
# xmloff/CppunitTest_deferred.cmake, include()'d from the root CMakeLists.txt
# at the appropriate later point, for all 5. JunitTest_xmloff_unoapi (Module_
# xmloff.mk's gb_Module_add_subsequentcheck_targets) is Java/JUnit-based and
# stays out of scope, same as every other JunitTest/PythonTest/UITest
# elsewhere in this port.

# xmloff/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/xmloff/CppunitTest_deferred.cmake")
