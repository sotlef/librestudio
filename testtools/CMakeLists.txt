# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# testtools — the UNO bridge/URP self-test used to exercise a language binding
# end to end: one process exports a service through it, a second process
# reaches across the bridge and calls back, and both sides assert what came
# back matches. bridgetest_server/bridgetest_client (generated below) are the
# two halves of that test, run by hand or from CustomTarget_uno_test.
#
# Scope, from Module_testtools.mk: CROSS_COMPILING is never TRUE for this
# port, so that whole module resolves. Left out, matching this port's existing
# policy of not porting test-run/language-binding-generator targets (compare
# desktop's Rdb_passive_* exclusions):
#   Jar_testComponent, CustomTarget_bridgetest_javamaker  -- ENABLE_JAVA is off
#   CustomTarget_bridgetest_climaker                      -- CLI/Windows only
#   CustomTarget_uno_test                                 -- .PHONY test-run
#                                                             target (gb_Module_
#                                                             add_check_targets)

# --- InternalUnoApi_bridgetest -----------------------------------------------
#
# gb_InternalUnoApi_InternalUnoApi,bridgetest,testtools/source/bridgetest/idl/
# bridgetest.idl -- the registry root is a single loose .idl file, not a
# directory tree, hence librestudio_uno_api_rdb_file() rather than
# librestudio_uno_api_rdb(). gb_InternalUnoApi_use_api,bridgetest,udkapi maps
# to DEP_RDBS.
librestudio_uno_api_rdb_file(bridgetest
    IDL      "${CMAKE_SOURCE_DIR}/testtools/source/bridgetest/idl/bridgetest.idl"
    OUTPUT   "${CMAKE_BINARY_DIR}/UnoApiTarget/bridgetest.rdb"
    DEP_RDBS "${udkapi_RDB}")

# testtools_bridgetest-common, testtools_cppobj, testtools_bridgetest and
# testtools_constructors all gb_Library_use_internal_api(...,bridgetest), and
# every consumer of a "normal" internal API elsewhere in this port (e.g.
# cppuhelper's own unotypes are the "comprehensive" exception, not the rule)
# takes the normal variant, so only that one is generated here.
librestudio_uno_api_headers(bridgetest_normal
    RDB        "${bridgetest_RDB}"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/UnoApiHeadersTarget/bridgetest/normal"
    VARIANT    normal
    EXTRA_RDBS "${udkapi_RDB}")

# --- InternalUnoApi_performance -----------------------------------------------
#
# gb_InternalUnoApi_InternalUnoApi,performance,testtools/source/bridgetest/idl/
# com/sun/star/test/performance/XPerformanceTest.idl -- same shape as
# bridgetest above, another single loose .idl file. Nothing in the sources
# built by this file #includes anything performance-related (grepped for
# "performance" across testtools/source/bridgetest/*.cxx|*.hxx -- no hits),
# and its only consumer upstream is the Java bridge test (Jar_testComponent,
# out of scope: ENABLE_JAVA is off), so no headers variant is generated for
# it -- the .rdb is built for module completeness only, matching
# Module_testtools.mk listing it unconditionally.
librestudio_uno_api_rdb_file(performance
    IDL      "${CMAKE_SOURCE_DIR}/testtools/source/bridgetest/idl/com/sun/star/test/performance/XPerformanceTest.idl"
    OUTPUT   "${CMAKE_BINARY_DIR}/UnoApiTarget/performance.rdb"
    DEP_RDBS "${udkapi_RDB}")

# --- testtools_bridgetest-common ---------------------------------------------
#
# Library_bridgetest-common.mk. Shared helpers used by the three components
# below; carries no .component of its own (no gb_Library_set_componentfile
# call in that .mk).
set(TESTTOOLS_BRIDGETESTCOMMON_SOURCES
    testtools/source/bridgetest/currentcontextchecker
    testtools/source/bridgetest/multi)
librestudio_expand_cxx(TESTTOOLS_BRIDGETESTCOMMON_SOURCES)

add_library(testtools_bridgetest-common SHARED ${TESTTOOLS_BRIDGETESTCOMMON_SOURCES})

target_include_directories(testtools_bridgetest-common BEFORE PRIVATE
    "${bridgetest_normal_HEADERS_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(testtools_bridgetest-common bridgetest_normal_headers udkapi_normal_headers)

# gb_Library_add_defs,testtools_bridgetest-common,-DLO_DLLIMPLEMENTATION_TESTTOOLS
# -- consumed by testtools/source/bridgetest/dllapi.hxx to pick
# SAL_DLLPUBLIC_EXPORT over _IMPORT.
target_compile_definitions(testtools_bridgetest-common PRIVATE LO_DLLIMPLEMENTATION_TESTTOOLS)

target_link_libraries(testtools_bridgetest-common PRIVATE
    cppu cppuhelper sal
    Boost::boost)

# PLAINLIBS_NONE in Repository.mk (unlike every other library in this
# module's neighbourhood, these four are never installed by gbuild's own
# packaging) -- plain "lib<name>.so" naming, no "lo"/"gcc3" suffix, same as
# e.g. bridges' gcc3_uno or jurt's jpipe.
librestudio_set_library_output_name(testtools_bridgetest-common testtools_bridgetest-common PLAIN)

install(TARGETS testtools_bridgetest-common LIBRARY DESTINATION program)

# --- testtools_cppobj ---------------------------------------------------------
#
# Library_cppobj.mk. Implements com.sun.star.comp.bridge.CppTestObject, the
# service the bridgetest_server script instantiates.
set(TESTTOOLS_CPPOBJ_SOURCES
    testtools/source/bridgetest/cppobj)
librestudio_expand_cxx(TESTTOOLS_CPPOBJ_SOURCES)

add_library(testtools_cppobj SHARED ${TESTTOOLS_CPPOBJ_SOURCES})

target_include_directories(testtools_cppobj BEFORE PRIVATE
    "${CMAKE_SOURCE_DIR}/testtools/source/bridgetest"
    "${bridgetest_normal_HEADERS_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(testtools_cppobj bridgetest_normal_headers udkapi_normal_headers)

target_link_libraries(testtools_cppobj PRIVATE
    cppu cppuhelper sal
    testtools_bridgetest-common
    Boost::boost)

librestudio_set_library_output_name(testtools_cppobj testtools_cppobj PLAIN)

# gb_Library_set_componentfile,testtools_cppobj,testtools/source/bridgetest/
# cppobj,uno_services -- packed into the module's own private registry, not
# the main app's services.rdb.
#
# LAYER: gbuild's own componentprefix for PLAINLIBS_NONE is
# vnd.sun.star.expand:$LO_BUILD_LIB_DIR/, a variable set only by the
# generated bridgetest_server/bridgetest_client scripts below (there is no
# "NONE" layer in librestudio_add_component -- only URELIB/OOO exist, because
# every other module in this port is either URE- or office-layer). URELIB is
# used here instead: this build has a single shared library/executable output
# directory (LIBRESTUDIO_PROGRAM_DIR, set as both CMAKE_RUNTIME_OUTPUT_DIRECTORY
# and CMAKE_LIBRARY_OUTPUT_DIRECTORY in LibreStudioPlatform.cmake), so
# $URE_INTERNAL_LIB_DIR -- already exported by unorc next to cppuhelper, which
# `uno` always links and bootstraps through -- resolves to exactly the
# directory these libraries land in, same as $LO_BUILD_LIB_DIR would. The
# generated scripts still export LO_BUILD_LIB_DIR for fidelity with
# CustomTarget_bridgetest.mk; it goes unused by the URI this LAYER produces.
librestudio_add_component(testtools_cppobj_component
    COMPONENT "${CMAKE_SOURCE_DIR}/testtools/source/bridgetest/cppobj.component"
    LIBRARY   testtools_cppobj
    LAYER     URELIB
    RDB       uno_services)

install(TARGETS testtools_cppobj LIBRARY DESTINATION program)

# --- testtools_bridgetest ------------------------------------------------------
#
# Library_bridgetest.mk. Implements com.sun.star.comp.bridge.BridgeTest, the
# service the bridgetest_client script drives. No extra gb_Executable_set_include
# in this .mk (unlike cppobj/constructors) -- it needs no same-directory include.
set(TESTTOOLS_BRIDGETEST_SOURCES
    testtools/source/bridgetest/bridgetest)
librestudio_expand_cxx(TESTTOOLS_BRIDGETEST_SOURCES)

add_library(testtools_bridgetest SHARED ${TESTTOOLS_BRIDGETEST_SOURCES})

target_include_directories(testtools_bridgetest BEFORE PRIVATE
    "${bridgetest_normal_HEADERS_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(testtools_bridgetest bridgetest_normal_headers udkapi_normal_headers)

target_link_libraries(testtools_bridgetest PRIVATE
    cppu cppuhelper sal
    testtools_bridgetest-common
    Boost::boost)

librestudio_set_library_output_name(testtools_bridgetest testtools_bridgetest PLAIN)

librestudio_add_component(testtools_bridgetest_component
    COMPONENT "${CMAKE_SOURCE_DIR}/testtools/source/bridgetest/bridgetest.component"
    LIBRARY   testtools_bridgetest
    LAYER     URELIB
    RDB       uno_services)

install(TARGETS testtools_bridgetest LIBRARY DESTINATION program)

# --- testtools_constructors -----------------------------------------------------
#
# Library_constructors.mk. Implements two services, Constructors and
# Constructors2. Deliberately does NOT link testtools_bridgetest-common --
# unlike cppobj and bridgetest, its gb_Library_use_libraries list is only
# cppu, cppuhelper, sal.
set(TESTTOOLS_CONSTRUCTORS_SOURCES
    testtools/source/bridgetest/constructors)
librestudio_expand_cxx(TESTTOOLS_CONSTRUCTORS_SOURCES)

add_library(testtools_constructors SHARED ${TESTTOOLS_CONSTRUCTORS_SOURCES})

target_include_directories(testtools_constructors BEFORE PRIVATE
    "${CMAKE_SOURCE_DIR}/testtools/source/bridgetest"
    "${bridgetest_normal_HEADERS_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(testtools_constructors bridgetest_normal_headers udkapi_normal_headers)

target_link_libraries(testtools_constructors PRIVATE
    cppu cppuhelper sal
    Boost::boost)

librestudio_set_library_output_name(testtools_constructors testtools_constructors PLAIN)

librestudio_add_component(testtools_constructors_component
    COMPONENT "${CMAKE_SOURCE_DIR}/testtools/source/bridgetest/constructors.component"
    LIBRARY   testtools_constructors
    LAYER     URELIB
    RDB       uno_services)

install(TARGETS testtools_constructors LIBRARY DESTINATION program)

# --- Rdb_uno_services ----------------------------------------------------------
#
# Packs the three .component files declared above into their own registry,
# never installed -- consumed only by the generated bridgetest_server/
# bridgetest_client scripts below via URE_MORE_SERVICES. OUTPUT matches
# gbuild's own per-Rdb path convention ($(WORKDIR)/Rdb/uno_services.rdb) since
# this is not the main app's "services" registry.
librestudio_finalize_components(uno_services
    OUTPUT "${CMAKE_BINARY_DIR}/Rdb/uno_services.rdb")

# --- CustomTarget_bridgetest -----------------------------------------------------
#
# CustomTarget_bridgetest.mk writes two tiny launcher scripts (a third,
# bridgetest_javaserver, is ENABLE_JAVA-gated and out of scope). Both are pure
# configure-time text -- every path they embed is already known once this
# module's targets exist -- so, like librestudio_generate_unorc()
# (cmake/LibreStudioUnorc.cmake), they are written with a plain file(WRITE)
# rather than an add_custom_command: there is no generated *content* that
# depends on a target being rebuilt, only on where things live, and gbuild's
# own recipe agrees -- neither script has a real prerequisite on the
# libraries/Rdbs whose paths it writes, only an order-only mark on the output
# directory existing.
#
# uno's own path is written out as a plain string rather than
# $<TARGET_FILE:uno> because file(WRITE) does not evaluate generator
# expressions (only add_custom_command/file(GENERATE) do) -- CMAKE_RUNTIME_
# OUTPUT_DIRECTORY is LIBRESTUDIO_PROGRAM_DIR and the executable is named
# plainly "uno" with no LAYER renaming, so the path is known outright.
set(TESTTOOLS_BRIDGEDIR "${CMAKE_BINARY_DIR}/CustomTarget/testtools/bridgetest")
file(MAKE_DIRECTORY "${TESTTOOLS_BRIDGEDIR}")

set(TESTTOOLS_UNO_EXE "${LIBRESTUDIO_PROGRAM_DIR}/uno")
set(TESTTOOLS_LO_BUILD_LIB_DIR_URL "file://${LIBRESTUDIO_PROGRAM_DIR}")
set(TESTTOOLS_URE_MORE_SERVICES_URL "file://${uno_services_RDB}")
set(TESTTOOLS_URE_MORE_TYPES_URL "file://${bridgetest_RDB}")

file(WRITE "${TESTTOOLS_BRIDGEDIR}/bridgetest_server"
"${TESTTOOLS_UNO_EXE} -s com.sun.star.test.bridge.CppTestObject -u 'uno:socket,host=127.0.0.1,port=2002;urp;test' --singleaccept -env:LO_BUILD_LIB_DIR=${TESTTOOLS_LO_BUILD_LIB_DIR_URL} -env:URE_MORE_SERVICES=${TESTTOOLS_URE_MORE_SERVICES_URL} -env:URE_MORE_TYPES=${TESTTOOLS_URE_MORE_TYPES_URL}
")
file(CHMOD "${TESTTOOLS_BRIDGEDIR}/bridgetest_server" PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE)

file(WRITE "${TESTTOOLS_BRIDGEDIR}/bridgetest_client"
"${TESTTOOLS_UNO_EXE} -s com.sun.star.test.bridge.BridgeTest -- -u 'uno:socket,host=127.0.0.1,port=2002;urp;test' -env:LO_BUILD_LIB_DIR=${TESTTOOLS_LO_BUILD_LIB_DIR_URL} -env:URE_MORE_SERVICES=${TESTTOOLS_URE_MORE_SERVICES_URL} -env:URE_MORE_TYPES=${TESTTOOLS_URE_MORE_TYPES_URL}
")
file(CHMOD "${TESTTOOLS_BRIDGEDIR}/bridgetest_client" PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE)

# Phony umbrella so `cmake --build . --target testtools_bridgetest_scripts`
# (or plain `all`) pulls in everything the two scripts point at, including
# performance_rdb -- which nothing else in this file consumes, but which
# Module_testtools.mk builds unconditionally alongside bridgetest's own .rdb,
# and whose _rdb target (from librestudio_uno_api_rdb_file) is not itself
# marked ALL.
add_custom_target(testtools_bridgetest_scripts ALL
    DEPENDS
        uno
        testtools_bridgetest-common
        testtools_cppobj
        testtools_bridgetest
        testtools_constructors
        bridgetest_rdb
        performance_rdb
        uno_services_rdb)
