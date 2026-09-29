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
#  - UIConfig_fps is ported below (its 4 .ui files, real packaging +
#    build-time mirror). AllLangMoTarget_fps (localization) remains out of
#    this port's scope, same as every other module.
#  - CppunitTest_fpicker_dialogs_test (the gb_Module_add_screenshot_targets
#    entry) is wired for real too, but deferred to fpicker/CppunitTest_
#    deferred.cmake -- its gb_CppunitTest_use_rdb(services) needs the
#    program_services_rdb target, which does not exist yet at this file's own
#    add_subdirectory() position (root CMakeLists.txt); see that file for the
#    full detail.

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

# --- UIConfig_fps ----------------------------------------------------------------
#
# UIConfig_fps.mk: 4 .ui files under fpicker/uiconfig/ui packaged into
# share/config/soffice.cfg/fps/ui -- gb_UIConfig_UIConfig's own destination
# convention (gb_UIConfig_INSTDIR/$(1)/ui, $(1) = "fps", the UIConfig name
# itself; see uui/CMakeLists.txt's own UIConfig_uui block for the full
# derivation, including the note on why "the module folder name" is not
# always the right destination). Verified directly against fpicker's own
# dialog sources, not assumed: breadcrumb.cxx, RemoteFilesDialog.cxx and
# iodlg.cxx (2 dialogs) all load from u"fps/ui/...", e.g. iodlg.cxx's
# u"fps/ui/explorerfiledialog.ui"_ustr -- so "fps" is the correct destination
# folder, matching the UIConfig name exactly.
#
# Same real build-time-mirror mechanism as UIConfig_vcl/UIConfig_svt/
# UIConfig_uui (see any of those for the full reasoning): FPS_UICONFIG_UI_FILES
# holds the list once, install(FILES ...) does the `cmake --install`
# packaging, and the per-file add_custom_command + one add_custom_target(...
# ALL DEPENDS ...) gives cmake/LibreStudioTest.cmake's UICONFIG option a real
# "fps_uiconfig" target to depend on for fpicker_dialogs_test (whose own
# gb_CppunitTest_use_uiconfigs(fpicker_dialogs_test,fps) call names "fps", not
# "fpicker").
set(FPS_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/breadcrumb.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/explorerfiledialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/foldernamedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/remotefilesdialog.ui"
)

install(FILES ${FPS_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/fps/ui)

set(_fps_uiconfig_outputs "")
foreach (_fps_uiconfig_src IN LISTS FPS_UICONFIG_UI_FILES)
    get_filename_component(_fps_uiconfig_name "${_fps_uiconfig_src}" NAME)
    set(_fps_uiconfig_dest "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/fps/ui/${_fps_uiconfig_name}")
    add_custom_command(
        OUTPUT "${_fps_uiconfig_dest}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/fps/ui"
        COMMAND ${CMAKE_COMMAND} -E copy "${_fps_uiconfig_src}" "${_fps_uiconfig_dest}"
        DEPENDS "${_fps_uiconfig_src}"
        COMMENT "Copying ${_fps_uiconfig_name} into build-tree soffice.cfg/fps/ui"
        VERBATIM)
    list(APPEND _fps_uiconfig_outputs "${_fps_uiconfig_dest}")
endforeach ()
unset(_fps_uiconfig_src)
unset(_fps_uiconfig_name)
unset(_fps_uiconfig_dest)

add_custom_target(fps_uiconfig ALL DEPENDS ${_fps_uiconfig_outputs})
unset(_fps_uiconfig_outputs)

# fpicker/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/fpicker/CppunitTest_deferred.cmake")
