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
