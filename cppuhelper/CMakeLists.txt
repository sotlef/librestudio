# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# cppuhelper — the C++ helpers on top of cppu: the component/service manager,
# bootstrap, weak references, property sets, and the exception throwing helper.
#
# This module is where the header variants earn their keep.  gbuild asks for
# gb_Library_use_internal_comprehensive_api(cppuhelper, cppuhelper udkapi
# offapi): the "comprehensive" variant inlines the type information of its
# dependencies, which is what lets cppuhelper link and run before any typelib
# exists to resolve types through.

# --- cppuhelper/unotypes: the module's own private type registry -------------
#
# A single IDL file (cppuhelper/detail/XExceptionThrower.idl) living in its own
# registry root, so the entity name comes out as
# cppuhelper.detail.XExceptionThrower.
librestudio_uno_api_rdb(cppuhelper_unotypes
    IDL_ROOT "${CMAKE_SOURCE_DIR}/cppuhelper/unotypes"
    OUTPUT   "${CMAKE_BINARY_DIR}/UnoApiTarget/cppuhelper.rdb"
    DEP_RDBS "${udkapi_RDB}")

librestudio_uno_api_headers(cppuhelper_unotypes_comprehensive
    RDB        "${cppuhelper_unotypes_RDB}"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/UnoApiHeadersTarget/cppuhelper/comprehensive"
    VARIANT    comprehensive
    EXTRA_RDBS "${udkapi_RDB}")

# --- findsofficepath ---------------------------------------------------------

# A small C helper cppuhelper links in to locate the installation directory.
add_library(findsofficepath STATIC
    "${CMAKE_SOURCE_DIR}/cppuhelper/source/findsofficepath.c")

# --- libcppuhelpergcc3 -------------------------------------------------------

set(CPPUHELPER_SOURCES
    cppuhelper/source/access_control
    cppuhelper/source/bootstrap
    cppuhelper/source/compat
    cppuhelper/source/compbase
    cppuhelper/source/component_context
    cppuhelper/source/component
    cppuhelper/source/defaultbootstrap
    cppuhelper/source/exc_thrower
    cppuhelper/source/factory
    cppuhelper/source/implbase
    cppuhelper/source/implbase_ex
    cppuhelper/source/implementationentry
    cppuhelper/source/interfacecontainer
    cppuhelper/source/macro_expander
    cppuhelper/source/paths
    cppuhelper/source/propertysetmixin
    cppuhelper/source/propshlp
    cppuhelper/source/servicemanager
    cppuhelper/source/shlib
    cppuhelper/source/supportsservice
    cppuhelper/source/tdmgr
    cppuhelper/source/typemanager
    cppuhelper/source/typeprovider
    cppuhelper/source/unoimplbase
    cppuhelper/source/unourl
    cppuhelper/source/weak)

librestudio_expand_cxx(CPPUHELPER_SOURCES)

add_library(cppuhelper SHARED ${CPPUHELPER_SOURCES})

target_include_directories(cppuhelper BEFORE PRIVATE
    "${CMAKE_SOURCE_DIR}/cppuhelper/inc"
    "${cppuhelper_unotypes_comprehensive_HEADERS_INCLUDE_DIR}"
    "${udkapi_comprehensive_HEADERS_INCLUDE_DIR}"
    "${offapi_comprehensive_HEADERS_INCLUDE_DIR}")

add_dependencies(cppuhelper
    cppuhelper_unotypes_comprehensive_headers
    udkapi_comprehensive_headers
    offapi_comprehensive_headers)

target_compile_definitions(cppuhelper PRIVATE CPPUHELPER_DLLIMPLEMENTATION)

target_link_libraries(cppuhelper PRIVATE
    findsofficepath
    cppu reg sal salhelper unoidl xmlreader
    Boost::boost)

target_link_options(cppuhelper PRIVATE
    "-Wl,--version-script=${CMAKE_SOURCE_DIR}/cppuhelper/source/gcc3.map")

librestudio_set_library_output_name(cppuhelper cppuhelper RT)

install(TARGETS cppuhelper LIBRARY DESTINATION program)
