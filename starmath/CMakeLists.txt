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
find_program(PYTHON3_EXECUTABLE NAMES python3 python REQUIRED)

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

# --- UIConfig_smath ----------------------------------------------------------
#
# UIConfig_smath.mk: gb_UIConfig_UIConfig(modules/smath) -- 1 menubarfiles, 2
# popupmenufiles, 1 statusbarfiles, 3 toolbarfiles and 16 uifiles, all under
# starmath/uiconfig/smath/, packaged into share/config/soffice.cfg/modules/
# smath/{menubar,popupmenu,statusbar,toolbar,ui} -- gb_UIConfig_DIR_FOR's own
# destination convention (see UIConfig_svt's own comment in svtools/
# CMakeLists.txt for its derivation), same plain install(FILES ...)
# mechanism basctl/CMakeLists.txt's own UIConfig_basicide block already
# extended to these same four extra file categories (popupmenu/menubar/
# statusbar/toolbar) beyond the uifiles-only shape UIConfig_svt/UIConfig_vcl/
# UIConfig_svx/UIConfig_formula needed. Verified against the live UIConfig_
# smath.mk and `find starmath/uiconfig -type f`: exactly 1+2+1+3+16 = 23
# entries, all 23 exist on disk, nothing invented.
#
# CppunitTest_starmath_dialogs_test.mk's own gb_CppunitTest_use_uiconfigs(
# starmath_dialogs_test,modules/smath) call (starmath/CppunitTest_deferred.
# cmake) needs a real build-time ALL target to depend on, the same reason
# UIConfig_basicide grew "basicide_uiconfig" (basctl/CMakeLists.txt) --
# introduced here as "smath_uiconfig" (this UIConfig's own gbuild leaf name;
# "modules/smath" itself is not a legal CMake target name -- it contains a
# "/"). Investigated directly before assuming either way: starmath/
# CMakeLists.txt had no UIConfig_smath block at all before this change (no
# "UIConfig"/"uiconfig" text anywhere in the file), so this is a real, fresh
# port, not a pre-existing target being reused.
set(SMATH_UICONFIG_MENUBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/menubar/menubar.xml"
)
set(SMATH_UICONFIG_POPUPMENU_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/popupmenu/edit.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/popupmenu/view.xml"
)
set(SMATH_UICONFIG_STATUSBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/statusbar/statusbar.xml"
)
set(SMATH_UICONFIG_TOOLBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/toolbar/toolbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/toolbar/standardbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/toolbar/fullscreenbar.xml"
)
set(SMATH_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/ui/alignmentdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/ui/catalogdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/ui/editwindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/ui/embedwindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/ui/fontdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/ui/fontsizedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/ui/fonttypedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/ui/mathwindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/ui/printeroptions.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/ui/savedefaultsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/ui/sidebarelements_math.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/ui/sidebarproperties_math.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/ui/smathsettings.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/ui/spacingdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/ui/symdefinedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/smath/ui/matrixeditor.ui"
)

install(FILES ${SMATH_UICONFIG_MENUBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/smath/menubar)
install(FILES ${SMATH_UICONFIG_POPUPMENU_FILES}
    DESTINATION share/config/soffice.cfg/modules/smath/popupmenu)
install(FILES ${SMATH_UICONFIG_STATUSBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/smath/statusbar)
install(FILES ${SMATH_UICONFIG_TOOLBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/smath/toolbar)
install(FILES ${SMATH_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/modules/smath/ui)

# Real build-time mirror of the install() rules above, alongside them (not
# instead of them) -- same rationale and per-file add_custom_command + one
# add_custom_target(... ALL DEPENDS ...) shape as UIConfig_basicide's own
# "basicide_uiconfig" (basctl/CMakeLists.txt); see that file's comment for
# the full reasoning this is not repeated here.
set(_smath_uiconfig_outputs "")
foreach (_smath_uiconfig_pair
        "SMATH_UICONFIG_MENUBAR_FILES:menubar"
        "SMATH_UICONFIG_POPUPMENU_FILES:popupmenu"
        "SMATH_UICONFIG_STATUSBAR_FILES:statusbar"
        "SMATH_UICONFIG_TOOLBAR_FILES:toolbar"
        "SMATH_UICONFIG_UI_FILES:ui")
    string(REPLACE ":" ";" _smath_uiconfig_pair "${_smath_uiconfig_pair}")
    list(GET _smath_uiconfig_pair 0 _smath_uiconfig_var)
    list(GET _smath_uiconfig_pair 1 _smath_uiconfig_category)
    foreach (_smath_uiconfig_src IN LISTS ${_smath_uiconfig_var})
        get_filename_component(_smath_uiconfig_name "${_smath_uiconfig_src}" NAME)
        set(_smath_uiconfig_dest
            "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/smath/${_smath_uiconfig_category}/${_smath_uiconfig_name}")
        add_custom_command(
            OUTPUT "${_smath_uiconfig_dest}"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/smath/${_smath_uiconfig_category}"
            COMMAND ${CMAKE_COMMAND} -E copy "${_smath_uiconfig_src}" "${_smath_uiconfig_dest}"
            DEPENDS "${_smath_uiconfig_src}"
            COMMENT "Copying ${_smath_uiconfig_name} into build-tree soffice.cfg/modules/smath/${_smath_uiconfig_category}"
            VERBATIM)
        list(APPEND _smath_uiconfig_outputs "${_smath_uiconfig_dest}")
    endforeach ()
endforeach ()
unset(_smath_uiconfig_pair)
unset(_smath_uiconfig_var)
unset(_smath_uiconfig_category)
unset(_smath_uiconfig_src)
unset(_smath_uiconfig_name)
unset(_smath_uiconfig_dest)

add_custom_target(smath_uiconfig ALL DEPENDS ${_smath_uiconfig_outputs})
unset(_smath_uiconfig_outputs)

# --- CppunitTest_starmath_qa_cppunit / _import / _export / _dialogs_test -----
# (all deferred) -------------------------------------------------------------
#
# All four of starmath's CppunitTest_*.mk files need data from modules add_
# subdirectory()'d LATER than starmath's own position (line ~1215) in the
# root CMakeLists.txt:
#   - Every one calls gb_CppunitTest_use_configuration -> CONFIGURATION,
#     which requires LIBRESTUDIO_TEST_UNITTEST_DIR (only set once test/
#     CMakeLists.txt has run, at line ~1226, after starmath's own position).
#     starmath_qa_cppunit's own use_components list (configmgr, fwk,
#     i18npool, xstor, package2, tk, sfx, ucb1, ucpfile1, utl, vcl) is
#     otherwise fully satisfiable at starmath's own position -- every owning
#     module (configmgr, package, vcl, toolkit, framework, sfx2, ucb) sits
#     textually BEFORE starmath in the root CMakeLists.txt -- so CONFIGURATION
#     alone is what forces its deferral, not COMPONENTS.
#   - starmath_import/_export/_dialogs_test each call gb_CppunitTest_use_rdb(
#     name,services) -> DEPENDS program_services_rdb instead, defined much
#     later still, right after librestudio_finalize_components(services) near
#     the end of the root CMakeLists.txt -- the same later-binding constraint
#     every other use_rdb(services) test in this port is already deferred to
#     (see e.g. svx/CppunitTest_deferred.cmake's or formula/CppunitTest_
#     deferred.cmake's own header comment for the identical reasoning).
# All four are therefore split into starmath/CppunitTest_deferred.cmake and
# scheduled with cmake_language(DEFER ...) below, the current, non-include()-
# in-root mechanism (see cmake/LibreStudioTest.cmake's header comment for the
# full mechanism and its two gotchas -- CMAKE_SOURCE_DIR, never CMAKE_
# CURRENT_SOURCE_DIR, in the path argument below, since DEFER's arguments are
# evaluated lazily in the DEFER DIRECTORY's own scope, not this one's).
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/starmath/CppunitTest_deferred.cmake")
