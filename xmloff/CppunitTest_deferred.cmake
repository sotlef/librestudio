# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# All 5 of Module_xmloff.mk's CppunitTest check targets (xmloff_uxmloff,
# xmloff_draw, xmloff_forms, xmloff_style, xmloff_text -- the .mk's own
# "$(if $(MERGELIBS),, CppunitTest_xmloff_uxmloff)" guard resolves to
# "include it", since MERGELIBS is never defined anywhere in this port,
# verified with `grep -rn MERGELIBS`) -- split out of xmloff/CMakeLists.txt
# into their own file, include()'d from the root CMakeLists.txt only once
# every module/target each one separately needs already exists as a real
# CMake target:
#   - xmloff_uxmloff's own COMPONENTS name ucb1/ucpfile1 (ucb/CMakeLists.txt,
#     add_subdirectory()'d well after xmloff's own position) and unoxml
#     (unoxml/CMakeLists.txt, add_subdirectory()'d immediately after xmloff's
#     own position, so still after it) -- plus, via CONFIGURATION and LIBRARIES
#     test_fixtures/the protector plug-ins, test/unotest's own add_subdirectory()
#     (also after xmloff). None of the four call program_services_rdb.
#   - xmloff_draw/xmloff_forms/xmloff_style/xmloff_text all call
#     gb_CppunitTest_use_rdb(name,services), which needs the program_
#     services_rdb target that only exists near the very end of the root
#     CMakeLists.txt (same reasoning as scaddins/CppunitTest_deferred.cmake's
#     own header comment -- read that file first for the full use_rdb(services)
#     / ini-fallback treatment, which applies identically to all four here).
# Since all 5 share one file, the single include() call site sits at the
# latest of these constraints -- program_services_rdb -- the same "combine
# into one file, bind at the later test's constraint" trade-off toolkit/
# CppunitTest_deferred.cmake's own header comment already documents for
# toolkit_toolkit+toolkit_a11y. xmloff_uxmloff's own, earlier-satisfied
# constraints (ucb/unoxml/test, all add_subdirectory()'d long before
# program_services_rdb exists) cost nothing extra by waiting until then too.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not xmloff's, source paths below are still fine: librestudio_expand_cxx
# (cmake/LibreStudioPlatform.cmake) resolves them against CMAKE_SOURCE_DIR
# (the absolute tree root), never CMAKE_CURRENT_SOURCE_DIR. XMLOFF_GENERATED_DIR
# (xmloff_uxmloff's own use_custom_headers dependency, below) is the one
# directory-scoped-by-default value xmloff/CMakeLists.txt sets that this file
# needs -- it is now CACHE INTERNAL there (same fix as oox/CMakeLists.txt's own
# OOX_GENERATED_DIR) specifically so it is visible here; see that file's own
# comment on the point. OFFICECFG_INCLUDE_DIR (officecfg/CMakeLists.txt) is
# already CACHE INTERNAL from when it was introduced, so no equivalent fix was
# needed for it. xmloff_generated (the custom target XMLOFF_GENERATED_DIR's
# outputs depend on) and every LIBRARIES/COMPONENTS target below are real
# CMake targets, not variables, so they are visible globally regardless of
# directory scope -- only variables have this wrinkle.

# CppunitTest_xmloff_uxmloff.mk, re-checked directly against the live file: 2
# sources (xmloff/qa/unit/uxmloff, xmloff/qa/unit/tokenmap-test), set_include
# adds xmloff/inc + xmloff/source/style + xmloff/source/chart on top of
# $$(INCLUDE) (use_sdk_api's own udkapi/offapi dirs) -- transcribed faithfully
# as all three, not narrowed to only what this file's own two .cxx sources
# directly reach for (SchXMLExport.hxx/XMLChartPropertySetMapper.hxx resolve
# via plain xmloff/inc alone, confirmed by locating both files there directly,
# but the .mk lists all three and per this task's brief every set_include
# entry is transcribed as declared, not second-guessed against one directory's
# own #include list). use_custom_headers(xmloff/generated) -> XMLOFF_GENERATED_DIR
# + DEPENDS xmloff_generated (the token.hxx/tokenhash.inc CustomTarget this
# module's own xmloff/CMakeLists.txt already builds; xmloff/qa/unit/
# tokenmap-test.cxx directly includes <xmloff/token/tokens.hxx>). use_sdk_api
# -> udkapi_normal + offapi_normal (normal variant, use_sdk_api not use_api).
# use_ure + use_vcl -> 3 protector pairs (unobootstrapprotector x2,
# unoexceptionprotector x2, vclbootstrapprotector x2), same precedent as every
# other use_ure+use_vcl combination in this port.
# gb_CppunitTest_use_library_objects(xmloff_uxmloff,xo): NOT modelled as an
# object-recompile the way sal_osl_file_details/svx_removewhichrange's
# genuinely-unexported-symbol cases are (see svx/CMakeLists.txt's own
# CppunitTest_svx_removewhichrange comment for the full reasoning this
# mirrors) -- re-checked directly: xmloff/qa/unit/uxmloff.cxx's own symbols
# (SvXMLAutoStylePoolP, XMLChartPropertySetMapper, XMLChartExportPropertyMapper,
# SchXMLExport, SvXMLExport, SvXMLImport, SvXMLMetaDocumentContext) carry no
# XMLOFF_DLLPUBLIC-style export restriction that would matter here: xmloff has
# no xmloff/util/*.map (or any *.map) file anywhere under xmloff/, xo's own
# target_link_libraries() applies no -Wl,--version-script (confirmed by
# reading xmloff/CMakeLists.txt's xo block directly), and this port sets no
# -fvisibility=hidden anywhere at all (grepped cmake/*.cmake and the root
# CMakeLists.txt, no matches) -- so every one of xo's global symbols is
# already reachable by linking against the real xo *library*, the same
# conclusion svx_removewhichrange's comment reaches for svxcore. Modelled as a
# plain LIBRARIES xo entry, added on top of the .mk's own use_libraries list
# below.
# use_libraries: basegfx chart2api comphelper cppu cppuhelper docmodel
# i18nlangtag sal salhelper sax svl test tl unotest utl vcl -- "test" ->
# test_fixtures, this port's established rename (bare "test" is reserved by
# CTest's own enable_testing(), see test/CMakeLists.txt's own comment).
# use_components (COMPONENTS below) lists 6 names; the 3 commented-out lines
# right after the live .mk's own use_components block ("#    uui/util/uui \",
# "#    sfx2/util/sfx \", "#    framework/util/fwk \") are dead Makefile
# comments outside the $(eval $(call ...)) block they trail, re-checked
# directly against the live file -- NOT part of the active list, so they are
# not transcribed into COMPONENTS below either.
# COMPONENTS lookup keys, each verified against the owning module's own
# librestudio_add_component() call: "configmgr/source/configmgr" ->
# "configmgr" (configmgr/CMakeLists.txt), "i18npool/util/i18npool" ->
# "i18npool" (i18npool/CMakeLists.txt), "sax/source/expatwrap/expwrap" ->
# "sax" (sax/CMakeLists.txt, same lookup key sax_xmlimport/configmgr_unit
# already use for this identical component), "ucb/source/core/ucb1" ->
# "ucb1" and "ucb/source/ucp/file/ucpfile1" -> "ucpfile1" (both ucb/
# CMakeLists.txt), "unoxml/source/service/unoxml" -> "unoxml" (unoxml/
# CMakeLists.txt).
# use_configuration -> CONFIGURATION (cmake/LibreStudioTest.cmake). No
# use_rdb here, so no program_services_rdb DEPENDS for this one test --
# unlike the other 4 below.
librestudio_add_cppunit_test(xmloff_uxmloff
    SOURCES
        xmloff/qa/unit/uxmloff
        xmloff/qa/unit/tokenmap-test
    LIBRARIES
        basegfx chart2api comphelper cppu cppuhelper docmodel i18nlangtag sal
        salhelper sax svl test_fixtures tl unotest utl vcl xo
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/xmloff/inc"
        "${CMAKE_SOURCE_DIR}/xmloff/source/style"
        "${CMAKE_SOURCE_DIR}/xmloff/source/chart"
        "${XMLOFF_GENERATED_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        xmloff_generated
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool sax ucb1 ucpfile1 unoxml
    CONFIGURATION)

# xmloff_draw, xmloff_forms, xmloff_style, xmloff_text (CppunitTest_xmloff_
# draw.mk / CppunitTest_xmloff_forms.mk / CppunitTest_xmloff_style.mk /
# CppunitTest_xmloff_text.mk): all 4 share the same overall shape, re-checked
# directly against their live files --
#   use_externals(boost_headers,libxml2) -> Boost::boost + LibXml2::LibXml2 +
#     LIBXML2_INCLUDE_DIRS (find_package(LibXml2 REQUIRED), root CMakeLists.txt
#     line 248, well before xmloff's own add_subdirectory() -- same global
#     availability every other use_externals(libxml2) call site in this port
#     already relies on).
#   use_sdk_api -> udkapi_normal + offapi_normal.
#   use_ure + use_vcl -> 3 protector pairs, same precedent as xmloff_uxmloff
#     above and every other use_ure+use_vcl call site in this port.
#   use_rdb(services) -> DEPENDS program_services_rdb, no COMPONENTS -- same
#     ini-fallback treatment as scaddins_analysis/forms_unit/officecfg_
#     accelerator_key_checker_test (see scaddins/CppunitTest_deferred.cmake's
#     header comment for the full reasoning).
#   use_custom_headers(officecfg/registry) -> INCLUDE OFFICECFG_INCLUDE_DIR +
#     DEPENDS officecfg_headers, the identical mapping officecfg_
#     cppheader_test/configmgr_unit already establish for this same
#     use_custom_headers call (officecfg/CMakeLists.txt's own comment on
#     officecfg_cppheader_test). Re-checked directly: none of the 4 .cxx files
#     below actually #include a generated officecfg header themselves, but the
#     .mk calls use_custom_headers(officecfg/registry) for all four regardless
#     (most likely reached only transitively, e.g. via <test/unoapi_test.hxx>'s
#     own bootstrap), so it is transcribed faithfully rather than dropped as
#     apparently-unused.
#   use_configuration -> CONFIGURATION.
#   add_arguments(-env:arg-env=...) -> EXTRA_ARGS, using this port's
#     established CMake-configure-time LD_LIBRARY_PATH snapshot (smoketest/
#     CMakeLists.txt's own SMOKETEST_EXTRA_ARGS precedent and its comment for
#     the full "why a shell parameter-expansion trick has no CMake add_test()
#     equivalent" reasoning, which applies identically here): if
#     LD_LIBRARY_PATH is set and non-empty at CMake configure time, bake in a
#     literal "-env:arg-env=LD_LIBRARY_PATH=<value>"; otherwise omit the flag
#     entirely. Computed once below and reused by all four tests, since all
#     four .mk files carry byte-for-byte the same add_arguments call.
#
# None of the 4 use set_include for anything beyond use_sdk_api's own udkapi/
# offapi dirs -- re-verified per test, not assumed identical just because
# they share a shape: xmloff_forms's own .mk (unlike the other three) DOES add
# one extra set_include entry, "-I$(SRCDIR)/xmloff/inc" (xmloff/qa/unit/
# forms.cxx #includes <forms/gridcolumnproptranslator.hxx>, which is NOT under
# the top-level include/ tree covered by this port's own global
# include_directories("${CMAKE_SOURCE_DIR}/include") -- cmake/
# LibreStudioPlatform.cmake -- but lives at xmloff/inc/forms/
# gridcolumnproptranslator.hxx instead, confirmed by locating the file
# directly), so that one INCLUDE entry is added to xmloff_forms alone below,
# not to draw/style/text (their own #include lists were checked directly too;
# every other header any of the 4 reaches for -- svx/*, docmodel/*, unotools/*,
# tools/color.hxx, test/unoapi_test.hxx, comphelper/* -- resolves via that same
# global include/ path already, so nothing else was needed).
set(XMLOFF_QA_EXTRA_ARGS "")
if (DEFINED ENV{LD_LIBRARY_PATH} AND NOT "$ENV{LD_LIBRARY_PATH}" STREQUAL "")
    set(XMLOFF_QA_EXTRA_ARGS "-env:arg-env=LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}")
endif ()

# CppunitTest_xmloff_draw.mk: 1 source (xmloff/qa/unit/draw), use_libraries
# comphelper cppu cppuhelper docmodel embobj sal subsequenttest test unotest
# utl tl avmedia svxcore vcl ("test" -> test_fixtures).
librestudio_add_cppunit_test(xmloff_draw
    SOURCES
        xmloff/qa/unit/draw
    LIBRARIES
        comphelper cppu cppuhelper docmodel embobj sal subsequenttest
        test_fixtures unotest utl tl avmedia svxcore vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
        "${LIBXML2_INCLUDE_DIRS}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION
    EXTRA_ARGS ${XMLOFF_QA_EXTRA_ARGS})

# CppunitTest_xmloff_forms.mk: 1 source (xmloff/qa/unit/forms), set_include
# adds xmloff/inc on top of $$(INCLUDE) (see this file's header comment above
# for why -- <forms/gridcolumnproptranslator.hxx>), use_libraries comphelper
# cppu cppuhelper docmodel embobj sal subsequenttest test unotest utl tl xo
# ("test" -> test_fixtures; "xo" is this module's own xmloff/CMakeLists.txt
# target, a plain link this time, not use_library_objects).
librestudio_add_cppunit_test(xmloff_forms
    SOURCES
        xmloff/qa/unit/forms
    LIBRARIES
        comphelper cppu cppuhelper docmodel embobj sal subsequenttest
        test_fixtures unotest utl tl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/xmloff/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
        "${LIBXML2_INCLUDE_DIRS}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION
    EXTRA_ARGS ${XMLOFF_QA_EXTRA_ARGS})

# CppunitTest_xmloff_style.mk: 1 source (xmloff/qa/unit/style), use_libraries
# basegfx comphelper cppu cppuhelper docmodel subsequenttest embobj sal test
# unotest utl tl ("test" -> test_fixtures).
librestudio_add_cppunit_test(xmloff_style
    SOURCES
        xmloff/qa/unit/style
    LIBRARIES
        basegfx comphelper cppu cppuhelper docmodel subsequenttest embobj sal
        test_fixtures unotest utl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
        "${LIBXML2_INCLUDE_DIRS}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION
    EXTRA_ARGS ${XMLOFF_QA_EXTRA_ARGS})

# CppunitTest_xmloff_text.mk: 3 sources (xmloff/qa/unit/text,
# xmloff/qa/unit/text/txtprmap, xmloff/qa/unit/text/txtparae), use_libraries
# comphelper cppu cppuhelper docmodel embobj sal subsequenttest test unotest
# utl tl ("test" -> test_fixtures).
librestudio_add_cppunit_test(xmloff_text
    SOURCES
        xmloff/qa/unit/text
        xmloff/qa/unit/text/txtprmap
        xmloff/qa/unit/text/txtparae
    LIBRARIES
        comphelper cppu cppuhelper docmodel embobj sal subsequenttest
        test_fixtures unotest utl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
        "${LIBXML2_INCLUDE_DIRS}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION
    EXTRA_ARGS ${XMLOFF_QA_EXTRA_ARGS})
