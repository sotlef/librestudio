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
)

# Library_configmgr.mk adds these two the same conditional way:
#   $(if $(ENABLE_DCONF), configmgr/source/dconf)
#   $(if $(filter $(OS),WNT), configmgr/source/winreg)
# ENABLE_DCONF is detected in LibreStudioConfigHeaders.cmake (autodetected via
# pkg-config, the same "on when the library is found" rule configure.ac
# applies); dconf is not installed on this machine, so this branch is dead
# here but wired up for one that has it.  WIN32 is this build's equivalent of
# configure.ac's $(OS) = WNT.
if (ENABLE_DCONF)
    list(APPEND CONFIGMGR_SOURCES configmgr/source/dconf)
endif ()

if (WIN32)
    # Library_configmgr.mk's use_externals names nothing WNT-specific
    # alongside this source file, so there is no extra Windows link library
    # to add here.
    list(APPEND CONFIGMGR_SOURCES configmgr/source/winreg)
endif ()

librestudio_expand_cxx(CONFIGMGR_SOURCES)
add_library(configmgr SHARED ${CONFIGMGR_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(configmgr PRIVATE
        "${CMAKE_SOURCE_DIR}/configmgr/inc/pch/precompiled_configmgr.hxx")
    target_compile_definitions(configmgr PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(configmgr PRIVATE
    "${CMAKE_SOURCE_DIR}/configmgr/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(configmgr udkapi_normal_headers offapi_normal_headers)

target_link_libraries(configmgr PRIVATE
    comphelper cppu cppuhelper i18nlangtag sal salhelper tl xmlreader
    Boost::boost)

if (ENABLE_DCONF)
    # Library_configmgr.mk's use_externals pulls in "dconf", which
    # RepositoryExternal.mk's gb_LinkTarget__use_dconf resolves to plain
    # DCONF_CFLAGS/DCONF_LIBS from configure.ac's
    # PKG_CHECK_MODULES([DCONF], [dconf >= 0.40.0]) -- i.e. just the "dconf"
    # pkg-config module itself, nothing else named alongside it.
    pkg_check_modules(DCONF_LIB REQUIRED IMPORTED_TARGET dconf)
    target_link_libraries(configmgr PRIVATE PkgConfig::DCONF_LIB)
endif ()

librestudio_add_component(configmgr
    COMPONENT "${CMAKE_SOURCE_DIR}/configmgr/source/configmgr.component"
    LIBRARY   configmgr
    LAYER     OOO)

librestudio_set_library_output_name(configmgr configmgr OOO)
install(TARGETS configmgr LIBRARY DESTINATION program)

# CppunitTest_configmgr_unit.mk: registered in configmgr/CppunitTest_
# deferred.cmake, NOT here, include()'d from the root CMakeLists.txt only
# after unoxml/ucb/test are all add_subdirectory()'d (use_components on
# unoxml/ucb1/ucpfile1, use_libraries on test_fixtures + its protector
# plug-ins) -- see that file for the full detail.

# configmgr/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/configmgr/CppunitTest_deferred.cmake")
