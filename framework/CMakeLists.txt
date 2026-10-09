# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# fwk (module framework) — the frame/dispatch/UI-configuration layer sfx2 sits
# on: frames, dispatchers, accelerators, toolbar/menu/statusbar management,
# layout management, job execution. Source list from Library_fwk.mk. Not
# ported from there: the precompiled header (a build-time optimisation only)
# and the WNT-only system-libs block.

set(FWK_SOURCES
    # ifneq ($(ENABLE_WASM_STRIP_RECOVERYUI),TRUE) -- applies, the variable is
    # unset in this build, so autorecovery is included unconditionally.
    framework/source/services/autorecovery

    framework/source/accelerators/acceleratorcache
    framework/source/accelerators/acceleratorconfiguration
    framework/source/accelerators/documentacceleratorconfiguration
    framework/source/accelerators/globalacceleratorconfiguration
    framework/source/accelerators/keymapping
    framework/source/accelerators/moduleacceleratorconfiguration
    framework/source/accelerators/presethandler
    framework/source/accelerators/storageholder
    framework/source/classes/framecontainer
    framework/source/classes/taskcreator
    framework/source/dispatch/closedispatcher
    framework/source/dispatch/dispatchinformationprovider
    framework/source/dispatch/dispatchprovider
    framework/source/dispatch/interceptionhelper
    framework/source/dispatch/loaddispatcher
    framework/source/dispatch/startmoduledispatcher
    framework/source/dispatch/windowcommanddispatch
    framework/source/dispatch/dispatchdisabler
    framework/source/dispatch/mailtodispatcher
    framework/source/dispatch/oxt_handler
    framework/source/dispatch/popupmenudispatcher
    framework/source/dispatch/servicehandler
    framework/source/dispatch/systemexec
    framework/source/helper/dockingareadefaultacceptor
    framework/source/helper/ocomponentaccess
    framework/source/helper/ocomponentenumeration
    framework/source/helper/oframes
    framework/source/helper/persistentwindowstate
    framework/source/helper/statusindicator
    framework/source/helper/statusindicatorfactory
    framework/source/helper/tagwindowasmodified
    framework/source/helper/titlebarupdate
    framework/source/helper/uiconfigelementwrapperbase
    framework/source/helper/uielementwrapperbase
    framework/source/helper/vclstatusindicator
    framework/source/helper/wakeupthread
    framework/source/interaction/quietinteraction
    framework/source/jobs/helponstartup
    framework/source/jobs/shelljob
    framework/source/fwe/classes/actiontriggercontainer
    framework/source/fwe/classes/actiontriggerpropertyset
    framework/source/fwe/classes/actiontriggerseparatorpropertyset
    framework/source/fwe/classes/addonmenu
    framework/source/fwe/classes/addonsoptions
    framework/source/fwe/classes/framelistanalyzer
    framework/source/fwe/classes/fwkresid
    framework/source/fwe/classes/rootactiontriggercontainer
    framework/source/fwe/classes/sfxhelperfunctions
    framework/source/fwe/dispatch/interaction
    framework/source/fwe/helper/actiontriggerhelper
    framework/source/fwe/helper/configimporter
    framework/source/fwe/helper/propertysetcontainer
    framework/source/fwe/helper/titlehelper
    framework/source/fwe/helper/documentundoguard
    framework/source/fwe/helper/undomanagerhelper
    framework/source/fwe/helper/windowstatehelper
    framework/source/fwe/xml/menuconfiguration
    framework/source/fwe/xml/menudocumenthandler
    framework/source/fwe/xml/saxnamespacefilter
    framework/source/fwe/xml/statusbarconfiguration
    framework/source/fwe/xml/statusbardocumenthandler
    framework/source/fwe/xml/toolboxconfiguration
    framework/source/fwe/xml/toolboxdocumenthandler
    framework/source/fwe/xml/xmlnamespaces
    framework/source/fwi/classes/converter
    framework/source/fwi/classes/protocolhandlercache
    framework/source/fwi/helper/mischelper
    framework/source/fwi/helper/shareablemutex
    framework/source/fwi/jobs/configaccess
    framework/source/fwi/threadhelp/transactionmanager
    framework/source/fwi/uielement/constitemcontainer
    framework/source/fwi/uielement/itemcontainer
    framework/source/fwi/uielement/rootitemcontainer
    framework/source/jobs/job
    framework/source/jobs/jobdata
    framework/source/jobs/jobdispatch
    framework/source/jobs/jobexecutor
    framework/source/jobs/jobresult
    framework/source/jobs/joburl
    framework/source/layoutmanager/helpers
    framework/source/layoutmanager/layoutmanager
    framework/source/layoutmanager/toolbarlayoutmanager
    framework/source/layoutmanager/uielement
    framework/source/loadenv/loadenv
    framework/source/loadenv/targethelper
    framework/source/recording/dispatchrecorder
    framework/source/recording/dispatchrecordersupplier
    framework/source/services/ContextChangeEventMultiplexer
    framework/source/services/desktop
    framework/source/services/dispatchhelper
    framework/source/services/frame
    framework/source/services/modulemanager
    framework/source/services/pathsettings
    framework/source/services/sessionlistener
    framework/source/services/substitutepathvars
    framework/source/services/taskcreatorsrv
    framework/source/services/urltransformer
    framework/source/services/mediatypedetectionhelper
    framework/source/services/uriabbreviation
    framework/source/uiconfiguration/CommandImageResolver
    framework/source/uiconfiguration/ImageList
    framework/source/uiconfiguration/globalsettings
    framework/source/uiconfiguration/graphicnameaccess
    framework/source/uiconfiguration/imagemanager
    framework/source/uiconfiguration/imagemanagerimpl
    framework/source/uiconfiguration/moduleuicfgsupplier
    framework/source/uiconfiguration/moduleuiconfigurationmanager
    framework/source/uiconfiguration/uicategorydescription
    framework/source/uiconfiguration/uiconfigurationmanager
    framework/source/uiconfiguration/windowstateconfiguration
    framework/source/uielement/addonstoolbarwrapper
    framework/source/uielement/buttontoolbarcontroller
    framework/source/uielement/comboboxtoolbarcontroller
    framework/source/uielement/complextoolbarcontroller
    framework/source/uielement/controlmenucontroller
    framework/source/uielement/dropdownboxtoolbarcontroller
    framework/source/uielement/edittoolbarcontroller
    framework/source/uielement/FixedImageToolbarController
    framework/source/uielement/FixedTextToolbarController
    framework/source/uielement/genericstatusbarcontroller
    framework/source/uielement/generictoolbarcontroller
    framework/source/uielement/imagebuttontoolbarcontroller
    framework/source/uielement/langselectionstatusbarcontroller
    framework/source/uielement/menubarmanager
    framework/source/uielement/menubarmerger
    framework/source/uielement/menubarwrapper
    framework/source/uielement/objectmenucontroller
    framework/source/uielement/popuptoolbarcontroller
    framework/source/uielement/progressbarwrapper
    framework/source/uielement/recentfilesmenucontroller
    framework/source/uielement/resourcemenucontroller
    framework/source/uielement/spinfieldtoolbarcontroller
    framework/source/uielement/statusbar
    framework/source/uielement/statusbaritem
    framework/source/uielement/statusbarmanager
    framework/source/uielement/statusbarmerger
    framework/source/uielement/statusbarwrapper
    framework/source/uielement/statusindicatorinterfacewrapper
    framework/source/uielement/styletoolbarcontroller
    framework/source/uielement/subtoolbarcontroller
    framework/source/uielement/thesaurusmenucontroller
    framework/source/uielement/togglebuttontoolbarcontroller
    framework/source/uielement/toolbarmanager
    framework/source/uielement/toolbarmerger
    framework/source/uielement/toolbarwrapper
    framework/source/uielement/uicommanddescription
    framework/source/uielement/fontmenucontroller
    framework/source/uielement/fontsizemenucontroller
    framework/source/uielement/footermenucontroller
    framework/source/uielement/headermenucontroller
    framework/source/uielement/langselectionmenucontroller
    framework/source/uielement/macrosmenucontroller
    framework/source/uielement/newmenucontroller
    framework/source/uielement/toolbarmodemenucontroller
    framework/source/uielement/toolbarsmenucontroller
    framework/source/uifactory/addonstoolbarfactory
    framework/source/uifactory/factoryconfiguration
    framework/source/uifactory/menubarfactory
    framework/source/uifactory/statusbarfactory
    framework/source/uifactory/toolbarfactory
    framework/source/uifactory/uicontrollerfactory
    framework/source/uifactory/uielementfactorymanager
    framework/source/uifactory/windowcontentfactorymanager
    framework/source/xml/acceleratorconfigurationreader
    framework/source/xml/acceleratorconfigurationwriter
    framework/source/xml/imagesconfiguration
    framework/source/xml/imagesdocumenthandler
)

librestudio_expand_cxx(FWK_SOURCES)

add_library(fwk SHARED ${FWK_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(fwk PRIVATE
        "${CMAKE_SOURCE_DIR}/framework/inc/pch/precompiled_fwk.hxx")
    target_compile_definitions(fwk PRIVATE PCH_LEVEL=2)
endif ()

# gb_Library_set_include plus gb_Library_use_sdk_api and
# gb_Library_use_custom_headers(... officecfg/registry).
target_include_directories(fwk PRIVATE
    "${CMAKE_SOURCE_DIR}/framework/source/inc"
    "${CMAKE_SOURCE_DIR}/framework/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(fwk officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(fwk PRIVATE FWK_DLLIMPLEMENTATION)

target_link_libraries(fwk PRIVATE
    comphelper cppu cppuhelper i18nlangtag sal salhelper sot svl svt tk tl
    ucbhelper utl vcl
    Boost::boost)

if (LIBRESTUDIO_ENABLE_BREAKPAD)
    target_link_libraries(fwk PRIVATE crashreport)
endif ()

# Library_fwk.mk: ifeq ($(OS),WNT) gb_Library_use_system_win32_libs,fwk,
# advapi32 endif. Kept structurally present, behind if (WIN32), per this
# port's policy of not silently dropping Windows-only gbuild targets -- see
# sal/CMakeLists.txt's sal_comtools for the same structural-but-inert
# pattern and the Windows/MSVC research explaining why this cannot actually
# be built or exercised on this machine (no cl.exe/ml64.exe/SDK). Inert on
# this Linux build: the if (WIN32) block below never executes here.
if (WIN32)
    target_link_libraries(fwk PRIVATE advapi32)
endif ()

# gb_Library_add_componentimpl,fwk,autorecovery -- fwk.component marks the
# AutoRecovery implementation <optional/>, and fwk.component.autorecovery
# names it as one this build does provide.
librestudio_add_component(fwk
    COMPONENT      "${CMAKE_SOURCE_DIR}/framework/util/fwk.component"
    IMPLEMENTATION "${CMAKE_SOURCE_DIR}/framework/util/fwk.component.autorecovery"
    LIBRARY        fwk
    LAYER          OOO)

librestudio_set_library_output_name(fwk fwk OOO)

install(TARGETS fwk LIBRARY DESTINATION program)

# --- Package_dtd --------------------------------------------------------------
#
# Package_dtd.mk: the 7 DTDs framework's own XML formats (accelerator config,
# event binding, menubar/toolbar/statusbar config, images, group-UI-names)
# are validated against, packaged verbatim into share/dtd/officedocument/1_0
# -- same gb_Package_add_files destination and plain install(FILES ...)
# convention already established for xmlscript/CMakeLists.txt's own
# Package_dtd.mk port (see that file's comment for the LIBO_SHARE_FOLDER
# derivation; not repeated here).
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/dtd/accelerator.dtd"
    "${CMAKE_CURRENT_SOURCE_DIR}/dtd/event.dtd"
    "${CMAKE_CURRENT_SOURCE_DIR}/dtd/groupuinames.dtd"
    "${CMAKE_CURRENT_SOURCE_DIR}/dtd/image.dtd"
    "${CMAKE_CURRENT_SOURCE_DIR}/dtd/menubar.dtd"
    "${CMAKE_CURRENT_SOURCE_DIR}/dtd/statusbar.dtd"
    "${CMAKE_CURRENT_SOURCE_DIR}/dtd/toolbar.dtd"
    DESTINATION share/dtd/officedocument/1_0)

# --- UIConfig_startmodule -----------------------------------------------------
#
# UIConfig_startmodule.mk: 1 menubar config file (framework/uiconfig/
# startmodule/menubar/menubar.xml) packaged for the "modules/StartModule"
# UIConfig via gb_UIConfig_add_menubarfiles. gb_UIConfig_add_menubarfile
# expands the destination to $(gb_UIConfig_INSTDIR)/$(1)/menubar/
# $(notdir $(2)).xml (solenv/gbuild/UIConfig.mk), i.e.
# share/config/soffice.cfg/modules/StartModule/menubar/menubar.xml --
# gb_UIConfig_INSTDIR being $(LIBO_SHARE_FOLDER)/config/soffice.cfg, the same
# "share/config/soffice.cfg" root UIConfig_svt's/UIConfig_vcl's own
# install(FILES ... DESTINATION share/config/soffice.cfg/<module>/ui) blocks
# already use (svtools/CMakeLists.txt, vcl/CMakeLists.txt), just with
# "menubar" in place of "ui" as the leaf directory and "modules/StartModule"
# in place of a plain module name. gb_UIMenubarTarget__command on this port's
# unxgcc.mk platform is a plain "cp $(3) $(1)" (solenv/gbuild/platform/
# unxgcc.mk:426-432) -- no transformation happens on Linux -- so a plain
# install(FILES ...) copy is the exact equivalent, not an approximation.
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/startmodule/menubar/menubar.xml"
    DESTINATION share/config/soffice.cfg/modules/StartModule/menubar)

# Build-tree mirror of the same file, for CONFIGURATION-driven CppunitTests
# that read from LIBRESTUDIO_SHARE_DIR (${CMAKE_BINARY_DIR}/instdir/share)
# rather than a real `cmake --install` -- the install(FILES ...) above alone
# never reaches that tree. Same add_custom_command+add_custom_target(...
# _uiconfig ALL DEPENDS ...) shape as every other UIConfig this port has
# ported (simpress_uiconfig, sfx_uiconfig, uui_uiconfig, ...) -- 2026-09-28,
# closing a gap where this section had the install() half but not this one.
set(_startmodule_uiconfig_dest
    "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/StartModule/menubar/menubar.xml")
add_custom_command(
    OUTPUT "${_startmodule_uiconfig_dest}"
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/StartModule/menubar"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/startmodule/menubar/menubar.xml"
            "${_startmodule_uiconfig_dest}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/startmodule/menubar/menubar.xml"
    COMMENT "Copying menubar.xml into build-tree soffice.cfg/modules/StartModule/menubar")
add_custom_target(startmodule_uiconfig ALL DEPENDS "${_startmodule_uiconfig_dest}")
unset(_startmodule_uiconfig_dest)

# --- CppunitTest targets -------------------------------------------------------
#
# All 5 of framework's CppunitTest_*.mk files (dispatch, loadenv, CheckXTitle,
# accelerators, services), re-checked directly against their live .mk files
# rather than trusted from any prior summary, call use_sdk_api, use_ure +
# use_vcl (3 protector pairs), use_rdb(services) and use_configuration. None
# calls use_components/set_include. use_rdb(services) needs no COMPONENTS --
# see scaddins/CppunitTest_deferred.cmake's header comment for what this port
# makes of that (a DEPENDS on program_services_rdb, the whole-services.rdb
# ini-default fallback) -- but use_configuration still needs
# LIBRESTUDIO_TEST_UNITTEST_DIR (test/CMakeLists.txt's staged test/
# user-template copy) and program_services_rdb itself is not a real target
# until the very end of the root CMakeLists.txt, both well AFTER framework's
# own add_subdirectory() (root CMakeLists.txt line ~523, long before
# add_subdirectory(test) at ~688 or program_services_rdb's own definition).
# So none of these 5 can be registered here directly -- all 5 are deferred
# into framework/CppunitTest_deferred.cmake, include()'d from the root
# CMakeLists.txt at the same point as vcl's/svtools' own use_rdb(services)
# tests; see that file for the per-test detail, including framework_services'
# ifneq($(OS),MACOSX) exclusion (Module_framework.mk) preserved there as
# if (NOT APPLE).

# framework/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/framework/CppunitTest_deferred.cmake")
