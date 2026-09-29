# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# writerperfect_calc, writerperfect_dialogs_test, writerperfect_draw,
# writerperfect_epubexport, writerperfect_import, writerperfect_impress,
# writerperfect_wpftimport and writerperfect_writer -- the 8 of writerperfect's
# 9 CppunitTest_*.mk files that call use_rdb(services) (writerperfect_stream,
# the 9th, needs no such thing and is wired directly in writerperfect/
# CMakeLists.txt instead -- see that file's own comment). All 8 need
# program_services_rdb, defined near the end of the root CMakeLists.txt,
# well after writerperfect's own add_subdirectory() position -- same
# later-binding constraint documented in e.g. svgio/CppunitTest_deferred.
# cmake's own header comment.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not writerperfect's, the PkgConfig::* IMPORTED_TARGETs writerperfect/
# CMakeLists.txt creates via pkg_check_modules() are NOT visible here (that
# port-wide convention -- one pkg_check_modules() call per external per
# directory scope -- is documented in e.g. comphelper/CppunitTest_deferred.
# cmake's own header comment for PkgConfig::NSS). writerperfect_epubexport
# and writerperfect_wpftimport each re-create the specific PkgConfig::*
# targets they need, guarded the same "if (NOT TARGET ...)" way every other
# such re-creation in this port already is.

# --- writerperfect_calc -------------------------------------------------------
#
# CppunitTest_writerperfect_calc.mk, re-checked directly: 1 source
# (writerperfect/qa/unit/WpftCalcFilterTest), use_external(boost_headers),
# use_sdk_api, use_libraries (comphelper cppu cppuhelper sal test tl
# ucbhelper unotest wpftqahelper -- "test" -> test_fixtures), use_ure +
# use_vcl (plain, headless) -> 3 protector pairs, use_rdb(services) ->
# DEPENDS program_services_rdb (no COMPONENTS -- bare use_rdb(services),
# same reasoning every other such test in this port already documents),
# use_configuration -> CONFIGURATION.
#
# NOT reproduced: the .mk's own trailing order-only dependency on
# `$(call gb_Library_get_target,ipb)` -- re-checked directly, no
# Library_ipb.mk exists anywhere in this tree (added by upstream commit
# e6401bb6a163 "Missing unit test dependency", 2014, with no further
# explanation in the commit message; `git log --all` finds no historical
# Library_ipb.mk under any name in this repository's full history either).
# This is a real, unresolved absence, not a fabricated one -- left
# unreproduced rather than invented, same honesty this port applies to
# other unresolved upstream oddities.
librestudio_add_cppunit_test(writerperfect_calc
    SOURCES
        writerperfect/qa/unit/WpftCalcFilterTest
    LIBRARIES
        comphelper cppu cppuhelper sal test_fixtures tl ucbhelper unotest
        wpftqahelper
        Boost::boost
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

# --- writerperfect_dialogs_test (screenshot test) -----------------------------
#
# CppunitTest_writerperfect_dialogs_test.mk, re-checked directly: 1 source
# (writerperfect/qa/unit/writerperfect-dialogs-test), use_sdk_api (called
# twice, no-op the second time), use_libraries (basegfx comphelper cppu
# cppuhelper drawinglayer editeng i18nlangtag i18nutil msfilter oox sal
# salhelper sax sfx sot svl svt test tl tk ucbhelper unotest utl vcl xo --
# "test" -> test_fixtures), use_external(boost_headers), use_ure +
# use_vcl_non_headless_with_windows -> 3 protector pairs + NONHEADLESS (same
# *_dialogs_test mechanism as vcl_dialogs_test/svtools_dialogs_test/svx_
# dialogs_test/basctl_dialogs_test/reportdesign_dialogs_test/cui_dialogs_test/
# filter_dialogs_test/desktop_dialogs_test/starmath_dialogs_test/sfx2_
# dialogs_test), use_rdb(services) -> DEPENDS program_services_rdb (no
# COMPONENTS), use_configuration -> CONFIGURATION, use_uiconfigs(
# writerperfect) -> UICONFIG writerperfect (writerperfect/CMakeLists.txt's
# own new "writerperfect_uiconfig" ALL target, ported alongside this wiring).
librestudio_add_cppunit_test(writerperfect_dialogs_test
    SOURCES
        writerperfect/qa/unit/writerperfect-dialogs-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
        i18nutil msfilter oox sal salhelper sax sfx sot svl svt test_fixtures
        tl tk ucbhelper unotest utl vcl xo
        Boost::boost
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
    UICONFIG writerperfect
    CONFIGURATION)

# --- writerperfect_draw --------------------------------------------------------
#
# CppunitTest_writerperfect_draw.mk, re-checked directly: 1 source
# (writerperfect/qa/unit/WpftDrawFilterTest), use_external(boost_headers),
# use_sdk_api, use_libraries (comphelper cppu cppuhelper sal test tl
# ucbhelper unotest wpftqahelper -- "test" -> test_fixtures), use_ure +
# use_vcl (plain, headless) -> same 3 protector pairs, use_rdb(services) ->
# DEPENDS program_services_rdb (no COMPONENTS), use_configuration ->
# CONFIGURATION. Same shape as writerperfect_calc above, minus the "ipb" gap
# (re-checked: no such trailing dependency line in this .mk).
librestudio_add_cppunit_test(writerperfect_draw
    SOURCES
        writerperfect/qa/unit/WpftDrawFilterTest
    LIBRARIES
        comphelper cppu cppuhelper sal test_fixtures tl ucbhelper unotest
        wpftqahelper
        Boost::boost
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

# --- writerperfect_epubexport --------------------------------------------------
#
# CppunitTest_writerperfect_epubexport.mk, re-checked directly: 1 source
# (writerperfect/qa/unit/EPUBExportTest), use_libraries (comphelper cppu
# cppuhelper sal subsequenttest test tl unotest utl wpftwriter, plus
# "$(gb_UWINAPI)" -- a WNT-only extra libs macro, empty on this Linux port,
# same unmodelled-without-comment treatment every other WNT-only gbuild
# macro already gets in this port -- "test" -> test_fixtures),
# use_externals("$(if $(ENABLE_WASM_STRIP_EPUB),,epubgen)" libxml2 revenge --
# ENABLE_WASM_STRIP_EPUB is one of the ENABLE_WASM_STRIP_* gates writerperfect/
# CMakeLists.txt's own header comment already documents as unset/0 in this
# port, so epubgen applies unconditionally here, same resolution), use_
# external(boost_headers), use_sdk_api (twice, no-op second time), use_ure +
# use_vcl (plain, headless) -> same 3 protector pairs, use_rdb(services) ->
# DEPENDS program_services_rdb (no COMPONENTS), use_configuration ->
# CONFIGURATION.
if (NOT TARGET PkgConfig::EPUBGEN)
    pkg_check_modules(EPUBGEN REQUIRED IMPORTED_TARGET libepubgen-0.1)
endif ()
if (NOT TARGET PkgConfig::REVENGE)
    pkg_check_modules(REVENGE REQUIRED IMPORTED_TARGET librevenge-0.0)
endif ()

librestudio_add_cppunit_test(writerperfect_epubexport
    SOURCES
        writerperfect/qa/unit/EPUBExportTest
    LIBRARIES
        comphelper cppu cppuhelper sal subsequenttest test_fixtures tl
        unotest utl wpftwriter
        Boost::boost LibXml2::LibXml2 PkgConfig::EPUBGEN PkgConfig::REVENGE
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

# --- writerperfect_import -------------------------------------------------------
#
# CppunitTest_writerperfect_import.mk, re-checked directly: 1 source
# (writerperfect/qa/unit/ImportTest), add_defs -DTEST_DIR=\"writerperfect/qa/
# unit/data/import\" -> DEFINITIONS, use_external(boost_headers), use_sdk_api,
# use_libraries (comphelper cppu sal test tl unotest utl wpftqahelper --
# "test" -> test_fixtures), use_ure + use_vcl (plain, headless) -> same 3
# protector pairs, use_rdb(services) -> DEPENDS program_services_rdb (no
# COMPONENTS), use_configuration -> CONFIGURATION.
librestudio_add_cppunit_test(writerperfect_import
    SOURCES
        writerperfect/qa/unit/ImportTest
    LIBRARIES
        comphelper cppu sal test_fixtures tl unotest utl wpftqahelper
        Boost::boost
    DEFINITIONS
        "TEST_DIR=\"writerperfect/qa/unit/data/import\""
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

# --- writerperfect_impress ------------------------------------------------------
#
# CppunitTest_writerperfect_impress.mk, re-checked directly: 1 source
# (writerperfect/qa/unit/WpftImpressFilterTest), use_external(boost_headers),
# use_sdk_api, use_libraries (comphelper cppu cppuhelper sal test tl
# ucbhelper unotest wpftqahelper -- "test" -> test_fixtures), use_ure +
# use_vcl (plain, headless) -> same 3 protector pairs, use_rdb(services) ->
# DEPENDS program_services_rdb (no COMPONENTS), use_configuration ->
# CONFIGURATION. Same shape as writerperfect_draw above.
librestudio_add_cppunit_test(writerperfect_impress
    SOURCES
        writerperfect/qa/unit/WpftImpressFilterTest
    LIBRARIES
        comphelper cppu cppuhelper sal test_fixtures tl ucbhelper unotest
        wpftqahelper
        Boost::boost
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

# --- writerperfect_wpftimport ---------------------------------------------------
#
# CppunitTest_writerperfect_wpftimport.mk, re-checked directly: 5 sources
# (writerperfect/qa/unit/DrawingImportTest, PresentationImportTest,
# SpreadsheetImportTest, TextImportTest, wpftimport), set_include (-I
# writerperfect/inc $$(INCLUDE)), use_externals(odfgen revenge),
# use_external(boost_headers), use_sdk_api (twice, no-op second time),
# use_libraries (comphelper cppu cppuhelper sal test unotest utl vcl
# wpftqahelper writerperfect xo -- "test" -> test_fixtures), use_ure +
# use_vcl (plain, headless) -> same 3 protector pairs, use_rdbs(services) --
# gb_CppunitTest_use_rdbs's plural form, a list of Rdb names, "services"
# being the only element here -- same effect as every other bare use_rdb(
# services) call in this port, -> DEPENDS program_services_rdb (no
# COMPONENTS), use_configuration -> CONFIGURATION.
if (NOT TARGET PkgConfig::ODFGEN)
    pkg_check_modules(ODFGEN REQUIRED IMPORTED_TARGET libodfgen-0.1)
endif ()
if (NOT TARGET PkgConfig::REVENGE)
    pkg_check_modules(REVENGE REQUIRED IMPORTED_TARGET librevenge-0.0)
endif ()

librestudio_add_cppunit_test(writerperfect_wpftimport
    SOURCES
        writerperfect/qa/unit/DrawingImportTest
        writerperfect/qa/unit/PresentationImportTest
        writerperfect/qa/unit/SpreadsheetImportTest
        writerperfect/qa/unit/TextImportTest
        writerperfect/qa/unit/wpftimport
    LIBRARIES
        comphelper cppu cppuhelper sal test_fixtures unotest utl vcl
        wpftqahelper writerperfect xo
        Boost::boost PkgConfig::ODFGEN PkgConfig::REVENGE
    INCLUDE
        "${CMAKE_SOURCE_DIR}/writerperfect/inc"
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

# --- writerperfect_writer -------------------------------------------------------
#
# CppunitTest_writerperfect_writer.mk, re-checked directly: 1 source
# (writerperfect/qa/unit/WpftWriterFilterTest), use_external(boost_headers),
# use_sdk_api, use_libraries (comphelper cppu cppuhelper sal test tl
# ucbhelper unotest wpftqahelper -- "test" -> test_fixtures), use_ure +
# use_vcl (plain, headless) -> same 3 protector pairs, use_rdb(services) ->
# DEPENDS program_services_rdb (no COMPONENTS), use_configuration ->
# CONFIGURATION, use_uiconfigs(modules/swriter) -> UICONFIG swriter (this
# port's own bare-name convention for sw/CMakeLists.txt's "swriter_uiconfig"
# target, same convention writerperfect_dialogs_test's own UICONFIG
# writerperfect and desktop_lib's own UICONFIG cui swriter already follow).
librestudio_add_cppunit_test(writerperfect_writer
    SOURCES
        writerperfect/qa/unit/WpftWriterFilterTest
    LIBRARIES
        comphelper cppu cppuhelper sal test_fixtures tl ucbhelper unotest
        wpftqahelper
        Boost::boost
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
    UICONFIG swriter
    CONFIGURATION)

# writerperfect/CppunitTest_deferred.cmake needs program_services_rdb (only
# defined near the end of the root CMakeLists.txt, well after writerperfect's
# own add_subdirectory() position) -- see writerperfect/CMakeLists.txt's own
# cmake_language(DEFER) call site comment for the scheduling mechanism, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required there.
