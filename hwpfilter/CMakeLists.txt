# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# hwpfilter (module hwpfilter) -- one library: hwp, the Hangul Word
# Processor (.hwp) import filter. Source list, external and dependencies are
# transcribed from Library_hwp.mk.
#
# Not reproduced:
#  - gb_Library_set_precompiled_header,hwp,... -- precompiled headers are not
#    ported anywhere in this build, same as every other module so far.
#
# ifeq ($(OS),WNT): gb_Library_use_system_win32_libs,hwp,ole32 uuid -- kept
# structurally present below, behind if (WIN32), per this port's policy of
# not silently dropping Windows-only gbuild targets (see sal/CMakeLists.txt's
# sal_comtools/sal_retry_if_failed and cpputools/CMakeLists.txt's sp2bv for
# the same pattern). Inert on this Linux build.

# zlib, resolved via librestudio_find_package() (cmake/LibreStudioTarballs.cmake).
librestudio_find_package(NAME ZLIB)

set(HWP_SOURCES
    hwpfilter/source/attributes
    hwpfilter/source/cspline
    hwpfilter/source/fontmap
    hwpfilter/source/formula
    hwpfilter/source/grammar
    hwpfilter/source/hbox
    hwpfilter/source/hcode
    hwpfilter/source/hfont
    hwpfilter/source/hgzip
    hwpfilter/source/hinfo
    hwpfilter/source/hiodev
    hwpfilter/source/hpara
    hwpfilter/source/hstream
    hwpfilter/source/hstyle
    hwpfilter/source/htags
    hwpfilter/source/hutil
    hwpfilter/source/hwpeq
    hwpfilter/source/hwpfile
    hwpfilter/source/hwpread
    hwpfilter/source/hwpreader
    hwpfilter/source/lexer
    hwpfilter/source/solver
)

librestudio_expand_cxx(HWP_SOURCES)
add_library(hwp SHARED ${HWP_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(hwp PRIVATE
        "${CMAKE_SOURCE_DIR}/hwpfilter/inc/pch/precompiled_hwp.hxx")
    target_compile_definitions(hwp PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(hwp PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(hwp udkapi_normal_headers offapi_normal_headers)

target_link_libraries(hwp PRIVATE
    comphelper cppu cppuhelper sal utl tl
    ${LIBRESTUDIO_ZLIB_TARGET}
    Boost::boost)

# Library_hwp.mk: ifeq ($(OS),WNT) gb_Library_use_system_win32_libs,hwp,
# ole32 uuid endif -- see this file's header comment above.
if (WIN32)
    target_link_libraries(hwp PRIVATE ole32 uuid)
endif ()

# hwpfilter/source/hwp.component's third gb_Library_set_componentfile
# argument is "services", i.e. the office layer.
librestudio_add_component(hwp
    COMPONENT "${CMAKE_SOURCE_DIR}/hwpfilter/source/hwp.component"
    LIBRARY   hwp
    LAYER     OOO)

librestudio_set_library_output_name(hwp hwp OOO)

install(TARGETS hwp LIBRARY DESTINATION program)

# --- CppunitTest_hwpfilter_test_hwpfilter -----------------------------------
#
# hwpfilter's only CppunitTest (Module_hwpfilter.mk's gb_Module_add_check_
# targets), checked directly against CppunitTest_hwpfilter_test_hwpfilter.mk.
# 1 source (hwpfilter/qa/cppunit/test_hwpfilter), use_libraries ucbhelper
# comphelper cppu cppuhelper sal test unotest ("test" is test/CMakeLists.txt's
# target name test_fixtures here, same rename every other CppunitTest in this
# port already applies -- see test/CMakeLists.txt's own comment on why),
# use_sdk_api (normal udkapi/offapi headers), use_ure + use_vcl -> 3 protector
# pairs (unobootstrapprotector/unoexceptionprotector/vclbootstrapprotector,
# same precedent as sax_parser/sax_xmlimport), use_components -> COMPONENTS
# below, use_configuration -> CONFIGURATION (cmake/LibreStudioTest.cmake;
# read its header comment for the mechanism).
#
# COMPONENTS lookup keys, each verified against the owning module's own
# librestudio_add_component() call rather than assumed from the .mk's own
# gbuild-path spelling: "configmgr/source/configmgr" -> "configmgr"
# (configmgr/CMakeLists.txt), "framework/util/fwk" -> "fwk" (framework/
# CMakeLists.txt), "hwpfilter/source/hwp" -> "hwp" (this file's own
# librestudio_add_component(hwp ...) call above, NOT "hwpfilter" -- the
# lookup key is the component's own CMake-side name, not the module
# directory), "ucb/source/core/ucb1" -> "ucb1" and "ucb/source/ucp/file/
# ucpfile1" -> "ucpfile1" (both ucb/CMakeLists.txt).
#
# No deferral needed: every dependency below (test/CMakeLists.txt's
# test_fixtures/PROTECTORS, configmgr/framework/ucb's components,
# officecfg_registry/LIBRESTUDIO_TEST_UNITTEST_DIR for CONFIGURATION) is
# already add_subdirectory()'d by the time hwpfilter's own add_subdirectory()
# runs in the root CMakeLists.txt (officecfg/configmgr/framework/ucb/
# unotest/test all sit textually before hwpfilter there) -- unlike
# configmgr_unit/xmlscript_cppunit/scaddins_analysis, which each need at
# least one dependency add_subdirectory()'d AFTER their own module and so are
# registered from their own *_CppunitTest_deferred.cmake files instead.
librestudio_add_cppunit_test(hwpfilter_test_hwpfilter
    SOURCES
        hwpfilter/qa/cppunit/test_hwpfilter
    LIBRARIES ucbhelper comphelper cppu cppuhelper sal test_fixtures unotest
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        configmgr fwk hwp ucb1 ucpfile1
    CONFIGURATION)
