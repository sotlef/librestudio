# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# configmgr — the configuration manager: the service that reads the office's
# configuration, walks it as a tree of nodes, and writes changes back.  The
# files it reads are the .xcd, .xcs and .xcu under officecfg and the modules'
# registry trees.  Source list from Library_configmgr.mk.

set(CONFIGMGR_SOURCES
    configmgr/source/access
    configmgr/source/broadcaster
    configmgr/source/childaccess
    configmgr/source/components
    configmgr/source/configurationprovider
    configmgr/source/configurationregistry
    configmgr/source/data
    configmgr/source/defaultprovider
    configmgr/source/groupnode
    configmgr/source/localizedpropertynode
    configmgr/source/localizedvaluenode
    configmgr/source/lock
    configmgr/source/modifications
    configmgr/source/node
    configmgr/source/nodemap
    configmgr/source/parsemanager
    configmgr/source/partial
    configmgr/source/propertynode
    configmgr/source/readonlyaccess
    configmgr/source/readwriteaccess
    configmgr/source/rootaccess
    configmgr/source/rootnode
    configmgr/source/setnode
    configmgr/source/type
    configmgr/source/update
    configmgr/source/valueparser
    configmgr/source/writemodfile
    configmgr/source/xcdparser
    configmgr/source/xcsparser
    configmgr/source/xcuparser
    configmgr/source/xmldata
    # Library_configmgr.mk adds these two the same conditional way:
    #   $(if $(ENABLE_DCONF), configmgr/source/dconf)
    #   $(if $(filter $(OS),WNT), configmgr/source/winreg)
    # dconf is not installed here, and the Windows registry backend is Windows
    # only.  With neither, the configuration is file-based, which is what the
    # rest of this build expects anyway.
)

librestudio_expand_cxx(CONFIGMGR_SOURCES)
add_library(configmgr SHARED ${CONFIGMGR_SOURCES})

target_include_directories(configmgr PRIVATE
    "${CMAKE_SOURCE_DIR}/configmgr/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(configmgr udkapi_normal_headers offapi_normal_headers)

target_link_libraries(configmgr PRIVATE
    comphelper cppu cppuhelper i18nlangtag sal salhelper tl xmlreader
    Boost::boost)

librestudio_add_component(configmgr
    COMPONENT "${CMAKE_SOURCE_DIR}/configmgr/source/configmgr.component"
    LIBRARY   configmgr
    LAYER     OOO)

librestudio_set_library_output_name(configmgr configmgr OOO)
install(TARGETS configmgr LIBRARY DESTINATION program)
