# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# extensions_bibliography (CppunitTest_extensions_bibliography.mk) and
# extensions_test_update (CppunitTest_extensions_test_update.mk) -- split out
# of extensions/CMakeLists.txt into their own file, include()'d from the root
# CMakeLists.txt only after program_services_rdb exists (and, for
# extensions_test_update, after officecfg/CMakeLists.txt's own registration of
# the officecfg_headers target -- already satisfied well before this point,
# see extensions/CMakeLists.txt's own use of it for updchk); see that
# include() call site for the ordering rationale, and scaddins/CppunitTest_
# deferred.cmake's header comment for the full use_rdb(services)/ini-fallback
# reasoning both of these follow identically (neither calls use_components,
# so both get: no COMPONENTS, a DEPENDS on program_services_rdb, and
# cppunittester's UNO bootstrap falling back to unorc's own ini default for
# UNO_SERVICES).
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), source paths below are still fine -- librestudio_expand_cxx resolves
# them against CMAKE_SOURCE_DIR, never CMAKE_CURRENT_SOURCE_DIR (same note as
# forms/CppunitTest_deferred.cmake's own).
#
# --- extensions_bibliography ------------------------------------------------
#
# CppunitTest_extensions_bibliography.mk, re-checked directly against the live
# file: 1 source (extensions/qa/bibliography/bibliography), use_externals
# (boost_headers), use_libraries comphelper cppu sal test unotest ("test" ->
# test_fixtures, this port's established rename), use_sdk_api (normal udkapi+
# offapi headers), use_ure + use_vcl -> 3 protector pairs (unobootstrap
# protector/unoexceptionprotector/vclbootstrapprotector, same precedent as
# forms_unit's own use_ure+use_vcl combination), use_rdb(services) -> DEPENDS
# program_services_rdb, use_custom_headers(officecfg/registry) ->
# OFFICECFG_INCLUDE_DIR + DEPENDS officecfg_headers (extensions/qa/
# bibliography/bibliography.cxx does not itself need this at a glance, but the
# .mk calls it unconditionally so it is reproduced as-is, matching the same
# "transcribe the .mk, don't second-guess it" posture used throughout this
# port), use_configuration -> CONFIGURATION.
#
# All libraries verified as real targets before wiring (grep -n
# "add_library(<name> "): comphelper, cppu, sal, test_fixtures (test/
# CMakeLists.txt), unotest (unotest/CMakeLists.txt). unobootstrapprotector/
# unoexceptionprotector (unotest/CMakeLists.txt) and vclbootstrapprotector
# (test/CMakeLists.txt) all resolve too, same protector targets forms_unit/
# officecfg_accelerator_key_checker_test already use.
librestudio_add_cppunit_test(extensions_bibliography
    SOURCES
        extensions/qa/bibliography/bibliography
    LIBRARIES
        comphelper cppu sal test_fixtures unotest Boost::boost
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- extensions_test_update --------------------------------------------------
#
# Module_extensions.mk nests CppunitTest_extensions_test_update inside the
# same "ifeq ($(ENABLE_ONLINE_UPDATE),TRUE)" block as Library_updchk/Library_
# updatecheckui/Configuration_updchk (see extensions/CMakeLists.txt's own
# header comment on ENABLE_ONLINE_UPDATE), and it directly needs updchk's own
# UPDCHK_SOURCES (see below) -- both are reasons this whole test is gated the
# same way here.
if (ENABLE_ONLINE_UPDATE)

# CppunitTest_extensions_test_update.mk, re-checked directly against the live
# file: 1 source (extensions/qa/update/test_update), use_external(
# boost_headers), use_libraries comphelper cppu cppuhelper sal salhelper test
# tk utl unotest vcl ("test" -> test_fixtures), use_library_objects(updchk)
# (see below), ifeq($(OS),WNT) use_system_win32_libs (shell32/ole32/wininet --
# Windows-only, not reproduced, same verdict as Library_updchk.mk's own WNT
# block in extensions/CMakeLists.txt), use_external(curl) -> CURL::libcurl,
# set_include -I$(SRCDIR)/extensions/inc, use_sdk_api (normal udkapi+offapi),
# use_ure + use_vcl -> 3 protector pairs, use_components -> COMPONENTS below,
# use_configuration -> CONFIGURATION.
#
# use_library_objects(extensions_test_update,updchk): extensions/qa/update/
# test_update.cxx includes "../../source/update/check/updatecheck.hxx" and
# "updateprotocol.hxx" directly -- internal headers under extensions/source/
# update/check, not part of updchk's public UNO API/component surface -- so
# this cannot be satisfied by linking against the updchk shared library the
# way a plain LIBRARIES entry would. Same technique as sal/CMakeLists.txt's
# own sal_osl_file_details (see that file's long comment on
# use_library_objects) and configmgr/CppunitTest_deferred.cmake's own note on
# why configmgr_unit does NOT need this treatment: here, unlike configmgr_
# unit, the qa .cxx genuinely does reach for internal, non-exported headers,
# so the updchk exception objects (UPDCHK_SOURCES, extensions/CMakeLists.txt,
# propagated to this file's own scope via "set(UPDCHK_SOURCES ... PARENT_
# SCOPE)" right after its own librestudio_expand_cxx() call) are recompiled
# directly into this MODULE below instead of linking the updchk library.
#
# COMPONENTS lookup keys, each verified against the owning module's own
# librestudio_add_component() call rather than assumed from the .mk's own
# gbuild-path spelling: "configmgr/source/configmgr" -> "configmgr"
# (configmgr/CMakeLists.txt), "extensions/source/update/feed/updatefeed" ->
# "updatefeed" (extensions/CMakeLists.txt), "ucb/source/core/ucb1" -> "ucb1"
# and "ucb/source/ucp/file/ucpfile1" -> "ucpfile1" (both ucb/CMakeLists.txt),
# "unoxml/source/service/unoxml" -> "unoxml" (unoxml/CMakeLists.txt) -- the
# identical five names/lookups configmgr_unit's own COMPONENTS (minus
# updatefeed, plus i18npool/i18nsearch/sax) already uses, re-verified here
# independently.
#
# All libraries verified as real targets before wiring: comphelper, cppu,
# cppuhelper, sal, salhelper, test_fixtures, tk, utl, unotest, vcl (all
# already used elsewhere in this port), CURL::libcurl (find_package(CURL)
# at the root, already consumed by updchk itself).
#
# OFFICECFG_INCLUDE_DIR / officecfg_headers below are NOT from this .mk's own
# use_custom_headers call (it makes none) -- they come from recompiling
# UPDCHK_SOURCES's download.cxx straight into this test, which itself needs
# them (extensions/CMakeLists.txt's own updchk block: download.cxx pulls in
# include/systools/curlinit.hxx, which needs the generated officecfg::Office::
# Security schema header, matching Library_updchk.mk's own gb_Library_use_
# custom_headers(updchk, officecfg/registry)) -- inherited here for the same
# reason, not independently re-derived.
librestudio_add_cppunit_test(extensions_test_update
    SOURCES
        ${UPDCHK_SOURCES}
        extensions/qa/update/test_update
    LIBRARIES
        comphelper cppu cppuhelper sal salhelper test_fixtures tk utl unotest
        vcl Boost::boost CURL::libcurl
    INCLUDE
        "${CMAKE_SOURCE_DIR}/extensions/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr updatefeed ucb1 ucpfile1 unoxml
    CONFIGURATION)

endif (ENABLE_ONLINE_UPDATE)
