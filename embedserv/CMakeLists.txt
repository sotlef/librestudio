# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# embedserv -- Windows OLE embedding support: the two DLLs that let
# soffice.bin act as an in-process OLE server (a document embedded inside
# another OLE container application, e.g. an OOXML sheet embedded in a Word
# document) and be embedded itself. There is no Linux/X11 equivalent of OLE
# embedding in this tree (LibreOffice's own cross-platform embedding story
# lives in embeddedobj/ instead), so this whole module is Windows-only.
#
# Module_embedserv.mk, read in full:
#
#   $(eval $(call gb_Module_Module,embedserv))
#   ifeq ($(OS),WNT)
#   $(eval $(call gb_Module_add_targets,embedserv,\
#       Library_emser \
#       Library_inprocserv \
#   ))
#   endif
#
# i.e. both targets only exist when $(OS)=WNT -- real gate: if (WIN32) below.
# Kept structurally present per this port's policy of not silently dropping
# Windows-only gbuild targets (cpputools/CMakeLists.txt's sp2bv is the
# canonical precedent for this exact pattern). Inert on this Linux build.
if (WIN32)
    # Both libraries compile against the MSVC ATL headers/import library
    # (ATL_INCLUDE, ATL_LIB, atls.lib -- part of the "Desktop development
    # with C++" Visual Studio workload, not something this port has any
    # equivalent of or can locate on this machine). Declared as CACHE PATH
    # so a real Windows+MSVC build of this port can point them at a real
    # Visual Studio installation; left unset (and therefore these targets
    # unbuildable) otherwise -- the same "no cl.exe/ml64.exe/SDK on this
    # machine" situation cpputools/CMakeLists.txt documents for sp2bv.
    set(ATL_INCLUDE "" CACHE PATH
        "MSVC ATL headers directory (e.g. .../VC/Tools/MSVC/<ver>/atlmfc/include), needed by embedserv's emser")
    set(ATL_LIB "" CACHE PATH
        "MSVC ATL import-library directory (e.g. .../VC/Tools/MSVC/<ver>/atlmfc/lib/x64), needed by embedserv's emser")

    # -----------------------------------------------------------------------
    # Library_emser.mk -- "emser" (embedserv/util/emser.component): the
    # in-process OLE server implementation (IOleObject/IDataObject/
    # IPersistStorage/... on top of a UNO document model). 11 exception
    # objects, boost_headers, use_sdk_api, comphelper/cppu/cppuhelper/sal.
    # -----------------------------------------------------------------------

    set(EMSER_SOURCES
        embedserv/source/embed/docholder
        embedserv/source/embed/ed_idataobj
        embedserv/source/embed/ed_iinplace
        embedserv/source/embed/ed_ioleobject
        embedserv/source/embed/ed_ipersiststr
        embedserv/source/embed/esdll
        embedserv/source/embed/guid
        embedserv/source/embed/iipaobj
        embedserv/source/embed/intercept
        embedserv/source/embed/servprov
        embedserv/source/embed/syswinwrapper
        embedserv/source/embed/tracker)
    librestudio_expand_cxx(EMSER_SOURCES)

    add_library(emser SHARED ${EMSER_SOURCES})

    target_include_directories(emser PRIVATE
        "${CMAKE_SOURCE_DIR}/embedserv/source/inc"
        "${ATL_INCLUDE}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(emser udkapi_normal_headers offapi_normal_headers)

    # gb_Library_add_defs,emser,-UNOMINMAX -- undefine (rather than define) the
    # NOMINMAX macro; upstream's own build passes -U here, so <windows.h>'s
    # min/max macros stay active for this translation unit. Reproduced
    # literally even though it looks backwards at a glance.
    target_compile_definitions(emser PRIVATE -UNOMINMAX)

    target_link_libraries(emser PRIVATE
        comphelper cppu cppuhelper sal
        Boost::boost
        gdi32 ole32 oleaut32 uuid
        "${ATL_LIB}/atls.lib")
    target_link_directories(emser PRIVATE "${ATL_LIB}")

    librestudio_add_component(emser
        COMPONENT "${CMAKE_SOURCE_DIR}/embedserv/util/emser.component"
        LIBRARY   emser
        LAYER     OOO)
    librestudio_set_library_output_name(emser emser OOO)

    install(TARGETS emser LIBRARY DESTINATION program)

    # -----------------------------------------------------------------------
    # Library_inprocserv.mk -- "inprocserv": the raw COM in-process-server
    # DllGetClassObject/DllCanUnloadNow entry points (dllentry.cxx) plus the
    # advise-sink and embedded-object glue (advisesink.cxx,
    # inprocembobj.cxx). No use_sdk_api/use_libraries/componentfile call
    # anywhere in Library_inprocserv.mk (verified) -- this is a plain COM DLL,
    # not a UNO service, so it needs neither UNO headers nor an install into
    # the services registry.
    # -----------------------------------------------------------------------

    set(INPROCSERV_SOURCES
        embedserv/source/inprocserv/advisesink
        embedserv/source/inprocserv/dllentry
        embedserv/source/inprocserv/inprocembobj)
    librestudio_expand_cxx(INPROCSERV_SOURCES)

    add_library(inprocserv SHARED ${INPROCSERV_SOURCES})

    target_include_directories(inprocserv PRIVATE
        "${CMAKE_SOURCE_DIR}/embedserv/source/inc")

    target_link_libraries(inprocserv PRIVATE gdi32 ole32 uuid)

    # ifeq ($(COM),MSC): /DEF:.../inprocserv.def -- the classic Windows way of
    # giving a DLL a fixed, undecorated export table (DllGetClassObject,
    # DllCanUnloadNow, DllRegisterServer, DllUnregisterServer). MSVC's own
    # /DEF linker switch maps directly onto CMake's LINK_FLAGS property (no
    # first-class "module definition file" target property exists in CMake
    # for a non-MODULE library target).
    if (MSVC)
        set_target_properties(inprocserv PROPERTIES
            LINK_FLAGS "/DEF:${CMAKE_SOURCE_DIR}/embedserv/source/inprocserv/inprocserv.def")
    endif ()

    install(TARGETS inprocserv LIBRARY DESTINATION program)
endif ()
