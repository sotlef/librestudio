# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sdext (module sdext) -- two libraries and one executable:
#  - PresentationMinimizer: Impress's "Minimize Presentation Size" wizard
#    (source list from Library_PresentationMinimizer.mk). Unconditional in
#    Module_sdext.mk, no external dependency beyond boost_headers.
#  - pdfimport: the ODF-tree side of the PDF import filter (source list from
#    Library_pdfimport.mk). Gated in Module_sdext.mk behind
#    ENABLE_PDFIMPORT, which every desktop configure defaults to TRUE, so it
#    is ported here unconditionally like the rest of this port's targets.
#  - xpdfimport (Executable_xpdfimport.mk): the actual poppler-backed PDF
#    parser, gated behind real package detection -- see "xpdfimport" below.
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
# machine regardless of xpdfimport's own gate below.
#
# Excluded:
#  - Executable_pdf2xml.mk / Executable_pdfunzip.mk: only reachable via
#    gb_Module_add_check_targets (test-only tooling), not named by the task's
#    target list; out of scope for this pass the same way every other
#    module's CppunitTest-only tooling has been left out so far.
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
# target (icu_headers/boost_headers already resolve via the root
# CMakeLists.txt) -- `pkg-config --exists fontconfig` exits 0 on this
# machine, same package vcl already depends on. zlib/png resolved via
# librestudio_find_package() (cmake/LibreStudioTarballs.cmake) into real
# CACHE INTERNAL exports, visible everywhere.
pkg_check_modules(SDEXT_FONTCONFIG REQUIRED IMPORTED_TARGET fontconfig)
librestudio_find_package(NAME ZLIB)
librestudio_find_package(NAME PNG)

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
    ${LIBRESTUDIO_ZLIB_TARGET}
    Boost::boost)

librestudio_add_component(pdfimport
    COMPONENT "${CMAKE_SOURCE_DIR}/sdext/source/pdfimport/pdfimport.component"
    LIBRARY   pdfimport
    LAYER     OOO)

librestudio_set_library_output_name(pdfimport pdfimport OOO)

install(TARGETS pdfimport LIBRARY DESTINATION program)

# --- xpdfimport -- Executable_xpdfimport.mk ---------------------------------
#
# The actual PDF-parsing helper process pdfimport's wrapper.cxx spawns over a
# pipe (see the header comment above) -- not a UNO component, so no udkapi/
# offapi headers. gb_Executable_use_externals,xpdfimport: boost_headers,
# expat, freetype, libpng, poppler, zlib, and (on Linux, same conditional
# pdfimport's own fontconfig use above already carries and is already
# unconditional in this Linux-only port) fontconfig.
#
# poppler -- two real prerequisites, checked directly rather than assumed.
# poppler itself is typically present on the system (`pkg-config --exists
# poppler` succeeds). But pdfioutdev_gpl.hxx's own `#include <cpp/poppler-
# version.h>` needs the *separate* poppler-cpp module for its POPPLER_
# VERSION_MAJOR/MINOR/MICRO macros -- and poppler-cpp may still be missing
# (only the runtime libpoppler-cpp.so.* installed, no -devel package,
# `pkg-config --exists poppler-cpp` exits 1). configure.ac checks both
# together too (PKG_CHECK_MODULES(POPPLER,[poppler >= 0.14 poppler-cpp]),
# configure.ac:13201) and falls back to a fully bundled build the moment
# either is missing -- SYSTEM_POPPLER, a real upstream distinction, not
# invented here. external/poppler/poppler.mk (read in full) compiles ~117
# of poppler's own .cc/.c files directly; xpdfimport only ever needs the 3
# version macros from that one header (grepped, no `poppler::` symbol
# anywhere in xpdfwrapper/), so vendoring core poppler alone is enough; no
# separate poppler-cpp build is needed.
#
# Routed through librestudio_pkgconfig() (cmake/LibreStudioTarballs.cmake)
# -- MODULES names both poppler and poppler-cpp (>1 module, so NAME stays
# explicit and no auto-detection candidate exists -- DRIVER given
# explicitly, short name "poppler"). Exports (CACHE INTERNAL):
#   LIBRESTUDIO_POPPLER_TARGET        -- link against these
#   LIBRESTUDIO_POPPLER_INCLUDE_DIRS  -- add to target_include_directories
#   LIBRESTUDIO_POPPLER_DEFS          -- add to target_compile_definitions
# INCLUDE_DIRS is kept a separate variable rather than folded into the
# poppler target's own INTERFACE_INCLUDE_DIRECTORIES, because the set a
# consumer needs (root + poppler/ + goo/, no fofi/) is genuinely narrower
# than what the library needs to compile itself.
set(ENABLE_POPPLER TRUE CACHE BOOL
    "configure.ac's ENABLE_POPPLER: build xpdfimport, the poppler-backed PDF-parsing helper process pdfimport spawns (system poppler+poppler-cpp if both present, the download.lst tarball otherwise)")

# 2026-09-29, found via the real remote-build test: pkg-config succeeding for
# both `poppler` and `poppler-cpp` does NOT guarantee the *private*,
# xpdf-derived headers pdfioutdev_gpl.hxx `#include`s (GfxState.h, GfxFont.h,
# PDFDoc.h, ...) are actually installed. Reproduced live on a stock Ubuntu
# 26.04 LTS box: `libpoppler-dev` (26.01.0-2ubuntu0.1) ships only
# /usr/include/poppler/cpp/ (the public C++ wrapper API poppler-cpp.pc
# advertises) -- zero of the internal Gfx*/PDFDoc*.h headers -- so `#include
# <GfxState.h>` fails with a real, reproducible "fatal error: GfxState.h: No
# such file or directory" despite POPPLER_FOUND=TRUE and a correct
# `-isystem /usr/include/poppler` on the compile line (verified directly:
# `ls /usr/include/poppler/*.h` is empty on that box).
#
# This is not a bug in the pkg-config check specifically -- upstream's own
# configure.ac (13201: `PKG_CHECK_MODULES(POPPLER,[poppler >= 0.14
# poppler-cpp])`) has exactly the same gap and would fail the identical way
# under `--with-system-poppler=yes` on this same machine. But re-reading
# configure.ac's own default chain (2633: `with_system_poppler=
# "$with_system_libs"`; 2515-2518: `with_system_libs`'s own AC_ARG_WITH has
# an EMPTY action-if-not-given, so it stays unset, not "yes", unless a distro
# build passes --with-system-libs/--with-system-poppler explicitly) confirms
# a bare `./configure` with no flags -- this port's own stated baseline --
# takes the `else` branch at configure.ac:13205 (`SYSTEM_POPPLER=` /
# "internal") every time, the same as jpeg/expat/zlib/etc.'s own
# with_system_*="$with_system_libs" defaults. So vendoring-by-default here is
# not a deviation from those other externals' own established system-or-
# vendor router convention -- it is what a bare configure of THIS specific
# external actually does, discovered only now because it took a real build
# to surface (the pkg-config-only FOUND_VAR check this port's router already
# uses for the other 11 externals happens to be accurate for all of them;
# poppler is the first case found where it silently isn't).
option(LIBRESTUDIO_FORCE_VENDOR_POPPLER
    "Force the vendored (download.lst) poppler build even when pkg-config finds system poppler+poppler-cpp -- see the header comment just above this option for why ON is the correct default here specifically."
    ON)

if (ENABLE_POPPLER)
    librestudio_pkgconfig(
        NAME POPPLER
        MODULES poppler poppler-cpp
        DRIVER poppler
        FOUND_VAR _poppler_found
        FORCE_VENDOR_VAR LIBRESTUDIO_FORCE_VENDOR_POPPLER)

    if (_poppler_found)
        # ifneq($(SYSTEM_POPPLER),): only defined on this branch. Skips
        # wrapper_gpl.cxx's own "#ifndef SYSTEM_POPPLER" block (a
        # LIBO_SHARE_FOLDER-based fallback for a bundled poppler's font
        # config, not needed against the system one). INCLUDE_DIRS stays
        # empty here (unlike the vendored branch) -- PkgConfig::POPPLER
        # already carries its own via IMPORTED_TARGET.
        set(LIBRESTUDIO_POPPLER_INCLUDE_DIRS "" CACHE INTERNAL "" FORCE)
        set(LIBRESTUDIO_POPPLER_DEFS BOOST_ALL_NO_LIB SYSTEM_POPPLER CACHE INTERNAL "" FORCE)
    endif ()
    unset(_poppler_found)

    find_package(EXPAT REQUIRED)
    find_package(Freetype REQUIRED)
    # JPEG needed because poppler.a (the vendored branch) links libjpeg
    # internally, and a static archive does not pull its own PRIVATE
    # dependencies into a consumer's link line -- resolved again here, in
    # this scope, same pattern as ZLIB/PNG below. 2026-09-29: now goes
    # through librestudio_find_package() -- external/poppler/poppler.cmake
    # already resolves the same NAME JPEG itself, so this call is instant
    # (memoized via the router's own GLOBAL PROPERTY cache) regardless of
    # which of the two happens to run first. See external/libjpeg-turbo/
    # libjpeg-turbo.cmake's own header comment for why this was deferred
    # until now (NASM/SIMD) and why it is librestudio_find_package(), not
    # librestudio_pkgconfig() (configure.ac probes it via plain AC_CHECK_
    # HEADER/AC_CHECK_LIB, not pkg-config).
    librestudio_find_package(NAME JPEG DRIVER libjpeg-turbo
        FORCE_VENDOR_VAR LIBRESTUDIO_FORCE_VENDOR_LIBJPEG_TURBO)

    set(XPDFIMPORT_SOURCES
        sdext/source/pdfimport/xpdfwrapper/pdfioutdev_gpl
        sdext/source/pdfimport/xpdfwrapper/pnghelper
        sdext/source/pdfimport/xpdfwrapper/wrapper_gpl
    )
    librestudio_expand_cxx(XPDFIMPORT_SOURCES)
    add_executable(xpdfimport ${XPDFIMPORT_SOURCES})

    target_compile_definitions(xpdfimport PRIVATE ${LIBRESTUDIO_POPPLER_DEFS})
    if (LIBRESTUDIO_POPPLER_INCLUDE_DIRS)
        target_include_directories(xpdfimport PRIVATE ${LIBRESTUDIO_POPPLER_INCLUDE_DIRS})
    endif ()

    target_link_libraries(xpdfimport PRIVATE
        ${LIBRESTUDIO_POPPLER_TARGET}
        PkgConfig::SDEXT_FONTCONFIG
        EXPAT::EXPAT
        Freetype::Freetype
        ${LIBRESTUDIO_PNG_TARGET}
        ${LIBRESTUDIO_JPEG_TARGET}
        ${LIBRESTUDIO_ZLIB_TARGET}
        Boost::boost)

    # gb_Executable_use_system_win32_libs,xpdfimport,shell32 -- real, but
    # inert on this Linux-only port (same shape as hwpfilter's ole32/uuid).
    if (WIN32)
        target_link_libraries(xpdfimport PRIVATE shell32)
    endif ()

    # gb_Executable_add_default_nativeres,xpdfimport embeds a Windows
    # icon/version resource; not applicable on this Linux-only port, so
    # omitted (same as opencl/CMakeLists.txt's opencltest).

    install(TARGETS xpdfimport RUNTIME DESTINATION program)
endif ()

# Package_pdfimport_xpdfimport.mk, read in full: one line,
# gb_Package_Package(sdext_pdfimport_pdf, $(SRCDIR)/sdext), adding not a
# single file to the package it declares. The actual xpdfimport binary
# already lands in program/ via install(TARGETS ...) above when
# ENABLE_POPPLER is on -- this .mk has nothing left for CMake to reproduce.

# --- CppunitTest_sdext_pdfimport --------------------------------------------
#
# CppunitTest_sdext_pdfimport.mk, re-checked directly against the live file:
# 1 source (sdext/qa/unit/pdfimport), use_sdk_api (normal udkapi+offapi),
# set_include(sdext/source/pdfimport/inc, sdext/source/pdfimport/test,
# $$(INCLUDE)), use_externals(boost_headers zlib libxml2), use_libraries(
# basegfx vcl comphelper cppu cppuhelper drawinglayercore drawinglayer sal
# salhelper test unotest tl xo i18nutil -- "test" -> test_fixtures),
# use_library_objects(pdfimport), use_executable(xpdfimport), use_ure +
# use_vcl (3 protector pairs), use_components (5 names, see below),
# use_configuration -> CONFIGURATION.
#
# Ordering, re-verified (grep -n "add_subdirectory(" CMakeLists.txt): sdext
# itself sits at line ~1125, after comphelper (725), configmgr (834),
# i18npool (730), ucb (943), unotest (1017) and test (1018) -- every module
# this test's COMPONENTS/CONFIGURATION reference is already add_subdirectory()
# 'd by sdext's own natural position, so this is wired directly here, right
# after the production pdfimport library above, no deferred file needed
# (unlike canvas_test, whose CONFIGURATION option hits this exact
# LIBRESTUDIO_TEST_UNITTEST_DIR ordering constraint from a module
# add_subdirectory()'d earlier than test -- see canvas/CppunitTest_deferred.cmake).
#
# COMPONENTS lookup, each checked against the owning module's own
# librestudio_add_component() call:
#   - "comphelper/util/comphelp" -> "comphelper" (comphelper/CMakeLists.txt --
#     PresentationMinimizer's own librestudio_add_component() call above uses
#     no such intermediate name; comphelper's own component is registered
#     under the plain module name, same mapping editeng_core's own comment
#     already established).
#   - "configmgr/source/configmgr" -> "configmgr" (configmgr/CMakeLists.txt).
#   - "i18npool/util/i18npool" -> "i18npool" (i18npool/CMakeLists.txt).
#   - "ucb/source/core/ucb1" -> "ucb1", "ucb/source/ucp/file/ucpfile1" ->
#     "ucpfile1" (both ucb/CMakeLists.txt).
#   All five resolve to real, already-registered components.
#
# use_executable(sdext_pdfimport, xpdfimport): a build-order-only dependency
# on a *file*, per librestudio_add_cppunit_test()'s own DEPENDS semantics
# (cmake/LibreStudioTest.cmake's own header comment) -- wrapper.cxx spawns
# xpdfimport as a separate short-lived process over a pipe at run time, never
# links against it. xpdfimport (see "xpdfimport" above) now exists as a real,
# conditionally-built target -- added to DEPENDS below via "if (TARGET
# xpdfimport)". ENABLE_POPPLER now always resolves to something buildable
# (system poppler+poppler-cpp, or the download.lst tarball otherwise -- see
# "xpdfimport" above), so the target should exist on every machine; the
# guard stays anyway; ENABLE_POPPLER can still be turned off by hand, and an
# unconditional name here would break configure the moment it is. The same
# one-line-if pattern setup_native/CMakeLists.txt's sellangmsi already uses
# for an optionally-built dependency.
#
# HAVE_FEATURE_POPPLER (cmake/config/config_features.h.in) stays hardcoded
# at 0 -- deliberately NOT flipped here. It is a separate, larger decision
# from "does the xpdfimport binary exist": every one of PDFITest's 13 test
# method BODIES (testXPDFParser, testOdfWriterExport, ..., testSpaces) is
# wrapped in "#if HAVE_FEATURE_POPPLER ... #endif" with nothing outside that
# guard (verified directly against qa/unit/pdfimport.cxx), so at 0 every
# method still compiles to an empty, trivially-passing function -- the
# CPPUNIT_TEST_SUITE registration itself is NOT guarded, so all 13 still
# run, assert nothing, and pass, regardless of xpdfimport's own gate above.
# Flipping HAVE_FEATURE_POPPLER to 1 would make these 13 exercise real PDF
# parsing for the first time -- a real behavioural change to verify with an
# actual build, which this pass's build policy (no cmake --build/ctest while
# porting) does not cover; left as an honest, separate follow-up.
#
# use_library_objects(sdext_pdfimport, pdfimport): modelled as a plain
# LIBRARIES pdfimport link, NOT a second recompilation of PDFIMPORT_SOURCES,
# same reasoning as ucb/CMakeLists.txt's own ucb_webdav_core (re-checked
# directly against svx_removewhichrange's precedent before choosing this):
# no *.map version-script exists anywhere under sdext/ (grepped, no matches)
# and this port sets no -fvisibility=hidden anywhere (established
# project-wide, see svx_removewhichrange's own comment) -- pdfimport's own
# production block above applies neither. PDFIRawAdaptor (pdfiadaptor.hxx),
# createDrawTreeVisitorFactory/createWriterTreeVisitorFactory
# (treevisitorfactory.hxx) and xpdf_ImportFromFile (wrapper.hxx) -- the four
# symbols qa/unit/pdfimport.cxx actually calls -- carry no DLLPRIVATE-style
# hidden-visibility marker in their own headers (grepped directly), so all
# four are already reachable by linking against the real pdfimport *shared
# library* as this port builds it.
set(SDEXT_PDFIMPORT_TEST_DEPENDS udkapi_normal_headers offapi_normal_headers)
if (TARGET xpdfimport)
    list(APPEND SDEXT_PDFIMPORT_TEST_DEPENDS xpdfimport)
endif ()

librestudio_add_cppunit_test(sdext_pdfimport
    SOURCES
        sdext/qa/unit/pdfimport
    LIBRARIES
        basegfx vcl comphelper cppu cppuhelper drawinglayercore drawinglayer
        sal salhelper test_fixtures unotest tl xo i18nutil pdfimport
        Boost::boost ${LIBRESTUDIO_ZLIB_TARGET} LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sdext/source/pdfimport/inc"
        "${CMAKE_SOURCE_DIR}/sdext/source/pdfimport/test"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS ${SDEXT_PDFIMPORT_TEST_DEPENDS}
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        comphelper configmgr i18npool ucb1 ucpfile1
    CONFIGURATION)

# qa/unit/pdfimport.cxx's PDFITest needs SRC_ROOT/WORKDIR_FOR_BUILD
# (BootstrapFixture's Directories member) -- now handled generically by
# librestudio_add_cppunit_test() itself (cmake/LibreStudioTest.cmake). This
# test's own getURLFromSrc() calls (testinput.pdf, etc.) are moot in this
# build regardless: every one sits inside "#if HAVE_FEATURE_POPPLER" (0
# here, see this test's own header comment above), so none actually execute.
