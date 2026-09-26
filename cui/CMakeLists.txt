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
#  - gb_Helper_optional(OPENCL, opencl) / the clew external: no OpenCL
#    infrastructure exists anywhere in this CMake port, so both are omitted.
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
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(cui officecfg_headers udkapi_normal_headers offapi_normal_headers)

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

librestudio_add_component(cui
    COMPONENT "${CMAKE_SOURCE_DIR}/cui/util/cui.component"
    LIBRARY   cui
    LAYER     OOO)

librestudio_set_library_output_name(cui cui OOO)

install(TARGETS cui LIBRARY DESTINATION program)
