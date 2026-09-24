# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sax — the SAX parser layer: the expat and fast-parser wrappers, the XML
# writer, and the encoding converter they share.
#
# It is reached for two different reasons.  Documents above it parse XML
# through these services; and i18npool's localedata generator uses the expat
# wrapper to read the locale descriptions, which is why it is ported here.
#
# Source list from Library_sax.mk (the precompiled header is not ported).

set(SAX_SOURCES
    sax/source/expatwrap/sax_expat
    sax/source/expatwrap/saxwriter
    sax/source/expatwrap/xml2utf
    sax/source/fastparser/fastparser
    sax/source/fastparser/legacyfastparser
    sax/source/tools/converter
    sax/source/tools/fastattribs
    sax/source/tools/fastserializer
    sax/source/tools/fshelper)

librestudio_expand_cxx(SAX_SOURCES)

add_library(sax SHARED ${SAX_SOURCES})

target_include_directories(sax PRIVATE
    "${CMAKE_SOURCE_DIR}/sax/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}"
    "${LIBXML2_INCLUDE_DIRS}")
add_dependencies(sax udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(sax PRIVATE SAX_DLLIMPLEMENTATION)
target_link_libraries(sax PRIVATE
    basegfx comphelper cppu cppuhelper sal salhelper tl
    EXPAT::EXPAT "${LIBXML2_LIBRARIES}")

librestudio_add_component(sax
    COMPONENT "${CMAKE_SOURCE_DIR}/sax/source/expatwrap/expwrap.component"
    LIBRARY   sax
    LAYER     OOO)

librestudio_set_library_output_name(sax sax OOO)

install(TARGETS sax LIBRARY DESTINATION program)
