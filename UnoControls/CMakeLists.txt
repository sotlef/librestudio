# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# UnoControls (module ctl) — a small set of UNO controls (progress bar,
# progress monitor, status indicator, frame control) plus the base
# container-control/multiplexer helpers they share. Source list from
# Library_ctl.mk. No *_DLLIMPLEMENTATION define: the .mk never calls
# gb_Library_add_defs for ctl.

set(CTL_SOURCES
    UnoControls/source/base/basecontainercontrol
    UnoControls/source/base/basecontrol
    UnoControls/source/base/multiplexer
    UnoControls/source/controls/OConnectionPointContainerHelper
    UnoControls/source/controls/OConnectionPointHelper
    UnoControls/source/controls/framecontrol
    UnoControls/source/controls/progressbar
    UnoControls/source/controls/progressmonitor
    UnoControls/source/controls/statusindicator
)

librestudio_expand_cxx(CTL_SOURCES)
add_library(ctl SHARED ${CTL_SOURCES})

target_include_directories(ctl PRIVATE
    "${CMAKE_SOURCE_DIR}/UnoControls/inc"
    "${CMAKE_SOURCE_DIR}/UnoControls/source/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(ctl officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(ctl PRIVATE
    comphelper cppu cppuhelper sal tk tl)

librestudio_add_component(ctl
    COMPONENT "${CMAKE_SOURCE_DIR}/UnoControls/util/ctl.component"
    LIBRARY   ctl
    LAYER     OOO)

librestudio_set_library_output_name(ctl ctl OOO)

install(TARGETS ctl LIBRARY DESTINATION program)
