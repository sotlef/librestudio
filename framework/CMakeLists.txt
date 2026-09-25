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
