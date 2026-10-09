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
#
# Module_embeddedobj.mk's 2 CppunitTest_*.mk files, re-checked directly:
#   ifeq ($(OS),LINUX)
#   $(eval $(call gb_Module_add_slowcheck_targets,embeddedobj,\
#       CppunitTest_embeddedobj_general \
#   ))
#   endif
#   ifeq ($(OS),WNT)
#   $(eval $(call gb_Module_add_slowcheck_targets,embeddedobj,\
#       CppunitTest_embeddedobj_msole \
#   ))
#   endif
# embeddedobj_general is genuinely Linux-applicable and is wired for real,
# but deferred to embeddedobj/CppunitTest_deferred.cmake -- its gb_CppunitTest_
# use_rdb(services) needs program_services_rdb, which does not exist yet at
# this file's own add_subdirectory() position. embeddedobj_msole is gated to
# $(OS),WNT ONLY upstream -- also wired for real in the same deferred file,
# inside if (WIN32), following cpputools/CMakeLists.txt's own sp2bv precedent
# of transcribing a real Windows-only target rather than dropping it -- inert
# on this Linux build, same posture shell/CMakeLists.txt's CppunitTest_
# shell_zip now takes.

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

# Library_emboleobj.mk, ifeq ($(OS),WNT): the real OLE implementation.
if (WIN32)
    list(APPEND EMBOLEOBJ_SOURCES
        embeddedobj/source/msole/advisesink
        embeddedobj/source/msole/graphconvert
        embeddedobj/source/msole/olecomponent
        embeddedobj/source/msole/olewrapclient
        embeddedobj/source/msole/xdialogcreator)
endif ()

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
if (WIN32)
    target_link_libraries(emboleobj PRIVATE embobj gdi32 ole32 oleaut32 oledlg uuid)
endif ()

if (WIN32)
    set(_emboleobj_component "${CMAKE_SOURCE_DIR}/embeddedobj/source/msole/emboleobj.windows.component")
else ()
    set(_emboleobj_component "${CMAKE_SOURCE_DIR}/embeddedobj/source/msole/emboleobj.component")
endif ()
librestudio_add_component(emboleobj
    COMPONENT "${_emboleobj_component}"
    LIBRARY   emboleobj
    LAYER     OOO)

librestudio_set_library_output_name(emboleobj emboleobj OOO)

install(TARGETS emboleobj LIBRARY DESTINATION program)

# embeddedobj/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/embeddedobj/CppunitTest_deferred.cmake")
