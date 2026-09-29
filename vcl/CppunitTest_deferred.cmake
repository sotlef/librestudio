# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# vcl_app_test, vcl_bitmapfilter_test, vcl_bitmap_test, vcl_filter_igif,
# vcl_graphic_test, vcl_idle_task, vcl_map, vcl_type_serializer_test -- split
# out of vcl/CMakeLists.txt into their own file, include()'d from the root
# CMakeLists.txt only after program_services_rdb exists, the same reason and
# call-site pattern as scaddins/CppunitTest_deferred.cmake and xmlscript/
# CppunitTest_deferred.cmake (read scaddins' file header comment first for
# the full use_rdb(services)/ini-fallback reasoning, which applies
# identically to all eight tests below: each calls gb_CppunitTest_use_rdb(
# name, services) -- re-checked directly against every one of these tests'
# live .mk files, not assumed from any prior summary -- so each gets no
# COMPONENTS, a DEPENDS on program_services_rdb, and relies on
# cppunittester's UNO bootstrap falling back to unorc's own
# "UNO_SERVICES=${ORIGIN}/services.rdb" ini default). Every one of them also
# calls gb_CppunitTest_use_configuration, which additionally requires
# LIBRESTUDIO_TEST_UNITTEST_DIR (test/CMakeLists.txt's staged test/
# user-template copy) -- re-checked against the root CMakeLists.txt's own
# add_subdirectory() order: vcl itself is add_subdirectory()'d at line 490,
# well BEFORE unotest (681) and test (682), so CONFIGURATION could not have
# been satisfied at vcl's own natural registration point either, even setting
# use_rdb(services)/program_services_rdb aside entirely -- both gaps push
# these tests to the same deferred-file treatment, and program_services_rdb
# (only defined much later still, right before this file's own include()
# point) ends up being the binding constraint in practice, exactly as it was
# for scaddins_analysis/xmlscript_cppunit (officecfg, CONFIGURATION's other
# dependency via officecfg_registry, IS add_subdirectory()'d before vcl, at
# line 390, so that half was never the problem).
#
# This is a real discrepancy from this wave's own planning notes, which
# expected these nine tests to need neither use_components nor
# use_configuration: every one of them (all but vcl_localize_digits/
# vcl_unit_conversion, which have no use_ure at all and are wired directly in
# vcl/CMakeLists.txt) does call use_configuration, and additionally calls
# use_rdb(services) -- a mechanism the original plan did not mention either.
# Confirmed by reading every one of these tests' live .mk file in full before
# writing anything here.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not vcl's, source paths below are still fine: librestudio_expand_cxx
# resolves them against CMAKE_SOURCE_DIR, not CMAKE_CURRENT_SOURCE_DIR (see
# sax/CppunitTest_deferred.cmake's own comment on this same point). The
# PkgConfig::NSS guard near the bottom of this file exists for the identical
# reason comphelper/CppunitTest_deferred.cmake's does: an IMPORTED_TARGET
# from pkg_check_modules is only visible in the directory it was created in
# and below, and this file's own directory scope is not a descendant of
# comphelper's (where PkgConfig::NSS is first created) or vcl's.
#
# vcl_pdfium_library_test (CppunitTest_vcl_pdfium_library_test.mk) is
# deliberately NOT registered anywhere in this port, unlike the eight tests
# below. What gates it upstream: 2 sources (PDFiumLibraryTest,
# PDFDocumentTest), use_externals boost_headers plus "$(if $(filter PDFIUM,
# $(BUILD_TYPE)),pdfium)" -- i.e. the external pdfium library is only linked
# at all when BUILD_TYPE contains PDFIUM (configure.ac's --enable-pdfium).
# This port models that same gate (vcl/CMakeLists.txt: "PDFIUM" IN_LIST
# LIBRESTUDIO_BUILD_TYPE, used there to choose between the real
# vcl/source/pdf/PDFiumLibrary.cxx and the vcl/source/pdf/
# DummyPDFiumLibrary.cxx stub), and LIBRESTUDIO_BUILD_TYPE defaults to just
# "DESKTOP" here -- PDFIUM is not in it, and external/pdfium has no
# CMakeLists.txt anywhere in this port (checked: the directory holds only
# upstream's Library_pdfium.mk/Module_pdfium.mk and vendoring patches, never
# ported to CMake), so there is no "pdfium" CMake target to link even if the
# BUILD_TYPE gate were flipped on. Unlike vcl_graphic_test's own PDFIUM-gated
# extra source below, this is not a case of "compiles fine either way, one
# source list is just shorter without it": PDFiumLibraryTest.cxx and
# PDFDocumentTest.cxx are the only two sources in the whole test, and both
# unconditionally exercise vcl::pdf::PDFiumLibrary::get() and then dereference
# its result (CPPUNIT_ASSERT(pPdfium) followed by pPdfium->openDocument(...))
# -- with DummyPDFiumLibrary.cxx's get() unconditionally returning a null
# shared_ptr (the only implementation this port ever compiles into vcl, since
# PDFIUM is never in LIBRESTUDIO_BUILD_TYPE here), the very first assertion in
# the test would fail on every run, unconditionally -- this is not a missing
# build dependency to work around, it is a test whose entire subject matter
# (the real PDFium backend) does not exist in this port yet. Wiring it up
# would mean registering a CTest that can never pass, so it is left
# unregistered, with this comment as the record of why, per this wave's own
# instruction to defer rather than force it.
#
# --- 2026-09-28: the remaining 8 of vcl's ~50 CppunitTest_*.mk files, all
# re-checked directly (not previously examined in any prior wave) ----------
#
# Only one, vcl_gen, is wireable and is wired below. The other 7:
#   - vcl_filter_ipdf, vcl_pdf_encryption, vcl_pdfexport, vcl_pdfexport2,
#     vcl_pdfexport3: each `$(if $(filter PDFIUM,$(BUILD_TYPE)),pdfium)` --
#     same inert-by-default gate as vcl_pdfium_library_test above -- but
#     re-checked directly against each one's own .cxx source (not assumed
#     from the .mk alone): unlike vcl_graphic_test's own PDFIUM-gated EXTRA
#     source (compiles away cleanly to nothing without it), these sources
#     call PDFium APIs unconditionally and pervasively (grep count of
#     "pdfium" case-insensitive: ipdf.cxx 5, PDFEncryptionTest.cxx 12,
#     pdfexport.cxx 138, pdfexport2.cxx 73, pdfexport3.cxx 128 -- no
#     HAVE_FEATURE_PDFIUM or similar preprocessor guard anywhere in any of
#     the five, confirmed by grepping for `^#if`/`^#ifdef` in each), the
#     exact same "test whose entire subject matter does not exist in this
#     port yet" shape vcl_pdfium_library_test's own comment above already
#     established -- not forced through for the same reason.
#   - vcl_skia: `$(if $(filter SKIA,$(BUILD_TYPE)),skia)` -- SKIA is not in
#     LIBRESTUDIO_BUILD_TYPE by default either (vcl/CMakeLists.txt's own
#     "DESKTOP, plus OPENCL, SKIA, PDFIUM or AFDKO" comment), and skia.cxx
#     itself (re-checked) uses Skia APIs unconditionally throughout (150
#     case-insensitive "skia" hits, no feature guard) -- same shape as the
#     PDFium-gated five above, not forced through either.
#   - vcl_gtk3_a11y: needs $(ATSPI2_CFLAGS)/$(ATSPI2_LIBS) directly (its own
#     set_include/add_libs calls) -- re-checked: `pkg-config --exists
#     atspi-2` fails on this machine (genuinely not installed), a real,
#     environment-specific external-dependency gap, not a design choice --
#     same class of gap as e.g. GPGME/breakpad being off because the
#     library is not present, not because this port decided not to support
#     it. GTK3 itself IS available here (`pkg-config --exists gtk+-3.0`
#     succeeds, and vcl/CMakeLists.txt's own ENABLE_GTK3 auto-detection
#     already builds vclplug_gtk3), so this is specifically the
#     accessibility-bus headers that are missing, not the whole GTK3
#     backend -- left unwired rather than adding a pkg_check_modules(...
#     REQUIRED ...) that would FATAL_ERROR every configure on this machine.
# None of the 7 forced through with an invented mechanism -- each a real,
# reproducible blocker (confirmed by reading the actual .cxx source, not
# just the .mk), not a fabricated one.

# CppunitTest_vcl_app_test.mk: 3 sources (vcl/qa/cppunit/app/
# test_IconThemeInfo, test_IconThemeScanner, test_IconThemeSelector),
# use_libraries sal vcl, use_externals boost_headers, use_sdk_api, use_ure +
# use_vcl -> 3 protector pairs (same precedent as hwpfilter_test_hwpfilter/
# sax_parser/scaddins_analysis), use_rdb(services) -> see this file's header
# comment, use_configuration -> CONFIGURATION.
#
# Fix (first real `cmake --build`, 2026-09-27): the previous wave's own claim
# above ("this is the one test that does NOT add vcl/inc") was wrong --
# re-checked directly against the live .mk (lines 18-21): it DOES call
# gb_CppunitTest_set_include with "-I$(SRCDIR)/vcl/inc" on top of $$(INCLUDE),
# same as every other test in this file. Without it, test_IconThemeScanner.cxx
# / test_IconThemeSelector.cxx (which #include the private IconThemeScanner.hxx
# / IconThemeSelector.hxx from vcl/inc directly) failed with "fatal error: ...
# No such file" -- test_IconThemeInfo.cxx alone does not need it (confirmed:
# it only reaches vcl/inc/IconThemeInfo.hxx, which is a genuinely public
# header already reachable via this port's own global include/ tree).
librestudio_add_cppunit_test(vcl_app_test
    SOURCES
        vcl/qa/cppunit/app/test_IconThemeInfo
        vcl/qa/cppunit/app/test_IconThemeScanner
        vcl/qa/cppunit/app/test_IconThemeSelector
    LIBRARIES sal vcl Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
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

# CppunitTest_vcl_bitmapfilter_test.mk: 1 source (vcl/qa/cppunit/
# BitmapFilterTest), set_include vcl/inc, use_externals boost_headers,
# use_libraries basegfx comphelper cppu cppuhelper sal svt test tl unotest
# vcl utl ("test" -> test_fixtures, this port's established rename), rest
# same shape as vcl_app_test above.
librestudio_add_cppunit_test(vcl_bitmapfilter_test
    SOURCES
        vcl/qa/cppunit/BitmapFilterTest
    LIBRARIES
        basegfx comphelper cppu cppuhelper sal svt test_fixtures tl unotest
        vcl utl Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
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

# CppunitTest_vcl_bitmap_test.mk: 6 sources (vcl/qa/cppunit/BitmapTest,
# bitmapcolor, ScanlineToolsTest, BitmapScaleTest, BmpFilterTest,
# XpmFilterTest), set_include vcl/inc, use_externals boost_headers AND (a
# second, separate use_externals call further down the .mk, after
# use_rdb(services)) harfbuzz -> LIBRESTUDIO_HARFBUZZ_TARGET, the same
# variable vcl/CMakeLists.txt's own fftester/listglyphs targets use rather
# than a fixed target name (see that file's own comment on why). Same
# use_libraries list as vcl_bitmapfilter_test above.
librestudio_add_cppunit_test(vcl_bitmap_test
    SOURCES
        vcl/qa/cppunit/BitmapTest
        vcl/qa/cppunit/bitmapcolor
        vcl/qa/cppunit/ScanlineToolsTest
        vcl/qa/cppunit/BitmapScaleTest
        vcl/qa/cppunit/BmpFilterTest
        vcl/qa/cppunit/XpmFilterTest
    LIBRARIES
        basegfx comphelper cppu cppuhelper sal svt test_fixtures tl unotest
        vcl utl Boost::boost ${LIBRESTUDIO_HARFBUZZ_TARGET}
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
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

# CppunitTest_vcl_filter_igif.mk: 1 source (vcl/qa/cppunit/filter/igif/igif),
# NO set_include call (confirmed: this test does not add vcl/inc, unlike most
# of its neighbours in this file), use_externals boost_headers,
# use_libraries comphelper cppu sal test tl unotest utl vcl, use_sdk_api,
# use_ure + use_vcl, use_rdb(services), use_custom_headers officecfg/registry
# -> OFFICECFG_INCLUDE_DIR + DEPENDS officecfg_headers (same precedent as
# xmlscript_cppunit -- see xmlscript/CppunitTest_deferred.cmake's own
# comment), use_configuration -> CONFIGURATION.
librestudio_add_cppunit_test(vcl_filter_igif
    SOURCES
        vcl/qa/cppunit/filter/igif/igif
    LIBRARIES comphelper cppu sal test_fixtures tl unotest utl vcl Boost::boost
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# CppunitTest_vcl_graphic_test.mk: 6 unconditional sources (vcl/qa/cppunit/
# GraphicTest, GraphicMemoryTest, GraphicDescriptorTest,
# GraphicFormatDetectorTest, GraphicNativeMetadataTest,
# BinaryDataContainerTest) plus one PDFIUM-gated extra source
# (VectorGraphicSearchTest, "$(if $(filter PDFIUM,$(BUILD_TYPE)),...)"),
# set_include vcl/inc, use_externals boost_headers + PDFIUM-gated pdfium,
# plus "ifeq ($(TLS),NSS)" plc4/nss3 (collapsed to the single "nss"
# pkg-config module, same simplification comphelper_test's own crypto
# backend already uses -- see that file's own comment), use_libraries
# comphelper cppu cppuhelper sal svt test tl unotest vcl utl, use_sdk_api,
# use_ure + use_vcl, use_rdb(services), use_configuration.
#
# The PDFIUM-gated source mirrors vcl/CMakeLists.txt's own "PDFIUM" IN_LIST
# LIBRESTUDIO_BUILD_TYPE convention (used there to pick PDFiumLibrary vs.
# DummyPDFiumLibrary) rather than being forced in unconditionally: since
# LIBRESTUDIO_BUILD_TYPE defaults to "DESKTOP" only and external/pdfium has
# no CMakeLists.txt anywhere in this port (see this file's header comment on
# vcl_pdfium_library_test for the full detail), this branch is inert today --
# VectorGraphicSearchTest.cxx is simply not compiled into this target, the
# same outcome upstream's own $(if ...) gate produces on a non-PDFIUM build.
# Unlike vcl_pdfium_library_test, the other six sources here do not depend on
# the real PDFium backend, so the rest of this test target is not blocked by
# any of this.
if ("PDFIUM" IN_LIST LIBRESTUDIO_BUILD_TYPE)
    set(_vcl_graphic_test_pdfium_sources vcl/qa/cppunit/VectorGraphicSearchTest)
else ()
    set(_vcl_graphic_test_pdfium_sources)
endif ()

librestudio_add_cppunit_test(vcl_graphic_test
    SOURCES
        vcl/qa/cppunit/GraphicTest
        vcl/qa/cppunit/GraphicMemoryTest
        vcl/qa/cppunit/GraphicDescriptorTest
        vcl/qa/cppunit/GraphicFormatDetectorTest
        vcl/qa/cppunit/GraphicNativeMetadataTest
        ${_vcl_graphic_test_pdfium_sources}
        vcl/qa/cppunit/BinaryDataContainerTest
    LIBRARIES
        comphelper cppu cppuhelper sal svt test_fixtures tl unotest vcl utl
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
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

unset(_vcl_graphic_test_pdfium_sources)

if (LIBRESTUDIO_TLS_BACKEND STREQUAL "NSS")
    if (NOT TARGET PkgConfig::NSS)
        pkg_check_modules(NSS REQUIRED IMPORTED_TARGET nss)
    endif ()
    target_link_libraries(vcl_graphic_test PRIVATE PkgConfig::NSS)
endif ()

# CppunitTest_vcl_idle_task.mk: 1 source (vcl/qa/cppunit/IdleTask),
# set_include vcl/inc, NO use_externals call (confirmed: no boost_headers
# here, unlike its neighbours in this file), use_libraries comphelper sal
# salhelper vcl, use_sdk_api, use_ure + use_vcl, use_rdb(services),
# use_configuration.
librestudio_add_cppunit_test(vcl_idle_task
    SOURCES
        vcl/qa/cppunit/IdleTask
    LIBRARIES comphelper sal salhelper vcl
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
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

# CppunitTest_vcl_map.mk: 1 source (vcl/qa/cppunit/vclmaptest), set_include
# vcl/inc, use_libraries test tl unotest vcl basegfx, use_externals
# boost_headers, use_sdk_api, use_ure + use_vcl, use_rdb(services),
# use_configuration.
librestudio_add_cppunit_test(vcl_map
    SOURCES
        vcl/qa/cppunit/vclmaptest
    LIBRARIES test_fixtures tl unotest vcl basegfx Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
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

# CppunitTest_vcl_type_serializer_test.mk: 1 source (vcl/qa/cppunit/
# TypeSerializerTest), use_external boost_headers (singular form, same
# effect as use_externals elsewhere in this file), "ifeq ($(TLS),NSS)"
# plc4/nss3 -> PkgConfig::NSS, same as vcl_graphic_test above, set_include
# vcl/inc, use_libraries comphelper cppu cppuhelper sal svt test tl unotest
# vcl, use_sdk_api, use_rdb(services), use_ure + use_vcl,
# use_configuration.
librestudio_add_cppunit_test(vcl_type_serializer_test
    SOURCES
        vcl/qa/cppunit/TypeSerializerTest
    LIBRARIES
        comphelper cppu cppuhelper sal svt test_fixtures tl unotest vcl
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
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

if (LIBRESTUDIO_TLS_BACKEND STREQUAL "NSS")
    if (NOT TARGET PkgConfig::NSS)
        pkg_check_modules(NSS REQUIRED IMPORTED_TARGET nss)
    endif ()
    target_link_libraries(vcl_type_serializer_test PRIVATE PkgConfig::NSS)
endif ()

# =============================================================================
# Second sub-wave: the next 27 of vcl's ~50 CppunitTest_*.mk files (fonts/UI,
# bitmap/graphic/filter, and the rest). Every one of them, re-checked directly
# against its own live .mk file rather than trusted from any prior survey,
# calls BOTH gb_CppunitTest_use_ure(+use_vcl) AND gb_CppunitTest_use_components
# AND gb_CppunitTest_use_configuration -- unlike the first sub-wave above
# (which split 2 "no use_ure at all" direct-in-vcl/CMakeLists.txt tests from 8
# deferred ones), none of these 27 qualify for direct wiring in vcl/
# CMakeLists.txt: they all need the same three protector pairs the tests above
# use (unobootstrapprotector, unoexceptionprotector, vclbootstrapprotector --
# only available once test/ and unotest/ have been add_subdirectory()'d) and
# named UNO_SERVICES components, so every one of them belongs here, in this
# file, for the same reasons already given above.
#
# COMPONENTS lookup keys used below, each re-verified directly against the
# owning module's own librestudio_add_component() call (grep -rn
# "librestudio_add_component(" --include=CMakeLists.txt .), not assumed from
# the gbuild path's own spelling:
#   configmgr/source/configmgr          -> configmgr  (configmgr/CMakeLists.txt)
#   i18npool/util/i18npool              -> i18npool   (i18npool/CMakeLists.txt)
#   ucb/source/core/ucb1                -> ucb1       (ucb/CMakeLists.txt)
#   ucb/source/ucp/file/ucpfile1        -> ucpfile1   (ucb/CMakeLists.txt)
#   ucb/source/ucp/tdoc/ucptdoc1        -> ucptdoc1   (ucb/CMakeLists.txt)
#   framework/util/fwk                  -> fwk        (framework/CMakeLists.txt,
#                                                       not "framework")
#   sfx2/util/sfx                       -> sfx        (sfx2/CMakeLists.txt,
#                                                       not "sfx2")
#   sax/source/expatwrap/expwrap        -> sax        (sax/CMakeLists.txt)
#   unotools/util/utl                   -> utl        (unotools/CMakeLists.txt)
#   unoxml/source/service/unoxml        -> unoxml     (unoxml/CMakeLists.txt)
#   uui/util/uui                        -> uui        (uui/CMakeLists.txt)
#   emfio/emfio                         -> emfio      (emfio/CMakeLists.txt)
#   linguistic/source/lng               -> lng         (linguistic/CMakeLists.txt)
#   comphelper/util/comphelp            -> comphelper (comphelper/CMakeLists.txt)
#   vcl/vcl.common                      -> vcl        (vcl/CMakeLists.txt itself)
# All of the above resolve to real, already add_subdirectory()'d components by
# the time this file's include() point is reached (root CMakeLists.txt line
# ~862, well after every module named above). The two that do NOT resolve --
# filter/source/config/cache/filterconfig1 and filter/source/
# storagefilterdetect/storagefd, both needed only by vcl_apitests below -- are
# explained where vcl_apitests itself would have gone; see that comment.
#
# ICU: three of these tests (vcl_cjk, vcl_textlayout, vcl_text) carry the same
# "ifeq ($(SYSTEM_ICU),TRUE) icuuc else icu_headers" pair every other module's
# identical use_externals trio collapses to -- here just ICU::uc (none of the
# three also asks for icui18n, unlike svl_qa_cppunit's icuuc+icui18n pair; see
# svl/CppunitTest_deferred.cmake's own comment on this exact mapping).
# ICU::uc comes from find_package(ICU ...) at the ROOT CMakeLists.txt's own
# scope (line 129, before any add_subdirectory), so -- like Boost::boost,
# LibXml2::LibXml2 and LIBRESTUDIO_HARFBUZZ_TARGET -- it is directly visible
# here with no guard. LIBRESTUDIO_GRAPHITE2_TARGET, needed only by vcl_font
# below, is the same shape now too: a CACHE INTERNAL variable
# librestudio_pkgconfig() exports (resolved by vcl/CMakeLists.txt's own
# call, near its own top), visible everywhere regardless of
# add_subdirectory() scope -- no guard needed, unlike PkgConfig::NSS above
# (a real directory-scoped IMPORTED_TARGET, see this file's own header
# comment, and cmake/
# LibreStudioTest.cmake's, on why that one still needs one).
#
# Two upstream mechanisms this sub-wave newly encounters have no equivalent in
# this port and are not modelled, each flagged at its one or two call sites
# rather than silently dropped:
#   - gb_CppunitTest_use_uiconfigs(name, vcl) (vcl_treeview, vcl_lifecycle):
#     an order-only dependency on UIConfig_vcl's packaged .ui files.
#     UIConfig_vcl.mk's own packaging (19 .ui files under vcl/uiconfig/ui) is
#     now ported -- see the "--- UIConfig_vcl ---" install(FILES ...) block in
#     vcl/CMakeLists.txt -- but that is an install() rule, not a named build
#     target, so there is still nothing to add as an order-only dependency
#     here to guarantee the .ui files are freshly copied into the install
#     tree before a test runs. Both tests are still wired below -- the .ui
#     files they read exist as plain source-tree files regardless, so this
#     remains a missing *build-order* guarantee, not a missing runtime
#     capability -- wiring that dependency (and the rest of these tests'
#     use_uiconfigs/use_more_fonts/etc. plumbing) is left to the parallel
#     follow-up task that wires the tests depending on this packaging.
#   - gb_CppunitTest_set_non_application_font_use(name, abort|allow)
#     (vcl_font: abort; vcl_cjk: allow): sets the NON_APPLICATION_FONT_USE
#     make variable, which CppunitTest.mk's own __make_args later turns into a
#     SAL_NON_APPLICATION_FONT_USE=abort|deny *environment* variable for the
#     test process. This port's librestudio_add_cppunit_test() has no
#     per-test environment-variable mechanism at all (only EXTRA_ARGS, which
#     appends command-line arguments, not env vars) -- unmodelled, same as
#     gb_CppunitTest_use_more_fonts below.
#
# gb_CppunitTest_use_more_fonts (vcl_fontfeature, vcl_fontmetric, vcl_font,
# vcl_fontvariation, vcl_cjk, vcl_svm_test, vcl_textlayout, vcl_text) is
# inert for every one of them here, not unaddressed: it only does anything
# "ifneq ($(filter MORE_FONTS,$(BUILD_TYPE)),)" (solenv/gbuild/CppunitTest.mk
# line 427), and LIBRESTUDIO_BUILD_TYPE (vcl/CMakeLists.txt) defaults to just
# "DESKTOP" in this port -- MORE_FONTS is never in it -- so upstream's own
# macro would add nothing here either. Nothing to wire.

# --- fonts/UI cluster --------------------------------------------------------

# CppunitTest_vcl_fontcharmap.mk: 1 source (vcl/qa/cppunit/fontcharmap),
# set_include vcl/inc, use_externals boost_headers, use_libraries comphelper
# cppu cppuhelper sal svt test tk tl unotest vcl, use_sdk_api, use_ure +
# use_vcl -> 3 protector pairs, use_components (configmgr, i18npool, ucb1),
# use_configuration.
librestudio_add_cppunit_test(vcl_fontcharmap
    SOURCES
        vcl/qa/cppunit/fontcharmap
    LIBRARIES
        comphelper cppu cppuhelper sal svt test_fixtures tl tk unotest vcl
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1
    CONFIGURATION)

# CppunitTest_vcl_fontfeature.mk: 1 source (vcl/qa/cppunit/FontFeatureTest),
# same set_include/use_externals/use_libraries/use_sdk_api/use_ure+use_vcl/
# use_components/use_configuration shape as vcl_fontcharmap above, plus
# use_more_fonts (inert here -- see this file's header comment).
librestudio_add_cppunit_test(vcl_fontfeature
    SOURCES
        vcl/qa/cppunit/FontFeatureTest
    LIBRARIES
        comphelper cppu cppuhelper sal svt test_fixtures tl tk unotest vcl
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1
    CONFIGURATION)

# CppunitTest_vcl_fontmetric.mk: 1 source (vcl/qa/cppunit/fontmetric), same
# shape as vcl_fontcharmap, plus a second use_externals(harfbuzz) call and
# use_more_fonts (inert).
librestudio_add_cppunit_test(vcl_fontmetric
    SOURCES
        vcl/qa/cppunit/fontmetric
    LIBRARIES
        comphelper cppu cppuhelper sal svt test_fixtures tl tk unotest vcl
        Boost::boost ${LIBRESTUDIO_HARFBUZZ_TARGET}
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1
    CONFIGURATION)

# CppunitTest_vcl_font.mk: 6 sources (font, physicalfontface,
# physicalfontfacecollection, physicalfontfamily, physicalfontcollection,
# logicalfontinstance -- confirmed 6, matching the earlier survey), set_include
# vcl/inc, use_externals(boost_headers harfbuzz graphite), use_libraries
# basegfx comphelper cppu cppuhelper i18nlangtag sal salhelper svt test tl tk
# unotest utl vcl, use_sdk_api, use_ure + use_vcl, use_components (configmgr,
# i18npool, ucb1), use_configuration, use_more_fonts (inert),
# set_non_application_font_use(abort) (not modelled -- see header comment).
librestudio_add_cppunit_test(vcl_font
    SOURCES
        vcl/qa/cppunit/font
        vcl/qa/cppunit/physicalfontface
        vcl/qa/cppunit/physicalfontfacecollection
        vcl/qa/cppunit/physicalfontfamily
        vcl/qa/cppunit/physicalfontcollection
        vcl/qa/cppunit/logicalfontinstance
    LIBRARIES
        basegfx comphelper cppu cppuhelper i18nlangtag sal salhelper svt
        test_fixtures tl tk unotest utl vcl Boost::boost
        ${LIBRESTUDIO_HARFBUZZ_TARGET}
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1
    CONFIGURATION)

target_link_libraries(vcl_font PRIVATE ${LIBRESTUDIO_GRAPHITE2_TARGET})

# CppunitTest_vcl_fontvariation.mk: 1 source (vcl/qa/cppunit/
# FontVariationTest), same shape as vcl_fontcharmap, plus use_more_fonts
# (inert).
librestudio_add_cppunit_test(vcl_fontvariation
    SOURCES
        vcl/qa/cppunit/FontVariationTest
    LIBRARIES
        comphelper cppu cppuhelper sal svt test_fixtures tl tk unotest vcl
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1
    CONFIGURATION)

# CppunitTest_vcl_drawmode.mk: 1 source (vcl/qa/cppunit/drawmode), set_include
# vcl/inc, NO use_externals call anywhere in the file (confirmed -- unlike
# every neighbour above, this one does not link Boost::boost), use_libraries
# comphelper cppu cppuhelper sal svt test tk tl unotest vcl, use_sdk_api,
# use_ure + use_vcl, use_configuration, use_components (configmgr, i18npool,
# ucb1).
librestudio_add_cppunit_test(vcl_drawmode
    SOURCES
        vcl/qa/cppunit/drawmode
    LIBRARIES
        comphelper cppu cppuhelper sal svt test_fixtures tl tk unotest vcl
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1
    CONFIGURATION)

# CppunitTest_vcl_errorhandler.mk: 1 source (vcl/qa/cppunit/errorhandler),
# same shape as vcl_fontcharmap (boost_headers, same use_libraries set, same
# 3 use_components names).
librestudio_add_cppunit_test(vcl_errorhandler
    SOURCES
        vcl/qa/cppunit/errorhandler
    LIBRARIES
        comphelper cppu cppuhelper sal svt test_fixtures tl tk unotest vcl
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1
    CONFIGURATION)

# CppunitTest_vcl_mnemonic.mk: 1 source (vcl/qa/cppunit/mnemonic), same shape
# as vcl_fontcharmap.
librestudio_add_cppunit_test(vcl_mnemonic
    SOURCES
        vcl/qa/cppunit/mnemonic
    LIBRARIES
        comphelper cppu cppuhelper sal svt test_fixtures tl tk unotest vcl
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1
    CONFIGURATION)

# CppunitTest_vcl_treeview.mk: 1 source (vcl/qa/cppunit/treeviewtest),
# set_include vcl/inc, use_externals(boost_headers harfbuzz), use_libraries
# comphelper cppu cppuhelper sal svt test tk tl unotest vcl, use_sdk_api,
# use_ure + use_vcl, use_components (configmgr, fwk, i18npool, sax, sfx, ucb1,
# ucpfile1, utl, unoxml -- 9 names, the widest use_components list of the
# fonts/UI cluster), use_configuration, use_uiconfigs(vcl) (not modelled --
# see this file's header comment).
librestudio_add_cppunit_test(vcl_treeview
    SOURCES
        vcl/qa/cppunit/treeviewtest
    LIBRARIES
        comphelper cppu cppuhelper sal svt test_fixtures tk tl unotest vcl
        Boost::boost ${LIBRESTUDIO_HARFBUZZ_TARGET}
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr fwk i18npool sax sfx ucb1 ucpfile1 utl unoxml
    CONFIGURATION)

# --- bitmap/graphic/filter cluster -------------------------------------------

# CppunitTest_vcl_animation.mk: 2 sources (vcl/qa/cppunit/animationrenderer,
# vcl/qa/cppunit/animation), set_include vcl/inc, use_libraries test tl
# unotest vcl (NO comphelper/cppu/cppuhelper/sal here, confirmed -- the
# smallest use_libraries list in this whole sub-wave), use_externals
# boost_headers, use_sdk_api, use_ure + use_vcl, use_components (configmgr,
# i18npool -- only 2, NO ucb1, confirmed by re-reading the file), use_configuration.
librestudio_add_cppunit_test(vcl_animation
    SOURCES
        vcl/qa/cppunit/animationrenderer
        vcl/qa/cppunit/animation
    LIBRARIES
        test_fixtures tl unotest vcl Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool
    CONFIGURATION)

# CppunitTest_vcl_backend_test.mk: 1 source (vcl/qa/cppunit/BackendTest), NO
# set_include-before-use_ure ordering issue (set_include appears after
# use_ure/use_vcl in the file but that's a gbuild eval-order detail, not a
# CMake one), use_libraries basegfx comphelper cppu cppuhelper sal svt test tl
# unotest vcl, use_sdk_api, use_ure + use_vcl, use_externals(boost_headers
# harfbuzz), use_components (configmgr, i18npool, ucb1), use_configuration.
librestudio_add_cppunit_test(vcl_backend_test
    SOURCES
        vcl/qa/cppunit/BackendTest
    LIBRARIES
        basegfx comphelper cppu cppuhelper sal svt test_fixtures tl unotest
        vcl Boost::boost ${LIBRESTUDIO_HARFBUZZ_TARGET}
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1
    CONFIGURATION)

# CppunitTest_vcl_bitmapprocessor_test.mk: 1 source (vcl/qa/cppunit/
# BitmapProcessorTest), set_include vcl/inc, use_externals boost_headers,
# use_libraries comphelper cppu cppuhelper sal svt test tl unotest vcl utl,
# use_api(udkapi offapi) (same INCLUDE/DEPENDS shape as use_sdk_api), use_ure +
# use_vcl, use_components (configmgr, i18npool, ucb1, utl -- utl appears both
# as a use_libraries entry and a use_components name, which is normal: one is
# the link-time library, the other the runtime UNO service), use_configuration.
librestudio_add_cppunit_test(vcl_bitmapprocessor_test
    SOURCES
        vcl/qa/cppunit/BitmapProcessorTest
    LIBRARIES
        comphelper cppu cppuhelper sal svt test_fixtures tl unotest vcl utl
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1 utl
    CONFIGURATION)

# CppunitTest_vcl_bitmap_render_test.mk: 1 source (vcl/qa/cppunit/bitmaprender/
# BitmapRenderTest), set_include vcl/inc, use_libraries comphelper cppu
# cppuhelper sal svt test tl unotest vcl, use_sdk_api, use_ure + use_vcl,
# use_externals(boost_headers harfbuzz), use_components (configmgr, i18npool,
# ucb1), use_configuration.
librestudio_add_cppunit_test(vcl_bitmap_render_test
    SOURCES
        vcl/qa/cppunit/bitmaprender/BitmapRenderTest
    LIBRARIES
        comphelper cppu cppuhelper sal svt test_fixtures tl unotest vcl
        Boost::boost ${LIBRESTUDIO_HARFBUZZ_TARGET}
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1
    CONFIGURATION)

# CppunitTest_vcl_blocklistparser_test.mk: 1 source (vcl/qa/cppunit/
# blocklistparsertest), set_include vcl/inc, use_externals boost_headers,
# use_libraries sal test unotest vcl (the smallest use_libraries list besides
# vcl_animation's), use_sdk_api, use_ure + use_vcl, use_components (configmgr,
# i18npool, ucb1), use_configuration.
librestudio_add_cppunit_test(vcl_blocklistparser_test
    SOURCES
        vcl/qa/cppunit/blocklistparsertest
    LIBRARIES
        sal test_fixtures unotest vcl Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1
    CONFIGURATION)

# CppunitTest_vcl_cjk.mk: 1 source (vcl/qa/cppunit/cjktext), set_include
# vcl/inc, use_externals(boost_headers harfbuzz) plus the SYSTEM_ICU ifeq ->
# ICU::uc here (see header comment), use_libraries comphelper cppu cppuhelper
# i18nlangtag sal svt test tl unotest vcl, use_sdk_api, use_ure + use_vcl,
# use_components (configmgr, i18npool, ucb1), use_configuration,
# use_more_fonts (inert), set_non_application_font_use(allow) (not modelled).
librestudio_add_cppunit_test(vcl_cjk
    SOURCES
        vcl/qa/cppunit/cjktext
    LIBRARIES
        comphelper cppu cppuhelper i18nlangtag sal svt test_fixtures tl
        unotest vcl Boost::boost ${LIBRESTUDIO_HARFBUZZ_TARGET} ICU::uc
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1
    CONFIGURATION)

# CppunitTest_vcl_filters_test.mk: 14 sources under vcl/qa/cppunit/
# graphicfilter/ (filters-dxf-test, filters-eps-test, filters-jxl-test,
# filters-met-test, filters-pcd-test, filters-pcx-test, filters-pict-test,
# filters-ppm-test, filters-psd-test, filters-ras-test, filters-test,
# filters-tiff-test, filters-tga-test, filters-webp-test -- confirmed 14,
# matching the earlier survey; this is the heaviest single test in this
# module), set_include vcl/inc, use_externals(boost_headers libxml2),
# use_libraries comphelper cppu cppuhelper sal svt test tl unotest vcl emfio
# (emfio is a genuine link-time library here, not just a use_components
# name), use_sdk_api, use_ure + use_vcl, use_components (configmgr, i18npool,
# ucb1, ucpfile1, uui, emfio), use_configuration. DISABLE_CVE_TESTS's
# -DDISABLE_CVE_TESTS define is conditional on $(DISABLE_CVE_TESTS) = TRUE,
# which this port has no CMake option for anywhere (grep confirms no
# precedent) and which configure.ac itself defaults to unset/FALSE -- so, like
# gb_CppunitTest_use_more_fonts elsewhere in this file, upstream's own default
# build already omits it; not wired here either, for the same reason.
librestudio_add_cppunit_test(vcl_filters_test
    SOURCES
        vcl/qa/cppunit/graphicfilter/filters-dxf-test
        vcl/qa/cppunit/graphicfilter/filters-eps-test
        vcl/qa/cppunit/graphicfilter/filters-jxl-test
        vcl/qa/cppunit/graphicfilter/filters-met-test
        vcl/qa/cppunit/graphicfilter/filters-pcd-test
        vcl/qa/cppunit/graphicfilter/filters-pcx-test
        vcl/qa/cppunit/graphicfilter/filters-pict-test
        vcl/qa/cppunit/graphicfilter/filters-ppm-test
        vcl/qa/cppunit/graphicfilter/filters-psd-test
        vcl/qa/cppunit/graphicfilter/filters-ras-test
        vcl/qa/cppunit/graphicfilter/filters-test
        vcl/qa/cppunit/graphicfilter/filters-tiff-test
        vcl/qa/cppunit/graphicfilter/filters-tga-test
        vcl/qa/cppunit/graphicfilter/filters-webp-test
    LIBRARIES
        comphelper cppu cppuhelper sal svt test_fixtures tl unotest vcl emfio
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1 ucpfile1 uui emfio
    CONFIGURATION)

# CppunitTest_vcl_gradient.mk: 1 source (vcl/qa/cppunit/gradient), set_include
# vcl/inc AND vcl/source/window (the extra -I is load-bearing: the test reads
# a header private to that directory), use_externals boost_headers,
# use_libraries basegfx comphelper cppu cppuhelper sal svt test tl unotest
# vcl, use_sdk_api, use_ure + use_vcl, use_components (configmgr, i18npool,
# ucb1), use_configuration.
librestudio_add_cppunit_test(vcl_gradient
    SOURCES
        vcl/qa/cppunit/gradient
    LIBRARIES
        basegfx comphelper cppu cppuhelper sal svt test_fixtures tl unotest
        vcl Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${CMAKE_SOURCE_DIR}/vcl/source/window"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1
    CONFIGURATION)

# CppunitTest_vcl_png_test.mk: 1 source (vcl/qa/cppunit/png/PngFilterTest),
# set_include vcl/inc, use_external(boost_headers) (singular form, same
# effect), use_libraries comphelper cppu cppuhelper sal svt test tl unotest
# vcl utl, plus $(gb_UWINAPI) (empty on this port's Linux-only platform, same
# as every other module that references it -- nothing to add), use_api(udkapi
# offapi), use_ure + use_vcl, use_components (configmgr, i18npool, ucb1, utl),
# use_configuration.
librestudio_add_cppunit_test(vcl_png_test
    SOURCES
        vcl/qa/cppunit/png/PngFilterTest
    LIBRARIES
        comphelper cppu cppuhelper sal svt test_fixtures tl unotest vcl utl
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1 utl
    CONFIGURATION)

# --- remaining cluster --------------------------------------------------------

# CppunitTest_vcl_apitests.mk: RESOLVED. An earlier wave left this unregistered
# because two of its sixteen use_components entries -- filter/source/config/
# cache/filterconfig1 and filter/source/storagefilterdetect/storagefd -- did
# not resolve at the time (filter/CMakeLists.txt then only ported msfilter and
# icg). filter/CMakeLists.txt has since grown real filterconfig and storagefd
# SHARED targets (its own "--- filterconfig ---"/"--- storagefd ---" blocks),
# each registered via its own librestudio_add_component() call -- "filterconfig"
# (filter/CMakeLists.txt, for the filterconfig1.component descriptor -- the
# gbuild use_components spelling "filterconfig1" names the .component file,
# not this port's lookup key, same "path spelling != lookup key" distinction
# this file's own header comment already makes for other components) and
# "storagefd" (spelling matches directly). Re-verified all sixteen keys
# directly against each owning module's own librestudio_add_component() call:
# comphelper, configmgr, filterconfig, storagefd, i18npool, xstor, package2,
# sfx, sot, fsstorage, svt, ucb1, ucpfile1, ucptdoc1, utl, uui, vcl -- all
# resolve now. 1 source (vcl/qa/api/XGraphicTest), use_externals boost_headers,
# use_api(offapi,udkapi) (normal variant), use_ure + use_vcl -> 3 protector
# pairs, use_configuration -> CONFIGURATION. No use_rdb(services) in this .mk
# (explicit COMPONENTS covers it instead), so no program_services_rdb
# dependency needed -- filter (580), uui (710), and every other component-
# owning module above are already add_subdirectory()'d well before this file's
# own include() point (root CMakeLists.txt, after program_services_rdb is
# defined), so ordering was never the blocker for this one, only the missing
# filter targets were.
librestudio_add_cppunit_test(vcl_apitests
    SOURCES
        vcl/qa/api/XGraphicTest
    LIBRARIES
        comphelper cppu cppuhelper sal svt test_fixtures tl unotest vcl
        Boost::boost
    INCLUDE
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        offapi_normal_headers udkapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        comphelper configmgr filterconfig storagefd i18npool xstor package2
        sfx sot fsstorage svt ucb1 ucpfile1 ucptdoc1 utl uui vcl
    CONFIGURATION)

# CppunitTest_vcl_jpeg_read_write_test.mk: 2 sources (vcl/qa/cppunit/jpeg/
# JpegReaderTest, vcl/qa/cppunit/jpeg/JpegWriterTest), use_external(
# boost_headers), set_include vcl/inc, use_libraries comphelper cppu
# cppuhelper sal svt test tl unotest vcl, use_sdk_api, use_ure + use_vcl,
# use_components (configmgr, i18npool, ucb1, ucpfile1, uui), use_configuration.
librestudio_add_cppunit_test(vcl_jpeg_read_write_test
    SOURCES
        vcl/qa/cppunit/jpeg/JpegReaderTest
        vcl/qa/cppunit/jpeg/JpegWriterTest
    LIBRARIES
        comphelper cppu cppuhelper sal svt test_fixtures tl unotest vcl
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1 ucpfile1 uui
    CONFIGURATION)

# CppunitTest_vcl_lifecycle.mk: 1 source (vcl/qa/cppunit/lifecycle),
# set_include vcl/inc, use_externals boost_headers, use_libraries comphelper
# cppu cppuhelper sal svt test tk tl unotest vcl, add_defs(-DVCL_INTERNALS),
# use_sdk_api, use_ure + use_vcl, use_components (configmgr, i18npool, ucb1,
# ucpfile1, fwk, sfx), use_configuration, use_uiconfigs(vcl) (not modelled --
# see this file's header comment).
librestudio_add_cppunit_test(vcl_lifecycle
    SOURCES
        vcl/qa/cppunit/lifecycle
    LIBRARIES
        comphelper cppu cppuhelper sal svt test_fixtures tk tl unotest vcl
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEFINITIONS
        VCL_INTERNALS
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1 ucpfile1 fwk sfx
    CONFIGURATION)

# CppunitTest_vcl_outdev.mk: 1 source (vcl/qa/cppunit/outdev), set_include
# vcl/inc AND vcl/source/window (same extra -I as vcl_gradient above),
# use_externals boost_headers plus a second use_externals(harfbuzz) call,
# use_libraries basegfx comphelper cppu cppuhelper sal svt test tl unotest
# vcl, use_sdk_api, use_ure + use_vcl, use_components (configmgr, i18npool,
# ucb1), use_configuration.
librestudio_add_cppunit_test(vcl_outdev
    SOURCES
        vcl/qa/cppunit/outdev
    LIBRARIES
        basegfx comphelper cppu cppuhelper sal svt test_fixtures tl unotest
        vcl Boost::boost ${LIBRESTUDIO_HARFBUZZ_TARGET}
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${CMAKE_SOURCE_DIR}/vcl/source/window"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1
    CONFIGURATION)

# CppunitTest_vcl_svm_test.mk: 1 source (vcl/qa/cppunit/svm/svmtest),
# use_externals(boost_headers libxml2), set_include vcl/inc, use_libraries
# basegfx comphelper cppu cppuhelper sal salhelper svt test tl unotest vcl
# utl, use_sdk_api, use_ure + use_vcl, use_components (configmgr, i18npool,
# ucb1, utl), use_configuration, use_more_fonts (inert).
librestudio_add_cppunit_test(vcl_svm_test
    SOURCES
        vcl/qa/cppunit/svm/svmtest
    LIBRARIES
        basegfx comphelper cppu cppuhelper sal salhelper svt test_fixtures tl
        unotest vcl utl Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1 utl
    CONFIGURATION)

# CppunitTest_vcl_textlayout.mk: 1 source (vcl/qa/cppunit/textlayout),
# set_include vcl/inc, use_externals(boost_headers harfbuzz) plus the
# SYSTEM_ICU ifeq -> ICU::uc, use_libraries comphelper cppu cppuhelper
# i18nlangtag sal svt test tl unotest vcl, use_sdk_api, use_ure + use_vcl,
# use_components (configmgr, i18npool, ucb1, linguistic/source/lng -> lng),
# use_configuration, use_more_fonts (inert).
librestudio_add_cppunit_test(vcl_textlayout
    SOURCES
        vcl/qa/cppunit/textlayout
    LIBRARIES
        comphelper cppu cppuhelper i18nlangtag sal svt test_fixtures tl
        unotest vcl Boost::boost ${LIBRESTUDIO_HARFBUZZ_TARGET} ICU::uc
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1 lng
    CONFIGURATION)

# CppunitTest_vcl_text.mk: 5 sources (canvasbitmaptest, canvastoolstest,
# complextext, justificationdata, text -- confirmed 5, matching the earlier
# survey), set_include vcl/inc, use_externals(boost_headers harfbuzz) plus the
# SYSTEM_ICU ifeq -> ICU::uc, use_libraries comphelper cppu cppuhelper
# i18nlangtag sal svt test tl unotest vcl, use_sdk_api, use_ure + use_vcl,
# use_components (configmgr, i18npool, ucb1), use_configuration,
# use_more_fonts (inert).
librestudio_add_cppunit_test(vcl_text
    SOURCES
        vcl/qa/cppunit/canvasbitmaptest
        vcl/qa/cppunit/canvastoolstest
        vcl/qa/cppunit/complextext
        vcl/qa/cppunit/justificationdata
        vcl/qa/cppunit/text
    LIBRARIES
        comphelper cppu cppuhelper i18nlangtag sal svt test_fixtures tl
        unotest vcl Boost::boost ${LIBRESTUDIO_HARFBUZZ_TARGET} ICU::uc
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1
    CONFIGURATION)

# CppunitTest_vcl_timer.mk: 1 source (vcl/qa/cppunit/timer), set_include
# vcl/inc, use_externals boost_headers plus a second use_externals(harfbuzz)
# call, use_libraries comphelper cppu cppuhelper sal salhelper test tl unotest
# vcl, use_sdk_api, use_ure + use_vcl, use_components (configmgr, i18npool,
# ucb1), use_configuration.
#
# Module_vcl.mk registers this one differently from every other test in this
# file: it is NOT in the unconditional gb_Module_add_check_targets list those
# 26 neighbours share (Module_vcl.mk lines 225-267) -- it is added separately,
# three times over, each under its own ifeq: "ifeq ($(USING_X11),TRUE)"
# (lines 269-273), "ifeq ($(DISABLE_GUI),TRUE)" (275-279), and, redundantly
# ("Is any configuration missing?"), "ifeq ($(OS),WNT)" (282-286) and
# "ifeq ($(OS),MACOSX)" (288-292) -- i.e. upstream runs it on every platform
# combination it supports, just never unconditionally. In THIS port,
# cmake/LibreStudioPlatform.cmake pins the build to Linux/x86_64 and
# vcl/CMakeLists.txt's own USING_X11 branch is the only one that does not
# FATAL_ERROR (see that file: "Library_vcl.mk has no branch for a build
# without X11" for anything else), so USING_X11 is always TRUE for any build
# that gets this far -- the gate is real (mirrored below with the same
# if (USING_X11) upstream uses) but, on this port's one supported
# configuration, always open.
if (USING_X11)
    librestudio_add_cppunit_test(vcl_timer
        SOURCES
            vcl/qa/cppunit/timer
        LIBRARIES
            comphelper cppu cppuhelper sal salhelper test_fixtures tl unotest
            vcl Boost::boost ${LIBRESTUDIO_HARFBUZZ_TARGET}
        INCLUDE
            "${CMAKE_SOURCE_DIR}/vcl/inc"
            "${udkapi_normal_HEADERS_INCLUDE_DIR}"
            "${offapi_normal_HEADERS_INCLUDE_DIR}"
        DEPENDS
            udkapi_normal_headers offapi_normal_headers
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
            vclbootstrapprotector vclbootstrapprotector
        COMPONENTS
            configmgr i18npool ucb1
        CONFIGURATION)
endif ()

# CppunitTest_vcl_widget_definition_reader_test.mk: 1 source (vcl/qa/cppunit/
# widgetdraw/WidgetDefinitionReaderTest), use_externals boost_headers,
# set_include vcl/inc, use_libraries comphelper cppu cppuhelper sal svt test
# tl unotest vcl utl, use_sdk_api, use_ure + use_vcl, use_components
# (configmgr, i18npool, ucb1, utl), use_configuration.
librestudio_add_cppunit_test(vcl_widget_definition_reader_test
    SOURCES
        vcl/qa/cppunit/widgetdraw/WidgetDefinitionReaderTest
    LIBRARIES
        comphelper cppu cppuhelper sal svt test_fixtures tl unotest vcl utl
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr i18npool ucb1 utl
    CONFIGURATION)

# =============================================================================
# Third sub-wave: vcl_a11y, vcl_font_ttf_structure_test, vcl_unx_generic --
# the three tests left unwired earlier this session specifically because
# subsequenttest (Library_subsequenttest.mk) did not exist as a real CMake
# target yet. test/CMakeLists.txt now has add_library(subsequenttest SHARED
# ...) for real, so all three are re-checked here directly against their own
# live .mk files (not trusted from that earlier survey) and wired below.

# CppunitTest_vcl_a11y.mk: 1 source (vcl/qa/cppunit/a11y/widgetaccessibilitytest),
# NO set_include call (confirmed: unlike most of this file's other entries,
# this one does not add vcl/inc -- the source lives under vcl/qa/cppunit/a11y
# and apparently needs no extra -I), use_libraries sal comphelper cppu
# cppuhelper subsequenttest test vcl ("test" -> test_fixtures, this port's
# established rename), use_api(offapi udkapi) -> use_sdk_api's own
# INCLUDE/DEPENDS shape, use_rdb(services) -> DEPENDS program_services_rdb
# (see this file's header comment: no COMPONENTS, relies on unorc's ini
# fallback), use_ure AND use_vcl -- both confirmed present in the live file
# (lines 33-34: gb_CppunitTest_use_ure(vcl_a11y) followed immediately by
# gb_CppunitTest_use_vcl(vcl_a11y)) -- so this gets the same 3 protector
# pairs as every other use_ure+use_vcl test in this file, not the "no
# protectors" shape some other tests use. use_instdir_configuration AND
# use_common_configuration (NOT the full use_configuration, and NOT
# use_user_ui_configuration) -- the same 2-of-3 pattern every other *_a11y
# CppunitTest in this tree uses (grep confirms sc_a11y, sd_a11y, sw_a11y,
# vcl_gtk3_a11y, test_a11y all call exactly these same two, never the third
# or the composite). librestudio_add_cppunit_test()'s CONFIGURATION option
# is all-or-nothing (see cmake/LibreStudioTest.cmake's own header comment --
# it always joins all three xcsxcu layers into one argument, with no partial
# mode), so this is wired with the bare CONFIGURATION option same as
# everything else here: a harmless over-approximation (this test's
# CONFIGURATION_LAYERS argument additionally carries the registry-user-ui
# layer upstream would not give it), not a functional gap -- an extra,
# unrequested read-only registry layer does not make the test fail.
#
# Module_vcl.mk gates vcl_a11y's own registration behind
# "ifneq ($(ENABLE_ATSPI_TESTS),)" (alongside CppunitTest_vcl_gtk3_a11y,
# which is NOT wired here -- that one is GTK3-specific and out of scope for
# this task). This port has no ENABLE_ATSPI_TESTS equivalent anywhere
# (grep confirms zero hits), so there is nothing to mirror as a CMake
# if()/IN_LIST guard the way PDFIUM/SKIA/MORE_FONTS are elsewhere in this
# file -- but per this session's own prior research, that gate is not a real
# functional blocker for THIS test: widgetaccessibilitytest.cxx exercises
# VCL's UNO accessibility API directly (css::accessibility::XAccessible /
# XAccessibleContext on widget peers), not any AT-SPI/ATK/D-Bus surface --
# no AT-SPI or ATK header is #included anywhere in this test's source, and
# it needs no real display or AT-SPI bus, only VCL's own headless-capable
# accessibility bridge (the same vclbootstrapprotector-backed bootstrap
# every other use_vcl test here already gets). So this is wired
# unconditionally, same posture as every one of this file's non-BUILD_TYPE-
# gated tests.
librestudio_add_cppunit_test(vcl_a11y
    SOURCES
        vcl/qa/cppunit/a11y/widgetaccessibilitytest
    LIBRARIES
        sal comphelper cppu cppuhelper subsequenttest test_fixtures vcl
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

# CppunitTest_vcl_font_ttf_structure_test.mk: 1 source (vcl/qa/cppunit/font/
# TTFStructureTest), set_include vcl/inc, use_externals boost_headers
# libxml2, use_libraries basegfx comphelper cppu cppuhelper sal
# subsequenttest test unotest utl tl vcl xmlsecurity (9 names, "test" ->
# test_fixtures), use_sdk_api, use_ure + use_vcl -> 3 protector pairs,
# use_rdb(services) -> DEPENDS program_services_rdb, use_configuration (the
# FULL composite this time, not the a11y tests' 2-of-3 pattern) ->
# CONFIGURATION, an exact match here.
#
# Module_vcl.mk registers this one completely unconditionally: it sits in
# the same big gb_Module_add_check_targets list (lines 225-267) as
# vcl_map/vcl_font/vcl_treeview/etc. above, not inside any ifneq/ifeq block
# -- confirmed by re-reading that whole block, not just grepping the one
# line. No BUILD_TYPE/OS gate, no PDFIUM dependency (unlike
# vcl_pdfium_library_test, this one needs no external pdfium library at
# all). subsequenttest was this test's only blocker; nothing else found.
librestudio_add_cppunit_test(vcl_font_ttf_structure_test
    SOURCES
        vcl/qa/cppunit/font/TTFStructureTest
    LIBRARIES
        basegfx comphelper cppu cppuhelper sal subsequenttest test_fixtures
        unotest utl tl vcl xmlsecurity Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
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

# CppunitTest_vcl_unx_generic.mk: 1 source (vcl/qa/unx/generic/generic),
# set_include vcl/inc, use_externals boost_headers harfbuzz libxml2,
# use_libraries comphelper cppu cppuhelper sal subsequenttest test tl
# unotest utl vcl (10 names, "test" -> test_fixtures), use_sdk_api, use_ure +
# use_vcl -> 3 protector pairs, use_rdb(services) -> DEPENDS
# program_services_rdb, use_custom_headers(officecfg/registry) ->
# OFFICECFG_INCLUDE_DIR + DEPENDS officecfg_headers (same precedent as
# vcl_filter_igif above), use_configuration (full) -> CONFIGURATION,
# set_non_application_font_use(deny) (not modelled anywhere in this port --
# see this file's header comment: no per-test environment-variable
# mechanism exists, same gap already flagged for vcl_font's abort/vcl_cjk's
# allow), use_more_fonts (inert on its own terms, same as every other
# use_more_fonts call in this file: MORE_FONTS is never in
# LIBRESTUDIO_BUILD_TYPE by default, and there is no more_fonts module/
# Package_extras_font anywhere in this tree).
#
# Module_vcl.mk's own registration of this test (lines 315-320) is a
# DIFFERENT, stronger gate than every other use_more_fonts call surveyed in
# this file: it wraps the entire gb_Module_add_slowcheck_targets call itself
# in "ifneq ($(filter MORE_FONTS,$(BUILD_TYPE)),)" + "ifeq ($(OS),LINUX)" --
# i.e. upstream does not build/register this CppunitTest AT ALL unless the
# MORE_FONTS feature is on, not merely "the use_more_fonts macro call inside
# the .mk is inert either way" (contrast toolkit/CppunitTest_deferred.cmake's
# own note on CppunitTest_toolkit.mk, whose OWN registration is unconditional
# and only the internal use_more_fonts call is inert). The OS=LINUX half is
# trivially satisfied here (this port's cmake/LibreStudioPlatform.cmake pins
# the whole build to Linux/x86_64, and unlike USING_X11 there is no tracked
# CMake variable for "OS" to mirror -- nothing else in this port ever checks
# it either, the same reason vcl_timer's own OS=WNT/OS=MACOSX branches above
# are not mirrored). The MORE_FONTS half is real and IS mirrored below,
# faithfully, the same way vcl_graphic_test/vcl_font mirror "PDFIUM"/"SKIA"
# IN_LIST LIBRESTUDIO_BUILD_TYPE elsewhere in this port:
# LIBRESTUDIO_BUILD_TYPE is a genuine user-settable CACHE STRING
# (vcl/CMakeLists.txt line 581, defaulting to just "DESKTOP"), so
# "MORE_FONTS" IN_LIST LIBRESTUDIO_BUILD_TYPE is a real, correctly-modelled
# condition -- closed by default today (same as PDFIUM/SKIA/OPENCL/AFDKO
# elsewhere in vcl/CMakeLists.txt), but a user who configures with
# -DLIBRESTUDIO_BUILD_TYPE="DESKTOP;MORE_FONTS" gets this test registered,
# matching upstream's own opt-in behaviour exactly. This is therefore wired,
# not left unregistered like vcl_pdfium_library_test/vcl_apitests: nothing
# about vcl_unx_generic's own sources is fundamentally missing or doomed to
# fail the way those two are (no external pdfium/filterconfig1/storagefd
# dependency gap here) -- it is a normal, currently-off opt-in feature gate,
# faithfully mirrored rather than forced open.
if ("MORE_FONTS" IN_LIST LIBRESTUDIO_BUILD_TYPE)
    librestudio_add_cppunit_test(vcl_unx_generic
        SOURCES
            vcl/qa/unx/generic/generic
        LIBRARIES
            comphelper cppu cppuhelper sal subsequenttest test_fixtures tl
            unotest utl vcl Boost::boost ${LIBRESTUDIO_HARFBUZZ_TARGET}
            LibXml2::LibXml2
        INCLUDE
            "${CMAKE_SOURCE_DIR}/vcl/inc"
            "${OFFICECFG_INCLUDE_DIR}"
            "${udkapi_normal_HEADERS_INCLUDE_DIR}"
            "${offapi_normal_HEADERS_INCLUDE_DIR}"
        DEPENDS
            udkapi_normal_headers offapi_normal_headers officecfg_headers
            program_services_rdb
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
            vclbootstrapprotector vclbootstrapprotector
        CONFIGURATION)
endif ()

# --- vcl_dialogs_test: RESOLVED -----------------------------------------------
#
# CppunitTest_vcl_dialogs_test.mk was previously left unwired -- "*_dialogs_
# test screenshot target ... this port does not implement" was this test's
# old place in the plan, the same posture svx/CppunitTest_deferred.cmake's
# own "svx_dialogs_test: NOT wired, here or anywhere" section (and svtools/
# CppunitTest_deferred.cmake's identical note on svtools_dialogs_test) spell
# out in full: librestudio_add_cppunit_test() had no HEADLESS-toggle nor
# use_uiconfigs equivalent at all, and UIConfig_vcl was install()-only, never
# a real build-time target. cmake/LibreStudioTest.cmake's NONHEADLESS and
# UICONFIG options (added this session -- read that file's header comment
# for the full mechanism, including the confirmed finding that this port has
# never forced --headless/SAL_USE_VCLPLUGIN=svp anywhere, so NONHEADLESS only
# has to guarantee the real backend and pin it explicitly, not undo a forced
# setting) close exactly that gap, and vcl/CMakeLists.txt's own UIConfig_vcl
# block now also defines the vcl_uiconfig build-time ALL target UICONFIG
# depends on. This test is wired below for real, no longer deferred pending
# missing infrastructure.
#
# CppunitTest_vcl_dialogs_test.mk, re-checked directly against the live file:
# 1 source (vcl/qa/unit/vcl-dialogs-test), use_external(boost_headers),
# use_sdk_api (normal udkapi+offapi, called twice -- a no-op the second time,
# the same idiom several other tests in this file already tolerate), NO
# set_include call anywhere in the file (confirmed -- unlike most of its
# neighbours, this test adds no extra -I), use_libraries basegfx comphelper
# cppu cppuhelper drawinglayer editeng i18nlangtag i18nutil msfilter oox sal
# salhelper sax sfx sot svl svt test tl tk ucbhelper unotest utl vcl xo (23
# names, "test" -> test_fixtures, this port's established rename, all 23
# re-verified as real add_library() targets before wiring),
# gb_CppunitTest_CppunitScreenShot (the same no-op-for-a-plain-CppunitTest
# macro svx_gallery_test's own comment in svx/CppunitTest_deferred.cmake
# already establishes -- adds nothing beyond what use_vcl_non_headless_
# with_windows itself provides), use_ure + use_vcl_non_headless_with_windows
# -> 3 protector pairs (same triple every other use_ure+use_vcl test in this
# port uses) plus NONHEADLESS, use_rdb(services) -> DEPENDS
# program_services_rdb (no COMPONENTS -- cppunittester's UNO bootstrap falls
# back to unorc's own ini default for UNO_SERVICES, same as every other
# use_rdb(services) test in this port), use_configuration -> CONFIGURATION,
# use_uiconfigs(vcl) -> UICONFIG vcl.
librestudio_add_cppunit_test(vcl_dialogs_test
    SOURCES
        vcl/qa/unit/vcl-dialogs-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
        i18nutil msfilter oox sal salhelper sax sfx sot svl svt test_fixtures
        tl tk ucbhelper unotest utl vcl xo Boost::boost
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
    UICONFIG vcl
    CONFIGURATION)

# --- vcl_gen ----------------------------------------------------------------
#
# CppunitTest_vcl_gen.mk, re-checked directly: 1 source (vcl/qa/cppunit/gen/
# gen), use_libraries (basegfx comphelper cppu cppuhelper sal sfx
# subsequenttest test tl unotest utl vcl -- "test" -> test_fixtures),
# use_externals(boost_headers libxml2), use_sdk_api, use_ure + use_vcl_non_
# headless (NOT the "_with_windows" variant NONHEADLESS elsewhere in this
# port already models -- but re-checked directly against solenv/gbuild/
# CppunitTest.mk: the only difference between the two is that "_with_
# windows" additionally clears the per-test HEADLESS make variable, which
# this port's own cppunittester invocation never sets to "--headless" in
# the first place -- see cmake/LibreStudioTest.cmake's own NONHEADLESS
# header comment for why that half is already a no-op here -- so reusing
# NONHEADLESS produces the identical effective behaviour: a real build-order
# dependency on vclplug_gen plus "-env:SAL_USE_VCLPLUGIN=gen"),
# use_rdb(services) -> DEPENDS program_services_rdb (no COMPONENTS),
# use_configuration -> CONFIGURATION. gen.cxx itself (re-checked) has no
# gen-plugin-specific API calls at all -- it exercises plain VirtualDevice/
# Bitmap drawing through whatever backend SAL_USE_VCLPLUGIN picks, and the
# whole point of the test is forcing that pick to be the real "gen" backend
# rather than the default svp one, exactly what NONHEADLESS already does.
librestudio_add_cppunit_test(vcl_gen
    SOURCES
        vcl/qa/cppunit/gen/gen
    LIBRARIES
        basegfx comphelper cppu cppuhelper sal sfx subsequenttest
        test_fixtures tl unotest utl vcl
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
    NONHEADLESS
    CONFIGURATION)
