# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# editeng (module editeng) — the shared rich-text editing engine (EditEngine,
# Outliner, the item-set-based paragraph/character attributes, autocorrect,
# RTF import helpers) that text-capable app modules (svx and up) build on.
# Source list from Library_editeng.mk. No editeng.component: this library
# exposes no UNO services of its own, only C++ API consumed by its callers
# (same shape as tools/comphelper/basegfx).
#
# SvXMLAutoCorrectTokenHandler.cxx reaches a generated tokens.cxx (a gperf
# perfect-hash table over editeng/source/misc/tokens.txt) via a bare
# #include <tokens.cxx> -- CustomTarget_generated.mk produces it with
# solenv/bin/gentoken.py plus gperf, the same two tools xmloff already needed.
# gentoken.py only ever writes the .gperf file (the tokens.hxx target listed
# alongside it in gbuild is never actually consumed by any #include here), so
# only tokens.gperf and tokens.cxx are generated.
#
# editeng/source/editeng/{editdoc,editview,impedit*}.cxx reach editeng/yrs.hxx
# under #if ENABLE_YRS, which config_collab.h stubs to 0 in this build (same
# as sfx2/source/view/frmload.cxx) -- the yrs external is therefore not linked.

find_program(GPERF_EXECUTABLE gperf REQUIRED)
find_program(PYTHON3_EXECUTABLE NAMES python3 REQUIRED)

set(EDITENG_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(EDITENG_TOKENS_TXT "${CMAKE_SOURCE_DIR}/editeng/source/misc/tokens.txt")

set(EDITENG_TOKENS_GPERF "${EDITENG_GENERATED_DIR}/tokens.gperf")
add_custom_command(
    OUTPUT "${EDITENG_TOKENS_GPERF}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${EDITENG_GENERATED_DIR}"
    COMMAND "${PYTHON3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/solenv/bin/gentoken.py"
            "${EDITENG_TOKENS_TXT}" "${EDITENG_TOKENS_GPERF}"
    DEPENDS "${EDITENG_TOKENS_TXT}" "${CMAKE_SOURCE_DIR}/solenv/bin/gentoken.py"
    COMMENT "gentoken.py: editeng/generated/tokens.gperf"
    VERBATIM)

set(EDITENG_TOKENS_CXX "${EDITENG_GENERATED_DIR}/tokens.cxx")
add_custom_command(
    OUTPUT "${EDITENG_TOKENS_CXX}"
    COMMAND sh -c "'${GPERF_EXECUTABLE}' --compare-strncmp --readonly-tables --pic '${EDITENG_TOKENS_GPERF}' | sed -e '/^#line/d' -e 's/(char\\*)0/(char\\*)0, XML_TOKEN_INVALID/g' > '${EDITENG_TOKENS_CXX}'"
    DEPENDS "${EDITENG_TOKENS_GPERF}"
    COMMENT "gperf: editeng/generated/tokens.cxx"
    VERBATIM)

add_custom_target(editeng_generated DEPENDS "${EDITENG_TOKENS_CXX}")

set(EDITENG_SOURCES
    # ifneq ($(ENABLE_WASM_STRIP_ACCESSIBILITY),TRUE) -- applies, the variable
    # is unset (0) in this build.
    editeng/source/accessibility/AccessibleContextBase
    editeng/source/accessibility/AccessibleEditableTextPara
    editeng/source/accessibility/AccessibleHyperlink
    editeng/source/accessibility/AccessibleImageBullet
    editeng/source/accessibility/AccessibleParaManager
    editeng/source/accessibility/AccessibleSelectionBase
    editeng/source/accessibility/AccessibleStaticTextBase
    editeng/source/accessibility/AccessibleStringWrap

    editeng/source/editeng/ContentNode
    editeng/source/editeng/editattr
    editeng/source/editeng/editdata
    editeng/source/editeng/editdbg
    editeng/source/editeng/editdoc
    editeng/source/editeng/editeng
    editeng/source/editeng/EditLine
    editeng/source/editeng/editobj
    editeng/source/editeng/editsel
    editeng/source/editeng/editundo
    editeng/source/editeng/editview
    editeng/source/editeng/edtspell
    editeng/source/editeng/eehtml
    editeng/source/editeng/eeobj
    editeng/source/editeng/eerdll
    editeng/source/editeng/eertfpar
    editeng/source/editeng/fieldupdater
    editeng/source/editeng/impedit
    editeng/source/editeng/impedit2
    editeng/source/editeng/impedit3
    editeng/source/editeng/impedit4
    editeng/source/editeng/impedit5
    editeng/source/editeng/misspellrange
    editeng/source/editeng/section
    editeng/source/editeng/StripPortionsHelper
    editeng/source/editeng/textconv
    editeng/source/editeng/TextPortionList
    editeng/source/items/borderline
    editeng/source/items/bulitem
    editeng/source/items/CustomPropertyField
    editeng/source/items/charhiddenitem
    editeng/source/items/flditem
    editeng/source/items/frmitems
    editeng/source/items/itemtype
    editeng/source/items/justifyitem
    editeng/source/items/numitem
    editeng/source/items/legacyitem
    editeng/source/items/optitems
    editeng/source/items/paperinf
    editeng/source/items/paraitem
    editeng/source/items/rubyitem
    editeng/source/items/svdfield
    editeng/source/items/svxfont
    editeng/source/items/textitem
    editeng/source/items/writingmodeitem
    editeng/source/items/xmlcnitm
    editeng/source/misc/acorrcfg
    editeng/source/misc/edtdlg
    editeng/source/misc/forbiddencharacterstable
    editeng/source/misc/hangulhanja
    editeng/source/misc/splwrap
    editeng/source/misc/svxacorr
    editeng/source/misc/SvXMLAutoCorrectTokenHandler
    editeng/source/misc/SvXMLAutoCorrectExport
    editeng/source/misc/SvXMLAutoCorrectImport
    editeng/source/misc/swafopt
    editeng/source/misc/txtrange
    editeng/source/misc/unolingu
    editeng/source/misc/urlfieldhelper
    editeng/source/outliner/outleeng
    editeng/source/outliner/outlin2
    editeng/source/outliner/outliner
    editeng/source/outliner/outlobj
    editeng/source/outliner/outlundo
    editeng/source/outliner/outlvw
    editeng/source/outliner/overflowingtxt
    editeng/source/outliner/paralist
    editeng/source/rtf/rtfitem
    editeng/source/rtf/svxrtf
    editeng/source/uno/unoedhlp
    editeng/source/uno/unoedprx
    editeng/source/uno/unoedsrc
    editeng/source/uno/unofdesc
    editeng/source/uno/unofield
    editeng/source/uno/UnoForbiddenCharsTable
    editeng/source/uno/unofored
    editeng/source/uno/unoforou
    editeng/source/uno/unoipset
    editeng/source/uno/unonrule
    editeng/source/uno/unopracc
    editeng/source/uno/unotext
    editeng/source/uno/unotext2
    editeng/source/uno/unoviwou
    editeng/source/xml/xmltxtexp
    editeng/source/xml/xmltxtimp
    editeng/source/lookuptree/Trie
)

librestudio_expand_cxx(EDITENG_SOURCES)
add_library(editeng SHARED ${EDITENG_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(editeng PRIVATE
        "${CMAKE_SOURCE_DIR}/editeng/inc/pch/precompiled_editeng.hxx")
    target_compile_definitions(editeng PRIVATE PCH_LEVEL=2)
endif ()
add_dependencies(editeng editeng_generated)

target_include_directories(editeng PRIVATE
    "${CMAKE_SOURCE_DIR}/editeng/inc"
    "${CMAKE_SOURCE_DIR}/editeng/source/editeng"
    "${EDITENG_GENERATED_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(editeng officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(editeng PRIVATE EDITENG_DLLIMPLEMENTATION)

target_link_libraries(editeng PRIVATE
    basegfx comphelper cppu cppuhelper docmodel drawinglayercore drawinglayer
    i18nlangtag i18nutil lng sal salhelper sax sfx sot svl svt tk tl
    ucbhelper utl vcl xo
    Boost::boost
    ICU::uc
    LibXml2::LibXml2)

librestudio_set_library_output_name(editeng editeng OOO)

install(TARGETS editeng LIBRARY DESTINATION program)
