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

    # WNT/MACOSX-only sources (shutdowniconw32, syspathw32) are out of scope
    # for this Linux port -- real Windows-specific implementations, not just
    # flags (see the WIN32 block below, past add_library(sfx ...), for what
    # IS still wired from that platform's Library_sfx.mk block: the win32
    # link libraries, which are small/mechanical rather than source-file-
    # sized). shutdowniconaqua, macOS's own equivalent, is wired below via
    # SFX_APPLE_SOURCES instead -- it is a real alternate-implementation
    # source file too, but this port already has an established technique
    # for compiling a single Objective-C++ .mm file into a library (sal/
    # CMakeLists.txt's uunxapi.mm/system.mm precedent, reused identically by
    # vcl/CMakeLists.txt's own cgutils.mm), so it is reproduced structurally
    # here rather than left as a scope call the way the two WNT-only files
    # are.
)

# Library_sfx.mk: `ifeq ($(OS),MACOSX)` block -- gb_Library_add_objcxxobjects,
# sfx2/source/appl/shutdowniconaqua.mm (confirmed to exist on disk, no .cxx
# sibling). Same technique as sal/CMakeLists.txt's uunxapi.mm/system.mm and
# vcl/CMakeLists.txt's cgutils.mm: appended as an absolute path so
# librestudio_expand_cxx() below (which appends ".cxx" to every relative
# entry) leaves it untouched, and CMake auto-detects a bare .mm as
# Objective-C++ on Apple platforms with no set_source_files_properties(...
# LANGUAGE OBJCXX)/gb_OBJCXXFLAGS equivalent needed (see vcl/CMakeLists.txt's
# own comment on this point for cgutils.mm). Unverified/best-effort, like
# every other APPLE branch already in this port: no macOS SDK on this
# machine to confirm the .mm compile path actually works.
set(SFX_APPLE_SOURCES "")
if (APPLE)
    list(APPEND SFX_APPLE_SOURCES
        "${CMAKE_SOURCE_DIR}/sfx2/source/appl/shutdowniconaqua.mm")
endif ()

librestudio_expand_cxx(SFX_SOURCES)
add_library(sfx SHARED ${SFX_SOURCES} ${SFX_APPLE_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(sfx PRIVATE
        "${CMAKE_SOURCE_DIR}/sfx2/inc/pch/precompiled_sfx.hxx")
    target_compile_definitions(sfx PRIVATE PCH_LEVEL=2)
endif ()
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

# Library_sfx.mk: `ifeq ($(OS),$(filter WNT MACOSX,$(OS)))` -- a compile
# define shared by both platforms (the "quickstarter" applet/tray icon
# feature), structural now rather than left undocumented: this port's
# established policy (per its platform-gaps audit) is that a small,
# mechanical platform addition like a single compile define is not left
# completely absent just because it never fires on Linux.
if (WIN32 OR APPLE)
    target_compile_definitions(sfx PRIVATE ENABLE_QUICKSTART_APPLET)
endif ()

# Library_sfx.mk: rest of the `ifeq ($(OS),MACOSX)` block -- gb_Library_
# add_libs(-lobjc) and gb_Library_use_system_darwin_frameworks(Cocoa), on top
# of shutdowniconaqua.mm's compilation above. "objc" follows this port's own
# established plain-name style for a raw "-l<name>" flag (sal/CMakeLists.txt
# links "rt"/"network" the same way, not "-lrt"/"-lnetwork"); "-framework
# Cocoa" follows sal's/vcl's own already-established Darwin-framework idiom
# (sal/CMakeLists.txt's "-framework Security", vcl/CMakeLists.txt's own
# "-framework Cocoa" for its identical MACOSX-and-not-iOS branch).
if (APPLE)
    target_link_libraries(sfx PRIVATE objc "-framework Cocoa")
endif ()

# Library_sfx.mk: `ifeq ($(OS),WNT)` block's win32 libs (gb_Library_
# use_system_win32_libs) -- linked here even though this port leaves the
# block's 2 WNT-only source files (shutdowniconw32, syspathw32) out of scope
# above: unlike those real Windows-specific implementation files, 5 plain
# system library names are exactly the kind of small, mechanical platform
# addition this port's policy says should not be left silently absent. Bare
# names, same style as vcl/CMakeLists.txt's own WIN32 block (dwrite ole32
# setupapi shell32 version).
if (WIN32)
    target_sources(sfx PRIVATE
        "${CMAKE_SOURCE_DIR}/sfx2/source/appl/shutdowniconw32.cxx"
        "${CMAKE_SOURCE_DIR}/sfx2/source/doc/syspathw32.cxx")
    target_link_libraries(sfx PRIVATE
        advapi32 gdi32 ole32 shell32 uuid)
endif ()

librestudio_add_component(sfx
    COMPONENT      "${CMAKE_SOURCE_DIR}/sfx2/util/sfx.component"
    IMPLEMENTATION "${CMAKE_SOURCE_DIR}/sfx2/util/sfx.component.extended"
    LIBRARY        sfx
    LAYER          OOO)

librestudio_set_library_output_name(sfx sfx OOO)

install(TARGETS sfx LIBRARY DESTINATION program)

# --- Package_classification / CustomTarget_classification --------------------
#
# Package_classification.mk packages 9 example classification XML files
# (sfx2/classification/example*.xml, all confirmed to exist on disk) into
# $(LIBO_SHARE_FOLDER)/classification, i.e. share/classification -- the same
# literal "share/..." destination style already established for a soffice.cfg
# install() below (and for officecfg's own registry packaging, whose
# OFFICECFG_SHARE_FOLDER variable is directory-scoped to officecfg/
# CMakeLists.txt and not visible here, so the literal string is used
# directly, the same choice svtools/CMakeLists.txt's own UIConfig install()
# already made for its literal "share/config/soffice.cfg/..." destination).
# This is a real runtime asset (the classification example files a document
# can point ClassificationHelper at), so it is ported unconditionally rather
# than deferred.
install(FILES
    "${CMAKE_SOURCE_DIR}/sfx2/classification/example.xml"
    "${CMAKE_SOURCE_DIR}/sfx2/classification/example_ca-ES.xml"
    "${CMAKE_SOURCE_DIR}/sfx2/classification/example_es-ES.xml"
    "${CMAKE_SOURCE_DIR}/sfx2/classification/example_fr-FR.xml"
    "${CMAKE_SOURCE_DIR}/sfx2/classification/example_hu-HU.xml"
    "${CMAKE_SOURCE_DIR}/sfx2/classification/example_nl-NL.xml"
    "${CMAKE_SOURCE_DIR}/sfx2/classification/example_pt-BR.xml"
    "${CMAKE_SOURCE_DIR}/sfx2/classification/example_sl-SI.xml"
    "${CMAKE_SOURCE_DIR}/sfx2/classification/example_zh-CN.xml"
    DESTINATION share/classification)

# CustomTarget_classification.mk validates example.xml against baf.xsd with
# xmllint at build time -- a build-time QA check, not something that
# produces a runtime artifact anything depends on, but straightforward to
# reproduce: officecfg/CMakeLists.txt already establishes the
# find_program(<TOOL>_EXECUTABLE <tool> REQUIRED) pattern for an equivalent
# external XML tool (XSLTPROC_EXECUTABLE/xsltproc), mirrored here for xmllint
# (no XMLLINT_EXECUTABLE/xmllint lookup exists anywhere else in this tree
# yet, confirmed by grep). Reproduced as an ALL custom target with a marker
# file, the same "touch on success" shape upstream's own
# sfx2_classification_GEN_example_validated marker gives
# gb_CustomTarget_get_target -- upstream also `cat`s the captured output on
# failure only because it redirects stdout/stderr into that same marker file
# first; CMake's add_custom_command already surfaces a failing xmllint's
# stderr directly and fails the build, so that redirect-and-cat step is not
# needed here.
#
# Target named "sfx2_classification_package", not the bare "sfx2_classification"
# a first reading of upstream's own Package_classification.mk (gb_Package_
# Package,sfx2_classification,...) would suggest: CppunitTest_sfx2_classification.mk
# (see "--- CppunitTests ---" below) needs its own CMake target to be named
# literally "sfx2_classification" too (librestudio_add_cppunit_test()'s <name>
# argument doubles as the add_library() target name, matching every other
# CppunitTest in this port, e.g. sfx2_metadatable/sfx2_controlleritem above).
# Upstream keeps these two apart for free -- gbuild's Package_ and CppunitTest_
# namespaces never collide -- but this port has no such namespace separation
# (both become one flat add_library()/add_custom_target() name), so keeping
# the literal name "sfx2_classification" on both would be a real add_custom_
# target()/add_library() duplicate-target-name error. Nothing else in the tree
# depends on this custom target under either name (grep -rn "\bsfx2_
# classification\b" across every CMakeLists.txt/*.cmake, checked when this
# rename was made), so renaming it here rather than the CppunitTest is the
# lower-risk fix: it keeps the CppunitTest's own CMake target name, ctest test
# name and OUTPUT_NAME ("test_sfx2_classification" -> "libtest_sfx2_
# classification.so") matching upstream's own CppunitTest_sfx2_classification
# name exactly, the same convention every other CppunitTest in this port
# relies on, and only touches this self-contained custom target's own name.
find_program(XMLLINT_EXECUTABLE xmllint REQUIRED)

set(SFX2_CLASSIFICATION_DIR "${CMAKE_SOURCE_DIR}/sfx2/classification")
set(SFX2_CLASSIFICATION_WORKDIR "${CMAKE_CURRENT_BINARY_DIR}/classification")
set(SFX2_CLASSIFICATION_VALIDATED "${SFX2_CLASSIFICATION_WORKDIR}/example.validated")

add_custom_command(
    OUTPUT "${SFX2_CLASSIFICATION_VALIDATED}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${SFX2_CLASSIFICATION_WORKDIR}"
    COMMAND "${XMLLINT_EXECUTABLE}" --noout --schema
            "${SFX2_CLASSIFICATION_DIR}/baf.xsd" "${SFX2_CLASSIFICATION_DIR}/example.xml"
    COMMAND "${CMAKE_COMMAND}" -E touch "${SFX2_CLASSIFICATION_VALIDATED}"
    DEPENDS "${SFX2_CLASSIFICATION_DIR}/baf.xsd" "${SFX2_CLASSIFICATION_DIR}/example.xml"
    COMMENT "xmllint: validating sfx2/classification/example.xml against baf.xsd"
    VERBATIM)

add_custom_target(sfx2_classification_package ALL
    DEPENDS "${SFX2_CLASSIFICATION_VALIDATED}")

# --- UIConfig_sfx --------------------------------------------------------------
#
# UIConfig_sfx.mk: 68 .ui files under sfx2/uiconfig/ui packaged into
# share/config/soffice.cfg/sfx/ui -- "sfx", not "sfx2", matching this UIConfig's
# own gb_UIConfig_UIConfig/add_uifiles argument (both called with "sfx" in the
# .mk) the same literal way vcl/CMakeLists.txt's own UIConfig_vcl block uses
# "vcl" (its own UIConfig name) rather than the module directory name --
# confirmed independently by CppunitTest_sfx2_dialogs_test.mk's and
# CppunitTest_sfx2_view.mk's own use_uiconfigs(...,sfx,...) calls, which name
# this same UIConfig "sfx". Same gb_UIConfig_DIR_FOR-derived install(FILES...)
# mechanism as UIConfig_svt (svtools/CMakeLists.txt) and UIConfig_vcl (vcl/
# CMakeLists.txt) -- just a longer file list, all 68 confirmed to exist on
# disk (find sfx2/uiconfig/ui -name '*.ui' | wc -l).
#
# Captured into a variable (SFX_UICONFIG_UI_FILES), not left as a bare
# install(FILES ...) argument list, because CppunitTest_sfx2_dialogs_test.mk's
# own gb_CppunitTest_use_uiconfigs(sfx2_dialogs_test,sfx) call (wired below in
# sfx2/CppunitTest_deferred.cmake) needs a real build-time ALL target to
# depend on -- verified directly first: no "sfx_uiconfig" target existed
# anywhere in this file before this change (grep -n "_uiconfig" sfx2/
# CMakeLists.txt was empty), unlike vcl/svtools/svx/cui, which each already
# grew their own "<module>_uiconfig" mirror the first time one of their own
# CppunitTests needed UICONFIG (see cmake/LibreStudioTest.cmake's own header
# comment on this, and cui/CMakeLists.txt's "cui_uiconfig" for the identical
# per-file add_custom_command + one add_custom_target(...ALL DEPENDS...)
# shape reused verbatim below).
set(SFX_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/addtargetdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/autoredactdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/bookmarkdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/bookmarkmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/charmapcontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/charviewmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/checkin.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/classificationbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/cmisinfopage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/cmisline.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/commandpopup.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/custominfopage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/deck.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/descriptioninfopage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/developmenttool.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/devtoolsmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/documentfontspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/documentinfopage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/documentpropertiesdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/editdocumentdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/editdurationdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/extrabutton.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/errorfindemaildialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/floatingrecord.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/helpbookmarkpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/helpcontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/helpcontentpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/helpindexpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/helpmanual.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/helpsearchpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/helpwindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/infobar.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/inputdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/licensedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/linefragment.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/linkeditdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/loadtemplatedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/managestylepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/navigator.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/newstyle.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/notebookbar.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optprintpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/panel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/password.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/notebookbarpopup.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/printeroptionsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/propertychip.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/propertychiprow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/propertycategoryrow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querysavedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/quickfind.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/saveastemplatedlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/safemodequerydialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/searchdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/securityinfopage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/singletabdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/startcenter.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/stylecontextmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tabbar.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tabbarcontents.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tabbutton.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/templatedlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/templatecategorydlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/templatepanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/urlbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/versionsofdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/versioncommentdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/versionscmis.ui"
)

install(FILES ${SFX_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/sfx/ui)

# Real build-time mirror of the install() rule above, alongside it (not
# instead of it) -- same per-file add_custom_command + one add_custom_target(
# ... ALL DEPENDS ...) shape as UIConfig_vcl's/UIConfig_svx's/cui_uiconfig's
# own blocks; see cui/CMakeLists.txt's own comment (right after its own
# CUI_UICONFIG_UI_FILES install()) for the full reasoning, not repeated here.
set(_sfx_uiconfig_outputs "")
foreach (_sfx_uiconfig_src IN LISTS SFX_UICONFIG_UI_FILES)
    get_filename_component(_sfx_uiconfig_name "${_sfx_uiconfig_src}" NAME)
    set(_sfx_uiconfig_dest "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/sfx/ui/${_sfx_uiconfig_name}")
    add_custom_command(
        OUTPUT "${_sfx_uiconfig_dest}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/sfx/ui"
        COMMAND ${CMAKE_COMMAND} -E copy "${_sfx_uiconfig_src}" "${_sfx_uiconfig_dest}"
        DEPENDS "${_sfx_uiconfig_src}"
        COMMENT "Copying ${_sfx_uiconfig_name} into build-tree soffice.cfg/sfx/ui"
        VERBATIM)
    list(APPEND _sfx_uiconfig_outputs "${_sfx_uiconfig_dest}")
endforeach ()
unset(_sfx_uiconfig_src)
unset(_sfx_uiconfig_name)
unset(_sfx_uiconfig_dest)

add_custom_target(sfx_uiconfig ALL DEPENDS ${_sfx_uiconfig_outputs})
unset(_sfx_uiconfig_outputs)

# --- CppunitTests --------------------------------------------------------------
#
# sfx2 has 8 CppunitTest_*.mk files, each read in full and checked directly
# against the live file. Only 2 of the 8 can actually be wired here; the
# other 6 are genuine, flagged gaps, not silently dropped.
#
# sfx2_metadatable and sfx2_controlleritem are the 2 that resolve cleanly:
# neither calls use_ure/use_vcl/use_rdb/use_components/use_configuration --
# just use_sdk_api + use_libraries (cppu cppuhelper sal sfx) +
# use_external(boost_headers) -- so both need nothing from a module
# add_subdirectory()'d after sfx2 itself, and are wired directly here rather
# than deferred (cmake/LibreStudioTest.cmake's own header comment explains
# when deferral into a separate *_deferred.cmake file is needed: only when a
# COMPONENTS/CONFIGURATION dependency is not yet satisfied at this point in
# the root CMakeLists.txt -- neither test needs either).
librestudio_add_cppunit_test(sfx2_metadatable
    SOURCES
        sfx2/qa/cppunit/test_metadatable
    LIBRARIES cppu cppuhelper sal sfx Boost::boost
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers)

librestudio_add_cppunit_test(sfx2_controlleritem
    SOURCES
        sfx2/qa/cppunit/test_controlleritem
    LIBRARIES cppu cppuhelper sal sfx Boost::boost
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers)

# sfx2_misc, sfx2_classification, sfx2_doc and sfx2_autoredaction ARE NOW
# WIRED -- test/CMakeLists.txt's "subsequenttest" (Library_subsequenttest.mk,
# 176 UNO-API-interface-test helper sources) is a real target as of this
# wave, which was the single blocker every one of these 5 tests' use_libraries
# lists named (re-checked directly against each live .mk again for this
# wave -- "test" in the same lists is a DIFFERENT, already-ported library,
# Library_test.mk -> this port's test_fixtures, per every other CppunitTest
# in this port; only "subsequenttest" itself was ever missing).
#
# All four are registered in sfx2/CppunitTest_deferred.cmake, NOT here:
# subsequenttest, test_fixtures and unotest (test/CMakeLists.txt, unotest/
# CMakeLists.txt) are all add_subdirectory()'d AFTER sfx2's own position in
# the root CMakeLists.txt, and use_configuration's CONFIGURATION option needs
# test/CMakeLists.txt's LIBRESTUDIO_TEST_UNITTEST_DIR for the same reason;
# use_rdb(services) needs the program_services_rdb target, which is not
# defined until much later still (right after librestudio_finalize_
# components(services)) -- the exact same ordering constraint already
# documented for svx_unit/svx_gallery_test/svx_core (svx/CppunitTest_
# deferred.cmake) and scaddins_analysis/xmlscript_cppunit before them, so
# these four are deferred to that same include() point, right after svx's own
# deferred file. See sfx2/CppunitTest_deferred.cmake for the full per-test
# detail, including:
#   - sfx2_classification's use_uiconfigs(svx) call, which has no CMake
#     equivalent anywhere in this port (grepped project-wide, confirmed
#     empty) -- but is NOT treated as a blocker, per vcl_treeview/vcl_
#     lifecycle's own precedent (vcl/CppunitTest_deferred.cmake): both are
#     wired despite calling use_uiconfigs(vcl), which is equally unmodelled,
#     so a bare use_uiconfigs call is a documented runtime-only gap, not a
#     build/registration blocker, in this port's established practice.
#   - sfx2_classification's use_packages(sfx2_classification) call, which
#     surfaced a genuine NEW blocker this wave's fresh verification caught
#     that the previous wave's analysis (written before subsequenttest
#     existed) had not: upstream's Package_classification.mk and CppunitTest_
#     sfx2_classification.mk share the literal name "sfx2_classification"
#     across two different gbuild namespaces (Package_ vs CppunitTest_),
#     which this port's flat CMake target namespace cannot also do (would be
#     an add_library()/add_custom_target() duplicate-name error). Fixed by
#     renaming the pre-existing custom target above to "sfx2_classification_
#     package" instead (see its own comment for why that one, and not the
#     CppunitTest, was the one renamed) -- freeing the literal name
#     "sfx2_classification" for the CppunitTest, matching this port's usual
#     CppunitTest-target-name-matches-upstream convention.
#   - sfx2_doc's use_custom_headers(officecfg/registry) call (re-verified
#     directly against the live .mk -- NOT mentioned by the previous wave's
#     analysis, which predates subsequenttest and this exact line-by-line
#     re-check), resolved the same way svx_core's identical call already is:
#     OFFICECFG_INCLUDE_DIR + a DEPENDS on officecfg_headers (officecfg/
#     CMakeLists.txt).
#
# sfx2_view remains genuinely blocked, for a second, independent reason
# beyond subsequenttest (now resolved): its own use_executable(xpdfimport)
# call needs Executable_xpdfimport.mk, which this port has never written --
# re-confirmed for this wave (grep -rn "add_executable(xpdfimport" across the
# whole tree: zero hits; sdext/CMakeLists.txt's own comment on why poppler is
# ported but Executable_xpdfimport.mk itself is not is the fullest statement
# of this). Forcing sfx2_view through without it, or silently dropping the
# use_executable dependency, would both misrepresent what this test actually
# needs to run, so it stays flagged here rather than either. Once xpdfimport
# exists, sfx2_view would join sfx2/CppunitTest_deferred.cmake the same way
# as the other four (same subsequenttest/test_fixtures/unotest/program_
# services_rdb/CONFIGURATION shape, plus use_custom_headers(officecfg/
# registry) like sfx2_doc, plus use_uiconfigs(modules/simpress, sfx, svt,
# svx) -- same non-blocking-gap treatment as sfx2_classification's own
# use_uiconfigs(svx) above -- plus DEPENDS on the xpdfimport executable
# target once it exists).
#
# sfx2_dialogs_test (gb_CppunitTest_CppunitScreenShot) IS NOW WIRED, in
# sfx2/CppunitTest_deferred.cmake, alongside sfx2_misc/sfx2_classification/
# sfx2_doc/sfx2_autoredaction: same subsequenttest/test_fixtures/unotest/
# program_services_rdb/CONFIGURATION shape as those four (use_rdb(services),
# use_configuration), PLUS use_ure + use_vcl_non_headless_with_windows ->
# NONHEADLESS (cmake/LibreStudioTest.cmake's vcl_dialogs_test/svtools_
# dialogs_test/svx_dialogs_test/cui_dialogs_test*/basctl_dialogs_test/
# reportdesign_dialogs_test precedent), and use_uiconfigs(sfx) -> UICONFIG sfx
# (this module's own new "sfx_uiconfig" ALL target, added just above
# alongside this wiring -- re-verified directly that no such target existed
# before this change, per that block's own comment). Unlike the earlier,
# now-stale comment here (superseded 2026-09-28): this is a genuine
# screenshot/*_dialogs_test target, same category as vcl_dialogs_test/
# svtools_dialogs_test/svx_dialogs_test/cui_dialogs_test*/basctl_dialogs_test/
# reportdesign_dialogs_test -- all of which this port DOES wire through
# NONHEADLESS+UICONFIG, not skip; only sfx2_view remains genuinely
# out-of-scope, for the separate xpdfimport reason above.

# sfx2/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/sfx2/CppunitTest_deferred.cmake")
