# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# comphelper_test (CppunitTest_comphelper_test.mk) -- split out of
# comphelper/CMakeLists.txt into its own file, include()'d from the root
# CMakeLists.txt well after i18npool's own add_subdirectory(); see that
# include() call site, and the pointer comment left in comphelper/
# CMakeLists.txt in this test's old place, for the full ordering rationale.
# In short: this test's COMPONENTS dependency on i18npool (below) needs
# i18npool/CMakeLists.txt's own librestudio_add_component() call to have
# already run, and i18npool is add_subdirectory()'d AFTER comphelper in this
# port's module order -- reordering comphelper itself would be the much
# larger, riskier change, given how many other modules already depend on it
# being available early. Moving just this one test's registration here,
# executed later, is the smaller, safer fix.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), NOT comphelper's, anything comphelper/CMakeLists.txt set up that is
# directory-scoped rather than global has to be re-derived here rather than
# assumed visible -- see the PkgConfig::NSS handling at the bottom of this
# file for the one place that actually matters.
#
# CppunitTest_comphelper_test.mk: 11 sources, use_ure + use_sdk_api,
# use_libraries comphelper cppuhelper cppu sal unotest. This is the one test
# in this wave that actually needs the protector plug-ins: unobootstrapprotector
# and unoexceptionprotector (unotest/CMakeLists.txt) exist now, so they are
# wired in via PROTECTORS -- <cmake-target> <symbol> pairs, and both
# libraries' exported factory symbol is their own name (unobootstrapprotector()
# / unoexceptionprotector(), both extern "C" SAL_DLLPUBLIC_EXPORT
# CppUnit::Protector*, verified against unotest/source/cpp/uno*protector/
# uno*protector.cxx), hence the target name repeated as its own symbol below.
#
# What use_ure itself pulls in beyond the protectors and UNO_SERVICES
# (solenv/gbuild/CppunitTest.mk's gb_CppunitTest_use_ure: linking the
# "ure/services" rdb, the CPPU_ENV-suffixed uno library, affine_uno, plus
# BRAND_BASE_DIR/UNO_TYPES/CONFIGURATION_LAYERS/URE_BIN_DIR/
# URE_INTERNAL_LIB_DIR/LO_LIB_DIR/LO_JAVA_DIR -env: arguments) is still NOT
# modelled here -- librestudio_add_cppunit_test() has no use_ure-equivalent
# parameter (cmake/LibreStudioTest.cmake's file header comment documents this
# gap). Only PROTECTORS and, now, COMPONENTS (for UNO_SERVICES) are wired
# below; a later wave that needs the rest of use_ure's runtime environment
# has to extend the helper function itself, not just this call site.
#
# use_components i18npool/util/i18npool (gb_CppunitTest_use_components) is
# now wired via COMPONENTS below. The lookup key is "i18npool", NOT the
# gbuild path "i18npool/util/i18npool": i18npool/CMakeLists.txt's own
# librestudio_add_component() call registers its component under the plain
# name "i18npool" (its own component *file* is still i18npool/util/
# i18npool.component, matching gbuild's component identity -- only the
# CMake-side lookup key differs), verified by reading i18npool/CMakeLists.txt
# directly rather than assumed from the .mk's spelling.
librestudio_add_cppunit_test(comphelper_test
    SOURCES
        comphelper/qa/string/test_string
        comphelper/qa/string/NaturalStringSortTest
        comphelper/qa/container/testifcontainer
        comphelper/qa/container/testifcontainer3
        comphelper/qa/unit/test_hash
        comphelper/qa/unit/base64_test
        comphelper/qa/unit/propertyvalue
        comphelper/qa/unit/types_test
        comphelper/qa/unit/test_guards
        comphelper/qa/unit/test_traceevent
        comphelper/qa/unit/CryptoTest
    LIBRARIES comphelper cppuhelper cppu sal unotest
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
    COMPONENTS
        i18npool)

# ifeq ($(TLS),NSS): plc4 + nss3 collapse to the single "nss" pkg-config
# module, the same simplification comphelper's own crypto backend (and oox's,
# xmlsecurity's, svl's) already uses. comphelper/CMakeLists.txt resolves
# PkgConfig::NSS the same way already, but that pkg_check_modules(...
# IMPORTED_TARGET ...) call's target is only visible in the directory it was
# created in and below (see librestudio_add_cppunit_test()'s own comment on
# this exact point for PkgConfig::CPPUNIT) -- this file runs in the root
# CMakeLists.txt's scope, comphelper's *parent*, not a descendant of it, so
# comphelper's copy is not visible here. Guarded and re-run here instead of
# assumed, the same pattern librestudio_add_cppunit_test() already uses.
if (LIBRESTUDIO_TLS_BACKEND STREQUAL "NSS")
    if (NOT TARGET PkgConfig::NSS)
        pkg_check_modules(NSS REQUIRED IMPORTED_TARGET nss)
    endif ()
    target_link_libraries(comphelper_test PRIVATE PkgConfig::NSS)
endif ()
