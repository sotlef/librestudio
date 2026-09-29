# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# test_a11y (CppunitTest_test_a11y.mk) -- split out of test/CMakeLists.txt
# into its own file, include()'d from the root CMakeLists.txt only after
# program_services_rdb exists (this test calls gb_CppunitTest_use_rdb(
# test_a11y,services); see scaddins/CppunitTest_deferred.cmake's own header
# comment for the full use_rdb(services)/ini-fallback reasoning, which
# applies identically here -- no COMPONENTS, a DEPENDS on
# program_services_rdb, cppunittester's UNO bootstrap falls back to unorc's
# own "UNO_SERVICES=${ORIGIN}/services.rdb" ini default).
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not test's, anything test/CMakeLists.txt already built as a real
# CMake target (test_fixtures, subsequenttest, the protector plug-ins, etc.)
# is still visible here -- nothing directory-scoped needs to be re-derived.
#
# CppunitTest_test_a11y.mk, re-checked directly against the live file: 1
# source (test/qa/cppunit/dialog), use_libraries sal comphelper cppu
# cppuhelper subsequenttest test ("test" -> test_fixtures, this module's own
# established rename), use_sdk_api (normal udkapi+offapi headers),
# use_rdb(services,services) -> see above, use_ure + use_vcl -> 3 protector
# pairs (same precedent as sax_parser/hwpfilter_test_hwpfilter).
#
# use_instdir_configuration + use_common_configuration (NOT
# use_user_ui_configuration -- re-checked directly, the third sub-call is
# genuinely absent from this .mk): a 2-of-3 CONFIGURATION_LAYERS combination
# librestudio_add_cppunit_test() has no dedicated parameter for (only the
# 3-of-3 CONFIGURATION and the 1-of-3 CONFIGURATION_INSTDIR_ONLY). Wired with
# the bare CONFIGURATION option instead, the exact same accepted
# over-approximation cmake/LibreStudioTest.cmake's own header comment already
# documents for vcl_a11y (vcl/CppunitTest_deferred.cmake) -- another *_a11y
# test hitting this identical 2-of-3 shape: it adds one harmless extra
# registry-user-ui CONFIGURATION_LAYERS entry this .mk itself never asked
# for, not a functional deviation for what test/qa/cppunit/dialog.cxx
# actually exercises.
#
# use_packages(postprocess_images) -- 2026-09-28: postprocess_images is now
# built (postprocess/CMakeLists.txt's own "CustomTarget_images + Package_
# images: implemented" section), so PACKAGES below includes it for real.
librestudio_add_cppunit_test(test_a11y
    SOURCES
        test/qa/cppunit/dialog
    LIBRARIES
        sal comphelper cppu cppuhelper subsequenttest test_fixtures
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
    PACKAGES
        postprocess_images
    CONFIGURATION)
