# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# svl_urihelper (CppunitTest_svl_urihelper.mk) and svl_qa_cppunit
# (CppunitTest_svl_qa_cppunit.mk) -- split out of svl/CMakeLists.txt into
# their own file, include()'d from the root CMakeLists.txt only after every
# module either one's COMPONENTS/PROTECTORS reference has already had its
# own add_subdirectory(); see that include() call site for the exact
# ordering rationale (svl_qa_cppunit's dependency on lingucomponent is the
# binding constraint for both -- see the pointer comment left in svl/
# CMakeLists.txt, in these two tests' old place, for the short version).
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not svl's, nothing svl/CMakeLists.txt set up that is directory-
# scoped rather than global needs to be re-derived here -- svl's own
# PkgConfig::NSS/PkgConfig::OPENSSL choice is not referenced by either test
# below, so unlike comphelper/CppunitTest_deferred.cmake's tail this file has
# no such re-derivation to do.

# --- svl_urihelper -------------------------------------------------------------
#
# CppunitTest_svl_urihelper.mk, re-checked directly against the live file:
# 1 source (svl/qa/unit/test_URIHelper), use_external(boost_headers),
# use_sdk_api (udkapi+offapi "normal" headers), use_libraries cppu cppuhelper
# i18nlangtag sal svl tl utl, use_ure (2 protector pairs -- the .mk calls no
# use_vcl, confirmed by reading the whole file, so only unobootstrapprotector
# + unoexceptionprotector, not vclbootstrapprotector), use_components
# ucb/source/core/ucb1 + ucb/source/ucp/file/ucpfile1 (before use_ure in the
# .mk) and i18npool/util/i18npool (after use_ure in the .mk -- gbuild allows
# use_components to be called more than once per test, both calls
# accumulate into the same UNO_SERVICES list; order between the two calls
# does not matter for librestudio_add_cppunit_test()'s single COMPONENTS
# list either). No set_include, no CONFIGURATION.
#
# COMPONENTS lookup keys, each verified against the owning module's own
# librestudio_add_component() call rather than assumed from the .mk's own
# gbuild-path spelling (grep -n "librestudio_add_component(" against each
# file): "ucb/source/core/ucb1" -> "ucb1" and "ucb/source/ucp/file/ucpfile1"
# -> "ucpfile1" (both ucb/CMakeLists.txt), "i18npool/util/i18npool" ->
# "i18npool" (i18npool/CMakeLists.txt) -- all three spellings match this
# port's already-established mapping (configmgr/CppunitTest_deferred.cmake's
# configmgr_unit uses the identical ucb1/ucpfile1/i18npool keys for the same
# gbuild paths).
librestudio_add_cppunit_test(svl_urihelper
    SOURCES
        svl/qa/unit/test_URIHelper
    LIBRARIES cppu cppuhelper i18nlangtag sal svl tl utl Boost::boost
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
    # 2026-09-29, found via real ctest run: testNormalizedMakeRelative/
    # testResolveIdnaHost need com.sun.star.uri.UriReferenceFactory, supplied
    # by stoc's "stocservices" component (stoc/source/uriproc/
    # UriReferenceFactory.cxx, registered as stoc_add_library(stocservices
    # ...) in stoc/CMakeLists.txt) -- real "component context fails to supply
    # service com.sun.star.uri.UriReferenceFactory" UNO exception without it.
    COMPONENTS
        ucb1 ucpfile1 i18npool stocservices)

# --- svl_qa_cppunit --------------------------------------------------------------
#
# CppunitTest_svl_qa_cppunit.mk, re-checked directly against the live file:
# 1 source (svl/qa/unit/svl, i.e. svl/qa/unit/svl.cxx -- ~2114 lines, a
# large CPPUNIT_TEST_SUITE aggregator of many sub-cases, still just ONE
# source file for gb_CppunitTest_add_exception_objects/CMake purposes),
# use_sdk_api, use_externals(boost_headers icui18n icuuc icu_headers),
# use_libraries comphelper cppu cppuhelper i18nlangtag sal sot svl tl unoidl
# unotest utl, set_include $$(INCLUDE) (a no-op -- $(INCLUDE) is gbuild's own
# ambient system-include accumulator, already fully subsumed here by
# use_sdk_api's normal udkapi/offapi headers, so nothing extra to add),
# use_components (4 names, below), use_ure (2 protector pairs -- no use_vcl
# in this .mk either, confirmed by reading the whole file, so only the same
# 2 pairs svl_urihelper above gets), use_configuration -> CONFIGURATION.
#
# icu externals: icu_headers+icui18n+icuuc collapse to ICU::uc + ICU::i18n
# (the same global targets from the root CMakeLists.txt every other module's
# identical use_externals(icu_headers icui18n icuuc) trio already resolves
# to -- see e.g. sc/CMakeLists.txt's own external-packages comment for this
# exact mapping spelled out).
#
# COMPONENTS lookup keys, each verified against the owning module's own
# librestudio_add_component() call (grep -n "librestudio_add_component("):
#   - "i18npool/util/i18npool" -> "i18npool" (i18npool/CMakeLists.txt).
#   - "configmgr/source/configmgr" -> "configmgr" (configmgr/CMakeLists.txt
#     line 71: librestudio_add_component(configmgr ...) -- configmgr was
#     ported earlier this session, and it really does register its
#     component under the plain name "configmgr", matching the gbuild path's
#     own last segment here, not shortened or renamed the way ucb1/ucpfile1/
#     i18npool are).
#   - "framework/util/fwk" -> "fwk" (framework/CMakeLists.txt line 221:
#     librestudio_add_component(fwk COMPONENT ".../framework/util/fwk.component"
#     ...) -- registered under "fwk", the same short name framework/
#     CMakeLists.txt's own add_library(fwk SHARED ...) target uses,
#     confirmed by reading the file directly rather than guessed from the
#     gbuild path).
#   - "lingucomponent/source/numbertext/numbertext" -> "numbertext"
#     (lingucomponent/CMakeLists.txt line 245: librestudio_add_component(
#     numbertext ...) -- lingucomponent registers five separate components
#     (guesslang, hyphen, lnth, spell, numbertext, plus LanguageTool), and
#     "numbertext" is the one matching this path; verified, not assumed).
# All four resolve to real, already-registered components -- nothing here
# needed to be flagged as unresolved.
#
# framework/util/fwk and vcl, transitively: framework/CMakeLists.txt's own
# fwk target does link vcl (target_link_libraries(fwk PRIVATE ... svt tk ...
# vcl ...), confirmed by reading the file directly) -- BUT that link is
# PRIVATE to fwk's own shared-library build, and more importantly,
# librestudio_add_cppunit_test()'s COMPONENTS parameter (cmake/
# LibreStudioTest.cmake) never calls target_link_libraries() against a named
# component's owning library at all -- it only add_dependencies()'s the
# test on that component's own "<name>_component" custom target (a build-
# order dependency, so the .component XML fragment + its LAYER copy exist on
# disk before the test runs) and appends the component's file:// URL to a
# single -env:UNO_SERVICES=... test argument, consumed by the UNO bootstrap
# at RUN time via dlopen(), not at CMake link time. So svl_qa_cppunit itself
# never links fwk (or vcl) at all through COMPONENTS -- only LIBRARIES would
# do that, and fwk/vcl appear in neither this test's own LIBRARIES list nor
# transitively through anything that IS in it (comphelper, cppu, cppuhelper,
# i18nlangtag, sal, sot, svl, tl, unoidl, unotest, utl -- none of those link
# vcl either, per their own CMakeLists.txt PRIVATE link lists). Concretely:
# no CMake link-graph issue, no configure-time failure risk, and nothing
# needed to be forced through or worked around here.
librestudio_add_cppunit_test(svl_qa_cppunit
    SOURCES
        svl/qa/unit/svl
    LIBRARIES comphelper cppu cppuhelper i18nlangtag sal sot svl tl unoidl
        unotest utl Boost::boost ICU::uc ICU::i18n
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
    COMPONENTS
        i18npool configmgr fwk numbertext
    CONFIGURATION)

# --- svl_lockfiles -------------------------------------------------------------
#
# Un-deferred 2026-09-27: svl/CMakeLists.txt's own "DEFERRED, not ported" note
# on this test (left in place just above where the test used to sit) already
# documents that its original justification -- "needs svt, sw, and vcl, none
# of which are part of this port's currently-completed dependency chain" --
# went stale once svt/vcl/sw were all actually ported, and that the only
# remaining real issue is ordering (sw is add_subdirectory()'d hundreds of
# lines after svl, and use_rdb(services) needs program_services_rdb, defined
# near the very end of the root CMakeLists.txt) -- exactly what this file's
# own cmake_language(DEFER) scheduling (see svl/CMakeLists.txt's tail) already
# solves for svl_urihelper/svl_qa_cppunit above. Ported here, into the same
# already-deferred file, rather than opening a fourth one.
#
# CppunitTest_svl_lockfiles.mk, re-checked directly against the live file: 1
# source (svl/qa/unit/lockfiles/test_lockfiles), use_external(boost_headers),
# use_sdk_api (normal udkapi+offapi) PLUS a separate use_api(udkapi offapi
# oovbaapi) call -- the first two names are a redundant re-assertion of what
# use_sdk_api already grants (harmless, same normal variant), but oovbaapi is
# new here and needs its own normal headers (oovbaapi/CMakeLists.txt's own
# librestudio_uno_api_headers(oovbaapi_normal ...) call -- same "oovbaapi_
# normal_HEADERS_INCLUDE_DIR" / "oovbaapi_normal_headers" pattern already
# used by reportdesign_basic_test this session), use_ure + use_vcl -> 3
# protector pairs, use_libraries comphelper cppu cppuhelper tl sal svl svt sw
# test unotest utl vcl ("test" -> test_fixtures), use_rdb(services) ->
# DEPENDS program_services_rdb (no use_components in this .mk -- re-checked,
# there really is none, so no COMPONENTS argument), use_custom_headers(
# officecfg/registry) -> OFFICECFG_INCLUDE_DIR + DEPENDS officecfg_headers,
# use_configuration -> CONFIGURATION.
librestudio_add_cppunit_test(svl_lockfiles
    SOURCES
        svl/qa/unit/lockfiles/test_lockfiles
    LIBRARIES
        comphelper cppu cppuhelper tl sal svl svt sw test_fixtures unotest
        utl vcl Boost::boost
    INCLUDE
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        officecfg_headers udkapi_normal_headers offapi_normal_headers
        oovbaapi_normal_headers program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)
