# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# stoc — the component model that makes UNO services loadable: the service
# manager, the shared-library loader, the type and implementation registries,
# reflection, invocation and the URI processors.
#
# Unlike most modules this one is not a single library: upstream splits it into
# eight, each self-contained and each declaring its own .component
# (the Library_*.mk files in this directory).  They differ only in their source
# list and link libraries, so they share a helper.
#
# The Java half of the module (javaloader, javavm) is added at the bottom of
# this file, gated behind LIBRESTUDIO_ENABLE_JAVA: both need jvmaccess and
# javavm additionally needs jvmfwk (see jvmfwk/CMakeLists.txt,
# jvmaccess/CMakeLists.txt), which are themselves only built under the same
# option.

# stoc_add_library(<name> <header-variant> COMPONENT <file> SOURCES <sources...>)
#
# variants: "normal" for gb_Library_use_udk_api, "bootstrap" for
# gb_Library_use_internal_bootstrap_api — the latter is used by libbootstrap
# alone, which has to work before any typelib exists and therefore takes the
# header variant carrying full static type information.
function(stoc_add_library name variant)
    cmake_parse_arguments(_stoc "" "COMPONENT" "LIBS;SOURCES" ${ARGN})

    librestudio_expand_cxx(_stoc_SOURCES)
    add_library(${name} SHARED ${_stoc_SOURCES})

    target_include_directories(${name} PRIVATE
        "${udkapi_${variant}_HEADERS_INCLUDE_DIR}")
    add_dependencies(${name} udkapi_${variant}_headers)

    target_link_libraries(${name} PRIVATE ${_stoc_LIBS} Boost::boost)

    librestudio_set_library_output_name(${name} ${name} OOO)
    install(TARGETS ${name} LIBRARY DESTINATION program)

    # gb_Library_set_componentfile(<name>, <path>, ure/services)
    librestudio_add_component(${name}
        COMPONENT "${CMAKE_SOURCE_DIR}/${_stoc_COMPONENT}.component"
        LIBRARY   ${name}
        LAYER     URELIB)
endfunction()

stoc_add_library(bootstrap bootstrap
    COMPONENT stoc/util/bootstrap
    LIBS cppu cppuhelper reg sal salhelper
    SOURCES
        stoc/source/defaultregistry/defaultregistry
        stoc/source/implementationregistration/implreg
        stoc/source/implementationregistration/mergekeys
        stoc/source/loader/dllcomponentloader
        stoc/source/security/access_controller
        stoc/source/security/file_policy
        stoc/source/security/permissions
        stoc/source/servicemanager/servicemanager
        stoc/source/simpleregistry/simpleregistry)

stoc_add_library(introspection normal
    COMPONENT stoc/source/inspect/introspection
    LIBS cppu cppuhelper sal salhelper
    SOURCES stoc/source/inspect/introspection)

stoc_add_library(invocadapt normal
    COMPONENT stoc/source/invocation_adapterfactory/invocadapt
    LIBS cppu cppuhelper sal
    SOURCES stoc/source/invocation_adapterfactory/iafactory)

stoc_add_library(invocation normal
    COMPONENT stoc/source/invocation/invocation
    LIBS cppu cppuhelper sal
    SOURCES stoc/source/invocation/invocation)

stoc_add_library(namingservice normal
    COMPONENT stoc/source/namingservice/namingservice
    LIBS cppu cppuhelper sal
    SOURCES stoc/source/namingservice/namingservice)

stoc_add_library(proxyfac normal
    COMPONENT stoc/source/proxy_factory/proxyfac
    LIBS cppu cppuhelper sal
    SOURCES stoc/source/proxy_factory/proxyfac)

stoc_add_library(reflection normal
    COMPONENT stoc/source/corereflection/reflection
    LIBS cppu cppuhelper sal
    SOURCES
        stoc/source/corereflection/crarray
        stoc/source/corereflection/crbase
        stoc/source/corereflection/crcomp
        stoc/source/corereflection/crefl
        stoc/source/corereflection/crenum
        stoc/source/corereflection/criface
        stoc/source/corereflection/dump)

stoc_add_library(stocservices normal
    COMPONENT stoc/util/stocservices
    LIBS cppu cppuhelper sal
    SOURCES
        stoc/source/typeconv/convert
        stoc/source/uriproc/ExternalUriReferenceTranslator
        stoc/source/uriproc/UriReference
        stoc/source/uriproc/UriReferenceFactory
        stoc/source/uriproc/UriSchemeParser_vndDOTsunDOTstarDOTexpand
        stoc/source/uriproc/UriSchemeParser_vndDOTsunDOTstarDOTscript
        stoc/source/uriproc/VndSunStarPkgUrlReferenceFactory)

# javaloader and javavm are the two Library_*.mk that make the JVM reachable
# as UNO services; both need jvmaccess (started via jvmfwk for javavm), so
# they can only exist once LIBRESTUDIO_ENABLE_JAVA has pulled those in (see
# the add_subdirectory gate in the root CMakeLists.txt).
if (LIBRESTUDIO_ENABLE_JAVA)

stoc_add_library(javaloader normal
    COMPONENT stoc/source/javaloader/javaloader
    LIBS cppu cppuhelper jvmaccess sal salhelper
    SOURCES stoc/source/javaloader/javaloader)

stoc_add_library(javavm normal
    COMPONENT stoc/source/javavm/javavm
    LIBS cppu cppuhelper i18nlangtag jvmaccess jvmfwk sal salhelper
    SOURCES
        stoc/source/javavm/interact
        stoc/source/javavm/javavm
        stoc/source/javavm/jvmargs)

# javaloader.cxx and javavm's own headers (javavm.hxx, jvmargs.hxx) include
# <jni.h> directly; jvmaccess/jvmfwk only carry JNI_INCLUDE_DIRS as PRIVATE
# (see jvmaccess/CMakeLists.txt, jvmfwk/CMakeLists.txt), which does not
# propagate through target_link_libraries, so both need it added here too.
target_include_directories(javaloader PRIVATE "${JNI_INCLUDE_DIRS}")
target_include_directories(javavm PRIVATE "${JNI_INCLUDE_DIRS}")

# Library_javavm.mk: ifeq ($(OS),MACOSX) $(eval $(call
# gb_Library_use_system_darwin_frameworks,javavm,CoreFoundation)) endif --
# macOS-only. Kept structurally present, behind if (APPLE), per this port's
# policy of not silently dropping platform-only gbuild targets -- see sal/
# CMakeLists.txt's sal_comtools and cpputools/CMakeLists.txt's sp2bv for the
# same pattern (Windows there, macOS here). Inert on this Linux build.
if (APPLE)
    target_link_libraries(javavm PRIVATE "-framework CoreFoundation")
endif ()

endif ()

# --- CppunitTest targets ------------------------------------------------------
#
# stoc's 2 CppunitTest_*.mk files, ported via librestudio_add_cppunit_test()
# (cmake/LibreStudioTest.cmake -- read its header comment first). Both
# re-checked directly against their live .mk files (not just the
# prior-research summary) -- neither "stoc_dump" nor "stoc_uriproc" collides
# with any of the 8 stoc_add_library() target names above or anything else in
# the tree (grepped), so no _test-suffix rename is needed.

# CppunitTest_stoc_dump.mk: 1 source, use_libraries cppu cppuhelper sal,
# use_internal_api(cppu_qa_cppumaker_types) -- NOT use_internal_comprehensive_api
# (the live .mk calls plain gb_CppunitTest_use_internal_api). That forwards
# unprefixed to gb_LinkTarget_use_internal_api (solenv/gbuild/LinkTarget.mk:
# 1260-1270), which resolves to the plain/-L "normal" header variant
# (gb_UnoApiHeadersTarget_get_target/get_dir, empty prefix), NOT the
# "comprehensive_"-prefixed one cppu_test_cppumaker uses
# (gb_CppunitTest_use_internal_comprehensive_api, LinkTarget.mk:1278-1282).
# cppu/CMakeLists.txt previously only generated the comprehensive variant of
# cppu_qa_cppumaker_types (its only other consumer); a normal-variant
# librestudio_uno_api_headers() (cppu_qa_cppumaker_types_normal) was added
# there for this test. use_udk_api -> plain udkapi, as everywhere else in this
# port. use_ure adds PROTECTORS as usual; its extra line --
#   $(call gb_CppunitTest_get_target,stoc_dump): \
#       UNO_TYPES += $(call gb_UnoApiTarget_get_target,cppu_qa_cppumaker_types)
# -- feeds the raw .rdb (not headers) into the runtime UNO_TYPES registry,
# which librestudio_add_cppunit_test() does not model at all (see
# cmake/LibreStudioTest.cmake's file header and cppu_typelib's own comment in
# cppu/CMakeLists.txt for the same already-accepted gap) -- not implemented
# here either, for the same reason.
librestudio_add_cppunit_test(stoc_dump
    SOURCES
        stoc/test/dump
    LIBRARIES cppu cppuhelper sal
    INCLUDE
        "${cppu_qa_cppumaker_types_normal_HEADERS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS cppu_qa_cppumaker_types_normal_headers udkapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector)

# CppunitTest_stoc_uriproc.mk: 1 source, use_libraries cppu cppuhelper sal,
# use_udk_api -> plain udkapi, use_ure -> PROTECTORS. Also has a manual
# dependency on stoc/util/stocservices's ComponentTarget --
#   $(call gb_CppunitTest_get_target,stoc_uriproc): \
#       $(call gb_ComponentTarget_get_target,stoc/util/stocservices)
# -- the .mk's own comment explains this is deliberately NOT
# gb_CppunitTest_use_component (which would register it as a UNO_SERVICES
# entry and throw a "duplicate implementation" DeploymentException in a
# module-only build), just a plain build-order file dependency so the
# .component this test's own uriproc test code loads is guaranteed to exist.
# stocservices is the "stocservices" stoc_add_library() call above
# (COMPONENT stoc/util/stocservices), whose librestudio_add_component() call
# registers a "stocservices_component" custom target
# (cmake/LibreStudioComponent.cmake) -- that is wired in below via DEPENDS
# (build-order only), never as a link dependency, matching the .mk's own
# distinction.
librestudio_add_cppunit_test(stoc_uriproc
    SOURCES
        stoc/test/uriproc/test_uriproc
    LIBRARIES cppu cppuhelper sal
    INCLUDE "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers stocservices_component
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector)
