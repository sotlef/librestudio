# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# forms (module forms) — the "frm" library implementing the UNO form
# component model (form controls such as Button/CheckBox/Edit/Grid/ListBox/
# ..., the DatabaseForm and its runtime, the richtext control, the
# NavigationToolBar, and the XForms (xforms) implementation). Source list,
# includes and library set are transcribed from Library_frm.mk.

set(FRM_SOURCES
    forms/source/component/Button
    forms/source/component/cachedrowset
    forms/source/component/CheckBox
    forms/source/component/clickableimage
    forms/source/component/cloneable
    forms/source/component/errorbroadcaster
    forms/source/component/Columns
    forms/source/component/ComboBox
    forms/source/component/Currency
    forms/source/component/Date
    forms/source/component/DatabaseForm
    forms/source/component/EditBase
    forms/source/component/Edit
    forms/source/component/entrylisthelper
    forms/source/component/EventThread
    forms/source/component/File
    forms/source/component/Filter
    forms/source/component/findpos
    forms/source/component/FixedText
    forms/source/component/FormattedField
    forms/source/component/FormattedFieldWrapper
    forms/source/component/FormComponent
    forms/source/component/formcontrolfont
    forms/source/component/FormsCollection
    forms/source/component/Grid
    forms/source/component/GroupBox
    forms/source/component/GroupManager
    forms/source/component/Hidden
    forms/source/component/ImageButton
    forms/source/component/ImageControl
    forms/source/component/imgprod
    forms/source/component/ListBox
    forms/source/component/navigationbar
    forms/source/component/Numeric
    forms/source/component/Pattern
    forms/source/component/propertybaghelper
    forms/source/component/RadioButton
    forms/source/component/refvaluecomponent
    forms/source/component/scrollbar
    forms/source/component/spinbutton
    forms/source/component/Time
    forms/source/helper/commandimageprovider
    forms/source/helper/controlfeatureinterception
    forms/source/helper/formnavigation
    forms/source/helper/resettable
    forms/source/helper/urltransformer
    forms/source/helper/windowstateguard
    forms/source/misc/componenttools
    forms/source/misc/InterfaceContainer
    forms/source/misc/limitedformats
    forms/source/misc/property
    forms/source/resource/frm_resource
    forms/source/richtext/attributedispatcher
    forms/source/richtext/clipboarddispatcher
    forms/source/richtext/featuredispatcher
    forms/source/richtext/parametrizedattributedispatcher
    forms/source/richtext/richtextcontrol
    forms/source/richtext/richtextengine
    forms/source/richtext/richtextimplcontrol
    forms/source/richtext/richtextmodel
    forms/source/richtext/richtextunowrapper
    forms/source/richtext/richtextvclcontrol
    forms/source/richtext/richtextviewport
    forms/source/richtext/rtattributehandler
    forms/source/richtext/specialdispatchers
    forms/source/runtime/formoperations
    forms/source/solar/component/navbarcontrol
    forms/source/solar/control/navtoolbar
    forms/source/xforms/binding
    forms/source/xforms/boolexpression
    forms/source/xforms/computedexpression
    forms/source/xforms/convert
    forms/source/xforms/datatyperepository
    forms/source/xforms/datatypes
    forms/source/xforms/enumeration
    forms/source/xforms/mip
    forms/source/xforms/model
    forms/source/xforms/model_ui
    forms/source/xforms/pathexpression
    forms/source/xforms/propertysetbase
    forms/source/xforms/resourcehelper
    forms/source/xforms/submission
    forms/source/xforms/submission/replace
    forms/source/xforms/submission/serialization_app_xml
    forms/source/xforms/submission/serialization_urlencoded
    forms/source/xforms/submission/submission_get
    forms/source/xforms/submission/submission_post
    forms/source/xforms/submission/submission_put
    forms/source/xforms/unohelper
    forms/source/xforms/xformsevent
    forms/source/xforms/xforms_services
    forms/source/xforms/xmlhelper
    forms/source/xforms/xpathlib/extension
    forms/source/xforms/xpathlib/xpathlib
)

librestudio_expand_cxx(FRM_SOURCES)
add_library(frm SHARED ${FRM_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(frm PRIVATE
        "${CMAKE_SOURCE_DIR}/forms/inc/pch/precompiled_frm.hxx")
    target_compile_definitions(frm PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(frm PRIVATE
    "${CMAKE_SOURCE_DIR}/forms/inc"
    "${CMAKE_SOURCE_DIR}/forms/source/inc"
    "${CMAKE_SOURCE_DIR}/forms/source/solar/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}")
# Library_frm.mk's gb_Library_use_api lists offapi/oovbaapi/udkapi -- the
# oovbaapi normal-variant headers dir/dependency was missing here (only
# udkapi/offapi were wired), same use_api trio scripting/CMakeLists.txt and
# basic/CMakeLists.txt already resolve to oovbaapi_normal_HEADERS_INCLUDE_DIR
# + oovbaapi_normal_headers.
add_dependencies(frm officecfg_headers udkapi_normal_headers offapi_normal_headers
    oovbaapi_normal_headers)

target_link_libraries(frm PRIVATE
    comphelper cppu cppuhelper editeng i18nlangtag sal salhelper sfx
    svl svt svxcore tk tl ucbhelper utl vcl
    Boost::boost
    ICU::i18n
    ICU::uc
    LibXml2::LibXml2)

# $(call gb_Helper_optional,DBCONNECTIVITY,dbtools) from Library_frm.mk's
# use_libraries -- the only DBCONNECTIVITY-gated piece in this library (no
# source files are gated).
if (ENABLE_DBCONNECTIVITY)
    target_link_libraries(frm PRIVATE dbtools)
endif (ENABLE_DBCONNECTIVITY)

librestudio_add_component(frm
    COMPONENT "${CMAKE_SOURCE_DIR}/forms/util/frm.component"
    LIBRARY   frm
    LAYER     OOO)

librestudio_set_library_output_name(frm frm OOO)

install(TARGETS frm LIBRARY DESTINATION program)

# CppunitTest_forms_unit.mk: registered in forms/CppunitTest_deferred.cmake,
# NOT here, include()'d from the root CMakeLists.txt only after
# program_services_rdb exists (use_rdb(services)) and test/unotest are
# add_subdirectory()'d (use_configuration/use_ure/use_vcl), all well after
# forms's own position. See that file for the full per-test detail and its
# include() call site in the root CMakeLists.txt for why it sits where it
# does.
#
# AllLangMoTarget_frm (Module_forms.mk's l10n target) and JunitTest_forms_
# unoapi_1..4/JunitTest_forms_complex (its subsequentcheck targets) are
# localization/Java-test targets outside this port's scope, same as every
# other module's AllLangMoTarget_*/JunitTest_* (see e.g. uui/CMakeLists.txt's
# own comment on AllLangMoTarget_uui).

# forms/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/forms/CppunitTest_deferred.cmake")
