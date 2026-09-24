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
# The Java half of the module (javaloader, javavm) is not built: it needs
# jvmaccess and jvmfwk.  LIBRESTUDIO_ENABLE_JAVA is meant to bring those in
# later.

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
