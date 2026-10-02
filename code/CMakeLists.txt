# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# "Code" -- a future Visual-Studio-like IDE app, recorded in memory as
# project_code_ide. This is original LibreStudio code, not a port of any
# upstream gbuild module -- there is no Module_code.mk anywhere to
# transcribe, so it has no row in PORTING_STATUS.md.
#
# v2: real sfx2/framework shell (SfxObjectShell/SfxViewShell pair, declarative
# menubar/toolbar/statusbar from uiconfig XML, dispatched through a real
# SfxDispatcher via a compiled .sdi slot table) instead of v1's hand-coded
# VCL MenuBar/ToolBox/StatusBar. Modelled directly on basctl -- the only
# other SFX app in this tree -- but simplified: code::DocShell is
# constructed directly (not via a dlopen()able UNO component) and handed to
# the FrameLoader with an already-built model, the same way basctl's own
# SID_BASICIDE_APPEAR does (sfx2/source/appl/appserv.cxx), so "code" stays a
# plain add_executable() -- no librestudio_add_component() needed, it is not
# a UNO service. Confirmed empirically feasible outside soffice.bin by
# building and running svx's own gengal executable, which does the same
# "link sfx directly, call SfxApplication::GetOrCreate() from main()" thing.

librestudio_add_sdi_target(code_sdi
    SDI        "${CMAKE_SOURCE_DIR}/code/sdi/codeslots.sdi"
    EXPORTS    "${CMAKE_SOURCE_DIR}/code/sdi/code.sdi"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/SdiTarget/code/sdi"
    INCLUDES   "${CMAKE_SOURCE_DIR}/code/inc" "${CMAKE_SOURCE_DIR}/svx/sdi" "${CMAKE_SOURCE_DIR}/sfx2/sdi")

add_executable(code
    "${CMAKE_SOURCE_DIR}/code/source/codeapp.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/codedocshell.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/codeviewshell.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/editormdipanel.cxx"
    # The Output dock's own SfxChildWindow/SfxDockingWindow/IDeckHost
    # trio -- unlike codesidebar's panel factory below, these are
    # constructed directly (RegisterChildWindow/ShowChildWindow in
    # codeapp.cxx), not UNO-discovered, so they link straight into the
    # executable the same way codeviewshell.cxx/editorarea.cxx do.
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/outputchildwindow.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/outputcontroller.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/outputdockingwindow.cxx")

target_compile_definitions(code PRIVATE VCL_INTERNALS)

target_include_directories(code PRIVATE
    "${CMAKE_SOURCE_DIR}/code/inc"
    "${CMAKE_SOURCE_DIR}/code/source"
    "${CMAKE_SOURCE_DIR}/code/sdi"
    "${CMAKE_SOURCE_DIR}/vcl/inc"
    "${code_sdi_INCLUDE_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(code code_sdi udkapi_normal_headers offapi_normal_headers gcc3_uno officecfg_registry officecfg_headers program_services_rdb
    # Runtime-only UNO components/build-tree assets "code" never links
    # against but silently needs to actually run without crashing or
    # missing content -- each one found and built ad hoc during a single
    # debugging session by reproducing the crash and reading the resulting
    # exception/log. Wiring them here means a plain `cmake --build . --
    # target code` from a fresh configure rebuilds all of them too, instead
    # of needing every one of those crashes to be rediscovered by hand
    # again next time (see project memory for the full list of how each
    # one was found):
    #  - uui:           "Open" dialog's InteractionHandler
    #  - filterconfig:  "Open" dialog's type/filter detection
    #  - i18nsearch:    toolbar Customize dialog's search box
    #  - vcl_uiconfig:  vcl's own generic .ui files (combobox.ui et al.),
    #                   used internally by ordinary widgets, not just code's
    #                   own uiconfig
    #  - package2:      ZipFileAccess, which vcl's own ImageTree needs to
    #                   read *any* icon out of *any* icon-theme zip -- so
    #                   this one was silently breaking every icon in the
    #                   app, not just the sidebar's, the whole session
    #  - cui:           Tools > Options (and every other cui-hosted dialog)
    #                   -- dlopened on first use; found via a crash after a
    #                   weld::Builder vtable change (new weld_mdi() slot)
    #                   left a stale cui mis-dispatching virtual calls,
    #                   because `ninja code` never rebuilt it
    #  - vclplug_gtk3/  the actual GUI backends: dlopened by vcl at startup
    #    vclplug_qt6:   based on SAL_USE_VCLPLUGIN/desktop detection, never
    #                   linked -- without them here, `ninja code` happily
    #                   reports "no work to do" after an edit inside
    #                   vcl/qt5/ or vcl/unx/gtk3/
    uui filterconfig i18nsearch vcl_uiconfig package2 cui)

if (ENABLE_GTK3)
    add_dependencies(code vclplug_gtk3)
endif()
if (ENABLE_QT6)
    add_dependencies(code vclplug_qt6)
endif()

target_link_libraries(code PRIVATE
    comphelper cppu cppuhelper sal sfx fwk svxcore svx svl svt tk tl ucbhelper utl vcl vclmain)

# Same reason every vcl workbench program above gets one: UNO/configmgr
# bootstrap reads CONFIGURATION_LAYERS from "<program name>rc", not unorc.
librestudio_generate_unorc("${LIBRESTUDIO_PROGRAM_DIR}/coderc")

install(TARGETS code RUNTIME DESTINATION program)

# --- codesidebar -----------------------------------------------------------
#
# The real Sidebar's ResourceManager (sfx2/source/sidebar/ResourceManager.cxx)
# instantiates a panel's content dynamically, by UNO service name, the
# moment the user actually shows that panel -- unlike code::DocShell
# (constructed directly in codeapp.cxx, never UNO-discovered), a panel
# factory has to be a real, dlopen()able UNO component, so unlike "code"
# itself this is a normal librestudio_add_component()-registered SHARED
# library, discovered at runtime via services.rdb/main.xcd exactly like
# uui/filterconfig -- "code" (the executable) does not link it.
add_library(codesidebar SHARED
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/codepanelfactory.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/errorlistpanel.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/outputlogpanel.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/solutionpanel.cxx")

target_include_directories(codesidebar PRIVATE
    "${CMAKE_SOURCE_DIR}/code/source"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(codesidebar udkapi_normal_headers offapi_normal_headers)

target_link_libraries(codesidebar PRIVATE
    comphelper cppu cppuhelper sal sfx svt tl ucbhelper vcl)

librestudio_add_component(codesidebar
    COMPONENT "${CMAKE_SOURCE_DIR}/code/util/codesidebar.component"
    LIBRARY   codesidebar
    LAYER     OOO)

librestudio_set_library_output_name(codesidebar codesidebar OOO)

install(TARGETS codesidebar LIBRARY DESTINATION program)

# Sibling runtime-only dependency (see the long comment on "code"'s own
# add_dependencies() above) -- not something "code" links against, but
# building it is what makes the Solution sidebar panel actually work
# instead of silently doing nothing.
add_dependencies(code codesidebar)

# --- UIConfig_code -------------------------------------------------------
#
# Mirrors basctl/CMakeLists.txt's own UIConfig_basicide block: installs
# code/uiconfig/code/{menubar,toolbar,statusbar}/*.xml into
# share/config/soffice.cfg/modules/Code/{menubar,toolbar,statusbar} -- the
# exact directory framework's PresetHandler resolves at runtime from
# ooSetupFactoryShortName=Code (officecfg/registry/data/org/openoffice/
# Setup.xcu's own new "com.sun.star.code.CodeDocument" node).
set(CODE_UICONFIG_MENUBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/menubar/menubar.xml"
)
set(CODE_UICONFIG_TOOLBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/toolbar/standardbar.xml"
)
set(CODE_UICONFIG_STATUSBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/statusbar/statusbar.xml"
)

install(FILES ${CODE_UICONFIG_MENUBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/Code/menubar)
install(FILES ${CODE_UICONFIG_TOOLBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/Code/toolbar)
install(FILES ${CODE_UICONFIG_STATUSBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/Code/statusbar)

set(_code_uiconfig_outputs "")
foreach (_code_uiconfig_pair
        "CODE_UICONFIG_MENUBAR_FILES:menubar"
        "CODE_UICONFIG_TOOLBAR_FILES:toolbar"
        "CODE_UICONFIG_STATUSBAR_FILES:statusbar")
    string(REPLACE ":" ";" _code_uiconfig_pair "${_code_uiconfig_pair}")
    list(GET _code_uiconfig_pair 0 _code_uiconfig_var)
    list(GET _code_uiconfig_pair 1 _code_uiconfig_category)
    foreach (_code_uiconfig_src IN LISTS ${_code_uiconfig_var})
        get_filename_component(_code_uiconfig_name "${_code_uiconfig_src}" NAME)
        set(_code_uiconfig_dest
            "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/Code/${_code_uiconfig_category}/${_code_uiconfig_name}")
        add_custom_command(
            OUTPUT "${_code_uiconfig_dest}"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/Code/${_code_uiconfig_category}"
            COMMAND ${CMAKE_COMMAND} -E copy "${_code_uiconfig_src}" "${_code_uiconfig_dest}"
            DEPENDS "${_code_uiconfig_src}"
            COMMENT "Copying ${_code_uiconfig_name} into build-tree soffice.cfg/modules/Code/${_code_uiconfig_category}"
            VERBATIM)
        list(APPEND _code_uiconfig_outputs "${_code_uiconfig_dest}")
    endforeach ()
endforeach ()

# solutionpanel.ui isn't module uiconfig (menubar/toolbar/statusbar under
# .../modules/Code/) -- it's a weld .ui a sidebar panel loads by a plain
# "code/ui/<file>.ui" path (PanelLayout's rUIXMLDescription argument,
# code/source/sidebar/solutionpanel.cxx), resolved by AllSettings::
# GetUIRootDir() (vcl/source/window/builder.cxx) straight against
# share/config/soffice.cfg/ -- same "<module>/ui/" convention every other
# module's own weld .ui files use (e.g. svx/ui/sidebarempty.ui).
set(CODE_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/editordocumentcontent.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/editormdi.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/errorlistpanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/outputpanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/solutionpanel.ui"
)
install(FILES ${CODE_UI_FILES} DESTINATION share/config/soffice.cfg/code/ui)
foreach (_code_ui_src IN LISTS CODE_UI_FILES)
    get_filename_component(_code_ui_name "${_code_ui_src}" NAME)
    set(_code_ui_dest "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/code/ui/${_code_ui_name}")
    add_custom_command(
        OUTPUT "${_code_ui_dest}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/code/ui"
        COMMAND ${CMAKE_COMMAND} -E copy "${_code_ui_src}" "${_code_ui_dest}"
        DEPENDS "${_code_ui_src}"
        COMMENT "Copying ${_code_ui_name} into build-tree soffice.cfg/code/ui"
        VERBATIM)
    list(APPEND _code_uiconfig_outputs "${_code_ui_dest}")
endforeach ()

add_custom_target(code_uiconfig ALL DEPENDS ${_code_uiconfig_outputs})
