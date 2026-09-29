# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# xmlsecurity_pdfsigning (CppunitTest_xmlsecurity_pdfsigning.mk),
# xmlsecurity_xmlsec (CppunitTest_xmlsecurity_xmlsec.mk), xmlsecurity_signing
# (CppunitTest_xmlsecurity_signing.mk), xmlsecurity_signing2 (CppunitTest_
# xmlsecurity_signing2.mk) and xmlsecurity_dialogs_test (CppunitTest_
# xmlsecurity_dialogs_test.mk) -- split out of xmlsecurity/CMakeLists.txt
# into their own file, include()'d from the root CMakeLists.txt only after
# program_services_rdb exists: all five call gb_CppunitTest_use_rdb(name,
# services) with no accompanying gb_CppunitTest_use_components (re-verified
# directly against all five live .mk files) -- see scaddins/CppunitTest_
# deferred.cmake's header comment for the full use_rdb(services)/ini-fallback
# reasoning this implies (no COMPONENTS, just DEPENDS program_services_rdb,
# and cppunittester's UNO bootstrap falling back to unorc's own ini default
# for UNO_SERVICES). program_services_rdb is only defined near the very end
# of the root CMakeLists.txt (right after librestudio_finalize_components
# (services)), well after xmlsecurity's own add_subdirectory() (line 753,
# itself already after officecfg's (390) and unotest's/test's (697/698), so
# CONFIGURATION's own two dependencies -- officecfg_registry and
# LIBRESTUDIO_TEST_UNITTEST_DIR -- are NOT the binding constraint here,
# program_services_rdb is), so all five are registered at that later point,
# the same one scaddins_analysis/toolkit_a11y/svx_unit/svx_gallery_test/
# svx_core/sfx2_misc are already deferred to.
#
# gb_Module_add_check_targets,xmlsecurity,CppunitTest_qa_certext (Module_
# xmlsecurity.mk) is NOT ported: re-verified directly against the live file
# -- the whole $(eval $(call ...)) block is commented out (the "# failing"
# line immediately above it, and every continuation line of that same
# logical statement joined to it by trailing "\", per GNU Make's line-
# continuation rules -- a "#" on the first physical line of a backslash-
# continued group comments out the entire joined logical line, not just that
# one physical line), so qa_certext is never built or run by upstream gbuild
# on this tree either. Matching upstream's own choice, not a gap introduced
# by this port.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not xmlsecurity's, anything xmlsecurity/CMakeLists.txt already built
# as a real CMake target (xmlsecurity, xsec_xmlsec, xmlsec_uiconfig, etc.) is
# still visible here -- but PkgConfig::NSS, the one IMPORTED_TARGET
# xmlsecurity/CMakeLists.txt itself creates (pkg_check_modules(NSS ...
# IMPORTED_TARGET nss), guarded by LIBRESTUDIO_TLS_BACKEND STREQUAL "NSS"),
# is NOT visible here -- same directory-scope wrinkle comphelper/CppunitTest_
# deferred.cmake's own header comment documents, guarded and re-run below
# wherever a test needs it.

# --- xmlsecurity_pdfsigning --------------------------------------------------
#
# CppunitTest_xmlsecurity_pdfsigning.mk, re-checked directly against the live
# file: 1 source (xmlsecurity/qa/unit/pdfsigning/pdfsigning), use_libraries
# (comphelper cppuhelper cppu sal sax sfx svl test tl unotest utl xmlsecurity
# vcl -- "test" -> test_fixtures, this port's established rename), use_
# externals(boost_headers), then `ifneq ($(OS),WNT) ifneq (,$(ENABLE_NSS))`
# -> use_externals(nssutil3 nss3) (Linux-only port, so the WNT half is
# always true/inert; ENABLE_NSS is this port's LIBRESTUDIO_TLS_BACKEND
# STREQUAL "NSS", the same equivalence svl/comphelper/oox/xmlsecurity's own
# production code already establishes -- see the NSS handling below), set_
# include -I$(SRCDIR)/xmlsecurity/inc + $$(INCLUDE) (a no-op re-inclusion of
# whatever use_sdk_api already set, same idiom svx_dialogs_test's own set_
# include already uses), use_sdk_api (normal udkapi+offapi), use_ure + use_
# vcl (3 protector pairs, same precedent as every other use_ure+use_vcl test
# in this port), use_rdb(services) (see file header above), use_
# configuration -> CONFIGURATION. `ifeq ($(ENABLE_POPPLER),TRUE) use_
# executable(...,xpdfimport) endif` is genuinely inert here, not merely
# skipped: this port has no ENABLE_POPPLER flag anywhere (grepped empty
# across every cmake/*.cmake and CMakeLists.txt), so there is no state in
# which this branch would ever fire -- the same "nothing here ever sets it"
# reasoning toolkit_toolkit's own MORE_FONTS comment already applies (see
# toolkit/CppunitTest_deferred.cmake). This is NOT the same situation as
# sfx2_view's unconditional use_executable(xpdfimport) call (sfx2/
# CppunitTest_deferred.cmake's own header comment), which is a real,
# unavoidable blocker because that .mk's call has no surrounding
# conditional -- xmlsecurity_pdfsigning's call does, and the condition never
# holds in this port, so nothing needed skipping. The `ifeq ($(OS),WNT)`
# gpgmepp use_packages block is dropped without further comment, same as
# every other WNT-only branch elsewhere in this Linux-only port.
librestudio_add_cppunit_test(xmlsecurity_pdfsigning
    SOURCES
        xmlsecurity/qa/unit/pdfsigning/pdfsigning
    LIBRARIES
        comphelper cppuhelper cppu sal sax sfx svl test_fixtures tl unotest
        utl xmlsecurity vcl Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/xmlsecurity/inc"
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

# ifneq ($(OS),WNT) ifneq (,$(ENABLE_NSS)): nssutil3 + nss3 collapse to the
# single "nss" pkg-config module, the same simplification xmlsecurity/
# CMakeLists.txt's own production targets already use for their NSS need.
# ENABLE_NSS's equivalent here is LIBRESTUDIO_TLS_BACKEND STREQUAL "NSS" (not
# XMLSECURITY_HAS_NSS_BACKEND, which additionally requires xmlsec1-nss.pc to
# be present -- pdfsigning.cxx's own `#if USE_CRYPTO_NSS` guard, config_
# crypto.h's macro, follows LIBRESTUDIO_TLS_BACKEND alone, matching upstream's
# ENABLE_NSS gate exactly, not the stricter xmlsec1-nss-specific one).
if (LIBRESTUDIO_TLS_BACKEND STREQUAL "NSS")
    if (NOT TARGET PkgConfig::NSS)
        pkg_check_modules(NSS REQUIRED IMPORTED_TARGET nss)
    endif ()
    target_link_libraries(xmlsecurity_pdfsigning PRIVATE PkgConfig::NSS)
endif ()

# --- xmlsecurity_xmlsec -------------------------------------------------------
#
# CppunitTest_xmlsecurity_xmlsec.mk, re-checked directly against the live
# file: 1 source (xmlsecurity/qa/xmlsec/xmlsec), use_externals(boost_headers
# libxml2), NSS externals under the same ifneq($(OS),WNT)/ifneq(,$(ENABLE_
# NSS)) gate as pdfsigning above, use_libraries (comphelper cppu cppuhelper
# embobj sal sfx subsequenttest test tl unotest utl xmlsecurity xsec_xmlsec
# -- "test" -> test_fixtures; note "vcl" is genuinely absent from this list,
# re-verified directly: gb_CppunitTest_use_vcl (solenv/gbuild/CppunitTest.mk,
# gb_CppunitTest__use_vcl) only adds an order-only dependency on
# vclbootstrapprotector/vclplug_gen, it does NOT gb_CppunitTest_use_library
# "vcl" itself -- so not listing vcl explicitly here matches upstream, it is
# not a fidelity gap), set_include -I$(SRCDIR)/xmlsecurity/inc + $$(INCLUDE)
# (needed for real this time: xmlsec.cxx #includes "documentsignaturemanager.
# hxx", a private xmlsecurity/inc header), use_sdk_api (normal udkapi+
# offapi), use_ure + use_vcl (3 protector pairs), use_rdb(services) (see file
# header above), use_custom_headers(officecfg/registry) -> OFFICECFG_
# INCLUDE_DIR + DEPENDS officecfg_headers (same precedent as svx_core's own
# use_custom_headers call), use_configuration -> CONFIGURATION, then
# gb_CppunitTest_add_arguments with one bespoke "-env:arg-env=$(gb_Helper_
# LIBRARY_PATH_VAR)..." argument -- the exact same idiom already solved for
# smoketest and reused verbatim by svx_unit (see svx/CppunitTest_deferred.
# cmake's own comment for the full reasoning: a CMake-configure-time
# snapshot of LD_LIBRARY_PATH, omitted entirely if unset/empty at configure
# time, since there is no CMake equivalent of make's own conditional
# shell-expansion trick inside a plain add_test() COMMAND list).
set(XMLSECURITY_LIBRARY_PATH_EXTRA_ARGS "")
if (DEFINED ENV{LD_LIBRARY_PATH} AND NOT "$ENV{LD_LIBRARY_PATH}" STREQUAL "")
    list(APPEND XMLSECURITY_LIBRARY_PATH_EXTRA_ARGS
        "-env:arg-env=LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}")
endif ()

librestudio_add_cppunit_test(xmlsecurity_xmlsec
    SOURCES
        xmlsecurity/qa/xmlsec/xmlsec
    LIBRARIES
        comphelper cppu cppuhelper embobj sal sfx subsequenttest test_fixtures
        tl unotest utl xmlsecurity xsec_xmlsec Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/xmlsecurity/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION
    EXTRA_ARGS ${XMLSECURITY_LIBRARY_PATH_EXTRA_ARGS})

if (LIBRESTUDIO_TLS_BACKEND STREQUAL "NSS")
    if (NOT TARGET PkgConfig::NSS)
        pkg_check_modules(NSS REQUIRED IMPORTED_TARGET nss)
    endif ()
    target_link_libraries(xmlsecurity_xmlsec PRIVATE PkgConfig::NSS)
endif ()

# --- xmlsecurity_signing / xmlsecurity_signing2 -----------------------------
#
# CppunitTest_xmlsecurity_signing.mk and CppunitTest_xmlsecurity_signing2.mk,
# re-checked directly against both live files -- they are near-duplicates
# (signing2 is signing's own successor test file, per upstream's usual
# split-a-growing-test-in-two pattern) but genuinely differ in use_libraries:
# signing lists svl and ucbhelper, signing2 lists neither (re-verified twice
# against both files to make sure this was not a transcription slip -- it is
# upstream's own difference, not an inconsistency to "fix").
#
# Both: use_externals(boost_headers libxml2), NSS externals under the same
# ifneq($(OS),WNT)/ifneq(,$(ENABLE_NSS)) gate as pdfsigning/xmlsec above
# (both signing.cxx and signing2.cxx directly #include <nss.h> and call
# NSS_SetAlgorithmPolicy/NSS_OptionSet themselves, guarded by the same `#if
# USE_CRYPTO_NSS` config_crypto.h macro), set_include -I$(SRCDIR)/
# xmlsecurity/inc + $$(INCLUDE) (needed: both sources #include
# "documentsignaturemanager.hxx"), use_sdk_api (normal udkapi+offapi),
# use_ure + use_vcl (3 protector pairs), use_uiconfigs(svt) -> UICONFIG svt
# (svtools/CMakeLists.txt's own svt_uiconfig build-time mirror, already
# defined -- svtools is add_subdirectory()'d at line 498, well before this
# file's own include() point), use_rdb(services) (see file header above),
# use_configuration -> CONFIGURATION, `ifeq ($(ENABLE_POPPLER),TRUE) use_
# executable(...,xpdfimport) endif` (genuinely inert, same reasoning as
# xmlsecurity_pdfsigning above -- this port has no ENABLE_POPPLER flag at
# all), the Linux-only "reset LD_LIBRARY_PATH for spawned GPG processes"
# hack (EXTRA_ENV_VARS += LIBO_LD_PATH=$$LD_LIBRARY_PATH, gb_CppunitTest_
# get_target's own per-target make-recipe environment mechanism -- this is a
# real OS environment variable set on the test process itself, not a
# "-env:" UNO bootstrap argument, so it does NOT go through EXTRA_ARGS; it
# is set below via set_tests_properties(... PROPERTIES ENVIRONMENT ...)
# instead, using the same configure-time LD_LIBRARY_PATH snapshot as
# xmlsecurity_xmlsec's EXTRA_ARGS above -- upstream's own comment ("various
# hacks to make unit test work on Linux more often") already frames this as
# best-effort, so a configure-time snapshot rather than reproducing make's
# exact recipe-time value is an accepted simplification, not a silent gap),
# use_custom_headers(officecfg/registry) -> OFFICECFG_INCLUDE_DIR + DEPENDS
# officecfg_headers. WNT gpgmepp use_packages dropped, same as pdfsigning
# above.
librestudio_add_cppunit_test(xmlsecurity_signing
    SOURCES
        xmlsecurity/qa/unit/signing/signing
    LIBRARIES
        comphelper cppuhelper cppu sal sax sfx svl svx subsequenttest
        test_fixtures tl ucbhelper unotest utl vcl xmlsecurity xsec_xmlsec
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/xmlsecurity/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG svt
    CONFIGURATION)

if (LIBRESTUDIO_TLS_BACKEND STREQUAL "NSS")
    if (NOT TARGET PkgConfig::NSS)
        pkg_check_modules(NSS REQUIRED IMPORTED_TARGET nss)
    endif ()
    target_link_libraries(xmlsecurity_signing PRIVATE PkgConfig::NSS)
endif ()

if (DEFINED ENV{LD_LIBRARY_PATH} AND NOT "$ENV{LD_LIBRARY_PATH}" STREQUAL "")
    set_tests_properties(xmlsecurity_signing PROPERTIES
        ENVIRONMENT "LIBO_LD_PATH=$ENV{LD_LIBRARY_PATH}")
endif ()

librestudio_add_cppunit_test(xmlsecurity_signing2
    SOURCES
        xmlsecurity/qa/unit/signing/signing2
    LIBRARIES
        comphelper cppuhelper cppu sal sax sfx svx subsequenttest test_fixtures
        tl unotest utl vcl xmlsecurity xsec_xmlsec Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/xmlsecurity/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG svt
    CONFIGURATION)

if (LIBRESTUDIO_TLS_BACKEND STREQUAL "NSS")
    if (NOT TARGET PkgConfig::NSS)
        pkg_check_modules(NSS REQUIRED IMPORTED_TARGET nss)
    endif ()
    target_link_libraries(xmlsecurity_signing2 PRIVATE PkgConfig::NSS)
endif ()

if (DEFINED ENV{LD_LIBRARY_PATH} AND NOT "$ENV{LD_LIBRARY_PATH}" STREQUAL "")
    set_tests_properties(xmlsecurity_signing2 PROPERTIES
        ENVIRONMENT "LIBO_LD_PATH=$ENV{LD_LIBRARY_PATH}")
endif ()

# --- xmlsecurity_dialogs_test ------------------------------------------------
#
# CppunitTest_xmlsecurity_dialogs_test.mk, re-checked directly against the
# live file: gb_CppunitTest_CppunitScreenShot (adds nothing beyond plain
# gb_CppunitTest_CppunitTest by itself -- see svx_gallery_test's own comment
# in svx/CppunitTest_deferred.cmake for why), 1 source (xmlsecurity/qa/unit/
# xmlsecurity-dialogs-test), use_sdk_api (normal udkapi+offapi, called
# twice -- a no-op repeat), use_libraries (basegfx comphelper cppu cppuhelper
# drawinglayer editeng i18nlangtag i18nutil msfilter oox sal salhelper sax
# sfx sot svl svt test tl tk ucbhelper unotest utl vcl xo -- "test" -> test_
# fixtures), use_external(boost_headers), use_ure + use_vcl_non_headless_
# with_windows -> 3 protector pairs + NONHEADLESS (real on-screen windowing,
# same precedent as svx_dialogs_test/vcl_dialogs_test/svtools_dialogs_test),
# use_rdb(services) (see file header above -- no COMPONENTS), use_
# configuration -> CONFIGURATION, use_uiconfigs(xmlsec) -> UICONFIG xmlsec.
#
# No set_include call in this .mk (unlike pdfsigning/xmlsec/signing/
# signing2) -- confirmed against xmlsecurity-dialogs-test.cxx's own #include
# list (sal/config.h, test/screenshot_test.hxx, vcl/abstdlg.hxx only): it
# needs no xmlsecurity/inc private header, so INCLUDE below omits it, and no
# use_custom_headers(officecfg/registry) call either, so no OFFICECFG_
# INCLUDE_DIR/officecfg_headers dependency. No NSS externals either --
# re-verified: this source has no NSS reference at all.
#
# UICONFIG xmlsec needs xmlsecurity/CMakeLists.txt's own UIConfig_xmlsec
# build-time mirror (the "xmlsec_uiconfig" ALL target) -- this did NOT exist
# before this change (xmlsecurity/CMakeLists.txt's own "What is not ported"
# comment previously listed "UIConfig_xmlsec (.ui packaging)" as out of
# scope); it has now been added there (see that file's own "--- UIConfig_
# xmlsec ---" block, modeled on vcl/CMakeLists.txt's "--- UIConfig_vcl ---"
# block and svx/svtools' own svx_uiconfig/svt_uiconfig equivalents), with the
# 9 real .ui filenames transcribed from the live UIConfig_xmlsec.mk (matches
# `find xmlsecurity/uiconfig/ui -name "*.ui" | wc -l` == 9 exactly).
librestudio_add_cppunit_test(xmlsecurity_dialogs_test
    SOURCES
        xmlsecurity/qa/unit/xmlsecurity-dialogs-test
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
    UICONFIG xmlsec
    CONFIGURATION)
