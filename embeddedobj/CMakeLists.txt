# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# embeddedobj — two libraries implementing the UNO embedded-object model.
#
# embobj (Library_embobj.mk) is the generic, format-independent
# implementation: the common EmbeddedObject/InplaceObject/persistence code
# shared by every embedded object regardless of what it wraps
# (commonembedding/), plus the DocumentHolder, dummy object, interceptor and
# creator/factory glue (general/). It registers the
# EmbeddedObjectCreator/OOoEmbeddedObjectFactory/
# OOoSpecialEmbeddedObjectFactory services.
#
# emboleobj (Library_emboleobj.mk) implements OLEEmbeddedObjectFactory —
# embedding of native MS-OLE objects. Its Windows leg (guarded by
# ifeq($(OS),WNT) in the .mk: five extra ATL/OLE sources -- advisesink,
# graphconvert, olecomponent, olewrapclient, xdialogcreator -- plus an
# additional gb_Library_use_libraries(emboleobj,embobj), win32 system libs,
# and the .windows variant of its componentfile -- is out of scope for this
# Linux-only port; on Linux emboleobj neither links embobj nor uses any of
# its headers (checked: no source here includes anything from
# embeddedobj/source/inc or embeddedobj/source/commonembedding).
#
# Both libraries call gb_Library_use_custom_headers(...,officecfg/registry)
# and gb_Library_use_sdk_api(...), wired the same way forms/CMakeLists.txt
# wires Library_frm.mk's gb_Library_use_api(frm,offapi udkapi ...):
# gb_Library_use_sdk_api expands to gb_LinkTarget_use_udk_api +
# gb_LinkTarget__use_api(offapi), i.e. the "normal" udkapi/offapi header
# variant (solenv/gbuild/LinkTarget.mk).
#
# Both are registered under PLAINLIBS_OOO in Repository.mk -> LAYER OOO.
# Neither .component file has an <optional/> implementation, so no
# IMPLEMENTATION filtering is needed for librestudio_add_component().

# --- embobj ---------------------------------------------------------------

set(EMBOBJ_SOURCES
    embeddedobj/source/commonembedding/embedobj
    embeddedobj/source/commonembedding/inplaceobj
    embeddedobj/source/commonembedding/miscobj
    embeddedobj/source/commonembedding/persistence
    embeddedobj/source/commonembedding/specialobject
    embeddedobj/source/commonembedding/visobj
    embeddedobj/source/commonembedding/xfactory
    embeddedobj/source/general/docholder
    embeddedobj/source/general/dummyobject
    embeddedobj/source/general/intercept
    embeddedobj/source/general/xcreator
)

librestudio_expand_cxx(EMBOBJ_SOURCES)
add_library(embobj SHARED ${EMBOBJ_SOURCES})

target_include_directories(embobj PRIVATE
    "${CMAKE_SOURCE_DIR}/embeddedobj/source/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(embobj officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(embobj PRIVATE EMBOBJ_DLLIMPLEMENTATION)

target_link_libraries(embobj PRIVATE
    comphelper cppu cppuhelper i18nlangtag sal svl svt tl utl vcl
    Boost::boost)

librestudio_add_component(embobj
    COMPONENT "${CMAKE_SOURCE_DIR}/embeddedobj/util/embobj.component"
    LIBRARY   embobj
    LAYER     OOO)

librestudio_set_library_output_name(embobj embobj OOO)

install(TARGETS embobj LIBRARY DESTINATION program)

# --- emboleobj --------------------------------------------------------------

set(EMBOLEOBJ_SOURCES
    embeddedobj/source/msole/closepreventer
    embeddedobj/source/msole/oleembed
    embeddedobj/source/msole/olemisc
    embeddedobj/source/msole/olepersist
    embeddedobj/source/msole/olevisual
    embeddedobj/source/msole/ownview
    embeddedobj/source/msole/xolefactory
)

librestudio_expand_cxx(EMBOLEOBJ_SOURCES)
add_library(emboleobj SHARED ${EMBOLEOBJ_SOURCES})

target_include_directories(emboleobj PRIVATE
    "${CMAKE_SOURCE_DIR}/embeddedobj/source/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(emboleobj officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(emboleobj PRIVATE
    comphelper cppu cppuhelper sal tl utl vcl
    Boost::boost)

librestudio_add_component(emboleobj
    COMPONENT "${CMAKE_SOURCE_DIR}/embeddedobj/source/msole/emboleobj.component"
    LIBRARY   emboleobj
    LAYER     OOO)

librestudio_set_library_output_name(emboleobj emboleobj OOO)

install(TARGETS emboleobj LIBRARY DESTINATION program)
