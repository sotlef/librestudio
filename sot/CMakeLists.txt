# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sot — the storage layer: the compound-document (OLE2) and ODF package
# containers, and the UNO services that expose them.  This is the first module
# that is about documents rather than about UNO itself.
#
# Source list from Library_sot.mk (the precompiled header is not ported).
# Note that the .mk links utl without declaring it in any way that makes the
# dependency obvious; libsot genuinely needs unotools, so it is listed below.

set(SOT_SOURCES
    sot/source/unoolestorage/xolesimplestorage
    sot/source/base/formats
    sot/source/base/object
    sot/source/base/exchange
    sot/source/base/filelist
    sot/source/sdstor/stg
    sot/source/sdstor/stgavl
    sot/source/sdstor/stgcache
    sot/source/sdstor/stgdir
    sot/source/sdstor/stgelem
    sot/source/sdstor/stgio
    sot/source/sdstor/stgole
    sot/source/sdstor/stgstrms
    sot/source/sdstor/storage
    sot/source/sdstor/storinfo
    sot/source/sdstor/ucbstorage)

librestudio_expand_cxx(SOT_SOURCES)

add_library(sot SHARED ${SOT_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(sot PRIVATE
        "${CMAKE_SOURCE_DIR}/sot/inc/pch/precompiled_sot.hxx")
    target_compile_definitions(sot PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(sot PRIVATE
    "${CMAKE_SOURCE_DIR}/sot/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(sot udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(sot PRIVATE SOT_DLLIMPLEMENTATION)

target_link_libraries(sot PRIVATE
    comphelper cppu cppuhelper i18nlangtag sal salhelper tl ucbhelper utl)

librestudio_add_component(sot
    COMPONENT "${CMAKE_SOURCE_DIR}/sot/util/sot.component"
    LIBRARY   sot
    LAYER     OOO)

librestudio_set_library_output_name(sot sot OOO)

install(TARGETS sot LIBRARY DESTINATION program)

# --- storage check -----------------------------------------------------------

# Reads the real OLE2 files under sot/qa/cppunit/data through sot.  Upstream's
# own test_sot.cxx cannot be used yet: it builds on unotest's fixtures, and
# unotest links vcl.
#
# EXCLUDE_FROM_ALL where there is no popen(): the check shells out to compare
# against a reference dump.
if (WIN32)
    set(_sot_check_all EXCLUDE_FROM_ALL)
else ()
    set(_sot_check_all "")
endif ()
add_executable(sot_storage_check ${_sot_check_all} qa/cppunit/sot_storage_check.cxx)

target_include_directories(sot_storage_check PRIVATE
    "${CMAKE_SOURCE_DIR}/sot/inc"
    "${CMAKE_SOURCE_DIR}/tools/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(sot_storage_check udkapi_normal_headers offapi_normal_headers)

# tl as well as sot: the check uses SvFileStream from tools/stream.hxx directly.
# comphelper is where the process service factory lives — the check publishes
# the context through it, because sot reaches for that context rather than
# taking one as an argument.  cppu and cppuhelper own the reference-counting and
# exception types the check touches.  -Wl,-z,defs requires all of them to be
# named rather than inherited.
target_link_libraries(sot_storage_check PRIVATE
    sot tl comphelper cppu cppuhelper sal salhelper)

# Why this needs i18npool, and why that is not a build detail:
#
# Opening an OLE2 file walks its directory stream, and StgEntry::Load upper-cases
# each entry name (sot/source/sdstor/stgelem.cxx:311) through a CharClass, which
# asks the component context for com.sun.star.i18n.CharacterClassification — a
# service i18npool provides.  Without it the first file open throws
# DeploymentException.  Upstream says the same thing in its own way:
# CppunitTest_sot_test_sot.mk asks for
# gb_CppunitTest_use_components(sot_test_sot, i18npool/util/i18npool).
#
# Nothing links i18npool: it has to be reachable through services.rdb and unorc
# instead, which is why the registries are named here — including the office API
# types, which is where com.sun.star.i18n lives and which are in
# types/offapi.rdb rather than in types.rdb.
if (BUILD_TESTING)
    add_dependencies(sot_storage_check
        program_services_rdb program_types_rdb program_offapi_rdb gcc3_uno officecfg_registry)

    add_test(NAME sot_storage_check
        COMMAND sot_storage_check "${CMAKE_SOURCE_DIR}/sot/qa/cppunit/data")
endif ()

# --- CppunitTest_sot_test_sot -----------------------------------------------
#
# CppunitTest_sot_test_sot.mk, re-checked directly against the live file: 1
# source (sot/qa/cppunit/test_sot), use_libraries(comphelper cppu cppuhelper
# sal sot tl unotest -- "test" is NOT in this list, unlike many other
# CppunitTests: qa/cppunit/test_sot.cxx uses unotest/bootstrapfixturebase.hxx
# (test::BootstrapFixtureBase), not test/bootstrapfixture.hxx
# (test::BootstrapFixture) -- the latter is what pulls in the "test" library
# (test_fixtures here) and vcl; this test needs neither), set_include(sot/inc,
# $$(INCLUDE)), use_sdk_api (normal udkapi+offapi, called with a trailing
# empty second argument), use_ure (2 protector pairs: unobootstrapprotector,
# unoexceptionprotector -- no use_vcl, so no vclbootstrapprotector and no
# NONHEADLESS-style backend dependency), use_components(i18npool/util/
# i18npool). No use_configuration anywhere in this .mk, so no CONFIGURATION
# option and, in turn, no LIBRESTUDIO_TEST_UNITTEST_DIR dependency -- the one
# thing that would have forced deferral past sot's own natural position (see
# canvas/CppunitTest_deferred.cmake for a test that does hit that).
#
# The old "unotest links vcl" comment above (on sot_storage_check) explains
# why upstream's own test_sot.cxx could not be used when this file was first
# written -- true of unotest's production block (unotest/CMakeLists.txt links
# vcl unconditionally), but irrelevant to *this* test's own use_libraries list
# (which never names vcl, directly or via "test"/test_fixtures) -- so that
# past blocker does not apply to sot_test_sot itself.
#
# Ordering, re-verified (grep -n "add_subdirectory(" CMakeLists.txt):
#   - i18npool/util/i18npool -> "i18npool" (i18npool/CMakeLists.txt's own
#     librestudio_add_component() call) is add_subdirectory()'d well before
#     sot's own position -- the GLOBAL property COMPONENTS reads is already
#     set here.
#   - unobootstrapprotector/unoexceptionprotector (unotest/CMakeLists.txt)
#     are add_subdirectory()'d AFTER sot's own position, but PROTECTORS is
#     wired purely through "$<TARGET_FILE:...>" generator expressions inside
#     add_test()'s COMMAND (cmake/LibreStudioTest.cmake) -- resolved at
#     CMake's generate step, once every directory's CMakeLists.txt has already
#     run, not at the point this call is textually reached -- and
#     target_link_libraries(unotest ...) below is the same kind of
#     generate-time-resolved forward reference sal/CMakeLists.txt's own
#     sal_osl already established works (see that file's comment on linking a
#     not-yet-defined target). So, unlike CONFIGURATION's LIBRESTUDIO_TEST_
#     UNITTEST_DIR (a plain CMake variable, read immediately) or COMPONENTS
#     (a GLOBAL property, also read immediately via get_property()), neither
#     PROTECTORS nor a forward LIBRARIES reference forces deferral here --
#     confirmed for real by reconfiguring after this call was added, not just
#     reasoned about.
librestudio_add_cppunit_test(sot_test_sot
    SOURCES
        sot/qa/cppunit/test_sot
    LIBRARIES
        comphelper cppu cppuhelper sal sot tl unotest
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sot/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
    COMPONENTS
        i18npool)

# qa/cppunit/test_sot.cxx's SotTest derives from test::BootstrapFixtureBase,
# whose Directories member needs SRC_ROOT/WORKDIR_FOR_BUILD -- this is now
# handled generically by librestudio_add_cppunit_test() itself (cmake/
# LibreStudioTest.cmake's own header comment on its ENVIRONMENT property has
# the full history of this gap and why the fix lives there, not per-test).
