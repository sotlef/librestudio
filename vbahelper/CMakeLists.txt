# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# vbahelper — the two libraries that make up the shared VBA-compatibility
# layer used by every application's VBA object model: vbahelper itself (the
# base classes -- ScVbaApplicationBase, CommandBar[s], the Shape/ShapeRange
# wrappers, the ColorFormat/LineFormat/FillFormat/PictureFormat property
# helpers, VbaEventsHelperBase, ... -- Library_vbahelper.mk) and msforms
# (the ooo.vba.msforms.* UserForm control wrappers -- Button, CheckBox,
# ComboBox, Frame, Image, Label, ListBox, MultiPage, ProgressBar,
# RadioButton, ScrollBar, SpinButton, TextBox, ToggleButton, plus the
# UserForm itself -- Library_msforms.mk), which links vbahelper.
#
# Both gb_Library_use_api udkapi/offapi/oovbaapi (heavily used throughout:
# every ooo/vba/... header comes from oovbaapi), so both need
# oovbaapi_normal_HEADERS_INCLUDE_DIR/oovbaapi_normal_headers the same way
# basic/CMakeLists.txt (sb) and scripting/CMakeLists.txt (vbaevents) already
# consume them. Neither Library_*.mk calls gb_Library_set_include -- all
# their own headers are either under the shared include/vbahelper (already
# on every target's include path via LibreStudioPlatform.cmake) or reached
# by same-directory quoted #include, and vbahelper/inc/ holds only the
# precompiled-header pair for each library, not real headers.
# gb_Library_set_precompiled_header is a build-speed knob with no CMake
# equivalent and is skipped, as elsewhere in this port.
#
# vbahelper has no gb_Library_set_componentfile call (it is commented out in
# Library_vbahelper.mk, and no vbahelper.component file exists in the tree)
# -- it exports no UNO services of its own, only the base classes
# application-specific VBA libraries derive from -- so no
# librestudio_add_component() call is made for it, matching singleprov's
# precedent in scripting/CMakeLists.txt. msforms.component
# (vbahelper/util/msforms.component) has no <optional/> markers, so no
# IMPLEMENTATION filtering is needed for it.
#
# Both libraries are registered in the OOO layer, matching Repository.mk's
# gb_Helper_register_libraries_for_install(OOOLIBS,ooo,...) (both listed
# there under gb_Helper_optional(SCRIPTING,...), which this port always
# enables).
#
# Source lists, includes, defines and library sets below are transcribed
# from each library's Library_*.mk; neither has any OS-conditional source or
# library.
#
# Module_vbahelper.mk wraps its gb_Module_add_targets list (Library_vbahelper
# + Library_msforms, both targets in this file) in
# `ifneq ($(filter SCRIPTING,$(BUILD_TYPE)),)`, so the whole body below is
# wrapped in `if (ENABLE_SCRIPTING)`, which defaults ON.
if (ENABLE_SCRIPTING)

set(VBAHELPER_SOURCES
    vbahelper/source/vbahelper/vbaapplicationbase
    vbahelper/source/vbahelper/vbacolorformat
    vbahelper/source/vbahelper/vbacommandbar
    vbahelper/source/vbahelper/vbacommandbarcontrol
    vbahelper/source/vbahelper/vbacommandbarcontrols
    vbahelper/source/vbahelper/vbacommandbarhelper
    vbahelper/source/vbahelper/vbacommandbars
    vbahelper/source/vbahelper/vbadialogbase
    vbahelper/source/vbahelper/vbadialogsbase
    vbahelper/source/vbahelper/vbadocumentbase
    vbahelper/source/vbahelper/vbadocumentsbase
    vbahelper/source/vbahelper/vbaeventshelperbase
    vbahelper/source/vbahelper/vbafillformat
    vbahelper/source/vbahelper/vbafontbase
    vbahelper/source/vbahelper/vbaglobalbase
    vbahelper/source/vbahelper/vbahelper
    vbahelper/source/vbahelper/vbalineformat
    vbahelper/source/vbahelper/vbapagesetupbase
    vbahelper/source/vbahelper/vbapictureformat
    vbahelper/source/vbahelper/vbapropvalue
    vbahelper/source/vbahelper/vbashape
    vbahelper/source/vbahelper/vbashaperange
    vbahelper/source/vbahelper/vbashapes
    vbahelper/source/vbahelper/vbatextframe
    vbahelper/source/vbahelper/vbawindowbase
)

librestudio_expand_cxx(VBAHELPER_SOURCES)
add_library(vbahelper SHARED ${VBAHELPER_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(vbahelper PRIVATE
        "${CMAKE_SOURCE_DIR}/vbahelper/inc/pch/precompiled_vbahelper.hxx")
    target_compile_definitions(vbahelper PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(vbahelper PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(vbahelper
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers)

target_compile_definitions(vbahelper PRIVATE VBAHELPER_DLLIMPLEMENTATION)

target_link_libraries(vbahelper PRIVATE
    comphelper cppu cppuhelper msfilter sal sb sfx svl svt svxcore tk tl utl vcl
    Boost::boost)

librestudio_set_library_output_name(vbahelper vbahelper OOO)

install(TARGETS vbahelper LIBRARY DESTINATION program)

# --- msforms ------------------------------------------------------------

set(MSFORMS_SOURCES
    vbahelper/source/msforms/vbabutton
    vbahelper/source/msforms/vbacheckbox
    vbahelper/source/msforms/vbacombobox
    vbahelper/source/msforms/vbacontrol
    vbahelper/source/msforms/vbacontrols
    vbahelper/source/msforms/vbaframe
    vbahelper/source/msforms/vbaimage
    vbahelper/source/msforms/vbalabel
    vbahelper/source/msforms/vbalistbox
    vbahelper/source/msforms/vbalistcontrolhelper
    vbahelper/source/msforms/vbamultipage
    vbahelper/source/msforms/vbanewfont
    vbahelper/source/msforms/vbapages
    vbahelper/source/msforms/vbaprogressbar
    vbahelper/source/msforms/vbaradiobutton
    vbahelper/source/msforms/vbascrollbar
    vbahelper/source/msforms/vbaspinbutton
    vbahelper/source/msforms/vbasystemaxcontrol
    vbahelper/source/msforms/vbatextbox
    vbahelper/source/msforms/vbatogglebutton
    vbahelper/source/msforms/vbauserform
)

librestudio_expand_cxx(MSFORMS_SOURCES)
add_library(msforms SHARED ${MSFORMS_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(msforms PRIVATE
        "${CMAKE_SOURCE_DIR}/vbahelper/inc/pch/precompiled_msforms.hxx")
    target_compile_definitions(msforms PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(msforms PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(msforms
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers)

target_link_libraries(msforms PRIVATE
    comphelper cppu cppuhelper sal sb sfx svl svt svx svxcore tk tl vbahelper vcl
    Boost::boost)

librestudio_add_component(msforms
    COMPONENT "${CMAKE_SOURCE_DIR}/vbahelper/util/msforms.component"
    LIBRARY   msforms
    LAYER     OOO)

librestudio_set_library_output_name(msforms msforms OOO)

install(TARGETS msforms LIBRARY DESTINATION program)

endif ()
