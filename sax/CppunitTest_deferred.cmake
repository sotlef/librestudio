# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sax_parser (CppunitTest_sax_parser.mk) and sax_xmlimport
# (CppunitTest_sax_xmlimport.mk) -- split out of sax/CMakeLists.txt into
# their own file, include()'d from the root CMakeLists.txt well after
# configmgr/framework/i18npool/sfx2/ucb/uui have all had their own
# add_subdirectory(); see that include() call site, and the pointer comment
# left in sax/CMakeLists.txt in these tests' old place, for the full ordering
# rationale.
#
# Both also call gb_CppunitTest_use_configuration -- noted for a long time in
# this file (and in sax/CMakeLists.txt's own comment on these two tests'
# former place) as a third, separate, then-unaddressed gap on top of
# PROTECTORS/COMPONENTS. That gap is now closed generically by
# librestudio_add_cppunit_test()'s CONFIGURATION option (cmake/
# LibreStudioTest.cmake -- read its header comment for the mechanism), added
# this wave alongside officecfg_registry/test/CMakeLists.txt's staged
# test/user-template copy that CONFIGURATION depends on; both tests below now
# pass it. officecfg (CONFIGURATION's own officecfg_registry dependency) and
# test (its LIBRESTUDIO_TEST_UNITTEST_DIR dependency) are already
# add_subdirectory()'d well before this file's own include() point, so no
# further reordering was needed to add it here.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not sax's, source paths below are still fine: librestudio_expand_cxx
# (cmake/LibreStudioPlatform.cmake) resolves them against CMAKE_SOURCE_DIR
# (the absolute tree root), never CMAKE_CURRENT_SOURCE_DIR, so it does not
# matter which directory's scope this file executes in.

# CppunitTest_sax_parser.mk: 1 source (sax/qa/cppunit/parser), use_sdk_api,
# use_ure, use_vcl, use_libraries comphelper cppu cppuhelper unotest sal sax
# test -- "test" is test/CMakeLists.txt's gbuild Library_test.mk, whose CMake
# target is named test_fixtures there, not test: enable_testing() in the root
# CMakeLists.txt reserves the bare name "test" for CTest's own built-in
# target (see test/CMakeLists.txt's own comment on this), so that is the name
# used in LIBRARIES below too. use_ure's protectors
# (unobootstrapprotector/unoexceptionprotector)
# and use_vcl's (vclbootstrapprotector) are wired via PROTECTORS, same
# precedent as sax_attributes/comphelper_test. use_components names
# configmgr/source/configmgr, framework/util/fwk, i18npool/util/i18npool,
# sfx2/util/sfx, ucb/source/core/ucb1, ucb/source/ucp/file/ucpfile1 -- this
# port's librestudio_add_component() lookup keys for those are, respectively,
# "configmgr", "fwk", "i18npool", "sfx", "ucb1", "ucpfile1" (verified against
# each owning module's own librestudio_add_component(<name> ...) call, NOT
# assumed from the gbuild path spelling -- see sax/CMakeLists.txt's own
# comment on this same point for where each was confirmed).
#
# use_configuration -> CONFIGURATION, see this file's header comment.
librestudio_add_cppunit_test(sax_parser
    SOURCES
        sax/qa/cppunit/parser
    LIBRARIES comphelper cppu cppuhelper unotest sal sax test_fixtures
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr fwk i18npool sfx ucb1 ucpfile1
    CONFIGURATION)

# CppunitTest_sax_xmlimport.mk: 1 source (sax/qa/cppunit/xmlimport),
# use_api offapi udkapi, use_ure, use_vcl, use_libraries test cppu comphelper
# sal cppuhelper sax unotest utl salhelper -- "test" is again test_fixtures
# here, same reason as sax_parser above (the .mk's own "$(if $(filter
# -fsanitize=vptr,$(gb_CXX)),tl)" conditional is a sanitizer-only extra
# library this port does not model, matching how sanitizer-conditional
# gbuild flags are skipped without comment elsewhere in this port).
# set_include adds sax/inc on top of the udkapi/offapi dirs use_api already
# gives (its own "$$(INCLUDE)" just means "keep whatever INCLUDE already
# has", already the effect of listing both here).
#
# use_components here is sax_xmlimport's OWN list, read directly from
# CppunitTest_sax_xmlimport.mk -- it is NOT sax_parser's list plus uui, it
# genuinely differs (no framework/i18npool/sfx2; includes sax's own expwrap
# component instead): configmgr/source/configmgr, sax/source/expatwrap/
# expwrap, ucb/source/core/ucb1, ucb/source/ucp/file/ucpfile1, uui/util/uui.
# Lookup keys in this port: "configmgr", "sax" (sax/CMakeLists.txt's own
# librestudio_add_component(sax ...) call, for its expwrap.component --
# confirmed by reading that call directly), "ucb1", "ucpfile1", "uui" (all
# verified against each owning module's own librestudio_add_component(<name>
# ...) call).
#
# use_configuration -> CONFIGURATION, same as sax_parser above.
librestudio_add_cppunit_test(sax_xmlimport
    SOURCES
        sax/qa/cppunit/xmlimport
    LIBRARIES test_fixtures cppu comphelper sal cppuhelper sax unotest utl salhelper
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sax/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr sax ucb1 ucpfile1 uui
    CONFIGURATION)
