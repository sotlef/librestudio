# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# xmlreader — a pull parser for XML and for the UNO configuration formats.
#
# Despite the name it does not use libxml2; it is a self-contained parser, which
# is why the UNO core does not depend on libxml2 at all.
# Source list from Library_xmlreader.mk.

set(XMLREADER_SOURCES
    xmlreader/source/pad
    xmlreader/source/span
    xmlreader/source/xmlreader)

librestudio_expand_cxx(XMLREADER_SOURCES)

add_library(xmlreader SHARED ${XMLREADER_SOURCES})

target_compile_definitions(xmlreader PRIVATE OOO_DLLIMPLEMENTATION_XMLREADER)

target_include_directories(xmlreader PRIVATE "${udkapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(xmlreader udkapi_normal_headers)

target_link_libraries(xmlreader PRIVATE cppu sal)

librestudio_set_library_output_name(xmlreader xmlreader OOO)

install(TARGETS xmlreader LIBRARY DESTINATION program)
