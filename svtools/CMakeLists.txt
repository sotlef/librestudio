# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# svtools — the shared GUI helper layer built on top of vcl and tk: the
# browse-box grid, colour/HTML/accelerator configuration, the RTF/HTML mini
# parsers, and a grab-bag of UNO services (file picker, popup menu and status
# bar controllers, the generic wizard shell) that other UI modules build on.
# Source list from Library_svt.mk. Java-only sources (javacontext,
# javainteractionhandler) are skipped: LIBRESTUDIO_ENABLE_JAVA is OFF.

set(SVTOOLS_SOURCES
    svtools/source/brwbox/accessibleeditbrowseboxcell
    svtools/source/brwbox/brwbox1
    svtools/source/brwbox/brwbox2
    svtools/source/brwbox/brwbox3
    svtools/source/brwbox/brwhead
    svtools/source/brwbox/datwin
    svtools/source/brwbox/ebbcontrols
    svtools/source/brwbox/editbrowsebox
    svtools/source/brwbox/editbrowsebox2
    svtools/source/brwbox/recorditemwindow
    svtools/source/config/accessibilityoptions
    svtools/source/config/apearcfg
    svtools/source/config/colorcfg
    svtools/source/config/extcolorcfg
    svtools/source/config/fontsubstconfig
    svtools/source/config/htmlcfg
    svtools/source/config/itemholder2
    svtools/source/config/miscopt
    svtools/source/config/optionsdrawinglayer
    svtools/source/config/printoptions
    svtools/source/contnr/querydelete
    svtools/source/config/viewoptions
    svtools/source/control/accessibleruler
    svtools/source/control/accessibletabbar
    svtools/source/control/accessibletabbarbase
    svtools/source/control/accessibletabbarpage
    svtools/source/control/accessibletabbarpagelist
    svtools/source/control/asynclink
    svtools/source/control/collatorres
    svtools/source/control/ctrlbox
    svtools/source/control/ctrltool
    svtools/source/control/indexentryres
    svtools/source/control/inettbc
    svtools/source/control/ruler
    svtools/source/control/scriptedtext
    svtools/source/control/scrolladaptor
    svtools/source/control/tabbar
    svtools/source/control/toolbarmenu
    svtools/source/control/valueacc
    svtools/source/control/valueset
    svtools/source/dialogs/addresstemplate
    svtools/source/dialogs/dlgname
    svtools/source/dialogs/insdlg
    svtools/source/dialogs/PlaceEditDialog
    svtools/source/dialogs/prnsetup
    svtools/source/dialogs/querydialog
    svtools/source/dialogs/restartdialog
    svtools/source/dialogs/ServerDetailsControls
    svtools/source/filter/SvFilterOptionsDialog
    svtools/source/filter/DocumentToGraphicRenderer
    svtools/source/filter/exportdialog
    svtools/source/graphic/renderer
    svtools/source/misc/acceleratorexecute
    svtools/source/misc/bindablecontrolhelper
    svtools/source/misc/cliplistener
    svtools/source/misc/dialogclosedlistener
    svtools/source/misc/ehdl
    svtools/source/misc/embedhlp
    svtools/source/misc/embedtransfer
    svtools/source/misc/filechangedchecker
    svtools/source/misc/imagemgr
    svtools/source/misc/imageresourceaccess
    svtools/source/misc/langtab
    svtools/source/misc/langhelp
    svtools/source/misc/openfiledroptargetlistener
    svtools/source/misc/sampletext
    svtools/source/misc/stringtransfer
    svtools/source/misc/svtresid
    svtools/source/misc/templatefoldercache
    svtools/source/misc/unitconv
    svtools/source/svhtml/htmlkywd
    svtools/source/svhtml/htmlout
    svtools/source/svhtml/htmlsupp
    svtools/source/svhtml/HtmlWriter
    svtools/source/svhtml/parhtml
    svtools/source/svrtf/parrtf
    svtools/source/svrtf/rtfkeywd
    svtools/source/svrtf/rtfout
    svtools/source/svrtf/svparser
    svtools/source/uno/addrtempuno
    svtools/source/uno/fpicker
    svtools/source/uno/framestatuslistener
    svtools/source/uno/genericunodialog
    svtools/source/uno/miscservices
    svtools/source/uno/popupmenucontrollerbase
    svtools/source/uno/popupwindowcontroller
    svtools/source/uno/statusbarcontroller
    svtools/source/uno/toolboxcontroller
    svtools/source/uno/unoevent
    svtools/source/uno/unoimap
    svtools/source/uno/wizard/unowizard
    svtools/source/uno/wizard/wizardpagecontroller
    svtools/source/uno/wizard/wizardshell
    svtools/source/uitest/uiobject
)

librestudio_expand_cxx(SVTOOLS_SOURCES)
add_library(svt SHARED ${SVTOOLS_SOURCES})

target_include_directories(svt PRIVATE
    "${CMAKE_SOURCE_DIR}/svtools/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(svt officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(svt PRIVATE SVT_DLLIMPLEMENTATION)
target_link_libraries(svt PRIVATE
    basegfx comphelper cppu cppuhelper drawinglayercore i18nlangtag i18nutil
    salhelper sal sot svl tk tl ucbhelper utl vcl
    Boost::boost
    ICU::uc ICU::i18n
    LibXml2::LibXml2)

librestudio_add_component(svt
    COMPONENT "${CMAKE_SOURCE_DIR}/svtools/util/svt.component"
    LIBRARY   svt
    LAYER     OOO)

librestudio_set_library_output_name(svt svt OOO)
install(TARGETS svt LIBRARY DESTINATION program)
