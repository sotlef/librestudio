# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# formula (module formula) — the formula-calculation engine (FormulaCompiler,
# token/opcode representation, OOXML formula rewriting) shared by Calc and any
# other formula-capable document, plus its "forui" formula-editing dialog
# widgets. Source lists from Library_for.mk and Library_forui.mk. Both
# libraries share the same -DFORMULA_DLLIMPLEMENTATION define, matching
# upstream gbuild.

set(FOR_SOURCES
    formula/source/core/api/callable
    formula/source/core/api/FormulaCompiler
    formula/source/core/api/FormulaOpCodeMapperObj
    formula/source/core/api/grammar
    formula/source/core/api/OOXMLRewriter
    formula/source/core/api/token
    formula/source/core/api/vectortoken
    formula/source/core/resource/core_resource
)

librestudio_expand_cxx(FOR_SOURCES)
add_library(for SHARED ${FOR_SOURCES})

target_include_directories(for PRIVATE
    "${CMAKE_SOURCE_DIR}/formula/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(for officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(for PRIVATE FORMULA_DLLIMPLEMENTATION)

target_link_libraries(for PRIVATE
    comphelper cppu cppuhelper i18nlangtag sal svl svt tl utl vcl
    Boost::boost)

librestudio_add_component(for
    COMPONENT "${CMAKE_SOURCE_DIR}/formula/util/for.component"
    LIBRARY   for
    LAYER     OOO)

librestudio_set_library_output_name(for for OOO)

install(TARGETS for LIBRARY DESTINATION program)

set(FORUI_SOURCES
    formula/source/ui/dlg/formula
    formula/source/ui/dlg/FormulaHelper
    formula/source/ui/dlg/funcpage
    formula/source/ui/dlg/funcutl
    formula/source/ui/dlg/parawin
    formula/source/ui/dlg/structpg
)

librestudio_expand_cxx(FORUI_SOURCES)
add_library(forui SHARED ${FORUI_SOURCES})

target_include_directories(forui PRIVATE
    "${CMAKE_SOURCE_DIR}/formula/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(forui officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(forui PRIVATE FORMULA_DLLIMPLEMENTATION)

target_link_libraries(forui PRIVATE
    comphelper cppu cppuhelper for i18nlangtag sal sfx svl svt tl utl vcl
    Boost::boost)

librestudio_set_library_output_name(forui forui OOO)

install(TARGETS forui LIBRARY DESTINATION program)

# --- UIConfig_formula ---------------------------------------------------------
#
# UIConfig_formula.mk: 4 .ui files under formula/uiconfig/ui, packaged into
# share/config/soffice.cfg/formula/ui by gb_UIConfig_UIConfig(formula) --
# same gb_UIConfig_DIR_FOR-derived destination convention already established
# by svtools/CMakeLists.txt's own UIConfig_svt block (see that file's comment
# for the full "share/config/soffice.cfg/<module>/ui" derivation) and vcl/
# CMakeLists.txt's UIConfig_vcl block. Needed for real here (not just
# documented as a gap): CppunitTest_formula_dialogs_test.mk calls gb_
# CppunitTest_use_uiconfigs(formula_dialogs_test,formula), and cmake/
# LibreStudioTest.cmake's UICONFIG parameter requires a real "formula_
# uiconfig" build-time ALL target to depend on (FATAL_ERRORs by name
# otherwise) -- ported below following the exact same per-file add_custom_
# command + one add_custom_target(... ALL DEPENDS ...) shape as vcl_uiconfig/
# svt_uiconfig, alongside (not instead of) the install(FILES ...) packaging
# rule. Named formula_uiconfig, not for_uiconfig: gb_CppunitTest_use_uiconfigs
# passes the literal module name "formula" (matching gb_UIConfig_UIConfig's
# own first argument above), not the "for" library target name, so this
# port's UICONFIG lookup key follows that same "formula" spelling.
set(FORMULA_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/formuladialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/functionpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/structpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/parameter.ui"
)

install(FILES ${FORMULA_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/formula/ui)

set(_formula_uiconfig_outputs "")
foreach (_formula_uiconfig_src IN LISTS FORMULA_UICONFIG_UI_FILES)
    get_filename_component(_formula_uiconfig_name "${_formula_uiconfig_src}" NAME)
    set(_formula_uiconfig_dest "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/formula/ui/${_formula_uiconfig_name}")
    add_custom_command(
        OUTPUT "${_formula_uiconfig_dest}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/formula/ui"
        COMMAND ${CMAKE_COMMAND} -E copy "${_formula_uiconfig_src}" "${_formula_uiconfig_dest}"
        DEPENDS "${_formula_uiconfig_src}"
        COMMENT "Copying ${_formula_uiconfig_name} into build-tree soffice.cfg/formula/ui"
        VERBATIM)
    list(APPEND _formula_uiconfig_outputs "${_formula_uiconfig_dest}")
endforeach ()
unset(_formula_uiconfig_src)
unset(_formula_uiconfig_name)
unset(_formula_uiconfig_dest)

add_custom_target(formula_uiconfig ALL DEPENDS ${_formula_uiconfig_outputs})
unset(_formula_uiconfig_outputs)

# --- CppunitTest targets -------------------------------------------------------
#
# formula's 2 CppunitTest_*.mk files, ported via librestudio_add_cppunit_test()
# (cmake/LibreStudioTest.cmake). Both are deferred to formula/CppunitTest_
# deferred.cmake, include()'d from the root CMakeLists.txt well after this
# point -- formula itself is add_subdirectory()'d before unotest/test, and
# both tests need those modules' protector plug-ins (formula_ooxml) and/or
# program_services_rdb (formula_dialogs_test, not defined until the very end
# of the root CMakeLists.txt). See that file's own header comment for the
# full per-test detail.

# formula/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/formula/CppunitTest_deferred.cmake")
