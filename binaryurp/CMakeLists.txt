# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# binaryurp — the UNO remote protocol bridge: marshalling of calls and the
# proxies that stand in for remote objects.
# Source list from Library_binaryurp.mk.

set(BINARYURP_SOURCES
    binaryurp/source/binaryany
    binaryurp/source/bridge
    binaryurp/source/bridgefactory
    binaryurp/source/currentcontext
    binaryurp/source/incomingrequest
    binaryurp/source/lessoperators
    binaryurp/source/marshal
    binaryurp/source/outgoingrequests
    binaryurp/source/proxy
    binaryurp/source/reader
    binaryurp/source/unmarshal
    binaryurp/source/writer)

librestudio_expand_cxx(BINARYURP_SOURCES)

add_library(binaryurp SHARED ${BINARYURP_SOURCES})

target_include_directories(binaryurp PRIVATE "${udkapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(binaryurp udkapi_normal_headers)

target_link_libraries(binaryurp PRIVATE cppu cppuhelper sal salhelper)

librestudio_add_component(binaryurp
    COMPONENT "${CMAKE_SOURCE_DIR}/binaryurp/source/binaryurp.component"
    LIBRARY   binaryurp
    LAYER     URELIB)

librestudio_set_library_output_name(binaryurp binaryurp OOO)

install(TARGETS binaryurp LIBRARY DESTINATION program)

# --- CppunitTest targets ------------------------------------------------------
#
# binaryurp's 2 CppunitTest_*.mk files, ported via librestudio_add_cppunit_test()
# (cmake/LibreStudioTest.cmake -- read its header comment first). Neither calls
# use_ure/use_vcl*, so neither needs PROTECTORS. Both re-checked directly
# against their live .mk files (not just the prior-research summary).
#
# Target-name collision check: "binaryurp_test-cache" / "binaryurp_test-unmarshal"
# do not collide with the "binaryurp" library target above, or with anything
# else in the tree (grepped) -- no _test-suffix rename needed here, unlike the
# store_test/basegfx_test/i18nutil_test/sax_test precedent (those collided
# because their gbuild CppunitTest name was literally the bare module name).

# CppunitTest_binaryurp_test-cache.mk: 1 source, use_libraries sal only --
# binaryurp::Cache (binaryurp/source/cache.hxx, included by the test) is a
# header-only template, so nothing from the binaryurp library itself needs to
# be linked or recompiled in for this one.
librestudio_add_cppunit_test(binaryurp_test-cache
    SOURCES
        binaryurp/qa/test-cache
    LIBRARIES sal)

# CppunitTest_binaryurp_test-unmarshal.mk: 1 source, use_libraries cppu
# cppuhelper sal salhelper, use_library_objects(binaryurp) and use_api(udkapi)
# -> plain udkapi (same variant add_library(binaryurp...) above links
# against).
#
# use_library_objects pulls in ALL of binaryurp's own compiled objects
# (gb_LinkTarget_use_library_objects -> gb_LinkTarget__use_linktarget_objects,
# solenv/gbuild/LinkTarget.mk:1839-1849 -- it imports the whole named
# LinkTarget's objects, not a hand-picked subset) -- and indeed
# binaryurp/qa/test-unmarshal.cxx includes "../source/bridge.hxx",
# "../source/cache.hxx", "../source/readerstate.hxx" and
# "../source/unmarshal.hxx" and constructs a real binaryurp::Unmarshal against
# a binaryurp::Bridge, none of which are part of binaryurp's public UNO
# service surface (not in any .component), so linking libbinaryurp.so would
# not expose them. Same technique and reasoning as sal_osl_file_details
# (sal/CMakeLists.txt) and jvmfwk_sunversion (jvmfwk/CMakeLists.txt): recompile
# the library's own already-expanded source list (${BINARYURP_SOURCES}, set
# above) directly into this MODULE instead of restructuring the production
# `binaryurp` target into an OBJECT library. BINARYURP_SOURCES already holds
# absolute paths after librestudio_expand_cxx() above, so
# librestudio_add_cppunit_test()'s own SOURCES expansion (which passes
# already-absolute paths through unchanged -- see cmake/LibreStudioTest.cmake)
# leaves them as-is.
librestudio_add_cppunit_test(binaryurp_test-unmarshal
    SOURCES
        ${BINARYURP_SOURCES}
        binaryurp/qa/test-unmarshal
    LIBRARIES cppu cppuhelper sal salhelper
    INCLUDE "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers)
