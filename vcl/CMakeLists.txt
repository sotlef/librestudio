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

)
# --- the switches of Library_vcl.mk -------------------------------------------
#
# The rest of this file decides what belongs to the library the way the .mk
# does: with conditionals.  Each one is an if() on a named variable, so that
# changing a switch changes the build — a condition written into a comment
# instead is a condition that silently stops being true when someone flips the
# flag.
#
# Most of these are not vcl's to decide.  USE_HEADLESS_CODE, USING_X11,
# ENABLE_CUPS, ENABLE_CPDB, ENABLE_WASM_STRIP_SPLASH and DISABLE_GUI are set in
# cmake/LibreStudioConfigHeaders.cmake, because the config headers answer the
# same questions for the whole tree and vcl is not their only reader.  They
# carry the values configure.ac would compute on this machine.
#
# The platform branches — WNT, MACOSX, iOS, ANDROID, HAIKU — are the one
# deliberate exception, and they are listed at the end of the file rather than
# written out.  cmake/LibreStudioPlatform.cmake pins this build to Linux/x86_64
# and stops with an error anywhere else, so those branches could be written but
# never compiled; leaving them out is honest, whereas writing them would claim
# support the rest of the build refuses to give.

# configure.ac's BUILD_TYPE, which is DESKTOP plus any of OPENCL, SKIA, PDFIUM
# and AFDKO that were asked for.  Only DESKTOP is supported here yet.
set(LIBRESTUDIO_BUILD_TYPE "DESKTOP" CACHE STRING
    "configure.ac's BUILD_TYPE: DESKTOP, plus OPENCL, SKIA, PDFIUM or AFDKO")

option(LIBRESTUDIO_ENABLE_BREAKPAD "Link the breakpad crash reporter" OFF)
option(LIBRESTUDIO_SYSTEM_LIBFIXMATH
    "Use a system libfixmath instead of the fix16 bundled in tools/" OFF)

# --- the conditional sources --------------------------------------------------

if (USING_X11)
    # Library_vcl.mk lines 691-697.  The desktop detector is X11 code that
    # belongs to libvcl rather than to the plug-in: it is what tells the toolkit
    # which desktop is running.
    list(APPEND VCL_SOURCES
        vcl/unx/generic/desktopdetect/desktopdetector)

    if (ENABLE_CPDB)
        list(APPEND VCL_SOURCES vcl/unx/generic/printer/cpdmgr)
    endif ()
else ()
    message(FATAL_ERROR
        "Library_vcl.mk has no branch for a build without X11; the headless "
        "and Windows cases would need writing first")
endif ()

if (NOT DISABLE_GUI)
    # Library_vcl.mk lines 723-734.
    list(APPEND VCL_SOURCES
        vcl/source/opengl/OpenGLContext
        vcl/source/opengl/OpenGLHelper)

    if ("SKIA" IN_LIST LIBRESTUDIO_BUILD_TYPE)
        list(APPEND VCL_SOURCES
            vcl/skia/salbmp
            vcl/skia/zone
            vcl/skia/gdiimpl)
    endif ()

    if (UNIX AND NOT APPLE)
        list(APPEND VCL_SOURCES vcl/unx/generic/window/sessioninhibitor)
    endif ()
else ()
    list(APPEND VCL_SOURCES vcl/headless/headlessinst)
endif ()

if (USE_HEADLESS_CODE)
    # Library_vcl.mk lines 747-759 — and this is not the DISABLE_GUI case.
    # configure.ac defines USE_HEADLESS_CODE whenever the build uses freetype
    # and fontconfig, which is what a Linux desktop build does, so a GUI build
    # has it on too.  It is what puts the generic font handling, the printer
    # back ends and the pieces the X11 plug-in shares into libvcl rather than
    # into the plug-in.
    if (ENABLE_CUPS)
        list(APPEND VCL_SOURCES vcl/unx/generic/printer/cupsmgr)
    endif ()

    # $(if $(filter TRUE,$(ENABLE_CPDB) $(ENABLE_CUPS)), a, b) — with neither
    # printing back end there is nothing to ask, so the null one is used.
    if (ENABLE_CPDB OR ENABLE_CUPS)
        list(APPEND VCL_SOURCES vcl/unx/generic/printer/printerinfomanager)
    else ()
        list(APPEND VCL_SOURCES vcl/null/printerinfomanager)
    endif ()

    # vcl_headless_code, lines 650-664, and vcl_headless_generic_code, lines
    # 666-683.  The $(if $(filter-out iOS,$(OS)), ...) around the middle of the
    # first only ever drops five entries for iOS.
    list(APPEND VCL_SOURCES
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
        vcl/unx/generic/printer/ppdparser)
endif ()

# Library_vcl.mk line 760: $(if $(filter PDFIUM,$(BUILD_TYPE)),,Dummy)PDFiumLibrary
# picks the name — PDFiumLibrary with PDFIUM, the stub otherwise.
if ("PDFIUM" IN_LIST LIBRESTUDIO_BUILD_TYPE)
    list(APPEND VCL_SOURCES vcl/source/pdf/PDFiumLibrary)
else ()
    list(APPEND VCL_SOURCES vcl/source/pdf/DummyPDFiumLibrary)
endif ()

librestudio_expand_cxx(VCL_SOURCES)
# vcl/source/filter/jpeg/transupp is the single C object, which is why
# gb_Library_add_cobjects lists it apart from the rest; it is libjpeg's
# transform helper, compiled into the same library.
add_library(vcl SHARED
    ${VCL_SOURCES}
    "${CMAKE_SOURCE_DIR}/vcl/source/filter/jpeg/transupp.c")

# --- externals ----------------------------------------------------------------
#
# The four add_externals blocks of the .mk, in its order, each under the same
# condition as there.  The gbuild names map onto pkg-config modules the way
# RepositoryExternal.mk defines them — gio is gio-2.0, graphite is graphite2 —
# except for jpeg, png and tiff, which come from CMake's own find modules.

# Library_vcl.mk lines 92-108: no condition around any of these.
find_package(PNG REQUIRED)
find_package(TIFF REQUIRED)
find_package(JPEG REQUIRED)

pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)
pkg_check_modules(GRAPHITE REQUIRED IMPORTED_TARGET graphite2)
pkg_check_modules(LCMS2 REQUIRED IMPORTED_TARGET lcms2)
pkg_check_modules(LIBEOT REQUIRED IMPORTED_TARGET libeot)
pkg_check_modules(LIBWEBP REQUIRED IMPORTED_TARGET libwebp)
# libjxl is not optional and not guarded by an #if anywhere: the reader in
# vcl/source/filter/jxl/reader.cxx includes it unconditionally.  It comes in two
# halves, and the second is not optional either — JxlResizableParallelRunner and
# its friends, which the reader decodes across threads with, are in
# libjxl_threads rather than in libjxl.
pkg_check_modules(JXL REQUIRED IMPORTED_TARGET libjxl)
pkg_check_modules(JXL_THREADS REQUIRED IMPORTED_TARGET libjxl_threads)
# harfbuzz is not here: it has to be 14.3 or newer and the system is older, so
# the top-level CMakeLists.txt bundles it and hands the result over in
# LIBRESTUDIO_HARFBUZZ_TARGETS — see the comment there.

if (USING_X11)
    # Lines 699-702.  valgrind is headers only: vcl/source/app/watchdog.cxx asks
    # the valgrind client whether the program is running under valgrind, so that
    # the watchdog does not fire there.  There is no library to link.
    pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)
    pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)
    pkg_check_modules(XEXT REQUIRED IMPORTED_TARGET xext)
    find_path(VALGRIND_INCLUDE_DIR valgrind/valgrind.h)
    if (NOT VALGRIND_INCLUDE_DIR)
        message(FATAL_ERROR
            "valgrind headers are required by vcl/source/app/watchdog.cxx")
    endif ()
endif ()

if (NOT DISABLE_GUI)
    # Lines 736-739.  $(if $(filter SKIA,$(BUILD_TYPE)),skia) is in the same
    # place and is added when that build type is asked for.
    pkg_check_modules(EPOXY REQUIRED IMPORTED_TARGET epoxy)
    if ("SKIA" IN_LIST LIBRESTUDIO_BUILD_TYPE)
        pkg_check_modules(SKIA REQUIRED IMPORTED_TARGET skia)
    endif ()
endif ()

if (USE_HEADLESS_CODE)
    # Lines 763-773: the headless back end draws with cairo, the fonts come from
    # fontconfig, and the printing back ends bring their own libraries.
    pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
    pkg_check_modules(FONTCONFIG REQUIRED IMPORTED_TARGET fontconfig)
    if (ENABLE_CUPS)
        pkg_check_modules(CUPS_LIB REQUIRED IMPORTED_TARGET cups)
    endif ()
    if (ENABLE_CPDB)
        pkg_check_modules(CPDB REQUIRED IMPORTED_TARGET cpdb)
    endif ()
endif ()

target_include_directories(vcl PRIVATE
    "${CMAKE_SOURCE_DIR}/vcl/inc"
    "${FROZEN_INCLUDE_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(vcl
    officecfg_headers udkapi_normal_headers offapi_normal_headers)

if (USING_X11)
    target_include_directories(vcl PRIVATE "${VALGRIND_INCLUDE_DIR}")
endif ()

# Library_vcl.mk lines 45-52.  -DDO_USE_TTF_ON_WIN32 belongs to the Windows
# headless build and is not written out; -DSYSTEM_LIBFIXMATH to a system
# libfixmath rather than the fix16 bundled in the tree, which is what is used
# unless the option above asks otherwise.
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

if (LIBRESTUDIO_SYSTEM_LIBFIXMATH)
    find_library(LIBFIXMATH_LIBRARY NAMES fixmath REQUIRED)
    target_compile_definitions(vcl PRIVATE SYSTEM_LIBFIXMATH)
    target_link_libraries(vcl PRIVATE "${LIBFIXMATH_LIBRARY}")
endif ()

# Library_vcl.mk lines 60-83, plus the conditional entries of that list and of
# the add_libs blocks.  crashreport is for BREAKPAD, jvmaccess for ENABLE_JAVA,
# opencl for a BUILD_TYPE of OPENCL, and cairo and fontconfig at the end of the
# .mk's list are for the Windows headless build — the unix headless build pulls
# them in above, through the externals block.
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
    m)

if (LIBRESTUDIO_ENABLE_BREAKPAD)
    target_link_libraries(vcl PRIVATE crashreport)
endif ()

if (LIBRESTUDIO_ENABLE_JAVA)
    target_link_libraries(vcl PRIVATE jvmaccess)
endif ()

if ("OPENCL" IN_LIST LIBRESTUDIO_BUILD_TYPE)
    target_link_libraries(vcl PRIVATE opencl)
endif ()

if (USING_X11)
    target_link_libraries(vcl PRIVATE
        PkgConfig::DBUS PkgConfig::X11 PkgConfig::XEXT)
endif ()

if (NOT DISABLE_GUI)
    target_link_libraries(vcl PRIVATE PkgConfig::EPOXY)
    if ("SKIA" IN_LIST LIBRESTUDIO_BUILD_TYPE)
        target_link_libraries(vcl PRIVATE PkgConfig::SKIA)
    endif ()
endif ()

if (USE_HEADLESS_CODE)
    target_link_libraries(vcl PRIVATE PkgConfig::CAIRO PkgConfig::FONTCONFIG)
    if (ENABLE_CUPS)
        target_link_libraries(vcl PRIVATE PkgConfig::CUPS_LIB)
    endif ()
    if (ENABLE_CPDB)
        target_link_libraries(vcl PRIVATE PkgConfig::CPDB)
    endif ()
endif ()

if ("AFDKO" IN_LIST LIBRESTUDIO_BUILD_TYPE)
    # $(if $(filter AFDKO,$(BUILD_TYPE)),afdko libxml2) from line 772.
    target_link_libraries(vcl PRIVATE LibXml2::LibXml2)
endif ()

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
# The platform branches: OS=WNT (Library_vcl.mk lines 854-876, the Win32
# sources, system libraries, resource file and icon-theme package), OS=MACOSX
# (839-851, the Quartz sources and the Cocoa, CoreFoundation and Metal
# frameworks), OS=iOS (806-836), OS=ANDROID (794-803) and OS=HAIKU (787-791).
#
# cmake/LibreStudioPlatform.cmake pins this build to Linux/x86_64 and stops with
# an error on anything else, so these could be written but never compiled.
# Writing them would claim support the rest of the build refuses to give; the
# list above is where to start when that changes.
#
# Everything else the .mk switches on is expressed above as a condition rather
# than described, so it is not repeated here.
