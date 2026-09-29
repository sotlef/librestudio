# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# uui (module uui) -- one library: uui, the "unified user interface"
# interaction handlers (password/authentication/SSL/locking dialogs and the
# error-handling machinery that turns UNO InteractionRequests into dialogs).
# Source list, include, externals and dependencies are transcribed from
# Library_uui.mk.
#
# Not reproduced:
#  - gb_Library_set_precompiled_header,uui,... -- precompiled headers are not
#    ported anywhere in this build.
#
# Module_uui.mk's other targets: UIConfig_uui is ported below (its 10 .ui
# files, real packaging + build-time mirror). AllLangMoTarget_uui
# (localization) remains out of this port's scope, same as every other
# module. CppunitTest_uui_dialogs_test (the gb_Module_add_screenshot_targets
# entry) is wired for real too, but deferred to uui/CppunitTest_deferred.cmake
# -- its gb_CppunitTest_use_rdb(services) needs the program_services_rdb
# target, which does not exist yet at this file's own add_subdirectory()
# position (root CMakeLists.txt); see that file for the full detail.

set(UUI_SOURCES
    uui/source/alreadyopen
    uui/source/authfallbackdlg
    uui/source/filechanged
    uui/source/fltdlg
    uui/source/iahndl
    uui/source/iahndl-authentication
    uui/source/iahndl-errorhandler
    uui/source/iahndl-filter
    uui/source/iahndl-ioexceptions
    uui/source/iahndl-locking
    uui/source/iahndl-oauth2
    uui/source/iahndl-ssl
    uui/source/interactionhandler
    uui/source/lockfailed
    uui/source/lockcorrupt
    uui/source/logindlg
    uui/source/masterpasscrtdlg
    uui/source/masterpassworddlg
    uui/source/nameclashdlg
    uui/source/openlocked
    uui/source/passwordcontainer
    uui/source/passworddlg
    uui/source/reloadeditable
    uui/source/requeststringresolver
    uui/source/secmacrowarnings
    uui/source/sslwarndlg
    uui/source/trylater
    uui/source/unknownauthdlg
)

librestudio_expand_cxx(UUI_SOURCES)
add_library(uui SHARED ${UUI_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(uui PRIVATE
        "${CMAKE_SOURCE_DIR}/uui/inc/pch/precompiled_uui.hxx")
    target_compile_definitions(uui PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(uui PRIVATE
    "${CMAKE_SOURCE_DIR}/uui/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(uui officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(uui PRIVATE
    comphelper cppu cppuhelper sal i18nlangtag svl svt svx tl utl vcl
    CURL::libcurl
    Boost::boost)

# uui/util/uui.component's third gb_Library_set_componentfile argument is
# "services", i.e. the office layer.
librestudio_add_component(uui
    COMPONENT "${CMAKE_SOURCE_DIR}/uui/util/uui.component"
    LIBRARY   uui
    LAYER     OOO)

librestudio_set_library_output_name(uui uui OOO)

install(TARGETS uui LIBRARY DESTINATION program)

# --- UIConfig_uui --------------------------------------------------------------
#
# UIConfig_uui.mk: 10 .ui files under uui/uiconfig/ui packaged into
# share/config/soffice.cfg/uui/ui -- gb_UIConfig_UIConfig's own destination
# convention (solenv/gbuild/UIConfig.mk: gb_UIConfig_INSTDIR/$(1)/ui, where
# $(1) is the UIConfig name itself, "uui" here -- confirmed directly against
# uui's own dialog sources below, not assumed, since svtools/CMakeLists.txt's
# own UIConfig_svt block installs to "svtools" even though its UIConfig name
# is "svt" and its dialogs actually reference "svt/ui/..." at run time (a
# pre-existing mismatch in that file, out of this module's scope to fix): all
# 10 of uui's own GenericDialogController/MessageDialogController
# constructors (uui/source/*.cxx, e.g. logindlg.cxx's
# u"uui/ui/logindialog.ui"_ustr) load from "uui/ui/...", so "uui" is the
# correct, verified destination folder here.
#
# Same real build-time-mirror mechanism as vcl/CMakeLists.txt's UIConfig_vcl
# and svtools/CMakeLists.txt's UIConfig_svt (see either for the full
# reasoning): UUI_UICONFIG_UI_FILES holds the list once, install(FILES ...)
# does the `cmake --install` packaging, and the per-file add_custom_command +
# one add_custom_target(... ALL DEPENDS ...) gives cmake/LibreStudioTest.cmake's
# UICONFIG option a real "uui_uiconfig" target to depend on for uui_dialogs_test.
set(UUI_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/filterselect.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/authfallback.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/logindialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/macrowarnmedium.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/masterpassworddlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/password.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/setmasterpassworddlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/simplenameclash.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sslwarndialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/unknownauthdialog.ui"
)

install(FILES ${UUI_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/uui/ui)

set(_uui_uiconfig_outputs "")
foreach (_uui_uiconfig_src IN LISTS UUI_UICONFIG_UI_FILES)
    get_filename_component(_uui_uiconfig_name "${_uui_uiconfig_src}" NAME)
    set(_uui_uiconfig_dest "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/uui/ui/${_uui_uiconfig_name}")
    add_custom_command(
        OUTPUT "${_uui_uiconfig_dest}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/uui/ui"
        COMMAND ${CMAKE_COMMAND} -E copy "${_uui_uiconfig_src}" "${_uui_uiconfig_dest}"
        DEPENDS "${_uui_uiconfig_src}"
        COMMENT "Copying ${_uui_uiconfig_name} into build-tree soffice.cfg/uui/ui"
        VERBATIM)
    list(APPEND _uui_uiconfig_outputs "${_uui_uiconfig_dest}")
endforeach ()
unset(_uui_uiconfig_src)
unset(_uui_uiconfig_name)
unset(_uui_uiconfig_dest)

add_custom_target(uui_uiconfig ALL DEPENDS ${_uui_uiconfig_outputs})
unset(_uui_uiconfig_outputs)

# uui/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/uui/CppunitTest_deferred.cmake")
