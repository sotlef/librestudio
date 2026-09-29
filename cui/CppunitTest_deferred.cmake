# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# cui_dialogs_test / cui_dialogs_test2 / cui_dialogs_test3 / cui_dialogs_test4
# (CppunitTest_cui_dialogs_test.mk, _test_2.mk, _test_3.mk, _test_4.mk --
# gb_CppunitTest_CppunitScreenShot screenshot targets; note the gbuild target
# names themselves are "cui_dialogs_test2/3/4", no underscore before the
# digit, even though the .mk file names do have one -- verified directly
# against each file's own gb_CppunitTest_CppunitScreenShot(...) call) --
# split out of cui/CMakeLists.txt into their own file, include()'d from the
# root CMakeLists.txt only after program_services_rdb exists; see that
# include() call site for the ordering rationale, and svx/CppunitTest_
# deferred.cmake's own "svx_dialogs_test: RESOLVED" section (or basctl/,
# reportdesign/CppunitTest_deferred.cmake's own single dialogs_test each) for
# the full NONHEADLESS/UICONFIG mechanism this follows identically -- this is
# the same category of test (vcl_dialogs_test, svtools_dialogs_test,
# svx_dialogs_test, basctl_dialogs_test, reportdesign_dialogs_test, now these
# four) that motivated both options in cmake/LibreStudioTest.cmake.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), source/include paths below are still fine -- librestudio_expand_cxx
# resolves them against CMAKE_SOURCE_DIR, never CMAKE_CURRENT_SOURCE_DIR (same
# note as reportdesign/forms/CppunitTest_deferred.cmake's own), and
# LIBRESTUDIO_REGISTRY_DIR (cmake/LibreStudioPlatform.cmake, include()'d into
# this same root scope near the top of CMakeLists.txt) is likewise visible
# here without needing to be re-derived.
#
# CppunitTest_cui_dialogs_test.mk, re-checked directly against the live file:
# 1 source (cui/qa/unit/cui-dialogs-test), use_sdk_api (normal udkapi+offapi,
# called twice -- a no-op the second time), set_include -I$(SRCDIR)/cui/
# source/inc -I$(SRCDIR)/cui/inc plus $$(INCLUDE) (a no-op re-inclusion, same
# idiom every other "*_dialogs_test" set_include call already uses),
# use_libraries basegfx comphelper cppu cppuhelper drawinglayer editeng
# i18nlangtag i18nutil msfilter oox sal salhelper sax sfx sot svl svt cui
# svxcore test tl tk ucbhelper unotest utl vcl xo (26 names -- the same
# 23-name set vcl_dialogs_test/svtools_dialogs_test/basctl_dialogs_test/
# reportdesign_dialogs_test all share, PLUS "cui" and "svxcore" -- "cui" is
# this module's own production library, unlike basctl_dialogs_test/
# reportdesign_dialogs_test, which link none of their own; "svxcore" is only
# linked by this first test of the four, re-verified directly -- test_2/3/4
# below do NOT list it), use_external(boost_headers), use_ure + use_vcl_non_
# headless_with_windows -> 3 protector pairs + NONHEADLESS, use_rdb(services)
# -> DEPENDS program_services_rdb (no COMPONENTS, same ini-fallback reasoning
# as every other use_rdb(services) test in this port), use_configuration ->
# CONFIGURATION (the full three-layer form -- unlike test_2/3/4 below), use_
# uiconfigs(cui) -> UICONFIG cui (cui/CMakeLists.txt's own new "cui_uiconfig"
# ALL target, added alongside this wiring).
librestudio_add_cppunit_test(cui_dialogs_test
    SOURCES
        cui/qa/unit/cui-dialogs-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
        i18nutil msfilter oox sal salhelper sax sfx sot svl svt cui svxcore
        test_fixtures tl tk ucbhelper unotest utl vcl xo Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/cui/source/inc"
        "${CMAKE_SOURCE_DIR}/cui/inc"
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
    UICONFIG cui
    CONFIGURATION)

# --- cui_dialogs_test2/3/4: use_instdir_configuration, NOT full use_
# configuration -------------------------------------------------------------
#
# CppunitTest_cui_dialogs_test_2.mk/_3.mk/_4.mk, re-checked directly against
# all three live files: identical to each other but for their one source file
# (cui/qa/unit/cui-dialogs-test_2, _3, _4) and target name suffix. Same
# set_include/use_sdk_api/use_external/use_ure+use_vcl_non_headless_with_
# windows/use_rdb(services) shape as cui_dialogs_test above, use_libraries
# basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
# i18nutil msfilter oox sal salhelper sax sfx sot svl svt cui test tl tk
# ucbhelper unotest utl vcl xo (25 names -- cui_dialogs_test's own 26 minus
# svxcore), use_uiconfigs(cui) -> UICONFIG cui.
#
# The one real difference: each calls gb_CppunitTest_use_instdir_
# configuration, NOT gb_CppunitTest_use_configuration (both lines are present
# in the .mk, but the use_configuration line is commented out with a leading
# "#" -- re-verified character-for-character against the live file, not
# assumed). Per solenv/gbuild/CppunitTest.mk:377-408: use_instdir_
# configuration adds only ONE "xcsxcu:<instdir-registry-url>" CONFIGURATION_
# LAYERS entry (plus an order-dep on the packaged instdir registry), while
# use_configuration expands to instdir_configuration + common_configuration +
# user_ui_configuration, i.e. three entries. These are genuinely different
# requests, not a stricter/looser rewording of the same thing.
#
# This is the first time this port has hit a plain use_instdir_configuration
# call on its own (grepped every prior *_deferred.cmake). Rather than
# over-approximate with the full CONFIGURATION option (which would silently
# add two CONFIGURATION_LAYERS entries -- registry-common, registry-user-ui
# -- these three .mk files never asked for), cmake/LibreStudioTest.cmake now
# has a dedicated CONFIGURATION_INSTDIR_ONLY option for exactly this shape
# (added alongside this wiring) -- see that file's own header comment for the
# full mechanism, including why the existing *_a11y tests' different 2-of-3
# combination is intentionally left as its own separate, already-accepted
# over-approximation rather than folded into this new option too.
#
# One accepted, pre-existing gap this does NOT attempt to work around (not
# specific to cui): librestudio_add_cppunit_test()'s "<name>.user" FIXTURES_
# SETUP step only does the full "rm -fr && cp -r test/user-template" dance
# when CONFIGURATION (the full option) is passed; CONFIGURATION_INSTDIR_ONLY
# does not upgrade it either (upstream's own use_instdir_configuration alone
# never touches test_unittest/WORKDIR/unittest), so these three tests get the
# plain empty-directory make_directory step every other non-CONFIGURATION
# use_ure test in this port already gets -- an accepted simplification, not
# unique to this file.
foreach (_cui_dt IN ITEMS 2 3 4)
    librestudio_add_cppunit_test(cui_dialogs_test${_cui_dt}
        SOURCES
            cui/qa/unit/cui-dialogs-test_${_cui_dt}
        LIBRARIES
            basegfx comphelper cppu cppuhelper drawinglayer editeng
            i18nlangtag i18nutil msfilter oox sal salhelper sax sfx sot svl
            svt cui test_fixtures tl tk ucbhelper unotest utl vcl xo
            Boost::boost
        INCLUDE
            "${CMAKE_SOURCE_DIR}/cui/source/inc"
            "${CMAKE_SOURCE_DIR}/cui/inc"
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
        UICONFIG cui
        CONFIGURATION_INSTDIR_ONLY)
endforeach ()
unset(_cui_dt)
