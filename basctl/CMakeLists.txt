# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# basctl (module basctl) — the Basic IDE: the source-code editor/debugger
# window (baside2/baside3), the dialog editor (dlged*) used to lay out Basic
# UserForms, the object catalog/macro organizer dialogs and the
# com.sun.star.script.BasicIDE UNO service. Source list from
# Library_basctl.mk.
#
# ENABLE_WASM_STRIP_ACCESSIBILITY is unset (0) in this port, so the
# `ifneq (...,TRUE)` guard around the accessibility sources
# (accessibledialogcontrolshape/accessibledialogwindow) resolves true and
# they ARE included (same resolution already used for editeng).
#
# basctl/sdi/basslots.sdi (compiled by svidl) is basctl's SDI target;
# basctl/sdi/baside.sdi is the EXPORTS slot list it includes, same
# gb_SdiTarget_SdiTarget argument order already used for sfx2_sdi and
# svx_sdi. basslots.sdi also includes sfxitems.sdi/sfx.sdi from sfx2/sdi and
# svxitems.sdi/xoitems.sdi/svx.sdi from svx/sdi, matching
# gb_SdiTarget_set_include's extra -I dirs.
#
# Module_basctl.mk wraps essentially its whole module content (Library_basctl
# + UIConfig_basicide + l10n targets) in
# `ifneq ($(filter SCRIPTING,$(BUILD_TYPE)),)`; only the screenshot check
# target (not ported here) sits outside that gate. So the whole body below is
# wrapped in `if (ENABLE_SCRIPTING)`, which defaults ON.
if (ENABLE_SCRIPTING)

librestudio_add_sdi_target(basctl_sdi
    SDI        "${CMAKE_SOURCE_DIR}/basctl/sdi/basslots.sdi"
    EXPORTS    "${CMAKE_SOURCE_DIR}/basctl/sdi/baside.sdi"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/SdiTarget/basctl/sdi"
    INCLUDES   "${CMAKE_SOURCE_DIR}/svx/sdi" "${CMAKE_SOURCE_DIR}/sfx2/sdi")

set(BASCTL_SOURCES
    # ifneq ($(ENABLE_WASM_STRIP_ACCESSIBILITY),TRUE) -- applies, the flag is
    # unset in this build.
    basctl/source/accessibility/accessibledialogcontrolshape
    basctl/source/accessibility/accessibledialogwindow

    basctl/source/basicide/basdoc
    basctl/source/basicide/IDEComboBox
    basctl/source/basicide/basicrenderable
    basctl/source/basicide/baside2b
    basctl/source/basicide/baside2
    basctl/source/basicide/baside3
    basctl/source/basicide/basidectrlr
    basctl/source/basicide/basides1
    basctl/source/basicide/basides2
    basctl/source/basicide/basides3
    basctl/source/basicide/basidesh
    basctl/source/basicide/basobj2
    basctl/source/basicide/basobj3
    basctl/source/basicide/bastype2
    basctl/source/basicide/bastype3
    basctl/source/basicide/bastypes
    basctl/source/basicide/breakpoint
    basctl/source/basicide/brkdlg
    basctl/source/basicide/doceventnotifier
    basctl/source/basicide/docsignature
    basctl/source/basicide/documentenumeration
    basctl/source/basicide/idecodecompletiontypes
    basctl/source/basicide/idedataprovider
    basctl/source/basicide/idetimer
    basctl/source/basicide/iderdll
    basctl/source/basicide/layout
    basctl/source/basicide/linenumberwindow
    basctl/source/basicide/LineStatusControl
    basctl/source/basicide/localizationmgr
    basctl/source/basicide/macrodlg
    basctl/source/basicide/moduldl2
    basctl/source/basicide/moduldlg
    basctl/source/basicide/BasicColorConfig
    basctl/source/basicide/ColorSchemeDialog
    basctl/source/basicide/objectbrowser
    basctl/source/basicide/objectbrowsersearch
    basctl/source/basicide/ObjectCatalog
    basctl/source/basicide/sbxitem
    basctl/source/basicide/scriptdocument
    basctl/source/basicide/textwindowaccessibility
    basctl/source/basicide/uiobject
    basctl/source/basicide/unomodel
    basctl/source/dlged/dlgedclip
    basctl/source/dlged/dlged
    basctl/source/dlged/dlgedfac
    basctl/source/dlged/dlgedfunc
    basctl/source/dlged/dlgedlist
    basctl/source/dlged/dlgedmod
    basctl/source/dlged/dlgedobj
    basctl/source/dlged/dlgedpage
    basctl/source/dlged/dlgedview
    basctl/source/dlged/managelang
    basctl/source/dlged/propbrw

    # ifeq ($(OS),WNT) -- advapi32/gdi32/shell32 system libraries -- out of
    # scope for this Linux port.
)

librestudio_expand_cxx(BASCTL_SOURCES)
add_library(basctl SHARED ${BASCTL_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(basctl PRIVATE
        "${CMAKE_SOURCE_DIR}/basctl/inc/pch/precompiled_basctl.hxx")
    target_compile_definitions(basctl PRIVATE PCH_LEVEL=2)
endif ()
add_dependencies(basctl basctl_sdi)

target_include_directories(basctl PRIVATE
    "${CMAKE_SOURCE_DIR}/basctl/inc"
    "${CMAKE_SOURCE_DIR}/basctl/sdi"
    "${CMAKE_SOURCE_DIR}/basctl/source/inc"
    "${basctl_sdi_INCLUDE_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(basctl officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(basctl PRIVATE BASCTL_DLLIMPLEMENTATION)

target_link_libraries(basctl PRIVATE
    basegfx comphelper cppu cppuhelper editeng fwk sal i18nlangtag sb sfx
    sot svl svt svx svxcore tk tl utl ucbhelper vcl xmlscript
    Boost::boost)

librestudio_add_component(basctl
    COMPONENT "${CMAKE_SOURCE_DIR}/basctl/util/basctl.component"
    LIBRARY   basctl
    LAYER     OOO)

librestudio_set_library_output_name(basctl basctl OOO)

install(TARGETS basctl LIBRARY DESTINATION program)

# --- UIConfig_basicide --------------------------------------------------
#
# UIConfig_basicide.mk: gb_UIConfig_UIConfig(modules/BasicIDE) -- 2
# popupmenufiles, 1 menubarfiles, 1 statusbarfiles, 8 toolbarfiles and 22
# uifiles, all under basctl/uiconfig/basicide/, packaged into
# share/config/soffice.cfg/modules/BasicIDE/{popupmenu,menubar,statusbar,
# toolbar,ui} -- gb_UIConfig_DIR_FOR's own destination convention (see
# UIConfig_svt's own comment in svtools/CMakeLists.txt for its derivation),
# same plain install(FILES ...) mechanism, extended here to the four extra
# file categories (popupmenu/menubar/statusbar/toolbar) UIConfig_svt/
# UIConfig_vcl/UIConfig_svx never needed (all three were uifiles-only).
# It sits inside Module_basctl.mk's own "ifneq ($(filter SCRIPTING,
# $(BUILD_TYPE)),)" block alongside Library_basctl (see this file's own
# opening comment), so it is gated on ENABLE_SCRIPTING here too, same as the
# basctl library above.
#
# CppunitTest_basctl_dialogs_test.mk's own gb_CppunitTest_use_uiconfigs(
# basctl_dialogs_test,modules/BasicIDE) call (basctl/CppunitTest_
# deferred.cmake) needs a real build-time ALL target to depend on, the same
# reason UIConfig_svt grew "svt_uiconfig" (svtools/CMakeLists.txt) --
# introduced here as "basicide_uiconfig" (this UIConfig's own gbuild leaf
# name, lowercased; "modules/BasicIDE" itself is not a legal CMake target
# name -- it contains a "/").
set(BASCTL_UICONFIG_POPUPMENU_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/popupmenu/dialog.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/popupmenu/tabbar.xml"
)
set(BASCTL_UICONFIG_MENUBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/menubar/menubar.xml"
)
set(BASCTL_UICONFIG_STATUSBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/statusbar/statusbar.xml"
)
set(BASCTL_UICONFIG_TOOLBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/toolbar/dialogbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/toolbar/findbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/toolbar/fullscreenbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/toolbar/insertcontrolsbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/toolbar/formcontrolsbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/toolbar/macrobar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/toolbar/standardbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/toolbar/translationbar.xml"
)
set(BASCTL_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/basicmacrodialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/breakpointmenus.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/codecomplete.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/colorscheme.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/combobox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/defaultlanguage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/deletelangdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/dialogpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/dockingorganizer.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/dockingstack.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/dockingwatch.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/exportdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/gotolinedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/importlibdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/libpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/managebreakpoints.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/managelanguages.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/modulepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/newlibdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/objectbrowser.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/organizedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/basicide/ui/sortmenu.ui"
)

install(FILES ${BASCTL_UICONFIG_POPUPMENU_FILES}
    DESTINATION share/config/soffice.cfg/modules/BasicIDE/popupmenu)
install(FILES ${BASCTL_UICONFIG_MENUBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/BasicIDE/menubar)
install(FILES ${BASCTL_UICONFIG_STATUSBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/BasicIDE/statusbar)
install(FILES ${BASCTL_UICONFIG_TOOLBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/BasicIDE/toolbar)
install(FILES ${BASCTL_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/modules/BasicIDE/ui)

# Real build-time mirror of the install() rules above, alongside them (not
# instead of them) -- same rationale and per-file add_custom_command + one
# add_custom_target(... ALL DEPENDS ...) shape as UIConfig_svt's own
# svt_uiconfig (svtools/CMakeLists.txt); see that file's comment for the full
# reasoning this is not repeated here.
set(_basicide_uiconfig_outputs "")
foreach (_basicide_uiconfig_pair
        "BASCTL_UICONFIG_POPUPMENU_FILES:popupmenu"
        "BASCTL_UICONFIG_MENUBAR_FILES:menubar"
        "BASCTL_UICONFIG_STATUSBAR_FILES:statusbar"
        "BASCTL_UICONFIG_TOOLBAR_FILES:toolbar"
        "BASCTL_UICONFIG_UI_FILES:ui")
    string(REPLACE ":" ";" _basicide_uiconfig_pair "${_basicide_uiconfig_pair}")
    list(GET _basicide_uiconfig_pair 0 _basicide_uiconfig_var)
    list(GET _basicide_uiconfig_pair 1 _basicide_uiconfig_category)
    foreach (_basicide_uiconfig_src IN LISTS ${_basicide_uiconfig_var})
        get_filename_component(_basicide_uiconfig_name "${_basicide_uiconfig_src}" NAME)
        set(_basicide_uiconfig_dest
            "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/BasicIDE/${_basicide_uiconfig_category}/${_basicide_uiconfig_name}")
        add_custom_command(
            OUTPUT "${_basicide_uiconfig_dest}"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/BasicIDE/${_basicide_uiconfig_category}"
            COMMAND ${CMAKE_COMMAND} -E copy "${_basicide_uiconfig_src}" "${_basicide_uiconfig_dest}"
            DEPENDS "${_basicide_uiconfig_src}"
            COMMENT "Copying ${_basicide_uiconfig_name} into build-tree soffice.cfg/modules/BasicIDE/${_basicide_uiconfig_category}"
            VERBATIM)
        list(APPEND _basicide_uiconfig_outputs "${_basicide_uiconfig_dest}")
    endforeach ()
endforeach ()
unset(_basicide_uiconfig_pair)
unset(_basicide_uiconfig_var)
unset(_basicide_uiconfig_category)
unset(_basicide_uiconfig_src)
unset(_basicide_uiconfig_name)
unset(_basicide_uiconfig_dest)

add_custom_target(basicide_uiconfig ALL DEPENDS ${_basicide_uiconfig_outputs})
unset(_basicide_uiconfig_outputs)

# AllLangMoTarget_basctl (l10n .po -> .mo): not ported, matching this port's
# established blanket exclusion of every AllLangMoTarget_* (see e.g. uui/
# CMakeLists.txt, desktop/CMakeLists.txt's own notes on their own module's
# AllLangMoTarget).

endif ()

# basctl/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/basctl/CppunitTest_deferred.cmake")
