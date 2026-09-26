# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# lingucomponent — the linguistic-backend UNO services that plug external
# NLP/spelling libraries into the linguistic dispatch layer (module lng):
# language guessing, spell checking, hyphenation, thesaurus lookup, number-
# to-text conversion and a LanguageTool grammar-checker bridge. Seven targets
# from Module_lingucomponent.mk:
#   - Library_guesslang       -> guesslang
#   - StaticLibrary_ulingu    -> ulingu       (static helper, linked into
#                                               hyphen/lnth/spell/LanguageTool)
#   - Library_hyphen          -> hyphen
#   - Library_lnth            -> lnth
#   - Library_spell           -> spell
#   - Library_numbertext      -> numbertext
#   - Library_LanguageTool    -> LanguageTool (gated in the .mk by
#                                               ENABLE_CURL, which this port
#                                               treats as unconditionally
#                                               true -- see svl/CMakeLists.txt
#                                               and linguistic/CMakeLists.txt)
#
# Library_MacOSXSpell.mk is macOS-only (Cocoa/ObjC++ exception objects,
# gb_Library_use_system_darwin_frameworks) and is not ported, same as every
# other Windows/macOS-only target in this port.
#
# IMPORTED_TARGET results from pkg_check_modules are directory-scoped, so
# hunspell/mythes/libexttextcat/libnumbertext are each (re-)declared here
# even though no other module in this tree has wired them yet (grepped: no
# hunspell/mythes/libexttextcat/libnumbertext pkg_check_modules calls exist
# anywhere else in the tree). HUNSPELL is declared once, below, and reused
# by ulingu/hyphen/lnth/spell.

# --- ulingu (StaticLibrary_ulingu.mk) -----------------------------------------
#
# SYSTEM_DICTS is never set anywhere in this port's tracked config (grepped
# repo-wide: only configure.ac, config_host.mk.in and the .mk/.cxx itself
# reference it), so the whole `ifneq ($(SYSTEM_DICTS),)` defs block resolves
# to "off" and is omitted entirely.

pkg_check_modules(HUNSPELL REQUIRED IMPORTED_TARGET hunspell)

set(ULINGU_SOURCES
    lingucomponent/source/lingutil/lingutil
)

librestudio_expand_cxx(ULINGU_SOURCES)
add_library(ulingu STATIC ${ULINGU_SOURCES})

target_include_directories(ulingu PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(ulingu udkapi_normal_headers offapi_normal_headers)

target_link_libraries(ulingu PRIVATE
    Boost::boost
    PkgConfig::HUNSPELL)

# Linked into shared libraries (hyphen, lnth, spell, LanguageTool below);
# needs PIC objects, same reasoning as shell/CMakeLists.txt's shell_xmlparser
# and package/CMakeLists.txt's argon2 STATIC libraries.
set_target_properties(ulingu PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- guesslang (Library_guesslang.mk) -----------------------------------------
#
# SYSTEM_LIBEXTTEXTCAT is required: guesslang.cxx does
# `#ifdef SYSTEM_LIBEXTTEXTCAT / #include <libexttextcat/textcat.h> / #else
# / #include <textcat.h>` -- without the define it looks for a nonexistent
# bundled header.

pkg_check_modules(LIBEXTTEXTCAT REQUIRED IMPORTED_TARGET libexttextcat)

set(GUESSLANG_SOURCES
    lingucomponent/source/languageguessing/guess
    lingucomponent/source/languageguessing/guesslang
    lingucomponent/source/languageguessing/simpleguesser
)

librestudio_expand_cxx(GUESSLANG_SOURCES)
add_library(guesslang SHARED ${GUESSLANG_SOURCES})

target_include_directories(guesslang PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(guesslang officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(guesslang PRIVATE SYSTEM_LIBEXTTEXTCAT)

target_link_libraries(guesslang PRIVATE
    cppu cppuhelper sal tl utl
    Boost::boost
    PkgConfig::LIBEXTTEXTCAT)

librestudio_add_component(guesslang
    COMPONENT "${CMAKE_SOURCE_DIR}/lingucomponent/source/languageguessing/guesslang.component"
    LIBRARY   guesslang
    LAYER     OOO)

librestudio_set_library_output_name(guesslang guesslang OOO)

install(TARGETS guesslang LIBRARY DESTINATION program)

# --- hyphen (Library_hyphen.mk) -----------------------------------------------
#
# No pkg-config module exists for hyphen -- upstream itself probes it via
# AC_CHECK_HEADER(hyphen.h)/AC_CHECK_LIB, not pkg-config -- so it is located
# directly with find_path/find_library.

find_path(HYPHEN_INCLUDE_DIR hyphen.h)
find_library(HYPHEN_LIBRARY hyphen)
if (NOT HYPHEN_INCLUDE_DIR OR NOT HYPHEN_LIBRARY)
    message(FATAL_ERROR "hyphen not found (looked for hyphen.h and libhyphen)")
endif ()

set(HYPHEN_SOURCES
    lingucomponent/source/hyphenator/hyphen/hyphenimp
)

librestudio_expand_cxx(HYPHEN_SOURCES)
add_library(hyphen SHARED ${HYPHEN_SOURCES})

target_include_directories(hyphen PRIVATE
    "${CMAKE_SOURCE_DIR}/lingucomponent/source/lingutil"
    "${HYPHEN_INCLUDE_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(hyphen officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(hyphen PRIVATE
    comphelper cppu cppuhelper i18nlangtag lng sal tl utl
    ulingu
    Boost::boost
    PkgConfig::HUNSPELL
    "${HYPHEN_LIBRARY}")

librestudio_add_component(hyphen
    COMPONENT "${CMAKE_SOURCE_DIR}/lingucomponent/source/hyphenator/hyphen/hyphen.component"
    LIBRARY   hyphen
    LAYER     OOO)

librestudio_set_library_output_name(hyphen hyphen OOO)

install(TARGETS hyphen LIBRARY DESTINATION program)

# --- lnth (Library_lnth.mk) ---------------------------------------------------

pkg_check_modules(MYTHES REQUIRED IMPORTED_TARGET mythes)

set(LNTH_SOURCES
    lingucomponent/source/thesaurus/libnth/nthesdta
    lingucomponent/source/thesaurus/libnth/nthesimp
)

librestudio_expand_cxx(LNTH_SOURCES)
add_library(lnth SHARED ${LNTH_SOURCES})

target_include_directories(lnth PRIVATE
    "${CMAKE_SOURCE_DIR}/lingucomponent/source/lingutil"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(lnth officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(lnth PRIVATE
    comphelper cppu cppuhelper i18nlangtag lng sal tl utl
    ulingu
    Boost::boost
    PkgConfig::HUNSPELL
    PkgConfig::MYTHES)

librestudio_add_component(lnth
    COMPONENT "${CMAKE_SOURCE_DIR}/lingucomponent/source/thesaurus/libnth/lnth.component"
    LIBRARY   lnth
    LAYER     OOO)

librestudio_set_library_output_name(lnth lnth OOO)

install(TARGETS lnth LIBRARY DESTINATION program)

# --- spell (Library_spell.mk) -------------------------------------------------

set(SPELL_SOURCES
    lingucomponent/source/spellcheck/spell/sspellimp
)

librestudio_expand_cxx(SPELL_SOURCES)
add_library(spell SHARED ${SPELL_SOURCES})

target_include_directories(spell PRIVATE
    "${CMAKE_SOURCE_DIR}/lingucomponent/source/lingutil"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(spell officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(spell PRIVATE
    comphelper cppu cppuhelper i18nlangtag lng sal tl utl
    ulingu
    Boost::boost
    PkgConfig::HUNSPELL
    ICU::uc)

librestudio_add_component(spell
    COMPONENT "${CMAKE_SOURCE_DIR}/lingucomponent/source/spellcheck/spell/spell.component"
    LIBRARY   spell
    LAYER     OOO)

librestudio_set_library_output_name(spell spell OOO)

install(TARGETS spell LIBRARY DESTINATION program)

# --- numbertext (Library_numbertext.mk) ---------------------------------------
#
# No boost, no hunspell, no ulingu dependency -- confirmed against the .mk
# (gb_Library_use_externals only lists libnumbertext; gb_Library_use_libraries
# only lists cppu/cppuhelper/i18nlangtag/sal/tl/utl). -DSYSTEM_LIBNUMBERTEXT
# is added for fidelity with upstream's RepositoryExternal.mk external macro,
# harmless even though numbertext.cxx does not itself branch on it.

pkg_check_modules(LIBNUMBERTEXT REQUIRED IMPORTED_TARGET libnumbertext)

set(NUMBERTEXT_SOURCES
    lingucomponent/source/numbertext/numbertext
)

librestudio_expand_cxx(NUMBERTEXT_SOURCES)
add_library(numbertext SHARED ${NUMBERTEXT_SOURCES})

target_include_directories(numbertext PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(numbertext officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(numbertext PRIVATE SYSTEM_LIBNUMBERTEXT)

target_link_libraries(numbertext PRIVATE
    cppu cppuhelper i18nlangtag sal tl utl
    PkgConfig::LIBNUMBERTEXT)

librestudio_add_component(numbertext
    COMPONENT "${CMAKE_SOURCE_DIR}/lingucomponent/source/numbertext/numbertext.component"
    LIBRARY   numbertext
    LAYER     OOO)

librestudio_set_library_output_name(numbertext numbertext OOO)

install(TARGETS numbertext LIBRARY DESTINATION program)

# --- LanguageTool (Library_LanguageTool.mk) -----------------------------------
#
# Gated in the .mk by `ifeq ($(ENABLE_CURL),TRUE)`. This port treats curl as
# a real, always-available dependency -- svl/CMakeLists.txt and
# linguistic/CMakeLists.txt both already link CURL::libcurl unconditionally
# (CMakeLists.txt:252 does find_package(CURL REQUIRED) at the root) -- so
# this target is ported normally, not wrapped in a conditional.

set(LANGUAGETOOL_SOURCES
    lingucomponent/source/spellcheck/languagetool/languagetoolimp
)

librestudio_expand_cxx(LANGUAGETOOL_SOURCES)
add_library(LanguageTool SHARED ${LANGUAGETOOL_SOURCES})

target_include_directories(LanguageTool PRIVATE
    "${CMAKE_SOURCE_DIR}/lingucomponent/source/lingutil"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(LanguageTool officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(LanguageTool PRIVATE
    comphelper cppu cppuhelper fwk i18nlangtag svt lng sal tl utl
    ulingu
    Boost::boost
    ICU::uc
    CURL::libcurl)

librestudio_add_component(LanguageTool
    COMPONENT "${CMAKE_SOURCE_DIR}/lingucomponent/source/spellcheck/languagetool/LanguageTool.component"
    LIBRARY   LanguageTool
    LAYER     OOO)

librestudio_set_library_output_name(LanguageTool LanguageTool OOO)

install(TARGETS LanguageTool LIBRARY DESTINATION program)
