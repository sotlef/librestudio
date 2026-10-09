# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# comphelper — the grab bag of helpers the UNO services are built from:
# property sets, containers, streams, accessibility plumbing, configuration
# access and the crypto backend used for document passwords.
#
# Source list from Library_comphelper.mk.  What is *not* ported from there:
# the GPGME support (disabled by default in gbuild too, via ENABLE_GPGMEPP),
# and the precompiled header.

# zlib, resolved via librestudio_find_package() (cmake/LibreStudioTarballs.cmake).
librestudio_find_package(NAME ZLIB)

# The crypto backend is chosen by LIBRESTUDIO_TLS_BACKEND, mirroring the
# TLS switch in the .mk: exactly one of these three files is compiled.
if (LIBRESTUDIO_TLS_BACKEND STREQUAL "NSS")
    set(COMPHELPER_CRYPTO_SOURCE comphelper/source/crypto/Crypto_NSS)
elseif (LIBRESTUDIO_TLS_BACKEND STREQUAL "OPENSSL")
    set(COMPHELPER_CRYPTO_SOURCE comphelper/source/crypto/Crypto_OpenSSL)
else ()
    message(FATAL_ERROR "Unknown LIBRESTUDIO_TLS_BACKEND: ${LIBRESTUDIO_TLS_BACKEND}")
endif ()

set(COMPHELPER_SOURCES
    comphelper/source/crypto/Crypto
    ${COMPHELPER_CRYPTO_SOURCE}
    comphelper/source/compare/AnyCompareFactory
    comphelper/source/container/container
    comphelper/source/container/containermultiplexer
    comphelper/source/container/embeddedobjectcontainer
    comphelper/source/container/enumerablemap
    comphelper/source/container/enumhelper
    comphelper/source/container/IndexedPropertyValuesContainer
    comphelper/source/container/interfacecontainer2
    comphelper/source/container/namecontainer
    comphelper/source/container/NamedPropertyValuesContainer
    comphelper/source/eventattachermgr/eventattachermgr
    comphelper/source/misc/accessibleeventnotifier
    comphelper/source/misc/AccessibleImplementationHelper
    comphelper/source/misc/accessiblekeybindinghelper
    comphelper/source/misc/accessibleselectionhelper
    comphelper/source/misc/accessibletexthelper
    comphelper/source/misc/anycompare
    comphelper/source/misc/anytohash
    comphelper/source/misc/anytostring
    comphelper/source/misc/asyncnotification
    comphelper/source/misc/asyncquithandler
    comphelper/source/misc/automationinvokedzone
    comphelper/source/misc/backupfilehelper
    comphelper/source/misc/base64
    comphelper/source/misc/compbase
    comphelper/source/misc/componentbase
    comphelper/source/misc/configuration
    comphelper/source/misc/configurationhelper
    comphelper/source/misc/date
    comphelper/source/misc/debuggerinfo
    comphelper/source/misc/diagnose_ex
    comphelper/source/misc/DirectoryHelper
    comphelper/source/misc/dispatchcommand
    comphelper/source/misc/docpasswordhelper
    comphelper/source/misc/docpasswordrequest
    comphelper/source/misc/documentinfo
    comphelper/source/misc/emscriptenthreading
    comphelper/source/misc/errcode
    comphelper/source/misc/evtlistenerhlp
    comphelper/source/misc/evtmethodhelper
    comphelper/source/misc/fileurl
    comphelper/source/misc/getexpandeduri
    comphelper/source/misc/graphicmimetype
    comphelper/source/misc/hash
    comphelper/source/misc/instancelocker
    comphelper/source/misc/interaction
    comphelper/source/misc/logging
    comphelper/source/misc/lok
    comphelper/source/misc/mimeconfighelper
    comphelper/source/misc/namedvaluecollection
    comphelper/source/misc/numberedcollection
    comphelper/source/misc/numbers
    comphelper/source/misc/OAccessible
    comphelper/source/misc/officerestartmanager
    comphelper/source/misc/random
    comphelper/source/misc/scriptbrowse
    comphelper/source/misc/SelectionMultiplex
    comphelper/source/misc/sequenceashashmap
    comphelper/source/misc/sharedmutex
    comphelper/source/misc/simplefileaccessinteraction
    comphelper/source/misc/solarmutex
    comphelper/source/misc/stillreadwriteinteraction
    comphelper/source/misc/storagehelper
    comphelper/source/misc/string
    comphelper/source/misc/synchronousdispatch
    comphelper/source/misc/syntaxhighlight
    comphelper/source/misc/threadpool
    comphelper/source/misc/traceevent
    comphelper/source/misc/types
    comphelper/source/misc/weakeventlistener
    comphelper/source/misc/xmlsechelper
    comphelper/source/officeinstdir/officeinstallationdirectories
    comphelper/source/processfactory/processfactory
    comphelper/source/property/ChainablePropertySet
    comphelper/source/property/ChainablePropertySetInfo
    comphelper/source/property/genericpropertyset
    comphelper/source/property/MasterPropertySet
    comphelper/source/property/MasterPropertySetInfo
    comphelper/source/property/opropertybag
    comphelper/source/property/propagg
    comphelper/source/property/property
    comphelper/source/property/propertybag
    comphelper/source/property/propertycontainer
    comphelper/source/property/propertycontainerhelper
    comphelper/source/property/propertysethelper
    comphelper/source/property/propertysetinfo
    comphelper/source/property/propertystatecontainer
    comphelper/source/property/propmultiplex
    comphelper/source/property/propmultiplex2
    comphelper/source/property/propshlp
    comphelper/source/property/propstate
    comphelper/source/streaming/basicio
    comphelper/source/streaming/memorystream
    comphelper/source/streaming/oslfile2streamwrap
    comphelper/source/streaming/seekableinput
    comphelper/source/streaming/seqinputstreamserv
    comphelper/source/streaming/seqoutputstreamserv
    comphelper/source/streaming/seqstream
    comphelper/source/streaming/streamsection
    comphelper/source/xml/attributelist
    comphelper/source/xml/ofopxmlhelper
    comphelper/source/xml/xmltools
)

librestudio_expand_cxx(COMPHELPER_SOURCES)

add_library(comphelper SHARED ${COMPHELPER_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(comphelper PRIVATE
        "${CMAKE_SOURCE_DIR}/comphelper/inc/pch/precompiled_comphelper.hxx")
    target_compile_definitions(comphelper PRIVATE PCH_LEVEL=2)
endif ()

# gb_Library_use_custom_headers(comphelper, officecfg/registry) — comphelper
# reads configuration through the generated officecfg:: types.
target_include_directories(comphelper PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}")
add_dependencies(comphelper officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(comphelper PRIVATE COMPHELPER_DLLIMPLEMENTATION)

target_link_libraries(comphelper PRIVATE
    cppu cppuhelper sal salhelper ucbhelper i18nlangtag
    ICU::uc
    ${LIBRESTUDIO_ZLIB_TARGET}
    Boost::boost)

if (LIBRESTUDIO_TLS_BACKEND STREQUAL "NSS")
    pkg_check_modules(NSS REQUIRED IMPORTED_TARGET nss)
    target_link_libraries(comphelper PRIVATE PkgConfig::NSS)
else ()
    pkg_check_modules(OPENSSL REQUIRED IMPORTED_TARGET openssl)
    target_link_libraries(comphelper PRIVATE PkgConfig::OPENSSL)
endif ()

# gb_Library_set_componentfile(comphelper, comphelper/util/comphelp, services) —
# comphelper is an office-layer library, so its component uses the OOO prefix.
librestudio_add_component(comphelper
    COMPONENT "${CMAKE_SOURCE_DIR}/comphelper/util/comphelp.component"
    LIBRARY   comphelper
    LAYER     OOO)

librestudio_set_library_output_name(comphelper comphelper OOO)

install(TARGETS comphelper LIBRARY DESTINATION program)

# --- StaticLibrary_windows_process ------------------------------------------

# StaticLibrary_windows_process.mk: gated at the module level in
# Module_comphelper.mk -- $(if $(filter WNT,$(OS)), StaticLibrary_windows_process)
# -- so the whole library is Windows-only, unlike the tl/ooopathutils and
# tl/UseUnixWrappers static libraries (tools/CMakeLists.txt), which are
# unconditional. Kept structurally present, behind if (WIN32), per this
# port's policy of not silently dropping Windows-only gbuild targets -- see
# sal/CMakeLists.txt's sal_comtools/sal_retry_if_failed and cpputools/
# CMakeLists.txt's sp2bv for the same pattern, and this project's own
# Windows/MSVC research on why it cannot actually be built or exercised on
# this machine (no cl.exe/ml64.exe/SDK). Inert on this Linux build: the
# if (WIN32) block below never executes here.
#
# 1 source (comphelper/source/windows/windows_process.cxx, gb_StaticLibrary_
# add_exception_objects), gb_StaticLibrary_set_include is just $$(INCLUDE)
# (no extra include dirs), and no use_libraries/use_api/use_sdk_api at all
# in the .mk.
if (WIN32)
    add_library(windows_process STATIC
        "${CMAKE_SOURCE_DIR}/comphelper/source/windows/windows_process.cxx")
endif ()

# comphelper's 7 CppunitTest_*.mk targets (Module_comphelper.mk's
# gb_Module_add_check_targets list), ported via librestudio_add_cppunit_test()
# (cmake/LibreStudioTest.cmake -- read its header comment first for the
# shared rationale). Each was re-checked directly against its live .mk file,
# not assumed from a summary -- one of them (CppunitTest_comphelper_weakbag_
# test.mk) turns out to name a DIFFERENT internal CppunitTest name
# ("comphelper_weakbag", via gb_CppunitTest_CppunitTest,comphelper_weakbag)
# than its own filename suggests; the CMake target below uses that real
# internal name, matching what gb_CppunitTest_get_filename would actually
# produce ("libtest_comphelper_weakbag.so"), not the filename-derived one.
#
# JunitTest_comphelper_complex and PythonTest_comphelper_python
# (Module_comphelper.mk's gb_Module_add_subsequentcheck_targets) are a
# completely different test-runner technology -- JUnit/Java and Python --
# that this port has no infrastructure for anywhere (no JUnit runner, no
# Python test harness). Deliberately deferred as their own category, not
# silently dropped and not attempted here.

# CppunitTest_comphelper_parallelsort_test.mk: 1 source, use_sdk_api (udkapi +
# offapi), use_libraries comphelper cppuhelper cppu sal tl -- no use_ure, no
# protectors needed.
librestudio_add_cppunit_test(comphelper_parallelsort_test
    SOURCES
        comphelper/qa/unit/parallelsorttest
    LIBRARIES comphelper cppuhelper cppu sal tl
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers)

# CppunitTest_comphelper_threadpool_test.mk: 1 source, use_sdk_api, same
# use_libraries shape as comphelper_parallelsort_test above.
librestudio_add_cppunit_test(comphelper_threadpool_test
    SOURCES
        comphelper/qa/unit/threadpooltest
    LIBRARIES comphelper cppuhelper cppu sal tl
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers)

# CppunitTest_comphelper_syntaxhighlight_test.mk: 1 source, use_sdk_api,
# use_libraries comphelper cppuhelper cppu sal (no tl here, unlike the two
# above).
librestudio_add_cppunit_test(comphelper_syntaxhighlight_test
    SOURCES
        comphelper/qa/unit/syntaxhighlighttest
    LIBRARIES comphelper cppuhelper cppu sal
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers)

# CppunitTest_comphelper_variadictemplates_test.mk: 1 source, use_sdk_api,
# same use_libraries shape as comphelper_syntaxhighlight_test above.
librestudio_add_cppunit_test(comphelper_variadictemplates_test
    SOURCES
        comphelper/qa/unit/variadictemplates
    LIBRARIES comphelper cppuhelper cppu sal
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers)

# CppunitTest_comphelper_ifcontainer.mk: 1 source, use_api udkapi ONLY (not
# use_sdk_api -- no offapi include here), use_libraries comphelper cppu
# cppuhelper sal.
librestudio_add_cppunit_test(comphelper_ifcontainer
    SOURCES
        comphelper/qa/container/comphelper_ifcontainer
    LIBRARIES comphelper cppu cppuhelper sal
    INCLUDE "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers)

# CppunitTest_comphelper_weakbag_test.mk: 1 source, use_api udkapi ONLY (same
# shape as comphelper_ifcontainer above). Internal CppunitTest name is
# "comphelper_weakbag" (see the file-mismatch note above), not
# "comphelper_weakbag_test" -- used verbatim here.
librestudio_add_cppunit_test(comphelper_weakbag
    SOURCES
        comphelper/qa/weakbag/test_weakbag
    LIBRARIES comphelper cppu cppuhelper sal
    INCLUDE "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers)

# CppunitTest_comphelper_test.mk: registered in comphelper/CppunitTest_
# deferred.cmake, NOT here, and include()'d from the root CMakeLists.txt well
# after i18npool's own add_subdirectory(). Reason: this test needs use_
# components i18npool/util/i18npool (gb_CppunitTest_use_components), now
# wired via librestudio_add_cppunit_test()'s COMPONENTS parameter (cmake/
# LibreStudioTest.cmake) -- but COMPONENTS resolves a component's built file
# through a GLOBAL property that i18npool/CMakeLists.txt's own
# librestudio_add_component() call sets, and i18npool is add_subdirectory()'d
# AFTER comphelper in the root CMakeLists.txt's module order (i18npool links
# comphelper, so the natural producer-before-consumer order runs comphelper
# first). Defining comphelper_test here, at comphelper's own natural
# position, would hit that property before i18npool has set it and abort
# configure with librestudio_add_cppunit_test()'s own FATAL_ERROR. Moving
# just this one test's registration to a deferred, later-included file is
# the smaller, safer fix -- see comphelper/CppunitTest_deferred.cmake and its
# include() call site in the root CMakeLists.txt for the rest.

# comphelper/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/comphelper/CppunitTest_deferred.cmake")
