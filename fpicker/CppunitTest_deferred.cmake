# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# fpicker_dialogs_test (CppunitTest_fpicker_dialogs_test.mk, a gb_CppunitTest_
# CppunitScreenShot screenshot target, gb_Module_add_screenshot_targets entry
# in Module_fpicker.mk, unconditional -- no OS/feature gate) -- split out of
# fpicker/CMakeLists.txt into its own file, include()'d from the root
# CMakeLists.txt only after program_services_rdb exists, same reason and
# call-site pattern as uui/CppunitTest_deferred.cmake's own uui_dialogs_test
# (read that file's header comment first for the full program_services_rdb/
# NONHEADLESS/UICONFIG reasoning, identical here).
#
# CppunitTest_fpicker_dialogs_test.mk, re-checked directly against the live
# file: 1 source (fpicker/qa/unit/fpicker-dialogs-test), use_sdk_api (normal
# udkapi+offapi, called twice -- a no-op the second time), NO set_include call
# anywhere in the file (confirmed), use_libraries basegfx comphelper cppu
# cppuhelper drawinglayer editeng i18nlangtag i18nutil msfilter oox sal
# salhelper sax sfx sot svl svt test tk tl ucbhelper unotest utl vcl xo (23
# names -- includes "tk" (toolkit), unlike uui_dialogs_test's own 22-name
# list without it; re-checked directly -- "test" -> test_fixtures, all 23
# re-verified as real add_library() targets), use_external(boost_headers),
# use_ure + use_vcl_non_headless_with_windows -> 3 protector pairs plus
# NONHEADLESS, use_rdb(services) -> DEPENDS program_services_rdb (no
# COMPONENTS, same ini-fallback reasoning as every other use_rdb(services)-
# only test in this port), use_configuration -> CONFIGURATION, use_uiconfigs
# (fps) -> UICONFIG fps (fpicker/CMakeLists.txt's own "fps_uiconfig" ALL
# target, ported alongside this file this session -- named "fps", not
# "fpicker", matching the .mk's own gb_CppunitTest_use_uiconfigs(
# fpicker_dialogs_test,fps) call).
#
# Notably absent from use_libraries: "fps_office" itself -- same "drives .ui
# files through VCL's dialog-builder machinery, not the production library"
# shape as uui_dialogs_test/basctl_dialogs_test/reportdesign_dialogs_test.
librestudio_add_cppunit_test(fpicker_dialogs_test
    SOURCES
        fpicker/qa/unit/fpicker-dialogs-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
        i18nutil msfilter oox sal salhelper sax sfx sot svl svt test_fixtures
        tk tl ucbhelper unotest utl vcl xo Boost::boost
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
    UICONFIG fps
    CONFIGURATION)
