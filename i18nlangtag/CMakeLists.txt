# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# i18nlangtag — language tag parsing and canonicalisation, on top of ICU and
# liblangtag.  comphelper depends on it, which is how it ends up in the UNO
# core's dependency closure.
# Source list from Library_i18nlangtag.mk.

set(I18NLANGTAG_SOURCES
    i18nlangtag/source/isolang/insys
    i18nlangtag/source/isolang/isolang
    i18nlangtag/source/isolang/mslangid
    i18nlangtag/source/languagetag/languagetag
    i18nlangtag/source/languagetag/languagetagicu)

librestudio_expand_cxx(I18NLANGTAG_SOURCES)

add_library(i18nlangtag SHARED ${I18NLANGTAG_SOURCES})

# gb_Library_use_sdk_api: the normal bindings of both udkapi and offapi
# (languagetag.cxx includes com/sun/star/i18n/ScriptType.hpp, which is offapi).
target_include_directories(i18nlangtag PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(i18nlangtag udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(i18nlangtag PRIVATE I18NLANGTAG_DLLIMPLEMENTATION)
target_link_libraries(i18nlangtag PRIVATE sal ICU::uc PkgConfig::LIBLANGTAG)

# Library_i18nlangtag.mk: $(eval $(call gb_Library_use_system_win32_libs,
# i18nlangtag,$(if $(filter $(COM),MSC), kernel32))) -- kernel32 only under
# MSVC. Kept structurally present, behind if (MSVC), per this port's policy
# of not silently dropping Windows-only gbuild targets -- see sal/
# CMakeLists.txt's sal_comtools/sal_retry_if_failed and cpputools/
# CMakeLists.txt's sp2bv for the same pattern. Inert on this Linux build.
if (MSVC)
    target_link_libraries(i18nlangtag PRIVATE kernel32)
endif ()

# Repository.mk registers i18nlangtag under PLAINLIBS_URE, the same category
# as jpipe (jurt/CMakeLists.txt) and juh/juhx (javaunohelper/CMakeLists.txt),
# both already ported with layer PLAIN. solenv/gbuild/platform/unxgcc.mk's
# gb_Library_FILENAMES shows PLAINLIBS_URE and PLAINLIBS_OOO both resolve to
# the bare libNAME.so pattern (gb_Library_PLAINEXT) -- the "lo" suffix
# (gb_Library_OOOEXT) belongs only to the distinct OOOLIBS category, which
# i18nlangtag is not a member of. Was wrongly OOO (-> i18nlangtaglo.so) from
# an earlier wave of this port; corrected to PLAIN (-> libi18nlangtag.so) to
# match its real gbuild category and its PLAINLIBS_URE siblings.
librestudio_set_library_output_name(i18nlangtag i18nlangtag PLAIN)

install(TARGETS i18nlangtag LIBRARY DESTINATION program)

# --- CppunitTest_i18nlangtag_test_languagetag -------------------------------
#
# CppunitTest_i18nlangtag_test_languagetag.mk, re-checked directly against the
# live file: 1 source (i18nlangtag/qa/cppunit/test_languagetag),
# use_sdk_api (normal udkapi+offapi), use_libraries(i18nlangtag cppu cppuhelper
# sal), use_externals(icu_headers icuuc liblangtag), an
# ifneq($(SYSTEM_LIBLANGTAG),) branch (-DSYSTEM_LIBLANGTAG define vs.
# use_package(liblangtag_data) for a vendored copy). No use_ure/use_vcl/
# use_components/use_configuration anywhere in this .mk, so no PROTECTORS/
# COMPONENTS/CONFIGURATION and no ordering constraint against anything
# add_subdirectory()'d after i18nlangtag -- wired directly here, right after
# the production library above.
#
# The SYSTEM_LIBLANGTAG branch is not actually conditional in this port:
# the root CMakeLists.txt's own liblangtag resolution
# (`pkg_check_modules(LIBLANGTAG REQUIRED IMPORTED_TARGET liblangtag>=0.4.0)`,
# CMakeLists.txt line ~245) is unconditional -- there is no vendored
# external/liblangtag build anywhere in this port's CMake side (grepped
# project-wide for "liblangtag_data"/"ExternalProject.*liblangtag", no
# matches outside the .mk tree itself) -- so this test always takes the
# SYSTEM_LIBLANGTAG=true branch: -DSYSTEM_LIBLANGTAG, no use_package.
#
# icu_headers/icuuc -> ICU::uc (same external-name collapsing already
# established by i18nlangtag's own production block above, and by
# basic/CppunitTest_deferred.cmake's own comment on the identical
# icu_headers+icuuc+icui18n -> ICU::uc[+ICU::i18n] mapping). liblangtag ->
# PkgConfig::LIBLANGTAG (root CMakeLists.txt's own IMPORTED_TARGET, reused
# as-is, same target i18nlangtag's own production block links).
#
# qa/cppunit/test_languagetag.cxx (grepped directly): a plain
# CppUnit::TestFixture, no test::BootstrapFixture/BootstrapFixtureBase
# anywhere in the file -- confirms the .mk's own lack of use_ure is not an
# oversight, this test never bootstraps UNO at all, it exercises
# i18nlangtag::LanguageTag/MsLangId directly.
librestudio_add_cppunit_test(i18nlangtag_test_languagetag
    SOURCES
        i18nlangtag/qa/cppunit/test_languagetag
    LIBRARIES
        i18nlangtag cppu cppuhelper sal
        ICU::uc PkgConfig::LIBLANGTAG
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEFINITIONS SYSTEM_LIBLANGTAG
    DEPENDS udkapi_normal_headers offapi_normal_headers)
