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
# The check is built but deliberately not registered with CTest, because it
# cannot pass yet — see the guard below.  Building it keeps it honest: the day
# i18npool is ported, the guard flips and it runs.
add_executable(sot_storage_check qa/cppunit/sot_storage_check.cxx)

target_include_directories(sot_storage_check PRIVATE
    "${CMAKE_SOURCE_DIR}/sot/inc"
    "${CMAKE_SOURCE_DIR}/tools/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(sot_storage_check udkapi_normal_headers offapi_normal_headers)

# tl as well as sot: the check uses SvFileStream from tools/stream.hxx directly.
# The full set of direct dependencies: cppu and cppuhelper own the reference-
# counting and exception types the check touches, and -Wl,-z,defs requires them
# to be named rather than inherited.
target_link_libraries(sot_storage_check PRIVATE sot tl cppu cppuhelper sal salhelper)

# Why this needs i18npool, and why that is not a build detail:
#
# Opening an OLE2 file walks its directory stream, and StgEntry::Load upper-cases
# each entry name (sot/source/sdstor/stgelem.cxx:311) through a CharClass, which
# asks the component context for com.sun.star.i18n.CharacterClassification — a
# service i18npool provides.  Without it the first file open throws
# DeploymentException, which is where this check currently stops.  Upstream says
# the same thing in its own way: CppunitTest_sot_test_sot.mk asks for
# gb_CppunitTest_use_components(sot_test_sot, i18npool/util/i18npool).
#
# So porting i18npool is a prerequisite for reading documents at all, not just
# for this test.
if (BUILD_TESTING AND LIBRESTUDIO_HAVE_I18NPOOL)
    add_test(NAME sot_storage_check
        COMMAND sot_storage_check "${CMAKE_SOURCE_DIR}/sot/qa/cppunit/data")
endif ()
