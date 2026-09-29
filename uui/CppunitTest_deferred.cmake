# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# uui_dialogs_test (CppunitTest_uui_dialogs_test.mk, a gb_CppunitTest_
# CppunitScreenShot screenshot target, gb_Module_add_screenshot_targets entry
# in Module_uui.mk, unconditional -- no OS/feature gate) -- split out of uui/
# CMakeLists.txt into its own file, include()'d from the root CMakeLists.txt
# only after program_services_rdb exists (its gb_CppunitTest_use_rdb(services)
# call needs that target, which does not exist at uui/CMakeLists.txt's own
# add_subdirectory() position, root CMakeLists.txt line 1030 -- program_
# services_rdb is only defined at line 1214, well after). See svtools/
# CppunitTest_deferred.cmake's own "svtools_dialogs_test: RESOLVED" section
# (or vcl/CppunitTest_deferred.cmake's vcl_dialogs_test, svx/CppunitTest_
# deferred.cmake's svx_dialogs_test, reportdesign/CppunitTest_deferred.cmake's
# reportdesign_dialogs_test) for the full NONHEADLESS/UICONFIG mechanism this
# follows identically -- cmake/LibreStudioTest.cmake's own header comment has
# the authoritative full explanation.
#
# CppunitTest_uui_dialogs_test.mk, re-checked directly against the live file:
# 1 source (uui/qa/unit/uui-dialogs-test), use_sdk_api (normal udkapi+offapi,
# called twice -- a no-op the second time, the same idiom several other tests
# in this port already tolerate), NO set_include call anywhere in the file
# (confirmed), use_libraries basegfx comphelper cppu cppuhelper drawinglayer
# editeng i18nlangtag i18nutil msfilter oox sal salhelper sax sfx sot svl svt
# test tl ucbhelper unotest utl vcl xo (22 names -- NO "tk", unlike fpicker_
# dialogs_test's own 23-name list; re-checked, uui_dialogs_test really does
# not link toolkit directly -- "test" -> test_fixtures, this port's
# established rename, all 22 re-verified as real add_library() targets),
# use_external(boost_headers), use_ure + use_vcl_non_headless_with_windows ->
# 3 protector pairs (same triple every other use_ure+use_vcl(_non_headless)
# test in this port uses) plus NONHEADLESS, use_rdb(services) -> DEPENDS
# program_services_rdb (no COMPONENTS -- cppunittester's UNO bootstrap falls
# back to unorc's own ini default for UNO_SERVICES, same as every other
# use_rdb(services)-only test in this port), use_configuration ->
# CONFIGURATION, use_uiconfigs(uui) -> UICONFIG uui (uui/CMakeLists.txt's own
# "uui_uiconfig" ALL target, ported alongside this file this session).
#
# Notably absent from use_libraries: "uui" itself -- same "drives .ui files
# through VCL's dialog-builder machinery, not the production library" shape
# already established by basctl_dialogs_test/reportdesign_dialogs_test (both
# in their own CppunitTest_deferred.cmake files).
librestudio_add_cppunit_test(uui_dialogs_test
    SOURCES
        uui/qa/unit/uui-dialogs-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
        i18nutil msfilter oox sal salhelper sax sfx sot svl svt test_fixtures
        tl ucbhelper unotest utl vcl xo Boost::boost
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    NONHEADLESS
    UICONFIG uui
    CONFIGURATION)
