# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# starmath (module starmath) — two libraries. sm is the Math formula editor
# itself (parser, formula-node tree, edit view, MathML/OOXML/RTF im-/export,
# the Elements sidebar panel); smd is a tiny "type detection" filter that
# sniffs old binary .smf/OLE StarMath documents and eqn-file headers so sm's
# real import can take over. Source lists from Library_sm.mk and
# Library_smd.mk.
#
# sm's slot/dispatch (SID) map is compiled by svidl from starmath/sdi/*.sdi,
# same mechanism as sfx2 and svx (see librestudio_add_sdi_target in
# cmake/LibreStudioSdiTarget.cmake). starmath/sdi/smslots.sdi is the file
# svidl actually compiles (gb_SdiTarget_SdiTarget's target arg); it in turn
# includes sfxitems.sdi/sfx.sdi (sfx2/sdi), svxitems.sdi/xoitems.sdi/svx.sdi
# (svx/sdi) and its own starmath/sdi/smath.sdi, and references SlotIdFiles
# under starmath/inc (starmath.hrc) and the top-level include/ tree (already
# covered by librestudio_add_sdi_target's default -I). starmath/sdi/smath is
# the EXPORTS master slot list (gb_SdiTarget_SdiTarget's source arg).
#
# msfilter (filter module) and oox are linked here as ordinary dependencies.
# sm's ooxmlexport.cxx/ooxmlimport.cxx need oox/token/tokens.hxx and
# oox/token/namespaces.hxx (gb_Library_use_custom_headers,sm,oox/generated),
# but oox/CMakeLists.txt declares its OOX_GENERATED_DIR include PRIVATE, so it
# is NOT propagated to sm just by linking oox -- the tokens.hxx/namespaces.hxx
# generate-tokens.py/namespaces.py pipeline is reproduced here into sm's own
# binary dir instead, same technique svx/CMakeLists.txt already uses for its
# own (tokens.hxx-only) need. Only the two headers sm's sources actually
# include are reproduced -- not the gperf tokenhash.inc or properties.hxx
# stages, which sm never uses.
find_program(PYTHON3_EXECUTABLE NAMES python3 REQUIRED)

set(SM_OOX_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(SM_OOX_TOKEN_SRC_DIR "${CMAKE_SOURCE_DIR}/oox/source/token")
set(SM_OOX_TOKENS_HXX "${SM_OOX_GENERATED_DIR}/oox/token/tokens.hxx")
set(SM_OOX_NAMESPACES_HXX "${SM_OOX_GENERATED_DIR}/oox/token/namespaces.hxx")
set(SM_OOX_NAMESPACENAMES_INC "${SM_OOX_GENERATED_DIR}/namespacenames.inc")
set(SM_OOX_NAMESPACES_STRICTNAMES_INC "${SM_OOX_GENERATED_DIR}/namespaces-strictnames.inc")

add_custom_command(
    OUTPUT "${SM_OOX_TOKENS_HXX}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SM_OOX_GENERATED_DIR}/misc"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SM_OOX_GENERATED_DIR}/oox/token"
    COMMAND "${PYTHON3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/solenv/bin/generate-tokens.py"
            "${SM_OOX_TOKEN_SRC_DIR}/tokens.txt"
            "${SM_OOX_GENERATED_DIR}/misc/tokenids.inc"
            "${SM_OOX_GENERATED_DIR}/tokennames.inc"
            "${SM_OOX_GENERATED_DIR}/misc/tokenhash.gperf"
    COMMAND ${CMAKE_COMMAND} -E cat
            "${SM_OOX_TOKEN_SRC_DIR}/tokens.hxx.head"
            "${SM_OOX_GENERATED_DIR}/misc/tokenids.inc"
            "${SM_OOX_TOKEN_SRC_DIR}/tokens.hxx.tail" > "${SM_OOX_TOKENS_HXX}"
    DEPENDS "${SM_OOX_TOKEN_SRC_DIR}/tokens.txt"
            "${SM_OOX_TOKEN_SRC_DIR}/tokens.hxx.head"
            "${SM_OOX_TOKEN_SRC_DIR}/tokens.hxx.tail"
            "${CMAKE_SOURCE_DIR}/solenv/bin/generate-tokens.py"
    COMMENT "generate-tokens.py: sm's oox/token/tokens.hxx"
    VERBATIM)

add_custom_command(
    OUTPUT "${SM_OOX_NAMESPACES_HXX}" "${SM_OOX_NAMESPACENAMES_INC}" "${SM_OOX_NAMESPACES_STRICTNAMES_INC}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SM_OOX_GENERATED_DIR}/misc"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SM_OOX_GENERATED_DIR}/oox/token"
    COMMAND "${PYTHON3_EXECUTABLE}" "${SM_OOX_TOKEN_SRC_DIR}/namespaces.py"
            "${SM_OOX_TOKEN_SRC_DIR}/namespaces.txt"
            "${SM_OOX_GENERATED_DIR}/misc/namespaceids.inc"
            "${SM_OOX_NAMESPACENAMES_INC}"
            "${SM_OOX_GENERATED_DIR}/misc/namespaces.txt"
            "${SM_OOX_TOKEN_SRC_DIR}/namespaces-strict.txt"
            "${SM_OOX_NAMESPACES_STRICTNAMES_INC}"
    COMMAND ${CMAKE_COMMAND} -E cat
            "${SM_OOX_TOKEN_SRC_DIR}/namespaces.hxx.head"
            "${SM_OOX_GENERATED_DIR}/misc/namespaceids.inc"
            "${SM_OOX_TOKEN_SRC_DIR}/namespaces.hxx.tail" > "${SM_OOX_NAMESPACES_HXX}"
    DEPENDS "${SM_OOX_TOKEN_SRC_DIR}/namespaces.txt"
            "${SM_OOX_TOKEN_SRC_DIR}/namespaces-strict.txt"
            "${SM_OOX_TOKEN_SRC_DIR}/namespaces.hxx.head"
            "${SM_OOX_TOKEN_SRC_DIR}/namespaces.hxx.tail"
            "${SM_OOX_TOKEN_SRC_DIR}/namespaces.py"
    COMMENT "namespaces.py: sm's oox/token/namespaces.hxx"
    VERBATIM)

add_custom_target(sm_oox_generated DEPENDS "${SM_OOX_TOKENS_HXX}" "${SM_OOX_NAMESPACES_HXX}")

librestudio_add_sdi_target(sm_sdi
    SDI        "${CMAKE_SOURCE_DIR}/starmath/sdi/smslots.sdi"
    EXPORTS    "${CMAKE_SOURCE_DIR}/starmath/sdi/smath.sdi"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/SdiTarget/starmath/sdi"
    INCLUDES   "${CMAKE_SOURCE_DIR}/starmath/inc"
               "${CMAKE_SOURCE_DIR}/svx/sdi"
               "${CMAKE_SOURCE_DIR}/sfx2/sdi")

set(SM_SOURCES
    starmath/source/ElementsDockingWindow
    starmath/source/SmElementsPanel
    starmath/source/SmPanelFactory
    starmath/source/SmPropertiesPanel
    starmath/source/accessibility
    starmath/source/action
    starmath/source/caret
    starmath/source/cfgitem
    starmath/source/cursor
    starmath/source/dialog
    starmath/source/document
    starmath/source/edit
    starmath/source/smediteng
    starmath/source/format
    starmath/source/mathtype
    starmath/source/node
    starmath/source/OpenTypeNode
    starmath/source/ooxmlexport
    starmath/source/ooxmlimport
    starmath/source/rtfexport
    starmath/source/parsebase
    starmath/source/parse
    starmath/source/parse5
    starmath/source/rect
    starmath/source/smdll
    starmath/source/smmod
    starmath/source/symbol
    starmath/source/tmpdevice
    starmath/source/typemap
    starmath/source/unodoc
    starmath/source/unofilter
    starmath/source/unomodel
    starmath/source/utility
    starmath/source/view
    starmath/source/visitors
    starmath/source/wordexportbase
    starmath/source/mathml/xparsmlbase
    starmath/source/mathml/mathmlattr
    starmath/source/mathml/mathmlexport
    starmath/source/mathml/mathmlimport
    starmath/source/mathml/mathmlMo
    starmath/source/mathml/export
    starmath/source/mathml/import
    starmath/source/mathml/iterator
    starmath/source/mathml/attribute
    starmath/source/mathml/element
    starmath/source/mathml/starmathdatabase
)

librestudio_expand_cxx(SM_SOURCES)
add_library(sm SHARED ${SM_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(sm PRIVATE
        "${CMAKE_SOURCE_DIR}/starmath/inc/pch/precompiled_sm.hxx")
    target_compile_definitions(sm PRIVATE PCH_LEVEL=2)
endif ()
add_dependencies(sm sm_sdi sm_oox_generated)

target_include_directories(sm PRIVATE
    "${CMAKE_SOURCE_DIR}/starmath/inc"
    "${CMAKE_SOURCE_DIR}/starmath/inc/mathml"
    "${sm_sdi_INCLUDE_DIR}"
    "${SM_OOX_GENERATED_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(sm officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(sm PRIVATE SM_DLLIMPLEMENTATION)

target_link_libraries(sm PRIVATE
    comphelper cppu cppuhelper editeng fwk i18nutil i18nlangtag msfilter oox
    sal salhelper sax sfx sot svl svt svx svxcore tk tl utl vcl xo
    Boost::boost
    ICU::uc)

librestudio_add_component(sm
    COMPONENT "${CMAKE_SOURCE_DIR}/starmath/util/sm.component"
    LIBRARY   sm
    LAYER     OOO)

librestudio_set_library_output_name(sm sm OOO)

install(TARGETS sm LIBRARY DESTINATION program)

set(SMD_SOURCES
    starmath/source/smdetect
    starmath/source/eqnolefilehdr
)

librestudio_expand_cxx(SMD_SOURCES)
add_library(smd SHARED ${SMD_SOURCES})

target_include_directories(smd PRIVATE
    "${CMAKE_SOURCE_DIR}/starmath/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(smd officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(smd PRIVATE
    comphelper cppu cppuhelper sal sfx sot tl utl
    Boost::boost)

librestudio_add_component(smd
    COMPONENT "${CMAKE_SOURCE_DIR}/starmath/util/smd.component"
    LIBRARY   smd
    LAYER     OOO)

librestudio_set_library_output_name(smd smd OOO)

install(TARGETS smd LIBRARY DESTINATION program)
