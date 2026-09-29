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
# HelpIndexer.cxx guards a call to the patched-only
# IndexWriter::setSegmentInfoStartVersion() behind "#ifndef SYSTEM_CLUCENE"
# -- that method only exists in LibreOffice's own patched/vendored CLucene
# fork, not in a plain system libclucene-core install, so the define must
# be present ONLY when actually linking a real system package (matching
# RepositoryExternal.mk's own gb_LinkTarget__use_clucene, which adds
# -DSYSTEM_CLUCENE only on its SYSTEM_CLUCENE branch, never on the
# vendored one -- see external/clucene/clucene.cmake's own header comment).
# This port originally hardcoded the define unconditionally (written before
# clucene had any vendoring fallback at all); corrected here to mirror
# LIBRESTUDIO_CLUCENE_FOUND, 2026-09-29. Neither executable's _main.cxx
# includes CLucene.h itself, so the define is only needed on helplinker.
#
# Fix (first real `cmake --build`, 2026-09-27): HelpIndexer.cxx also
# instantiates lucene::analysis::LanguageBasedAnalyzer
# (CLucene/analysis/LanguageBasedAnalyzer.h) -- linking failed with
# "undefined reference" for it despite PkgConfig::LIBCLUCENE_CORE being
# linked, because that class is CLUCENE_CONTRIBS_EXPORT: it lives in the
# separate libclucene-contribs-lib.so, not libclucene-core.so/
# libclucene-shared.so (what the "libclucene-core" pkg-config module's own
# Libs: line names). No .pc file exists for the contribs library on this
# system (only CLuceneConfig.cmake, which carries just version numbers, no
# importable target) -- linked by plain library name below instead, which
# resolves via the linker's default search of /usr/lib64 (already a standard
# system library directory). This split is a SYSTEM-only wrinkle: the
# vendored clucene.cmake driver compiles core+shared+contribs-lib into ONE
# archive (matching Library_clucene.mk's own single target), so the extra
# "clucene-contribs-lib" link is only added when LIBRESTUDIO_CLUCENE_FOUND
# is true.

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
    HELPLINKER_DLLIMPLEMENTATION)
if (LIBRESTUDIO_CLUCENE_FOUND)
    target_compile_definitions(helplinker PRIVATE SYSTEM_CLUCENE)
endif ()

target_link_libraries(helplinker PRIVATE
    sal comphelper
    EXPAT::EXPAT
    "${LIBXML2_LIBRARIES}"
    PkgConfig::LIBXSLT
    ${LIBRESTUDIO_CLUCENE_TARGET})
if (LIBRESTUDIO_CLUCENE_FOUND)
    target_link_libraries(helplinker PRIVATE clucene-contribs-lib)
endif ()

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
    ${LIBRESTUDIO_CLUCENE_TARGET})

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
    ${LIBRESTUDIO_CLUCENE_TARGET})

install(TARGETS HelpLinker RUNTIME DESTINATION program)
