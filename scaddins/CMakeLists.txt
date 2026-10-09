# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# scaddins — three independent Calc add-in libraries, each a handful of
# exception objects: analysis (Library_analysis.mk, the Analysis ("Excel
# compatibility") function set), date (Library_date.mk, DateFunctions/
# MiscFunctions), pricing (Library_pricing.mk, PricingFunctions/Black-Scholes).
#
# Module_scaddins.mk's remaining target, AllLangMoTarget_sca (.po -> .mo
# translation compilation for the three add-ins' localized function/argument
# names), is UI-localization/install machinery outside this port's scope,
# the same "not a build target" call already made for AllLangMoTarget_uui,
# _fps, _dkt, _xsc, _shell, _wpt and every other module surveyed so far (see
# e.g. uui/CMakeLists.txt, fpicker/CMakeLists.txt, desktop/CMakeLists.txt,
# xmlsecurity/CMakeLists.txt, shell/CMakeLists.txt, writerperfect/
# CMakeLists.txt) -- not a one-off skip, so not singled out for a different
# treatment here just because this wave happened to look at it again. Its own
# CppunitTest_scaddins_analysis.mk (Module_scaddins.mk's other check target)
# IS wired below, deferred into scaddins/CppunitTest_deferred.cmake.
#
# All three implement services declared by scaddins' own private UNO API
# (InternalUnoApi_scaddins.mk): eight .idl files under
# com/sun/star/sheet/addin/ (Analysis, DateFunctions, PricingFunctions,
# XAnalysis, XCompatibilityNames, XDateFunctions, XPricingFunctions,
# XMiscFunctions). That API is compiled here, once, into a registry and the
# C++ headers all three libraries include (analysis.hxx, datefunc.hxx,
# pricing.hxx each pull in their X*Functions.hpp) — the same two-step
# rdb-then-headers pattern oovbaapi/CMakeLists.txt uses for the VBA API.
#
# Each Library_*.mk calls gb_Library_use_internal_comprehensive_api, not
# gb_Library_use_sdk_api/gb_Library_use_api: per
# gb_LinkTarget_use_internal_comprehensive_api (solenv/gbuild/LinkTarget.mk),
# that resolves offapi, scaddins *and* udkapi to their "comprehensive" (-C)
# header variant, not "normal" -- the same variant cppuhelper already builds
# udkapi/offapi in ("comprehensive" inlines the dependency's type information,
# which is why cppuhelper/CMakeLists.txt reaches for it too). So scaddins'
# own headers are generated as the comprehensive variant here, and the
# already-built udkapi_comprehensive_HEADERS_INCLUDE_DIR /
# offapi_comprehensive_HEADERS_INCLUDE_DIR (from udkapi/CMakeLists.txt and
# offapi/CMakeLists.txt) are used instead of the _normal_ ones other modules
# reach for.
#
# The scaddins .idl files reference offapi types directly (XAnalysis.idl uses
# com::sun::star::beans::XPropertySet and com::sun::star::lang::
# IllegalArgumentException), matching InternalUnoApi_scaddins.mk's
# gb_InternalUnoApi_use_api(scaddins, offapi udkapi) and
# gb_InternalUnoApi_define_api_dependencies(scaddins, offapi, udkapi) — so
# both udkapi_RDB and offapi_RDB are given as DEP_RDBS/EXTRA_RDBS below.
#
# gb_InternalUnoApi_InternalUnoApi (unlike gb_UnoApi_UnoApi, which oovbaapi
# uses) never installs its registry into program/types -- it is a compile-time
# type source only, matching cppuhelper's own InternalUnoApi
# (cppuhelper/unotypes) not installing cppuhelper.rdb either -- so no
# install(FILES ...) for scaddins.rdb is added here.

librestudio_uno_api_rdb(scaddins
    IDL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/idl"
    OUTPUT   "${CMAKE_BINARY_DIR}/UnoApiTarget/scaddins.rdb"
    DEP_RDBS "${udkapi_RDB}" "${offapi_RDB}")

librestudio_uno_api_headers(scaddins_comprehensive
    RDB        "${scaddins_RDB}"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/UnoApiHeadersTarget/scaddins/comprehensive"
    VARIANT    comprehensive
    EXTRA_RDBS "${udkapi_RDB}" "${offapi_RDB}")

# --- analysis -----------------------------------------------------------

set(ANALYSIS_SOURCES
    scaddins/source/analysis/analysis
    scaddins/source/analysis/analysishelper
    scaddins/source/analysis/bessel
    scaddins/source/analysis/financial
)

librestudio_expand_cxx(ANALYSIS_SOURCES)
add_library(analysis SHARED ${ANALYSIS_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(analysis REUSE_FROM precompiled_system)
endif ()

target_include_directories(analysis PRIVATE
    "${CMAKE_SOURCE_DIR}/scaddins/inc"
    "${udkapi_comprehensive_HEADERS_INCLUDE_DIR}"
    "${offapi_comprehensive_HEADERS_INCLUDE_DIR}"
    "${scaddins_comprehensive_HEADERS_INCLUDE_DIR}")
add_dependencies(analysis
    udkapi_comprehensive_headers
    offapi_comprehensive_headers
    scaddins_comprehensive_headers)

target_link_libraries(analysis PRIVATE
    comphelper cppu cppuhelper i18nlangtag sal utl)

librestudio_add_component(analysis
    COMPONENT "${CMAKE_SOURCE_DIR}/scaddins/source/analysis/analysis.component"
    LIBRARY   analysis
    LAYER     OOO)

librestudio_set_library_output_name(analysis analysis OOO)

install(TARGETS analysis LIBRARY DESTINATION program)

# --- date -----------------------------------------------------------------

set(DATE_SOURCES
    scaddins/source/datefunc/datefunc
)

librestudio_expand_cxx(DATE_SOURCES)
add_library(date SHARED ${DATE_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(date REUSE_FROM precompiled_system)
endif ()

target_include_directories(date PRIVATE
    "${CMAKE_SOURCE_DIR}/scaddins/inc"
    "${udkapi_comprehensive_HEADERS_INCLUDE_DIR}"
    "${offapi_comprehensive_HEADERS_INCLUDE_DIR}"
    "${scaddins_comprehensive_HEADERS_INCLUDE_DIR}")
add_dependencies(date
    udkapi_comprehensive_headers
    offapi_comprehensive_headers
    scaddins_comprehensive_headers)

target_link_libraries(date PRIVATE
    cppu cppuhelper i18nlangtag sal utl)

librestudio_add_component(date
    COMPONENT "${CMAKE_SOURCE_DIR}/scaddins/source/datefunc/date.component"
    LIBRARY   date
    LAYER     OOO)

librestudio_set_library_output_name(date date OOO)

install(TARGETS date LIBRARY DESTINATION program)

# --- pricing ----------------------------------------------------------------

set(PRICING_SOURCES
    scaddins/source/pricing/pricing
    scaddins/source/pricing/black_scholes
)

librestudio_expand_cxx(PRICING_SOURCES)
add_library(pricing SHARED ${PRICING_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(pricing REUSE_FROM precompiled_system)
endif ()

target_include_directories(pricing PRIVATE
    "${CMAKE_SOURCE_DIR}/scaddins/inc"
    "${udkapi_comprehensive_HEADERS_INCLUDE_DIR}"
    "${offapi_comprehensive_HEADERS_INCLUDE_DIR}"
    "${scaddins_comprehensive_HEADERS_INCLUDE_DIR}")
add_dependencies(pricing
    udkapi_comprehensive_headers
    offapi_comprehensive_headers
    scaddins_comprehensive_headers)

target_link_libraries(pricing PRIVATE
    cppu cppuhelper i18nlangtag sal utl)

librestudio_add_component(pricing
    COMPONENT "${CMAKE_SOURCE_DIR}/scaddins/source/pricing/pricing.component"
    LIBRARY   pricing
    LAYER     OOO)

librestudio_set_library_output_name(pricing pricing OOO)

install(TARGETS pricing LIBRARY DESTINATION program)

# scaddins/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/scaddins/CppunitTest_deferred.cmake")
