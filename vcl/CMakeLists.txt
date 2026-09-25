# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# vcl — the visual class library: windows, controls, output devices, fonts,
# bitmaps, and the filters that read and write image formats.
#
# Transcription of Library_vcl.mk for a Linux desktop build with the GUI
# enabled.  That file is a long list of objects and dependencies with some
# thirty conditionals woven through it, and the conditionals are the part that
# matters: some pick a platform, some a build type, some a feature.  Each one
# that this build resolves is named below next to what it decides, and the ones
# that resolve to nothing are named at the end, so that the next reader does not
# have to work out again which of them were considered and which overlooked.
#
# This library is only half of vcl.  The backend that puts pixels on a screen is
# a plug-in loaded by name at run time — vclplug_gen for X11 and the headless
# backends, vclplug_gtk3 and the rest for the others — and is not here; without
# one of them libvcl links but cannot start.

set(VCL_SOURCES
    vcl/source/rendercontext/drawmode
    vcl/skia/SkiaHelper
    vcl/source/accessibility/AccessibleBrowseBox
    vcl/source/accessibility/AccessibleBrowseBoxBase
    vcl/source/accessibility/AccessibleBrowseBoxCheckBoxCell
    vcl/source/accessibility/AccessibleBrowseBoxHeaderBar
    vcl/source/accessibility/AccessibleBrowseBoxHeaderCell
    vcl/source/accessibility/AccessibleBrowseBoxTable
    vcl/source/accessibility/AccessibleBrowseBoxTableBase
    vcl/source/accessibility/AccessibleBrowseBoxTableCell
    vcl/source/accessibility/AccessibleTextAttributeHelper
    vcl/source/accessibility/accessiblebrowseboxcell
    vcl/source/accessibility/accessibleiconchoicectrl
    vcl/source/accessibility/accessibleiconchoicectrlentry
    vcl/source/accessibility/accessiblelistbox
    vcl/source/accessibility/accessiblelistboxentry
    vcl/source/accessibility/accessiblemenubasecomponent
    vcl/source/accessibility/accessiblemenucomponent
    vcl/source/accessibility/accessiblemenuitemcomponent
    vcl/source/accessibility/accessibletablistbox
    vcl/source/accessibility/accessibletablistboxtable
    vcl/source/accessibility/characterattributeshelper
    vcl/source/accessibility/floatingwindowaccessible
    vcl/source/accessibility/IComboListBoxHelper
    vcl/source/accessibility/svtaccessiblenumericfield
    vcl/source/accessibility/vclxaccessiblebox
    vcl/source/accessibility/vclxaccessiblebutton
    vcl/source/accessibility/vclxaccessiblecheckbox
    vcl/source/accessibility/vclxaccessiblecombobox
    vcl/source/accessibility/vclxaccessiblecomponent
    vcl/source/accessibility/vclxaccessibledropdowncombobox
    vcl/source/accessibility/vclxaccessibledropdownlistbox
    vcl/source/accessibility/vclxaccessibleedit
    vcl/source/accessibility/vclxaccessiblefixedhyperlink
    vcl/source/accessibility/vclxaccessiblefixedtext
    vcl/source/accessibility/vclxaccessibleheaderbar
    vcl/source/accessibility/vclxaccessibleheaderbaritem
    vcl/source/accessibility/vclxaccessiblelist
    vcl/source/accessibility/vclxaccessiblelistbox
    vcl/source/accessibility/vclxaccessiblelistitem
    vcl/source/accessibility/vclxaccessiblemenu
    vcl/source/accessibility/vclxaccessiblemenubar
    vcl/source/accessibility/vclxaccessiblemenuitem
    vcl/source/accessibility/vclxaccessiblemenuseparator
    vcl/source/accessibility/vclxaccessiblepopupmenu
    vcl/source/accessibility/vclxaccessibleradiobutton
    vcl/source/accessibility/vclxaccessiblescrollbar
    vcl/source/accessibility/vclxaccessiblestatusbar
    vcl/source/accessibility/vclxaccessiblestatusbaritem
    vcl/source/accessibility/vclxaccessibletabcontrol
    vcl/source/accessibility/vclxaccessibletabpage
    vcl/source/accessibility/vclxaccessibletabpagewindow
    vcl/source/accessibility/vclxaccessibletextcomponent
    vcl/source/accessibility/vclxaccessibletextfield
    vcl/source/accessibility/vclxaccessibletoolbox
    vcl/source/accessibility/vclxaccessibletoolboxitem
    vcl/source/animate/Animation
    vcl/source/animate/AnimationFrame
    vcl/source/animate/AnimationRenderer
    vcl/source/cnttype/mcnttfactory
    vcl/source/cnttype/mcnttype
    vcl/source/printer/Options
    vcl/source/printer/QueueInfo
    vcl/source/window/bubblewindow
    vcl/source/window/errinf
    vcl/source/window/settings
    vcl/source/window/paint
    vcl/source/window/abstdlg
    vcl/source/window/accel
    vcl/source/window/accmgr
    vcl/source/window/brdwin
    vcl/source/window/bufferdevice
    vcl/source/window/accessibility
    vcl/source/window/colorpicker
    vcl/source/window/legacyaccessibility
    vcl/source/window/clipping
    vcl/source/window/stacking
    vcl/source/window/globalization
    vcl/source/window/builder
    vcl/source/window/commandevent
    vcl/source/window/cursor
    vcl/source/window/debugevent
    vcl/source/window/decoview
    vcl/source/window/dialog
    vcl/source/window/dlgctrl
    vcl/source/window/dndeventdispatcher
    vcl/source/window/dndlistenercontainer
    vcl/source/window/dockingarea
    vcl/source/window/dockmgr
    vcl/source/window/dockwin
    vcl/source/window/DocWindow
    vcl/source/window/event
    vcl/source/window/floatwin
    vcl/source/window/hexcolorcontrol
    # $(if $(ENABLE_WASM_STRIP_SPLASH),,vcl/source/window/introwin) — the
    # splash window is dropped only from the wasm build, where the embedder
    # draws the screen and there is nothing to splash.
    vcl/source/window/introwin
    vcl/source/window/keycod
    vcl/source/window/keyevent
    vcl/source/window/layout
    vcl/source/window/menu
    vcl/source/window/menubarwindow
    vcl/source/window/menufloatingwindow
    vcl/source/window/menuitemlist
    vcl/source/window/menuwindow
    vcl/source/window/mnemonic
    vcl/source/window/mouse
    vcl/source/window/NotebookBarAddonsMerger
    vcl/source/window/OptionalBox
    vcl/source/window/printdlg
    vcl/source/window/scrwnd
    vcl/source/window/seleng
    vcl/source/window/split
    vcl/source/window/splitwin
    vcl/source/window/status
    vcl/source/window/syschild
    vcl/source/window/syswin
    vcl/source/window/tabdlg
    vcl/source/window/tabpage
    vcl/source/window/taskpanelist
    vcl/source/window/toolbox2
    vcl/source/window/toolbox
    vcl/source/window/window2
    vcl/source/window/window3
    vcl/source/window/window
    vcl/source/window/winproc
    vcl/source/window/wrkwin
    vcl/source/window/EnumContext
    vcl/source/control/button
    vcl/source/control/calendar
    vcl/source/control/combobox
    vcl/source/control/ctrl
    vcl/source/control/ContextVBox
    vcl/source/control/DropdownBox
    vcl/source/control/edit
    vcl/source/control/field2
    vcl/source/control/field
    vcl/source/control/fixed
    vcl/source/control/fixedhyper
    vcl/source/control/hyperlabel
    vcl/source/control/fmtfield
    vcl/source/control/FormattedField
    vcl/source/control/InterimItemWindow
    vcl/source/control/imgctrl
    vcl/source/control/imivctl1
    vcl/source/control/imivctl2
    vcl/source/control/ivctrl
    vcl/source/control/longcurr
    vcl/source/control/imp_listbox
    vcl/source/control/listbox
    vcl/source/control/managedmenubutton
    vcl/source/control/MenuButton
    vcl/source/control/NotebookbarPopup
    vcl/source/control/PriorityHBox
    vcl/source/control/PriorityMergedHBox
    vcl/source/control/notebookbar
    vcl/source/control/quickselectionengine
    vcl/source/control/prgsbar
    vcl/source/control/roadmap
    vcl/source/control/roadmapwizard
    vcl/source/control/roadmapwizardmachine
    vcl/source/control/scrbar
    vcl/source/control/slider
    vcl/source/control/spinbtn
    vcl/source/control/spinfld
    vcl/source/control/tabctrl
    vcl/source/control/throbber
    vcl/source/control/verticaltabctrl
    vcl/source/control/wizardmachine
    vcl/source/edit/IdleFormatter
    vcl/source/edit/vclmedit
    vcl/source/edit/textdata
    vcl/source/edit/textdoc
    vcl/source/edit/texteng
    vcl/source/edit/textundo
    vcl/source/edit/textview
    vcl/source/edit/txtattr
    vcl/source/edit/xtextedt
    vcl/source/toolkit/group
    vcl/source/toolkit/morebtn
    vcl/source/outdev/CoordinateMapper
    vcl/source/outdev/ImplMapRes
    vcl/source/outdev/background
    vcl/source/outdev/eps
    vcl/source/outdev/outdev
    vcl/source/outdev/EmphasisMarks
    vcl/source/outdev/stack
    vcl/source/outdev/clipping
    vcl/source/outdev/fill
    vcl/source/outdev/polygon
    vcl/source/outdev/transparent
    vcl/source/outdev/mask
    vcl/source/outdev/bitmap
    vcl/source/outdev/bitmapex
    vcl/source/outdev/font
    vcl/source/outdev/text
    vcl/source/outdev/textline
    vcl/source/outdev/pixel
    vcl/source/outdev/rect
    vcl/source/outdev/line
    vcl/source/outdev/polyline
    vcl/source/outdev/hatch
    vcl/source/outdev/gradient
    vcl/source/outdev/curvedshapes
    vcl/source/outdev/wallpaper
    vcl/source/outdev/vclreferencebase
    vcl/source/outdev/nativecontrols
    vcl/source/outdev/map
    vcl/source/text/ImplLayoutArgs
    vcl/source/text/ImplLayoutRuns
    vcl/source/text/mnemonic
    vcl/source/text/TextLayoutCache
    vcl/source/text/textlayout
    vcl/source/treelist/SvLBoxButton
    vcl/source/treelist/SvHeaderTabListBox
    vcl/source/treelist/headbar
    vcl/source/treelist/iconview
    vcl/source/treelist/iconviewimpl
    vcl/source/treelist/imap
    vcl/source/treelist/imap2
    vcl/source/treelist/imap3
    vcl/source/treelist/inetimg
    vcl/source/treelist/svtabbx
    vcl/source/treelist/transfer
    vcl/source/treelist/transfer2
    vcl/source/treelist/viewdataentry
    vcl/source/treelist/treelist
    vcl/source/treelist/treelistbox
    vcl/source/treelist/treelistentry
    vcl/source/treelist/svimpbox
    vcl/source/treelist/svlbitm
    vcl/source/treelist/uiobject
    vcl/source/gdi/formpdfexport
    vcl/source/gdi/cvtgrf
    vcl/source/gdi/embeddedfontsmanager
    vcl/source/gdi/embeddedfontsafdko
    vcl/source/gdi/FileDefinitionWidgetDraw
    vcl/source/gdi/WidgetDefinitionReader
    vcl/source/gdi/WidgetDefinition
    vcl/source/gdi/extoutdevdata
    vcl/source/gdi/gdimtf
    vcl/source/gdi/mtfxmldump
    vcl/source/gdi/gdimetafiletools
    vcl/source/gdi/gfxlink
    vcl/source/gdi/gradient
    vcl/source/gdi/graph
    vcl/source/gdi/graphictools
    vcl/source/gdi/hatch
    vcl/source/gdi/impglyphitem
    vcl/source/gdi/impgraph
    vcl/source/gdi/jobset
    vcl/source/gdi/lineinfo
    vcl/source/gdi/mapmod
    vcl/source/gdi/metaact
    vcl/source/gdi/oldprintadaptor
    vcl/source/gdi/print2
    vcl/source/gdi/print3
    vcl/source/gdi/print
    vcl/source/gdi/regband
    vcl/source/gdi/region
    vcl/source/gdi/regionband
    vcl/source/gdi/salgdilayout
    vcl/source/gdi/salgdiimpl
    vcl/source/gdi/sallayout
    vcl/source/gdi/salmisc
    vcl/source/gdi/vectorgraphicdata
    vcl/source/gdi/virdev
    vcl/source/gdi/wall
    vcl/source/gdi/scrptrun
    vcl/source/gdi/CommonSalLayout
    vcl/source/gdi/TypeSerializer
    vcl/source/graphic/BinaryDataContainer
    vcl/source/graphic/BinaryDataContainerTools
    vcl/source/graphic/GraphicID
    vcl/source/graphic/GraphicLoader
    vcl/source/graphic/GraphicObject
    vcl/source/graphic/GraphicObject2
    vcl/source/graphic/Manager
    vcl/source/graphic/UnoBinaryDataContainer
    vcl/source/graphic/UnoGraphic
    vcl/source/graphic/UnoGraphicMapper
    vcl/source/graphic/UnoGraphicDescriptor
    vcl/source/graphic/UnoGraphicObject
    vcl/source/graphic/UnoGraphicProvider
    vcl/source/graphic/VectorGraphicSearch
    vcl/source/graphic/VectorGraphicLoader
    vcl/source/bitmap/Vectorizer
    vcl/source/bitmap/bitmap
    vcl/source/bitmap/bitmappalette
    vcl/source/bitmap/BitmapInfoAccess
    vcl/source/bitmap/BitmapReadAccess
    vcl/source/bitmap/BitmapWriteAccess
    vcl/source/bitmap/alpha
    vcl/source/bitmap/dibtools
    vcl/source/bitmap/bmpfast
    vcl/source/bitmap/bitmapfilter
    vcl/source/bitmap/bitmappaint
    vcl/source/bitmap/BitmapArithmeticBlendFilter
    vcl/source/bitmap/BitmapShadowFilter
    vcl/source/bitmap/BitmapAlphaClampFilter
    vcl/source/bitmap/BitmapBasicMorphologyFilter
    vcl/source/bitmap/BitmapDarkenBlendFilter
    vcl/source/bitmap/BitmapLightenBlendFilter
    vcl/source/bitmap/BitmapMaskToAlphaFilter
    vcl/source/bitmap/BitmapMonochromeFilter
    vcl/source/bitmap/BitmapMultiplyBlendFilter
    vcl/source/bitmap/BitmapNormalBlendFilter
    vcl/source/bitmap/BitmapScreenBlendFilter
    vcl/source/bitmap/BitmapSmoothenFilter
    vcl/source/bitmap/BitmapLightenFilter
    vcl/source/bitmap/BitmapDisabledImageFilter
    vcl/source/bitmap/BitmapColorizeFilter
    vcl/source/bitmap/BitmapGaussianSeparableBlurFilter
    vcl/source/bitmap/BitmapSobelGreyFilter
    vcl/source/bitmap/BitmapSolarizeFilter
    vcl/source/bitmap/BitmapSepiaFilter
    vcl/source/bitmap/BitmapMosaicFilter
    vcl/source/bitmap/BitmapEmbossGreyFilter
    vcl/source/bitmap/BitmapPopArtFilter
    vcl/source/bitmap/BitmapDuoToneFilter
    vcl/source/bitmap/BitmapConvolutionMatrixFilter
    vcl/source/bitmap/BitmapMedianFilter
    vcl/source/bitmap/BitmapInterpolateScaleFilter
    vcl/source/bitmap/BitmapSeparableUnsharpenFilter
    vcl/source/bitmap/BitmapFastScaleFilter
    vcl/source/bitmap/BitmapScaleSuperFilter
    vcl/source/bitmap/BitmapScaleConvolutionFilter
    vcl/source/bitmap/BitmapSymmetryCheck
    vcl/source/bitmap/BitmapColorQuantizationFilter
    vcl/source/bitmap/BitmapSimpleColorQuantizationFilter
    vcl/source/bitmap/BitmapTools
    vcl/source/bitmap/BlendFrameCache
    vcl/source/bitmap/Octree
    vcl/source/bitmap/salbmp
    vcl/source/image/Image
    vcl/source/image/ImageTree
    vcl/source/image/ImageRepository
    vcl/source/image/ImplImage
    vcl/source/image/ImplImageTree
    vcl/source/bitmap/BitmapFilterStackBlur
    vcl/source/helper/canvasbitmap
    vcl/source/helper/canvastools
    vcl/source/helper/commandinfoprovider
    vcl/source/helper/displayconnectiondispatch
    vcl/source/helper/driverblocklist
    vcl/source/helper/evntpost
    vcl/source/helper/idletask
    vcl/source/helper/strhelper
    vcl/source/helper/threadex
    vcl/source/app/brand
    vcl/source/app/ColorDialog
    vcl/source/app/customweld
    vcl/source/app/dbggui
    vcl/source/app/dndhelp
    vcl/source/app/ClipboardBase
    vcl/source/app/DropTarget
    vcl/source/app/help
    vcl/source/app/i18nhelp
    vcl/source/app/idle
    vcl/source/app/lok
    vcl/source/app/salframe
    vcl/source/app/salinst
    vcl/source/app/salplug
    vcl/source/app/salusereventlist
    vcl/source/app/salvtables
    vcl/source/app/SalYieldMutex
    vcl/source/app/scheduler
    vcl/source/app/session
    vcl/source/app/settings
    vcl/source/app/themecolors
    vcl/source/app/IconHelper
    vcl/source/app/IconThemeInfo
    vcl/source/app/IconThemeScanner
    vcl/source/app/IconThemeSelector
    vcl/source/app/ITiledRenderable
    vcl/source/app/sound
    vcl/source/app/stdtext
    vcl/source/app/svapp
    vcl/source/app/svcache
    vcl/source/app/svdata
    vcl/source/app/svmain
    vcl/source/app/timer
    vcl/source/app/unohelp2
    vcl/source/app/htmltransferable
    vcl/source/app/UserResourceScanner
    vcl/source/app/unohelp
    vcl/source/app/vclevent
    vcl/source/app/watchdog
    vcl/source/app/winscheduler
    vcl/source/components/ColorPicker
    vcl/source/components/dtranscomp
    vcl/source/components/fontident
    vcl/source/filter/bmp/BmpReader
    vcl/source/filter/bmp/BmpWriter
    vcl/source/filter/egif/egif
    vcl/source/filter/egif/giflzwc
    vcl/source/filter/eps/eps
    vcl/source/filter/etiff/etiff
    vcl/source/filter/FilterConfigCache
    vcl/source/filter/FilterConfigItem
    vcl/source/filter/graphicfilter
    vcl/source/filter/graphicfilter2
    vcl/source/filter/GraphicNativeTransform
    vcl/source/filter/GraphicNativeMetadata
    vcl/source/filter/GraphicFormatDetector
    vcl/source/filter/idxf/dxf2mtf
    vcl/source/filter/idxf/dxfblkrd
    vcl/source/filter/idxf/dxfentrd
    vcl/source/filter/idxf/dxfgrprd
    vcl/source/filter/idxf/dxfreprd
    vcl/source/filter/idxf/dxftblrd
    vcl/source/filter/idxf/dxfvec
    vcl/source/filter/idxf/idxf
    vcl/source/filter/ieps/ieps
    vcl/source/filter/igif/decode
    vcl/source/filter/igif/gifread
    vcl/source/filter/imet/ios2met
    vcl/source/filter/ipbm/ipbm
    vcl/source/filter/ipcd/ipcd
    vcl/source/filter/ipcx/ipcx
    vcl/source/filter/ipict/ipict
    vcl/source/filter/ipsd/ipsd
    vcl/source/filter/ipict/shape
    vcl/source/filter/ipdf/pdfcompat
    vcl/source/filter/ipdf/pdfread
    vcl/source/filter/ipdf/pdfdocument
    vcl/source/filter/iras/iras
    vcl/source/filter/itga/itga
    vcl/source/filter/itiff/itiff
    vcl/source/filter/ixbm/xbmread
    vcl/source/filter/ixpm/xpmread
    vcl/source/filter/jpeg/Exif
    vcl/source/filter/jpeg/jpeg
    vcl/source/filter/jpeg/jpegc
    vcl/source/filter/jpeg/JpegReader
    vcl/source/filter/jpeg/JpegWriter
    vcl/source/filter/jpeg/JpegTransform
    vcl/source/filter/jxl/reader
    vcl/source/filter/svm/SvmConverter
    vcl/source/filter/svm/SvmReader
    vcl/source/filter/svm/SvmWriter
    vcl/source/filter/wmf/emfwr
    vcl/source/filter/wmf/wmf
    vcl/source/filter/wmf/wmfexternal
    vcl/source/filter/wmf/wmfwr
    vcl/source/filter/png/PngImageReader
    vcl/source/filter/png/PngImageWriter
    vcl/source/filter/webp/reader
    vcl/source/filter/webp/writer
    vcl/source/font/DirectFontSubstitution
    vcl/source/font/EmphasisMark
    vcl/source/font/Feature
    vcl/source/font/FeatureCollector
    vcl/source/font/FeatureParser
    vcl/source/font/FontSelectPattern
    vcl/source/font/LogicalFontInstance
    vcl/source/font/OpenTypeFeatureDefinitionList
    vcl/source/font/PhysicalFontCollection
    vcl/source/font/PhysicalFontFace
    vcl/source/font/PhysicalFontFamily
    vcl/source/font/fontattributes
    vcl/source/font/fontcache
    vcl/source/font/fontcharmap
    vcl/source/font/fontmetric
    vcl/source/font/font
    vcl/source/font/FontVariation
    vcl/source/font/EOTConverter
    vcl/source/font/TrueTypeFont
    vcl/source/pdf/COSWriter
    vcl/source/pdf/EncryptionHashTransporter
    vcl/source/pdf/ExternalPDFStreams
    vcl/source/pdf/PDFiumTools
    vcl/source/pdf/PDFEncryptionInitialization
    vcl/source/pdf/PDFEncryptor
    vcl/source/pdf/PDFEncryptorR6
    vcl/source/pdf/PdfConfig
    vcl/source/pdf/pwdinteract
    vcl/source/pdf/ResourceDict
    vcl/source/pdf/Matrix3
    vcl/source/pdf/XmpMetadata
    vcl/source/pdf/pdfobjectcopier
    vcl/source/pdf/pdfbuildin_fonts
    vcl/source/pdf/pdfextoutdevdata
    vcl/source/pdf/pdfwriter
    vcl/source/pdf/pdfwriterimpl_utils
    vcl/source/pdf/PDFPage
    vcl/source/pdf/pdfwriter_impl2
    vcl/source/pdf/pdfwriter_impl
    vcl/source/uitest/logger
    vcl/source/uitest/toolboxitemuiobject
    vcl/source/uitest/toolboxuiobject
    vcl/source/uitest/uiobject
    vcl/source/uitest/uitest
    vcl/source/uitest/uno/uiobject_uno
    vcl/source/uitest/uno/uitest_uno
    vcl/source/weld/Assistant
    vcl/source/weld/AssistantController
    vcl/source/weld/Builder
    vcl/source/weld/ComboBox
    vcl/source/weld/DateFormatter
    vcl/source/weld/Dialog
    vcl/source/weld/DialogController
    vcl/source/weld/DoubleNumericFormatter
    vcl/source/weld/EntryFormatter
    vcl/source/weld/EntryTreeView
    vcl/source/weld/IconView
    vcl/source/weld/ItemView
    vcl/source/weld/LongCurrencyFormatter
    vcl/source/weld/MetricSpinButton
    vcl/source/weld/PatternFormatter
    vcl/source/weld/ScrolledWindow
    vcl/source/weld/SpinButton
    vcl/source/weld/TextView
    vcl/source/weld/TextWidget
    vcl/source/weld/TimeFormatter
    vcl/source/weld/TransportAsXWindow
    vcl/source/weld/TreeView
    vcl/source/weld/TriStateEnabled
    vcl/source/weld/WaitObject
    vcl/source/weld/weldutils
    vcl/backendtest/outputdevice/bitmap
    vcl/backendtest/outputdevice/clip
    vcl/backendtest/outputdevice/common
    vcl/backendtest/outputdevice/gradient
    vcl/backendtest/outputdevice/line
    vcl/backendtest/outputdevice/outputdevice
    vcl/backendtest/outputdevice/pixel
    vcl/backendtest/outputdevice/polygon
    vcl/backendtest/outputdevice/polypolygon
    vcl/backendtest/outputdevice/polypolygon_b2d
    vcl/backendtest/outputdevice/polyline
    vcl/backendtest/outputdevice/polyline_b2d
    vcl/backendtest/outputdevice/text
    vcl/backendtest/outputdevice/rectangle
    vcl/backendtest/GraphicsRenderTests

    # --- USING_X11, Library_vcl.mk lines 691-708 -----------------------------
    #
    # The desktop detector is X11 code that nevertheless belongs to libvcl
    # rather than to the plug-in: it is what tells the toolkit which desktop is
    # running.  $(if $(ENABLE_CPDB), vcl/unx/generic/printer/cpdmgr) is absent —
    # that is upstream's option for the Common Print Dialog Backends, and this
    # build does not turn it on.

    vcl/unx/generic/desktopdetect/desktopdetector

    # --- the GUI backends, Library_vcl.mk lines 716-740 ----------------------
    #
    # The DISABLE_GUI branch would add vcl/headless/headlessinst here instead of
    # these two; this build is a GUI one.

    vcl/source/opengl/OpenGLContext
    vcl/source/opengl/OpenGLHelper

    # $(if $(filter SKIA,$(BUILD_TYPE)), vcl/skia/salbmp zone gdiimpl) is
    # absent, BUILD_TYPE being DESKTOP.  Note that vcl/skia/SkiaHelper above is
    # *not* conditional: it compiles in every build and guards its Skia calls
    # internally.  $(if $(filter LINUX SOLARIS %BSD,$(OS)), ...) is the branch
    # that does apply — the session inhibitor talks to the desktop's idle
    # handling to keep the screen from blanking mid-presentation.

    vcl/unx/generic/window/sessioninhibitor

    # --- the headless code, Library_vcl.mk lines 650-683 and 747-761 ----------
    #
    # USE_HEADLESS_CODE does not mean "a build without a GUI".  configure.ac
    # defines it whenever the build uses freetype and fontconfig, which is what
    # a Linux desktop build does, so it is on here as well: it is what puts the
    # generic font handling, the printer back ends and the pieces the X11
    # plug-in shares into libvcl rather than into the plug-in.  Reading it as
    # the DISABLE_GUI case and leaving this block out costs about thirty
    # objects, and the link then fails on X11Helper::getFrameClassName(), which
    # vcl/unx/generic/window/sessioninhibitor.cxx above calls and which only
    # this block defines.
    #
    # $(if $(ENABLE_CUPS), vcl/unx/generic/printer/cupsmgr) is here because CUPS
    # is present, and printerinfomanager because the line above it picks between
    # it and vcl/null/printerinfomanager depending on whether CUPS or CPDB is
    # on — with CUPS on, it is the real one.

    vcl/unx/generic/printer/cupsmgr
    vcl/unx/generic/printer/printerinfomanager

    # vcl_headless_code, lines 650-664.  The $(if $(filter-out iOS,$(OS)), ...)
    # around the middle five is the branch that applies.
    vcl/headless/svpframe
    vcl/headless/svpbmp
    vcl/headless/svpgdi
    vcl/headless/SvpGraphicsBackend
    vcl/headless/CairoCommon
    vcl/headless/BitmapHelper
    vcl/headless/svpdummies
    vcl/headless/svpinst
    vcl/headless/svpvd
    vcl/unx/generic/app/X11Helper
    vcl/unx/generic/app/gendisp
    vcl/unx/generic/app/geninst

    # vcl_headless_generic_code, lines 666-683.
    vcl/headless/svpprn
    vcl/headless/svptext
    vcl/unx/generic/app/gendata
    vcl/unx/generic/gdi/cairotextrender
    vcl/unx/generic/gdi/generictextrender
    vcl/unx/generic/font/GenericFont
    vcl/unx/generic/font/GenericFontFace
    vcl/unx/generic/font/GenericFontList
    vcl/unx/generic/font/fontsubst
    vcl/unx/generic/font/fontconfig
    vcl/unx/generic/font/fontmanager
    vcl/unx/generic/print/helper
    vcl/unx/generic/print/genpspgraphics
    vcl/unx/generic/print/genprnpsp
    vcl/unx/generic/print/prtsetup
    vcl/unx/generic/printer/jobdata
    vcl/unx/generic/printer/ppdparser

    # --- the PDF backend, Library_vcl.mk line 760 ----------------------------
    #
    # $(if $(filter PDFIUM,$(BUILD_TYPE)),,Dummy)PDFiumLibrary picks the name:
    # with PDFIUM it is PDFiumLibrary, and otherwise the stub that reports the
    # feature as unavailable.  DESKTOP means the stub.

    vcl/source/pdf/DummyPDFiumLibrary
)

librestudio_expand_cxx(VCL_SOURCES)
# vcl/source/filter/jpeg/transupp is the single C object, which is why
# gb_Library_add_cobjects lists it apart from the rest; it is libjpeg's
# transform helper, compiled into the same library.
add_library(vcl SHARED
    ${VCL_SOURCES}
    "${CMAKE_SOURCE_DIR}/vcl/source/filter/jpeg/transupp.c")

# --- externals ----------------------------------------------------------------
#
# The three add_externals blocks of the .mk, in its order.  Their gbuild names
# map onto pkg-config modules the way RepositoryExternal.mk defines them — gio
# is gio-2.0, graphite is graphite2 — except for jpeg, png and tiff, which come
# from CMake's own find modules.

find_package(PNG REQUIRED)
find_package(TIFF REQUIRED)
find_package(JPEG REQUIRED)

# Library_vcl.mk lines 92-108.  harfbuzz is not here: it has to be 14.3 or newer
# and the system is older, so the top-level CMakeLists.txt bundles it and hands
# the result over in LIBRESTUDIO_HARFBUZZ_TARGETS — see the comment there.
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)
pkg_check_modules(GRAPHITE REQUIRED IMPORTED_TARGET graphite2)
pkg_check_modules(LCMS2 REQUIRED IMPORTED_TARGET lcms2)
pkg_check_modules(LIBEOT REQUIRED IMPORTED_TARGET libeot)
pkg_check_modules(LIBWEBP REQUIRED IMPORTED_TARGET libwebp)
# libjxl is not optional and not guarded by an #if anywhere: the reader in
# vcl/source/filter/jxl/reader.cxx includes it unconditionally.  It comes in two
# halves, and the second is not optional either — JxlResizableParallelRunner and
# its friends, which the reader uses to decode across threads, are in
# libjxl_threads rather than in libjxl.
pkg_check_modules(JXL REQUIRED IMPORTED_TARGET libjxl)
pkg_check_modules(JXL_THREADS REQUIRED IMPORTED_TARGET libjxl_threads)

# Library_vcl.mk lines 699-702, the USING_X11 block.  valgrind is headers only:
# vcl/source/app/watchdog.cxx asks the valgrind client whether the program is
# running under valgrind, so that the watchdog does not fire there.  There is no
# library to link.
pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)
find_path(VALGRIND_INCLUDE_DIR valgrind/valgrind.h)
if (NOT VALGRIND_INCLUDE_DIR)
    message(FATAL_ERROR
        "valgrind headers are required by vcl/source/app/watchdog.cxx")
endif ()

# Library_vcl.mk lines 736-739, the GUI branch of the DISABLE_GUI choice.
# $(if $(filter SKIA,$(BUILD_TYPE)),skia) is absent with it.
pkg_check_modules(EPOXY REQUIRED IMPORTED_TARGET epoxy)

# Library_vcl.mk lines 763-773, the externals of the USE_HEADLESS_CODE branch:
# the headless back end draws with cairo, the fonts come from fontconfig, and
# $(if $(ENABLE_CUPS),cups) is there because CUPS is.  $(if $(ENABLE_CPDB),cpdb)
# is not — the option is off — and neither are pdfium or afdko, which belong to
# build types this one is not.
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
pkg_check_modules(FONTCONFIG REQUIRED IMPORTED_TARGET fontconfig)
pkg_check_modules(CUPS_LIB REQUIRED IMPORTED_TARGET cups)

# Library_vcl.mk lines 704-707 and 710-714 — the X11 libraries, and the -lm
# that every unix build adds.  Lines 775-780 add -lm a second time for the same
# platforms; once is enough here.
pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)
pkg_check_modules(XEXT REQUIRED IMPORTED_TARGET xext)

target_include_directories(vcl PRIVATE
    "${CMAKE_SOURCE_DIR}/vcl/inc"
    "${FROZEN_INCLUDE_DIR}"
    "${VALGRIND_INCLUDE_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(vcl
    officecfg_headers udkapi_normal_headers offapi_normal_headers)

# Library_vcl.mk lines 45-52.  Two of the six entries do not apply: with a
# bundled libfixmath rather than a system one there is no -DSYSTEM_LIBFIXMATH,
# and -DDO_USE_TTF_ON_WIN32 is for the Windows headless build.
#
# CUI_DLL_NAME and TK_DLL_NAME are library names vcl passes to dlopen by hand.
# gb_Library_get_runtime_filename() produced them upstream; both follow the
# naming rule in cmake/LibreStudioPlatform.cmake, where an office-layer library
# is lib<name>lo.so.
target_compile_definitions(vcl PRIVATE
    VCL_DLLIMPLEMENTATION
    DLLIMPLEMENTATION_UITEST
    CUI_DLL_NAME="libcuilo.so"
    TK_DLL_NAME="libtklo.so")

# Library_vcl.mk lines 60-83.  Three entries there are conditional and none of
# them applies: crashreport is for BREAKPAD, which this build does not use;
# jvmaccess is for ENABLE_JAVA; opencl is for a BUILD_TYPE of OPENCL.  The cairo
# entry at the end of that list is for the Windows headless build, as are the
# static fontconfig and freetype of lines 85-90.  The externals of lines 92-108
# are linked below.
target_link_libraries(vcl PRIVATE
    basegfx comphelper cppu cppuhelper drawinglayercore i18nlangtag i18nutil
    sal salhelper sax sot svl tl ucbhelper utl xmlreader
    Boost::boost
    EXPAT::EXPAT
    ICU::uc
    JPEG::JPEG PNG::PNG TIFF::TIFF
    ${LIBRESTUDIO_HARFBUZZ_TARGETS}
    PkgConfig::GIO PkgConfig::GRAPHITE PkgConfig::LCMS2
    PkgConfig::LIBEOT PkgConfig::LIBWEBP PkgConfig::JXL PkgConfig::JXL_THREADS
    PkgConfig::DBUS PkgConfig::EPOXY PkgConfig::X11 PkgConfig::XEXT
    PkgConfig::CAIRO PkgConfig::FONTCONFIG PkgConfig::CUPS_LIB
    m)

# vcl.common.component is the component file, and vcl.common.component.unx the
# list of implementations this platform adds to it — the two are combined
# through the filtered parameter, see librestudio_add_component().  The ifeq
# chain at the top of the .mk picks that suffix by platform; unx is its else
# branch, which is also the one the headless alternative does not take.
librestudio_add_component(vcl
    COMPONENT      "${CMAKE_SOURCE_DIR}/vcl/vcl.common.component"
    IMPLEMENTATION "${CMAKE_SOURCE_DIR}/vcl/vcl.common.component.unx"
    LIBRARY        vcl
    LAYER          OOO)

librestudio_set_library_output_name(vcl vcl OOO)
install(TARGETS vcl LIBRARY DESTINATION program)

# --- what is not here ---------------------------------------------------------
#
# Every remaining branch of Library_vcl.mk, so that their absence reads as a
# decision rather than an oversight.  Each is a platform or a build type this
# build is not.
#
#   OS=WNT       lines 854-876: the Win32 sources, its system libraries, the
#                resource file, and the icon-theme package dependency.  The
#                Windows component implementations replace the unx ones.
#   OS=MACOSX    lines 839-851: the Quartz objective-C++ sources and the Cocoa,
#                CoreFoundation and Metal frameworks.
#   OS=iOS       lines 806-836: the Quartz and ios sources, the headless code
#                again, and the UIKit and CoreFoundation frameworks.
#   OS=ANDROID   lines 794-803: the android sources and -llog -landroid.
#   OS=HAIKU     lines 787-791: -lbe.
#
#   DISABLE_GUI  lines 716-720 — a build without a toolkit at all.  It takes
#                vcl/headless/headlessinst instead of the OpenGL sources, and
#                the headless component implementation.  USE_HEADLESS_CODE goes
#                with it: lines 650-683 define the headless and unx-generic
#                object lists, lines 747-761 pull them in, and lines 763-773
#                pull in cairo, cups and fontconfig for them.  None of that
#                happens when a GUI is built, which is why this file needs
#                neither cairo nor fontconfig.
#
#   BUILD_TYPE   OPENCL adds the opencl library (line 70); SKIA adds the Skia
#                objects and the skia external (lines 726-730, 738);
#                PDFIUM swaps the stub for PDFiumLibrary (line 760) and adds
#                the pdfium external (line 771); AFDKO adds afdko and libxml2
#                (line 772).  DESKTOP is none of these.
#
#   Features     ENABLE_JAVA adds jvmaccess (line 69), BREAKPAD adds
#                crashreport (line 65), ENABLE_CPDB adds cpdmgr and the cpdb
#                external (lines 694, 767), ENABLE_CUPS the cups ones (line
#                749, 768), ENABLE_WASM_STRIP_SPLASH removes introwin (line
#                205), and a system libfixmath adds -DSYSTEM_LIBFIXMATH and
#                links it (lines 50, 685-689).
