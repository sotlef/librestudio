# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# chart2_splines, chart2_uichart and chart2_dialogs_test -- the 3 of
# chart2's 15 CppunitTest_*.mk files that call use_rdb(services) -> DEPENDS
# program_services_rdb, defined near the end of the root CMakeLists.txt,
# well after chart2's own add_subdirectory() position. See chart2/
# CMakeLists.txt's own header comment (added alongside this wiring) for
# which of the other 12 are wired directly there (2) vs. left as documented,
# not-yet-attempted gaps (originally 10: postprocess_images, writerfilter,
# unordf, odfflatxml/xmlfa/xmlfd).
#
# 2026-09-28: sw_writerfilter is now built (sw/CMakeLists.txt) and unordf was
# already vendored earlier this same session (raptor2/rasqal/redland) -- of
# the 4 tests chart2/CMakeLists.txt's own header comment flagged as blocked
# by those two, chart2_dump and chart2_xshape needed ONLY writerfilter+unordf
# (re-verified directly: neither one's own use_components list mentions
# odfflatxml/xmlfa/xmlfd at all) and are wired below. chart2_pivot_chart_test
# and chart2_trendcalculators additionally needed odfflatxml/xmlfa/xmlfd --
# now ALSO resolved, same day, separate follow-up (filter/CMakeLists.txt's
# own odfflatxml/xmlfa/xmlfd wave -- each a 1-source-file library, no
# external beyond boost_headers, use_sdk_api only, every use_libraries entry
# already ported elsewhere in this tree; also closes the same gap sc_
# filters_test's own COMPONENTS list already documented) -- both wired below
# too, alongside chart2_import/import2/geometry (same blocker, chart2/
# import_setup.mk's own shared macro + the standalone geometry .mk).
# COMPONENTS names below (both new tests) were re-verified the same way,
# against a fresh project-wide grep, not assumed from gbuild path spelling --
# two mapping surprises: "filter/source/config/cache/filterconfig1" ->
# registered name "filterconfig" (not "filterconfig1", filter/CMakeLists.txt
# names its own componentfile filterconfig1.component but the CMake target/
# component name is "filterconfig"), and "sax/source/expatwrap/expwrap" ->
# registered name "sax" (sax/CMakeLists.txt's own single librestudio_add_
# component(sax ...) call covers that componentfile too -- same finding this
# file's own header already documented for sax_xmlimport).

# --- chart2_splines -------------------------------------------------------
#
# CppunitTest_chart2_splines.mk, re-checked directly: 1 source (chart2/qa/
# unit/splines_test), use_external(libxml2), use_libraries (chart2 sal
# subsequenttest tl test unotest utl cppu cppuhelper -- "test" ->
# test_fixtures), set_include (-Ichart2/source/view/charttypes $$(INCLUDE)),
# use_sdk_api, use_ure + use_vcl (plain, headless) -> 3 protector pairs,
# use_rdb(services) -> DEPENDS program_services_rdb (no COMPONENTS),
# use_configuration -> CONFIGURATION.
librestudio_add_cppunit_test(chart2_splines
    SOURCES
        chart2/qa/unit/splines_test
    LIBRARIES
        chart2 sal subsequenttest tl test_fixtures unotest utl cppu cppuhelper
        LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/chart2/source/view/charttypes"
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

# --- chart2_uichart --------------------------------------------------------
#
# CppunitTest_chart2_uichart.mk, re-checked directly: 1 source (chart2/qa/
# extras/uichart), use_externals(boost_headers libxml2), use_libraries
# (comphelper cppu cppuhelper i18nlangtag sal sc sfx subsequenttest svl svx
# svxcore test tl unotest utl vcl -- "svl" listed twice in the live .mk,
# de-duplicated here, "test" -> test_fixtures), set_include (-Ichart2/qa/
# extras -Ichart2/inc $$(INCLUDE)), use_sdk_api, use_ure + use_vcl (plain,
# headless) -> same 3 protector pairs, use_rdb(services) -> DEPENDS
# program_services_rdb (no COMPONENTS), use_configuration -> CONFIGURATION.
librestudio_add_cppunit_test(chart2_uichart
    SOURCES
        chart2/qa/extras/uichart
    LIBRARIES
        comphelper cppu cppuhelper i18nlangtag sal sc sfx subsequenttest svl
        svx svxcore test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/chart2/qa/extras"
        "${CMAKE_SOURCE_DIR}/chart2/inc"
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

# --- chart2_dialogs_test (screenshot test) ---------------------------------
#
# CppunitTest_chart2_dialogs_test.mk, re-checked directly: 1 source (chart2/
# qa/unit/chart2-dialogs-test), use_sdk_api (twice, no-op second time),
# set_include (-Ichart2/source/inc -Ichart2/inc $$(INCLUDE)), use_libraries
# (basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
# i18nutil msfilter oox sal salhelper sax sfx sot svl svt svx svxcore test tl
# tk ucbhelper unotest utl vcl xo -- "test" -> test_fixtures), use_external(
# boost_headers), use_ure + use_vcl_non_headless_with_windows -> 3 protector
# pairs + NONHEADLESS, use_rdb(services) -> DEPENDS program_services_rdb (no
# COMPONENTS), use_configuration -> CONFIGURATION, use_uiconfigs(
# "modules/schart", svx) -> UICONFIG schart svx (chart2/CMakeLists.txt's own
# new "schart_uiconfig" ALL target, ported alongside this wiring; "svx"
# already existed), use_packages(extras_palettes) -> PACKAGES extras_palettes
# (extras/CMakeLists.txt's own new librestudio_add_package_mirror() call,
# ported alongside this wiring, using the same generic helper wizards'/
# scripting's own PACKAGES targets already established -- cmake/
# LibreStudioPackage.cmake).
librestudio_add_cppunit_test(chart2_dialogs_test
    SOURCES
        chart2/qa/unit/chart2-dialogs-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
        i18nutil msfilter oox sal salhelper sax sfx sot svl svt svx svxcore
        test_fixtures tl tk ucbhelper unotest utl vcl xo
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/chart2/source/inc"
        "${CMAKE_SOURCE_DIR}/chart2/inc"
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
        extras_palettes
    UICONFIG schart svx
    NONHEADLESS
    CONFIGURATION)

# --- chart2_dump -------------------------------------------------------------
#
# CppunitTest_chart2_dump.mk, re-checked directly: 1 source (chart2/qa/
# extras/chart2dump/chart2dump), use_externals(boost_headers libxml2),
# use_libraries (a large list, "sw" listed twice in the real .mk -- de-
# duplicated here -- + gb_Helper_optional(SCRIPTING, vbahelper), resolves to
# "on" since ENABLE_SCRIPTING defaults ON in this port, same resolution
# already used throughout the sc/sw/sd wave), set_include (-Ichart2/qa/extras
# -Ichart2/inc $$(INCLUDE)), use_sdk_api, use_ure + use_vcl (plain, headless)
# -> 3 protector pairs, use_components (48-name list, no use_rdb(services)),
# use_configuration -> CONFIGURATION.
librestudio_add_cppunit_test(chart2_dump
    SOURCES
        chart2/qa/extras/chart2dump/chart2dump
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng for forui
        i18nlangtag msfilter vcl oox sal salhelper sax sb sc sw sd sfx sot
        subsequenttest svl svt svx svxcore test_fixtures tl tk ucbhelper
        unotest utl vbahelper xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/chart2/qa/extras"
        "${CMAKE_SOURCE_DIR}/chart2/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        sb animcore chart2 comphelper configmgr embobj evtatt filterconfig
        storagefd frm fwk i18npool lng oox xstor package2 sax sc scd scfilt
        sw swd msword sd sdd vbaobj analysis date basprov scriptframe sfx sot
        fsstorage svl svt svx svxcore sw_writerfilter tk vcl ucb1 ucpfile1
        ucptdoc1 uui utl unordf unoxml xo
    CONFIGURATION)

# --- chart2_xshape -------------------------------------------------------------
#
# CppunitTest_chart2_xshape.mk, re-checked directly: 1 source (chart2/qa/
# extras/xshape/chart2xshape), same LIBRARIES/COMPONENTS shape as chart2_dump
# (same "sw" listed twice, same gb_Helper_optional(SCRIPTING, vbahelper) ->
# on), set_include (-Ichart2/qa/extras -Ichart2/inc $$(INCLUDE)), use_sdk_api,
# use_ure + use_vcl -> 3 protector pairs, use_components (same 48 names as
# chart2_dump), use_configuration -> CONFIGURATION, use_uiconfigs(modules/
# scalc modules/simpress sfx svt) -- "scalc_uiconfig" does not exist in this
# port (only simpress_uiconfig was ported, same gap already documented
# throughout the sc/sd waves), so "scalc" is omitted from UICONFIG below, a
# real, honest gap, not a mapping error.
librestudio_add_cppunit_test(chart2_xshape
    SOURCES
        chart2/qa/extras/xshape/chart2xshape
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng for forui
        i18nlangtag msfilter vcl oox sal salhelper sax sb sc sw sd sfx sot
        subsequenttest svl svt svx svxcore test_fixtures tl tk ucbhelper
        unotest utl vbahelper xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/chart2/qa/extras"
        "${CMAKE_SOURCE_DIR}/chart2/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        sb animcore chart2 comphelper configmgr embobj evtatt filterconfig
        storagefd frm fwk i18npool lng oox xstor package2 sax sc scd scfilt
        sw swd msword sd sdd vbaobj analysis date basprov scriptframe sfx sot
        fsstorage svl svt svx svxcore sw_writerfilter tk vcl ucb1 ucpfile1
        ucptdoc1 uui utl unordf unoxml xo
    UICONFIG
        simpress sfx svt
    CONFIGURATION)

# --- chart2_export / chart2_export2 / chart2_export3 -------------------------
#
# chart2/export_setup.mk's own `define chart2_export$(1)_test`, re-checked
# directly: 1 source (chart2/qa/extras/chart2export$(1)), use_externals
# (boost_headers libxml2), use_libraries (AVMEDIA optional -> avmedia "on" +
# gb_Helper_optional(SCRIPTING, vbahelper) -> "on", both already-established
# resolutions in this port), set_include (-Ichart2/inc $$(INCLUDE)),
# use_sdk_api, use_ure + use_vcl -> 3 protector pairs, use_rdb(services) ->
# DEPENDS program_services_rdb (NO use_components at all -- unlike chart2_
# import/geometry below, these 3 were blocked ONLY by postprocess_images),
# use_uiconfigs(modules/swriter) -> UICONFIG swriter, use_configuration ->
# CONFIGURATION, plus the raw postprocess_images order-only prerequisite
# (2026-09-28: postprocess_images is now built, see postprocess/CMakeLists.
# txt) -> DEPENDS postprocess_images.
function(librestudio_add_chart2_export_test suffix)
    librestudio_add_cppunit_test(chart2_export${suffix}
        SOURCES
            chart2/qa/extras/chart2export${suffix}
        LIBRARIES
            avmedia basegfx comphelper cppu cppuhelper drawinglayer editeng for
            forui i18nlangtag msfilter oox sal salhelper sax sb sc sw sd sfx
            sot subsequenttest svl svt svx svxcore test_fixtures tl tk
            ucbhelper unotest utl vbahelper vcl xo
            Boost::boost LibXml2::LibXml2
        INCLUDE
            "${CMAKE_SOURCE_DIR}/chart2/inc"
            "${udkapi_normal_HEADERS_INCLUDE_DIR}"
            "${offapi_normal_HEADERS_INCLUDE_DIR}"
        DEPENDS
            udkapi_normal_headers offapi_normal_headers
            program_services_rdb postprocess_images
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
            vclbootstrapprotector vclbootstrapprotector
        UICONFIG
            swriter
        CONFIGURATION)
endfunction()

librestudio_add_chart2_export_test("")
librestudio_add_chart2_export_test(2)
librestudio_add_chart2_export_test(3)

# --- chart2_import / chart2_import2 --------------------------------------
#
# chart2/import_setup.mk's own `define chart2_import$(1)_test`, re-checked
# directly: 1 source (chart2/qa/extras/chart2import$(1)), use_externals
# (boost_headers libxml2), use_libraries (AVMEDIA optional -> avmedia "on" +
# gb_Helper_optional(SCRIPTING, vbahelper) -> "on"), set_include (-Ichart2/
# inc $$(INCLUDE)), use_sdk_api, use_ure + use_vcl -> 3 protector pairs,
# use_components (48-name list, now fully resolvable -- odfflatxml/xmlfa/
# xmlfd all built, see filter/CMakeLists.txt), use_uiconfigs(modules/
# swriter) -> UICONFIG swriter, use_configuration -> CONFIGURATION, plus the
# raw postprocess_images order-only prerequisite (now built, see
# postprocess/CMakeLists.txt) -> DEPENDS postprocess_images. COMPONENTS
# names re-verified against a fresh project-wide grep: "dbaccess/util/dba"
# -> "dba" (dbaccess/CMakeLists.txt), "xmlscript/util/xmlscript" ->
# "xmlscript" (xmlscript/CMakeLists.txt), "emfio/emfio" -> "emfio" (emfio/
# CMakeLists.txt) -- all three confirmed real and already ported.
function(librestudio_add_chart2_import_test suffix)
    librestudio_add_cppunit_test(chart2_import${suffix}
        SOURCES
            chart2/qa/extras/chart2import${suffix}
        LIBRARIES
            avmedia basegfx comphelper cppu cppuhelper drawinglayer docmodel
            editeng for forui i18nlangtag msfilter oox sal salhelper sax sb
            sc sw sd sfx sot subsequenttest svl svt svx svxcore test_fixtures
            tl tk ucbhelper unotest utl vbahelper vcl xo
            Boost::boost LibXml2::LibXml2
        INCLUDE
            "${CMAKE_SOURCE_DIR}/chart2/inc"
            "${udkapi_normal_HEADERS_INCLUDE_DIR}"
            "${offapi_normal_HEADERS_INCLUDE_DIR}"
        DEPENDS
            udkapi_normal_headers offapi_normal_headers
            program_services_rdb postprocess_images
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
            vclbootstrapprotector vclbootstrapprotector
        COMPONENTS
            sb animcore chart2 comphelper configmgr dba embobj emfio evtatt
            filterconfig odfflatxml storagefd xmlfa xmlfd frm fwk i18npool
            lng oox xstor package2 sax sc scd scfilt sw swd msword sd sdd
            vbaobj analysis date basprov scriptframe sfx sot fsstorage svl
            svt svx svxcore sw_writerfilter tk vcl ucb1 ucpfile1 ucptdoc1
            utl unordf unoxml uui xo xmlscript
        UICONFIG
            swriter
        CONFIGURATION)
endfunction()

librestudio_add_chart2_import_test("")
librestudio_add_chart2_import_test(2)

# --- chart2_geometry -------------------------------------------------------
#
# CppunitTest_chart2_geometry.mk, re-checked directly: 1 source (chart2/qa/
# extras/chart2geometry), otherwise identical shape to chart2_import above
# (same AVMEDIA/SCRIPTING optionals, same 48-name COMPONENTS list, same
# UICONFIG swriter, same postprocess_images order-only prerequisite) -- the
# one difference is LIBRARIES has no "docmodel" (re-checked, genuinely
# absent from this .mk unlike chart2_import's own list).
librestudio_add_cppunit_test(chart2_geometry
    SOURCES
        chart2/qa/extras/chart2geometry
    LIBRARIES
        avmedia basegfx comphelper cppu cppuhelper drawinglayer editeng for
        forui i18nlangtag msfilter oox sal salhelper sax sb sc sw sd sfx sot
        subsequenttest svl svt svx svxcore test_fixtures tl tk ucbhelper
        unotest utl vbahelper vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/chart2/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb postprocess_images
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        sb animcore chart2 comphelper configmgr dba embobj emfio evtatt
        filterconfig odfflatxml storagefd xmlfa xmlfd frm fwk i18npool lng
        oox xstor package2 sax sc scd scfilt sw swd msword sd sdd vbaobj
        analysis date basprov scriptframe sfx sot fsstorage svl svt svx
        svxcore sw_writerfilter tk vcl ucb1 ucpfile1 ucptdoc1 utl unordf
        unoxml uui xo xmlscript
    UICONFIG
        swriter
    CONFIGURATION)

# --- chart2_pivot_chart_test -------------------------------------------------
#
# CppunitTest_chart2_pivot_chart_test.mk, re-checked directly: 1 source
# (chart2/qa/extras/PivotChartTest), LIBRARIES additionally includes
# "chart2api" (chart2/CMakeLists.txt's own tiny abstract-interface library,
# already built) and "sw" listed twice in the real .mk (de-duplicated here);
# COMPONENTS additionally includes "dbaccess/util/dba" -> "dba" (unlike
# chart2_trendcalculators below, which has no dbaccess dependency) and
# "xmlscript/util/xmlscript" -> "xmlscript". NO use_uiconfigs call in this
# .mk (re-checked, genuinely absent) -- no UICONFIG parameter needed.
librestudio_add_cppunit_test(chart2_pivot_chart_test
    SOURCES
        chart2/qa/extras/PivotChartTest
    LIBRARIES
        basegfx chart2api comphelper cppu cppuhelper drawinglayer editeng
        for forui i18nlangtag msfilter vcl oox sal salhelper sax sb sc sw sd
        sfx sot subsequenttest svl svt svx svxcore test_fixtures tl tk
        ucbhelper unotest utl vbahelper xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/chart2/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb postprocess_images
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        sb animcore chart2 comphelper configmgr dba embobj evtatt
        filterconfig odfflatxml storagefd xmlfa xmlfd frm fwk i18npool lng
        oox xstor package2 sax sc scd scfilt sw swd msword sd sdd vbaobj
        analysis date basprov scriptframe sfx sot fsstorage svl svt svx
        svxcore sw_writerfilter tk vcl ucb1 ucpfile1 ucptdoc1 utl unordf
        unoxml uui xo xmlscript
    CONFIGURATION)

# --- chart2_trendcalculators -------------------------------------------------
#
# CppunitTest_chart2_trendcalculators.mk, re-checked directly: 1 source
# (chart2/qa/extras/chart2_trendcalculators), same LIBRARIES/COMPONENTS
# shape as chart2_dump/chart2_xshape (no chart2api, no dbaccess dependency),
# now also including odfflatxml/xmlfa/xmlfd. NO use_uiconfigs call in this
# .mk either.
librestudio_add_cppunit_test(chart2_trendcalculators
    SOURCES
        chart2/qa/extras/chart2_trendcalculators
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng for forui
        i18nlangtag msfilter vcl oox sal salhelper sax sb sc sw sd sfx sot
        subsequenttest svl svt svx svxcore test_fixtures tl tk ucbhelper
        unotest utl vbahelper xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/chart2/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb postprocess_images
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        sb animcore chart2 comphelper configmgr embobj evtatt filterconfig
        odfflatxml storagefd xmlfa xmlfd frm fwk i18npool lng oox xstor
        package2 sax sc scd scfilt sw swd msword sd sdd vbaobj analysis date
        basprov scriptframe sfx sot fsstorage svl svt svx svxcore
        sw_writerfilter tk vcl ucb1 ucpfile1 ucptdoc1 utl unordf unoxml uui
        xo xmlscript
    CONFIGURATION)

# chart2/CppunitTest_deferred.cmake needs program_services_rdb (only defined
# near the end of the root CMakeLists.txt, well after chart2's own add_
# subdirectory() position) -- see chart2/CMakeLists.txt's own cmake_language(
# DEFER) call site comment for the scheduling mechanism, and cmake/
# LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required there.
