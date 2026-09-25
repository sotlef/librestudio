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

# Library_vcl.mk line 205.  This is the one condition that sits in the middle of
# the object list rather than in a block of its own, which is how it came to be
# written down as a comment next to an unconditional source; it is a condition
# like any other.  The splash window is dropped only from the wasm build, where
# the embedder draws the screen and there is nothing to splash.
if (NOT ENABLE_WASM_STRIP_SPLASH)
    list(APPEND VCL_SOURCES vcl/source/window/introwin)
endif ()

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

# --- vclplug_gen ---------------------------------------------------------------

# The back end that puts libvcl on an X11 screen.  It is a plug-in: libvcl
# dlopens it by name at run time, and gb_Library_set_plugin_for(vclplug_gen,vcl)
# — unlike the _nodep variant — also links vcl into it.  That is why vcl is
# among the libraries below although Library_vclplug_gen.mk never names it; only
# the non-trivial half is in that file.
#
# The whole library is under ENABLE_GEN, as it is under $(if $(ENABLE_GEN), ...)
# in Module_vcl.mk.

# libSM and libICE ship no pkg-config file on this distribution, so they are
# found by name.  Both back ends ask for them — Library_vclplug_gen.mk lines
# 70-75 and 147-152, Library_vclplug_gtk3.mk line 57 — so they are found once,
# outside either block.  -lm and -ldl are the unix branch of the same rule and
# stay with the library that asks for them.
find_library(SM_LIBRARY NAMES SM REQUIRED)
find_library(ICE_LIBRARY NAMES ICE REQUIRED)

if (ENABLE_GEN)
    set(VCLPLUG_GEN_SOURCES
        vcl/unx/generic/app/X11DisplayConnectionDispatch
        vcl/unx/generic/app/i18n_cb
        vcl/unx/generic/app/i18n_ic
        vcl/unx/generic/app/i18n_im
        vcl/unx/generic/app/i18n_keysym
        vcl/unx/generic/app/i18n_xkb
        vcl/unx/generic/app/keysymnames
        vcl/unx/generic/app/randrwrapper
        vcl/unx/generic/app/saldata
        vcl/unx/generic/app/saldisp
        vcl/unx/generic/app/salinst
        vcl/unx/generic/app/saltimer
        vcl/unx/generic/app/sm
        vcl/unx/generic/app/wmadaptor
        vcl/unx/generic/dtrans/bmp
        vcl/unx/generic/dtrans/X11_clipboard
        vcl/unx/generic/dtrans/X11_dndcontext
        vcl/unx/generic/dtrans/X11_droptarget
        vcl/unx/generic/dtrans/X11_selection
        vcl/unx/generic/dtrans/X11_service
        vcl/unx/generic/dtrans/X11_transferable
        vcl/unx/generic/gdi/cairo_xlib_cairo
        vcl/unx/generic/gdi/X11CairoSalGraphicsImpl
        vcl/unx/generic/gdi/font
        vcl/unx/generic/gdi/salgdi
        vcl/unx/generic/gdi/salvd
        vcl/unx/generic/window/salframe
        vcl/unx/generic/window/salobj
        vcl/unx/x11/x11sys
        vcl/unx/x11/xlimits
        vcl/source/opengl/x11/context)

    if ("SKIA" IN_LIST LIBRESTUDIO_BUILD_TYPE)
        list(APPEND VCLPLUG_GEN_SOURCES
            vcl/skia/x11/gdiimpl
            vcl/skia/x11/salvd
            vcl/skia/x11/textrender)
    endif ()

    librestudio_expand_cxx(VCLPLUG_GEN_SOURCES)
    add_library(vclplug_gen SHARED ${VCLPLUG_GEN_SOURCES})

    target_include_directories(vclplug_gen PRIVATE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${VALGRIND_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(vclplug_gen
        officecfg_headers udkapi_normal_headers offapi_normal_headers)

    # Library_vclplug_gen.mk lines 119-122.
    target_compile_definitions(vclplug_gen PRIVATE
        VCLPLUG_GEN_IMPLEMENTATION
        VCL_INTERNALS)

    # Lines 124-132: RandR lets the back end notice a display change instead of
    # being told about it.
    if (ENABLE_RANDR)
        pkg_check_modules(XRANDR REQUIRED IMPORTED_TARGET xrandr)
        target_compile_definitions(vclplug_gen PRIVATE USE_RANDR)
    endif ()

    # Lines 53-68.  skia and fontconfig are alternatives there, not additions:
    # the Skia back end does its own font handling.
    pkg_check_modules(CAIRO_GEN REQUIRED IMPORTED_TARGET cairo)
    pkg_check_modules(GRAPHITE_GEN REQUIRED IMPORTED_TARGET graphite2)
    pkg_check_modules(EPOXY_GEN REQUIRED IMPORTED_TARGET epoxy)
    pkg_check_modules(XRENDER REQUIRED IMPORTED_TARGET xrender)
    if ("SKIA" IN_LIST LIBRESTUDIO_BUILD_TYPE)
        pkg_check_modules(SKIA_GEN REQUIRED IMPORTED_TARGET skia)
    else ()
        pkg_check_modules(FONTCONFIG_GEN REQUIRED IMPORTED_TARGET fontconfig)
    endif ()

    target_link_libraries(vclplug_gen PRIVATE
        vcl
        tl utl sot ucbhelper basegfx comphelper cppuhelper i18nlangtag i18nutil
        cppu sal
        Boost::boost
        EXPAT::EXPAT
        ICU::uc
        ${LIBRESTUDIO_HARFBUZZ_TARGETS}
        PkgConfig::CAIRO_GEN PkgConfig::GRAPHITE_GEN PkgConfig::EPOXY_GEN
        PkgConfig::XRENDER PkgConfig::X11 PkgConfig::XEXT
        "${SM_LIBRARY}" "${ICE_LIBRARY}"
        m dl)

    if (ENABLE_RANDR)
        target_link_libraries(vclplug_gen PRIVATE PkgConfig::XRANDR)
    endif ()

    if ("SKIA" IN_LIST LIBRESTUDIO_BUILD_TYPE)
        target_link_libraries(vclplug_gen PRIVATE PkgConfig::SKIA_GEN)
    else ()
        target_link_libraries(vclplug_gen PRIVATE PkgConfig::FONTCONFIG_GEN)
    endif ()

    # Lines 135-145: Xinerama lets a window span several monitors.  configure.ac
    # picks a static link when the library is only there as an archive; that is
    # the rare case, and the shared one is what is written here.
    if (USING_X11)
        pkg_check_modules(XINERAMA REQUIRED IMPORTED_TARGET xinerama)
        target_link_libraries(vclplug_gen PRIVATE PkgConfig::XINERAMA)
    endif ()

    # $(if $(ENABLE_JAVA), jvmaccess) from the library list.
    if (LIBRESTUDIO_ENABLE_JAVA)
        target_link_libraries(vclplug_gen PRIVATE jvmaccess)
    endif ()

    # No component file: a plug-in is named by SAL_USE_VCLPLUGIN, not registered
    # as a UNO service.
    librestudio_set_library_output_name(vclplug_gen vclplug_gen OOO)
    install(TARGETS vclplug_gen LIBRARY DESTINATION program)
endif ()

# --- vclplug_gtk3 --------------------------------------------------------------

# The back end that puts libvcl on a GTK desktop.  Same shape as vclplug_gen: a
# plug-in, dlopen()ed by name, with vcl on its link line because
# gb_Library_set_plugin_for(vclplug_gtk3,vcl) is the non-_nodep variant.
#
# What it is made of is not the same.  Two thirds of the objects are the
# accessibility bridge — vcl/unx/gtk3/a11y/atk* wraps every VCL accessible
# object in an ATK one, which is how a screen reader reads an office window —
# and the rest is the GTK file and folder pickers, the native GTK widgets drawn
# where the toolkit asks for them, the GTK window and instance classes, and the
# global-menu and HUD integration.  Nothing is generated: no .ui file, no
# resource, and the widgets are built in code.
#
# GSTREAMER_1_0_CFLAGS is in the .mk's include list but no object in its list
# includes a gst header, so it is left out rather than looked for.
#
# The switch agrees with config_vclplug.h, which has to be so: libvcl builds its
# autodetection and fallback lists under ENABLE_GTK3, so a back end that is
# linked but not named there is never loaded.

if (ENABLE_GTK3)
    set(VCLPLUG_GTK3_SOURCES
        vcl/unx/gtk3/a11y/atkaction
        vcl/unx/gtk3/a11y/atkcomponent
        vcl/unx/gtk3/a11y/atkeditabletext
        vcl/unx/gtk3/a11y/atkfactory
        vcl/unx/gtk3/a11y/atkhypertext
        vcl/unx/gtk3/a11y/atkimage
        vcl/unx/gtk3/a11y/atklistener
        vcl/unx/gtk3/a11y/atkregistry
        vcl/unx/gtk3/a11y/atkselection
        vcl/unx/gtk3/a11y/atktable
        vcl/unx/gtk3/a11y/atktablecell
        vcl/unx/gtk3/a11y/atktextattributes
        vcl/unx/gtk3/a11y/atktext
        vcl/unx/gtk3/a11y/atkvalue
        vcl/unx/gtk3/a11y/atkwrapper
        vcl/unx/gtk3/fpicker/resourceprovider
        vcl/unx/gtk3/fpicker/SalGtkFilePicker
        vcl/unx/gtk3/fpicker/SalGtkFolderPicker
        vcl/unx/gtk3/fpicker/SalGtkPicker
        vcl/unx/gtk3/customcellrenderer
        vcl/unx/gtk3/gtkdata
        vcl/unx/gtk3/gtkinst
        vcl/unx/gtk3/gtksys
        vcl/unx/gtk3/gtkcairo
        vcl/unx/gtk3/custom-theme
        vcl/unx/gtk3/salnativewidgets-gtk
        vcl/unx/gtk3/gtkframe
        vcl/unx/gtk3/gtkobject
        vcl/unx/gtk3/gtksalmenu
        vcl/unx/gtk3/glomenu
        vcl/unx/gtk3/gloactiongroup
        vcl/unx/gtk3/hudawareness)

    librestudio_expand_cxx(VCLPLUG_GTK3_SOURCES)
    add_library(vclplug_gtk3 SHARED ${VCLPLUG_GTK3_SOURCES})

    target_include_directories(vclplug_gtk3 PRIVATE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${CMAKE_SOURCE_DIR}/vcl/unx"
        "${CMAKE_SOURCE_DIR}/vcl/unx/gtk3"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(vclplug_gtk3
        officecfg_headers udkapi_normal_headers offapi_normal_headers)

    # Lines 33-39: the plug-in marker, the guard the toolkit headers carry, and
    # the two switches configure.ac appends to GTK3_CFLAGS.  The
    # -Wno-deprecated-declarations is upstream's own note to itself that
    # vcl/unx/gtk3/*.cxx forwards into deprecated GTK interfaces.
    target_compile_definitions(vclplug_gtk3 PRIVATE
        VCLPLUG_GTK_IMPLEMENTATION
        VCL_INTERNALS
        GDK_DISABLE_DEPRECATED
        GTK_DISABLE_DEPRECATED)
    target_compile_options(vclplug_gtk3 PRIVATE -Wno-deprecated-declarations)

    # configure.ac line 12716.  cairo is named here as well as being linked into
    # libvcl through USE_HEADLESS_CODE; configure.ac asks for it twice for the
    # same reason.
    pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET
        gtk+-3.0>=3.24 gmodule-no-export-2.0 glib-2.0>=2.38 atk>=2.36 cairo)

    target_link_libraries(vclplug_gtk3 PRIVATE
        vcl
        tl utl sot ucbhelper basegfx comphelper cppuhelper i18nlangtag i18nutil
        cppu sal
        Boost::boost
        PkgConfig::GTK3
        PkgConfig::EPOXY PkgConfig::DBUS PkgConfig::GRAPHITE
        ${LIBRESTUDIO_HARFBUZZ_TARGETS}
        PkgConfig::X11 PkgConfig::XEXT
        "${SM_LIBRARY}" "${ICE_LIBRARY}"
        m dl)

    # The GTK3 plug-in does not use EGL itself — configure.ac wants it only for
    # a bundled epoxy, and epoxy here is the system one — so there is nothing to
    # check for here that the epoxy package has not already answered.

    # $(if $(ENABLE_JAVA), jvmaccess) from the library list.
    if (LIBRESTUDIO_ENABLE_JAVA)
        target_link_libraries(vclplug_gtk3 PRIVATE jvmaccess)
    endif ()

    # No component file here either.
    librestudio_set_library_output_name(vclplug_gtk3 vclplug_gtk3 OOO)
    install(TARGETS vclplug_gtk3 LIBRARY DESTINATION program)
endif ()

# --- svptest -------------------------------------------------------------------

# The smallest program that starts VCL: it bootstraps UNO, calls InitVCL(), puts
# up a window, paints gradients and lines into it and runs the event loop until
# the window is closed.  That it runs at all is the end-to-end check for this
# module — the library linking proves much less, because the back end is chosen
# at run time and the toolkit is only initialised when something asks for a
# window.
#
# It builds and links, and without two more things it stops inside InitVCL()
# and exits 1 without a word.  The word is missing because there is none to be
# had: SAL_WARN and SAL_INFO compile to nothing unless SAL_LOG_WARN and
# SAL_LOG_INFO are defined, gbuild defines them for ENABLE_SAL_LOG, and
# configure.ac sets that only for a debug build.  A release build has no SAL
# logging at all, and no runtime SAL_LOG setting can switch on what the
# compiler removed — so the log is not where to look, a debugger is.
#
# The first thing found this way: configmgr cannot resolve
# /org.openoffice.VCL/DefaultFonts — the set is in
# officecfg/registry/schema/org/openoffice/VCL.xcs, the data next to it in
# VCL.xcu — because nothing had turned the officecfg .xcu files into .xcd and
# told configmgr to read them.  officecfg/CMakeLists.txt now builds that
# main.xcd, and officecfg_registry below makes sure it exists before the
# program runs.
#
# The second, found only once the first was fixed: even with main.xcd built
# and unorc's CONFIGURATION_LAYERS pointing at it, the very same
# "cannot find /org.openoffice.Setup/L10N" kept happening — because
# configmgr never even opened the registry directory (confirmed with
# strace -e trace=openat,access: not one call touched share/registry).
# CONFIGURATION_LAYERS is read through rtl::Bootstrap's *default* ini, which
# is resolved in sal/rtl/bootstrap.cxx's getIniFileName() as
# "<the executable's own directory>/<its name without extension>rc" — here
# "svptestrc" — and that is a different file from unorc, which cppuhelper's
# defaultBootstrap_InitialComponentContext() locates on its own (via
# cppuhelper/source/paths.cxx, next to libcppuhelpergcc3.so) and reads
# explicitly, never registering it as the process-wide default.  strace showed
# four ENOENT attempts at "svptestrc" and confirmed it. Every VCL program below
# that bootstraps UNO and touches configuration needs its own "<name>rc" with
# the same content, so librestudio_generate_unorc() is called again for each
# one, further down, right after they are all defined.
#
# Module_vcl.mk declares it among a group of workbench programs, under a guard
# for cross-compiling and iOS/Android; none of those apply here.
add_executable(svptest "${CMAKE_SOURCE_DIR}/vcl/workben/svptest.cxx")

target_include_directories(svptest PRIVATE
    "${CMAKE_SOURCE_DIR}/vcl/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(svptest udkapi_normal_headers offapi_normal_headers gcc3_uno officecfg_registry)

target_link_libraries(svptest PRIVATE
    vcl tl sal comphelper cppu cppuhelper)

install(TARGETS svptest RUNTIME DESTINATION program)

# --- vclmain and the workbench programs ----------------------------------------

# vcl/source/salmain/salmain is the SAL_IMPLEMENT_MAIN() that a program links
# against when it has no main() of its own: it extends the application
# environment, creates the application object and calls SVMain().  A static
# library, because it becomes part of the program rather than something loaded
# at run time — and because ld pulls an archive member in only when something
# asks for a symbol it defines, which here is the main() the C runtime wants.
add_library(vclmain STATIC "${CMAKE_SOURCE_DIR}/vcl/source/salmain/salmain.cxx")
target_include_directories(vclmain PUBLIC
    "${CMAKE_SOURCE_DIR}/vcl/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(vclmain udkapi_normal_headers offapi_normal_headers)

# The rest of the workbench programs Module_vcl.mk declares, each one source.
# They are how the module is exercised by hand, and they are how VCL is debugged
# without an office on top of it.  The library lists are each program's own; the
# three that include a vcl/toolkit header need VCL_INTERNALS, the guard those
# headers carry against being used from outside the module.
#
# Every one of them bootstraps UNO, which means it dlopen()s the bridge rather
# than linking it — hence gcc3_uno in each dependency line.  See the note in
# bridges/CMakeLists.txt.
#
# svptest above is here too, and takes no vclmain — it has a main() of its own.

# comphelper, cppu, cppuhelper, sal, tl, ucbhelper, vcl; and vclmain.
add_executable(icontest "${CMAKE_SOURCE_DIR}/vcl/workben/icontest.cxx")
target_compile_definitions(icontest PRIVATE VCL_INTERNALS)
target_include_directories(icontest PRIVATE
    "${CMAKE_SOURCE_DIR}/vcl/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(icontest udkapi_normal_headers offapi_normal_headers gcc3_uno officecfg_registry)
target_link_libraries(icontest PRIVATE
    comphelper cppu cppuhelper sal tl ucbhelper vcl vclmain)

# basegfx, comphelper, cppu, cppuhelper, tl, sal, salhelper, vcl; and vclmain.
# Its source is not under vcl/workben.
add_executable(visualbackendtest
    "${CMAKE_SOURCE_DIR}/vcl/backendtest/VisualBackendTest.cxx")
target_include_directories(visualbackendtest PRIVATE
    "${CMAKE_SOURCE_DIR}/vcl/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(visualbackendtest udkapi_normal_headers offapi_normal_headers gcc3_uno officecfg_registry)
target_link_libraries(visualbackendtest PRIVATE
    basegfx comphelper cppu cppuhelper tl sal salhelper vcl vclmain)

# tl, sal, utl, vcl, cppu, cppuhelper, comphelper, and harfbuzz — the last
# because it reads vcl/inc/font/TrueTypeFont.hxx, which includes hb.h and
# hb-ot.h.  LIBRESTUDIO_HARFBUZZ_TARGETS rather than the target names, so that
# this keeps working whichever way the top level got harfbuzz.
add_executable(fftester "${CMAKE_SOURCE_DIR}/vcl/workben/fftester.cxx")
target_include_directories(fftester PRIVATE
    "${CMAKE_SOURCE_DIR}/vcl/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(fftester udkapi_normal_headers offapi_normal_headers gcc3_uno officecfg_registry)
target_link_libraries(fftester PRIVATE
    tl sal utl vcl cppu cppuhelper comphelper ${LIBRESTUDIO_HARFBUZZ_TARGETS})

# tl, sal, vcl, cppu, cppuhelper, comphelper, i18nlangtag.  The last is not
# decoration: the program asks MsLangId for the language of the system, and
# without i18nlangtag on the link line that symbol is simply missing.
add_executable(listfonts "${CMAKE_SOURCE_DIR}/vcl/workben/listfonts.cxx")
target_include_directories(listfonts PRIVATE
    "${CMAKE_SOURCE_DIR}/vcl/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(listfonts udkapi_normal_headers offapi_normal_headers gcc3_uno officecfg_registry)
target_link_libraries(listfonts PRIVATE
    tl sal vcl cppu cppuhelper comphelper i18nlangtag)

# The same list as listfonts, plus harfbuzz and graphite — this one is about
# what the shapers make of a font.
add_executable(listglyphs "${CMAKE_SOURCE_DIR}/vcl/workben/listglyphs.cxx")
target_include_directories(listglyphs PRIVATE
    "${CMAKE_SOURCE_DIR}/vcl/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(listglyphs udkapi_normal_headers offapi_normal_headers gcc3_uno officecfg_registry)
target_link_libraries(listglyphs PRIVATE
    tl sal vcl cppu cppuhelper comphelper i18nlangtag
    ${LIBRESTUDIO_HARFBUZZ_TARGETS} PkgConfig::GRAPHITE)

# tl, sal, vcl, cppu, cppuhelper, comphelper; VCL_INTERNALS; and its own main().
add_executable(svpclient "${CMAKE_SOURCE_DIR}/vcl/workben/svpclient.cxx")
target_compile_definitions(svpclient PRIVATE VCL_INTERNALS)
target_include_directories(svpclient PRIVATE
    "${CMAKE_SOURCE_DIR}/vcl/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(svpclient udkapi_normal_headers offapi_normal_headers gcc3_uno officecfg_registry)
target_link_libraries(svpclient PRIVATE
    tl sal vcl cppu cppuhelper comphelper)

install(TARGETS icontest visualbackendtest fftester listfonts listglyphs svpclient
    RUNTIME DESTINATION program)

# Each of these bootstraps UNO and, through it, configmgr — which reads
# CONFIGURATION_LAYERS from rtl::Bootstrap's default ini, not from unorc (see
# the long comment above svptest for why the two are different files).  Give
# each program its own "<name>rc" with the same content as unorc so that
# default ini resolves to something that actually names the registry.
foreach (_vcl_workbench_prog IN ITEMS
    svptest icontest visualbackendtest fftester listfonts listglyphs svpclient)
    librestudio_generate_unorc("${LIBRESTUDIO_PROGRAM_DIR}/${_vcl_workbench_prog}rc")
endforeach ()

# Five more are not built, and each for a reason that is not about vcl:
#
#   vcldemo, svdemo, minvcl, minweld   link fwk, the library of the framework
#                                      module, which is not ported.
#   mtfdemo                            links drawinglayer and emfio as well —
#                                      the first is only here as
#                                      drawinglayercore, the second not at all.
#
# They are one source each, so they are worth coming back to when those modules
# are in.
