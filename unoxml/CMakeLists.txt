# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# unoxml — the unoxml library, Library_unordf.mk (RDF support), plus its two
# CppunitTest_*.mk targets (unoxml_domtest, unoxml_rdftest -- see unoxml/
# CppunitTest_deferred.cmake, include()'d from the root CMakeLists.txt only
# once every module their use_components/use_rdb calls need has itself been
# add_subdirectory()'d). The JunitTest_unoxml_complex check target from
# Module_unoxml.mk is still not ported (Java-only infrastructure gap, same
# as every other JunitTest_* in this port).
#
# Library_unordf.mk was, until 2026-09-28, a documented, not-yet-attempted
# gap: it needs raptor2/rasqal/redland (librdf), three external RDF
# libraries this port had never vendored or detected (blocking 4 chart2
# CppunitTests and 4 dbaccess CppunitTests via their own use_components
# "unoxml/source/rdf/unordf" call -- see chart2/CMakeLists.txt's and
# dbaccess/CMakeLists.txt's own comments). All three are now vendored (root
# CMakeLists.txt's own "--- raptor2 ---"/"--- rasqal ---"/"--- redland ---"
# sections, prefer-system-fall-back-to-source-build, same shape as harfbuzz/
# ixion/orcus above them), so Library_unordf.mk is ported for real here.

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

# --- Library_unordf (RDF support) --------------------------------------------
#
# Library_unordf.mk, re-checked directly: 4 sources (unoxml/source/rdf/
# CBlankNode, CURI, CLiteral, librdf_repository), use_sdk_api, use_libraries
# (comphelper cppuhelper cppu sal), use_externals(boost_headers librdf
# redland_headers raptor_headers rasqal_headers libxslt libxml2) -- "librdf"
# is the actual link target (LIBRESTUDIO_REDLAND_TARGET, resolved right
# below via librestudio_pkgconfig() -- cmake/LibreStudioTarballs.cmake --
# this module is redland's only real consumer); redland_headers/
# raptor_headers/rasqal_headers are header-only (no direct link -- confirmed
# by grepping this module's own 4 sources: only librdf_repository.cxx
# #includes anything RDF-related, a single `#include <redland.h>`, which
# transitively pulls in raptor2.h/rasqal.h -- see librdf.h's own #include
# chain -- so the compiler needs their include dirs on its search path, but
# the linker does not need them as direct -l flags: redland's own
# INTERFACE_LINK_LIBRARIES chain, set where LIBRESTUDIO_REDLAND_TARGET is
# resolved, already carries raptor2/rasqal as transitive shared-library
# dependencies for whatever finally links against "redland").
# set_componentfile(unordf,unoxml/source/rdf/unordf,services) -> a real
# UNO service registration, distinct from unoxml's own service/unoxml.
# component (confirmed: two separate .component files on disk, verified
# present).
#
# DRIVER omitted: MODULES redland is a single clean element, auto-detected
# as external/redland/redland.cmake. LIBRESTUDIO_FORCE_VENDOR_REDLAND is
# declared once in cmake/LibreStudioTarballs.cmake, alongside its 4 sibling
# force-vendor options -- not re-declared here.
librestudio_pkgconfig(
    MODULES redland
    FORCE_VENDOR_VAR LIBRESTUDIO_FORCE_VENDOR_REDLAND)

set(UNORDF_SOURCES
    unoxml/source/rdf/CBlankNode
    unoxml/source/rdf/CURI
    unoxml/source/rdf/CLiteral
    unoxml/source/rdf/librdf_repository
)
librestudio_expand_cxx(UNORDF_SOURCES)
add_library(unordf SHARED ${UNORDF_SOURCES})

target_include_directories(unordf PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(unordf udkapi_normal_headers offapi_normal_headers)

target_link_libraries(unordf PRIVATE
    comphelper cppuhelper cppu sal
    Boost::boost
    LibXml2::LibXml2 PkgConfig::LIBXSLT
    ${LIBRESTUDIO_REDLAND_TARGET})

librestudio_add_component(unordf
    COMPONENT "${CMAKE_SOURCE_DIR}/unoxml/source/rdf/unordf.component"
    LIBRARY   unordf
    LAYER     OOO)

librestudio_set_library_output_name(unordf unordf OOO)
install(TARGETS unordf LIBRARY DESTINATION program)

# unoxml/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/unoxml/CppunitTest_deferred.cmake")
