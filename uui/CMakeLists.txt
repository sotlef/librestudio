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
# Module_uui.mk's other targets (UIConfig_uui, AllLangMoTarget_uui, the
# CppunitTest_uui_dialogs_test screenshot target) are UI-config/localization/
# test targets outside this port's scope, same as every other module.

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
