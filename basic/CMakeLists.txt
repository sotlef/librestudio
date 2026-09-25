# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sb (module basic) — the StarBasic interpreter: scanner/parser/compiler,
# runtime, SBX value system, and (since BUILD_TYPE includes SCRIPTING by
# default -- configure.ac:3538) the full macro-editing/VBA-compatibility layer
# sfx2 needs. Source list from Library_sb.mk. Not ported from there: the
# precompiled header, and the WNT/WNTINTEL-only asm/dllmgr variants -- this is
# a non-Windows build, so dllmgr-none is the one that applies unconditionally.

set(SB_SOURCES
    # ifneq ($(filter SCRIPTING,$(BUILD_TYPE)),) -- applies: enable_scripting
    # defaults to yes, so configure.ac appends SCRIPTING to BUILD_TYPE.
    basic/source/basmgr/basicmanagerrepository
    basic/source/basmgr/basmgr
    basic/source/basmgr/vbahelper
    basic/source/classes/codecompletecache
    basic/source/classes/eventatt
    basic/source/classes/image
    basic/source/classes/propacc
    basic/source/classes/sb
    basic/source/classes/sbunoobj
    basic/source/classes/sbxmod
    basic/source/comp/buffer
    basic/source/comp/codegen
    basic/source/comp/dim
    basic/source/comp/exprtree
    basic/source/comp/exprgen
    basic/source/comp/exprnode
    basic/source/comp/io
    basic/source/comp/loops
    basic/source/comp/parser
    basic/source/comp/sbcomp
    basic/source/comp/scanner
    basic/source/comp/symtbl
    basic/source/comp/token
    basic/source/uno/dlgcont
    basic/source/uno/modsizeexceeded
    basic/source/uno/namecont
    basic/source/uno/scriptcont
    basic/source/runtime/comenumwrapper
    basic/source/runtime/ddectrl
    basic/source/runtime/inputbox
    basic/source/runtime/iosys
    basic/source/runtime/props
    basic/source/runtime/stdobj
    basic/source/runtime/stdobj1
    basic/source/classes/errobject
    basic/source/util/sbutil

    basic/source/classes/global
    basic/source/classes/sbintern
    basic/source/comp/basiccharclass
    basic/source/runtime/basrdll
    basic/source/runtime/methods
    basic/source/runtime/methods1
    basic/source/runtime/runtime
    basic/source/sbx/sbxarray
    basic/source/sbx/sbxbool
    basic/source/sbx/sbxcoll
    basic/source/sbx/sbxcurr
    basic/source/sbx/sbxbase
    basic/source/sbx/sbxdate
    basic/source/sbx/sbxdec
    basic/source/sbx/sbxexec
    basic/source/sbx/sbxform
    basic/source/sbx/sbxint
    basic/source/sbx/sbxobj
    basic/source/sbx/sbxres
    basic/source/sbx/sbxscan
    basic/source/sbx/sbxstr
    basic/source/sbx/sbxvalue
    basic/source/sbx/sbxvar

    # ifeq ($(OS)$(CPUNAME),WNTINTEL) / ifeq ($(OS)...,WNT) -- neither applies
    # on Linux; dllmgr-none is the else-of-else branch.
    basic/source/runtime/dllmgr-none
)

librestudio_expand_cxx(SB_SOURCES)

add_library(sb SHARED ${SB_SOURCES})

# gb_Library_set_include plus gb_Library_use_sdk_api,
# gb_Library_use_api(sb, oovbaapi), and
# gb_Library_use_custom_headers(... officecfg/registry).
target_include_directories(sb PRIVATE
    "${CMAKE_SOURCE_DIR}/basic/inc"
    "${CMAKE_SOURCE_DIR}/basic/source/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(sb officecfg_headers
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers)

target_compile_definitions(sb PRIVATE BASIC_DLLIMPLEMENTATION)

target_link_libraries(sb PRIVATE
    comphelper cppu cppuhelper sal salhelper i18nlangtag sot svl svt tl utl
    vcl xmlscript
    ICU::uc ICU::i18n)

librestudio_add_component(sb
    COMPONENT "${CMAKE_SOURCE_DIR}/basic/util/sb.component"
    LIBRARY   sb
    LAYER     OOO)

librestudio_set_library_output_name(sb sb OOO)

install(TARGETS sb LIBRARY DESTINATION program)
