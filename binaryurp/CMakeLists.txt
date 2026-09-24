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
