# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# fpicker (module fpicker) -- one library: fps_office, the cross-platform
# (VCL-based) file/folder picker services used whenever no native platform
# picker is available. Source list, include and dependencies are transcribed
# from Library_fps_office.mk.
#
# Excluded from Module_fpicker.mk:
#  - Library_fps_aqua (ifeq ($(OS),MACOSX)): the Cocoa-native file picker
#    (fpicker/source/aqua) -- macOS-only, not applicable to this Linux-only
#    port.
#  - Library_fps (ifeq ($(OS),WNT)): the Win32-native file picker
#    (fpicker/source/win32) -- Windows-only, likewise excluded.
#  - UIConfig_fps, AllLangMoTarget_fps, and the
#    CppunitTest_fpicker_dialogs_test screenshot target: UI-config/
#    localization/test targets outside this port's scope, same as every
#    other module.

set(FPS_OFFICE_SOURCES
    fpicker/source/office/asyncfilepicker
    fpicker/source/office/autocmpledit
    fpicker/source/office/breadcrumb
    fpicker/source/office/commonpicker
    fpicker/source/office/contentenumeration
    fpicker/source/office/fileview
    fpicker/source/office/foldertree
    fpicker/source/office/fpinteraction
    fpicker/source/office/fpsmartcontent
    fpicker/source/office/iodlg
    fpicker/source/office/iodlgimp
    fpicker/source/office/OfficeControlAccess
    fpicker/source/office/OfficeFilePicker
    fpicker/source/office/OfficeFolderPicker
    fpicker/source/office/PlacesListBox
    fpicker/source/office/RemoteFilesDialog
)

librestudio_expand_cxx(FPS_OFFICE_SOURCES)
add_library(fps_office SHARED ${FPS_OFFICE_SOURCES})

target_include_directories(fps_office PRIVATE
    "${CMAKE_SOURCE_DIR}/fpicker/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(fps_office officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(fps_office PRIVATE
    comphelper cppu cppuhelper sal salhelper svl svt tk tl ucbhelper utl vcl
    i18nlangtag
    Boost::boost)

# fpicker/source/office/fps_office.component's third
# gb_Library_set_componentfile argument is "services", i.e. the office layer.
librestudio_add_component(fps_office
    COMPONENT "${CMAKE_SOURCE_DIR}/fpicker/source/office/fps_office.component"
    LIBRARY   fps_office
    LAYER     OOO)

librestudio_set_library_output_name(fps_office fps_office OOO)

install(TARGETS fps_office LIBRARY DESTINATION program)
