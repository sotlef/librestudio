# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# configmgr_unit (CppunitTest_configmgr_unit.mk) -- split out into its own
# file (configmgr/CMakeLists.txt has no CppunitTest section of its own; this
# is the module's only one), include()'d from the root CMakeLists.txt well
# after unoxml/ucb/test have all had their own add_subdirectory(); see that
# include() call site for the ordering rationale. In short: this test's
# COMPONENTS depend on ucb1/ucpfile1 (ucb/CMakeLists.txt) and unoxml
# (unoxml/CMakeLists.txt), both add_subdirectory()'d AFTER configmgr in this
# port's module order, and it needs test_fixtures/the protector plug-ins
# (add_subdirectory(test), likewise after) -- registering it at configmgr's
# own natural position would hit unset GLOBAL properties / undefined targets
# and abort configure, the same reordering-is-riskier trade-off already made
# for comphelper_test/sax_parser/sax_xmlimport (see comphelper/CppunitTest_
# deferred.cmake and sax/CppunitTest_deferred.cmake).
#
# CppunitTest_configmgr_unit.mk, re-checked directly against the live file:
# 1 source (configmgr/qa/unit/test), use_sdk_api (normal udkapi/offapi
# headers), use_custom_headers officecfg/registry (configmgr/qa/unit/test.cxx
# includes <officecfg/Office/Math.hxx> directly) -> OFFICECFG_INCLUDE_DIR +
# DEPENDS officecfg_headers, use_ure -> 2 protector pairs (no use_vcl in this
# .mk, unlike hwpfilter_test_hwpfilter/scaddins_analysis/xmlscript_cppunit --
# so no vclbootstrapprotector here), use_libraries comphelper cppu cppuhelper
# sal salhelper test tl unotest xmlreader i18nlangtag ("test" is
# test_fixtures here, same rename as every other CppunitTest in this port),
# use_externals boost_headers dconf icu_headers, use_components -> COMPONENTS
# below, use_configuration -> CONFIGURATION (cmake/LibreStudioTest.cmake).
#
# use_externals dconf/icu_headers are NOT reproduced: configmgr/CMakeLists.txt
# itself already does not build the dconf backend ("dconf is not installed
# here... the configuration is file-based" -- that file's own comment), so
# there is no dconf-specific code path in this test to need dconf's headers
# for either; and configmgr/qa/unit/test.cxx's own #include list (checked
# directly) uses no ICU API. boost_headers IS reproduced (Boost::boost,
# header-only, matching every other use_externals(boost_headers) call site
# in this port).
#
# COMPONENTS lookup keys, each verified against the owning module's own
# librestudio_add_component() call rather than assumed from the .mk's own
# gbuild-path spelling: "i18npool/util/i18npool" -> "i18npool" and
# "i18npool/source/search/i18nsearch" -> "i18nsearch" (both i18npool/
# CMakeLists.txt -- it really does register two separate components, one per
# name, confirmed by grep rather than assumed), "sax/source/expatwrap/
# expwrap" -> "sax" (sax/CMakeLists.txt's own librestudio_add_component(sax
# ...) call, same lookup key sax_xmlimport already uses for the identical
# component), "ucb/source/core/ucb1" -> "ucb1" and "ucb/source/ucp/file/
# ucpfile1" -> "ucpfile1" (both ucb/CMakeLists.txt), "unoxml/source/service/
# unoxml" -> "unoxml" (unoxml/CMakeLists.txt).
#
# What is deliberately NOT modelled: the .mk also calls
# gb_CppunitTest_use_library_objects(configmgr_unit,configmgr) and
# gb_CppunitTest_set_componentfile(configmgr_unit,configmgr/source/
# configmgr). Unlike sal_osl_file_details/jvmfwk_sunversion's use of
# use_library_objects (recompiling a module's sources straight into a
# CppunitTest because the qa .cxx reaches for an internal, non-exported
# symbol -- see sal/CMakeLists.txt's long comment on that technique),
# configmgr/qa/unit/test.cxx's own #include list (checked directly) names no
# configmgr/source/*.hxx internal header at all -- every include is a public
# UNO API type (css::configuration::ReadOnlyAccess, theDefaultProvider,
# comphelper::configuration, the generated officecfg headers) or a
# comphelper/cppuhelper/cppunit utility. So use_library_objects here is not
# about compile-time symbol access the way it was for sal/jvmfwk; combined
# with set_componentfile (which registers the CppunitTest's own output
# binary, once built, as ALSO satisfying the "configmgr/source/configmgr"
# component identity), the real purpose upstream is bundling a *self-hosted*
# copy of the configuration-provider service directly inside this one test
# binary, rather than depending on the separately-built configmgr shared
# library or its production .component being reachable through this test's
# own UNO_SERVICES. There is no CMake equivalent of "recompile a module's
# sources into this CppunitTest AND register the resulting binary itself as
# a UNO component" anywhere in this port yet (COMPONENTS only resolves
# already-built, separately-compiled .component files by name -- see cmake/
# LibreStudioTest.cmake's own header comment) -- modelling it would mean
# inventing a new mechanism this task's brief does not cover, so it is left
# here as a genuine, flagged gap rather than either invented or silently
# skipped: this test will build and link (it needs no internal configmgr
# symbol to compile), and COMPONENTS below still gives it i18npool/
# i18nsearch/sax/ucb1/ucpfile1/unoxml, but it does NOT itself add "configmgr"
# to UNO_SERVICES (the real .mk does not either, relying entirely on the
# self-hosting trick instead) -- so whether
# css::configuration::theDefaultProvider actually resolves to a working
# implementation at run time depends on this unmodelled mechanism. A later
# wave that wants this test fully functional has to add the self-hosting
# equivalent to librestudio_add_cppunit_test() (or a sibling helper), not
# just link against the production "configmgr" library object naively.
librestudio_add_cppunit_test(configmgr_unit
    SOURCES
        configmgr/qa/unit/test
    LIBRARIES comphelper cppu cppuhelper sal salhelper test_fixtures tl unotest
        xmlreader i18nlangtag Boost::boost
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers officecfg_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
    COMPONENTS
        i18npool i18nsearch sax ucb1 ucpfile1 unoxml
    CONFIGURATION)
