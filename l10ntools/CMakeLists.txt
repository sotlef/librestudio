# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# l10ntools — the localization toolchain: host-build tools that extract and
# merge translatable strings for the various source formats used across the
# tree (help XHP, .ulf/.lng, registrymodifications-style .xcu/.xcd config,
# .xrm description files, PO files, .tree, .properties). Like svidl/cppumaker
# they run on the build machine, are not shipped, but are still installed to
# program/ alongside the other host tools. Transcribed from
# Module_l10ntools.mk's gb_Module_add_targets_for_build list (10 targets, no
# gates); nothing in this port currently invokes them (matches idl's own
# history before sfx2 needed it) — add_subdirectory(l10ntools) is added by
# the coordinator separately, near idl.

find_package(FLEX 2.6 REQUIRED)

set(L10NTOOLS_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

# --- transex ------------------------------------------------------------
#
# Shared helper code (PO handling, merge helpers, common string utilities)
# linked by 8 of the 9 executables below (all but idxdict). STATIC, like
# codemaker's StaticLibrary_codemaker* targets: no install, and externals are
# PUBLIC so consumers inherit them without repeating use_externals.

set(TRANSEX_SOURCES
    l10ntools/source/helper
    l10ntools/source/common
    l10ntools/source/merge
    l10ntools/source/po
)

librestudio_expand_cxx(TRANSEX_SOURCES)

add_library(transex STATIC ${TRANSEX_SOURCES})

target_include_directories(transex PRIVATE "${CMAKE_SOURCE_DIR}/l10ntools/inc")

target_link_libraries(transex PUBLIC Boost::boost LibXml2::LibXml2)

# --- helpex ---------------------------------------------------------------
#
# Extracts/merges translatable strings from help XHP sources.

set(HELPEX_SOURCES
    l10ntools/source/helpex
    l10ntools/source/xmlparse
    l10ntools/source/helpmerge
)

librestudio_expand_cxx(HELPEX_SOURCES)

add_executable(helpex ${HELPEX_SOURCES})

target_include_directories(helpex PRIVATE "${CMAKE_SOURCE_DIR}/l10ntools/inc")

target_link_libraries(helpex PRIVATE
    sal
    transex
    EXPAT::EXPAT
    Boost::boost
    LibXml2::LibXml2
    ICU::uc
    ICU::i18n)

install(TARGETS helpex RUNTIME DESTINATION program)

# --- idxdict ----------------------------------------------------------------
#
# Fully standalone: no use_libraries, no use_static_libraries (not even
# transex), no use_externals, no set_include in Executable_idxdict.mk --
# confirmed, it has nothing beyond gb_Executable_Executable /
# add_exception_objects.

set(IDXDICT_SOURCES
    l10ntools/source/idxdict/idxdict
)

librestudio_expand_cxx(IDXDICT_SOURCES)

add_executable(idxdict ${IDXDICT_SOURCES})

install(TARGETS idxdict RUNTIME DESTINATION program)

# --- ulfex --------------------------------------------------------------
#
# Extracts/merges translatable strings from .ulf/.lng files.

set(ULFEX_SOURCES
    l10ntools/source/lngmerge
    l10ntools/source/lngex
)

librestudio_expand_cxx(ULFEX_SOURCES)

add_executable(ulfex ${ULFEX_SOURCES})

target_include_directories(ulfex PRIVATE "${CMAKE_SOURCE_DIR}/l10ntools/inc")

target_link_libraries(ulfex PRIVATE
    sal
    transex
    Boost::boost
    LibXml2::LibXml2)

install(TARGETS ulfex RUNTIME DESTINATION program)

# --- cfgex --------------------------------------------------------------
#
# Extracts/merges translatable strings from .xcu/.xcd-style config sources.
# cfglex.l is a standalone flex scanner (%option yylineno nounput
# never-interactive, no .y grammar) -- gbuild invokes flex directly and
# compiles the generated .cxx as an exception object. cfg_yy_wrapper.c /
# cfg_yy.c under l10ntools/source are dead leftovers from the pre-gbuild
# build; Executable_cfgex.mk references neither (verified: only
# gb_Executable_add_scanners,cfgex,l10ntools/source/cfglex and
# gb_Executable_add_exception_objects,cfgex,l10ntools/source/cfgmerge).

flex_target(CfgLexScanner
    "${CMAKE_SOURCE_DIR}/l10ntools/source/cfglex.l"
    "${L10NTOOLS_GENERATED_DIR}/cfglex.cxx")

set(CFGEX_SOURCES
    l10ntools/source/cfgmerge
)

librestudio_expand_cxx(CFGEX_SOURCES)

add_executable(cfgex
    ${CFGEX_SOURCES}
    ${FLEX_CfgLexScanner_OUTPUTS})

target_include_directories(cfgex PRIVATE "${CMAKE_SOURCE_DIR}/l10ntools/inc")

target_link_libraries(cfgex PRIVATE
    sal
    transex
    Boost::boost
    LibXml2::LibXml2)

install(TARGETS cfgex RUNTIME DESTINATION program)

# --- xrmex --------------------------------------------------------------
#
# Extracts/merges translatable strings from .xrm description files. Same
# shape as cfgex (standalone flex scanner, xrm_yy_wrapper.c / xrm_yy.c are
# dead leftovers not referenced by Executable_xrmex.mk), but xrmex also needs
# ICU (icu_headers/icuuc/icui18n) where cfgex does not -- verified against
# the two .mk files' use_externals lists.

flex_target(XrmLexScanner
    "${CMAKE_SOURCE_DIR}/l10ntools/source/xrmlex.l"
    "${L10NTOOLS_GENERATED_DIR}/xrmlex.cxx")

set(XRMEX_SOURCES
    l10ntools/source/xrmmerge
)

librestudio_expand_cxx(XRMEX_SOURCES)

add_executable(xrmex
    ${XRMEX_SOURCES}
    ${FLEX_XrmLexScanner_OUTPUTS})

target_include_directories(xrmex PRIVATE "${CMAKE_SOURCE_DIR}/l10ntools/inc")

target_link_libraries(xrmex PRIVATE
    sal
    transex
    Boost::boost
    LibXml2::LibXml2
    ICU::uc
    ICU::i18n)

install(TARGETS xrmex RUNTIME DESTINATION program)

# --- localize -----------------------------------------------------------

set(LOCALIZE_SOURCES
    l10ntools/source/localize
)

librestudio_expand_cxx(LOCALIZE_SOURCES)

add_executable(localize ${LOCALIZE_SOURCES})

target_include_directories(localize PRIVATE "${CMAKE_SOURCE_DIR}/l10ntools/inc")

target_link_libraries(localize PRIVATE
    sal
    transex
    Boost::boost
    LibXml2::LibXml2)

install(TARGETS localize RUNTIME DESTINATION program)

# --- pocheck --------------------------------------------------------------

set(POCHECK_SOURCES
    l10ntools/source/pocheck
)

librestudio_expand_cxx(POCHECK_SOURCES)

add_executable(pocheck ${POCHECK_SOURCES})

target_include_directories(pocheck PRIVATE "${CMAKE_SOURCE_DIR}/l10ntools/inc")

target_link_libraries(pocheck PRIVATE
    sal
    transex
    Boost::boost
    LibXml2::LibXml2)

install(TARGETS pocheck RUNTIME DESTINATION program)

# --- propex -------------------------------------------------------------

set(PROPEX_SOURCES
    l10ntools/source/propmerge
    l10ntools/source/propex
)

librestudio_expand_cxx(PROPEX_SOURCES)

add_executable(propex ${PROPEX_SOURCES})

target_include_directories(propex PRIVATE "${CMAKE_SOURCE_DIR}/l10ntools/inc")

target_link_libraries(propex PRIVATE
    sal
    transex
    Boost::boost
    LibXml2::LibXml2)

install(TARGETS propex RUNTIME DESTINATION program)

# --- treex --------------------------------------------------------------

set(TREEX_SOURCES
    l10ntools/source/treemerge
    l10ntools/source/treex
)

librestudio_expand_cxx(TREEX_SOURCES)

add_executable(treex ${TREEX_SOURCES})

target_include_directories(treex PRIVATE "${CMAKE_SOURCE_DIR}/l10ntools/inc")

target_link_libraries(treex PRIVATE
    sal
    transex
    Boost::boost
    LibXml2::LibXml2)

install(TARGETS treex RUNTIME DESTINATION program)
