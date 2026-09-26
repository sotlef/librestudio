# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# scripting — the macro/scripting-provider framework (Tools > Macros): the
# generic ScriptProvider/BrowseNodeFactory machinery (scriptframe), the
# single-script-file provider used by pyprov/jsprov (singleprov), the
# "vnd.sun.star.script:" URL dispatcher (protocolhandler), the dialog
# .properties string-resource reader used by the Basic IDE dialog editor
# (stringresource), and the QuickJS-based JavaScript script provider
# (jsprov, Library_jsprov.mk) -- despite its name this needs no JDK: jsuno,
# which it links, embeds QuickJS rather than a JVM/Rhino (see
# jsuno/CMakeLists.txt), so jsprov is included below like any other library.
#
# All 8 of the module's gb_Library targets satisfy the "every
# gb_Library_use_libraries/use_external entry already ported" scoping rule
# used elsewhere in this port (see connectivity/CMakeLists.txt for the
# precedent).
#
# vbaevents (Library_vbaevents.mk) needs msfilter, which IS now ported (the
# "msfilter" library lives under the filter/ module directory, see
# filter/CMakeLists.txt) — so vbaevents is included below, linking msfilter,
# sb, sfx and the oovbaapi private API headers the same way basic/
# CMakeLists.txt already consumes them for sb.
#
# dlgprov (Library_dlgprov.mk) and basprov (Library_basprov.mk) both
# gb_Library_use_libraries ... vbahelper ...; vbahelper has now been ported
# (see vbahelper/CMakeLists.txt), so both are included below too. dlgprov
# also links xmlscript (now ported, see xmlscript/CMakeLists.txt) and uses
# the oovbaapi private API the same way vbaevents does; basprov only uses
# gb_Library_use_sdk_api (offapi + udkapi, no oovbaapi). Both are registered
# in the OOO layer, matching Repository.mk's
# gb_Helper_register_libraries_for_install(OOOLIBS,ooo,...) (both listed
# there under gb_Helper_optional(SCRIPTING,...)).
#
# Source lists, includes and library sets below are transcribed from each
# library's Library_*.mk. Only protocolhandler (Library_protocolhandler.mk)
# actually calls gb_Library_use_custom_headers(...,officecfg/registry), so it
# is the only library here wiring OFFICECFG_INCLUDE_DIR/officecfg_headers;
# none of the other seven libraries' .mk files call it.
#
# Module_scripting.mk wraps its ENTIRE gb_Module_add_targets list (all 8
# libraries above, jsprov included -- it sits inside the same
# `ifneq ($(filter SCRIPTING,$(BUILD_TYPE)),)` block as the rest, ENABLE_QUICKJS
# is a separate, narrower gate within it) in that one SCRIPTING check. So the
# whole body below is wrapped in `if (ENABLE_SCRIPTING)`, which defaults ON.
if (ENABLE_SCRIPTING)

set(SCRIPTFRAME_SOURCES
    scripting/source/provider/ActiveMSPList
    scripting/source/provider/BrowseNodeFactoryImpl
    scripting/source/provider/MasterScriptProvider
    scripting/source/provider/MasterScriptProviderFactory
    scripting/source/provider/ProviderCache
    scripting/source/provider/URIHelper
)

librestudio_expand_cxx(SCRIPTFRAME_SOURCES)
add_library(scriptframe SHARED ${SCRIPTFRAME_SOURCES})

target_include_directories(scriptframe PRIVATE
    "${CMAKE_SOURCE_DIR}/scripting/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(scriptframe udkapi_normal_headers offapi_normal_headers)

target_link_libraries(scriptframe PRIVATE
    comphelper cppu cppuhelper sal tl ucbhelper utl
    Boost::boost)

librestudio_add_component(scriptframe
    COMPONENT "${CMAKE_SOURCE_DIR}/scripting/util/scriptframe.component"
    LIBRARY   scriptframe
    LAYER     OOO)

librestudio_set_library_output_name(scriptframe scriptframe OOO)

install(TARGETS scriptframe LIBRARY DESTINATION program)

# singleprov has no gb_Library_set_componentfile call in Library_singleprov.mk
# -- it is not a UNO service on its own, only a helper library linked by
# jsprov/pyprov's script providers -- so no .component is registered for it.
# gb_Library_use_system_win32_libs (shell32, shlwapi) is Windows-only and does
# not apply on this Linux build.
set(SINGLEPROV_SOURCES
    scripting/source/singleprov/externaledit
    scripting/source/singleprov/provcontext
    scripting/source/singleprov/scriptdir
    scripting/source/singleprov/scriptfile
    scripting/source/singleprov/scriptmacro
    scripting/source/singleprov/scriptprovider
)

librestudio_expand_cxx(SINGLEPROV_SOURCES)
add_library(singleprov SHARED ${SINGLEPROV_SOURCES})

target_include_directories(singleprov PRIVATE
    "${CMAKE_SOURCE_DIR}/scripting/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(singleprov udkapi_normal_headers offapi_normal_headers)

target_link_libraries(singleprov PRIVATE
    comphelper cppu cppuhelper sal)

librestudio_set_library_output_name(singleprov singleprov OOO)

install(TARGETS singleprov LIBRARY DESTINATION program)

set(PROTOCOLHANDLER_SOURCES
    scripting/source/protocolhandler/scripthandler
)

librestudio_expand_cxx(PROTOCOLHANDLER_SOURCES)
add_library(protocolhandler SHARED ${PROTOCOLHANDLER_SOURCES})

target_include_directories(protocolhandler PRIVATE
    "${CMAKE_SOURCE_DIR}/scripting/source/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(protocolhandler officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(protocolhandler PRIVATE
    comphelper cppu cppuhelper fwk sal sfx tl utl vcl
    Boost::boost)

librestudio_add_component(protocolhandler
    COMPONENT "${CMAKE_SOURCE_DIR}/scripting/source/protocolhandler/protocolhandler.component"
    LIBRARY   protocolhandler
    LAYER     OOO)

librestudio_set_library_output_name(protocolhandler protocolhandler OOO)

install(TARGETS protocolhandler LIBRARY DESTINATION program)

set(STRINGRESOURCE_SOURCES
    scripting/source/stringresource/stringresource
)

librestudio_expand_cxx(STRINGRESOURCE_SOURCES)
add_library(stringresource SHARED ${STRINGRESOURCE_SOURCES})

target_include_directories(stringresource PRIVATE
    "${CMAKE_SOURCE_DIR}/scripting/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(stringresource udkapi_normal_headers offapi_normal_headers)

target_link_libraries(stringresource PRIVATE
    comphelper cppu cppuhelper sal utl tl i18nlangtag
    Boost::boost)

librestudio_add_component(stringresource
    COMPONENT "${CMAKE_SOURCE_DIR}/scripting/source/stringresource/stringresource.component"
    LIBRARY   stringresource
    LAYER     OOO)

librestudio_set_library_output_name(stringresource stringresource OOO)

install(TARGETS stringresource LIBRARY DESTINATION program)

set(VBAEVENTS_SOURCES
    scripting/source/vbaevents/eventhelper
)

librestudio_expand_cxx(VBAEVENTS_SOURCES)
add_library(vbaevents SHARED ${VBAEVENTS_SOURCES})

target_include_directories(vbaevents PRIVATE
    "${CMAKE_SOURCE_DIR}/scripting/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(vbaevents
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers)

target_link_libraries(vbaevents PRIVATE
    comphelper cppu cppuhelper msfilter sal sb sfx tl
    Boost::boost)

librestudio_add_component(vbaevents
    COMPONENT "${CMAKE_SOURCE_DIR}/scripting/source/vbaevents/vbaevents.component"
    LIBRARY   vbaevents
    LAYER     OOO)

librestudio_set_library_output_name(vbaevents vbaevents OOO)

install(TARGETS vbaevents LIBRARY DESTINATION program)

set(DLGPROV_SOURCES
    scripting/source/dlgprov/DialogModelProvider
    scripting/source/dlgprov/dlgevtatt
    scripting/source/dlgprov/dlgprov
)

librestudio_expand_cxx(DLGPROV_SOURCES)
add_library(dlgprov SHARED ${DLGPROV_SOURCES})

target_include_directories(dlgprov PRIVATE
    "${CMAKE_SOURCE_DIR}/scripting/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(dlgprov
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers)

target_link_libraries(dlgprov PRIVATE
    comphelper cppu cppuhelper sal i18nlangtag sb sfx tl ucbhelper utl vbahelper vcl xmlscript
    Boost::boost)

librestudio_add_component(dlgprov
    COMPONENT "${CMAKE_SOURCE_DIR}/scripting/source/dlgprov/dlgprov.component"
    LIBRARY   dlgprov
    LAYER     OOO)

librestudio_set_library_output_name(dlgprov dlgprov OOO)

install(TARGETS dlgprov LIBRARY DESTINATION program)

set(BASPROV_SOURCES
    scripting/source/basprov/baslibnode
    scripting/source/basprov/basmethnode
    scripting/source/basprov/basmodnode
    scripting/source/basprov/basprov
    scripting/source/basprov/basscript
)

librestudio_expand_cxx(BASPROV_SOURCES)
add_library(basprov SHARED ${BASPROV_SOURCES})

target_include_directories(basprov PRIVATE
    "${CMAKE_SOURCE_DIR}/scripting/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(basprov udkapi_normal_headers offapi_normal_headers)

target_link_libraries(basprov PRIVATE
    comphelper cppu cppuhelper sal sb sfx svl tl ucbhelper vcl
    Boost::boost)

librestudio_add_component(basprov
    COMPONENT "${CMAKE_SOURCE_DIR}/scripting/source/basprov/basprov.component"
    LIBRARY   basprov
    LAYER     OOO)

librestudio_set_library_output_name(basprov basprov OOO)

install(TARGETS basprov LIBRARY DESTINATION program)

set(JSPROV_SOURCES
    scripting/source/jsprov/loader
)

librestudio_expand_cxx(JSPROV_SOURCES)
add_library(jsprov SHARED ${JSPROV_SOURCES})

target_include_directories(jsprov PRIVATE
    "${CMAKE_SOURCE_DIR}/scripting/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(jsprov udkapi_normal_headers offapi_normal_headers)

target_link_libraries(jsprov PRIVATE
    comphelper cppu cppuhelper jsuno sal singleprov)

librestudio_add_component(jsprov
    COMPONENT "${CMAKE_SOURCE_DIR}/scripting/source/jsprov/jsprov.component"
    LIBRARY   jsprov
    LAYER     OOO)

librestudio_set_library_output_name(jsprov jsprov OOO)

install(TARGETS jsprov LIBRARY DESTINATION program)

endif ()
