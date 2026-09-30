# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# "Code" -- a visual-only prop for a future Visual-Studio-like IDE, the first
# wave of the plan recorded in memory as project_code_ide. This is original
# LibreStudio code, not a port of any upstream gbuild module -- there is no
# Module_code.mk anywhere to transcribe, so it has no row in
# PORTING_STATUS.md.
#
# Wired the same way vcl/CMakeLists.txt wires its own workbench programs
# (icontest et al.): a plain add_executable() with its own main() via the
# shared vclmain static library, VCL_INTERNALS for vcl/toolkit headers
# (ListBox), bootstrapping UNO the same way icontest does. No UNO service is
# exposed, so no librestudio_add_component() call.

add_executable(code
    "${CMAKE_SOURCE_DIR}/code/source/codeapp.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/mainwindow.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/solutionexplorer.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/editorarea.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/outputpanel.cxx")

target_compile_definitions(code PRIVATE VCL_INTERNALS)

target_include_directories(code PRIVATE
    "${CMAKE_SOURCE_DIR}/code/source"
    "${CMAKE_SOURCE_DIR}/vcl/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(code udkapi_normal_headers offapi_normal_headers gcc3_uno officecfg_registry)

target_link_libraries(code PRIVATE
    comphelper cppu cppuhelper sal svt tl ucbhelper vcl vclmain)

# Same reason every vcl workbench program above gets one: UNO/configmgr
# bootstrap reads CONFIGURATION_LAYERS from "<program name>rc", not unorc.
librestudio_generate_unorc("${LIBRESTUDIO_PROGRAM_DIR}/coderc")

install(TARGETS code RUNTIME DESTINATION program)
