# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# The 6 Java Jars Module_wizards.mk gates behind `ifeq ($(ENABLE_JAVA),TRUE)`
# -- Jar_commonwizards, Jar_form, Jar_query, Jar_report, Jar_reportbuilder
# (jar name "reportbuilderwizard", not "reportbuilder" -- gb_Jar_Jar's own
# first argument, not the .mk filename), Jar_table. Every one of them
# declares `gb_Jar_use_jars(...,java_uno)`, so every one of their
# librestudio_add_uno_jar() calls needs ${java_uno_JAR}, which is itself only
# set once bridges/Jar_java_uno_deferred.cmake's own deferred call fires --
# hence this whole file is itself deferred the same way, scheduled from
# wizards/CMakeLists.txt. Because deferred calls against the same DIRECTORY
# fire in scheduling order, and bridges/CMakeLists.txt (add_subdirectory()'d
# at a textually earlier point in the root CMakeLists.txt) schedules its own
# deferred call before this file's own scheduling call runs, ${java_uno_JAR}
# is guaranteed to already be set by the time this file executes.
#
# Real (compile-time) dependency graph, from each Jar_*.mk's own
# gb_Jar_use_jars, re-verified by reading every file directly: commonwizards
# depends on nothing here but libreoffice/java_uno; form/query/table each add
# commonwizards; report adds commonwizards (not reportbuilderwizard); and
# reportbuilderwizard (Jar_reportbuilder.mk) adds commonwizards AND report.
# So the build order below is: commonwizards, then form/query/table (order
# among these three does not matter), then report, then reportbuilderwizard.
#
# report's own gb_Jar_add_manifest_classpath(report,reportbuilderwizard.jar)
# is a manifest-only Class-Path hint for runtime UNO-service lookup, in the
# OPPOSITE direction of the real build dependency above -- modeling it as a
# CLASSPATH/DEPENDS on reportbuilderwizard here would both be wrong (report
# never javac-depends on reportbuilderwizard) and impossible (a real circular
# custom_target, since reportbuilderwizard already depends on report).

librestudio_add_uno_jar(commonwizards
    PACKAGEROOT com
    MANIFEST "${CMAKE_SOURCE_DIR}/wizards/com/sun/star/wizards/common/MANIFEST.MF"
    CLASSPATH
        "${libreoffice_JAR}"
        "${java_uno_JAR}"
    DEPENDS
        libreoffice_jar
        java_uno_jar
    SOURCES
        wizards/com/sun/star/wizards/common/HelpIds
        wizards/com/sun/star/wizards/common/Desktop
        wizards/com/sun/star/wizards/common/NumberFormatter
        wizards/com/sun/star/wizards/common/FileAccess
        wizards/com/sun/star/wizards/common/Helper
        wizards/com/sun/star/wizards/common/InvalidQueryException
        wizards/com/sun/star/wizards/common/JavaTools
        wizards/com/sun/star/wizards/common/NoValidPathException
        wizards/com/sun/star/wizards/common/Properties
        wizards/com/sun/star/wizards/common/PropertyNames
        wizards/com/sun/star/wizards/common/Resource
        wizards/com/sun/star/wizards/common/SystemDialog
        wizards/com/sun/star/wizards/common/Configuration
        wizards/com/sun/star/wizards/common/IRenderer
        wizards/com/sun/star/wizards/common/DebugHelper
        wizards/com/sun/star/wizards/common/PropertySetHelper
        wizards/com/sun/star/wizards/common/NumericalHelper
        wizards/com/sun/star/wizards/common/NamedValueCollection
        wizards/com/sun/star/wizards/common/ParaStyled
        wizards/com/sun/star/wizards/common/PlaceholderTextElement
        wizards/com/sun/star/wizards/common/TextElement
        wizards/com/sun/star/wizards/db/DBMetaData
        wizards/com/sun/star/wizards/db/CommandMetaData
        wizards/com/sun/star/wizards/db/QueryMetaData
        wizards/com/sun/star/wizards/db/BlindtextCreator
        wizards/com/sun/star/wizards/db/FieldColumn
        wizards/com/sun/star/wizards/db/CommandName
        wizards/com/sun/star/wizards/db/RecordParser
        wizards/com/sun/star/wizards/db/TypeInspector
        wizards/com/sun/star/wizards/db/ColumnPropertySet
        wizards/com/sun/star/wizards/db/RelationController
        wizards/com/sun/star/wizards/db/TableDescriptor
        wizards/com/sun/star/wizards/db/SQLQueryComposer
        wizards/com/sun/star/wizards/db/DatabaseObjectWizard
        wizards/com/sun/star/wizards/ui/event/XActionListenerAdapter
        wizards/com/sun/star/wizards/ui/event/XItemListenerAdapter
        wizards/com/sun/star/wizards/ui/event/XTextListenerAdapter
        wizards/com/sun/star/wizards/ui/AggregateComponent
        wizards/com/sun/star/wizards/ui/CommandFieldSelection
        wizards/com/sun/star/wizards/ui/ControlScroller
        wizards/com/sun/star/wizards/ui/DBLimitedFieldSelection
        wizards/com/sun/star/wizards/ui/FilterComponent
        wizards/com/sun/star/wizards/ui/FieldSelection
        wizards/com/sun/star/wizards/ui/ButtonList
        wizards/com/sun/star/wizards/ui/PeerConfig
        wizards/com/sun/star/wizards/ui/SortingComponent
        wizards/com/sun/star/wizards/ui/TitlesComponent
        wizards/com/sun/star/wizards/ui/WizardDialog
        wizards/com/sun/star/wizards/ui/UIConsts
        wizards/com/sun/star/wizards/ui/UnoDialog
        wizards/com/sun/star/wizards/ui/UnoDialog2
        wizards/com/sun/star/wizards/ui/XFieldSelectionListener
        wizards/com/sun/star/wizards/document/Control
        wizards/com/sun/star/wizards/document/DatabaseControl
        wizards/com/sun/star/wizards/document/FormHandler
        wizards/com/sun/star/wizards/document/GridControl
        wizards/com/sun/star/wizards/document/OfficeDocument
        wizards/com/sun/star/wizards/document/Shape
        wizards/com/sun/star/wizards/document/TimeStampControl
        wizards/com/sun/star/wizards/text/TextDocument
        wizards/com/sun/star/wizards/text/TextFieldHandler
        wizards/com/sun/star/wizards/text/TextSectionHandler
        wizards/com/sun/star/wizards/text/TextStyleHandler
        wizards/com/sun/star/wizards/text/TextTableHandler
        wizards/com/sun/star/wizards/text/ViewHandler)

librestudio_add_uno_jar(form
    PACKAGEROOT com
    MANIFEST "${CMAKE_SOURCE_DIR}/wizards/com/sun/star/wizards/form/MANIFEST.MF"
    CLASSPATH
        "${libreoffice_JAR}"
        "${java_uno_JAR}"
        "${commonwizards_JAR}"
    DEPENDS
        libreoffice_jar
        java_uno_jar
        commonwizards_jar
    SOURCES
        wizards/com/sun/star/wizards/form/CallFormWizard
        wizards/com/sun/star/wizards/form/DataEntrySetter
        wizards/com/sun/star/wizards/form/FieldLinker
        wizards/com/sun/star/wizards/form/Finalizer
        wizards/com/sun/star/wizards/form/FormConfiguration
        wizards/com/sun/star/wizards/form/FormControlArranger
        wizards/com/sun/star/wizards/form/FormDocument
        wizards/com/sun/star/wizards/form/FormWizard
        wizards/com/sun/star/wizards/form/StyleApplier
        wizards/com/sun/star/wizards/form/UIControlArranger)

# gb_Jar_set_componentfile(form,wizards/com/sun/star/wizards/form/form,OOO,
# services) -- layer OOO (real, checked against the .component file's own
# loader="com.sun.star.loader.Java2" attribute), but NOT routed to the
# shared "services" RDB: see this file's own header (or wizards/CMakeLists.
# txt's) fuller explanation of why that registry is unreachable from here --
# librestudio_finalize_components(services) runs synchronously at a fixed
# point in the un-deferred pass over the root CMakeLists.txt, long before any
# DEFERRED call (this file included) ever fires, so a component registered
# here would silently never make it into services.rdb. Routed instead to a
# dedicated "wizards" registry, finalized at the end of this same file --
# same accepted limitation already established for scripting's own
# scriptproviderforbeanshell RDB (real, correct artifact; not wired into the
# live application's actual service manager).
librestudio_add_java_component(form
    COMPONENT "${CMAKE_SOURCE_DIR}/wizards/com/sun/star/wizards/form/form.component"
    JARFILE "${form_JAR}"
    LAYER OOO
    RDB wizards)

librestudio_add_uno_jar(query
    PACKAGEROOT com
    MANIFEST "${CMAKE_SOURCE_DIR}/wizards/com/sun/star/wizards/query/MANIFEST.MF"
    CLASSPATH
        "${libreoffice_JAR}"
        "${java_uno_JAR}"
        "${commonwizards_JAR}"
    DEPENDS
        libreoffice_jar
        java_uno_jar
        commonwizards_jar
    SOURCES
        wizards/com/sun/star/wizards/query/CallQueryWizard
        wizards/com/sun/star/wizards/query/Finalizer
        wizards/com/sun/star/wizards/query/QuerySummary
        wizards/com/sun/star/wizards/query/QueryWizard)

librestudio_add_java_component(query
    COMPONENT "${CMAKE_SOURCE_DIR}/wizards/com/sun/star/wizards/query/query.component"
    JARFILE "${query_JAR}"
    LAYER OOO
    RDB wizards)

librestudio_add_uno_jar(table
    PACKAGEROOT com
    MANIFEST "${CMAKE_SOURCE_DIR}/wizards/com/sun/star/wizards/table/MANIFEST.MF"
    CLASSPATH
        "${libreoffice_JAR}"
        "${java_uno_JAR}"
        "${commonwizards_JAR}"
    DEPENDS
        libreoffice_jar
        java_uno_jar
        commonwizards_jar
    SOURCES
        wizards/com/sun/star/wizards/table/CallTableWizard
        wizards/com/sun/star/wizards/table/CGCategory
        wizards/com/sun/star/wizards/table/CGTable
        wizards/com/sun/star/wizards/table/FieldDescription
        wizards/com/sun/star/wizards/table/FieldFormatter
        wizards/com/sun/star/wizards/table/Finalizer
        wizards/com/sun/star/wizards/table/PrimaryKeyHandler
        wizards/com/sun/star/wizards/table/ScenarioSelector
        wizards/com/sun/star/wizards/table/TableWizard)

librestudio_add_java_component(table
    COMPONENT "${CMAKE_SOURCE_DIR}/wizards/com/sun/star/wizards/table/table.component"
    JARFILE "${table_JAR}"
    LAYER OOO
    RDB wizards)

librestudio_add_uno_jar(report
    PACKAGEROOT com
    MANIFEST "${CMAKE_SOURCE_DIR}/wizards/com/sun/star/wizards/report/MANIFEST.MF"
    MANIFEST_CLASSPATH
        reportbuilderwizard.jar
    CLASSPATH
        "${libreoffice_JAR}"
        "${java_uno_JAR}"
        "${commonwizards_JAR}"
    DEPENDS
        libreoffice_jar
        java_uno_jar
        commonwizards_jar
    SOURCES
        wizards/com/sun/star/wizards/report/CallReportWizard
        wizards/com/sun/star/wizards/report/Dataimport
        wizards/com/sun/star/wizards/report/DBColumn
        wizards/com/sun/star/wizards/report/GroupFieldHandler
        wizards/com/sun/star/wizards/report/IReportBuilderLayouter
        wizards/com/sun/star/wizards/report/IReportDefinitionReadAccess
        wizards/com/sun/star/wizards/report/IReportDocument
        wizards/com/sun/star/wizards/report/RecordTable
        wizards/com/sun/star/wizards/report/ReportFinalizer
        wizards/com/sun/star/wizards/report/ReportImplementationHelper
        wizards/com/sun/star/wizards/report/ReportLayouter
        wizards/com/sun/star/wizards/report/ReportTextDocument
        wizards/com/sun/star/wizards/report/ReportTextImplementation
        wizards/com/sun/star/wizards/report/ReportWizard)

librestudio_add_java_component(report
    COMPONENT "${CMAKE_SOURCE_DIR}/wizards/com/sun/star/wizards/report/report.component"
    JARFILE "${report_JAR}"
    LAYER OOO
    RDB wizards)

# Jar_reportbuilder.mk's own gb_Jar_Jar name is "reportbuilderwizard", not
# "reportbuilder" (that name belongs to the separate, unrelated reportbuilder/
# module's own Jar_reportbuilder.mk, gated on the unrelated and unported
# ENABLE_REPORTBUILDER/external/jfreereport -- see reportbuilder/CMakeLists.txt
# and PORTING_STATUS.md's own note on that module). No gb_Jar_set_componentfile
# call in this .mk -- reportbuilderwizard is a pure support jar, like
# commonwizards.
librestudio_add_uno_jar(reportbuilderwizard
    PACKAGEROOT com
    MANIFEST "${CMAKE_SOURCE_DIR}/wizards/com/sun/star/wizards/reportbuilder/MANIFEST.MF"
    CLASSPATH
        "${libreoffice_JAR}"
        "${java_uno_JAR}"
        "${commonwizards_JAR}"
        "${report_JAR}"
    DEPENDS
        libreoffice_jar
        java_uno_jar
        commonwizards_jar
        report_jar
    SOURCES
        wizards/com/sun/star/wizards/reportbuilder/ReportBuilderImplementation
        wizards/com/sun/star/wizards/reportbuilder/layout/ColumnarSingleColumn
        wizards/com/sun/star/wizards/reportbuilder/layout/ColumnarThreeColumns
        wizards/com/sun/star/wizards/reportbuilder/layout/ColumnarTwoColumns
        wizards/com/sun/star/wizards/reportbuilder/layout/DesignTemplate
        wizards/com/sun/star/wizards/reportbuilder/layout/InBlocksLabelsAbove
        wizards/com/sun/star/wizards/reportbuilder/layout/InBlocksLabelsLeft
        wizards/com/sun/star/wizards/reportbuilder/layout/LayoutConstants
        wizards/com/sun/star/wizards/reportbuilder/layout/ReportBuilderLayouter
        wizards/com/sun/star/wizards/reportbuilder/layout/SectionEmptyObject
        wizards/com/sun/star/wizards/reportbuilder/layout/SectionLabel
        wizards/com/sun/star/wizards/reportbuilder/layout/SectionObject
        wizards/com/sun/star/wizards/reportbuilder/layout/SectionTextField
        wizards/com/sun/star/wizards/reportbuilder/layout/Tabular)

# gb_Helper_register_jars_for_install(OOO,ooo,[commonwizards form ... table])
# and the separate (OOO,reportbuilder,[reportbuilder reportbuilderwizard])
# group (Repository.mk) -- both real jars, both resolve to program/classes on
# this single-layer Linux port regardless of the (cosmetic here) group name.
install(FILES
    "${commonwizards_JAR}"
    "${form_JAR}"
    "${query_JAR}"
    "${report_JAR}"
    "${reportbuilderwizard_JAR}"
    "${table_JAR}"
    DESTINATION program/classes)

# form/query/report/table's 4 components, packed into their own dedicated
# "wizards" registry (see the header comment on form's own
# librestudio_add_java_component() call above for why not "services").
librestudio_finalize_components(wizards)
