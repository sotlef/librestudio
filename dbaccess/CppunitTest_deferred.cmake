# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# dbaccess_dialogs_test, dbaccess_dialog_save, dbaccess_empty_stdlib_save,
# dbaccess_nolib_save and dbaccess_hsqlschema_import -- 5 of dbaccess's 11
# CppunitTest_*.mk files this pass wires (see dbaccess/CMakeLists.txt's own
# "--- CppunitTests ---" section for exactly why the other 6 are not: Java-
# UNO-bridge/firebird_sdbc/JARS-mechanism gaps, none forced through).
# dbaccess_dialogs_test is deferred for use_rdb(services) -> program_
# services_rdb; the other four need only use_configuration -> CONFIGURATION
# (test/CMakeLists.txt's own LIBRESTUDIO_TEST_UNITTEST_DIR, well after
# dbaccess's own add_subdirectory() position) -- none of the four calls
# use_rdb at all (re-verified directly). The four were, until 2026-09-28,
# blocked on "unoxml/source/rdf/unordf" (needs raptor2/rasqal/redland, none
# vendored) -- now real, real build-time targets (see unoxml/CMakeLists.txt's
# own "--- Library_unordf ---" section and root CMakeLists.txt's own
# "--- raptor2/rasqal/redland ---" sections), so all four are wired for real
# here.

# --- dbaccess_dialogs_test (screenshot test) -------------------------------
#
# CppunitTest_dbaccess_dialogs_test.mk, re-checked directly: 1 source
# (dbaccess/qa/unit/dbaccess-dialogs-test), use_sdk_api (twice, no-op second
# time), set_include (-Idbaccess/source/inc -Idbaccess/inc $$(INCLUDE)),
# use_libraries (basegfx comphelper cppu cppuhelper drawinglayer editeng
# i18nlangtag i18nutil msfilter oox sal salhelper sax sfx sot svl svt test tl
# tk ucbhelper unotest utl vcl xo -- "test" -> test_fixtures), use_external(
# boost_headers), use_ure + use_vcl_non_headless_with_windows -> 3 protector
# pairs + NONHEADLESS, use_rdb(services) -> DEPENDS program_services_rdb (no
# COMPONENTS), use_configuration -> CONFIGURATION, use_uiconfigs(dbaccess)
# -> UICONFIG dbaccess (dbaccess/CMakeLists.txt's own new "dbaccess_
# uiconfig" ALL target, ported alongside this wiring).
librestudio_add_cppunit_test(dbaccess_dialogs_test
    SOURCES
        dbaccess/qa/unit/dbaccess-dialogs-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
        i18nutil msfilter oox sal salhelper sax sfx sot svl svt test_fixtures
        tl tk ucbhelper unotest utl vcl xo
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/dbaccess/source/inc"
        "${CMAKE_SOURCE_DIR}/dbaccess/inc"
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
    UICONFIG dbaccess
    CONFIGURATION)

# --- dbaccess_dialog_save ---------------------------------------------------
#
# CppunitTest_dbaccess_dialog_save.mk, re-checked directly: 1 source
# (dbaccess/qa/extras/dialog-save), use_externals(boost_headers libxml2),
# use_libraries (basegfx comphelper cppu cppuhelper dba dbu sdbt
# drawinglayer editeng for forui i18nlangtag msfilter oox sal salhelper sax
# sb sfx sot svl svt svx svxcore test subsequenttest tl tk ucbhelper unotest
# utl vbahelper(optional SCRIPTING -- applies, ENABLE_SCRIPTING defaults ON
# in this port) vcl xo -- "test" -> test_fixtures), use_api (offapi oovbaapi
# udkapi, all normal), use_ure + use_vcl (plain, headless) -> 3 protector
# pairs, use_components (basic/util/sb, comphelper/util/comphelp ->
# "comphelper", configmgr/source/configmgr, dbaccess/util/dba,
# dbaccess/util/dbu, dbaccess/util/sdbt, dbaccess/source/filter/xml/dbaxml,
# filter/source/config/cache/filterconfig1 -> "filterconfig", forms/util/
# frm, framework/util/fwk, i18npool/util/i18npool, linguistic/source/lng,
# package/source/xstor/xstor, package/util/package2, sax/source/expatwrap/
# expwrap -> "sax", scripting/source/basprov/basprov, scripting/util/
# scriptframe, sfx2/util/sfx, sot/util/sot, svl/source/fsstor/fsstorage,
# svl/util/svl, toolkit/util/tk, ucb/source/core/ucb1, ucb/source/ucp/file/
# ucpfile1, ucb/source/ucp/tdoc/ucptdoc1, unotools/util/utl, unoxml/source/
# rdf/unordf, unoxml/source/service/unoxml, uui/util/uui, xmloff/util/xo --
# each lookup key re-verified against its owning module's own librestudio_
# add_component() call), use_configuration -> CONFIGURATION.
librestudio_add_cppunit_test(dbaccess_dialog_save
    SOURCES
        dbaccess/qa/extras/dialog-save
    LIBRARIES
        basegfx comphelper cppu cppuhelper dba dbu sdbt drawinglayer editeng
        for forui i18nlangtag msfilter oox sal salhelper sax sb sfx sot svl
        svt svx svxcore test_fixtures subsequenttest tl tk ucbhelper unotest
        utl vbahelper vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        sb comphelper configmgr dba dbu sdbt dbaxml filterconfig frm fwk
        i18npool lng package2 xstor sax basprov scriptframe sfx sot
        fsstorage svl tk ucb1 ucpfile1 ucptdoc1 utl unordf unoxml uui xo
    CONFIGURATION)

# --- dbaccess_empty_stdlib_save ---------------------------------------------
#
# CppunitTest_dbaccess_empty_stdlib_save.mk, re-checked directly: 1 source
# (dbaccess/qa/extras/empty-stdlib-save), identical LIBRARIES/use_api/
# use_ure+use_vcl/use_components shape to dbaccess_dialog_save above, plus
# use_uiconfigs(dbaccess) -> UICONFIG dbaccess (this module's own "dbaccess_
# uiconfig" ALL target, ported alongside dbaccess_dialogs_test above).
librestudio_add_cppunit_test(dbaccess_empty_stdlib_save
    SOURCES
        dbaccess/qa/extras/empty-stdlib-save
    LIBRARIES
        basegfx comphelper cppu cppuhelper dba dbu sdbt drawinglayer editeng
        for forui i18nlangtag msfilter oox sal salhelper sax sb sfx sot svl
        svt svx svxcore test_fixtures subsequenttest tl tk ucbhelper unotest
        utl vbahelper vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        sb comphelper configmgr dba dbu sdbt dbaxml filterconfig frm fwk
        i18npool lng package2 xstor sax basprov scriptframe sfx sot
        fsstorage svl tk ucb1 ucpfile1 ucptdoc1 utl unordf unoxml uui xo
    UICONFIG dbaccess
    CONFIGURATION)

# --- dbaccess_nolib_save -----------------------------------------------------
#
# CppunitTest_dbaccess_nolib_save.mk, re-checked directly: 1 source
# (dbaccess/qa/extras/nolib-save), identical shape to dbaccess_dialog_save
# above (no use_uiconfigs call in this one -- re-verified directly).
librestudio_add_cppunit_test(dbaccess_nolib_save
    SOURCES
        dbaccess/qa/extras/nolib-save
    LIBRARIES
        basegfx comphelper cppu cppuhelper dba dbu sdbt drawinglayer editeng
        for forui i18nlangtag msfilter oox sal salhelper sax sb sfx sot svl
        svt svx svxcore test_fixtures subsequenttest tl tk ucbhelper unotest
        utl vbahelper vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        sb comphelper configmgr dba dbu sdbt dbaxml filterconfig frm fwk
        i18npool lng package2 xstor sax basprov scriptframe sfx sot
        fsstorage svl tk ucb1 ucpfile1 ucptdoc1 utl unordf unoxml uui xo
    CONFIGURATION)

# --- dbaccess_hsqlschema_import ----------------------------------------------
#
# CppunitTest_dbaccess_hsqlschema_import.mk, re-checked directly: 1 source
# (dbaccess/qa/extras/hsql_schema_import), set_include (-Idbaccess/source/
# filter/hsqldb $$(INCLUDE)), use_libraries (basegfx comphelper cppu
# cppuhelper dba dbahsql dbu sdbt drawinglayer editeng for forui i18nlangtag
# msfilter oox sal salhelper sax sb sfx sot svl svt svx svxcore test
# subsequenttest tl tk ucbhelper unotest utl vbahelper(optional SCRIPTING)
# vcl xo -- needs "dbahsql" additionally vs. the three above), use_api
# (offapi oovbaapi udkapi). NOTABLY ABSENT from this .mk (re-verified
# directly, twice): no use_ure, no use_vcl, no use_rdb call anywhere -- so
# no PROTECTORS at all, unlike every other CppunitTest in this port so far.
# use_components (same 28-name set as dbaccess_dialog_save above, PLUS
# oox/util/oox -> "oox" -- re-verified this is the one extra component the
# other three do not have), use_configuration -> CONFIGURATION.
librestudio_add_cppunit_test(dbaccess_hsqlschema_import
    SOURCES
        dbaccess/qa/extras/hsql_schema_import
    LIBRARIES
        basegfx comphelper cppu cppuhelper dba dbahsql dbu sdbt drawinglayer
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sb sfx
        sot svl svt svx svxcore test_fixtures subsequenttest tl tk ucbhelper
        unotest utl vbahelper vcl xo
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/dbaccess/source/filter/hsqldb"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
    COMPONENTS
        sb comphelper configmgr dba dbu sdbt dbaxml filterconfig frm fwk
        i18npool lng oox package2 xstor sax basprov scriptframe sfx sot
        fsstorage svl tk ucb1 ucpfile1 ucptdoc1 utl unordf unoxml uui xo
    CONFIGURATION)

# dbaccess/CppunitTest_deferred.cmake needs program_services_rdb (only
# defined near the end of the root CMakeLists.txt, well after dbaccess's own
# add_subdirectory() position) -- see dbaccess/CMakeLists.txt's own
# cmake_language(DEFER) call site comment for the scheduling mechanism, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required there.
