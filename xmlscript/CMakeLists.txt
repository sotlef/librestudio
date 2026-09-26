# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# xmlscript — import and export of the XML-based Basic libraries and dialogs:
# the .xdl dialog descriptions, the library containers and the module tree.
# Source list from Library_xmlscript.mk.

set(XMLSCRIPT_SOURCES
    xmlscript/source/xml_helper/xml_byteseq
    xmlscript/source/xml_helper/xml_element
    xmlscript/source/xml_helper/xml_impctx
    xmlscript/source/xmldlg_imexp/xmldlg_addfunc
    xmlscript/source/xmldlg_imexp/xmldlg_expmodels
    xmlscript/source/xmldlg_imexp/xmldlg_export
    xmlscript/source/xmldlg_imexp/xmldlg_impmodels
    xmlscript/source/xmldlg_imexp/xmldlg_import
    xmlscript/source/xmlflat_imexp/xmlbas_export
    xmlscript/source/xmllib_imexp/xmllib_export
    xmlscript/source/xmllib_imexp/xmllib_import
    xmlscript/source/xmlmod_imexp/xmlmod_export
    xmlscript/source/xmlmod_imexp/xmlmod_import
)

librestudio_expand_cxx(XMLSCRIPT_SOURCES)
add_library(xmlscript SHARED ${XMLSCRIPT_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(xmlscript PRIVATE
        "${CMAKE_SOURCE_DIR}/xmlscript/inc/pch/precompiled_xmlscript.hxx")
    target_compile_definitions(xmlscript PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(xmlscript PRIVATE
    "${CMAKE_SOURCE_DIR}/xmlscript/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(xmlscript udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(xmlscript PRIVATE XMLSCRIPT_DLLIMPLEMENTATION)
target_link_libraries(xmlscript PRIVATE
    comphelper cppu cppuhelper i18nlangtag sal tl
    Boost::boost)

librestudio_add_component(xmlscript
    COMPONENT "${CMAKE_SOURCE_DIR}/xmlscript/util/xmlscript.component"
    LIBRARY   xmlscript
    LAYER     OOO)

librestudio_set_library_output_name(xmlscript xmlscript OOO)
install(TARGETS xmlscript LIBRARY DESTINATION program)

# Package_dtd.mk: the four DTDs xmlscript's own XML formats are validated
# against, packaged verbatim into share/dtd/officedocument/1_0. Plain
# install(FILES ...), this port's established convention for packaging a
# handful of static files (see e.g. udkapi/CMakeLists.txt, officecfg/
# CMakeLists.txt's own Package_misc) -- no generic Package helper needed for
# four files.
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/dtd/dialog.dtd"
    "${CMAKE_CURRENT_SOURCE_DIR}/dtd/library.dtd"
    "${CMAKE_CURRENT_SOURCE_DIR}/dtd/libraries.dtd"
    "${CMAKE_CURRENT_SOURCE_DIR}/dtd/module.dtd"
    DESTINATION share/dtd/officedocument/1_0)

# CppunitTest_xmlscript_cppunit.mk: registered in xmlscript/CppunitTest_
# deferred.cmake, NOT here, include()'d from the root CMakeLists.txt only
# after program_services_rdb exists (use_rdb(services)) and framework/test
# are add_subdirectory()'d (use_libraries fwk / use_ure+use_vcl's
# protectors) -- see that file for the full detail.
