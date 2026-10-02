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

# --- the platform tails of Library_vcl.mk (sources) ---------------------------
#
# Library_vcl.mk lines 787-876 keep five OS-specific blocks of their own,
# after everything above. Structural now rather than only described: each is
# a real if() on the same CMAKE_SYSTEM_NAME/WIN32/APPLE tests
# cmake/LibreStudioPlatform.cmake and sal/CMakeLists.txt already use
# elsewhere in this port, so none of the five ever actually fires on this
# Linux build -- correctly inert, not merely unwritten. HAIKU (787-791) adds
# no source at all (just -lbe, with the rest of this platform's link
# libraries and frameworks further below, past add_library(vcl ...)).

# Library_vcl.mk lines 794-803: OS=ANDROID. One exception object; confirmed
# vcl/android/androidinst.cxx exists on disk. Its three link libraries
# (-llog -landroid -llo-bootstrap) are added below, past add_library(vcl
# ...); lo-bootstrap in particular is linked as the real CMake target
# sal/CMakeLists.txt defines under the same Android/Emscripten gate, not as a
# raw -l flag.
if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    list(APPEND VCL_SOURCES vcl/android/androidinst)
endif ()

# Library_vcl.mk lines 806-836: OS=iOS.
#
# cgutils (gb_Library_add_objcxxobjects) is shared with the MACOSX branch
# below -- confirmed vcl/quartz/cgutils.mm exists on disk with no .cxx
# sibling at all (unlike sal's uunxapi/system pair, where the .mm is a thin
# #include-only wrapper around a real .cxx of the same name -- there is
# nothing here for cgutils.mm to wrap). It is still the same technique
# sal/CMakeLists.txt's own uunxapi.mm/system.mm precedent established: CMake
# auto-detects a bare .mm as Objective-C++ on an Apple platform, no
# set_source_files_properties(... LANGUAGE OBJCXX) needed. Appended as an
# absolute path so librestudio_expand_cxx() below -- which appends ".cxx" to
# every relative entry -- leaves it untouched.
#
# The 14 exception objects that follow are plain .cxx, every one confirmed to
# exist on disk.
#
# The last eight entries are NOT a reuse of the USE_HEADLESS_CODE block
# above: Library_vcl.mk's iOS branch adds them unconditionally inside
# `ifeq ($(OS),iOS)`, independent of USE_HEADLESS_CODE -- iOS has no
# freetype/fontconfig (it draws fonts through Core Text instead), so
# USE_HEADLESS_CODE is never true there upstream either, and this is iOS's
# own separate inclusion, not something shared with the Linux path. Of
# vcl_headless_code's ten members (Library_vcl.mk lines 650-664), the .mk
# itself drops five specifically for iOS via
# `$(if $(filter-out iOS,$(OS)), svpbmp svpgdi SvpGraphicsBackend
# CairoCommon BitmapHelper)`, so only the other seven appear below; the
# eighth line is vcl/unx/generic/app/gendata, the one
# vcl_headless_generic_code member the .mk names by hand for iOS.
#
# gb_OBJCXXFLAGS (`-x objective-c++` on iOS, Library_vcl.mk line 808) is not
# reproduced: CMake already treats a bare .mm as Objective-C++ on Apple
# platforms without it, and Library_sal.mk carries this identical iOS-only
# cxxflags line for its own two .mm files (Library_sal.mk:132-136) which
# sal/CMakeLists.txt likewise never ported -- this follows that same,
# already-established gap rather than inventing new per-file compile-option
# plumbing for it here.
if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    list(APPEND VCL_SOURCES
        "${CMAKE_SOURCE_DIR}/vcl/quartz/cgutils.mm"
        vcl/ios/iosinst
        vcl/ios/dummies
        vcl/ios/salios
        vcl/ios/iOSTransferable
        vcl/ios/DataFlavorMapping
        vcl/ios/HtmlFmtFlt
        vcl/quartz/CoreTextFont
        vcl/quartz/CoreTextFontFace
        vcl/quartz/SystemFontList
        vcl/quartz/salbmp
        vcl/quartz/salgdi
        vcl/quartz/salgdicommon
        vcl/quartz/salvd
        vcl/quartz/utils
        vcl/quartz/AquaGraphicsBackend
        vcl/headless/svpframe
        vcl/headless/svpdummies
        vcl/headless/svpinst
        vcl/headless/svpvd
        vcl/unx/generic/app/X11Helper
        vcl/unx/generic/app/gendisp
        vcl/unx/generic/app/geninst
        vcl/unx/generic/app/gendata)
endif ()

# Library_vcl.mk lines 839-851: OS=MACOSX, checked as `APPLE AND NOT
# CMAKE_SYSTEM_NAME STREQUAL "iOS"` -- the same ordering
# sal/CMakeLists.txt's own APPLE block uses, since APPLE is true for iOS too
# and gbuild's `ifeq ($(OS),MACOSX)` is native-macOS-only. cgutils.mm is the
# very same file as iOS's above (confirmed by reading both branches of the
# .mk: the path is identical). The Skia variant is gated behind the same
# "SKIA" IN_LIST LIBRESTUDIO_BUILD_TYPE test this file already uses
# elsewhere (the USE_HEADLESS_CODE-independent vcl/skia/salbmp source in the
# !DISABLE_GUI block above, and vclplug_gen's own vcl/skia/x11/* sources
# further down) -- vcl/skia/quartz/salbmp.mm is confirmed to exist on disk,
# likewise with no .cxx sibling.
if (APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    list(APPEND VCL_SOURCES "${CMAKE_SOURCE_DIR}/vcl/quartz/cgutils.mm")
    if ("SKIA" IN_LIST LIBRESTUDIO_BUILD_TYPE)
        list(APPEND VCL_SOURCES "${CMAKE_SOURCE_DIR}/vcl/skia/quartz/salbmp.mm")
    endif ()
endif ()

# Library_vcl.mk lines 854-876: OS=WNT, checked as WIN32 -- the same test
# sal/CMakeLists.txt's own Windows block uses. Both exception objects
# confirmed to exist on disk (vcl/source/opengl/win/WinDeviceInfo.cxx,
# vcl/win/app/fileregistration.cxx). The system libraries and
# -DDO_USE_TTF_ON_WIN32 define are added below, past add_library(vcl ...).
#
# Deliberately not ported here: vcl/salsrc (really vcl/win/src/salsrc.rc, a
# native Win32 resource -- gb_Library_add_nativeres has no CMake equivalent
# anywhere in this tree yet) and the two use_packages, vcl_opengl_denylist
# and postprocess_images (lines 872-875) -- packaging/resource-compiler
# concerns, out of scope for this pass; left as a smaller, separate
# follow-up rather than attempted here.
if (WIN32)
    list(APPEND VCL_SOURCES
        vcl/source/opengl/win/WinDeviceInfo
        vcl/win/app/fileregistration)
endif ()

librestudio_expand_cxx(VCL_SOURCES)
# vcl/source/filter/jpeg/transupp is the single C object, which is why
# gb_Library_add_cobjects lists it apart from the rest; it is libjpeg's
# transform helper, compiled into the same library.
add_library(vcl SHARED
    ${VCL_SOURCES}
    "${CMAKE_SOURCE_DIR}/vcl/source/filter/jpeg/transupp.c")

if (ENABLE_PCH)
    target_precompile_headers(vcl PRIVATE
        "${CMAKE_SOURCE_DIR}/vcl/inc/pch/precompiled_vcl.hxx")
    target_compile_definitions(vcl PRIVATE PCH_LEVEL=2)
endif ()

# --- externals ----------------------------------------------------------------
#
# The four add_externals blocks of the .mk, in its order, each under the same
# condition as there.  The gbuild names map onto pkg-config modules the way
# RepositoryExternal.mk defines them — gio is gio-2.0, graphite is graphite2 —
# except for jpeg and tiff, which come from CMake's own find modules. png and
# graphite2 no longer do (system-or-vendor refactor): both now come from
# librestudio_find_package()/librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake), resolved right below into LIBRESTUDIO_PNG_
# TARGET/LIBRESTUDIO_GRAPHITE2_TARGET -- vcl is their real consumer (along
# with sdext/desktop/writerperfect for PNG) -- tiff stays plain system
# find_package() for now, not yet ported the same way. jpeg (2026-09-29):
# now goes through librestudio_find_package() too -- see external/
# libjpeg-turbo/libjpeg-turbo.cmake's own header comment (NASM/SIMD is no
# longer a reason to defer it; sdext/CMakeLists.txt and external/poppler/
# poppler.cmake's own internal JPEG need were converted the same way).

# Library_vcl.mk lines 92-108: no condition around any of these.
find_package(TIFF REQUIRED)
librestudio_find_package(NAME JPEG DRIVER libjpeg-turbo
    FORCE_VENDOR_VAR LIBRESTUDIO_FORCE_VENDOR_LIBJPEG_TURBO)

# PACKAGE not given: defaults to NAME itself (PNG), the real CMake
# Find-module name. DRIVER auto-detection tries external/png/ (does not
# exist) then external/libpng/ (exists) -- no explicit DRIVER needed.
librestudio_find_package(NAME PNG)

# graphite2 must be resolved BEFORE harfbuzz below: harfbuzz's own meson
# build does not link graphite2 as a CMake target (it finds it via
# PKG_CONFIG_PATH internally, see external/harfbuzz/harfbuzz.cmake's own
# CONFIGURE_COMMAND), so calling librestudio_pkgconfig(NAME HARFBUZZ ...)
# alone would never trigger graphite2's own resolution (and, if vendored,
# its .pc write) on its own -- the one place in this whole system-or-vendor
# router where call order between two resolutions is still semantically
# required. DRIVER explicit ("graphite"): MODULES graphite2 does not match
# external/graphite2/ or external/libgraphite2/ (the real directory is
# external/graphite/, a deliberate rename from this port's own earlier
# LIBRESTUDIO_GRAPHITE_TARGET -- a port-invented identifier, free to rename).
librestudio_pkgconfig(
    NAME GRAPHITE2
    MODULES graphite2
    DRIVER graphite)

# harfbuzz has to be 14.3 or newer and the system is older, so this bundles
# it and hands the result over in LIBRESTUDIO_HARFBUZZ_TARGET. DRIVER
# explicit ("harfbuzz"): MODULES has 2 elements (harfbuzz+harfbuzz-subset),
# so no auto-detection candidate exists at all (even though harfbuzz's own
# real directory happens to equal TOLOWER(NAME) -- deliberately not relied
# on, same reasoning as ORCUS/POPPLER/COINMP below).
find_program(MESON_EXECUTABLE meson REQUIRED)
librestudio_pkgconfig(
    NAME HARFBUZZ
    MODULES harfbuzz harfbuzz-subset
    VERSION 14.3.0
    DRIVER harfbuzz)

pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)

# lcms2/libeot/libwebp (2026-09-29, system-or-vendor refactor): resolved
# via librestudio_pkgconfig() into LIBRESTUDIO_LCMS2_TARGET/LIBRESTUDIO_
# LIBEOT_TARGET/LIBRESTUDIO_LIBWEBP_TARGET -- real vendoring fallbacks in
# external/lcms2/lcms2.cmake, external/libeot/libeot.cmake, external/
# libwebp/libwebp.cmake (the last sets a 2-element target list: webp +
# sharpyuv, see that driver's own header comment). VERSION 0.01 for
# libeot matches configure.ac's own `libeot >= 0.01`; lcms2/libwebp have
# no version floor in configure.ac (grepped: plain `libo_CHECK_SYSTEM_
# MODULE`/no PKG_CHECK_MODULES version constraint for either).
librestudio_pkgconfig(NAME LCMS2 MODULES lcms2)
librestudio_pkgconfig(NAME LIBEOT MODULES libeot VERSION 0.01)
librestudio_pkgconfig(NAME LIBWEBP MODULES libwebp)
# libjxl is not optional and not guarded by an #if anywhere: the reader in
# vcl/source/filter/jxl/reader.cxx includes it unconditionally.  It comes in two
# halves, and the second is not optional either — JxlResizableParallelRunner and
# its friends, which the reader decodes across threads with, are in
# libjxl_threads rather than in libjxl.
#
# 2026-09-29, system-or-vendor refactor: ONE librestudio_pkgconfig() call,
# not two -- configure.ac's own libo_CHECK_SYSTEM_MODULE([libjxl],[LIBJXL],
# [libjxl libjxl_threads]) probes both pkg-config modules together into a
# single LIBJXL_CFLAGS/LIBJXL_LIBS pair, and Library_vcl.mk's own
# gb_Library_use_externals list has a single "libjxl" entry -- there never
# was a separate "libjxl_threads" external upstream (grepped
# RepositoryExternal.mk: no gb_LinkTarget__use_jxl_threads exists at all).
# The vendoring fallback (external/libjxl/libjxl.cmake) compiles both
# halves into one static archive too, matching StaticLibrary_libjxl.mk's
# own single target. VERSION is not given: configure.ac has no version
# floor for this pair either.
librestudio_pkgconfig(NAME LIBJXL MODULES libjxl libjxl_threads
    DRIVER libjxl
    FORCE_VENDOR_VAR LIBRESTUDIO_FORCE_VENDOR_LIBJXL)

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
# headless build and is written out in the WIN32 block further below,
# alongside the rest of that platform's tail; -DSYSTEM_LIBFIXMATH goes to a
# system libfixmath rather than the fix16 bundled in the tree, which is what
# is used unless the option above asks otherwise.
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
    ${LIBRESTUDIO_JPEG_TARGET} ${LIBRESTUDIO_PNG_TARGET} TIFF::TIFF
    ${LIBRESTUDIO_HARFBUZZ_TARGET}
    PkgConfig::GIO ${LIBRESTUDIO_GRAPHITE2_TARGET} ${LIBRESTUDIO_LCMS2_TARGET}
    ${LIBRESTUDIO_LIBEOT_TARGET} ${LIBRESTUDIO_LIBWEBP_TARGET} ${LIBRESTUDIO_LIBJXL_TARGET}
    m)

if (LIBRESTUDIO_ENABLE_BREAKPAD)
    target_link_libraries(vcl PRIVATE crashreport)
endif ()

if (LIBRESTUDIO_ENABLE_JAVA)
    target_link_libraries(vcl PRIVATE jvmaccess)
endif ()

# Fix (first real `cmake --build`, 2026-09-27): upstream's own gate here is
# "$(if $(filter OPENCL,$(BUILD_TYPE)),opencl)" -- a literal BUILD_TYPE
# membership test, because upstream's BUILD_TYPE is the single source of
# truth that ALSO drives HAVE_FEATURE_OPENCL (config_features.h). This port
# split that single upstream knob into two independent ones: ENABLE_OPENCL
# (cmake/LibreStudioConfigHeaders.cmake, defaults ON, drives
# HAVE_FEATURE_OPENCL) and LIBRESTUDIO_BUILD_TYPE (defaults to plain
# "DESKTOP", no "OPENCL" entry) -- so with defaults, vcl/source/app/svmain.cxx
# compiled in its "#if HAVE_FEATURE_OPENCL" call to OpenCLZone::hardDisable()
# (opencl/source/OpenCLZone.cxx), but this IN_LIST check never linked the
# opencl library that defines it: "undefined reference" at link time. sc's
# own opencl-gated sources (sc/CMakeLists.txt, "if (ENABLE_OPENCL)") already
# established ENABLE_OPENCL as this port's real equivalent of upstream's
# BUILD_TYPE-for-OpenCL-purposes gate -- vcl's own check is changed to match
# it here, rather than trying to keep two independent flags in sync (the same
# class of gap already found and fixed for HAVE_FEATURE_DBCONNECTIVITY/
# AVAHI/AVMEDIA/XMLHELP in earlier waves). add_subdirectory(opencl) itself is
# unconditional (CMakeLists.txt), so the opencl target always exists to link
# against once ENABLE_OPENCL is on, same as before.
if (ENABLE_OPENCL)
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

# --- the platform tails of Library_vcl.mk (libraries, frameworks, defines) ----
#
# The other half of the same five blocks (Library_vcl.mk lines 787-876) whose
# sources were appended to VCL_SOURCES above, before add_library(vcl ...).
# Each if() here tests the same CMAKE_SYSTEM_NAME/WIN32/APPLE condition its
# source-list counterpart above used, and, like those, is structural rather
# than reachable: cmake/LibreStudioPlatform.cmake's own platform tests decide
# which of these ever fires, and on this Linux build none of them do -- the
# same "written, never taken" standing sal/CMakeLists.txt's own Haiku/
# Android/APPLE/WIN32 blocks already have. The Linux-active code above this
# point (everything through librestudio_add_component() and the install()
# call) is unchanged by any of what follows.

# Library_vcl.mk lines 787-791: OS=HAIKU. "be" is the BeOS/Haiku application
# kit; nothing else in this tree links it yet, so there is no prior
# precedent to match beyond the plain-name style sal/CMakeLists.txt already
# uses for its own single-word platform libraries (network, log, android).
if (CMAKE_SYSTEM_NAME STREQUAL "Haiku")
    target_link_libraries(vcl PRIVATE be)
endif ()

# Library_vcl.mk lines 794-803: OS=ANDROID. lo-bootstrap is linked as the
# real CMake target sal/CMakeLists.txt defines
# (`if (CMAKE_SYSTEM_NAME STREQUAL "Android" OR ... "Emscripten")
# add_library(lo-bootstrap ...)`), not as a raw -llo-bootstrap flag; log and
# android are plain NDK system libraries, the same two names
# sal/CMakeLists.txt's own Android block links.
if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(vcl PRIVATE log android lo-bootstrap)
endif ()

# Library_vcl.mk lines 832-835 (part of the iOS branch) and 846-850 (the
# MACOSX branch): gb_Library_use_system_darwin_frameworks. No macOS/iOS SDK
# on this machine to confirm either actually links -- both use the same
# "-framework <Name>" idiom sal/CMakeLists.txt's own APPLE block already
# established as this port's precedent for a Darwin framework link (that
# comment notes no other precedent was found anywhere in the tree; this
# reuses the same one rather than inventing a second).
if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_link_libraries(vcl PRIVATE
        "-framework UIKit"
        "-framework CoreFoundation")
endif ()

if (APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_link_libraries(vcl PRIVATE
        "-framework Cocoa"
        "-framework CoreFoundation"
        "-framework Metal")
endif ()

# Library_vcl.mk lines 854-876: OS=WNT. dwrite/ole32/setupapi/shell32/version
# are use_system_win32_libs, linked by plain name the same way
# sal/CMakeLists.txt's own WIN32 block links advapi32/comdlg32/dbghelp/etc --
# no "-l" prefix, no .lib suffix, both left for the linker to resolve.
if (WIN32)
    target_link_libraries(vcl PRIVATE
        dwrite ole32 setupapi shell32 version)

    # Library_vcl.mk line 51: $(if $(filter WNT-TRUE,$(OS)-$(USE_HEADLESS_CODE)),
    # -DDO_USE_TTF_ON_WIN32) tests both halves of that filter, not OS=WNT
    # alone.
    if (USE_HEADLESS_CODE)
        target_compile_definitions(vcl PRIVATE DO_USE_TTF_ON_WIN32)
    endif ()
endif ()

# --- what is not here ---------------------------------------------------------
#
# vcl/salsrc (vcl/win/src/salsrc.rc, a native Win32 resource) and the two
# use_packages the WNT block also carries, vcl_opengl_denylist and
# postprocess_images (Library_vcl.mk lines 868, 872-875) -- packaging and
# resource-compiler concerns, deliberately left out of the WIN32 block above
# as a smaller, separate follow-up rather than attempted here.
#
# Everything else the .mk switches on is expressed above as a condition
# rather than described, so it is not repeated here.

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
    # the Skia back end does its own font handling. graphite2 no longer
    # gets its own independent probe here (2026-09-28) -- same module as
    # the plain GRAPHITE one above, so the same LIBRESTUDIO_GRAPHITE2_TARGET
    # is reused directly below instead of a second, redundant
    # pkg_check_modules(...graphite2) call.
    pkg_check_modules(CAIRO_GEN REQUIRED IMPORTED_TARGET cairo)
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
        ${LIBRESTUDIO_HARFBUZZ_TARGET}
        PkgConfig::CAIRO_GEN ${LIBRESTUDIO_GRAPHITE2_TARGET} PkgConfig::EPOXY_GEN
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
        PkgConfig::EPOXY PkgConfig::DBUS ${LIBRESTUDIO_GRAPHITE2_TARGET}
        ${LIBRESTUDIO_HARFBUZZ_TARGET}
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

# --- vclplug_qt6: moc code generation -------------------------------------------

# Foundational piece for the (future) vclplug_qt6 back end: Qt6 detection and the
# moc (meta-object compiler) code-generation step it needs. This is genuinely new
# ground for this port -- every external dependency built so far (EPOXY,
# GRAPHITE, GTK3, ...) is a plain pkg-config library with nothing generated from
# it, and the one place this port already generates code, the UNO IDL toolchain
# (cmake/LibreStudioUnoApi.cmake), runs tools this build produces itself
# (cppumaker, unoidl-write), not one found on the host. moc is the first case of
# both at once: a tool found on the host, invoked once per header, to produce a
# compiled source moc needs. vclplug_qt6's 90 sources are a separate, later task;
# this block only builds the ground under it far enough to verify moc runs
# correctly in isolation, which is why there is no vclplug_qt6 library target
# here yet -- vclplug_qt6_moc below is the entire deliverable.
#
# Header shim: vcl/inc/qt6/*.hxx (e.g. vcl/inc/qt6/QtFrame.hxx) are one-line
# `#include "../qt5/QtFrame.hxx"` forwarders, nothing more -- verified by reading
# a handful of them. moc is run against vcl/inc/qt5/*.hxx instead, per
# vcl/CustomTarget_qt6_moc.mk's pattern rule and its own comment on why:
#
#   $(gb_CustomTarget_workdir)/vcl/qt6/%.moc : $(SRCDIR)/vcl/inc/qt5/%.hxx | ...
#       $(MOC6) $< -o $@
#
#   "For now, the headers in vcl/inc/qt6 just '#include' the ones in vcl/inc/qt5.
#   Since moc does not process classes from the included headers, it needs to be
#   run on the headers in the qt5 dir. That will have to be adapted in case the
#   qt6 VCL plugin uses "own" headers."
#
# ENABLE_QT6 defaults off: configure.ac's AC_ARG_ENABLE(qt6, ...) (line 1972) has
# no default-on path of its own -- only `enable_kf6=yes` forces it on (line
# 6698-6701), and this port has no KF6 support -- unlike gtk3/gen, which this
# build enables by default on Linux/X11. It is declared with option() here
# rather than the `set(... CACHE BOOL ...)` some other ENABLE_* switches use in
# cmake/LibreStudioConfigHeaders.cmake: those are centralised there because more
# than one module reads them (ENABLE_GTK3 alone is read by vcl/CMakeLists.txt,
# libreofficekit/CMakeLists.txt and config_vclplug.h.in's runtime
# back-end-autodetection macro); ENABLE_QT6 has no other reader yet -- there is
# nothing to centralise until vclplug_qt6 itself exists, so it stays local to the
# one place that uses it, matching this task's isolated, purely-additive scope.
# (config_vclplug.h.in already carries a hardcoded `#define ENABLE_QT6 0` of its
# own, for salplug.cxx's runtime back-end list -- deliberately left untouched
# here: it governs loading a plugin that this task does not build.)
option(ENABLE_QT6 "Build the Qt6 VCL plugin backend (vclplug_qt6)" OFF)

if (ENABLE_QT6)
    # configure.ac does not use pkg-config for the Qt6 core modules itself -- it
    # is qmake-driven (AC_PATH_PROGS(QMAKE6, [qmake-qt6 qmake6 qmake]), then
    # `$QMAKE6 -query QT_INSTALL_HEADERS/LIBS`, configure.ac lines 14210-14226) --
    # but every other external dependency in this port is found via pkg-config,
    # and Qt6Core/Qt6Gui/Qt6Widgets are real pkg-config modules on this system
    # (`pkg-config --list-all | grep -i qt6`), matching configure.ac's own link
    # check (`qt6_test_library="Qt6Widgets"`, line 14262) and its
    # `QT6_LIBS="... -lQt6Core -lQt6Gui -lQt6Widgets"` Linux branch (line 14319).
    # This port's own convention is followed here rather than configure.ac's
    # qmake mechanism.
    pkg_check_modules(QT6 REQUIRED IMPORTED_TARGET Qt6Core Qt6Gui Qt6Widgets)

    # Meta Object Compiler lookup. configure.ac (lines 14357-14369) builds
    # moc6_search_path by hand from every Qt6 libdir's libexec/ sibling, then the
    # plain libdirs, then $PATH, and calls
    #   AC_PATH_PROGS(MOC6, [moc-qt6 moc], no, [$moc6_search_path])
    # pkg-config already exposes that same libexec directory directly as
    # Qt6Core's own "libexecdir" variable (`pkg-config --variable=libexecdir
    # Qt6Core` -> /usr/lib64/qt6/libexec on this machine, which is exactly where
    # moc lives here), so that is used instead of reconstructing
    # dirname($libdir)/libexec by hand; Qt6Core's libdir and bindir are added the
    # same way configure.ac falls back to the plain libdirs, and find_program()
    # already searches $PATH itself, so nothing else needs to be added for that
    # half of moc6_search_path.
    pkg_get_variable(QT6_CORE_LIBEXECDIR Qt6Core libexecdir)
    pkg_get_variable(QT6_CORE_LIBDIR Qt6Core libdir)
    pkg_get_variable(QT6_CORE_BINDIR Qt6Core bindir)

    # This port's established style for a required external tool lookup --
    # find_program(<VAR> <name> REQUIRED), which fails configure outright with
    # CMake's own clear error message -- is XSLTPROC_EXECUTABLE (root
    # CMakeLists.txt, officecfg/CMakeLists.txt) and ZIP_EXECUTABLE
    # (cmake/LibreStudioZip.cmake); mirrored here rather than a hand-written
    # message(FATAL_ERROR ...). configure.ac looks for moc-qt6 before the bare
    # moc name (AC_PATH_PROGS(MOC6, [moc-qt6${suffix} moc${suffix}], ...)); the
    # same order is used below.
    find_program(QT6_MOC NAMES moc-qt6 moc
        PATHS "${QT6_CORE_LIBEXECDIR}" "${QT6_CORE_LIBDIR}" "${QT6_CORE_BINDIR}"
        REQUIRED)

    # The 64 vcl/inc/qt5/*.hxx headers that declare Q_OBJECT -- this is
    # vcl/CustomTarget_qt6_moc.mk's own output list (its QtSvpSalFrame /
    # QtSvpSalInstance pair is gated there on USE_HEADLESS_CODE, which this
    # build has on, so both are unconditional here). vcl/Library_vclplug_qt6.mk,
    # the 90-source library target, is out of scope for this task and is not
    # read for this list.
    set(VCLPLUG_QT6_MOC_HEADERS
        QtAccessibleWidget QtClipboard QtDoubleSpinBox QtDragAndDrop QtExpander
        QtFilePicker QtFrame QtHyperlinkLabel QtInstance QtInstanceAssistant
        QtInstanceBox QtInstanceCalendar QtInstanceCheckButton
        QtInstanceColorChooserDialog QtInstanceComboBox QtInstanceContainer
        QtInstanceDialog QtInstanceDrawingArea QtInstanceEntry
        QtInstanceEntryTreeView QtInstanceExpander
        QtInstanceFormattedSpinButton QtInstanceFrame QtInstanceGrid
        QtInstanceIconView QtInstanceImage QtInstanceItemView QtInstanceLabel
        QtInstanceLevelBar QtInstanceLinkButton QtInstanceMDI QtInstanceMenu
        QtInstanceMenuButton QtInstanceMessageDialog QtInstanceNotebook
        QtInstancePaned QtInstancePopover QtInstanceProgressBar
        QtInstanceRadioButton QtInstanceScale QtInstanceScrollbar
        QtInstanceScrolledWindow QtInstanceSizeGroup QtInstanceSpinButton
        QtInstanceSpinner QtInstanceTextView QtInstanceTextWidget
        QtInstanceToggleButton QtInstanceToolbar QtInstanceTreeView
        QtInstanceWidget QtInstanceWindow QtMainWindow QtMenu QtObject
        QtSalFrame QtSalInstance QtSvpSalFrame QtSvpSalInstance QtTimer
        QtTransferable QtTreeViewItemDelegate QtTreeViewModel QtWidget
        QtXAccessible)

    set(VCLPLUG_QT6_MOC_DIR "${CMAKE_CURRENT_BINARY_DIR}/qt6-moc")
    file(MAKE_DIRECTORY "${VCLPLUG_QT6_MOC_DIR}")

    set(VCLPLUG_QT6_MOC_OUTPUTS)
    foreach (_qt6_moc_name IN LISTS VCLPLUG_QT6_MOC_HEADERS)
        set(_qt6_moc_header "${CMAKE_SOURCE_DIR}/vcl/inc/qt5/${_qt6_moc_name}.hxx")
        set(_qt6_moc_output "${VCLPLUG_QT6_MOC_DIR}/${_qt6_moc_name}.moc")
        add_custom_command(
            OUTPUT "${_qt6_moc_output}"
            COMMAND "${QT6_MOC}" "${_qt6_moc_header}" -o "${_qt6_moc_output}"
            DEPENDS "${_qt6_moc_header}"
            COMMENT "MOC ${_qt6_moc_name}"
            VERBATIM)
        list(APPEND VCLPLUG_QT6_MOC_OUTPUTS "${_qt6_moc_output}")
    endforeach ()

    # The one deliverable of the first task on this: a working, isolated
    # custom target that produces all 64 .moc files. (It no longer stands
    # alone: the vclplug_qt6 library below now depends on it.)
    add_custom_target(vclplug_qt6_moc DEPENDS ${VCLPLUG_QT6_MOC_OUTPUTS})
endif ()

# --- vclplug_qt6: library --------------------------------------------------------

# The vclplug_qt6 library itself, on top of the Qt6 detection and moc code
# generation in the ENABLE_QT6 block above. Same shape as vclplug_gen and
# vclplug_gtk3: a plug-in, dlopen()ed by name at run time, with vcl on its own
# link line because gb_Library_set_plugin_for(vclplug_qt6,vcl)
# (Library_vclplug_qt6.mk line 22) is the non-_nodep variant, exactly like the
# other two back ends above.
#
# Kept as its own if (ENABLE_QT6) block rather than folded into the moc block
# above, so that block's "moc runs correctly in isolation" deliverable stays
# intact and separately reviewable; ENABLE_QT6 still defaults OFF, so the two
# blocks together remain a single no-op switch.

if (ENABLE_QT6)
    # Library_vclplug_qt6.mk lines 79-174: the 90 .cxx sources. The two
    # conditional tails mirror vcl/CustomTarget_qt6_moc.mk's own .moc list
    # (USE_HEADLESS_CODE for the Svp* quartet) and the plain .mk's own
    # $(if $(USING_X11), vcl/qt6/QtX11Support). Both switches are TRUE in this
    # build (cmake/LibreStudioConfigHeaders.cmake), so all 90 sources end up
    # wired, but both conditions are written out for correctness on a build
    # where either one is off.
    set(VCLPLUG_QT6_SOURCES
        vcl/qt6/QtAccessibleEventListener
        vcl/qt6/QtAccessibleInterimChildWidget
        vcl/qt6/QtAccessibleInterimParentWidget
        vcl/qt6/QtAccessibleRegistry
        vcl/qt6/QtAccessibleWidget
        vcl/qt6/QtBitmap
        vcl/qt6/QtBuilder
        vcl/qt6/QtClipboard
        vcl/qt6/QtCustomStyle
        vcl/qt6/QtDoubleSpinBox
        vcl/qt6/QtDragAndDrop
        vcl/qt6/QtExpander
        vcl/qt6/QtFilePicker
        vcl/qt6/QtFont
        vcl/qt6/QtFontFace
        vcl/qt6/QtFrame
        vcl/qt6/QtGraphics
        vcl/qt6/QtGraphicsBase
        vcl/qt6/QtGraphics_Controls
        vcl/qt6/QtGraphics_GDI
        vcl/qt6/QtHyperlinkLabel
        vcl/qt6/QtInstance
        vcl/qt6/QtInstanceAssistant
        vcl/qt6/QtInstanceBox
        vcl/qt6/QtInstanceBuilder
        vcl/qt6/QtInstanceButton
        vcl/qt6/QtInstanceCalendar
        vcl/qt6/QtInstanceCheckButton
        vcl/qt6/QtInstanceColorChooserDialog
        vcl/qt6/QtInstanceComboBox
        vcl/qt6/QtInstanceContainer
        vcl/qt6/QtInstanceDialog
        vcl/qt6/QtInstanceDrawingArea
        vcl/qt6/QtInstanceEntry
        vcl/qt6/QtInstanceEntryTreeView
        vcl/qt6/QtInstanceExpander
        vcl/qt6/QtInstanceFormattedSpinButton
        vcl/qt6/QtInstanceFrame
        vcl/qt6/QtInstanceGrid
        vcl/qt6/QtInstanceIconView
        vcl/qt6/QtInstanceImage
        vcl/qt6/QtInstanceItemView
        vcl/qt6/QtInstanceLabel
        vcl/qt6/QtInstanceLevelBar
        vcl/qt6/QtInstanceLinkButton
        vcl/qt6/QtInstanceMDI
        vcl/qt6/QtInstanceMenu
        vcl/qt6/QtInstanceMenuButton
        vcl/qt6/QtInstanceMessageDialog
        vcl/qt6/QtInstanceNotebook
        vcl/qt6/QtInstancePaned
        vcl/qt6/QtInstancePopover
        vcl/qt6/QtInstanceProgressBar
        vcl/qt6/QtInstanceRadioButton
        vcl/qt6/QtInstanceScale
        vcl/qt6/QtInstanceScrollbar
        vcl/qt6/QtInstanceScrolledWindow
        vcl/qt6/QtInstanceSizeGroup
        vcl/qt6/QtInstanceSpinButton
        vcl/qt6/QtInstanceSpinner
        vcl/qt6/QtInstanceTextView
        vcl/qt6/QtInstanceTextWidget
        vcl/qt6/QtInstanceToggleButton
        vcl/qt6/QtInstanceToolbar
        vcl/qt6/QtInstanceTreeIter
        vcl/qt6/QtInstanceTreeView
        vcl/qt6/QtInstanceWidget
        vcl/qt6/QtInstanceWindow
        vcl/qt6/QtMainWindow
        vcl/qt6/QtMenu
        vcl/qt6/QtObject
        vcl/qt6/QtOpenGLContext
        vcl/qt6/QtPainter
        vcl/qt6/QtSalFrame
        vcl/qt6/QtSalInstance
        vcl/qt6/QtSalInstanceBuilderWrapper
        vcl/qt6/QtSystem
        vcl/qt6/QtTimer
        vcl/qt6/QtTools
        vcl/qt6/QtTransferable
        vcl/qt6/QtTreeViewItemDelegate
        vcl/qt6/QtTreeViewModel
        vcl/qt6/QtVirtualDevice
        vcl/qt6/QtWidget
        vcl/qt6/QtXAccessible
        vcl/qt6/QtXWindow)

    if (USE_HEADLESS_CODE)
        list(APPEND VCLPLUG_QT6_SOURCES
            vcl/qt6/QtSvpGraphics
            vcl/qt6/QtSvpSalFrame
            vcl/qt6/QtSvpSalInstance
            vcl/qt6/QtSvpSurface)
    endif ()

    if (USING_X11)
        list(APPEND VCLPLUG_QT6_SOURCES
            vcl/qt6/QtX11Support)
    endif ()

    librestudio_expand_cxx(VCLPLUG_QT6_SOURCES)
    add_library(vclplug_qt6 SHARED ${VCLPLUG_QT6_SOURCES})

    # gb_Library_use_custom_headers(vclplug_qt6,vcl/qt6) forwards the moc
    # CustomTarget's own output dir onto the library so a .cxx that says
    # #include "X.moc" (Qt's own convention for a translation unit defining a
    # Q_OBJECT class) finds it; VCLPLUG_QT6_MOC_DIR/vclplug_qt6_moc are wave
    # 1's variable/target for exactly that directory/step.
    add_dependencies(vclplug_qt6
        vclplug_qt6_moc officecfg_headers udkapi_normal_headers
        offapi_normal_headers)
    target_include_directories(vclplug_qt6 PRIVATE
        "${CMAKE_SOURCE_DIR}/vcl/inc"
        "${CMAKE_SOURCE_DIR}/vcl/inc/qt6"
        "${VCLPLUG_QT6_MOC_DIR}"
        "${FROZEN_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}")

    # Library_vclplug_qt6.mk lines 32-35.
    target_compile_definitions(vclplug_qt6 PRIVATE
        VCLPLUG_QT_IMPLEMENTATION
        VCL_INTERNALS)

    target_link_libraries(vclplug_qt6 PRIVATE
        vcl
        tl utl sot ucbhelper basegfx comphelper cppuhelper i18nlangtag i18nutil
        cppu sal salhelper xmlreader
        Boost::boost
        ICU::uc
        ${LIBRESTUDIO_HARFBUZZ_TARGET}
        PkgConfig::EPOXY ${LIBRESTUDIO_GRAPHITE2_TARGET}
        PkgConfig::QT6
        m dl)

    # $(if $(filter TRUE,$(ENABLE_CAIRO_CANVAS) $(USE_HEADLESS_CODE)),cairo)
    # from the externals list (lines 58-68). Both switches are TRUE in this
    # build, same as vcl's own USE_HEADLESS_CODE cairo block above (line 884),
    # but this runs its own pkg_check_modules call under its own condition --
    # mirroring vclplug_gen's own separate CAIRO_GEN precedent above -- rather
    # than assuming vcl's PkgConfig::CAIRO (created only under USE_HEADLESS_CODE
    # there, not ENABLE_CAIRO_CANVAS) is already available by the time this
    # block runs.
    if (ENABLE_CAIRO_CANVAS OR USE_HEADLESS_CODE)
        pkg_check_modules(CAIRO_QT6 REQUIRED IMPORTED_TARGET cairo)
        target_link_libraries(vclplug_qt6 PRIVATE PkgConfig::CAIRO_QT6)
    endif ()

    # $(if $(ENABLE_JAVA), jvmaccess) from the library list.
    if (LIBRESTUDIO_ENABLE_JAVA)
        target_link_libraries(vclplug_qt6 PRIVATE jvmaccess)
    endif ()

    # QT6_HAVE_GOBJECT (lines 70-77) is a configure.ac-detected sub-feature --
    # whether the Qt6 platform plugin on the host was itself built with
    # GObject introspection support, which unlocks an extra cxxflags/libs pair
    # here. This port has no equivalent detection (no configure.ac run, and
    # pkg-config's Qt6Core/Gui/Widgets modules carry no such variable): left as
    # an honest, explicit gap rather than invented, so that extra
    # cxxflags/libs pair is simply never added.

    # vclplug_win (Windows) is this library's other platform-conditional
    # dependency in the plain use_libraries list ($(if $(filter
    # WNT,$(OS)),vclplug_win)); irrelevant on this Linux build and not added.
    # -lm -ldl (lines 197-202, ifeq ($(OS),LINUX)) are written as the plain
    # "m dl" already on the link line above, matching vclplug_gen's and
    # vclplug_gtk3's own literal "m dl" in this same file rather than
    # ${CMAKE_DL_LIBS} (that portable spelling is this port's convention in
    # sal/CMakeLists.txt, bridges/CMakeLists.txt, pyuno/CMakeLists.txt and
    # external/clew/CMakeLists.txt, but not here).

    # macOS block (Library_vclplug_qt6.mk lines 176-195), added structurally
    # per this port's policy of not silently dropping a platform-only branch --
    # best-effort and unverified, like every other Apple-framework precedent
    # already in this file and in sal/CMakeLists.txt, since there is no Mac on
    # this machine to build or check it against.
    if (APPLE)
        target_link_libraries(vclplug_qt6 PRIVATE
            "-framework ApplicationServices"
            "-framework Cocoa"
            "-framework Carbon"
            "-framework CoreFoundation"
            vclplug_osx)
        # gb_Library_darwin_add_rpath(vclplug_qt6,"$(QT6_LIBDIR)") and
        # gb_OBJCXXFLAGS (the Objective-C++ flags added at lines 184-186) have
        # no established CMake analog anywhere in this port (QT6_LIBDIR is a
        # gbuild/configure.ac variable, and a grep for gb_OBJCXXFLAGS's
        # equivalent here turns up nothing) -- left out rather than guessed
        # at, an explicit gap rather than a silent omission.
    endif ()

    # No component file: a plug-in is named by SAL_USE_VCLPLUGIN, not
    # registered as a UNO service -- same as vclplug_gen/vclplug_gtk3 above.
    librestudio_set_library_output_name(vclplug_qt6 vclplug_qt6 OOO)
    install(TARGETS vclplug_qt6 LIBRARY DESTINATION program)
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
# hb-ot.h.  LIBRESTUDIO_HARFBUZZ_TARGET rather than the target names, so that
# this keeps working whichever way the top level got harfbuzz.
add_executable(fftester "${CMAKE_SOURCE_DIR}/vcl/workben/fftester.cxx")
target_include_directories(fftester PRIVATE
    "${CMAKE_SOURCE_DIR}/vcl/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(fftester udkapi_normal_headers offapi_normal_headers gcc3_uno officecfg_registry)
target_link_libraries(fftester PRIVATE
    tl sal utl vcl cppu cppuhelper comphelper ${LIBRESTUDIO_HARFBUZZ_TARGET})

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
    ${LIBRESTUDIO_HARFBUZZ_TARGET} ${LIBRESTUDIO_GRAPHITE2_TARGET})

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

# --- UIConfig_vcl ---------------------------------------------------------------
#
# UIConfig_vcl.mk: 19 .ui files under vcl/uiconfig/ui packaged into
# share/config/soffice.cfg/vcl/ui -- same gb_UIConfig_UIConfig destination
# convention as UIConfig_svt's own block in svtools/CMakeLists.txt (that
# block's own comment has the gb_UIConfig_DIR_FOR derivation and the
# LIBO_SHARE_FOLDER="share" precedent; not repeated here), followed directly:
# install(FILES ...) naming every .ui file, DESTINATION swapped from
# .../svtools/ui to .../vcl/ui.
#
# VCL_UICONFIG_UI_FILES holds the list once so both the packaging install()
# below and the real build-time mirror right after it stay in sync by
# construction -- no prior variable existed here to reuse (this block used to
# inline the list straight into install(FILES ...)); introduced now as the
# minimal refactor needed to give both mechanisms one shared source of truth,
# the same relationship program_services_rdb's DEPENDS list has to
# librestudio_finalize_components()'s own _components property.
set(VCL_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/aboutbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/combobox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/cupspassworddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dockingwindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/editmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/errornocontentdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/errornoprinterdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/interimdockparent.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/interimtearableparent.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/printdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/printerdevicepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/printerpaperpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/printerpropertiesdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/printprogressdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querydialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/screenshotmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/screenshotparent.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/wizard.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/openlockedquerybox.ui"
)

install(FILES ${VCL_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/vcl/ui)

# Real build-time mirror of the install() rule above, alongside it (not
# instead of it -- install() still does the real distribution packaging at
# `cmake --install` time). Needed because a plain `cmake --build` never runs
# install(), so nothing previously put these .ui files where a CppunitTest
# running straight out of the build tree (LIBRESTUDIO_SHARE_DIR, the same
# instdir/share build-tree root program_services_rdb/officecfg_registry
# already populate for services.rdb/main.xcd) could find them at run time --
# see cmake/LibreStudioTest.cmake's UICONFIG parameter, which depends on the
# vcl_uiconfig target defined below.
#
# One add_custom_command(OUTPUT ...) per file, each with its own
# make_directory before the copy -- the same idiom librestudio_add_component()
# (cmake/LibreStudioComponent.cmake) and officecfg/CMakeLists.txt's own
# per-header custom commands already use, rather than a separate directory-only
# step. All outputs are then collected into one add_custom_target(... ALL
# DEPENDS ...), the same two-step shape program_services_rdb (root
# CMakeLists.txt) and librestudio_finalize_components() (cmake/
# LibreStudioComponent.cmake) both already use for their own single-output
# case.
set(_vcl_uiconfig_outputs "")
foreach (_vcl_uiconfig_src IN LISTS VCL_UICONFIG_UI_FILES)
    get_filename_component(_vcl_uiconfig_name "${_vcl_uiconfig_src}" NAME)
    set(_vcl_uiconfig_dest "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/vcl/ui/${_vcl_uiconfig_name}")
    add_custom_command(
        OUTPUT "${_vcl_uiconfig_dest}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/vcl/ui"
        COMMAND ${CMAKE_COMMAND} -E copy "${_vcl_uiconfig_src}" "${_vcl_uiconfig_dest}"
        DEPENDS "${_vcl_uiconfig_src}"
        COMMENT "Copying ${_vcl_uiconfig_name} into build-tree soffice.cfg/vcl/ui"
        VERBATIM)
    list(APPEND _vcl_uiconfig_outputs "${_vcl_uiconfig_dest}")
endforeach ()
unset(_vcl_uiconfig_src)
unset(_vcl_uiconfig_name)
unset(_vcl_uiconfig_dest)

add_custom_target(vcl_uiconfig ALL DEPENDS ${_vcl_uiconfig_outputs})
unset(_vcl_uiconfig_outputs)

# --- CppunitTest targets -------------------------------------------------------
#
# First sub-wave of vcl's ~50 CppunitTest_*.mk files: the 11 originally
# planned as needing no use_components -- true of all 11 -- and no
# use_configuration, which turned out to be wrong for 9 of them (this port's
# own discovery, not in the original plan for this wave -- re-verified
# directly against each live .mk file, not assumed; see vcl/
# CppunitTest_deferred.cmake's own header comment for the detail). Two need
# no use_ure at all (vcl_localize_digits, vcl_unit_conversion -- wired
# directly below, right here, since they need nothing this file does not
# already provide). The other nine all call use_ure (+use_vcl) *and*
# use_configuration *and* use_rdb(services), so, like scaddins_analysis/
# xmlscript_cppunit before them, they need program_services_rdb to exist
# first and are deferred to vcl/CppunitTest_deferred.cmake, include()'d from
# the root CMakeLists.txt right after xmlscript's own deferred include (see
# that include() call site). vcl_pdfium_library_test, the twelfth candidate
# this wave considered, is deliberately NOT wired anywhere -- see vcl/
# CppunitTest_deferred.cmake's own comment on it for why.

# CppunitTest_vcl_localize_digits.mk, re-checked directly against the live
# file: 1 source (vcl/qa/cppunit/LocalizeDigits), set_include vcl/inc,
# use_libraries sal vcl. No use_ure anywhere in the file, so no PROTECTORS, no
# CONFIGURATION -- this is as simple as a CppunitTest gets in this port.
librestudio_add_cppunit_test(vcl_localize_digits
    SOURCES
        vcl/qa/cppunit/LocalizeDigits
    LIBRARIES sal vcl
    INCLUDE "${CMAKE_SOURCE_DIR}/vcl/inc")

# CppunitTest_vcl_unit_conversion.mk, re-checked directly against the live
# file: 1 source (vcl/qa/cppunit/UnitConversionTest), set_include vcl/inc,
# use_libraries vcl only (no sal). No use_ure, same as vcl_localize_digits
# above.
librestudio_add_cppunit_test(vcl_unit_conversion
    SOURCES
        vcl/qa/cppunit/UnitConversionTest
    LIBRARIES vcl
    INCLUDE "${CMAKE_SOURCE_DIR}/vcl/inc")

# The other eight use_ure tests of this wave (vcl_app_test,
# vcl_bitmapfilter_test, vcl_bitmap_test, vcl_filter_igif, vcl_graphic_test,
# vcl_idle_task, vcl_map, vcl_type_serializer_test) are registered from vcl/
# CppunitTest_deferred.cmake instead of here -- see that file's own header
# comment for why they cannot be registered at this point in the file.
# vcl_pdfium_library_test, the eleventh test this wave's plan named, is not
# registered anywhere at all -- see that same file's header comment for why.

# vcl/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/vcl/CppunitTest_deferred.cmake")
