# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# desktop's 5 CppunitTest_*.mk files, re-checked directly. desktop_version,
# desktop_app and desktop_lok_init needed no deferral (no use_rdb(services)
# call in any of the three) and are wired directly in desktop/CMakeLists.txt
# instead -- see that file's own per-test comments. desktop_dialogs_test and
# desktop_lib are both deferred here (both need program_services_rdb;
# desktop_lib additionally needs the PACKAGES-resolved wizards/scripting
# targets and swriter_uiconfig, both only add_subdirectory()'d/defined in
# modules positioned after desktop's own -- see desktop_lib's own section
# below for exactly which, and for the one dependency, postprocess_images,
# still not reproduced).

# --- desktop_dialogs_test (CppunitTest_desktop_dialogs_test.mk) --------------
#
# The same *_dialogs_test screenshot-test family as vcl_dialogs_test/
# svtools_dialogs_test/svx_dialogs_test/basctl_dialogs_test/reportdesign_
# dialogs_test/cui_dialogs_test(_2/_3/_4)/filter_dialogs_test. Re-checked
# directly: 1 source (desktop/qa/unit/desktop-dialogs-test), use_sdk_api,
# set_include(-Idesktop/source/inc -Idesktop/inc), use_libraries basegfx
# comphelper cppu cppuhelper drawinglayer editeng i18nlangtag i18nutil
# msfilter oox sal salhelper sax sfx sot svl svt test tl tk ucbhelper unotest
# utl vcl xo ("test" -> test_fixtures, the identical 23-name set basctl_
# dialogs_test/vcl_dialogs_test/svtools_dialogs_test/filter_dialogs_test all
# share), use_external(boost_headers), use_ure + use_vcl_non_headless_with_
# windows -> 3 protector pairs + NONHEADLESS, use_rdb(services) -> DEPENDS
# program_services_rdb (no COMPONENTS -- ini-fallback default, same as every
# other use_rdb(services)-only test), use_configuration -> CONFIGURATION,
# use_uiconfigs(desktop) -> UICONFIG desktop (desktop/CMakeLists.txt's own
# new "desktop_uiconfig" ALL target, registered from UIConfig_deployment.mk
# despite that .mk's filename -- see that file's own comment for the
# filename/registered-name mismatch).
librestudio_add_cppunit_test(desktop_dialogs_test
    SOURCES
        desktop/qa/unit/desktop-dialogs-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng i18nlangtag
        i18nutil msfilter oox sal salhelper sax sfx sot svl svt test_fixtures
        tl tk ucbhelper unotest utl vcl xo Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/desktop/source/inc"
        "${CMAKE_SOURCE_DIR}/desktop/inc"
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
    UICONFIG desktop
    CONFIGURATION)

# --- desktop_lib -------------------------------------------------------------
#
# CppunitTest_desktop_lib.mk, read in full: 1 source (desktop/qa/desktop_lib/
# test_desktop_lib), use_libraries comphelper cppu cppuhelper i18nlangtag sal
# sc scfilt sfx sofficeapp subsequenttest svt sw test unotest utl tl vcl --
# "test" -> test_fixtures, same rename every other CppunitTest in this port
# already applies; the trailing "$(if $(filter -fsanitize=vptr,$(gb_CXX)),
# svl)" is a sanitizer-only extra library this port does not model, skipped
# without further comment, same precedent as sax_xmlimport's own identical
# "$(if $(filter -fsanitize=vptr,..." conditional (see sax/CppunitTest_
# deferred.cmake) -- use_externals(boost_headers cairo libxml2) -> Boost::
# boost, PkgConfig::CAIRO (guarded the same "if (NOT TARGET ...)" way every
# other pkg_check_modules()-based IMPORTED_TARGET already is in this file's
# style, since vcl/CMakeLists.txt's own PkgConfig::CAIRO is directory-scoped
# and not visible here -- same reasoning as PkgConfig::NSS below),
# LibXml2::LibXml2 -- set_include desktop/inc, use_sdk_api (normal), use_ure
# + use_vcl (plain, headless) -> 3 protector pairs, use_rdb(services) ->
# DEPENDS program_services_rdb (bare, no COMPONENTS -- same reasoning every
# other bare use_rdb(services) test in this port already documents),
# use_configuration -> CONFIGURATION, use_more_fonts -> not modelled
# anywhere in this port (same acknowledged gap svgio_read/vcl's *_font tests
# already document -- see e.g. svgio/CppunitTest_deferred.cmake's own
# comment), "ifeq ($(TLS),NSS)" use_externals(plc4 nss3) -> PkgConfig::NSS
# under "if (LIBRESTUDIO_TLS_BACKEND STREQUAL NSS)", same collapse-two-
# gbuild-externals-into-one-pkg-config-module precedent already established
# by vcl_graphic_test/vcl_type_serializer_test (vcl/CppunitTest_deferred.
# cmake's own comment: "plc4/nss3 -> PkgConfig::NSS").
#
# use_packages(desktop_lib, scripting_scriptbindinglib wizards_basicshare
# wizards_basicsrv* [11 more]) -> PACKAGES (cmake/LibreStudioTest.cmake's own
# gb_CppunitTest_use_packages equivalent, added alongside this wiring): all
# 14 names now resolve to real build-time ALL targets -- scripting_
# scriptbindinglib (scripting/CMakeLists.txt's own new "--- Package_
# scriptbindinglib ---" section) and the 13 wizards_basicsrv* + wizards_
# basicshare targets (wizards/CMakeLists.txt's own new librestudio_add_
# package_mirror() calls, one per gb_Package_Package -- see cmake/
# LibreStudioPackage.cmake's header comment for the generic helper this
# reuses). use_uiconfigs(desktop_lib, cui "modules/swriter") -> UICONFIG cui
# swriter (UICONFIG is now a list -- see cmake/LibreStudioTest.cmake's header
# comment for why -- "swriter" being this port's own bare-name convention
# for sw/CMakeLists.txt's new "swriter_uiconfig" target, the same convention
# every other UICONFIG argument in this port already follows, not gbuild's
# own "modules/swriter" path string).
#
# The .mk's SECOND, separate use_packages(desktop_lib, postprocess_images)
# call -- 2026-09-28: postprocess_images is now built (postprocess/
# CMakeLists.txt's own "CustomTarget_images + Package_images: implemented"
# section), so it is included in PACKAGES below alongside the other 14.
librestudio_add_cppunit_test(desktop_lib
    SOURCES
        desktop/qa/desktop_lib/test_desktop_lib
    LIBRARIES
        comphelper cppu cppuhelper i18nlangtag sal sc scfilt sfx sofficeapp
        subsequenttest svt sw test_fixtures unotest utl tl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/desktop/inc"
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
        scripting_scriptbindinglib
        wizards_basicshare
        wizards_basicsrvaccess2base
        wizards_basicsrvdepot
        wizards_basicsrvform
        wizards_basicsrvgimmicks
        wizards_basicsrvimport
        wizards_basicsrvscriptforge
        wizards_basicsrvsfdatabases
        wizards_basicsrvsfdialogs
        wizards_basicsrvsfdocuments
        wizards_basicsrvsfunittests
        wizards_basicsrvsfwidgets
        wizards_basicsrvtemplate
        wizards_basicsrvtools
        postprocess_images
    UICONFIG cui swriter
    CONFIGURATION)

if (NOT TARGET PkgConfig::CAIRO)
    pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
endif ()
target_link_libraries(desktop_lib PRIVATE PkgConfig::CAIRO)

if (LIBRESTUDIO_TLS_BACKEND STREQUAL "NSS")
    if (NOT TARGET PkgConfig::NSS)
        pkg_check_modules(NSS REQUIRED IMPORTED_TARGET nss)
    endif ()
    target_link_libraries(desktop_lib PRIVATE PkgConfig::NSS)
endif ()

# desktop/CppunitTest_deferred.cmake needs data (program_services_rdb) from a
# module add_subdirectory()'d later than this one -- see desktop/
# CMakeLists.txt's own cmake_language(DEFER) call for the scheduling
# mechanism, and cmake/LibreStudioTest.cmake's header comment for why
# CMAKE_SOURCE_DIR (not CMAKE_CURRENT_SOURCE_DIR) is required there.
