# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# officecfg — the configuration schema, and the registry built from it.
#
# Two halves, and they are unrelated except in name.  The C++ half is below:
# officecfg/registry/cppheader.xsl turns each .xcs schema into a typed header
# (officecfg::Office::Common::Save::…), which comphelper's
# embeddedobjectcontainer.cxx reads configuration through, and upstream's
# equivalent is officecfg/CustomTarget_registry.mk.
#
# The registry half is at the bottom of this file, under "the registry".  That
# one is what makes a program able to start at all: configmgr reads a registry,
# and without one every VCL program dies inside InitVCL().  It is transcribed
# from three .mk files — officecfg/Configuration_officecfg.mk for what goes in,
# solenv/gbuild/Configuration.mk for how each piece is processed, and
# postprocess/CustomTarget_registry.mk for the packing.

find_program(XSLTPROC_EXECUTABLE xsltproc REQUIRED)

set(OFFICECFG_SCHEMA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/registry/schema/org/openoffice")
set(OFFICECFG_CPPHEADER_XSL "${CMAKE_CURRENT_SOURCE_DIR}/registry/cppheader.xsl")
set(OFFICECFG_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/registry")
set(OFFICECFG_HEADER_ROOT "${OFFICECFG_INCLUDE_DIR}/officecfg")

file(GLOB_RECURSE OFFICECFG_XCS_FILES
    RELATIVE "${OFFICECFG_SCHEMA_ROOT}" "${OFFICECFG_SCHEMA_ROOT}/*.xcs")

set(OFFICECFG_HEADERS)
foreach (_xcs IN LISTS OFFICECFG_XCS_FILES)
    string(REGEX REPLACE "\\.xcs$" "" _base "${_xcs}")

    # cppheader.xsl namespaces the generated C++ by the schema's path segments:
    # Office/Common.xcs becomes officecfg::Office::Common, and the stylesheet is
    # told each segment through its own nsN parameter.
    string(REPLACE "/" ";" _segments "${_base}")
    set(_ns_params)
    set(_index 1)
    foreach (_segment IN LISTS _segments)
        list(APPEND _ns_params "--stringparam" "ns${_index}" "${_segment}")
        math(EXPR _index "${_index} + 1")
    endforeach ()

    get_filename_component(_out_dir "${OFFICECFG_HEADER_ROOT}/${_base}" DIRECTORY)
    set(_out "${OFFICECFG_HEADER_ROOT}/${_base}.hxx")

    add_custom_command(
        OUTPUT "${_out}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_out_dir}"
        COMMAND "${XSLTPROC_EXECUTABLE}" --nonet ${_ns_params}
                -o "${_out}" "${OFFICECFG_CPPHEADER_XSL}" "${OFFICECFG_SCHEMA_ROOT}/${_xcs}"
        DEPENDS "${OFFICECFG_SCHEMA_ROOT}/${_xcs}" "${OFFICECFG_CPPHEADER_XSL}"
        COMMENT "cppheader.xsl: officecfg/${_base}.hxx"
        VERBATIM)

    list(APPEND OFFICECFG_HEADERS "${_out}")
endforeach ()

add_custom_target(officecfg_headers DEPENDS ${OFFICECFG_HEADERS})

set(OFFICECFG_INCLUDE_DIR "${OFFICECFG_INCLUDE_DIR}" CACHE INTERNAL
    "officecfg generated configuration headers")


# --- the registry --------------------------------------------------------------
#
# What a program reads its configuration from, and what has to exist before any
# of them can start: without a registry configmgr throws while InitVCL() is
# building the default style settings, and the program dies with no message at
# all.  See the note on svptest in vcl/CMakeLists.txt.
#
# libvcl does not read the .xcs and .xcu files in the tree.  It reads a registry
# — main.xcd — that three xsltproc passes and one packing pass turn them into:
#
#   schema_val.xsl   validates a .xcs against component-schema.dtd
#   schema_trim.xsl  and writes the copy that goes into the registry
#
#   data_val.xsl     validates a .xcu against its .xcs
#   alllang.xsl      and resolves the two things a .xcu is not yet: the
#                    ${...} placeholders the build substitutes, and the values
#                    marked install:module, of which exactly one module's are
#                    kept
#
#   packregistry.xslt  takes a list of the processed files and writes the
#                      single XML document that is main.xcd
#
# Two of those are worth spelling out because they are not obvious from the
# file names.  The second pass of the data pipeline is the same stylesheet as
# the spool step, told a different module — a spool module named
# Inet-unixdesktop.xcu is not a file, it is Inet.xcu read with -module
# unixdesktop, which is how one source yields one output per platform.  And the
# ${...} placeholders survive into the registry, so the packing step ends in the
# sed that fills them in; see the note on the identity variables below.
#
# Transcribed from three files, and each part is theirs:
#   officecfg/Configuration_officecfg.mk     what goes in — the four lists
#   solenv/gbuild/Configuration.mk           the three pipelines
#   postprocess/CustomTarget_registry.mk     FILES_main, and the packing

# The stylesheets and DTDs the pipelines need.
set(OFFICECFG_XSL_SCHEMA_VAL "${CMAKE_CURRENT_SOURCE_DIR}/util/schema_val.xsl")
set(OFFICECFG_XSL_SCHEMA_TRIM "${CMAKE_CURRENT_SOURCE_DIR}/util/schema_trim.xsl")
set(OFFICECFG_XSL_DATA_VAL "${CMAKE_CURRENT_SOURCE_DIR}/util/data_val.xsl")
set(OFFICECFG_XSL_ALLLANG "${CMAKE_CURRENT_SOURCE_DIR}/util/alllang.xsl")
set(OFFICECFG_DTD_SCHEMA "${CMAKE_CURRENT_SOURCE_DIR}/registry/component-schema.dtd")
set(OFFICECFG_DTD_UPDATE "${CMAKE_CURRENT_SOURCE_DIR}/registry/component-update.dtd")
set(OFFICECFG_PACKREGISTRY "${CMAKE_SOURCE_DIR}/solenv/bin/packregistry.xslt")

# The two roots the lists are relative to, and a build tree to mirror the
# XcsTarget / XcuDataTarget / XcuModuleTarget directories into.
set(OFFICECFG_SCHEMA_SRC "${CMAKE_CURRENT_SOURCE_DIR}/registry/schema")
set(OFFICECFG_DATA_SRC "${CMAKE_CURRENT_SOURCE_DIR}/registry/data")
set(OFFICECFG_REGISTRY_BUILD "${CMAKE_CURRENT_BINARY_DIR}/registry-build")
set(OFFICECFG_XCS_BUILD "${OFFICECFG_REGISTRY_BUILD}/XcsTarget")
set(OFFICECFG_XCU_BUILD "${OFFICECFG_REGISTRY_BUILD}/XcuDataTarget")
set(OFFICECFG_XCM_BUILD "${OFFICECFG_REGISTRY_BUILD}/XcuModuleTarget")

# configure.ac's LIBO_SHARE_FOLDER and LIBO_SHARE_HELP_FOLDER on unx, which
# alllang.xsl substitutes for the @LIBO_SHARE_FOLDER@ that Office/Paths.xcu
# writes into its path settings.
set(OFFICECFG_SHARE_FOLDER "share")
set(OFFICECFG_SHARE_HELP_FOLDER "help")

# Where the office says who it is.  configure.ac takes the product name from
# --with-product-name and the vendor from --with-vendor, the version comes from
# AC_INIT, and postprocess's sed substitutes all of it into main.xcd: these are
# the strings the about box and the start centre read.
#
# A fork has to answer these for itself and this one has not yet, so they are
# cache variables.  The product name is the project's own name; the vendor and
# the two URLs are left empty rather than filled with the upstream ones, since
# an empty vendor reads as an empty vendor where a borrowed one would read as a
# claim about who made this.
set(LIBRESTUDIO_PRODUCT_NAME "LibreStudio" CACHE STRING
    "the office's own name; configure.ac's PRODUCTNAME")
set(LIBRESTUDIO_VENDOR "" CACHE STRING
    "who ships this; configure.ac's OOO_VENDOR, empty until the project decides")
set(LIBRESTUDIO_INFO_URL "" CACHE STRING
    "postprocess's STARTCENTER_INFO_URL, empty until the project decides")
set(LIBRESTUDIO_PRIVACY_POLICY_URL "" CACHE STRING
    "postprocess's PRIVACY_POLICY_URL, empty until the project decides")

# The version, read from AC_INIT so that it stays in the one place the tree
# keeps it.  configure.ac splits it into four numbers and a suffix; the
# substitutions want the four back together and the suffix on its own.
file(STRINGS "${CMAKE_SOURCE_DIR}/configure.ac" OFFICECFG_AC_INIT REGEX "^AC_INIT")
string(REGEX MATCH "\\[([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)([^]]*)\\]"
    OFFICECFG_AC_INIT_MATCH "${OFFICECFG_AC_INIT}")
set(OFFICECFG_VERSION
    "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}.${CMAKE_MATCH_4}")
set(OFFICECFG_VERSION_SUFFIX "${CMAKE_MATCH_5}")
unset(OFFICECFG_AC_INIT)

# --- the three pipelines -------------------------------------------------------

# gb_XcsTarget.  schema_val validates against the DTD and prints nothing;
# schema_trim writes the copy that goes into the registry.  The name is the path
# below officecfg/registry/schema, so it names the output as well as the input,
# and -componentName is that path with its separators turned into dots —
# org.openoffice.Office.Common, which schema_val checks against the schema's own
# component name.
function(librestudio_officecfg_xcs name)
    set(_src "${OFFICECFG_SCHEMA_SRC}/${name}")
    set(_out "${OFFICECFG_XCS_BUILD}/${name}")
    string(REGEX REPLACE "\\.xcs$" "" _base "${name}")
    string(REPLACE "/" "." _component "${_base}")
    get_filename_component(_dir "${_out}" DIRECTORY)
    add_custom_command(
        OUTPUT "${_out}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_dir}"
        COMMAND "${XSLTPROC_EXECUTABLE}" --nonet --noout
                --stringparam componentName "${_component}"
                --stringparam root "${OFFICECFG_SCHEMA_SRC}/"
                --stringparam schemaRoot "${OFFICECFG_SCHEMA_SRC}"
                "${OFFICECFG_XSL_SCHEMA_VAL}" "${_src}"
        COMMAND "${XSLTPROC_EXECUTABLE}" --nonet
                -o "${_out}"
                --stringparam LIBO_SHARE_FOLDER "${OFFICECFG_SHARE_FOLDER}"
                --stringparam LIBO_SHARE_HELP_FOLDER "${OFFICECFG_SHARE_HELP_FOLDER}"
                "${OFFICECFG_XSL_SCHEMA_TRIM}" "${_src}"
        DEPENDS "${_src}"
                "${OFFICECFG_XSL_SCHEMA_VAL}" "${OFFICECFG_XSL_SCHEMA_TRIM}"
                "${OFFICECFG_DTD_SCHEMA}"
        COMMENT "schema_trim.xsl: ${name}"
        VERBATIM)
    set(OFFICECFG_XCS_OUTPUTS ${OFFICECFG_XCS_OUTPUTS} "${_out}" PARENT_SCOPE)
endfunction()

# gb_XcuDataTarget.  data_val validates the data against its schema and prints
# nothing; alllang writes the copy that goes into the registry.  The schema is
# named by the .xcu's own path with .xcs on the end — Office/Common.xcu is
# checked against Office/Common.xcs — and it is a dependency, so the schema is
# always built first.
function(librestudio_officecfg_xcu name)
    set(_src "${OFFICECFG_DATA_SRC}/${name}")
    set(_out "${OFFICECFG_XCU_BUILD}/${name}")
    string(REGEX REPLACE "\\.xcu$" "" _base "${name}")
    set(_xcs "${OFFICECFG_XCS_BUILD}/${_base}.xcs")
    get_filename_component(_dir "${_out}" DIRECTORY)
    add_custom_command(
        OUTPUT "${_out}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_dir}"
        COMMAND "${XSLTPROC_EXECUTABLE}" --nonet --noout
                --stringparam xcs "${_xcs}"
                --stringparam schemaRoot "${OFFICECFG_SCHEMA_SRC}"
                --path "${CMAKE_CURRENT_SOURCE_DIR}/registry"
                "${OFFICECFG_XSL_DATA_VAL}" "${_src}"
        COMMAND "${XSLTPROC_EXECUTABLE}" --nonet
                -o "${_out}"
                --stringparam xcs "${_xcs}"
                --stringparam schemaRoot "${OFFICECFG_SCHEMA_SRC}"
                --stringparam LIBO_SHARE_FOLDER "${OFFICECFG_SHARE_FOLDER}"
                --stringparam LIBO_SHARE_HELP_FOLDER "${OFFICECFG_SHARE_HELP_FOLDER}"
                --path "${CMAKE_CURRENT_SOURCE_DIR}/registry"
                "${OFFICECFG_XSL_ALLLANG}" "${_src}"
        DEPENDS "${_src}" "${_xcs}"
                "${OFFICECFG_XSL_DATA_VAL}" "${OFFICECFG_XSL_ALLLANG}"
                "${OFFICECFG_DTD_UPDATE}"
        COMMENT "alllang.xsl: ${name}"
        VERBATIM)
    set(OFFICECFG_XCU_OUTPUTS ${OFFICECFG_XCU_OUTPUTS} "${_out}" PARENT_SCOPE)
endfunction()

# gb_XcuModuleTarget, the spool step — and the one place where the name of the
# output is not the name of an input.  Inet-unixdesktop.xcu is read from
# Inet.xcu, because the values that belong to a module sit in the base file
# marked install:module="unixdesktop" alongside the ones for every other
# platform.  alllang.xsl is told which module is wanted and drops the rest, so
# one source yields one file per platform and the list below says which of them
# a Linux registry takes and in what order.
#
# The .mk finds both the source and the module by string surgery on the name,
# and the surgery is what this repeats: the source is the name with the last
# dash-segment cut off, the module is that segment.
function(librestudio_officecfg_xcm name)
    string(REGEX REPLACE "\\.xcu$" "" _base "${name}")
    get_filename_component(_leaf "${_base}" NAME)
    string(REPLACE "-" "." _dotted "${_base}")
    string(REGEX REPLACE "\\.[^./]*$" "" _stem "${_dotted}")
    string(REGEX REPLACE "^.*-" "" _module "${_leaf}")
    set(_src "${OFFICECFG_DATA_SRC}/${_stem}.xcu")
    set(_xcs "${OFFICECFG_XCS_BUILD}/${_stem}.xcs")
    set(_out "${OFFICECFG_XCM_BUILD}/${name}")
    get_filename_component(_dir "${_out}" DIRECTORY)
    add_custom_command(
        OUTPUT "${_out}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_dir}"
        COMMAND "${XSLTPROC_EXECUTABLE}" --nonet
                -o "${_out}"
                --stringparam xcs "${_xcs}"
                --stringparam schemaRoot "${OFFICECFG_SCHEMA_SRC}"
                --stringparam module "${_module}"
                --stringparam LIBO_SHARE_FOLDER "${OFFICECFG_SHARE_FOLDER}"
                --stringparam LIBO_SHARE_HELP_FOLDER "${OFFICECFG_SHARE_HELP_FOLDER}"
                "${OFFICECFG_XSL_ALLLANG}" "${_src}"
        DEPENDS "${_src}" "${_xcs}" "${OFFICECFG_XSL_ALLLANG}"
        COMMENT "alllang.xsl (${_module}): ${name}"
        VERBATIM)
    set(OFFICECFG_XCM_OUTPUTS ${OFFICECFG_XCM_OUTPUTS} "${_out}" PARENT_SCOPE)
endfunction()


# --- what goes in --------------------------------------------------------------

# gb_Configuration_add_schemas.  All of them are built, because the spool and
# localized steps below name schemas of their own; the ones main.xcd does not
# take are the schemas belonging to the per-module .xcd files — calc.xcd,
# writer.xcd and the rest — which this build does not produce yet.

set(OFFICECFG_SCHEMA_FILES
    org/openoffice/Inet.xcs
    org/openoffice/Interaction.xcs
    org/openoffice/LDAP.xcs
    org/openoffice/Office/Accelerators.xcs
    org/openoffice/Office/Addons.xcs
    org/openoffice/Office/BasicIDE.xcs
    org/openoffice/Office/Calc.xcs
    org/openoffice/Office/CalcAddIns.xcs
    org/openoffice/Office/Canvas.xcs
    org/openoffice/Office/Chart.xcs
    org/openoffice/Office/Commands.xcs
    org/openoffice/Office/Common.xcs
    org/openoffice/Office/Compatibility.xcs
    org/openoffice/Office/DataAccess.xcs
    org/openoffice/Office/DataAccess/Drivers.xcs
    org/openoffice/Office/Draw.xcs
    org/openoffice/Office/Embedding.xcs
    org/openoffice/Office/Events.xcs
    org/openoffice/Office/ExtendedColorScheme.xcs
    org/openoffice/Office/ExtensionDependencies.xcs
    org/openoffice/Office/ExtensionManager.xcs
    org/openoffice/Office/FormWizard.xcs
    org/openoffice/Office/Histories.xcs
    org/openoffice/Office/Impress.xcs
    org/openoffice/Office/Java.xcs
    org/openoffice/Office/Jobs.xcs
    org/openoffice/Office/Labels.xcs
    org/openoffice/Office/Linguistic.xcs
    org/openoffice/Office/Logging.xcs
    org/openoffice/Office/Math.xcs
    org/openoffice/Office/OptionsDialog.xcs
    org/openoffice/Office/Paths.xcs
    org/openoffice/Office/PresentationMinimizer.xcs
    org/openoffice/Office/PresenterScreen.xcs
    org/openoffice/Office/ProtocolHandler.xcs
    org/openoffice/Office/Recovery.xcs
    org/openoffice/Office/ReportDesign.xcs
    org/openoffice/Office/Scripting.xcs
    org/openoffice/Office/Security.xcs
    org/openoffice/Office/TableWizard.xcs
    org/openoffice/Office/TypeDetection.xcs
    org/openoffice/Office/UI.xcs
    org/openoffice/Office/UI/BaseWindowState.xcs
    org/openoffice/Office/UI/BasicIDECommands.xcs
    org/openoffice/Office/UI/BasicIDEWindowState.xcs
    org/openoffice/Office/UI/BibliographyCommands.xcs
    org/openoffice/Office/UI/BibliographyWindowState.xcs
    org/openoffice/Office/UI/CalcCommands.xcs
    org/openoffice/Office/UI/CalcWindowState.xcs
    org/openoffice/Office/UI/Category.xcs
    org/openoffice/Office/UI/ChartCommands.xcs
    org/openoffice/Office/UI/ChartWindowState.xcs
    org/openoffice/Office/UI/Commands.xcs
    org/openoffice/Office/UI/Controller.xcs
    org/openoffice/Office/UI/DbBrowserWindowState.xcs
    org/openoffice/Office/UI/DbQueryWindowState.xcs
    org/openoffice/Office/UI/DbRelationWindowState.xcs
    org/openoffice/Office/UI/DbReportWindowState.xcs
    org/openoffice/Office/UI/DbTableDataWindowState.xcs
    org/openoffice/Office/UI/DbTableWindowState.xcs
    org/openoffice/Office/UI/DbuCommands.xcs
    org/openoffice/Office/UI/DrawImpressCommands.xcs
    org/openoffice/Office/UI/DrawWindowState.xcs
    org/openoffice/Office/UI/Effects.xcs
    org/openoffice/Office/UI/Factories.xcs
    org/openoffice/Office/UI/GenericCategories.xcs
    org/openoffice/Office/UI/GenericCommands.xcs
    org/openoffice/Office/UI/GlobalSettings.xcs
    org/openoffice/Office/UI/ImpressWindowState.xcs
    org/openoffice/Office/UI/Infobar.xcs
    org/openoffice/Office/UI/MathCommands.xcs
    org/openoffice/Office/UI/MathWindowState.xcs
    org/openoffice/Office/UI/ReportCommands.xcs
    org/openoffice/Office/UI/Sidebar.xcs
    org/openoffice/Office/UI/StartModuleCommands.xcs
    org/openoffice/Office/UI/StartModuleWindowState.xcs
    org/openoffice/Office/UI/ToolbarMode.xcs
    org/openoffice/Office/UI/WindowContentFactories.xcs
    org/openoffice/Office/UI/WindowState.xcs
    org/openoffice/Office/UI/WriterCommands.xcs
    org/openoffice/Office/UI/WriterFormWindowState.xcs
    org/openoffice/Office/UI/WriterGlobalWindowState.xcs
    org/openoffice/Office/UI/WriterReportWindowState.xcs
    org/openoffice/Office/UI/WriterWebWindowState.xcs
    org/openoffice/Office/UI/WriterWindowState.xcs
    org/openoffice/Office/UI/XFormsWindowState.xcs
    org/openoffice/Office/Update.xcs
    org/openoffice/Office/Views.xcs
    org/openoffice/Office/Writer.xcs
    org/openoffice/Office/WriterWeb.xcs
    org/openoffice/Setup.xcs
    org/openoffice/System.xcs
    org/openoffice/TypeDetection/Filter.xcs
    org/openoffice/TypeDetection/GraphicFilter.xcs
    org/openoffice/TypeDetection/Misc.xcs
    org/openoffice/TypeDetection/Types.xcs
    org/openoffice/TypeDetection/UISort.xcs
    org/openoffice/UserProfile.xcs
    org/openoffice/VCL.xcs
    org/openoffice/ucb/Configuration.xcs
    org/openoffice/ucb/Hierarchy.xcs
    org/openoffice/ucb/Store.xcs
)

# gb_Configuration_add_datas.

set(OFFICECFG_DATA_FILES
    org/openoffice/Inet.xcu
    org/openoffice/System.xcu
    org/openoffice/UserProfile.xcu
    org/openoffice/VCL.xcu
    org/openoffice/Interaction.xcu
    org/openoffice/Office/Calc.xcu
    org/openoffice/Office/BasicIDE.xcu
    org/openoffice/Office/Canvas.xcu
    org/openoffice/Office/Compatibility.xcu
    org/openoffice/Office/ExtensionDependencies.xcu
    org/openoffice/Office/ExtensionManager.xcu
    org/openoffice/Office/Impress.xcu
    org/openoffice/Office/Jobs.xcu
    org/openoffice/Office/Logging.xcu
    org/openoffice/Office/Math.xcu
    org/openoffice/Office/ProtocolHandler.xcu
    org/openoffice/Office/Security.xcu
    org/openoffice/Office/Views.xcu
    org/openoffice/Office/Paths.xcu
    org/openoffice/Office/Histories.xcu
    org/openoffice/Office/ReportDesign.xcu
    org/openoffice/Office/UI/Controller.xcu
    org/openoffice/Office/UI/Factories.xcu
    org/openoffice/TypeDetection/UISort.xcu
    org/openoffice/ucb/Configuration.xcu
)

# gb_Configuration_add_spool_modules.  The entries inside $(call
# gb_Helper_optional,...) in the .mk are left out: they are REPORTBUILDER and
# LIBRELOGO, neither of which this build enables.

set(OFFICECFG_SPOOL_FILES
    org/openoffice/Inet-macosx.xcu
    org/openoffice/Inet-unixdesktop.xcu
    org/openoffice/Inet-wnt.xcu
    org/openoffice/Setup-writer.xcu
    org/openoffice/Setup-calc.xcu
    org/openoffice/Setup-draw.xcu
    org/openoffice/Setup-impress.xcu
    org/openoffice/Setup-base.xcu
    org/openoffice/Setup-math.xcu
    org/openoffice/Setup-report.xcu
    org/openoffice/Setup-start.xcu
    org/openoffice/UserProfile-unixdesktop.xcu
    org/openoffice/VCL-unixdesktop.xcu
    org/openoffice/Office/Accelerators-macosx.xcu
    org/openoffice/Office/Accelerators-unxwnt.xcu
    org/openoffice/Office/Common-writer.xcu
    org/openoffice/Office/Common-calc.xcu
    org/openoffice/Office/Common-draw.xcu
    org/openoffice/Office/Common-impress.xcu
    org/openoffice/Office/Common-base.xcu
    org/openoffice/Office/Common-math.xcu
    org/openoffice/Office/Common-unx.xcu
    org/openoffice/Office/Common-unixdesktop.xcu
    org/openoffice/Office/Common-macosx.xcu
    org/openoffice/Office/Common-macosxsandbox.xcu
    org/openoffice/Office/Common-wnt.xcu
    org/openoffice/Office/Common-UseOOoFileDialogs.xcu
    org/openoffice/Office/Common-32bit.xcu
    org/openoffice/Office/Jobs-impress.xcu
    org/openoffice/Office/ProtocolHandler-impress.xcu
    org/openoffice/Office/Common-cjk.xcu
    org/openoffice/Office/Common-ctl.xcu
    org/openoffice/Office/Common-ctlseqcheck.xcu
    org/openoffice/Office/DataAccess-evoab2.xcu
    org/openoffice/Office/Paths-macosx.xcu
    org/openoffice/Office/Paths-unxwnt.xcu
    org/openoffice/Office/Paths-unixdesktop.xcu
    org/openoffice/Office/Paths-internallibexttextcatdata.xcu
    org/openoffice/Office/Paths-externallibexttextcatdata.xcu
    org/openoffice/Office/Paths-internallibnumbertextdata.xcu
    org/openoffice/Office/Paths-externallibnumbertextdata.xcu
    org/openoffice/Office/Writer-cjk.xcu
    org/openoffice/Office/Impress-ogltrans.xcu
    org/openoffice/Office/Embedding-calc.xcu
    org/openoffice/Office/Embedding-chart.xcu
    org/openoffice/Office/Embedding-draw.xcu
    org/openoffice/Office/Embedding-impress.xcu
    org/openoffice/Office/Embedding-math.xcu
    org/openoffice/Office/Embedding-base.xcu
    org/openoffice/Office/Embedding-writer.xcu
    org/openoffice/Office/UI/Infobar-macosxsandbox.xcu
    org/openoffice/TypeDetection/UISort-writer.xcu
    org/openoffice/TypeDetection/UISort-calc.xcu
    org/openoffice/TypeDetection/UISort-draw.xcu
    org/openoffice/TypeDetection/UISort-impress.xcu
    org/openoffice/TypeDetection/UISort-math.xcu
    org/openoffice/ucb/Configuration-gio.xcu
    org/openoffice/ucb/Configuration-webdav.xcu
    org/openoffice/ucb/Configuration-win.xcu
)

# gb_Configuration_add_localized_datas.

set(OFFICECFG_LOCALIZED_FILES
    org/openoffice/Setup.xcu
    org/openoffice/Office/Accelerators.xcu
    org/openoffice/Office/Common.xcu
    org/openoffice/Office/DataAccess.xcu
    org/openoffice/Office/PresentationMinimizer.xcu
    org/openoffice/Office/PresenterScreen.xcu
    org/openoffice/Office/TableWizard.xcu
    org/openoffice/Office/UI.xcu
    org/openoffice/Office/Embedding.xcu
    org/openoffice/Office/FormWizard.xcu
    org/openoffice/Office/Writer.xcu
    org/openoffice/Office/UI/BasicIDECommands.xcu
    org/openoffice/Office/UI/BibliographyCommands.xcu
    org/openoffice/Office/UI/CalcCommands.xcu
    org/openoffice/Office/UI/ChartCommands.xcu
    org/openoffice/Office/UI/ChartWindowState.xcu
    org/openoffice/Office/UI/DbuCommands.xcu
    org/openoffice/Office/UI/BaseWindowState.xcu
    org/openoffice/Office/UI/WriterFormWindowState.xcu
    org/openoffice/Office/UI/WriterReportWindowState.xcu
    org/openoffice/Office/UI/DbQueryWindowState.xcu
    org/openoffice/Office/UI/DbTableWindowState.xcu
    org/openoffice/Office/UI/DbRelationWindowState.xcu
    org/openoffice/Office/UI/DbBrowserWindowState.xcu
    org/openoffice/Office/UI/DbTableDataWindowState.xcu
    org/openoffice/Office/UI/DrawImpressCommands.xcu
    org/openoffice/Office/UI/Effects.xcu
    org/openoffice/Office/UI/GenericCommands.xcu
    org/openoffice/Office/UI/MathCommands.xcu
    org/openoffice/Office/UI/BasicIDEWindowState.xcu
    org/openoffice/Office/UI/CalcWindowState.xcu
    org/openoffice/Office/UI/DrawWindowState.xcu
    org/openoffice/Office/UI/ImpressWindowState.xcu
    org/openoffice/Office/UI/MathWindowState.xcu
    org/openoffice/Office/UI/Sidebar.xcu
    org/openoffice/Office/UI/StartModuleWindowState.xcu
    org/openoffice/Office/UI/WriterWindowState.xcu
    org/openoffice/Office/UI/XFormsWindowState.xcu
    org/openoffice/Office/UI/WriterGlobalWindowState.xcu
    org/openoffice/Office/UI/WriterWebWindowState.xcu
    org/openoffice/Office/UI/WriterCommands.xcu
    org/openoffice/Office/UI/GenericCategories.xcu
    org/openoffice/Office/UI/ToolbarMode.xcu
)

# --- main.xcd ------------------------------------------------------------------

# postprocess_FILES_main, in three parts because each is written somewhere else.
# The order is the .mk's and it matters: Inet-unixdesktop.xcu has to come after
# Inet.xcu, and VCL-unixdesktop.xcu after VCL.xcu, because a later document in
# the same layer overrides an earlier one.
set(OFFICECFG_MAIN_XCS
    Inet.xcs
    Interaction.xcs
    LDAP.xcs
    Office/Accelerators.xcs
    Office/Addons.xcs
    Office/BasicIDE.xcs
    Office/Calc.xcs
    Office/CalcAddIns.xcs
    Office/Canvas.xcs
    Office/Chart.xcs
    Office/Commands.xcs
    Office/Common.xcs
    Office/Compatibility.xcs
    Office/DataAccess/Drivers.xcs
    Office/DataAccess.xcs
    Office/Draw.xcs
    Office/Embedding.xcs
    Office/Events.xcs
    Office/ExtendedColorScheme.xcs
    Office/ExtensionDependencies.xcs
    Office/ExtensionManager.xcs
    Office/FormWizard.xcs
    Office/Histories.xcs
    Office/Impress.xcs
    Office/Java.xcs
    Office/Jobs.xcs
    Office/Labels.xcs
    Office/Linguistic.xcs
    Office/Logging.xcs
    Office/Math.xcs
    Office/OptionsDialog.xcs
    Office/Paths.xcs
    Office/ProtocolHandler.xcs
    Office/Recovery.xcs
    Office/Scripting.xcs
    Office/Security.xcs
    Office/TableWizard.xcs
    Office/TypeDetection.xcs
    Office/UI/BaseWindowState.xcs
    Office/UI/BasicIDECommands.xcs
    Office/UI/BasicIDEWindowState.xcs
    Office/UI/BibliographyCommands.xcs
    Office/UI/BibliographyWindowState.xcs
    Office/UI/Category.xcs
    Office/UI/ChartCommands.xcs
    Office/UI/ChartWindowState.xcs
    Office/UI/Commands.xcs
    Office/UI/Controller.xcs
    Office/UI/DbBrowserWindowState.xcs
    Office/UI/DbQueryWindowState.xcs
    Office/UI/DbRelationWindowState.xcs
    Office/UI/DbTableDataWindowState.xcs
    Office/UI/DbTableWindowState.xcs
    Office/UI/DbuCommands.xcs
    Office/UI/DrawImpressCommands.xcs
    Office/UI/Factories.xcs
    Office/UI/GenericCategories.xcs
    Office/UI/GenericCommands.xcs
    Office/UI/GlobalSettings.xcs
    Office/UI/Infobar.xcs
    Office/UI/Sidebar.xcs
    Office/UI/StartModuleCommands.xcs
    Office/UI/StartModuleWindowState.xcs
    Office/UI/ToolbarMode.xcs
    Office/UI/WindowContentFactories.xcs
    Office/UI/WindowState.xcs
    Office/UI.xcs
    Office/Update.xcs
    Office/Views.xcs
    Office/Writer.xcs
    Office/WriterWeb.xcs
    Setup.xcs
    System.xcs
    TypeDetection/Filter.xcs
    TypeDetection/GraphicFilter.xcs
    TypeDetection/Misc.xcs
    TypeDetection/Types.xcs
    TypeDetection/UISort.xcs
    UserProfile.xcs
    VCL.xcs
    ucb/Configuration.xcs
    ucb/Hierarchy.xcs
    ucb/Store.xcs
)

set(OFFICECFG_MAIN_XCU
    Inet.xcu
    Interaction.xcu
    Office/Accelerators.xcu
    Office/BasicIDE.xcu
    Office/Calc.xcu
    Office/Canvas.xcu
    Office/Common.xcu
    Office/Compatibility.xcu
    Office/DataAccess.xcu
    Office/Embedding.xcu
    Office/ExtensionDependencies.xcu
    Office/ExtensionManager.xcu
    Office/FormWizard.xcu
    Office/Histories.xcu
    Office/Impress.xcu
    Office/Jobs.xcu
    Office/Logging.xcu
    Office/Math.xcu
    Office/Paths.xcu
    Office/ProtocolHandler.xcu
    Office/Security.xcu
    Office/TableWizard.xcu
    Office/UI/BaseWindowState.xcu
    Office/UI/BasicIDECommands.xcu
    Office/UI/BasicIDEWindowState.xcu
    Office/UI/BibliographyCommands.xcu
    Office/UI/ChartCommands.xcu
    Office/UI/ChartWindowState.xcu
    Office/UI/Controller.xcu
    Office/UI/DbBrowserWindowState.xcu
    Office/UI/DbQueryWindowState.xcu
    Office/UI/DbRelationWindowState.xcu
    Office/UI/DbTableDataWindowState.xcu
    Office/UI/DbTableWindowState.xcu
    Office/UI/DbuCommands.xcu
    Office/UI/DrawImpressCommands.xcu
    Office/UI/Factories.xcu
    Office/UI/GenericCategories.xcu
    Office/UI/GenericCommands.xcu
    Office/UI/Sidebar.xcu
    Office/UI/StartModuleWindowState.xcu
    Office/UI/ToolbarMode.xcu
    Office/UI.xcu
    Office/Views.xcu
    Office/Writer.xcu
    Setup.xcu
    System.xcu
    TypeDetection/UISort.xcu
    UserProfile.xcu
    VCL.xcu
    ucb/Configuration.xcu
)

set(OFFICECFG_MAIN_MOD
    org/openoffice/Office/Embedding-chart.xcu
    org/openoffice/Setup-start.xcu
    org/openoffice/TypeDetection/UISort-calc.xcu
    org/openoffice/TypeDetection/UISort-draw.xcu
    org/openoffice/TypeDetection/UISort-impress.xcu
    org/openoffice/TypeDetection/UISort-math.xcu
    org/openoffice/TypeDetection/UISort-writer.xcu
    org/openoffice/Inet-unixdesktop.xcu
    org/openoffice/Office/Accelerators-unxwnt.xcu
    org/openoffice/Office/Common-UseOOoFileDialogs.xcu
    org/openoffice/Office/Common-unixdesktop.xcu
    org/openoffice/Office/Common-unx.xcu
    org/openoffice/Office/Paths-unixdesktop.xcu
    org/openoffice/Office/Paths-unxwnt.xcu
    org/openoffice/UserProfile-unixdesktop.xcu
    org/openoffice/VCL-unixdesktop.xcu
    org/openoffice/Office/Paths-internallibexttextcatdata.xcu
    org/openoffice/Office/Paths-internallibnumbertextdata.xcu
)

# Eight more are named by the .mk and cannot be built here.  fcfg_base_filters
# and its seven neighbours come from filter/Configuration_filter.mk, and the
# filter module is not ported — it is a 979-line .mk of its own.  They are the
# file-type table: which filter opens what.  Nothing in this build can open a
# document through a filter yet, so the registry is complete without them and
# says so rather than pretending.

# --- instantiating them --------------------------------------------------------

set(OFFICECFG_XCS_OUTPUTS)
set(OFFICECFG_XCU_OUTPUTS)
set(OFFICECFG_XCM_OUTPUTS)
foreach (_name IN LISTS OFFICECFG_SCHEMA_FILES)
    librestudio_officecfg_xcs("${_name}")
endforeach ()
foreach (_name IN LISTS OFFICECFG_DATA_FILES)
    librestudio_officecfg_xcu("${_name}")
endforeach ()
foreach (_name IN LISTS OFFICECFG_LOCALIZED_FILES)
    librestudio_officecfg_xcu("${_name}")
endforeach ()
foreach (_name IN LISTS OFFICECFG_SPOOL_FILES)
    librestudio_officecfg_xcm("${_name}")
endforeach ()

# --- packing -------------------------------------------------------------------

# The list packregistry.xslt reads.  Upstream writes it from the makefile with
# $(file ...); here it is written when CMake configures, which is the same thing
# said earlier — the custom command depends on the file, so changing any list
# above rewrites it and ninja sees a newer input.
# The three lists are written the way the .mk writes them, which is relative to
# a different root each time and is not the way the targets are addressed here.
# postprocess_XCS is XcsTarget/org/openoffice and postprocess_XCU is
# XcuDataTarget/officecfg/registry/data/org/openoffice — those two have the
# org/openoffice prefix inside the variable — while the entries below are
# relative to it.  The module list is the other way round: postprocess_MOD stops
# at XcuModuleTarget/officecfg/registry/data and its entries carry the prefix
# themselves.  Hence the prefix added to the first two and not the third.
set(OFFICECFG_MAIN_FILES)
foreach (_name IN LISTS OFFICECFG_MAIN_XCS)
    list(APPEND OFFICECFG_MAIN_FILES
        "${OFFICECFG_XCS_BUILD}/org/openoffice/${_name}")
endforeach ()
foreach (_name IN LISTS OFFICECFG_MAIN_XCU)
    list(APPEND OFFICECFG_MAIN_FILES
        "${OFFICECFG_XCU_BUILD}/org/openoffice/${_name}")
endforeach ()
foreach (_name IN LISTS OFFICECFG_MAIN_MOD)
    list(APPEND OFFICECFG_MAIN_FILES "${OFFICECFG_XCM_BUILD}/${_name}")
endforeach ()

set(OFFICECFG_MAIN_LIST "${OFFICECFG_REGISTRY_BUILD}/main.list")
set(_list "<list>")
foreach (_file IN LISTS OFFICECFG_MAIN_FILES)
    string(APPEND _list "<filename>${_file}</filename>")
endforeach ()
string(APPEND _list "</list>")
file(WRITE "${OFFICECFG_MAIN_LIST}" "${_list}")

# The substitutions packregistry.xslt leaves behind, as postprocess_main_SED
# writes them.  A file rather than a -e list, because the values contain the
# ${...} that has to survive to sed and a command line would have eaten them.
set(OFFICECFG_MAIN_SED "${OFFICECFG_REGISTRY_BUILD}/main.sed")
file(WRITE "${OFFICECFG_MAIN_SED}"
"s,\${ABOUTBOXPRODUCTVERSION},${OFFICECFG_VERSION}${OFFICECFG_VERSION_SUFFIX},g
s,\${ABOUTBOXPRODUCTVERSIONSUFFIX},${OFFICECFG_VERSION_SUFFIX},g
s,\${PRODUCTEXTENSION},.${OFFICECFG_VERSION}${OFFICECFG_VERSION_SUFFIX},g
s,\${PRODUCTNAME},${LIBRESTUDIO_PRODUCT_NAME},g
s,\${PRODUCTVERSION},${OFFICECFG_VERSION},g
s,\${OOOVENDOR},${LIBRESTUDIO_VENDOR},g
s,\${STARTCENTER_INFO_URL},${LIBRESTUDIO_INFO_URL},g
s,\${PRIVACY_POLICY_URL},${LIBRESTUDIO_PRIVACY_POLICY_URL},g
s,\${SYSTEM_LIBEXTTEXTCAT_DATA},,g
s,\${SYSTEM_LIBNUMBERTEXT_DATA},,g
")

# The registry itself, in the one place configmgr is told to look for it: the
# xcsxcu layer in unorc points at share/registry, and it reads every .xcd in
# there.  See librestudio_generate_unorc.
add_custom_command(
    OUTPUT "${LIBRESTUDIO_REGISTRY_DIR}/main.xcd"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${LIBRESTUDIO_REGISTRY_DIR}"
    COMMAND sh -c "${XSLTPROC_EXECUTABLE} --nonet '${OFFICECFG_PACKREGISTRY}' '${OFFICECFG_MAIN_LIST}' | sed -f '${OFFICECFG_MAIN_SED}' > '${LIBRESTUDIO_REGISTRY_DIR}/main.xcd'"
    DEPENDS "${OFFICECFG_MAIN_LIST}" "${OFFICECFG_MAIN_SED}"
            "${OFFICECFG_PACKREGISTRY}" ${OFFICECFG_MAIN_FILES}
    COMMENT "packregistry.xslt: main.xcd"
    VERBATIM)

# The three output lists are named here as well as inside main.xcd's own
# dependencies, and that is not redundant: CMake keeps a custom command's rule
# only when a *target* in the directory names one of its outputs.  main.xcd
# depends on these files, but a dependency from one custom command to another's
# output does not count — without this, four hundred rules are dropped and ninja
# reports the first missing file as having no rule to make it.
add_custom_target(officecfg_registry ALL
    DEPENDS "${LIBRESTUDIO_REGISTRY_DIR}/main.xcd"
            ${OFFICECFG_XCS_OUTPUTS} ${OFFICECFG_XCU_OUTPUTS}
            ${OFFICECFG_XCM_OUTPUTS})

