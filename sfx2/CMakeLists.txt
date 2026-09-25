# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sfx2 (module sfx) — the application framework: SfxObjectShell/SfxViewFrame
# document-shell machinery, the sidebar, the backing window/Start Center, the
# slot/dispatch (SID) system whose IDs are compiled by svidl from sfx2/sdi/*.sdi.
# Source list from Library_sfx.mk.

librestudio_add_sdi_target(sfx2_sdi
    SDI        "${CMAKE_SOURCE_DIR}/sfx2/sdi/sfxslots.sdi"
    EXPORTS    "${CMAKE_SOURCE_DIR}/sfx2/sdi/sfx.sdi"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/SdiTarget/sfx2/sdi")

set(SFX_SOURCES
    # ifneq ($(ENABLE_WASM_STRIP_RECENT),TRUE) -- applies unconditionally, the
    # flag is unset in this build.
    sfx2/source/dialog/backingcomp
    sfx2/source/dialog/backingwindow
    sfx2/source/control/recentdocsview
    sfx2/source/control/recentdocsviewitem

    sfx2/source/accessibility/AccessibilityCheck
    sfx2/source/accessibility/AccessibilityIssue
    sfx2/source/appl/app
    sfx2/source/appl/appbas
    sfx2/source/appl/appbaslib
    sfx2/source/appl/appcfg
    sfx2/source/appl/appchild
    sfx2/source/appl/appdata
    sfx2/source/appl/appdde
    sfx2/source/appl/appdispatchprovider
    sfx2/source/appl/appinit
    sfx2/source/appl/appmain
    sfx2/source/appl/appmisc
    sfx2/source/appl/appopen
    sfx2/source/appl/appquit
    sfx2/source/appl/appreg
    sfx2/source/appl/appserv
    sfx2/source/appl/appuno
    sfx2/source/appl/childwin
    sfx2/source/appl/fileobj
    sfx2/source/appl/flatpak
    sfx2/source/appl/fwkhelper
    sfx2/source/appl/getbasctlfunction
    sfx2/source/appl/helpdispatch
    sfx2/source/appl/helpinterceptor
    sfx2/source/appl/impldde
    sfx2/source/appl/linkmgr2
    sfx2/source/appl/linksrc
    sfx2/source/appl/lnkbase2
    sfx2/source/appl/macroloader
    sfx2/source/appl/module
    sfx2/source/appl/newhelp
    sfx2/source/appl/opengrf
    sfx2/source/appl/openuriexternally
    sfx2/source/appl/preventduplicateinteraction
    sfx2/source/appl/sfxhelp
    sfx2/source/appl/sfxpicklist
    sfx2/source/appl/shutdownicon
    sfx2/source/appl/workwin
    sfx2/source/appl/xpackcreator
    sfx2/source/bastyp/bitset
    sfx2/source/bastyp/fltfnc
    sfx2/source/bastyp/fltlst
    sfx2/source/bastyp/frmhtml
    sfx2/source/bastyp/frmhtmlw
    sfx2/source/bastyp/helper
    sfx2/source/bastyp/mieclip
    sfx2/source/bastyp/progress
    sfx2/source/bastyp/sfxhtml
    sfx2/source/bastyp/sfxresid
    sfx2/source/commandpopup/CommandPopup
    sfx2/source/config/evntconf
    sfx2/source/control/bindings
    sfx2/source/control/ctrlitem
    sfx2/source/control/dispatch
    sfx2/source/control/minfitem
    sfx2/source/control/msg
    sfx2/source/control/msgpool
    sfx2/source/control/objface
    sfx2/source/control/request
    sfx2/source/control/sfxstatuslistener
    sfx2/source/control/shell
    sfx2/source/control/sorgitm
    sfx2/source/control/statcach
    sfx2/source/control/templatedefaultview
    sfx2/source/control/templateviewitem
    sfx2/source/control/templatelocalview
    sfx2/source/control/templatecontaineritem
    sfx2/source/control/templatedlglocalview
    sfx2/source/control/listview
    sfx2/source/control/thumbnailviewitem
    sfx2/source/control/thumbnailviewacc
    sfx2/source/control/thumbnailviewitemacc
    sfx2/source/control/thumbnailview
    sfx2/source/control/charmapcontainer
    sfx2/source/control/charmapcontrol
    sfx2/source/control/uiobject
    sfx2/source/control/unoctitm
    sfx2/source/devtools/DevelopmentToolChildWindow
    sfx2/source/devtools/DevelopmentToolDockingWindow
    sfx2/source/devtools/DocumentModelTreeHandler
    sfx2/source/devtools/ObjectInspectorTreeHandler
    sfx2/source/devtools/ObjectInspectorWidgets
    sfx2/source/dialog/AdditionsDialogHelper
    sfx2/source/dialog/SfxTabPage
    sfx2/source/dialog/basedlgs
    sfx2/source/dialog/checkin
    sfx2/source/dialog/dialoghelper
    sfx2/source/dialog/charmappopup
    sfx2/source/dialog/dinfdlg
    sfx2/source/dialog/dockwin
    sfx2/source/dialog/documentfontsdialog
    sfx2/source/dialog/filedlghelper
    sfx2/source/dialog/filtergrouping
    sfx2/source/dialog/infobar
    sfx2/source/dialog/inputdlg
    sfx2/source/dialog/mailmodel
    sfx2/source/dialog/bluthsnd
    sfx2/source/dialog/mgetempl
    sfx2/source/dialog/navigat
    sfx2/source/dialog/quickfind
    sfx2/source/dialog/newstyle
    sfx2/source/dialog/partwnd
    sfx2/source/dialog/passwd
    sfx2/source/dialog/printopt
    sfx2/source/dialog/recfloat
    sfx2/source/dialog/securitypage
    sfx2/source/dialog/sfxdlg
    sfx2/source/dialog/splitwin
    sfx2/source/dialog/srchdlg
    sfx2/source/dialog/styfitem
    sfx2/source/dialog/styledlg
    sfx2/source/dialog/tabdlg
    sfx2/source/dialog/templdlg
    sfx2/source/dialog/StyleList
    sfx2/source/dialog/tplcitem
    sfx2/source/dialog/tplpitem
    sfx2/source/dialog/versdlg
    sfx2/source/doc/DocumentMetadataAccess
    sfx2/source/doc/DocumentSigner
    sfx2/source/doc/Metadatable
    sfx2/source/doc/QuerySaveDocument
    sfx2/source/doc/SfxDocumentMetaData
    sfx2/source/doc/autoredactdialog
    sfx2/source/doc/docfac
    sfx2/source/doc/docfile
    sfx2/source/doc/docfilt
    sfx2/source/doc/docinf
    sfx2/source/doc/docinsert
    sfx2/source/doc/docmacromode
    sfx2/source/doc/docstoragemodifylistener
    sfx2/source/doc/doctempl
    sfx2/source/doc/doctemplates
    sfx2/source/doc/doctemplateslocal
    sfx2/source/doc/exoticfileloadexception
    sfx2/source/doc/frmdescr
    sfx2/source/doc/graphhelp
    sfx2/source/doc/guisaveas
    sfx2/source/doc/iframe
    sfx2/source/doc/new
    sfx2/source/doc/objcont
    sfx2/source/doc/objembed
    sfx2/source/doc/objitem
    sfx2/source/doc/objmisc
    sfx2/source/doc/objserv
    sfx2/source/doc/objstor
    sfx2/source/doc/objxtor
    sfx2/source/doc/oleprops
    sfx2/source/doc/ownsubfilterservice
    sfx2/source/doc/printhelper
    sfx2/source/doc/docundomanager
    sfx2/source/doc/sfxbasemodel
    sfx2/source/doc/sfxmodelfactory
    sfx2/source/doc/SfxRedactionHelper
    sfx2/source/doc/signaturestate
    sfx2/source/doc/syspath
    sfx2/source/doc/zoomitem
    sfx2/source/doc/templatedlg
    sfx2/source/doc/watermarkitem
    sfx2/source/doc/saveastemplatedlg
    sfx2/source/explorer/nochaos
    sfx2/source/inet/inettbc
    sfx2/source/notebookbar/SfxNotebookBar
    sfx2/source/notebookbar/NotebookbarTabControl
    sfx2/source/notify/eventsupplier
    sfx2/source/notify/globalevents
    sfx2/source/notify/hintpost
    sfx2/source/notify/openurlhint
    sfx2/source/safemode/safemode
    sfx2/source/sidebar/Sidebar
    sfx2/source/sidebar/SidebarChildWindow
    sfx2/source/sidebar/SidebarDockingWindow
    sfx2/source/sidebar/SidebarController
    sfx2/source/sidebar/SidebarModelUpdate
    sfx2/source/sidebar/SidebarPanelBase
    sfx2/source/sidebar/SidebarToolBox
    sfx2/source/sidebar/AsynchronousCall
    sfx2/source/sidebar/Context
    sfx2/source/sidebar/ContextChangeBroadcaster
    sfx2/source/sidebar/ContextList
    sfx2/source/sidebar/ControllerFactory
    sfx2/source/sidebar/ControllerItem
    sfx2/source/sidebar/Deck
    sfx2/source/sidebar/DeckDescriptor
    sfx2/source/sidebar/DeckLayouter
    sfx2/source/sidebar/DeckTitleBar
    sfx2/source/sidebar/FocusManager
    sfx2/source/sidebar/IContextChangeReceiver
    sfx2/source/sidebar/ILayoutableWindow
    sfx2/source/sidebar/Panel
    sfx2/source/sidebar/PanelDescriptor
    sfx2/source/sidebar/PanelLayout
    sfx2/source/sidebar/PanelTitleBar
    sfx2/source/sidebar/ResourceManager
    sfx2/source/sidebar/TabBar
    sfx2/source/sidebar/TitleBar
    sfx2/source/sidebar/Theme
    sfx2/source/sidebar/Tools
    sfx2/source/sidebar/uiobject
    sfx2/source/sidebar/UnoPanel
    sfx2/source/sidebar/UnoPanels
    sfx2/source/sidebar/UnoDeck
    sfx2/source/sidebar/UnoDecks
    sfx2/source/sidebar/UnoSidebar
    sfx2/source/statbar/stbitem
    sfx2/source/styles/StyleManager
    sfx2/source/toolbox/tbxitem
    sfx2/source/toolbox/weldutils
    sfx2/source/view/classificationcontroller
    sfx2/source/view/classificationhelper
    sfx2/source/view/frame
    sfx2/source/view/frame2
    sfx2/source/view/frmload
    sfx2/source/view/ipclient
    sfx2/source/view/lokcharthelper
    sfx2/source/view/lokstarmathhelper
    sfx2/source/view/lokhelper
    sfx2/source/view/printer
    sfx2/source/view/sfxbasecontroller
    sfx2/source/view/userinputinterception
    sfx2/source/view/viewfac
    sfx2/source/view/viewfrm
    sfx2/source/view/viewfrm2
    sfx2/source/view/viewprn
    sfx2/source/view/viewsh

    # WNT/MACOSX-only sources (shutdowniconaqua, shutdowniconw32, syspathw32)
    # are out of scope for this Linux port.
)

librestudio_expand_cxx(SFX_SOURCES)
add_library(sfx SHARED ${SFX_SOURCES})
add_dependencies(sfx sfx2_sdi)

target_include_directories(sfx PRIVATE
    "${CMAKE_SOURCE_DIR}/sfx2/inc"
    "${CMAKE_SOURCE_DIR}/sfx2/source/inc"
    "${sfx2_sdi_INCLUDE_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}")
add_dependencies(sfx officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(sfx PRIVATE SFX2_DLLIMPLEMENTATION)

target_link_libraries(sfx PRIVATE
    basegfx comphelper cppu cppuhelper docmodel drawinglayercore drawinglayer
    fwk i18nlangtag i18nutil sal salhelper sax sb sot svl svt tk tl
    ucbhelper utl vcl
    Boost::boost
    ICU::uc ICU::i18n
    LibXml2::LibXml2)

if (LIBRESTUDIO_ENABLE_BREAKPAD)
    target_link_libraries(sfx PRIVATE crashreport)
endif ()

librestudio_add_component(sfx
    COMPONENT      "${CMAKE_SOURCE_DIR}/sfx2/util/sfx.component"
    IMPLEMENTATION "${CMAKE_SOURCE_DIR}/sfx2/util/sfx.component.extended"
    LIBRARY        sfx
    LAYER          OOO)

librestudio_set_library_output_name(sfx sfx OOO)

install(TARGETS sfx LIBRARY DESTINATION program)
