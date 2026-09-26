# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# helpcompiler — turns the help XHP sources into the linked help tree and
# builds/searches its full-text index. Three targets, from
# Library_helplinker.mk and Executable_HelpIndexer.mk/Executable_HelpLinker.mk:
#
#   helplinker  (SHARED) — the actual XHP-to-XSLT linking (HelpCompiler,
#               HelpLinker) and CLucene indexing/searching (HelpIndexer,
#               HelpSearch, LuceneHelper) logic, plus BasCodeTagger.
#   HelpIndexer (host executable) — thin CLI wrapper around
#               helpcompiler::HelpIndexer, links helplinker.
#   HelpLinker  (host executable) — thin CLI wrapper around HelpCompiler /
#               HelpLinker, links helplinker.
#
# helplinker must be built against the system CLucene, not LibreOffice's
# bundled/patched one: HelpIndexer.cxx guards a call to the patched-only
# IndexWriter::setSegmentInfoStartVersion() behind "#ifndef SYSTEM_CLUCENE",
# so helplinker defines SYSTEM_CLUCENE to take the branch that matches the
# plain system libclucene-core installed here. Neither executable's _main.cxx
# includes CLucene.h itself, so the define is only needed on helplinker.

set(HELPLINKER_SOURCES
    helpcompiler/source/HelpCompiler
    helpcompiler/source/LuceneHelper
    helpcompiler/source/HelpIndexer
    helpcompiler/source/HelpSearch
    helpcompiler/source/BasCodeTagger
    helpcompiler/source/HelpLinker
)

librestudio_expand_cxx(HELPLINKER_SOURCES)

add_library(helplinker SHARED ${HELPLINKER_SOURCES})

target_include_directories(helplinker PRIVATE
    "${CMAKE_SOURCE_DIR}/helpcompiler/inc"
    "${LIBXML2_INCLUDE_DIRS}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(helplinker udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(helplinker PRIVATE
    L10N_DLLIMPLEMENTATION
    HELPLINKER_DLLIMPLEMENTATION
    SYSTEM_CLUCENE)

target_link_libraries(helplinker PRIVATE
    sal comphelper
    EXPAT::EXPAT
    "${LIBXML2_LIBRARIES}"
    PkgConfig::LIBXSLT
    PkgConfig::LIBCLUCENE_CORE)

librestudio_set_library_output_name(helplinker helplinker OOO)

install(TARGETS helplinker LIBRARY DESTINATION program)

set(HELPINDEXER_SOURCES
    helpcompiler/source/HelpIndexer_main
)

librestudio_expand_cxx(HELPINDEXER_SOURCES)

add_executable(HelpIndexer ${HELPINDEXER_SOURCES})

target_link_libraries(HelpIndexer PRIVATE
    sal helplinker
    EXPAT::EXPAT
    PkgConfig::LIBXSLT
    PkgConfig::LIBCLUCENE_CORE)

install(TARGETS HelpIndexer RUNTIME DESTINATION program)

set(HELPLINKER_MAIN_SOURCES
    helpcompiler/source/HelpLinker_main
)

librestudio_expand_cxx(HELPLINKER_MAIN_SOURCES)

add_executable(HelpLinker ${HELPLINKER_MAIN_SOURCES})

target_include_directories(HelpLinker PRIVATE
    "${CMAKE_SOURCE_DIR}/helpcompiler/inc"
    "${LIBXML2_INCLUDE_DIRS}")

target_link_libraries(HelpLinker PRIVATE
    sal helplinker
    EXPAT::EXPAT
    "${LIBXML2_LIBRARIES}"
    PkgConfig::LIBXSLT
    PkgConfig::LIBCLUCENE_CORE)

install(TARGETS HelpLinker RUNTIME DESTINATION program)
