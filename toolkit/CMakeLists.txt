# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# toolkit — the UNO AWT peer implementation: VCLXWindow and its subclasses wrap
# vcl widgets as UNO services, plus the peer factory (vclxtoolkit) that svtools
# and every other UI-building module asks for widgets through.  Source list
# from Library_tk.mk; the CppunitTest_toolkit* check targets are not ported.

set(TOOLKIT_SOURCES
    toolkit/source/awt/asynccallback
    toolkit/source/awt/stylesettings
    toolkit/source/awt/scrollabledialog
    toolkit/source/awt/vclxbitmap
    toolkit/source/awt/vclxcontainer
    toolkit/source/awt/vclxdevice
    toolkit/source/awt/vclxfont
    toolkit/source/awt/vclxfontvariations
    toolkit/source/awt/vclxgraphics
    toolkit/source/awt/vclxmenu
    toolkit/source/awt/vclxpointer
    toolkit/source/awt/vclxprinter
    toolkit/source/awt/vclxregion
    toolkit/source/awt/vclxspinbutton
    toolkit/source/awt/vclxsystemdependentwindow
    toolkit/source/awt/vclxtabpagecontainer
    toolkit/source/awt/animatedimagespeer
    toolkit/source/awt/vclxtoolkit
    toolkit/source/awt/vclxtopwindow
    toolkit/source/awt/vclxwindow
    toolkit/source/awt/vclxwindow1
    toolkit/source/awt/vclxwindows
    toolkit/source/controls/accessiblecontrolcontext
    toolkit/source/controls/controlmodelcontainerbase
    toolkit/source/controls/dialogcontrol
    toolkit/source/controls/eventcontainer
    toolkit/source/controls/filectrl
    toolkit/source/controls/formattedcontrol
    toolkit/source/controls/geometrycontrolmodel
    toolkit/source/controls/grid/defaultgridcolumnmodel
    toolkit/source/controls/grid/defaultgriddatamodel
    toolkit/source/controls/grid/gridcolumn
    toolkit/source/controls/grid/grideventforwarder
    toolkit/source/controls/grid/sortablegriddatamodel
    toolkit/source/controls/grid/gridcontrol
    toolkit/source/controls/roadmapcontrol
    toolkit/source/controls/roadmapentry
    toolkit/source/controls/svtxgridcontrol
    toolkit/source/controls/svmedit
    toolkit/source/controls/tabpagecontainer
    toolkit/source/controls/tabpagemodel
    toolkit/source/controls/stdtabcontroller
    toolkit/source/controls/stdtabcontrollermodel
    toolkit/source/controls/tkscrollbar
    toolkit/source/controls/tkspinbutton
    toolkit/source/controls/animatedimages
    toolkit/source/controls/spinningprogress
    toolkit/source/controls/table/AccessibleGridControl
    toolkit/source/controls/table/AccessibleGridControlBase
    toolkit/source/controls/table/AccessibleGridControlHeader
    toolkit/source/controls/table/AccessibleGridControlHeaderCell
    toolkit/source/controls/table/AccessibleGridControlTable
    toolkit/source/controls/table/AccessibleGridControlTableBase
    toolkit/source/controls/table/AccessibleGridControlTableCell
    toolkit/source/controls/table/cellvalueconversion
    toolkit/source/controls/table/defaultinputhandler
    toolkit/source/controls/table/gridtablerenderer
    toolkit/source/controls/table/mousefunction
    toolkit/source/controls/table/TableControl
    toolkit/source/controls/table/tabledatawindow
    toolkit/source/controls/table/TableFunctionSet
    toolkit/source/controls/table/tablegeometry
    toolkit/source/controls/tree/treecontrol
    toolkit/source/controls/tree/treecontrolpeer
    toolkit/source/controls/tree/treedatamodel
    toolkit/source/controls/unocontrol
    toolkit/source/controls/unocontrolbase
    toolkit/source/controls/unocontrolcontainer
    toolkit/source/controls/unocontrolcontainermodel
    toolkit/source/controls/unocontrolmodel
    toolkit/source/controls/unocontroltablemodel
    toolkit/source/controls/unocontrols
    toolkit/source/controls/unogridcolumnfacade
    toolkit/source/hatchwindow/documentcloser
    toolkit/source/hatchwindow/hatchwindow
    toolkit/source/hatchwindow/hatchwindowfactory
    toolkit/source/hatchwindow/ipwin
    toolkit/source/helper/btndlg
    toolkit/source/helper/imagealign
    toolkit/source/helper/listenermultiplexer
    toolkit/source/helper/property
    toolkit/source/helper/tkresmgr
    toolkit/source/helper/unopropertyarrayhelper
    toolkit/source/helper/unowrapper
    toolkit/source/helper/vclunohelper
)

librestudio_expand_cxx(TOOLKIT_SOURCES)
add_library(tk SHARED ${TOOLKIT_SOURCES})

target_include_directories(tk PRIVATE
    "${CMAKE_SOURCE_DIR}/toolkit/inc"
    "${CMAKE_SOURCE_DIR}/toolkit/source"
    "${OFFICECFG_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(tk officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(tk PRIVATE TOOLKIT_DLLIMPLEMENTATION)
target_link_libraries(tk PRIVATE
    comphelper cppu cppuhelper i18nlangtag sal salhelper svl tl utl vcl
    Boost::boost)

librestudio_add_component(tk
    COMPONENT "${CMAKE_SOURCE_DIR}/toolkit/util/tk.component"
    LIBRARY   tk
    LAYER     OOO)

librestudio_set_library_output_name(tk tk OOO)
install(TARGETS tk LIBRARY DESTINATION program)
