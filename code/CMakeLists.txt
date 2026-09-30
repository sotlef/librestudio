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
    INCLUDES   "${CMAKE_SOURCE_DIR}/svx/sdi" "${CMAKE_SOURCE_DIR}/sfx2/sdi")

add_executable(code
    "${CMAKE_SOURCE_DIR}/code/source/codeapp.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/codedocshell.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/codeviewshell.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/solutionexplorer.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/editorarea.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/outputpanel.cxx")

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
add_dependencies(code code_sdi udkapi_normal_headers offapi_normal_headers gcc3_uno officecfg_registry officecfg_headers)

target_link_libraries(code PRIVATE
    comphelper cppu cppuhelper sal sfx fwk svxcore svx svl svt tk tl ucbhelper utl vcl vclmain)

# Same reason every vcl workbench program above gets one: UNO/configmgr
# bootstrap reads CONFIGURATION_LAYERS from "<program name>rc", not unorc.
librestudio_generate_unorc("${LIBRESTUDIO_PROGRAM_DIR}/coderc")

install(TARGETS code RUNTIME DESTINATION program)

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
add_custom_target(code_uiconfig ALL DEPENDS ${_code_uiconfig_outputs})
