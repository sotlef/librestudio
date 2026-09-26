# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sdext (module sdext) -- two libraries:
#  - PresentationMinimizer: Impress's "Minimize Presentation Size" wizard
#    (source list from Library_PresentationMinimizer.mk). Unconditional in
#    Module_sdext.mk, no external dependency beyond boost_headers.
#  - pdfimport: the ODF-tree side of the PDF import filter (source list from
#    Library_pdfimport.mk). Gated in Module_sdext.mk behind
#    ENABLE_PDFIMPORT, which every desktop configure defaults to TRUE, so it
#    is ported here unconditionally like the rest of this port's targets.
#
# pdfimport does NOT link poppler, despite what the module name suggests:
# poppler only appears in Executable_xpdfimport.mk (a separate, short-lived
# helper process spawned by pdfimport's wrapper.cxx over a pipe -- see
# sdext/source/pdfimport/wrapper/wrapper.cxx's use of osl::Process /
# rtl::Bootstrap to locate and launch it, and ContentSink parsing what comes
# back). Library_pdfimport.mk's own gb_Library_use_externals list is just
# boost_headers, icu_headers, zlib and (on Linux) fontconfig -- confirmed by
# grepping sdext/source/pdfimport/**/*.cxx for poppler/Fc*/PDF* symbols: none
# of the objects listed in gb_Library_add_exception_objects,pdfimport include
# a poppler header. So pdfimport itself builds fine without poppler on this
# machine; only the actual PDF-parsing executable is out of scope here.
#
# Excluded:
#  - Executable_xpdfimport.mk: gb_Executable_use_externals lists poppler
#    directly (plus expat, freetype, libpng, fontconfig, zlib, all of which
#    ARE present -- `pkg-config --exists expat/freetype2/libpng/fontconfig`
#    all exit 0). `pkg-config --exists poppler` exits 1 on this machine (only
#    poppler-data, the encoding-tables package, is installed -- no poppler
#    or poppler-glib .pc file, no headers under /usr/include). Per the "don't
#    invent a workaround" rule, this target is deferred rather than vendored
#    or stubbed. Package_pdfimport_xpdfimport.mk (packages the xpdfimport
#    binary) is gated on the same executable and deferred with it.
#  - Executable_pdf2xml.mk / Executable_pdfunzip.mk / CppunitTest_sdext_pdfimport.mk:
#    only reachable via gb_Module_add_check_targets (test-only tooling), not
#    named by the task's target list; out of scope for this pass the same
#    way every other module's CppunitTest targets have been left out so far.
#  - CustomTarget_pdfimport's *module-level* CustomTarget wrapper is not
#    reproduced as such -- gb_CustomTarget_CustomTarget is just gbuild's
#    generic per-directory bookkeeping target; the one real artifact it
#    produces (hash.cxx, below) is generated directly against the pdfimport
#    library target instead, the same "regenerate the one artifact instead
#    of reaching into another target's plumbing" approach already used by
#    dbaccess/CMakeLists.txt (sqlbison.hxx) and svx/CMakeLists.txt
#    (oox/source/token/tokens.hxx).
#
# No new cmake/config/config_*.h.in headers are needed: neither library's
# sources read a macro from a config header this port does not already ship
# (config_folders.h, read by wrapper.cxx, has existed since early in the
# port; HAVE_FEATURE_PDFIMPORT is only tested in cmake/config/config_features.h.in
# itself, never inside sdext's own sources).

# --- PresentationMinimizer -- Library_PresentationMinimizer.mk -------------

set(PRESENTATIONMINIMIZER_SOURCES
    sdext/source/minimizer/configurationaccess
    sdext/source/minimizer/fileopendialog
    sdext/source/minimizer/graphiccollector
    sdext/source/minimizer/impoptimizer
    sdext/source/minimizer/informationdialog
    sdext/source/minimizer/optimizationstats
    sdext/source/minimizer/optimizerdialog
    sdext/source/minimizer/optimizerdialogcontrols
    sdext/source/minimizer/pagecollector
    sdext/source/minimizer/pppoptimizer
    sdext/source/minimizer/pppoptimizerdialog
    sdext/source/minimizer/pppoptimizertoken
    sdext/source/minimizer/sdextresid
)

librestudio_expand_cxx(PRESENTATIONMINIMIZER_SOURCES)
add_library(PresentationMinimizer SHARED ${PRESENTATIONMINIMIZER_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(PresentationMinimizer PRIVATE
        "${CMAKE_SOURCE_DIR}/sdext/inc/pch/precompiled_PresentationMinimizer.hxx")
    target_compile_definitions(PresentationMinimizer PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(PresentationMinimizer PRIVATE
    "${CMAKE_SOURCE_DIR}/sdext/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(PresentationMinimizer udkapi_normal_headers offapi_normal_headers)

target_link_libraries(PresentationMinimizer PRIVATE
    comphelper cppu cppuhelper sal sfx svt utl vcl tl
    Boost::boost)

librestudio_add_component(PresentationMinimizer
    COMPONENT "${CMAKE_SOURCE_DIR}/sdext/source/minimizer/minimizer.component"
    LIBRARY   PresentationMinimizer
    LAYER     OOO)

librestudio_set_library_output_name(PresentationMinimizer PresentationMinimizer OOO)

install(TARGETS PresentationMinimizer LIBRARY DESTINATION program)

# --- pdfimport -- Library_pdfimport.mk --------------------------------------

# wrapper.cxx (sdext/source/pdfimport/wrapper/wrapper.cxx:135) does a bare
# `#include <hash.cxx>` to pull in a gperf-generated perfect-hash lookup
# (PdfKeywordHash::in_word_set) over the keyword table in
# sdext/source/pdfimport/wrapper/keyword_list. Upstream generates it via
# CustomTarget_pdfimport.mk with a raw $(GPERF) invocation (no
# generator script this time, unlike xmloff/editeng's tokens.txt pipeline) --
# transcribed here with the same flags.
find_program(GPERF_EXECUTABLE gperf REQUIRED)

set(SDEXT_PDFIMPORT_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(SDEXT_PDFIMPORT_HASH_CXX "${SDEXT_PDFIMPORT_GENERATED_DIR}/hash.cxx")

add_custom_command(
    OUTPUT "${SDEXT_PDFIMPORT_HASH_CXX}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SDEXT_PDFIMPORT_GENERATED_DIR}"
    COMMAND sh -c "'${GPERF_EXECUTABLE}' -C -t -l -L C++ -m 20 --switch=2 --readonly-tables --pic -Z PdfKeywordHash -k'4-5,\$' '${CMAKE_SOURCE_DIR}/sdext/source/pdfimport/wrapper/keyword_list' > '${SDEXT_PDFIMPORT_HASH_CXX}'"
    DEPENDS "${CMAKE_SOURCE_DIR}/sdext/source/pdfimport/wrapper/keyword_list"
    COMMENT "gperf: sdext/pdfimport/generated/hash.cxx"
    VERBATIM)

add_custom_target(sdext_pdfimport_generated DEPENDS "${SDEXT_PDFIMPORT_HASH_CXX}")

# fontconfig is the only external here without an already-global CMake
# target (icu_headers/zlib/boost_headers all resolve through the root
# CMakeLists.txt's ICU::uc/ICU::i18n, ZLIB::ZLIB and Boost::boost already) --
# `pkg-config --exists fontconfig` exits 0 on this machine, same package vcl
# already depends on.
pkg_check_modules(SDEXT_FONTCONFIG REQUIRED IMPORTED_TARGET fontconfig)

set(PDFIMPORT_SOURCES
    sdext/source/pdfimport/filterdet
    sdext/source/pdfimport/misc/pdfihelper
    sdext/source/pdfimport/odf/odfemitter
    sdext/source/pdfimport/pdfiadaptor
    sdext/source/pdfimport/pdfparse/pdfentries
    sdext/source/pdfimport/pdfparse/pdfparse
    sdext/source/pdfimport/sax/emitcontext
    sdext/source/pdfimport/sax/saxattrlist
    sdext/source/pdfimport/tree/drawtreevisiting
    sdext/source/pdfimport/tree/genericelements
    sdext/source/pdfimport/tree/imagecontainer
    sdext/source/pdfimport/tree/pdfiprocessor
    sdext/source/pdfimport/tree/style
    sdext/source/pdfimport/tree/treevisitorfactory
    sdext/source/pdfimport/tree/writertreevisiting
    sdext/source/pdfimport/wrapper/wrapper
)

librestudio_expand_cxx(PDFIMPORT_SOURCES)
add_library(pdfimport SHARED ${PDFIMPORT_SOURCES})

target_include_directories(pdfimport PRIVATE
    "${CMAKE_SOURCE_DIR}/sdext/source/pdfimport/inc"
    "${SDEXT_PDFIMPORT_GENERATED_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(pdfimport
    udkapi_normal_headers offapi_normal_headers
    sdext_pdfimport_generated)

target_compile_definitions(pdfimport PRIVATE BOOST_ALL_NO_LIB)

target_link_libraries(pdfimport PRIVATE
    basegfx comphelper drawinglayercore drawinglayer vcl cppu cppuhelper
    sal salhelper tl xo i18nutil
    PkgConfig::SDEXT_FONTCONFIG
    ICU::uc ICU::i18n
    ZLIB::ZLIB
    Boost::boost)

librestudio_add_component(pdfimport
    COMPONENT "${CMAKE_SOURCE_DIR}/sdext/source/pdfimport/pdfimport.component"
    LIBRARY   pdfimport
    LAYER     OOO)

librestudio_set_library_output_name(pdfimport pdfimport OOO)

install(TARGETS pdfimport LIBRARY DESTINATION program)
