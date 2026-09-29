# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# emfio_emf (CppunitTest_emfio_emf.mk) and emfio_wmf (CppunitTest_emfio_wmf.mk)
# -- split out of emfio/CMakeLists.txt into their own file, include()'d from
# the root CMakeLists.txt only after every module/target either needs has
# already run. emfio itself is add_subdirectory()'d at line ~911, well before
# unotest/test (~1017-1018) -- both tests call use_ure + use_vcl, whose
# protector plug-ins (unobootstrapprotector/unoexceptionprotector,
# vclbootstrapprotector) come from unotest/test, so neither test could be
# registered at emfio/CMakeLists.txt's own position either way. Both also call
# gb_CppunitTest_use_rdb(name,services) (see scaddins/CppunitTest_deferred.
# cmake's own header comment for the full "ini fallback" reasoning this port
# gives that call: no COMPONENTS, a DEPENDS on program_services_rdb -- not
# defined until the very end of the root CMakeLists.txt -- and cppunittester's
# UNO bootstrap falling back to unorc's own ini default for UNO_SERVICES), so
# program_services_rdb is the binding constraint for both, same as every other
# use_rdb(services) test already deferred to this same point in this port
# (editeng_editeng/editeng_core just above are the closest precedent -- same
# combination of use_ure+use_vcl, use_rdb(services), use_configuration).
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not emfio's, source paths below are still fine: librestudio_expand_
# cxx (cmake/LibreStudioPlatform.cmake) resolves them against CMAKE_SOURCE_DIR
# (the absolute tree root), never CMAKE_CURRENT_SOURCE_DIR.
#
# use_more_fonts (both .mk files): same treatment already established by
# editeng/CppunitTest_deferred.cmake's own header comment -- gated on
# "MORE_FONTS" being in BUILD_TYPE and a "more_fonts" module existing, neither
# true here (this port's LIBRESTUDIO_BUILD_TYPE, vcl/CMakeLists.txt, defaults
# to just "DESKTOP", and no more_fonts module/Package_extras_font exists
# anywhere in this tree) -- genuinely inert, nothing to wire.

# --- CppunitTest_emfio_emf ---------------------------------------------------
#
# CppunitTest_emfio_emf.mk, re-checked directly against the live file: 1
# source (emfio/qa/cppunit/emf/EmfImportTest), use_externals(boost_headers
# libxml2), use_sdk_api (normal udkapi+offapi), use_libraries (basegfx
# drawinglayer drawinglayercore cppu cppuhelper comphelper sal subsequenttest
# svt test tl unotest vcl utl -- "test" -> test_fixtures, this port's
# established rename; note "emfio" itself is NOT in this list -- the test
# talks to the Graphic.EmfTools UNO service via UNO_SERVICES/program_
# services_rdb, not by linking the production library directly, confirmed by
# EmfImportTest.cxx's own #include <test/unoapi_test.hxx> and
# <com/sun/star/graphic/EmfTools.hpp>), use_ure + use_vcl -> 3 protector
# pairs (unobootstrapprotector x2, unoexceptionprotector x2,
# vclbootstrapprotector x2, same precedent as editeng_editeng/editeng_core),
# use_rdb(services) -> DEPENDS program_services_rdb (no COMPONENTS, see file
# header above), use_more_fonts -> inert (see file header above), use_
# configuration -> CONFIGURATION.
#
# All libraries verified as real targets (grep -n "add_library(<name> "):
# basegfx, drawinglayer, drawinglayercore, cppu, cppuhelper, comphelper, sal,
# subsequenttest (test/CMakeLists.txt), svt (svtools/CMakeLists.txt),
# test_fixtures (test/CMakeLists.txt), tl (tools/CMakeLists.txt), unotest,
# vcl, utl (unotools/CMakeLists.txt) -- all resolve.
librestudio_add_cppunit_test(emfio_emf
    SOURCES
        emfio/qa/cppunit/emf/EmfImportTest
    LIBRARIES
        basegfx drawinglayer drawinglayercore cppu cppuhelper comphelper sal
        subsequenttest svt test_fixtures tl unotest vcl utl
        Boost::boost LibXml2::LibXml2
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
    CONFIGURATION)

# EmfImportTest needs SRC_ROOT/WORKDIR_FOR_BUILD (BootstrapFixtureBase's
# Directories member) -- now handled generically by
# librestudio_add_cppunit_test() itself (cmake/LibreStudioTest.cmake).

# --- CppunitTest_emfio_wmf ---------------------------------------------------
#
# CppunitTest_emfio_wmf.mk, re-checked directly against the live file: 1
# source (emfio/qa/cppunit/wmf/wmfimporttest), set_include(-I$(SRCDIR)/
# emfio/inc, $$(INCLUDE)) -- wmfimporttest.cxx's own #include "mtftools.hxx"
# (unprefixed, header-only helper class) resolves under emfio/inc, confirmed
# by `find emfio -iname mtftools.hxx`. use_externals(boost_headers libxml2),
# use_libraries (emfio sal test tl unotest vcl -- "test" -> test_fixtures;
# unlike emfio_emf this test DOES link the production "emfio" library
# directly), use_more_fonts -> inert (see file header above), use_
# configuration -> CONFIGURATION, use_sdk_api (normal udkapi+offapi), use_ure
# + use_vcl -> 3 protector pairs (same precedent as emfio_emf above), use_rdb
# (services) -> DEPENDS program_services_rdb (no COMPONENTS, see file header
# above).
librestudio_add_cppunit_test(emfio_wmf
    SOURCES
        emfio/qa/cppunit/wmf/wmfimporttest
    LIBRARIES
        emfio sal test_fixtures tl unotest vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/emfio/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# WmfTest needs the same SRC_ROOT/WORKDIR_FOR_BUILD as emfio_emf above --
# same generic fix in librestudio_add_cppunit_test().
