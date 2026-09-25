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
