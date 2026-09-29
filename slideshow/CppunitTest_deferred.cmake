# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# slideshow_engine (CppunitTest_slideshow_engine.mk) -- split out of
# slideshow/CMakeLists.txt into its own file, include()'d from the root
# CMakeLists.txt only after program_services_rdb exists (this test calls
# gb_CppunitTest_use_rdb(slideshow_engine,services); see scaddins/
# CppunitTest_deferred.cmake's own header comment for the full
# use_rdb(services)/ini-fallback reasoning, which applies identically here --
# no COMPONENTS, a DEPENDS on program_services_rdb, cppunittester's UNO
# bootstrap falls back to unorc's own "UNO_SERVICES=${ORIGIN}/services.rdb"
# ini default). slideshow itself is add_subdirectory()'d at line ~1088,
# already after unotest/test (~1017-1018) -- so subsequenttest/test_fixtures/
# unotest and the three protector plug-ins are all available at slideshow's
# own natural position -- but program_services_rdb is only defined much
# later, right after librestudio_finalize_components(services), so this one
# test alone forces the deferral.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not slideshow's, anything slideshow/CMakeLists.txt already built as
# a real CMake target (the "slideshow" production library, its component,
# etc.) is still visible here -- nothing directory-scoped needs to be
# re-derived.
#
# CppunitTest_slideshow_engine.mk, re-checked directly against the live
# file: 1 source (slideshow/qa/engine/engine), use_externals
# (boost_headers, libxml2), use_libraries (comphelper cppu slideshow
# cppuhelper sal subsequenttest test unotest utl tl -- "test" is
# test_fixtures here, this port's established rename), use_sdk_api (normal
# udkapi+offapi headers), set_include (-I$(SRCDIR)/slideshow/source/inc,
# -I$(SRCDIR)/slideshow/source/engine/animationnodes, $$(INCLUDE)), use_ure +
# use_vcl -> 3 protector pairs (same precedent as sax_parser/
# hwpfilter_test_hwpfilter/svx_unit), use_rdb(services,services) -> see
# above, use_custom_headers(officecfg/registry) -> OFFICECFG_INCLUDE_DIR +
# DEPENDS officecfg_headers (same mechanism slideshow/CMakeLists.txt's own
# production library already uses), use_configuration -> CONFIGURATION.
#
# Unlike CppunitTest_slideshow.mk (slideshow/CMakeLists.txt's own
# "slideshow_test", see that file's header comment on the naming wrinkle),
# this test's own gbuild name "slideshow_engine" does not collide with any
# existing CMake target, so no rename is needed here -- the CMake/CTest
# target below really is named "slideshow_engine".
librestudio_add_cppunit_test(slideshow_engine
    SOURCES
        slideshow/qa/engine/engine
    LIBRARIES
        comphelper cppu slideshow cppuhelper sal subsequenttest test_fixtures
        unotest utl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/slideshow/source/inc"
        "${CMAKE_SOURCE_DIR}/slideshow/source/engine/animationnodes"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${LIBXML2_INCLUDE_DIRS}"
    DEPENDS
        officecfg_headers
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)
