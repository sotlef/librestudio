# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sfx2_misc (CppunitTest_sfx2_misc.mk), sfx2_classification (CppunitTest_
# sfx2_classification.mk), sfx2_doc (CppunitTest_sfx2_doc.mk) and
# sfx2_autoredaction (CppunitTest_sfx2_autoredaction.mk) -- split out of
# sfx2/CMakeLists.txt into their own file, include()'d from the root
# CMakeLists.txt only after program_services_rdb exists: all four call
# gb_CppunitTest_use_rdb(name, services) (see scaddins/CppunitTest_deferred.
# cmake's header comment first for the full use_rdb(services)/ini-fallback
# reasoning, which applies identically here -- no COMPONENTS, a DEPENDS on
# program_services_rdb, and cppunittester's UNO bootstrap falling back to
# unorc's own ini default for UNO_SERVICES). All four also need test_fixtures/
# unotest/subsequenttest (test/CMakeLists.txt, unotest/CMakeLists.txt) and,
# via CONFIGURATION, test/CMakeLists.txt's own LIBRESTUDIO_TEST_UNITTEST_DIR
# -- both add_subdirectory()'d AFTER sfx2's own position in the root
# CMakeLists.txt -- but program_services_rdb (defined much later still,
# right after librestudio_finalize_components(services)) is the later-
# binding constraint for all four, so they are registered at that point, the
# same one scaddins_analysis/xmlscript_cppunit/vcl's own eight tests/
# svtools_graphic/svx_unit/svx_gallery_test/svx_core are already deferred to.
#
# sfx2_view (CppunitTest_sfx2_view.mk) is NOT registered here, or anywhere:
# beyond this same subsequenttest/program_services_rdb shape, it also calls
# use_executable(xpdfimport), and Executable_xpdfimport.mk has never been
# ported in this tree (re-confirmed for this wave: grep -rn "add_executable(
# xpdfimport" across the whole tree has zero hits; poppler itself IS
# available per sdext/CMakeLists.txt's own research, but the xpdfimport
# executable that would use it was never written). See sfx2/CMakeLists.txt's
# own comment on this, right after sfx2_controlleritem, for the fuller
# statement -- flagged there rather than here since it was never close
# enough to being wired to need its own block in this file.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not sfx2's, anything sfx2/CMakeLists.txt already built as a real
# CMake target (sfx, sfx2_classification_package, etc.) is still visible
# here -- nothing directory-scoped that sfx2/CMakeLists.txt set up (it
# creates no IMPORTED_TARGET of its own) needs to be re-derived.

# --- sfx2_misc ---------------------------------------------------------------
#
# CppunitTest_sfx2_misc.mk, re-checked directly against the live file: 1
# source (sfx2/qa/cppunit/test_misc), use_sdk_api (normal udkapi+offapi,
# called twice in the .mk -- a no-op repeat), use_libraries (comphelper cppu
# cppuhelper test unotest vcl sal subsequenttest sfx utl tl -- "test" is
# test_fixtures here, same rename every other CppunitTest in this port
# already applies), use_externals(libxml2 boost_headers), use_ure + use_vcl
# (3 protector pairs), use_rdb(services) (see file header above -- no
# COMPONENTS), use_configuration -> CONFIGURATION. No set_include, no
# use_custom_headers.
librestudio_add_cppunit_test(sfx2_misc
    SOURCES
        sfx2/qa/cppunit/test_misc
    LIBRARIES comphelper cppu cppuhelper test_fixtures unotest vcl sal
        subsequenttest sfx utl tl Boost::boost LibXml2::LibXml2
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

# --- sfx2_classification ------------------------------------------------------
#
# CppunitTest_sfx2_classification.mk, re-checked directly against the live
# file: 1 source (sfx2/qa/cppunit/test_classification), use_sdk_api (normal
# udkapi+offapi), use_libraries (comphelper cppu cppuhelper test unotest utl
# tl vcl sal subsequenttest sfx -- "test" -> test_fixtures, same rename),
# use_externals(boost_headers libxml2), use_ure + use_vcl (3 protector
# pairs), use_rdb(services) (see file header above -- no COMPONENTS),
# use_configuration -> CONFIGURATION, use_uiconfigs(svx) and
# use_packages(sfx2_classification).
#
# use_uiconfigs(svx): no CMake equivalent exists in this port at all
# (grepped project-wide -- cmake/LibreStudioTest.cmake's librestudio_add_
# cppunit_test() has no such parameter, and no CppunitTest_deferred.cmake
# anywhere invents one). NOT treated as a blocker here, consistent with
# this port's own established practice: vcl_treeview and vcl_lifecycle
# (vcl/CppunitTest_deferred.cmake) are both wired despite calling
# use_uiconfigs(vcl), which is equally unmodelled -- a bare use_uiconfigs
# call is a documented runtime-only gap (the test may not find its .ui
# files if actually run), not a build/CTest-registration blocker.
#
# use_packages(sfx2_classification): gb_CppunitTest_use_packages's own
# semantics add a build-order DEPENDS on the named Package target -- here,
# upstream's Package_classification.mk, this port's "sfx2_classification_
# package" custom target (sfx2/CMakeLists.txt, renamed this wave from the
# bare "sfx2_classification" specifically to free that literal name for the
# CppunitTest target below -- see that custom target's own comment for the
# full collision this avoids).
librestudio_add_cppunit_test(sfx2_classification
    SOURCES
        sfx2/qa/cppunit/test_classification
    LIBRARIES comphelper cppu cppuhelper test_fixtures unotest utl tl vcl sal
        subsequenttest sfx Boost::boost LibXml2::LibXml2
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
        sfx2_classification_package
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sfx2_doc ------------------------------------------------------------------
#
# CppunitTest_sfx2_doc.mk, re-checked directly against the live file: 1
# source (sfx2/qa/cppunit/doc), use_externals(boost_headers libxml2),
# use_libraries (comphelper cppu cppuhelper sal subsequenttest test unotest
# sfx svl utl tl -- "test" -> test_fixtures, same rename), use_sdk_api
# (normal udkapi+offapi), use_ure + use_vcl (3 protector pairs), use_rdb
# (services) (see file header above -- no COMPONENTS), use_custom_headers
# (officecfg/registry), use_configuration -> CONFIGURATION.
#
# use_custom_headers(officecfg/registry): resolved the same way svx_core's
# identical call already is (svx/CppunitTest_deferred.cmake) -- officecfg/
# CMakeLists.txt's OFFICECFG_INCLUDE_DIR (a CACHE INTERNAL variable, so
# visible here regardless of directory scope) plus a DEPENDS on
# officecfg_headers, both already satisfied well before this include() point
# (officecfg is add_subdirectory()'d at the very top of the root
# CMakeLists.txt).
librestudio_add_cppunit_test(sfx2_doc
    SOURCES
        sfx2/qa/cppunit/doc
    LIBRARIES comphelper cppu cppuhelper sal subsequenttest test_fixtures
        unotest sfx svl utl tl Boost::boost LibXml2::LibXml2
    INCLUDE
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
    CONFIGURATION)

# --- sfx2_dialogs_test (gb_CppunitTest_CppunitScreenShot) ---------------------
#
# CppunitTest_sfx2_dialogs_test.mk, re-checked directly against the live
# file: 1 source (sfx2/qa/unit/sfx2-dialogs-test), use_sdk_api (normal
# udkapi+offapi, called twice -- a no-op repeat), use_libraries basegfx
# comphelper cppu cppuhelper drawinglayer editeng i18nlangtag i18nutil
# msfilter oox sal salhelper sax sfx sot svl svt test tl tk ucbhelper unotest
# utl vcl xo ("test" -> test_fixtures, this port's established rename; "sfx"
# is this module's own production library; all other 21 names are the same
# already-ported targets cui_dialogs_test's own near-identical library list
# already proves out -- basegfx/comphelper/cppu/cppuhelper/drawinglayer/
# editeng/i18nlangtag/i18nutil/msfilter/oox/sal/salhelper/sax/sot/svl/svt/tl/
# tk/ucbhelper/unotest/utl/vcl/xo), use_external(boost_headers) ->
# Boost::boost, use_ure + use_vcl_non_headless_with_windows -> NONHEADLESS +
# 3 protector pairs, use_rdb(services) -> DEPENDS program_services_rdb (no
# COMPONENTS, same ini-fallback reasoning as every other use_rdb(services)
# test in this port), use_configuration -> CONFIGURATION (full three-layer
# form), use_uiconfigs(sfx) -> UICONFIG sfx.
#
# UICONFIG sfx: sfx2/CMakeLists.txt's own "sfx_uiconfig" ALL target (added
# alongside this wiring -- verified directly beforehand that no such target
# existed yet in this port, unlike vcl_uiconfig/svt_uiconfig/svx_uiconfig/
# cui_uiconfig, which each already had one). librestudio_add_cppunit_test()'s
# UICONFIG option resolves the module argument "sfx" to the literal target
# name "sfx_uiconfig" -- matching this UIConfig's own gb_UIConfig_UIConfig
# name "sfx" (not the module directory name "sfx2"), independently confirmed
# by this very .mk's own use_uiconfigs(sfx2_dialogs_test,sfx) call.
librestudio_add_cppunit_test(sfx2_dialogs_test
    SOURCES
        sfx2/qa/unit/sfx2-dialogs-test
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
    UICONFIG sfx
    CONFIGURATION)

# --- sfx2_autoredaction --------------------------------------------------------
#
# CppunitTest_sfx2_autoredaction.mk, re-checked directly against the live
# file: 1 source (sfx2/qa/cppunit/autoredaction), use_externals(boost_headers
# libxml2), use_libraries (comphelper cppu cppuhelper sal subsequenttest test
# unotest sfx svl utl tl svx vcl -- "test" -> test_fixtures, same rename),
# use_sdk_api (normal udkapi+offapi), use_ure + use_vcl (3 protector pairs),
# use_rdb(services) (see file header above -- no COMPONENTS), use_
# configuration -> CONFIGURATION. No use_custom_headers (unlike sfx2_doc/
# sfx2_view -- confirmed by reading the whole file).
#
# svx: svx/CMakeLists.txt's add_subdirectory() (root CMakeLists.txt, before
# unotest/test/program_services_rdb) has already run by this include() point,
# same as every other library this test links.
librestudio_add_cppunit_test(sfx2_autoredaction
    SOURCES
        sfx2/qa/cppunit/autoredaction
    LIBRARIES comphelper cppu cppuhelper sal subsequenttest test_fixtures
        unotest sfx svl utl tl svx vcl Boost::boost LibXml2::LibXml2
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
