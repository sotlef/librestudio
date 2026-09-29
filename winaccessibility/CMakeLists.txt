# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# winaccessibility -- the Windows accessibility bridge: a UNO service
# (winaccessibility.component) that walks LibreOffice's own
# com.sun.star.accessibility::XAccessible tree and exposes it to Windows
# assistive technology (screen readers, etc.) through MSAA/IAccessible2 --
# implemented as an in-process ATL COM server (UAccCOM.dll) that the
# winaccessibility.dll UNO service talks to. There is no Linux/X11
# equivalent of MSAA/IAccessible2 in this tree (the Linux accessibility
# bridge is the separate, already-ported ATK code under vcl/unx/gtk3 and
# accessibility/), so this whole module is Windows+MSVC-only.
#
# Module_winaccessibility.mk, read in full:
#
#   $(eval $(call gb_Module_Module,winaccessibility))
#   ifeq ($(OS),WNT)
#   ifeq ($(COM),MSC)
#   $(eval $(call gb_Module_add_targets,winaccessibility,\
#       WinResTarget_uacccom \
#       CustomTarget_ia2_idl \
#       Library_uacccom \
#       Library_winaccessibility \
#   ))
#   endif # COM=MSC
#   endif # WNT
#
# i.e. all four targets need both $(OS)=WNT and $(COM)=MSC (MIDL and ATL are
# MSVC-toolchain-specific; no Windows/mingw-w64 branch exists upstream
# either) -- real gate: if (WIN32 AND MSVC) below. Kept structurally
# present per this port's policy of not silently dropping platform-only
# gbuild targets (cpputools/CMakeLists.txt's sp2bv is the canonical
# precedent). Inert on this Linux build.
#
# NOTE on precedent: extensions/CMakeLists.txt deliberately left an
# analogous MIDL+ATL+.rc group (WinResTarget_activex/Library_so_activex/
# CustomTarget_so_activex_idl) as a documented-but-unwritten gap. This
# module's task explicitly calls for a real transcription attempt instead
# (per the "nothing is out of scope by default" porting policy), so unlike
# that precedent, real (if MSVC/MIDL-toolchain-dependent and therefore
# unverifiable on this machine) CMake is written below rather than just
# a citation of the gap.
if (WIN32 AND MSVC)
    set(ATL_INCLUDE "" CACHE PATH
        "MSVC ATL headers directory, needed by winaccessibility's UAccCOM (see embedserv/CMakeLists.txt for the same variable)")
    set(ATL_LIB "" CACHE PATH
        "MSVC ATL import-library directory, needed by winaccessibility's UAccCOM (see embedserv/CMakeLists.txt for the same variable)")

    # -----------------------------------------------------------------------
    # CustomTarget_ia2_idl.mk -- runs midl.exe over the three IAccessible2 COM
    # IDL files (UAccCOM.idl, defines.idl, and IAccessible2's own upstream
    # ia2_api_all.idl) to produce the three generated headers UAccCOM.mk and
    # WinResTarget_uacccom.mk both consume via gb_*_use_custom_headers,
    # winaccessibility/ia2/idl: UAccCOM.h, defines.h, ia2_api_all.h (the
    # matching .tlb/.c stub outputs the real rule also produces are not
    # consumed by anything else in this module and are omitted here).
    #
    # gb_ExternalProject_get_target,IAccessible2 -- the ia2_api_all.idl source
    # itself lives in the vendored external/IAccessible2 tree (present in
    # this checkout: external/IAccessible2/), but that external project has
    # not been given its own CMakeLists.txt/ExternalProject wiring anywhere
    # in this port yet (`grep -rl IAccessible2 --include=CMakeLists.txt`
    # comes back empty except this file) -- porting external/IAccessible2 is
    # out of scope for this module's own assignment. The IAccessible2::idl
    # imported target referenced below is a placeholder for whatever target
    # that future port defines; until it exists, configuring with
    # WIN32/MSVC and reaching this branch will fail at generate time on the
    # missing target, which is the correct, loud failure mode for a real
    # but not-yet-satisfiable dependency (rather than silently producing a
    # broken custom command).
    # -----------------------------------------------------------------------
    find_program(MIDL_COMPILER midl.exe midl)

    set(WINA11Y_IDL_SRCDIR "${CMAKE_SOURCE_DIR}/winaccessibility/source/UAccCOMIDL")
    set(WINA11Y_IDL_OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/ia2/idl")
    file(MAKE_DIRECTORY "${WINA11Y_IDL_OUTDIR}")

    add_custom_command(
        OUTPUT "${WINA11Y_IDL_OUTDIR}/ia2_api_all.h"
        COMMAND "${MIDL_COMPILER}" -tlb "${WINA11Y_IDL_OUTDIR}/ia2_api_all.tlb"
                -h "${WINA11Y_IDL_OUTDIR}/ia2_api_all.h"
                -iid "${WINA11Y_IDL_OUTDIR}/ia2_api_all_i.c"
                -dlldata "${WINA11Y_IDL_OUTDIR}/dlldata.c"
                -proxy "${WINA11Y_IDL_OUTDIR}/ia2_api_all_p.c"
                -Oicf -I "${WINA11Y_IDL_SRCDIR}" -I "$<TARGET_PROPERTY:IAccessible2::idl,INTERFACE_INCLUDE_DIRECTORIES>"
                "$<TARGET_PROPERTY:IAccessible2::idl,INTERFACE_SOURCES>"
        DEPENDS IAccessible2::idl
        COMMENT "MIDL: ia2_api_all.idl (IAccessible2)"
        VERBATIM)

    foreach (_ia2_idl_name defines UAccCOM)
        add_custom_command(
            OUTPUT "${WINA11Y_IDL_OUTDIR}/${_ia2_idl_name}.h"
            COMMAND "${MIDL_COMPILER}" -tlb "${WINA11Y_IDL_OUTDIR}/${_ia2_idl_name}.tlb"
                    -h "${WINA11Y_IDL_OUTDIR}/${_ia2_idl_name}.h"
                    -iid "${WINA11Y_IDL_OUTDIR}/${_ia2_idl_name}_i.c"
                    -dlldata "${WINA11Y_IDL_OUTDIR}/dlldata.c"
                    -proxy "${WINA11Y_IDL_OUTDIR}/${_ia2_idl_name}_p.c"
                    -Oicf -I "${WINA11Y_IDL_SRCDIR}"
                    "${WINA11Y_IDL_SRCDIR}/${_ia2_idl_name}.idl"
            DEPENDS "${WINA11Y_IDL_SRCDIR}/${_ia2_idl_name}.idl" IAccessible2::idl
            COMMENT "MIDL: ${_ia2_idl_name}.idl"
            VERBATIM)
    endforeach ()

    add_custom_target(winaccessibility_ia2_idl DEPENDS
        "${WINA11Y_IDL_OUTDIR}/ia2_api_all.h"
        "${WINA11Y_IDL_OUTDIR}/defines.h"
        "${WINA11Y_IDL_OUTDIR}/UAccCOM.h")

    # -----------------------------------------------------------------------
    # Library_uacccom.mk -- "UAccCOM": the ATL COM automation server
    # implementing IAccessible2 on top of LibreOffice's own accessibility
    # tree (MAccessible, AccText, AccTable, ... -- 17 exception objects).
    # WinResTarget_uacccom.mk's UAccCOM.rc is folded straight into this
    # library's own sources: gbuild keeps WinResTarget as a separate class
    # only because its LinkTarget machinery predates letting a Library carry
    # its own .rc directly, but the .res this produces is linked into
    # UAccCOM.dll and nothing else -- exactly what happens when a .rc file
    # is listed directly among an MSVC CMake target's sources.
    # -----------------------------------------------------------------------

    set(UACCCOM_SOURCES
        winaccessibility/source/UAccCOM/AccAction
        winaccessibility/source/UAccCOM/AccActionBase
        winaccessibility/source/UAccCOM/AccComponent
        winaccessibility/source/UAccCOM/AccEditableText
        winaccessibility/source/UAccCOM/AccHyperLink
        winaccessibility/source/UAccCOM/AccHypertext
        winaccessibility/source/UAccCOM/AccImage
        winaccessibility/source/UAccCOM/AccRelation
        winaccessibility/source/UAccCOM/AccTable
        winaccessibility/source/UAccCOM/AccTableCell
        winaccessibility/source/UAccCOM/AccText
        winaccessibility/source/UAccCOM/AccTextBase
        winaccessibility/source/UAccCOM/AccValue
        winaccessibility/source/UAccCOM/EnumVariant
        winaccessibility/source/UAccCOM/MAccessible
        winaccessibility/source/UAccCOM/StdAfx
        winaccessibility/source/UAccCOM/UAccCOM
        winaccessibility/source/UAccCOM/UNOXWrapper)
    librestudio_expand_cxx(UACCCOM_SOURCES)

    add_library(UAccCOM SHARED
        ${UACCCOM_SOURCES}
        "${CMAKE_SOURCE_DIR}/winaccessibility/source/UAccCOM/UAccCOM.rc")
    add_dependencies(UAccCOM winaccessibility_ia2_idl)

    target_include_directories(UAccCOM PRIVATE
        "${WINA11Y_IDL_OUTDIR}"
        "${CMAKE_SOURCE_DIR}/winaccessibility/inc"
        "${ATL_INCLUDE}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(UAccCOM udkapi_normal_headers offapi_normal_headers)

    target_compile_definitions(UAccCOM PRIVATE UACCCOM_DLLIMPLEMENTATION)
    # ifneq ($(ENABLE_DBGUTIL),TRUE): -DPRODUCT, applied by
    # WinResTarget_uacccom.mk to the .rc compile specifically. ENABLE_DBGUTIL
    # is not a tracked switch anywhere in this CMake port (untracked -> off,
    # same convention as ENABLE_BREAKPAD/ENABLE_PAGEIN elsewhere), so this
    # define is applied unconditionally here, matching a bare, non-debug
    # ./configure.
    target_compile_definitions(UAccCOM PRIVATE PRODUCT)

    target_link_libraries(UAccCOM PRIVATE
        vcl comphelper cppu sal utl
        Boost::boost
        oleacc advapi32 delayimp kernel32 ole32 oleaut32 shlwapi user32 uuid
        "${ATL_LIB}/atls.lib")
    target_link_directories(UAccCOM PRIVATE "${ATL_LIB}")

    set_target_properties(UAccCOM PROPERTIES
        LINK_FLAGS "/DEF:${CMAKE_SOURCE_DIR}/winaccessibility/source/UAccCOM/UAccCOM.def")

    install(TARGETS UAccCOM LIBRARY DESTINATION program)

    # -----------------------------------------------------------------------
    # Library_winaccessibility.mk -- "winaccessibility"
    # (winaccessibility/source/service/winaccessibility.component): the UNO
    # service that owns the accessibility-event listeners (18 exception
    # objects) and drives the UAccCOM objects above through the
    # AccObjectWinManager.
    # -----------------------------------------------------------------------

    set(WINACCESSIBILITY_SOURCES
        winaccessibility/source/service/AccObject
        winaccessibility/source/service/ResIDGenerator
        winaccessibility/source/service/AccObjectWinManager
        winaccessibility/source/service/AccEventListener
        winaccessibility/source/service/AccComponentEventListener
        winaccessibility/source/service/AccContainerEventListener
        winaccessibility/source/service/AccDialogEventListener
        winaccessibility/source/service/AccFrameEventListener
        winaccessibility/source/service/AccWindowEventListener
        winaccessibility/source/service/AccMenuEventListener
        winaccessibility/source/service/AccTextComponentEventListener
        winaccessibility/source/service/AccObjectContainerEventListener
        winaccessibility/source/service/AccParagraphEventListener
        winaccessibility/source/service/AccDescendantManagerEventListener
        winaccessibility/source/service/AccListEventListener
        winaccessibility/source/service/AccTableEventListener
        winaccessibility/source/service/AccTreeEventListener
        winaccessibility/source/service/AccTopWindowListener
        winaccessibility/source/service/msaaservice_impl)
    librestudio_expand_cxx(WINACCESSIBILITY_SOURCES)

    add_library(winaccessibility SHARED ${WINACCESSIBILITY_SOURCES})
    add_dependencies(winaccessibility winaccessibility_ia2_idl)

    target_include_directories(winaccessibility PRIVATE
        "${WINA11Y_IDL_OUTDIR}"
        "${CMAKE_SOURCE_DIR}/winaccessibility/inc"
        "${ATL_INCLUDE}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(winaccessibility udkapi_normal_headers offapi_normal_headers)

    target_link_libraries(winaccessibility PRIVATE
        UAccCOM comphelper cppu cppuhelper vcl sal tk tl
        Boost::boost
        oleacc advapi32 delayimp kernel32 ole32 oleaut32 shlwapi user32 uuid
        gdi32 shell32 imm32 winspool)

    librestudio_add_component(winaccessibility
        COMPONENT "${CMAKE_SOURCE_DIR}/winaccessibility/source/service/winaccessibility.component"
        LIBRARY   winaccessibility
        LAYER     OOO)
    librestudio_set_library_output_name(winaccessibility winaccessibility OOO)

    install(TARGETS winaccessibility LIBRARY DESTINATION program)
endif ()
