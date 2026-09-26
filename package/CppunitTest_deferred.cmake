# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# package2_test (CppunitTest_package2_test.mk) -- split out of package/
# CMakeLists.txt into its own file, include()'d from the root CMakeLists.txt
# well after ucb has had its own add_subdirectory() (drawinglayer/
# CppunitTest_deferred.cmake's own include() call site sits at the same
# point, for the same reason -- see that file's header comment for the full
# use_components/GLOBAL-property mechanism, not repeated here). package's own
# add_subdirectory() position (line 493) sits well before ucb's (601), and
# this test's own use_components list names two of ucb's components
# (ucb/source/core/ucb1, ucb/source/ucp/file/ucpfile1), so it cannot be
# registered at package/CMakeLists.txt's own position -- ucb is the binding
# constraint here, not configmgr/sax/package's own components, all of which
# are already available at package's own position.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not package's, source paths below are still fine: librestudio_expand_cxx
# (cmake/LibreStudioPlatform.cmake) resolves them against CMAKE_SOURCE_DIR
# (the absolute tree root), never CMAKE_CURRENT_SOURCE_DIR, so it does not
# matter which directory's scope this file executes in.
#
# CppunitTest_package2_test.mk, re-checked directly against the live file: 2
# sources (package/qa/cppunit/test_package, package/qa/cppunit/
# test_zippackage), use_libraries comphelper cppu cppuhelper sal tl unotest,
# use_sdk_api (udkapi+offapi "normal" variant), use_components (6 names,
# below), use_ure -> 2 protector pairs (unobootstrapprotector/
# unoexceptionprotector -- no use_vcl in this .mk, confirmed by reading the
# whole file, so no vclbootstrapprotector pair), use_configuration ->
# CONFIGURATION.
#
# use_components lookup keys, each verified against the owning module's own
# librestudio_add_component() call rather than assumed from the .mk's own
# gbuild-path spelling (grep -n "librestudio_add_component("):
#   - "configmgr/source/configmgr" -> "configmgr" (configmgr/CMakeLists.txt).
#   - "package/source/xstor/xstor" -> "xstor" (package/CMakeLists.txt, this
#     same file, above).
#   - "package/util/package2" -> "package2" (package/CMakeLists.txt, this
#     same file, above).
#   - "sax/source/expatwrap/expwrap" -> "sax" (sax/CMakeLists.txt --
#     registers its component under the plain library name "sax" even though
#     the .component file itself is expwrap.component; same mapping sax_
#     xmlimport already relies on, see sax/CMakeLists.txt's own comment on
#     this exact point).
#   - "ucb/source/core/ucb1" -> "ucb1" and "ucb/source/ucp/file/ucpfile1" ->
#     "ucpfile1" (both ucb/CMakeLists.txt).
# All six resolve to real, already-registered components -- nothing here
# needed to be flagged as unresolved.
#
# All libraries verified as real targets before wiring (grep -n
# "add_library(<name> "): comphelper (comphelper/CMakeLists.txt), cppu (cppu/
# CMakeLists.txt), cppuhelper (cppuhelper/CMakeLists.txt), sal (sal/
# CMakeLists.txt), tl (tools/CMakeLists.txt), unotest (unotest/CMakeLists.txt).
# unobootstrapprotector/unoexceptionprotector (unotest/CMakeLists.txt) both
# resolve too, same protector targets svl_urihelper/svl_qa_cppunit already
# use.
librestudio_add_cppunit_test(package2_test
    SOURCES
        package/qa/cppunit/test_package
        package/qa/cppunit/test_zippackage
    LIBRARIES
        comphelper cppu cppuhelper sal tl unotest
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
    COMPONENTS
        configmgr xstor package2 sax ucb1 ucpfile1
    CONFIGURATION)
