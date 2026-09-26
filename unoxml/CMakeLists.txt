# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# unoxml — only the unoxml library so far: a libxml2-backed DOM/XPath/DOM
# events implementation exposed as a UNO service. Library_unordf.mk (RDF
# support, gb_Side=host only) and the CppunitTest_unoxml_domtest/rdftest and
# JunitTest_unoxml_complex check targets from Module_unoxml.mk are not ported.

set(UNOXML_SOURCES
    unoxml/source/dom/node
    unoxml/source/dom/document
    unoxml/source/dom/element
    unoxml/source/dom/attr
    unoxml/source/dom/cdatasection
    unoxml/source/dom/characterdata
    unoxml/source/dom/comment
    unoxml/source/dom/documentbuilder
    unoxml/source/dom/documentfragment
    unoxml/source/dom/documenttype
    unoxml/source/dom/entity
    unoxml/source/dom/entityreference
    unoxml/source/dom/notation
    unoxml/source/dom/processinginstruction
    unoxml/source/dom/text
    unoxml/source/dom/domimplementation
    unoxml/source/dom/elementlist
    unoxml/source/dom/childlist
    unoxml/source/dom/notationsmap
    unoxml/source/dom/entitiesmap
    unoxml/source/dom/attributesmap
    unoxml/source/dom/saxbuilder
    unoxml/source/xpath/xpathobject
    unoxml/source/xpath/nodelist
    unoxml/source/xpath/xpathapi
    unoxml/source/events/event
    unoxml/source/events/eventdispatcher
    unoxml/source/events/mutationevent
    unoxml/source/events/uievent
    unoxml/source/events/mouseevent
)

librestudio_expand_cxx(UNOXML_SOURCES)
add_library(unoxml SHARED ${UNOXML_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(unoxml PRIVATE
        "${CMAKE_SOURCE_DIR}/unoxml/inc/pch/precompiled_unoxml.hxx")
    target_compile_definitions(unoxml PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(unoxml PRIVATE
    "${CMAKE_SOURCE_DIR}/unoxml/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(unoxml udkapi_normal_headers offapi_normal_headers)

target_link_libraries(unoxml PRIVATE
    ucbhelper sax comphelper cppuhelper cppu sal tl xo
    Boost::boost
    LibXml2::LibXml2)

librestudio_add_component(unoxml
    COMPONENT "${CMAKE_SOURCE_DIR}/unoxml/source/service/unoxml.component"
    LIBRARY   unoxml
    LAYER     OOO)

librestudio_set_library_output_name(unoxml unoxml OOO)
install(TARGETS unoxml LIBRARY DESTINATION program)
