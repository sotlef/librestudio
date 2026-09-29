# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# toolkit — the UNO AWT peer implementation: VCLXWindow and its subclasses wrap
# vcl widgets as UNO services, plus the peer factory (vclxtoolkit) that svtools
# and every other UI-building module asks for widgets through.  Source list
# from Library_tk.mk. Both CppunitTest_toolkit (toolkit_toolkit) and
# CppunitTest_toolkit_a11y (toolkit_a11y) are ported, deferred to toolkit/
# CppunitTest_deferred.cmake (see that file for why neither is registered
# here directly).

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

# Library_tk.mk's platform tail (verified against the live file):
#   ifeq ($(OS),MACOSX)
#   $(eval $(call gb_Library_add_cxxflags,tk,$(gb_OBJCXXFLAGS)))
#   $(eval $(call gb_Library_add_libs,tk,-lobjc))
#   endif
#   ifeq ($(OS),iOS)
#   $(eval $(call gb_Library_add_cxxflags,tk,$(gb_OBJCXXFLAGS)))
#   endif
# gb_OBJCXXFLAGS differs per platform.mk: solenv/gbuild/platform/macosx.mk
# defines it as "-x objective-c++ -fobjc-exceptions" (line 59), solenv/gbuild/
# platform/iOS.mk as plain "-x objective-c++" (line 60, no -fobjc-exceptions)
# -- the same file this port's cppuhelper iOS branch already sourced its own
# gb_OBJCXXFLAGS value from earlier this session. Neither branch is
# exercised on this Linux port; wired for completeness only.
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_options(tk PRIVATE -x objective-c++ -fobjc-exceptions)
    target_link_libraries(tk PRIVATE objc)
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_compile_options(tk PRIVATE -x objective-c++)
endif ()

librestudio_add_component(tk
    COMPONENT "${CMAKE_SOURCE_DIR}/toolkit/util/tk.component"
    LIBRARY   tk
    LAYER     OOO)

librestudio_set_library_output_name(tk tk OOO)
install(TARGETS tk LIBRARY DESTINATION program)

# toolkit/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/toolkit/CppunitTest_deferred.cmake")
