# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# cui (module cui) -- the "common UI" dialog library: Tools > Options pages,
# the customize (menu/toolbar/event/macro) dialogs, character map, hyperlink,
# gallery, spell-check, hangul/hanja, thesaurus and drawing-object tabpages
# used by every application. Source list from Library_cui.mk.
#
# Flag resolutions applying to this build:
#  - gb_Helper_optional(AVMEDIA, avmedia): ENABLE_AVMEDIA defaults ON
#    (cmake/LibreStudioConfigHeaders.cmake, see avmedia/CMakeLists.txt's own
#    comment for the configure.ac evidence), so the avmedia link is
#    conditional on that switch below -- same resolution already established
#    for svx/svxcore (see svx's comment on the identical optional).
#  - gb_Helper_optional(SCRIPTING, basctl): HAVE_FEATURE_SCRIPTING is 1 in
#    this build (cmake/config/config_features.h.in, confirmed by basic (sb)
#    already being built), so basctl IS linked, and the two SCRIPTING-gated
#    exception objects below (MacroManagerDialog, optbasic) ARE included.
#  - $(if $(ENABLE_JAVA),jvmfwk): Java is out of scope for this port -- no
#    other module links jvmfwk or has any Java build infrastructure -- so
#    this link is omitted.
#  - gb_Helper_optional(OPENCL, opencl) / the clew external: stale note --
#    external/clew + opencl were ported later in this project (the "open
#    tasks" wave); ENABLE_OPENCL defaults ON, so both ARE linked, see the
#    fix below (added during the first real `cmake --build`, 2026-09-27,
#    when about.cxx's <opencl/openclwrapper.hxx> include turned out to need
#    clew/clew.h, unreachable without linking clew directly).
#  - gb_Helper_optional(EXTENSIONS, AdditionsDialog): HAVE_FEATURE_EXTENSIONS
#    is 0 (cmake/config/config_extensions.h.in -- the extension-manager
#    mechanism is not in this port's scope, per the note already recorded
#    when config_extensions.h.in was first created for basic (sb)), so
#    cui/source/dialogs/AdditionsDialog is omitted. Its only external
#    dependency, orcus/orcus-parser (json_document_tree/json_parser), is
#    consequently unused by any remaining source in this library and is
#    likewise omitted from gb_Library_use_externals -- no orcus CMake
#    infrastructure exists in this port either.
#  - zxing: cui/source/dialogs/QrCodeGenDialog.cxx is NOT gated by any
#    gb_Helper_optional in the .mk (it is an unconditional exception object),
#    but every ZXing header/symbol it touches is itself wrapped in
#    "#if ENABLE_ZXING" (cui/source/inc/QrCodeGenDialog.hxx,
#    cui/source/dialogs/QrCodeGenDialog.cxx), and config_zxing.h hardcodes
#    ENABLE_ZXING to 0 upstream. cmake/config/config_zxing.h.in (registered in
#    cmake/LibreStudioConfigHeaders.cmake's _config_headers list) now
#    generates <build>/config_host/config_zxing.h, which is on every target's
#    include path (same as every other config_*.h) -- no per-library include
#    wiring is needed here. The zxing external link is omitted (ENABLE_ZXING
#    resolves to 0), and QrCodeGenDialog.cxx is compiled as an ordinary
#    non-ZXing source.
#  - ifeq($(OS),WNT): cui/source/dialogs/fileextcheckdlg and the
#    advapi32/shlwapi system-library link are Windows-only, out of scope for
#    this Linux port.
#  - ENABLE_WASM_STRIP_PINGUSER and ENABLE_WASM_STRIP_HUNSPELL are both 0
#    (build/config_host/config_wasm_strip.h), so both "ifneq (...,TRUE)"
#    guards resolve true: tipofthedaydlg and hyphen ARE included.
#  - $(if $(ENABLE_BREAKPAD),crashreport): LIBRESTUDIO_ENABLE_BREAKPAD is not
#    defined anywhere as a real option in this port (same defensive guard
#    already noted for sfx2's and svx's identical crashreport link), so this
#    link is omitted.
#  - icu_headers (gb_Library_use_externals) is folded into the icuuc ->
#    ICU::uc mapping below -- ICU::uc's own interface include directories
#    already cover it, same as editeng and svtools do not declare a separate
#    target for it either.
#
# gb_Library_use_custom_headers,cui lists "oox/generated" (the oox token
# headers svx/svxcore generates for itself), but nothing under cui/source or
# cui/inc includes anything from oox/token -- unlike svx, cui needs no oox
# token generation, so it is not reproduced here.

set(CUI_SOURCES
    cui/source/dialogs/tipofthedaydlg
    cui/source/dialogs/hyphen
    cui/source/customize/acccfg
    cui/source/customize/cfg
    cui/source/customize/cfgutil
    cui/source/customize/CommandCategoryListBox
    cui/source/customize/eventdlg
    cui/source/customize/macropg
    cui/source/customize/SvxConfigPageHelper
    cui/source/customize/SvxMenuConfigPage
    cui/source/customize/SvxToolbarConfigPage
    cui/source/dialogs/about
    # gb_Helper_optional(EXTENSIONS, AdditionsDialog) -- excluded, see notes above.
    cui/source/dialogs/cuifmsearch
    cui/source/dialogs/cuigaldlg
    cui/source/dialogs/cuigrfflt
    cui/source/dialogs/cuiimapwnd
    cui/source/dialogs/cuitbxform
    cui/source/dialogs/DiagramDialog
    cui/source/dialogs/FontFeaturesDialog
    cui/source/dialogs/hangulhanjadlg
    cui/source/dialogs/hlmarkwn
    cui/source/dialogs/hyperlinktabpagebase
    cui/source/dialogs/hyperlinkdlg
    cui/source/dialogs/hyperlinkinternettabpage
    cui/source/dialogs/hyperlinkmailtabpage
    cui/source/dialogs/hyperlinkdoctabpage
    cui/source/dialogs/hyperlinknewdoctabpage
    cui/source/dialogs/insdlg
    cui/source/dialogs/insrc
    cui/source/dialogs/linkdlg
    cui/source/dialogs/multipat
    cui/source/dialogs/newtabledlg
    cui/source/dialogs/passwdomdlg
    cui/source/dialogs/screenshotannotationdlg
    cui/source/dialogs/pastedlg
    cui/source/dialogs/postdlg
    cui/source/dialogs/QrCodeGenDialog
    cui/source/dialogs/GraphicTestsDialog
    cui/source/dialogs/ImageViewerDialog
    cui/source/dialogs/scriptdlg
    cui/source/dialogs/SignatureLineDialogBase
    cui/source/dialogs/SignatureLineDialog
    cui/source/dialogs/SignSignatureLineDialog
    cui/source/dialogs/sdrcelldlg
    cui/source/dialogs/showcols
    cui/source/dialogs/SpellDialog
    cui/source/dialogs/splitcelldlg
    cui/source/dialogs/srchxtra
    cui/source/dialogs/thesdlg
    cui/source/dialogs/uipickerdlg
    cui/source/dialogs/whatsnewtabpage
    cui/source/dialogs/uitabpage
    cui/source/dialogs/toolbartabpage
    cui/source/dialogs/welcomedlg
    cui/source/dialogs/zoom
    cui/source/factory/cuiexp
    cui/source/factory/cuiresmgr
    cui/source/factory/dlgfact
    cui/source/factory/init
    cui/source/options/certpath
    cui/source/options/cfgchart
    cui/source/options/connpoolconfig
    cui/source/options/connpooloptions
    cui/source/options/connpoolsettings
    cui/source/options/cuisrchdlg
    cui/source/options/dbregister
    cui/source/options/dbregisterednamesconfig
    cui/source/options/dbregistersettings
    cui/source/options/doclinkdialog
    cui/source/options/fontsubs
    cui/source/options/optaboutconfig
    cui/source/options/optaccessibility
    cui/source/options/optasian
    cui/source/options/optchart
    cui/source/options/optctl
    cui/source/options/optdict
    cui/source/options/optfltr
    cui/source/options/optgdlg
    cui/source/options/optgenrl
    cui/source/options/opthtml
    cui/source/options/optlanguagetool
    cui/source/options/optinet2
    cui/source/options/optjava
    cui/source/options/optjsearch
    cui/source/options/optlingu
    cui/source/options/optpath
    cui/source/options/optsave
    cui/source/options/optupdt
    cui/source/options/appearance
    cui/source/options/sdbcdriverenum
    cui/source/options/securityoptions
    cui/source/options/treeopt
    cui/source/options/treeopthelper
    cui/source/options/tsaurls
    cui/source/options/webconninfo
    cui/source/tabpages/align
    cui/source/tabpages/autocdlg
    cui/source/tabpages/backgrnd
    cui/source/tabpages/bbdlg
    cui/source/tabpages/border
    cui/source/tabpages/chardlg
    cui/source/tabpages/connect
    cui/source/tabpages/grfpage
    cui/source/tabpages/labdlg
    cui/source/tabpages/macroass
    cui/source/tabpages/measure
    cui/source/tabpages/numfmt
    cui/source/tabpages/numpages
    cui/source/tabpages/page
    cui/source/tabpages/paragrph
    cui/source/tabpages/SvxPresetListBox
    cui/source/tabpages/swpossizetabpage
    cui/source/tabpages/tabarea
    cui/source/tabpages/tabline
    cui/source/tabpages/tabstpge
    cui/source/tabpages/textanim
    cui/source/tabpages/textattr
    cui/source/tabpages/TextColumnsPage
    cui/source/tabpages/tparea
    cui/source/tabpages/tpbitmap
    cui/source/tabpages/tpcolor
    cui/source/tabpages/tpgradnt
    cui/source/tabpages/tphatch
    cui/source/tabpages/tpline
    cui/source/tabpages/tplnedef
    cui/source/tabpages/tplneend
    cui/source/tabpages/tppattern
    cui/source/tabpages/tpshadow
    cui/source/tabpages/tptrans
    cui/source/tabpages/transfrm
    cui/source/util/FontFeatures
    cui/source/util/GetDocumentModel
    cui/source/dialogs/widgettestdlg
)

# gb_Helper_optional(SCRIPTING, cui/source/dialogs/MacroManagerDialog) and
# gb_Helper_optional(SCRIPTING, cui/source/options/optbasic) in
# Library_cui.mk -- both need basctl (also SCRIPTING-gated, see below).
# ENABLE_SCRIPTING defaults ON.
if (ENABLE_SCRIPTING)
    list(APPEND CUI_SOURCES
        cui/source/dialogs/MacroManagerDialog
        cui/source/options/optbasic
    )
endif ()

librestudio_expand_cxx(CUI_SOURCES)
add_library(cui SHARED ${CUI_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(cui PRIVATE
        "${CMAKE_SOURCE_DIR}/cui/inc/pch/precompiled_cui.hxx")
    target_compile_definitions(cui PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(cui PRIVATE
    "${CMAKE_SOURCE_DIR}/cui/inc"
    "${CMAKE_SOURCE_DIR}/cui/source/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${OOX_GENERATED_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(cui officecfg_headers oox_generated udkapi_normal_headers offapi_normal_headers)

# Fix (first real `cmake --build`, 2026-09-27): Library_cui.mk's own
# use_custom_headers list (lines 26-29) includes "oox/generated" alongside
# "officecfg/registry" -- missing here (only officecfg/registry was wired).
# cui/source/dialogs/DiagramDialog.cxx -> include/svx/diagram/datamodel_svx.hxx
# -> <oox/token/tokens.hxx> needs it; "fatal error: oox/token/tokens.hxx: No
# such file" without this. OOX_GENERATED_DIR/oox_generated (oox/CMakeLists.txt)
# were already made globally visible for sc/sd's own identical need.

target_compile_definitions(cui PRIVATE CUI_DLLIMPLEMENTATION)

target_link_libraries(cui PRIVATE
    basegfx comphelper cppu cppuhelper docmodel drawinglayer editeng
    fwk i18nlangtag i18nutil lng sal salhelper sax sb sfx sot svl svt svxcore
    svx tk tl ucbhelper utl vcl
    Boost::boost
    ICU::uc
    LibXml2::LibXml2)

# gb_Helper_optional(SCRIPTING, basctl) in Library_cui.mk. ENABLE_SCRIPTING
# defaults ON.
if (ENABLE_SCRIPTING)
    target_link_libraries(cui PRIVATE basctl)
endif ()

# gb_Helper_optional(AVMEDIA, avmedia) in Library_cui.mk:34. ENABLE_AVMEDIA
# defaults ON.
if (ENABLE_AVMEDIA)
    target_link_libraries(cui PRIVATE avmedia)
endif ()

# Fix (first real `cmake --build`, 2026-09-27): Library_cui.mk also links
# both opencl (lines 49-50) and clew (lines 73-74) under
# gb_Helper_optional(OPENCL, ...) -- missing here, the same "OPENCL
# BUILD_TYPE vs ENABLE_OPENCL" mismatch already found and fixed for vcl/scui/
# sofficeapp (see vcl/CMakeLists.txt's own comment on this exact class of
# gap): about.cxx includes <opencl/openclwrapper.hxx> (a public, global
# header), which itself needs <clew/clew.h> -- "fatal error: clew/clew.h: No
# such file" without this. clew's own target_include_directories(clew PUBLIC
# ...) (external/clew/CMakeLists.txt) propagates the include path
# automatically once linked directly (opencl links clew PRIVATE, so linking
# opencl alone would not be enough -- both are linked here, matching the
# real .mk's own list).
if (ENABLE_OPENCL)
    target_link_libraries(cui PRIVATE opencl clew)
endif ()

librestudio_add_component(cui
    COMPONENT "${CMAKE_SOURCE_DIR}/cui/util/cui.component"
    LIBRARY   cui
    LAYER     OOO)

librestudio_set_library_output_name(cui cui OOO)

install(TARGETS cui LIBRARY DESTINATION program)

# --- UIConfig_cui -----------------------------------------------------------
#
# UIConfig_cui.mk: gb_UIConfig_UIConfig(cui) -- .ui files under cui/uiconfig/ui/
# packaged into share/config/soffice.cfg/cui/ui -- same gb_UIConfig_DIR_FOR
# destination convention as UIConfig_vcl's own block above (module name "cui"
# used directly, not a "modules/..." path, matching UIConfig_svt/UIConfig_vcl/
# UIConfig_svx, not UIConfig_basicide's "modules/BasicIDE"). Re-checked
# directly against the live UIConfig_cui.mk:
#  - "ifeq ($(OS),WNT)" gb_UIConfig_add_uifiles(cui, fileextcheckdialog) --
#    Windows-only, out of scope for this Linux port (same resolution already
#    recorded above for Library_cui.mk's own OS==WNT branch).
#  - "ifneq ($(ENABLE_WASM_STRIP_PINGUSER),TRUE)" gb_UIConfig_add_uifiles(cui,
#    tipofthedaydialog) -- ENABLE_WASM_STRIP_PINGUSER is 0 in this build (same
#    resolution already recorded above for Library_cui.mk's own guard), so
#    this resolves true: tipofthedaydialog IS included.
#  - The remaining unconditional gb_UIConfig_add_uifiles(cui, ...) block: 207
#    further .ui files.
# Total: 208 .ui files (209 files exist on disk under cui/uiconfig/ui/;
# fileextcheckdialog.ui is the one WNT-only exclusion), all verified present
# via the live directory listing.
#
# CppunitTest_cui_dialogs_test*.mk's own gb_CppunitTest_use_uiconfigs(
# cui_dialogs_testN, cui) calls (cui/CppunitTest_deferred.cmake) need a real
# build-time ALL target to depend on, the same reason UIConfig_vcl/UIConfig_svt/
# UIConfig_svx/UIConfig_basicide each grew their own "<module>_uiconfig"
# target -- introduced here as "cui_uiconfig".
set(CUI_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tipofthedaydialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/aboutdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/aboutconfigdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/accelconfigpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/additionsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/additionsfragment.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/agingdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/acorexceptpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/acoroptionspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/acorreplacepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/applyautofmtpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/applylocalizedpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/areadialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/areatabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/asiantypography.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/assigncomponentdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/autocorrectdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/baselinksdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/imagetabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/borderareatransparencydialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/borderbackgrounddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/borderpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/bulletandposition.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/breaknumberoption.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/calloutdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/calloutpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/cellalignment.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/certdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/chapterfragment.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/charnamepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/colorfragment.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/colorpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/colorpickerdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/comment.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/connectortabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/connpooloptions.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/croppage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/cuiimapdlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/databaselinkdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/diagramdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dimensionlinestabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/editdictionarydialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/editmodulesdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/embossdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/entrycontextmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/eventassigndialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/eventassignpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/fontfragment.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/formatnumberdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/fmsearchdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/gradientpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/customizedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dbregisterpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/effectspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/eventsconfigpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/formatcellsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/fontfeaturesdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/galleryapplyprogress.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/galleryfilespage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/gallerygeneralpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/gallerysearchprogress.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/gallerythemedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/gallerythemeiddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/gallerytitledialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/galleryupdateprogress.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/graphictestdlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/graphictestentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/imageviewer.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/hangulhanjaadddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/hangulhanjaeditdictdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/hangulhanjaconversiondialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/hangulhanjaoptdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/hatchpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/hyperlinkdocpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/hyperlinkmarkdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/hyperlinknewdocpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/hyperlinkdlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/hyperlinkinternetpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/hyperlinkmailpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/hyphenate.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/iconchangedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/iconselectordialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/insertfloatingframe.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/insertoleobject.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/insertrowcolumn.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/javaclasspathdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/javastartparametersdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/linedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/linetabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/lineendstabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/linestyletabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/listdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/macroassigndialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/macroassignpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/macromanagerdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/macroselectordialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/menuassignpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/mosaicdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/movemenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/multipathdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/namedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/newlibdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/newtabledialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/newtoolbardialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/numberdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/numberingformatpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/numberingoptionspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/numberingpositionpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/objectnamedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/objecttitledescdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optaccessibilitypage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optadvancedpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optasianpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optbasicidepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optctlpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optchartcolorspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optemailpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optfltrpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optfltrembedpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optfontspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optgeneralpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/opthtmlpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/langtoolconfigpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optionsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optjsearchpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optlanguagespage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optlingupage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optnewdictionarydialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optonlineupdatepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optpathspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optproxypage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optsavepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optsecuritypage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optuserpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optviewpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/pageformatpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/paragalignpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/paraindentspacing.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/paratabspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/password.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/screenshotannotationdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/pastespecial.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/patterntabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/percentdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/appearance.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/pickbulletpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/pickgraphicpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/picknumberingpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/pickoutlinepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/positionpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/positionsizedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/possizetabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/posterdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/qrcodegen.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querychangelineenddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querydeletebitmapdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querydeletechartcolordialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querydeletecolordialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querydeletedictionarydialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querydeletegradientdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querydeletehatchdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querydeletelineenddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querydeletelinestyledialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/queryduplicatedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querynoloadedfiledialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querynosavefiledialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querysavelistdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/queryupdategalleryfilelistdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/recordnumberdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/rotationtabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/scriptorganizer.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/searchattrdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/searchformatdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/securityoptionsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/selectpathdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/shadowtabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/showcoldialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/similaritysearchdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/signatureline.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/signsignatureline.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/slantcornertabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/smarttagoptionspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/smoothdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/solarizedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/specialcharacters.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/spellingdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/spelloptionsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/splitcellsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/storedwebconnectiondialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/swpossizepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/textattrtabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/textanimtabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/textcolumnstabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/textdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/textflowpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/thesaurus.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/uipickerdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/uitabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/toolbartabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/transparencytabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tsaurldialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/twolinespage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/wordcompletionpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/spinbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/zoomdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/widgettestdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/welcomedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/whatsnewtabpage.ui"
)

install(FILES ${CUI_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/cui/ui)

# Real build-time mirror of the install() rule above, alongside it (not
# instead of it) -- same per-file add_custom_command + one add_custom_target(
# ... ALL DEPENDS ...) shape as UIConfig_vcl's own vcl_uiconfig above; see
# that block's comment for the full reasoning, not repeated here.
set(_cui_uiconfig_outputs "")
foreach (_cui_uiconfig_src IN LISTS CUI_UICONFIG_UI_FILES)
    get_filename_component(_cui_uiconfig_name "${_cui_uiconfig_src}" NAME)
    set(_cui_uiconfig_dest "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/cui/ui/${_cui_uiconfig_name}")
    add_custom_command(
        OUTPUT "${_cui_uiconfig_dest}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/cui/ui"
        COMMAND ${CMAKE_COMMAND} -E copy "${_cui_uiconfig_src}" "${_cui_uiconfig_dest}"
        DEPENDS "${_cui_uiconfig_src}"
        COMMENT "Copying ${_cui_uiconfig_name} into build-tree soffice.cfg/cui/ui"
        VERBATIM)
    list(APPEND _cui_uiconfig_outputs "${_cui_uiconfig_dest}")
endforeach ()
unset(_cui_uiconfig_src)
unset(_cui_uiconfig_name)
unset(_cui_uiconfig_dest)

add_custom_target(cui_uiconfig ALL DEPENDS ${_cui_uiconfig_outputs})
unset(_cui_uiconfig_outputs)

# cui/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/cui/CppunitTest_deferred.cmake")
