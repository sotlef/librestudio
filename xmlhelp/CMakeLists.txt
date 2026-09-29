# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# xmlhelp (module xmlhelp) -- one library, from Library_ucpchelp1.mk.
#
#   ucpchelp1 (SHARED) -- a UCB content provider (CHelpContentProvider,
#               services com.sun.star.help.XMLHelp / .ucb.HelpContentProvider)
#               that serves the linked/indexed help tree produced by
#               helpcompiler's HelpLinker/HelpIndexer at doc-build time
#               (xmlhelp/source/cxxhelp/provider), plus a second UNO service,
#               com.sun.star.help.TreeViewImpl (services
#               com.sun.star.help.TreeView / .ucb.HierarchyDataSource), that
#               reads the help contents tree for the Help navigation pane's
#               tree view (xmlhelp/source/treeview). Both share the private
#               header directory xmlhelp/source/cxxhelp/inc.
#
# Not ported: Package_helpxsl.mk. It only copies three static XSL files
# (idxcaption.xsl, idxcontent.xsl, main_transform.xsl) into the install
# tree -- no compiled code, and this port has never ported a gb_Package_*
# target anywhere (see e.g. pyuno/CMakeLists.txt, sdext/CMakeLists.txt,
# slideshow/CMakeLists.txt, jvmfwk/CMakeLists.txt, oox/CMakeLists.txt,
# test/CMakeLists.txt). These XSLs only matter to the separate,
# not-yet-started help-content-generation pipeline that consumes them at
# doc-build time; ucpchelp1 itself does not compile or link against them.

set(UCPCHELP1_SOURCES
    xmlhelp/source/cxxhelp/provider/content
    xmlhelp/source/cxxhelp/provider/contentcaps
    xmlhelp/source/cxxhelp/provider/databases
    xmlhelp/source/cxxhelp/provider/db
    xmlhelp/source/cxxhelp/provider/provider
    xmlhelp/source/cxxhelp/provider/resultset
    xmlhelp/source/cxxhelp/provider/resultsetbase
    xmlhelp/source/cxxhelp/provider/resultsetforquery
    xmlhelp/source/cxxhelp/provider/resultsetforroot
    xmlhelp/source/cxxhelp/provider/urlparameter
    xmlhelp/source/treeview/tvfactory
    xmlhelp/source/treeview/tvread
)

librestudio_expand_cxx(UCPCHELP1_SOURCES)
add_library(ucpchelp1 SHARED ${UCPCHELP1_SOURCES})

target_include_directories(ucpchelp1 PRIVATE
    "${CMAKE_SOURCE_DIR}/xmlhelp/source/cxxhelp/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(ucpchelp1 officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(ucpchelp1 PRIVATE
    comphelper cppu cppuhelper helplinker sal tl ucbhelper utl i18nlangtag vcl
    EXPAT::EXPAT
    LibXml2::LibXml2
    PkgConfig::LIBXSLT
    ${LIBRESTUDIO_CLUCENE_TARGET})

librestudio_add_component(ucpchelp1
    COMPONENT "${CMAKE_SOURCE_DIR}/xmlhelp/util/ucpchelp1.component"
    LIBRARY   ucpchelp1
    LAYER     OOO)

librestudio_set_library_output_name(ucpchelp1 ucpchelp1 OOO)

install(TARGETS ucpchelp1 LIBRARY DESTINATION program)
