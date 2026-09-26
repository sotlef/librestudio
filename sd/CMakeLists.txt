# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sd (module sd) -- Impress/Draw. Three libraries in Module_sd.mk:
#  - sdd:  a single-file type-detection UNO service
#          (com.sun.star.comp.draw.FormatDetector). No dependency on the
#          other two sd libraries -- built below.
#  - sd:   the core document model (SdDrawDocument/SdPage), every import/
#          export filter (ppt/eppt, html, pdf, grf, cgm, xml/odf), and the
#          whole Impress/Draw UI. Ported below (2026-09-26), 389 exception
#          objects transcribed from Library_sd.mk with today's default flags
#          (ENABLE_SDREMOTE=ON, ENABLE_AVAHI=OFF, ENABLE_SDREMOTE_BLUETOOTH=OFF):
#          12 accessibility (guarded by ENABLE_WASM_STRIP_ACCESSIBILITY,
#          unset/false in this port, so included), 369 core/filter/UI, and 8
#          remotecontrol (the base 7 files plus DiscoveryService.cxx, all
#          gated on ENABLE_SDREMOTE). AvahiNetworkService.cxx
#          (ENABLE_SDREMOTE AND ENABLE_AVAHI) and BluetoothServer.cxx
#          (ENABLE_SDREMOTE AND ENABLE_SDREMOTE_BLUETOOTH) are real CMake-level
#          conditionals below (see the ENABLE_AVAHI/ENABLE_SDREMOTE/
#          ENABLE_SDREMOTE_BLUETOOTH options in
#          cmake/LibreStudioConfigHeaders.cmake) -- both happen to resolve to
#          today's actual 389, since neither avahi-client.pc nor dbus-glib-1.pc
#          is installed on this machine, but flipping either option on now
#          adds the corresponding source for real, unlike a hardcoded
#          exclusion.
#  - sdui: dialogs (present.cxx, custsdlg, prltempl, ...) and the Presenter
#          Console (sd/source/console/*, its own component file
#          sd/source/console/presenter.component). Ported below, 66 exception
#          objects transcribed from Library_sdui.mk. Links sd itself.
#
# Exclusions (this Linux-only build; unaffected by ENABLE_AVAHI/
# ENABLE_SDREMOTE/ENABLE_SDREMOTE_BLUETOOTH, which are handled by real
# conditionals below instead):
#  - Every OS==MACOSX and OS==WNT branch (DiscoveryService's Objective-C++
#    variant/OSXNetworkService, WINNetworkService, the WNT uuid/ws2_32 system
#    libs, OSXBluetooth.mm/IOBluetooth/-lobjc) -- out of scope for this Linux
#    port; the plain (non-Mac, non-WNT) DiscoveryService.cxx and
#    BluetoothServer.cxx are used instead.
#  - xmlsecurity ($(if $(ENABLE_NSS),xmlsecurity)) -- ENABLE_NSS is not
#    tracked in this port's config, and xmlsecurity has no CMake target yet
#    anyway.
#
# pkg-config modules used: dbus-1 (PkgConfig::DBUS, gb_Library_use_externals'
# `dbus`; precedent in vcl/CMakeLists.txt). icu_headers maps to the global
# ICU::uc target (precedent in cui/CMakeLists.txt) and frozen to the global
# FROZEN_INCLUDE_DIR (precedent in comphelper/CMakeLists.txt) -- neither
# needs a pkg_check_modules call of its own here.
#
# oox/generated: only sd/source/filter/eppt/pptx-epptooxml.cxx and
# sd/source/filter/eppt/pptx-animations.cxx reach into oox/token/*.hxx
# (verified by grepping sd/source/filter/eppt for '#include.*oox/token').
# oox/CMakeLists.txt's OOX_GENERATED_DIR is CACHE INTERNAL and its
# oox_generated custom target is globally visible (oox is add_subdirectory()'d
# before sd in the root CMakeLists.txt), so both are used directly.
#
# SDI: sd/sdi/sdslots.sdi and sd/sdi/sdgslots.sdi are each self-referential
# (SDI and EXPORTS arguments are the same file for each, unlike svx/sfx2's
# separate xxxslots.sdi/xxx.sdi pair) -- two librestudio_add_sdi_target()
# calls below, both writing into the same OUTPUT_DIR (distinct target names,
# distinct output basenames, so no collision).

librestudio_add_sdi_target(sd_sdslots
    SDI        "${CMAKE_SOURCE_DIR}/sd/sdi/sdslots.sdi"
    EXPORTS    "${CMAKE_SOURCE_DIR}/sd/sdi/sdslots.sdi"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/SdiTarget/sd/sdi"
    INCLUDES   "${CMAKE_SOURCE_DIR}/svx/sdi" "${CMAKE_SOURCE_DIR}/sfx2/sdi")

librestudio_add_sdi_target(sd_sdgslots
    SDI        "${CMAKE_SOURCE_DIR}/sd/sdi/sdgslots.sdi"
    EXPORTS    "${CMAKE_SOURCE_DIR}/sd/sdi/sdgslots.sdi"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/SdiTarget/sd/sdi"
    INCLUDES   "${CMAKE_SOURCE_DIR}/svx/sdi" "${CMAKE_SOURCE_DIR}/sfx2/sdi")

set(SD_SOURCES
    # ifneq ($(ENABLE_WASM_STRIP_ACCESSIBILITY),TRUE) -- applies, the flag is
    # unset in this build.
    sd/source/ui/accessibility/AccessibleDocumentViewBase
    sd/source/ui/accessibility/AccessibleDrawDocumentView
    sd/source/ui/accessibility/AccessibleOutlineEditSource
    sd/source/ui/accessibility/AccessibleOutlineView
    sd/source/ui/accessibility/AccessiblePageShape
    sd/source/ui/accessibility/AccessiblePresentationGraphicShape
    sd/source/ui/accessibility/AccessiblePresentationOLEShape
    sd/source/ui/accessibility/AccessiblePresentationShape
    sd/source/ui/accessibility/AccessibleSlideSorterObject
    sd/source/ui/accessibility/AccessibleSlideSorterView
    sd/source/ui/accessibility/AccessibleViewForwarder
    sd/source/ui/accessibility/SdShapeTypes

    sd/source/core/CustomAnimationCloner
    sd/source/core/CustomAnimationEffect
    sd/source/core/CustomAnimationPreset
    sd/source/core/EffectMigration
    sd/source/core/ModelTraverser
    sd/source/core/PageListWatcher
    sd/source/core/TransitionPreset
    sd/source/core/ThemeColorChanger
    sd/source/core/anminfo
    sd/source/core/annotations/Annotation
    sd/source/core/cusshow
    sd/source/core/SlideSectionManager
    sd/source/core/UndoSlideSection
    sd/source/core/drawdoc
    sd/source/core/drawdoc2
    sd/source/core/drawdoc3
    sd/source/core/drawdoc4
    sd/source/core/drawdoc_animations
    sd/source/core/pglink
    sd/source/core/sdpage
    sd/source/core/sdpage2
    sd/source/core/sdpage_animations
    sd/source/core/shapelist
    sd/source/core/stlfamily
    sd/source/core/stlpool
    sd/source/core/stlsheet
    sd/source/core/typemap
    sd/source/core/undo/undofactory
    sd/source/core/undo/undomanager
    sd/source/core/undo/undoobjects
    sd/source/core/undoanim
    sd/source/filter/cgm/sdcgmfilter
    sd/source/filter/eppt/eppt
    sd/source/filter/eppt/epptso
    sd/source/filter/eppt/escherex
    sd/source/filter/eppt/pptexanimations
    sd/source/filter/eppt/pptexsoundcollection
    sd/source/filter/eppt/pptx-epptbase
    sd/source/filter/eppt/pptx-epptooxml
    sd/source/filter/eppt/pptx-animations
    sd/source/filter/eppt/pptx-animations-cond
    sd/source/filter/eppt/pptx-animations-nodectx
    sd/source/filter/eppt/pptx-grouptable
    sd/source/filter/eppt/pptx-stylesheet
    sd/source/filter/eppt/pptx-text
    sd/source/filter/ppt/ppt97animations
    sd/source/filter/ppt/pptatom
    sd/source/filter/ppt/pptin
    sd/source/filter/ppt/pptinanimations
    sd/source/filter/ppt/propread
    sd/source/filter/grf/sdgrffilter
    sd/source/filter/html/htmlex
    sd/source/filter/html/a11yex
    sd/source/filter/html/sdhtmlfilter
    sd/source/filter/pdf/sdpdffilter
    sd/source/filter/sdfilter
    sd/source/filter/sdpptwrp
    sd/source/filter/xml/sdtransform
    sd/source/filter/xml/sdxmlwrp
    sd/source/helper/simplereferencecomponent
    sd/source/ui/animations/CustomAnimationDialog
    sd/source/ui/animations/CustomAnimationList
    sd/source/ui/animations/CustomAnimationPane
    sd/source/ui/animations/STLPropertySet
    sd/source/ui/animations/SlideTransitionPane
    sd/source/ui/animations/motionpathtag
    sd/source/ui/annotations/annotationmanager
    sd/source/ui/annotations/AnnotationPopup
    sd/source/ui/annotations/annotationwindow
    sd/source/ui/app/optsitem
    sd/source/ui/app/sddll
    sd/source/ui/app/sdmod
    sd/source/ui/app/sdmod1
    sd/source/ui/app/sdmod2
    sd/source/ui/app/sdpopup
    sd/source/ui/app/sdxfer
    sd/source/ui/app/tmplctrl
    sd/source/ui/app/scalectrl
    sd/source/ui/controller/slidelayoutcontroller
    sd/source/ui/controller/displaymodecontroller
    sd/source/ui/dlg/AnimationChildWindow
    sd/source/ui/dlg/LayerTabBar
    sd/source/ui/dlg/NavigatorChildWindow
    sd/source/ui/dlg/PaneChildWindows
    sd/source/ui/dlg/PaneShells
    sd/source/ui/dlg/SpellDialogChildWindow
    sd/source/ui/dlg/TemplateScanner
    sd/source/ui/dlg/animobjs
    sd/source/ui/dlg/diactrl
    sd/source/ui/dlg/filedlg
    sd/source/ui/dlg/gluectrl
    sd/source/ui/dlg/ins_paste
    sd/source/ui/dlg/layeroptionsdlg
    sd/source/ui/dlg/navigatr
    sd/source/ui/dlg/sdabstdlg
    sd/source/ui/dlg/sdtreelb
    sd/source/ui/dlg/SelectLayerDlg
    sd/source/ui/dlg/titledockwin
    sd/source/ui/dlg/unchss
    sd/source/ui/dlg/UndoThemeChange
    sd/source/ui/docshell/docshel2
    sd/source/ui/docshell/docshel3
    sd/source/ui/docshell/docshel4
    sd/source/ui/docshell/docshell
    sd/source/ui/docshell/grdocsh
    sd/source/ui/docshell/sdclient
    sd/source/ui/framework/configuration/ChangeRequestQueue
    sd/source/ui/framework/configuration/ChangeRequestQueueProcessor
    sd/source/ui/framework/configuration/Configuration
    sd/source/ui/framework/configuration/ConfigurationClassifier
    sd/source/ui/framework/configuration/ConfigurationController
    sd/source/ui/framework/configuration/ConfigurationControllerBroadcaster
    sd/source/ui/framework/configuration/ConfigurationControllerResourceManager
    sd/source/ui/framework/configuration/ConfigurationTracer
    sd/source/ui/framework/configuration/ConfigurationUpdater
    sd/source/ui/framework/configuration/GenericConfigurationChangeRequest
    sd/source/ui/framework/configuration/ResourceFactoryManager
    sd/source/ui/framework/configuration/ResourceId
    sd/source/ui/framework/configuration/UpdateRequest
    sd/source/ui/framework/factories/BasicPaneFactory
    sd/source/ui/framework/factories/BasicToolBarFactory
    sd/source/ui/framework/factories/BasicViewFactory
    sd/source/ui/framework/factories/ChildWindowPane
    sd/source/ui/framework/factories/FrameWindowPane
    sd/source/ui/framework/factories/FullScreenPane
    sd/source/ui/framework/factories/Pane
    sd/source/ui/framework/factories/PresentationFactory
    sd/source/ui/framework/factories/ViewShellWrapper
    sd/source/ui/framework/module/CenterViewFocusModule
    sd/source/ui/framework/module/DrawModule
    sd/source/ui/framework/module/ImpressModule
    sd/source/ui/framework/module/ModuleController
    sd/source/ui/framework/module/NotesPaneModule
    sd/source/ui/framework/module/PresentationModule
    sd/source/ui/framework/module/ShellStackGuard
    sd/source/ui/framework/module/SlideSorterModule
    sd/source/ui/framework/module/ToolBarModule
    sd/source/ui/framework/module/ViewTabBarModule
    sd/source/ui/framework/tools/FrameworkHelper
    sd/source/ui/func/bulmaper
    sd/source/ui/func/fuarea
    sd/source/ui/func/fubullet
    sd/source/ui/func/fuchar
    sd/source/ui/func/fucon3d
    sd/source/ui/func/fuconarc
    sd/source/ui/func/fuconbez
    sd/source/ui/func/fuconcs
    sd/source/ui/func/fuconnct
    sd/source/ui/func/fuconrec
    sd/source/ui/func/fuconstr
    sd/source/ui/func/fuconuno
    sd/source/ui/func/fucopy
    sd/source/ui/func/fucushow
    sd/source/ui/func/fudraw
    sd/source/ui/func/fudspord
    sd/source/ui/func/fuediglu
    sd/source/ui/func/fuexecuteinteraction
    sd/source/ui/func/fuexpand
    sd/source/ui/func/fuformatpaintbrush
    sd/source/ui/func/fuhhconv
    sd/source/ui/func/fuinsert
    sd/source/ui/func/fuinsfil
    sd/source/ui/func/fuline
    sd/source/ui/func/fulinend
    sd/source/ui/func/fulink
    sd/source/ui/func/fumeasur
    sd/source/ui/func/fumorph
    sd/source/ui/func/funavig
    sd/source/ui/func/fuoaprms
    sd/source/ui/func/fuolbull
    sd/source/ui/func/fuoltext
    sd/source/ui/func/fupage
    sd/source/ui/func/fuparagr
    sd/source/ui/func/fupoor
    sd/source/ui/func/fuprlout
    sd/source/ui/func/fuprobjs
    sd/source/ui/func/fuscale
    sd/source/ui/func/fusearch
    sd/source/ui/func/fusel
    sd/source/ui/func/fusldlg
    sd/source/ui/func/fusnapln
    sd/source/ui/func/fusumry
    sd/source/ui/func/futempl
    sd/source/ui/func/futext
    sd/source/ui/func/futhes
    sd/source/ui/func/futransf
    sd/source/ui/func/futxtatt
    sd/source/ui/func/fuvect
    sd/source/ui/func/fuzoom
    sd/source/ui/func/sdundogr
    sd/source/ui/func/smarttag
    sd/source/ui/func/undoback
    sd/source/ui/func/undoheaderfooter
    sd/source/ui/func/undolayer
    sd/source/ui/func/undopage
    sd/source/ui/func/unmovss
    sd/source/ui/func/unoaprms
    sd/source/ui/func/unprlout
    sd/source/ui/presenter/PresenterPreviewCache
    sd/source/ui/presenter/SlideRenderer
    sd/source/ui/sidebar/AllMasterPagesSelector
    sd/source/ui/sidebar/CurrentMasterPagesSelector
    sd/source/ui/sidebar/DocumentHelper
    sd/source/ui/sidebar/LayoutMenu
    sd/source/ui/sidebar/MasterPageContainer
    sd/source/ui/sidebar/MasterPageContainerFiller
    sd/source/ui/sidebar/MasterPageContainerProviders
    sd/source/ui/sidebar/MasterPageContainerQueue
    sd/source/ui/sidebar/MasterPageDescriptor
    sd/source/ui/sidebar/MasterPageObserver
    sd/source/ui/sidebar/MasterPagesSelector
    sd/source/ui/sidebar/NavigatorWrapper
    sd/source/ui/sidebar/PanelFactory
    sd/source/ui/sidebar/RecentlyUsedMasterPages
    sd/source/ui/sidebar/RecentMasterPagesSelector
    sd/source/ui/sidebar/SlideBackground
    sd/source/ui/sidebar/SlideMasterPagesAllToolBoxControl
    sd/source/ui/slideshow/PaneHider
    sd/source/ui/slideshow/SlideShowRestarter
    sd/source/ui/slideshow/showwin
    sd/source/ui/slideshow/slideshow
    sd/source/ui/slideshow/slideshowimpl
    sd/source/ui/slideshow/slideshowviewimpl
    sd/source/ui/slidesorter/cache/SlsBitmapCache
    sd/source/ui/slidesorter/cache/SlsBitmapCompressor
    sd/source/ui/slidesorter/cache/SlsBitmapFactory
    sd/source/ui/slidesorter/cache/SlsCacheCompactor
    sd/source/ui/slidesorter/cache/SlsGenericPageCache
    sd/source/ui/slidesorter/cache/SlsPageCache
    sd/source/ui/slidesorter/cache/SlsPageCacheManager
    sd/source/ui/slidesorter/cache/SlsQueueProcessor
    sd/source/ui/slidesorter/cache/SlsRequestFactory
    sd/source/ui/slidesorter/cache/SlsRequestQueue
    sd/source/ui/slidesorter/controller/SlideSorterController
    sd/source/ui/slidesorter/controller/SlsAnimationFunction
    sd/source/ui/slidesorter/controller/SlsAnimator
    sd/source/ui/slidesorter/controller/SlsClipboard
    sd/source/ui/slidesorter/controller/SlsCurrentSlideManager
    sd/source/ui/slidesorter/controller/SlsDragAndDropContext
    sd/source/ui/slidesorter/controller/SlsFocusManager
    sd/source/ui/slidesorter/controller/SlsInsertionIndicatorHandler
    sd/source/ui/slidesorter/controller/SlsListener
    sd/source/ui/slidesorter/controller/SlsPageSelector
    sd/source/ui/slidesorter/controller/SlsProperties
    sd/source/ui/slidesorter/controller/SlsScrollBarManager
    sd/source/ui/slidesorter/controller/SlsSelectionFunction
    sd/source/ui/slidesorter/controller/SlsSelectionManager
    sd/source/ui/slidesorter/controller/SlsSelectionObserver
    sd/source/ui/slidesorter/controller/SlsSlotManager
    sd/source/ui/slidesorter/controller/SlsTransferableData
    sd/source/ui/slidesorter/controller/SlsVisibleAreaManager
    sd/source/ui/slidesorter/model/SlideSorterModel
    sd/source/ui/slidesorter/model/SlsPageDescriptor
    sd/source/ui/slidesorter/model/SlsPageEnumeration
    sd/source/ui/slidesorter/model/SlsPageEnumerationProvider
    sd/source/ui/slidesorter/model/SlsVisualState
    sd/source/ui/slidesorter/shell/SlideSorter
    sd/source/ui/slidesorter/shell/SlideSorterViewShell
    sd/source/ui/slidesorter/view/SlideSorterView
    sd/source/ui/slidesorter/view/SlsFramePainter
    sd/source/ui/slidesorter/view/SlsInsertAnimator
    sd/source/ui/slidesorter/view/SlsInsertionIndicatorOverlay
    sd/source/ui/slidesorter/view/SlsSectionHeaderPainter
    sd/source/ui/slidesorter/view/SlsLayeredDevice
    sd/source/ui/slidesorter/view/SlsLayouter
    sd/source/ui/slidesorter/view/SlsPageObjectLayouter
    sd/source/ui/slidesorter/view/SlsPageObjectPainter
    sd/source/ui/slidesorter/view/SlsTheme
    sd/source/ui/slidesorter/view/SlsToolTip
    sd/source/ui/slidesorter/view/SlsViewCacheContext
    sd/source/ui/table/TableDesignPane
    sd/source/ui/table/tablefunction
    sd/source/ui/table/tableobjectbar
    sd/source/ui/tools/AsynchronousCall
    sd/source/ui/tools/ConfigurationAccess
    sd/source/ui/tools/EventMultiplexer
    sd/source/ui/tools/GraphicSizeCheck
    sd/source/ui/tools/IconCache
    sd/source/ui/tools/IdleDetection
    sd/source/ui/tools/PreviewRenderer
    sd/source/ui/tools/SdGlobalResourceContainer
    sd/source/ui/tools/SlideshowLayerRenderer
    sd/source/ui/tools/SlotStateListener
    sd/source/ui/tools/TimerBasedTaskExecution
    sd/source/ui/uitest/uiobject
    sd/source/ui/unoidl/DrawController
    sd/source/ui/unoidl/SdUnoDrawView
    sd/source/ui/unoidl/SdUnoOutlineView
    sd/source/ui/unoidl/SdUnoSlideView
    sd/source/ui/unoidl/UnoDocumentSettings
    sd/source/ui/unoidl/randomnode
    sd/source/ui/unoidl/unocpres
    sd/source/ui/unoidl/unodoc
    sd/source/ui/unoidl/unolayer
    sd/source/ui/unoidl/unomodel
    sd/source/ui/unoidl/unomodule
    sd/source/ui/unoidl/unoobj
    sd/source/ui/unoidl/unopage
    sd/source/ui/unoidl/unopback
    sd/source/ui/unoidl/unopool
    sd/source/ui/unoidl/unosrch
    sd/source/ui/unoidl/unowcntr
    sd/source/ui/view/DocumentRenderer
    sd/source/ui/view/FormShellManager
    sd/source/ui/view/GraphicObjectBar
    sd/source/ui/view/GraphicViewShellBase
    sd/source/ui/view/ImpressViewShellBase
    sd/source/ui/view/MediaObjectBar
    sd/source/ui/view/OutlineViewShellBase
    sd/source/ui/view/Outliner
    sd/source/ui/view/OutlinerIterator
    sd/source/ui/view/PresentationViewShellBase
    sd/source/ui/view/SlideSorterViewShellBase
    sd/source/ui/view/ToolBarManager
    sd/source/ui/view/ViewClipboard
    sd/source/ui/view/ViewShellBase
    sd/source/ui/view/ViewShellHint
    sd/source/ui/view/ViewShellImplementation
    sd/source/ui/view/ViewShellManager
    sd/source/ui/view/ViewTabBar
    sd/source/ui/view/WindowUpdater
    sd/source/ui/view/clview
    sd/source/ui/view/drawview
    sd/source/ui/view/drbezob
    sd/source/ui/view/drtxtob
    sd/source/ui/view/drtxtob1
    sd/source/ui/view/drviews1
    sd/source/ui/view/drviews2
    sd/source/ui/view/drviews3
    sd/source/ui/view/drviews4
    sd/source/ui/view/drviews5
    sd/source/ui/view/drviews6
    sd/source/ui/view/drviews7
    sd/source/ui/view/drviews8
    sd/source/ui/view/drviews9
    sd/source/ui/view/drviewsa
    sd/source/ui/view/drviewsb
    sd/source/ui/view/drviewsc
    sd/source/ui/view/drviewsd
    sd/source/ui/view/drviewse
    sd/source/ui/view/drviewsf
    sd/source/ui/view/drviewsg
    sd/source/ui/view/drviewsh
    sd/source/ui/view/drviewsi
    sd/source/ui/view/drviewsj
    sd/source/ui/view/drviewsk
    sd/source/ui/view/drvwshrg
    sd/source/ui/view/frmview
    sd/source/ui/view/grviewsh
    sd/source/ui/view/outlnvs2
    sd/source/ui/view/outlnvsh
    sd/source/ui/view/outlview
    sd/source/ui/view/presvish
    sd/source/ui/view/sdruler
    sd/source/ui/view/sdview
    sd/source/ui/view/sdview2
    sd/source/ui/view/sdview3
    sd/source/ui/view/sdview4
    sd/source/ui/view/sdview5
    sd/source/ui/view/sdwindow
    sd/source/ui/view/tabcontr
    sd/source/ui/view/unmodpg
    sd/source/ui/view/viewoverlaymanager
    sd/source/ui/view/viewshe2
    sd/source/ui/view/viewshe3
    sd/source/ui/view/viewshel
    sd/source/ui/view/zoomlist
    sd/source/ui/view/NotesPanelView
    sd/source/ui/view/NotesPanelViewShell
)

# ifeq ($(ENABLE_SDREMOTE),TRUE) -- Library_sd.mk nests the entire
# remotecontrol source block, plus the ENABLE_AVAHI and
# ENABLE_SDREMOTE_BLUETOOTH sub-branches below, inside this one gate; none of
# these sources (or their defines/libs, handled further down) apply when
# ENABLE_SDREMOTE is off.
if (ENABLE_SDREMOTE)
    list(APPEND SD_SOURCES
        sd/source/ui/remotecontrol/BufferedStreamSocket
        sd/source/ui/remotecontrol/Communicator
        sd/source/ui/remotecontrol/ImagePreparer
        sd/source/ui/remotecontrol/Server
        sd/source/ui/remotecontrol/Receiver
        sd/source/ui/remotecontrol/Listener
        sd/source/ui/remotecontrol/Transmitter)

    # ifeq ($(OS),MACOSX) else ifeq ($(ENABLE_AVAHI),TRUE) -- the OS==MACOSX
    # and OS==WNT branches are out of scope for this Linux port (see the
    # module header comment in cmake/LibreStudioConfigHeaders.cmake), leaving
    # the plain branch: AvahiNetworkService.cxx only when ENABLE_AVAHI is also
    # on, then DiscoveryService.cxx unconditionally within ENABLE_SDREMOTE.
    if (ENABLE_AVAHI)
        list(APPEND SD_SOURCES sd/source/ui/remotecontrol/AvahiNetworkService)
    endif ()

    list(APPEND SD_SOURCES sd/source/ui/remotecontrol/DiscoveryService)

    # ifeq ($(ENABLE_SDREMOTE_BLUETOOTH),TRUE) -- also nested inside
    # ENABLE_SDREMOTE in Library_sd.mk; the OS==MACOSX branch (OSXBluetooth.mm,
    # IOBluetooth, -lobjc) is out of scope for this Linux port.
    if (ENABLE_SDREMOTE_BLUETOOTH)
        list(APPEND SD_SOURCES sd/source/ui/remotecontrol/BluetoothServer)
    endif ()
endif ()

librestudio_expand_cxx(SD_SOURCES)
add_library(sd SHARED ${SD_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(sd PRIVATE
        "${CMAKE_SOURCE_DIR}/sd/inc/pch/precompiled_sd.hxx")
    target_compile_definitions(sd PRIVATE PCH_LEVEL=2)
endif ()
add_dependencies(sd sd_sdslots sd_sdgslots oox_generated)

pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)

# ENABLE_AVAHI's gb_Library_use_externals entry (avahi) sits before the
# ENABLE_SDREMOTE ifeq in Library_sd.mk, so it is linked whenever ENABLE_AVAHI
# is on by itself (only the AvahiNetworkService.cxx source above additionally
# requires ENABLE_SDREMOTE). REQUIRED is safe here because this only runs when
# the flag is actually on.
if (ENABLE_AVAHI)
    pkg_check_modules(AVAHI REQUIRED IMPORTED_TARGET avahi-client)
endif ()

# ENABLE_SDREMOTE_BLUETOOTH's dbus-glib-1 dependency: this port ties the
# dbus-glib CFLAGS/LIBS directly to ENABLE_SDREMOTE_BLUETOOTH (see
# cmake/LibreStudioConfigHeaders.cmake's comment next to its DBUS_GLIB_FOUND
# use) rather than replicating upstream's separate, unconditional
# DBUS_HAVE_GLIB gate. The coordinator's pkg_check_modules(DBUS_GLIB
# dbus-glib-1) call has no IMPORTED_TARGET, so a local, directory-scoped call
# is needed here to get a linkable PkgConfig::... target.
if (ENABLE_SDREMOTE AND ENABLE_SDREMOTE_BLUETOOTH)
    pkg_check_modules(DBUS_GLIB_LOCAL REQUIRED IMPORTED_TARGET dbus-glib-1)
endif ()

target_include_directories(sd PRIVATE
    "${CMAKE_SOURCE_DIR}/sd/inc"
    "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
    "${CMAKE_SOURCE_DIR}/sd/source/ui/slidesorter/inc"
    "${sd_sdslots_INCLUDE_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${OOX_GENERATED_DIR}"
    "${FROZEN_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(sd udkapi_normal_headers offapi_normal_headers officecfg_headers)

# gb_LinkTarget__use_bluez_bluetooth_headers (RepositoryExternal.mk, !SYSTEM_BLUEZ
# branch): a vendored header-only include path, needed only for
# BluetoothServer.cxx above.
if (ENABLE_SDREMOTE AND ENABLE_SDREMOTE_BLUETOOTH)
    target_include_directories(sd PRIVATE "${CMAKE_SOURCE_DIR}/external/bluez_bluetooth/inc")
endif ()

target_compile_definitions(sd PRIVATE
    -DSD_DLLIMPLEMENTATION
    -DSDUI_DLL_NAME=\"libsduilo.so\")

# ifeq ($(ENABLE_SDREMOTE),TRUE) / nested ifeq ($(ENABLE_SDREMOTE_BLUETOOTH),TRUE)
if (ENABLE_SDREMOTE)
    target_compile_definitions(sd PRIVATE -DENABLE_SDREMOTE)
    if (ENABLE_SDREMOTE_BLUETOOTH)
        target_compile_definitions(sd PRIVATE -DENABLE_SDREMOTE_BLUETOOTH)
    endif ()
endif ()

target_link_libraries(sd PRIVATE
    avmedia basegfx canvastools comphelper cppcanvas cppu cppuhelper docmodel
    drawinglayercore drawinglayer editeng fwk i18nlangtag i18nutil icg
    msfilter oox sal salhelper sax xo sb sfx sot svl svt svxcore svx
    textconversiondlgs tk tl ucbhelper utl vcl
    Boost::boost
    ICU::uc
    LibXml2::LibXml2
    PkgConfig::DBUS)

if (ENABLE_AVAHI)
    target_link_libraries(sd PRIVATE PkgConfig::AVAHI)
endif ()

if (ENABLE_SDREMOTE AND ENABLE_SDREMOTE_BLUETOOTH)
    target_link_libraries(sd PRIVATE PkgConfig::DBUS_GLIB_LOCAL)
endif ()

librestudio_add_component(sd
    COMPONENT "${CMAKE_SOURCE_DIR}/sd/util/sd.component"
    LIBRARY   sd
    LAYER     OOO)

librestudio_set_library_output_name(sd sd OOO)

install(TARGETS sd LIBRARY DESTINATION program)

set(SDUI_SOURCES
    sd/source/ui/dlg/brkdlg
    sd/source/ui/dlg/copydlg
    sd/source/ui/dlg/custsdlg
    sd/source/ui/dlg/dlgchar
    sd/source/ui/dlg/dlgfield
    sd/source/ui/dlg/dlgolbul
    sd/source/ui/dlg/dlgpage
    sd/source/ui/dlg/dlgsnap
    sd/source/ui/dlg/headerfooterdlg
    sd/source/ui/dlg/inspagob
    sd/source/ui/dlg/masterlayoutdlg
    sd/source/ui/dlg/morphdlg
    sd/source/ui/dlg/paragr
    sd/source/ui/dlg/present
    sd/source/ui/dlg/prltempl
    sd/source/ui/dlg/prntopts
    sd/source/ui/dlg/RemoteDialog
    sd/source/ui/dlg/RemoteDialogClientBox
    sd/source/ui/dlg/sddlgfact
    sd/source/ui/dlg/sdpreslt
    sd/source/ui/dlg/sduiexp
    sd/source/ui/dlg/tabtempl
    sd/source/ui/dlg/tpaction
    sd/source/ui/dlg/tpoption
    sd/source/ui/dlg/vectdlg
    sd/source/ui/dlg/PhotoAlbumDialog
    sd/source/ui/dlg/BulletAndPositionDlg
    sd/source/console/AccessibleFocusManager
    sd/source/console/AccessibleNotes
    sd/source/console/AccessibleObject
    sd/source/console/AccessibleParagraph
    sd/source/console/CanvasUpdateRequester
    sd/source/console/PresenterAccessibility
    sd/source/console/PresenterBitmapContainer
    sd/source/console/PresenterButton
    sd/source/console/PresenterCanvas
    sd/source/console/PresenterCanvasHelper
    sd/source/console/PresenterConfigurationAccess
    sd/source/console/PresenterController
    sd/source/console/PresenterCurrentSlideObserver
    sd/source/console/PresenterFrameworkObserver
    sd/source/console/PresenterGeometryHelper
    sd/source/console/PresenterHelpView
    sd/source/console/PresenterHelper
    sd/source/console/PresenterNotesView
    sd/source/console/PresenterPaintManager
    sd/source/console/PresenterPane
    sd/source/console/PresenterPaneBase
    sd/source/console/PresenterPaneBorderPainter
    sd/source/console/PresenterPaneContainer
    sd/source/console/PresenterPaneFactory
    sd/source/console/PresenterProtocolHandler
    sd/source/console/PresenterScreen
    sd/source/console/PresenterScrollBar
    sd/source/console/PresenterSlidePreview
    sd/source/console/PresenterSlideShowView
    sd/source/console/PresenterSlideSorter
    sd/source/console/PresenterSprite
    sd/source/console/PresenterSpritePane
    sd/source/console/PresenterTextView
    sd/source/console/PresenterTheme
    sd/source/console/PresenterTimer
    sd/source/console/PresenterToolBar
    sd/source/console/PresenterUIPainter
    sd/source/console/PresenterViewFactory
    sd/source/console/PresenterWindowManager
)

librestudio_expand_cxx(SDUI_SOURCES)
add_library(sdui SHARED ${SDUI_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(sdui PRIVATE
        "${CMAKE_SOURCE_DIR}/sd/inc/pch/precompiled_sdui.hxx")
    target_compile_definitions(sdui PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(sdui PRIVATE
    "${CMAKE_SOURCE_DIR}/sd/inc"
    "${CMAKE_SOURCE_DIR}/sd/source/ui/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(sdui udkapi_normal_headers offapi_normal_headers officecfg_headers)

# ifeq ($(ENABLE_SDREMOTE),TRUE) / nested ifeq ($(ENABLE_SDREMOTE_BLUETOOTH),TRUE)
# -- Library_sdui.mk only adds defines here, no extra sources/libs.
if (ENABLE_SDREMOTE)
    target_compile_definitions(sdui PRIVATE -DENABLE_SDREMOTE)
    if (ENABLE_SDREMOTE_BLUETOOTH)
        target_compile_definitions(sdui PRIVATE -DENABLE_SDREMOTE_BLUETOOTH)
    endif ()
endif ()

target_link_libraries(sdui PRIVATE
    basegfx comphelper cppcanvas cppu cppuhelper editeng i18nlangtag fwk sal
    salhelper sd sfx sot svl svt svxcore svx tk tl utl vcl
    Boost::boost)

librestudio_add_component(sdui
    COMPONENT "${CMAKE_SOURCE_DIR}/sd/source/console/presenter.component"
    LIBRARY   sdui
    LAYER     OOO)

librestudio_set_library_output_name(sdui sdui OOO)

install(TARGETS sdui LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# sdd -- Library_sdd.mk
# ---------------------------------------------------------------------------

set(SDD_SOURCES
    sd/source/ui/unoidl/sddetect
)

librestudio_expand_cxx(SDD_SOURCES)
add_library(sdd SHARED ${SDD_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(sdd REUSE_FROM precompiled_system)
endif ()

target_include_directories(sdd PRIVATE
    "${CMAKE_SOURCE_DIR}/sd/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(sdd udkapi_normal_headers offapi_normal_headers)

target_link_libraries(sdd PRIVATE
    comphelper cppu cppuhelper sal sfx sot tl utl vcl
    Boost::boost)

librestudio_add_component(sdd
    COMPONENT "${CMAKE_SOURCE_DIR}/sd/util/sdd.component"
    LIBRARY   sdd
    LAYER     OOO)

librestudio_set_library_output_name(sdd sdd OOO)

install(TARGETS sdd LIBRARY DESTINATION program)
