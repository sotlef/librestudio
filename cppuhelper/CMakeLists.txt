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

if (ENABLE_PCH)
    target_precompile_headers(cppuhelper PRIVATE
        "${CMAKE_SOURCE_DIR}/cppuhelper/inc/pch/precompiled_cppuhelper.hxx")
    target_compile_definitions(cppuhelper PRIVATE PCH_LEVEL=2)
endif ()

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

# Library_cppuhelper.mk:
#   ifeq ($(OS),iOS)
#   $(eval $(call gb_Library_add_cxxflags,cppuhelper, $(gb_OBJCXXFLAGS) ))
#   endif
#
# gb_OBJCXXFLAGS is a real, confirmed value, not a guess: it is defined once
# per platform file, and solenv/gbuild/platform/iOS.mk:60 sets it to exactly
# `-x objective-c++` (no -fobjc-exceptions -- that extra flag is only added
# by the sibling macosx.mk:59 definition, which does not apply here). This
# is structurally present but inert on this Linux build (CMAKE_SYSTEM_NAME
# is never "iOS" here), same status as the rest of this port's iOS/Android/
# Windows branches -- see sal/CMakeLists.txt's own header comment.
if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_compile_options(cppuhelper PRIVATE "-x" "objective-c++")
endif ()

target_link_libraries(cppuhelper PRIVATE
    findsofficepath
    cppu reg sal salhelper unoidl xmlreader
    Boost::boost)

target_link_options(cppuhelper PRIVATE
    "-Wl,--version-script=${CMAKE_SOURCE_DIR}/cppuhelper/source/gcc3.map")

librestudio_set_library_output_name(cppuhelper cppuhelper RT)

install(TARGETS cppuhelper LIBRARY DESTINATION program)

# --- CppunitTest targets ------------------------------------------------------
#
# cppuhelper's 4 CppunitTest_*.mk files, ported via
# librestudio_add_cppunit_test() (cmake/LibreStudioTest.cmake). All 4 were
# re-checked directly against their live .mk files before wiring. None of
# them call use_internal_comprehensive_api or use_ure -- gb_CppunitTest_use_api
# here always means the plain "normal" udkapi variant (see cppu/CMakeLists.txt's
# CppunitTest section header comment for why use_api resolves to "normal", not
# "comprehensive").

# CppunitTest_cppuhelper_cppu_ifcontainer.mk: 1 source, use_libraries cppu
# cppuhelper sal, use_api udkapi -> plain udkapi.
librestudio_add_cppunit_test(cppuhelper_cppu_ifcontainer
    SOURCES
        cppuhelper/qa/ifcontainer/cppu_ifcontainer
    LIBRARIES cppu cppuhelper sal
    INCLUDE "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers)

# CppunitTest_cppuhelper_cppu_unourl.mk: 1 source, use_libraries cppu
# cppuhelper sal, no use_api call at all -- confirmed by reading the .mk file
# in full, matching cppuhelper/qa/unourl/cppu_unourl.cxx's own #include list
# (cppuhelper/unourl.hxx, rtl/*, sal/types.h -- nothing generated). No API
# header wiring needed for this one.
librestudio_add_cppunit_test(cppuhelper_cppu_unourl
    SOURCES
        cppuhelper/qa/unourl/cppu_unourl
    LIBRARIES cppu cppuhelper sal)

# CppunitTest_cppuhelper_qa_misc.mk: 1 source, use_libraries cppu cppuhelper
# sal, use_api udkapi -> plain udkapi.
librestudio_add_cppunit_test(cppuhelper_qa_misc
    SOURCES
        cppuhelper/qa/misc/test_misc
    LIBRARIES cppu cppuhelper sal
    INCLUDE "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers)

# CppunitTest_cppuhelper_qa_weak.mk: 1 source, use_libraries cppu cppuhelper
# sal, use_api udkapi -> plain udkapi.
librestudio_add_cppunit_test(cppuhelper_qa_weak
    SOURCES
        cppuhelper/qa/weak/test_weak
    LIBRARIES cppu cppuhelper sal
    INCLUDE "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers)
