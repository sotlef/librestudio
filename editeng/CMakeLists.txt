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
find_program(PYTHON3_EXECUTABLE NAMES python3 python REQUIRED)

# Cached CACHE INTERNAL (not a plain set()) so editeng/CppunitTest_deferred.cmake
# -- which include()'s into the root CMakeLists.txt's own directory scope, not
# this one -- can still see it when recompiling EDITENG_SOURCES directly into
# editeng_core (same reuse pattern as basic/CMakeLists.txt's SB_SOURCES).
set(EDITENG_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated" CACHE INTERNAL
    "editeng generated headers directory")
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

# Cached for reuse by editeng_core (editeng/CppunitTest_deferred.cmake),
# which needs to recompile these same sources directly into the test binary
# -- see that file's own comment on gb_CppunitTest_use_library_objects for
# why (several EditEngine methods core-test.cxx calls are SAL_DLLPRIVATE,
# hidden ELF visibility, so linking against the real shared library cannot
# reach them). Same CACHE INTERNAL reuse pattern as basic/CMakeLists.txt's
# SB_SOURCES.
set(EDITENG_SOURCES "${EDITENG_SOURCES}" CACHE INTERNAL
    "editeng library's own expanded source list, for editeng_core's use_library_objects")

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

# --- UIConfig_editeng (gb_UIConfig_UIConfig(editeng)) -----------------------
#
# UIConfig_editeng.mk, read in full: 1 file (editeng/uiconfig/ui/spellmenu),
# same "bare module name -> share/config/soffice.cfg/<name>/..." convention
# already established for uui_uiconfig/filter_uiconfig/cui_uiconfig (not the
# "modules/<app>" convention simpress_uiconfig/swriter_uiconfig use).
set(EDITENG_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/spellmenu.ui")

install(FILES ${EDITENG_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/editeng/ui)

set(_editeng_uiconfig_outputs "")
foreach (_editeng_uiconfig_src IN LISTS EDITENG_UICONFIG_UI_FILES)
    get_filename_component(_editeng_uiconfig_name "${_editeng_uiconfig_src}" NAME)
    set(_editeng_uiconfig_dest "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/editeng/ui/${_editeng_uiconfig_name}")
    add_custom_command(
        OUTPUT "${_editeng_uiconfig_dest}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/editeng/ui"
        COMMAND ${CMAKE_COMMAND} -E copy "${_editeng_uiconfig_src}" "${_editeng_uiconfig_dest}"
        DEPENDS "${_editeng_uiconfig_src}"
        COMMENT "Copying ${_editeng_uiconfig_name} into build-tree soffice.cfg/editeng/ui")
    list(APPEND _editeng_uiconfig_outputs "${_editeng_uiconfig_dest}")
endforeach ()
unset(_editeng_uiconfig_src)
unset(_editeng_uiconfig_name)
unset(_editeng_uiconfig_dest)

add_custom_target(editeng_uiconfig ALL DEPENDS ${_editeng_uiconfig_outputs})
unset(_editeng_uiconfig_outputs)

# --- CppunitTest_editeng_borderline, CppunitTest_editeng_lookuptree --------
#
# Module_editeng.mk's gb_Module_add_check_targets lists 4 CppunitTest
# targets: editeng_editeng and editeng_core are guarded by "$(if $(and
# $(filter $(COM),MSC),$(MERGELIBS)),,...)" -- MSVC+MERGELIBS only, never
# true for this Linux/GCC port -- so all 4 are always in scope here; those
# two are wired below, unconditionally. editeng_editeng and editeng_core
# themselves are NOT wired here: both call use_ure/use_vcl (protector
# plug-ins from unotest/test, add_subdirectory()'d at lines ~697-698, well
# after editeng's own position above) and one or the other also needs
# use_rdb(services) (program_services_rdb, defined near the very end of the
# root CMakeLists.txt) or use_components on ucb's ucb1/ucpfile1 (ucb is
# add_subdirectory()'d at line 601, also after editeng) plus use_configuration
# (test/CMakeLists.txt's LIBRESTUDIO_TEST_UNITTEST_DIR, same ~698 position) --
# none of that is available yet at this point in the file. Both are deferred
# to editeng/CppunitTest_deferred.cmake instead, include()'d from the root
# CMakeLists.txt once every one of those dependencies exists; see that file's
# own header comment for the full detail.
#
# editeng_borderline (CppunitTest_editeng_borderline.mk) and
# editeng_lookuptree (CppunitTest_editeng_lookuptree.mk), re-checked directly
# against the live files: neither calls use_ure, use_vcl, use_rdb,
# use_components or use_configuration -- only use_sdk_api (offapi+udkapi
# normal headers) and use_externals(boost_headers icuuc). Every library
# either lists (xo basegfx editeng lng svt tk vcl svl sot utl tl comphelper
# ucbhelper cppuhelper cppu sal salhelper i18nlangtag i18nutil) is already
# used, as a real target, by the "editeng" production library defined just
# above in this very file -- so there is no ordering constraint against
# anything add_subdirectory()'d later, and both are wired here directly, at
# editeng's own natural position.
librestudio_add_cppunit_test(editeng_borderline
    SOURCES
        editeng/qa/items/borderline_test
    LIBRARIES
        xo basegfx editeng lng svt tk vcl svl sot utl tl comphelper
        ucbhelper cppuhelper cppu sal salhelper i18nlangtag i18nutil
        Boost::boost ICU::uc
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers)

librestudio_add_cppunit_test(editeng_lookuptree
    SOURCES
        editeng/qa/lookuptree/lookuptree_test
    LIBRARIES
        xo basegfx editeng lng svt tk vcl svl sot utl tl comphelper
        ucbhelper cppuhelper cppu sal salhelper i18nlangtag i18nutil
        Boost::boost ICU::uc
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers)

# editeng/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/editeng/CppunitTest_deferred.cmake")
