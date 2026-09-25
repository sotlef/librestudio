# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# svx (module svx) — two libraries. svxcore is the heavy "core" drawing/form/
# gallery/theme engine (SdrObjects, item sets, gallery, table shapes, the
# uno::drawing API); svx is a thinner UI-facing wrapper on top of it that adds
# dialogs, sidebar panels, toolbox controls and the RecoveryUI/CrashReportUI
# UNO services. Source lists from Library_svxcore.mk and Library_svx.mk.
#
# Flag resolutions applying to both libraries in this build:
#  - HAVE_FEATURE_AVMEDIA is hardcoded 0 (cmake/config/config_features.h.in):
#    svxcore's sdrmediawindow (guarded by gb_Helper_optional,AVMEDIA) is
#    excluded, as is svx's MediaPlaybackPanel sidebar source and its direct
#    link to avmedia (svxcore still links avmedia unconditionally for the
#    media item itself, same reasoning as editeng -> avmedia).
#  - HAVE_FEATURE_DBCONNECTIVITY defaults to enabled on desktop Linux (it is
#    only forced off under --enable-wasm-strip -- config_features.h.in's
#    literal 0 is just autoheader's unconfigured template, not a real
#    default): dbtools is linked into both libraries, and
#    svx/source/form/filtnav is included. See connectivity/CMakeLists.txt.
#  - LIBRESTUDIO_ENABLE_BREAKPAD is not defined anywhere as a real option
#    (same defensive-guard pattern as sfx2's crashreport link): svx's
#    crashreport link, curl external, crashreportdlg/crashreportui sources
#    and the svx.component.crashreport marker are all excluded.
#  - ENABLE_WASM_STRIP_ACCESSIBILITY and ENABLE_WASM_STRIP_RECOVERYUI are
#    unset (0) in this port, so both `ifneq (...,TRUE)` guards resolve true:
#    the accessibility sources and the recoveryui/docrecovery sources ARE
#    included.
#  - ENABLE_WASM_STRIP_BASIC_DRAW_MATH_IMPRESS is not defined anywhere in
#    this port, so svxcore's `draw` componentimpl IS included (same
#    resolution already established for xmloff's xo.component.draw).
#
# svx/sdi/svxslots.sdi (compiled by svidl, like sfx2/sdi/sfxslots.sdi) is
# svxcore's SDI target; svx/sdi/svx.sdi is the EXPORTS master slot list --
# confirmed against the real gb_SdiTarget_SdiTarget macro in
# solenv/gbuild/SdiTarget.mk, same argument order already used for sfx2_sdi.
#
# include/svx/diagram/datamodel_svx.hxx (a svxcore header) reaches
# oox/token/tokens.hxx -- the one piece of the oox module svxcore actually
# needs, even though oox itself has not been ported. Nothing else in svx or
# svxcore includes anything else from oox (no oox/token/namespaces.hxx,
# properties.hxx, or any runtime oox:: symbol), so only the generate-tokens.py
# pipeline for oox/source/token/tokens.txt is reproduced here -- the same
# script and head/tail-sandwich technique already used for
# xmloff/source/token/tokens.txt in xmloff/CMakeLists.txt, minus the
# tokenhash.inc/gperf stage (that only feeds oox's own fasttokenhandler.cxx,
# which is not built).
find_program(PYTHON3_EXECUTABLE NAMES python3 REQUIRED)

set(OOX_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(OOX_TOKEN_SRC_DIR "${CMAKE_SOURCE_DIR}/oox/source/token")
set(OOX_TOKENS_HXX "${OOX_GENERATED_DIR}/oox/token/tokens.hxx")

add_custom_command(
    OUTPUT "${OOX_TOKENS_HXX}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OOX_GENERATED_DIR}/misc"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OOX_GENERATED_DIR}/oox/token"
    COMMAND "${PYTHON3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/solenv/bin/generate-tokens.py"
            "${OOX_TOKEN_SRC_DIR}/tokens.txt"
            "${OOX_GENERATED_DIR}/misc/tokenids.inc"
            "${OOX_GENERATED_DIR}/tokennames.inc"
            "${OOX_GENERATED_DIR}/misc/tokenhash.gperf"
    COMMAND ${CMAKE_COMMAND} -E cat
            "${OOX_TOKEN_SRC_DIR}/tokens.hxx.head"
            "${OOX_GENERATED_DIR}/misc/tokenids.inc"
            "${OOX_TOKEN_SRC_DIR}/tokens.hxx.tail" > "${OOX_TOKENS_HXX}"
    DEPENDS "${OOX_TOKEN_SRC_DIR}/tokens.txt"
            "${OOX_TOKEN_SRC_DIR}/tokens.hxx.head"
            "${OOX_TOKEN_SRC_DIR}/tokens.hxx.tail"
            "${CMAKE_SOURCE_DIR}/solenv/bin/generate-tokens.py"
    COMMENT "generate-tokens.py: oox/token/tokens.hxx"
    VERBATIM)

add_custom_target(svx_oox_tokens DEPENDS "${OOX_TOKENS_HXX}")

librestudio_add_sdi_target(svx_sdi
    SDI        "${CMAKE_SOURCE_DIR}/svx/sdi/svxslots.sdi"
    EXPORTS    "${CMAKE_SOURCE_DIR}/svx/sdi/svx.sdi"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/SdiTarget/svx/sdi"
    INCLUDES   "${CMAKE_SOURCE_DIR}/sfx2/sdi")

set(SVXCORE_SOURCES
    svx/source/annotation/Annotation
    svx/source/annotation/AnnotationEnumeration
    svx/source/annotation/AnnotationObject
    svx/source/annotation/TextAPI
    svx/source/core/extedit
    svx/source/core/graphichelper
    svx/source/core/extendedprimitive2dxmldump
    svx/source/customshapes/EnhancedCustomShape2d
    svx/source/customshapes/EnhancedCustomShapeFunctionParser
    svx/source/customshapes/EnhancedCustomShapeGeometry
    svx/source/customshapes/EnhancedCustomShapeTypeNames
    svx/source/diagram/datamodel_svx
    svx/source/diagram/DiagramHelper_svx
    svx/source/dialog/dialmgr
    svx/source/dialog/dlgutil
    svx/source/dialog/framelink
    svx/source/dialog/framelinkarray
    svx/source/dialog/langbox
    svx/source/dialog/pagenumberlistbox
    svx/source/dialog/papersizelistbox
    svx/source/dialog/samecontentlistbox
    svx/source/dialog/spacinglistbox
    svx/source/dialog/svxdlg
    svx/source/dialog/SvxNumOptionsTabPageHelper
    svx/source/engine3d/camera3d
    svx/source/engine3d/cube3d
    svx/source/engine3d/deflt3d
    svx/source/engine3d/dragmt3d
    svx/source/engine3d/e3dsceneupdater
    svx/source/engine3d/e3dundo
    svx/source/engine3d/extrud3d
    svx/source/engine3d/helperhittest3d
    svx/source/engine3d/helperminimaldepth3d
    svx/source/engine3d/lathe3d
    svx/source/engine3d/obj3d
    svx/source/engine3d/objfac3d
    svx/source/engine3d/polygn3d
    svx/source/engine3d/scene3d
    svx/source/engine3d/sphere3d
    svx/source/engine3d/svx3ditems
    svx/source/engine3d/view3d
    svx/source/engine3d/view3d1
    svx/source/engine3d/viewpt3d2
    svx/source/fmcomp/dbaexchange
    svx/source/fmcomp/fmgridcl
    svx/source/fmcomp/fmgridif
    svx/source/fmcomp/gridcell
    svx/source/fmcomp/gridcols
    svx/source/fmcomp/gridctrl
    svx/source/fmcomp/xmlexchg
    svx/source/form/dataaccessdescriptor
    svx/source/form/datalistener
    svx/source/form/datanavi
    svx/source/form/delayedevent
    svx/source/form/labelitemwindow
    svx/source/form/fmcontrolbordermanager
    svx/source/form/fmcontrollayout
    svx/source/form/fmdmod
    svx/source/form/fmdocumentclassification
    svx/source/form/fmexch
    svx/source/form/fmexpl
    svx/source/form/fmmodel
    svx/source/form/fmobj
    svx/source/form/fmpage
    svx/source/form/fmpgeimp
    svx/source/form/fmscriptingenv
    svx/source/form/fmservs
    svx/source/form/fmshell
    svx/source/form/fmshimp
    svx/source/form/fmtextcontroldialogs
    svx/source/form/fmtextcontrolfeature
    svx/source/form/fmtextcontrolshell
    svx/source/form/fmtools
    svx/source/form/fmundo
    svx/source/form/fmview
    svx/source/form/fmvwimp
    svx/source/form/formcontrolfactory
    svx/source/form/formcontroller
    svx/source/form/formcontrolling
    svx/source/form/formdispatchinterceptor
    svx/source/form/formfeaturedispatcher
    svx/source/form/formtoolbars
    svx/source/form/legacyformcontroller
    svx/source/form/navigatortree
    svx/source/form/navigatortreemodel
    svx/source/form/ParseContext
    svx/source/form/sdbdatacolumn
    svx/source/form/sqlparserclient
    svx/source/form/typemap
    svx/source/form/xfm_addcondition
    svx/source/gallery2/codec
    svx/source/gallery2/galbrws1
    svx/source/gallery2/galctrl
    svx/source/gallery2/galexpl
    svx/source/gallery2/galini
    svx/source/gallery2/gallery1
    svx/source/gallery2/galmisc
    svx/source/gallery2/galobj
    svx/source/gallery2/galtheme
    svx/source/gallery2/GalleryControl
    svx/source/gallery2/galleryobjectcollection
    svx/source/gallery2/galleryfilestorage
    svx/source/gallery2/galleryfilestorageentry
    svx/source/gallery2/gallerystoragelocations
    svx/source/items/chrtitem
    svx/source/items/clipfmtitem
    svx/source/items/customshapeitem
    svx/source/items/drawitem
    svx/source/items/e3ditem
    svx/source/items/RectangleAlignmentItem
    svx/source/items/galleryitem
    svx/source/items/grfitem
    svx/source/sdr/animation/scheduler
    svx/source/sdr/animation/objectanimator
    svx/source/sdr/animation/animationstate
    svx/source/sdr/attribute/sdrallfillattributeshelper
    svx/source/sdr/attribute/sdrlinefilleffectstextattribute
    svx/source/sdr/attribute/sdrfilltextattribute
    svx/source/sdr/attribute/sdreffectstextattribute
    svx/source/sdr/attribute/sdrtextattribute
    svx/source/sdr/attribute/sdrlineeffectstextattribute
    svx/source/sdr/attribute/sdrformtextattribute
    svx/source/sdr/attribute/sdrformtextoutlineattribute
    svx/source/sdr/contact/viewobjectcontactofgroup
    svx/source/sdr/contact/viewobjectcontactofsdrpage
    svx/source/sdr/contact/viewcontactofpageobj
    svx/source/sdr/contact/viewcontactofe3d
    svx/source/sdr/contact/viewcontactofe3dcube
    svx/source/sdr/contact/viewcontactofsdrmediaobj
    svx/source/sdr/contact/viewobjectcontactofmasterpagedescriptor
    svx/source/sdr/contact/viewobjectcontactofpageobj
    svx/source/sdr/contact/viewobjectcontactofe3dscene
    svx/source/sdr/contact/viewcontactofgraphic
    svx/source/sdr/contact/viewobjectcontactredirector
    svx/source/sdr/contact/viewcontactofsdrcircobj
    svx/source/sdr/contact/viewcontactofgroup
    svx/source/sdr/contact/viewcontactofdiagram
    svx/source/sdr/contact/viewcontactofunocontrol
    svx/source/sdr/contact/viewcontactofsdrobjcustomshape
    svx/source/sdr/contact/viewcontactofsdrobj
    svx/source/sdr/contact/objectcontact
    svx/source/sdr/contact/viewcontactoftextobj
    svx/source/sdr/contact/viewcontactofe3dlathe
    svx/source/sdr/contact/viewcontactofsdrpage
    svx/source/sdr/contact/viewcontactofe3dextrude
    svx/source/sdr/contact/viewcontactofmasterpagedescriptor
    svx/source/sdr/contact/viewcontact
    svx/source/sdr/contact/viewcontactofsdrole2obj
    svx/source/sdr/contact/displayinfo
    svx/source/sdr/contact/viewcontactofe3dscene
    svx/source/sdr/contact/viewcontactofsdrrectobj
    svx/source/sdr/contact/viewobjectcontactofsdrole2obj
    svx/source/sdr/contact/viewobjectcontactofunocontrol
    svx/source/sdr/contact/viewobjectcontactofgraphic
    # gb_Helper_optional(AVMEDIA, sdrmediawindow) -- excluded, HAVE_FEATURE_AVMEDIA is 0.
    svx/source/sdr/contact/viewobjectcontactofsdrmediaobj
    svx/source/sdr/contact/viewcontactofsdrcaptionobj
    svx/source/sdr/contact/viewcontactofvirtobj
    svx/source/sdr/contact/viewcontactofe3dsphere
    svx/source/sdr/contact/viewcontactofe3dpolygon
    svx/source/sdr/contact/viewcontactofsdredgeobj
    svx/source/sdr/contact/viewobjectcontact
    svx/source/sdr/contact/viewobjectcontactofsdrobj
    svx/source/sdr/contact/objectcontactofpageview
    svx/source/sdr/contact/viewcontactofsdrpathobj
    svx/source/sdr/contact/viewcontactofsdrmeasureobj
    svx/source/sdr/contact/objectcontactofobjlistpainter
    svx/source/sdr/contact/viewobjectcontactofe3d
    svx/source/sdr/overlay/overlayline
    svx/source/sdr/overlay/overlaycrosshair
    svx/source/sdr/overlay/overlayrollingrectangle
    svx/source/sdr/overlay/overlaytriangle
    svx/source/sdr/overlay/overlayselection
    svx/source/sdr/overlay/overlayrectangle
    svx/source/sdr/overlay/overlaybitmap
    svx/source/sdr/overlay/overlaymanagerbuffered
    svx/source/sdr/overlay/overlayhelpline
    svx/source/sdr/overlay/overlayanimatedbitmap
    svx/source/sdr/overlay/overlaypolypolygon
    svx/source/sdr/overlay/overlayobjectcell
    svx/source/sdr/overlay/overlaytools
    svx/source/sdr/overlay/overlayprimitive2dsequenceobject
    svx/source/sdr/overlay/overlayobject
    svx/source/sdr/overlay/overlaymanager
    svx/source/sdr/overlay/overlayobjectlist
    svx/source/sdr/overlay/overlayhandle
    svx/source/sdr/misc/ImageMapInfo
    svx/source/sdr/primitive2d/sdrellipseprimitive2d
    svx/source/sdr/primitive2d/sdrprimitivetools
    svx/source/sdr/primitive2d/sdrtextprimitive2d
    svx/source/sdr/primitive2d/primitivefactory2d
    svx/source/sdr/primitive2d/sdrolecontentprimitive2d
    svx/source/sdr/primitive2d/sdrcustomshapeprimitive2d
    svx/source/sdr/primitive2d/sdrgrafprimitive2d
    svx/source/sdr/primitive2d/sdrole2primitive2d
    svx/source/sdr/primitive2d/sdrpathprimitive2d
    svx/source/sdr/primitive2d/sdrrectangleprimitive2d
    svx/source/sdr/primitive2d/sdrcaptionprimitive2d
    svx/source/sdr/primitive2d/sdrconnectorprimitive2d
    svx/source/sdr/primitive2d/sdrmeasureprimitive2d
    svx/source/sdr/primitive2d/sdrframeborderprimitive2d
    svx/source/sdr/primitive2d/sdrattributecreator
    svx/source/sdr/primitive2d/sdrdecompositiontools
    svx/source/sdr/primitive3d/sdrattributecreator3d
    svx/source/sdr/properties/pageproperties
    svx/source/sdr/properties/e3dsceneproperties
    svx/source/sdr/properties/itemsettools
    svx/source/sdr/properties/captionproperties
    svx/source/sdr/properties/e3dextrudeproperties
    svx/source/sdr/properties/e3dproperties
    svx/source/sdr/properties/measureproperties
    svx/source/sdr/properties/customshapeproperties
    svx/source/sdr/properties/rectangleproperties
    svx/source/sdr/properties/groupproperties
    svx/source/sdr/properties/properties
    svx/source/sdr/properties/e3dlatheproperties
    svx/source/sdr/properties/graphicproperties
    svx/source/sdr/properties/circleproperties
    svx/source/sdr/properties/emptyproperties
    svx/source/sdr/properties/e3dsphereproperties
    svx/source/sdr/properties/defaultproperties
    svx/source/sdr/properties/textproperties
    svx/source/sdr/properties/attributeproperties
    svx/source/sdr/properties/connectorproperties
    svx/source/sdr/properties/e3dcompoundproperties
    svx/source/sdr/properties/oleproperties
    svx/source/svdraw/charthelper
    svx/source/svdraw/constructhelper
    svx/source/svdraw/clonelist
    svx/source/svdraw/gradtrns
    svx/source/svdraw/polypolygoneditor
    svx/source/svdraw/presetooxhandleadjustmentrelations
    svx/source/svdraw/sdrhittesthelper
    svx/source/svdraw/sdrmasterpagedescriptor
    svx/source/svdraw/sdrpagewindow
    svx/source/svdraw/sdrpaintwindow
    svx/source/svdraw/sdrundomanager
    svx/source/svdraw/selectioncontroller
    svx/source/svdraw/svdattr
    svx/source/svdraw/svdcrtv
    svx/source/svdraw/svddrag
    svx/source/svdraw/svddrgmt
    svx/source/svdraw/svddrgv
    svx/source/svdraw/svdedtv
    svx/source/svdraw/svdedtv1
    svx/source/svdraw/svdedtv2
    svx/source/svdraw/svdedxv
    svx/source/svdraw/svdetc
    svx/source/svdraw/svdfmtf
    svx/source/svdraw/svdglev
    svx/source/svdraw/svdglue
    svx/source/svdraw/svdhdl
    svx/source/svdraw/svdhlpln
    svx/source/svdraw/svditer
    svx/source/svdraw/svdlayer
    svx/source/svdraw/svdmark
    svx/source/svdraw/svdmodel
    svx/source/svdraw/svdmrkv
    svx/source/svdraw/svdmrkv1
    svx/source/svdraw/svdoashp
    svx/source/svdraw/svdoattr
    svx/source/svdraw/svdobj
    svx/source/svdraw/svdobjplusdata
    svx/source/svdraw/svdobjuserdatalist
    svx/source/svdraw/svdocapt
    svx/source/svdraw/svdocirc
    svx/source/svdraw/svdoedge
    svx/source/svdraw/svdograf
    svx/source/svdraw/svdogrp
    svx/source/svdraw/svdomeas
    svx/source/svdraw/svdomedia
    svx/source/svdraw/svdoole2
    svx/source/svdraw/svdopage
    svx/source/svdraw/svdopath
    svx/source/svdraw/svdorect
    svx/source/svdraw/svdotext
    svx/source/svdraw/svdotextdecomposition
    svx/source/svdraw/svdotextpathdecomposition
    svx/source/svdraw/svdotxat
    svx/source/svdraw/svdotxdr
    svx/source/svdraw/svdotxed
    svx/source/svdraw/svdotxfl
    svx/source/svdraw/svdotxln
    svx/source/svdraw/svdotxtr
    svx/source/svdraw/svdouno
    svx/source/svdraw/svdoutl
    svx/source/svdraw/svdoutlinercache
    svx/source/svdraw/svdovirt
    svx/source/svdraw/svdpage
    svx/source/svdraw/svdpagv
    svx/source/svdraw/svdpdf
    svx/source/svdraw/svdpntv
    svx/source/svdraw/svdpoev
    svx/source/svdraw/svdsnpv
    svx/source/svdraw/svdtext
    svx/source/svdraw/svdtrans
    svx/source/svdraw/svdundo
    svx/source/svdraw/svdview
    svx/source/svdraw/svdviter
    svx/source/svdraw/svdxcgv
    svx/source/svdraw/sxelditm
    svx/source/svdraw/textchain
    svx/source/svdraw/textchainflow
    svx/source/svdraw/textchaincursor
    svx/source/styles/CommonStylePreviewRenderer
    svx/source/styles/CommonStyleManager
    svx/source/styles/ColorSets
    svx/source/table/cell
    svx/source/table/cellcursor
    svx/source/table/cellrange
    svx/source/table/propertyset
    svx/source/table/svdotable
    svx/source/table/tablecolumn
    svx/source/table/tablecolumns
    svx/source/table/tablecontroller
    svx/source/table/tablehandles
    svx/source/table/tablelayouter
    svx/source/table/tablemodel
    svx/source/table/tablerow
    svx/source/table/tablerows
    svx/source/table/tableundo
    svx/source/table/viewcontactoftableobj
    svx/source/theme/ThemeColorChangerCommon
    svx/source/theme/ThemeColorPaletteManager
    svx/source/tbxctrls/ColorIconView
    svx/source/tbxctrls/extrusioncontrols
    svx/source/tbxctrls/fontworkgallery
    svx/source/tbxctrls/IColorSet
    svx/source/tbxctrls/linectrl
    svx/source/tbxctrls/Palette
    svx/source/tbxctrls/PaletteManager
    svx/source/tbxctrls/tbcontrl
    svx/source/tbxctrls/tbxcolorupdate
    svx/source/tbxctrls/StylesPreviewToolBoxControl
    svx/source/tbxctrls/StylesPreviewWindow
    svx/source/toolbars/extrusionbar
    svx/source/toolbars/fontworkbar
    svx/source/unodraw/gluepts
    svx/source/unodraw/SvxXTextColumns
    svx/source/unodraw/tableshape
    svx/source/unodraw/unobrushitemhelper
    svx/source/unodraw/unobtabl
    svx/source/unodraw/unodtabl
    svx/source/unodraw/UnoGraphicExporter
    svx/source/unodraw/unogtabl
    svx/source/unodraw/unohtabl
    svx/source/unodraw/unomlstr
    svx/source/unodraw/unomod
    svx/source/unodraw/unomtabl
    svx/source/unodraw/UnoNameItemTable
    svx/source/unodraw/unopage
    svx/source/unodraw/unoprov
    svx/source/unodraw/unoshap2
    svx/source/unodraw/unoshap3
    svx/source/unodraw/unoshap4
    svx/source/unodraw/unoshape
    svx/source/unodraw/unoshtxt
    svx/source/unodraw/unottabl
    svx/source/unodraw/XPropertyTable
    svx/source/xml/xmleohlp
    svx/source/xml/xmlexport
    svx/source/xml/xmlxtexp
    svx/source/xml/xmlxtimp
    svx/source/xoutdev/fillbitmaplink
    svx/source/xoutdev/xattr
    svx/source/xoutdev/xattr2
    svx/source/xoutdev/xattrbmp
    svx/source/xoutdev/_xoutbmp
    svx/source/xoutdev/_xpoly
    svx/source/xoutdev/xtabbtmp
    svx/source/xoutdev/xtabcolr
    svx/source/xoutdev/xtabdash
    svx/source/xoutdev/xtabgrdt
    svx/source/xoutdev/xtabhtch
    svx/source/xoutdev/xtable
    svx/source/xoutdev/xtabptrn
    svx/source/xoutdev/XPropertyEntry
    svx/source/xoutdev/xtablend
    svx/source/customshapes/EnhancedCustomShape3d
    svx/source/customshapes/EnhancedCustomShapeEngine
    svx/source/customshapes/EnhancedCustomShapeFontWork
    svx/source/customshapes/EnhancedCustomShapeHandle
)

librestudio_expand_cxx(SVXCORE_SOURCES)
add_library(svxcore SHARED ${SVXCORE_SOURCES})
add_dependencies(svxcore svx_sdi svx_oox_tokens)

target_include_directories(svxcore PRIVATE
    "${CMAKE_SOURCE_DIR}/svx/inc"
    "${CMAKE_SOURCE_DIR}/svx/source/inc"
    "${svx_sdi_INCLUDE_DIR}"
    "${OOX_GENERATED_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}")
add_dependencies(svxcore officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(svxcore PRIVATE SVXCORE_DLLIMPLEMENTATION)

target_link_libraries(svxcore PRIVATE
    avmedia basegfx sb comphelper cppuhelper cppu dbtools docmodel
    drawinglayercore drawinglayer editeng fwk i18nlangtag i18nutil lng sal
    salhelper sax sfx sot svl svt tk tl ucbhelper utl vcl xo
    Boost::boost
    ICU::uc
    LibXml2::LibXml2)

# gb_Library_add_componentimpls(svxcore, draw) -- applies unconditionally,
# ENABLE_WASM_STRIP_BASIC_DRAW_MATH_IMPRESS is not defined in this port (same
# resolution as xmloff's xo.component.draw).
librestudio_add_component(svxcore
    COMPONENT      "${CMAKE_SOURCE_DIR}/svx/util/svxcore.component"
    IMPLEMENTATION "${CMAKE_SOURCE_DIR}/svx/util/svxcore.component.draw"
    LIBRARY        svxcore
    LAYER          OOO)

librestudio_set_library_output_name(svxcore svxcore OOO)

install(TARGETS svxcore LIBRARY DESTINATION program)

set(SVX_SOURCES
    # ifneq ($(ENABLE_WASM_STRIP_RECOVERYUI),TRUE) -- applies, the flag is
    # unset in this build.
    svx/source/dialog/docrecovery
    svx/source/unodraw/recoveryui

    # ifneq ($(ENABLE_WASM_STRIP_ACCESSIBILITY),TRUE) -- applies, the flag is
    # unset in this build.
    svx/source/accessibility/AccessibleControlShape
    svx/source/accessibility/AccessibleEmptyEditSource
    svx/source/accessibility/AccessibleGraphicShape
    svx/source/accessibility/AccessibleOLEShape
    svx/source/accessibility/AccessibleShape
    svx/source/accessibility/AccessibleShapeInfo
    svx/source/accessibility/AccessibleShapeTreeInfo
    svx/source/accessibility/AccessibleTextEventQueue
    svx/source/accessibility/AccessibleTextHelper
    svx/source/accessibility/ChildrenManager
    svx/source/accessibility/ChildrenManagerImpl
    svx/source/accessibility/DescriptionGenerator
    svx/source/accessibility/GraphCtlAccessibleContext
    svx/source/accessibility/ShapeTypeHandler
    svx/source/accessibility/SvxShapeTypes
    svx/source/accessibility/accessiblewrapper
    svx/source/accessibility/lookupcolorname
    svx/source/accessibility/proxyaggregation

    svx/source/accessibility/AccessibleFrameSelector
    svx/source/accessibility/charmapacc
    svx/source/accessibility/svxpixelctlaccessiblecontext
    svx/source/accessibility/svxrectctaccessiblecontext
    svx/source/dialog/GenericCheckDialog
    svx/source/dialog/_bmpmask
    svx/source/dialog/charmap
    svx/source/dialog/cuicharmap
    svx/source/dialog/searchcharmap
    svx/source/dialog/connctrl
    svx/source/dialog/_contdlg
    svx/source/dialog/contwnd
    svx/source/dialog/gotodlg
    svx/source/dialog/compressgraphicdialog
    # gb_Helper_optional(BREAKPAD, crashreportdlg crashreportui) -- excluded,
    # LIBRESTUDIO_ENABLE_BREAKPAD is not defined in this port.
    svx/source/dialog/ctredlin
    svx/source/dialog/ClassificationCommon
    svx/source/dialog/ClassificationDialog
    svx/source/dialog/ClassificationEditView
    svx/source/dialog/ClassificationField
    svx/source/dialog/databaseregistrationui
    svx/source/dialog/dialcontrol
    svx/source/dialog/dlgctl3d
    svx/source/dialog/dlgctrl
    svx/source/dialog/fntctrl
    svx/source/dialog/fontwork
    svx/source/dialog/frmdirlbox
    svx/source/dialog/frmsel
    svx/source/dialog/graphctl
    svx/source/dialog/grfflt
    svx/source/dialog/hdft
    svx/source/dialog/hyperdlg
    svx/source/dialog/imapdlg
    svx/source/dialog/imapwnd
    svx/source/dialog/linkwarn
    svx/source/dialog/measctrl
    svx/source/dialog/optgrid
    svx/source/dialog/pagectrl
    svx/source/dialog/paraprev
    svx/source/dialog/passwd
    svx/source/dialog/relfld
    svx/source/dialog/rlrcitem
    svx/source/dialog/rubydialog
    svx/source/dialog/rulritem
    svx/source/dialog/SafeModeDialog
    svx/source/dialog/FileExportedDialog
    svx/source/dialog/SafeModeUI
    svx/source/dialog/SpellDialogChildWindow
    svx/source/dialog/srchctrl
    svx/source/dialog/srchdlg
    svx/source/dialog/strarray
    svx/source/dialog/svxbmpnumvalueset
    svx/source/dialog/svxgraphicitem
    svx/source/dialog/svxruler
    svx/source/dialog/swframeexample
    svx/source/dialog/swframeposstrings
    svx/source/dialog/ThemeColorsPaneBase
    svx/source/dialog/ThemeDialog
    svx/source/dialog/ThemeColorEditDialog
    svx/source/dialog/txencbox
    svx/source/dialog/txenctab
    svx/source/dialog/weldeditview
    svx/source/dialog/signaturelinehelper
    svx/source/dialog/TableAutoFmtDlg
    svx/source/dialog/TableStylesDlg
    svx/source/engine3d/float3d
    svx/source/fmcomp/dbaobjectex
    svx/source/form/databaselocationinput
    svx/source/form/dbcharsethelper
    svx/source/form/filtnav
    svx/source/form/fmobjfac
    svx/source/form/fmPropBrw
    svx/source/form/fmsrccfg
    svx/source/form/fmsrcimp
    svx/source/form/tabwin
    svx/source/form/tbxform
    svx/source/items/algitem
    svx/source/items/autoformathelper
    svx/source/items/ehdl
    svx/source/items/hlnkitem
    svx/source/items/numfmtsh
    svx/source/items/numinf
    svx/source/items/ofaitem
    svx/source/items/pageitem
    svx/source/items/postattr
    svx/source/items/rotmodit
    svx/source/items/SmartTagItem
    svx/source/items/statusitem
    svx/source/items/svxerr
    svx/source/items/viewlayoutitem
    svx/source/items/zoomslideritem
    svx/source/mnuctrls/clipboardctl
    svx/source/mnuctrls/smarttagmenu
    svx/source/sidebar/ContextChangeEventMultiplexer
    svx/source/sidebar/EmptyPanel
    svx/source/sidebar/inspector/InspectorTextPanel
    svx/source/sidebar/nbdtmg
    svx/source/sidebar/nbdtmgfact
    svx/source/sidebar/PanelFactory
    svx/source/sidebar/SelectionAnalyzer
    svx/source/sidebar/SelectionChangeHandler
    svx/source/sidebar/text/TextCharacterSpacingControl
    svx/source/sidebar/text/TextCharacterSpacingPopup
    svx/source/sidebar/text/TextUnderlineControl
    svx/source/sidebar/text/TextUnderlinePopup
    svx/source/sidebar/text/TextPropertyPanel
    svx/source/sidebar/styles/StylesPropertyPanel
    svx/source/sidebar/lists/ListsPropertyPanel
    svx/source/sidebar/paragraph/ParaLineSpacingControl
    svx/source/sidebar/paragraph/ParaLineSpacingPopup
    svx/source/sidebar/paragraph/ParaPropertyPanel
    svx/source/sidebar/paragraph/ParaSpacingWindow
    svx/source/sidebar/paragraph/ParaSpacingControl
    svx/source/sidebar/area/AreaPropertyPanel
    svx/source/sidebar/area/AreaPropertyPanelBase
    svx/source/sidebar/area/AreaTransparencyGradientPopup
    svx/source/sidebar/effect/EffectPropertyPanel
    svx/source/sidebar/effect/TextEffectPropertyPanel
    svx/source/sidebar/fontwork/FontworkPropertyPanel
    svx/source/sidebar/shadow/ShadowPropertyPanel
    svx/source/sidebar/graphic/GraphicPropertyPanel
    svx/source/sidebar/line/LinePropertyPanel
    svx/source/sidebar/line/LinePropertyPanelBase
    svx/source/sidebar/line/LineWidthValueSet
    svx/source/sidebar/line/LineWidthPopup
    # gb_Helper_optional(AVMEDIA, MediaPlaybackPanel) -- excluded,
    # HAVE_FEATURE_AVMEDIA is 0.
    svx/source/sidebar/possize/PosSizePropertyPanel
    svx/source/sidebar/shapes/DefaultShapesPanel
    svx/source/sidebar/shapes/ShapesUtil
    svx/source/sidebar/textcolumns/TextColumnsPropertyPanel
    svx/source/stbctrls/pszctrl
    svx/source/stbctrls/insctrl
    svx/source/stbctrls/selctrl
    svx/source/stbctrls/xmlsecctrl
    svx/source/stbctrls/modctrl
    svx/source/stbctrls/zoomsliderctrl
    svx/source/stbctrls/zoomctrl
    svx/source/svdraw/ActionDescriptionProvider
    svx/source/svdraw/MediaShellHelpers
    svx/source/smarttags/SmartTagMgr
    svx/source/table/accessiblecell
    svx/source/table/accessibletableshape
    svx/source/table/tabledesign
    svx/source/table/tablertfexporter
    svx/source/table/tablertfimporter
    svx/source/table/tablehtmlimporter
    svx/source/table/tableautofmt
    svx/source/table/tableautofmtpreview
    svx/source/table/tablestylesparser
    svx/source/tbxctrls/bulletsnumbering
    svx/source/tbxctrls/colrctrl
    svx/source/tbxctrls/SvxColorChildWindow
    svx/source/tbxctrls/SvxColorValueSet
    svx/source/tbxctrls/fillctrl
    svx/source/tbxctrls/formatpaintbrushctrl
    svx/source/tbxctrls/grafctrl
    svx/source/tbxctrls/itemwin
    svx/source/tbxctrls/layctrl
    svx/source/tbxctrls/lboxctrl
    svx/source/tbxctrls/linewidthctrl
    svx/source/tbxctrls/tbunocontroller
    svx/source/tbxctrls/tbunosearchcontrollers
    svx/source/tbxctrls/tbxcolor
    svx/source/tbxctrls/tbxdrctl
    svx/source/tbxctrls/ThemeColorsToolBoxControl
    svx/source/tbxctrls/verttexttbxctrl
    svx/source/uitest/uiobject
    svx/source/unodraw/unoctabl
    svx/source/unodraw/UnoNamespaceMap
    svx/source/unodraw/unopool
    svx/source/unodraw/unoshcol
    svx/source/unogallery/unogalitem
    svx/source/unogallery/unogaltheme
    svx/source/unogallery/unogalthemeprovider

    # ifeq ($(OS),WNT) -- advapi32 system library -- out of scope for this
    # Linux port.
)

librestudio_expand_cxx(SVX_SOURCES)
add_library(svx SHARED ${SVX_SOURCES})
add_dependencies(svx svx_sdi)

target_include_directories(svx PRIVATE
    "${CMAKE_SOURCE_DIR}/svx/inc"
    "${CMAKE_SOURCE_DIR}/svx/source/inc"
    "${CMAKE_SOURCE_DIR}/sc/inc"
    "${svx_sdi_INCLUDE_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(svx officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(svx PRIVATE SVX_DLLIMPLEMENTATION)

target_link_libraries(svx PRIVATE
    basegfx sb comphelper cppuhelper cppu dbtools docmodel drawinglayercore
    drawinglayer editeng fwk i18nlangtag i18nutil sal salhelper sfx sot svl
    svt svxcore tk tl ucbhelper utl vcl xo xmlscript sax
    Boost::boost
    ICU::uc)

# gb_Helper_optional(AVMEDIA, avmedia) -- svx (unlike svxcore) only links
# avmedia directly when AVMEDIA is enabled; HAVE_FEATURE_AVMEDIA is 0 here,
# so this link is omitted (svxcore still pulls it in transitively for the
# media item).
#
# gb_Library_add_componentimpls(svx, crashreport recoveryui) -- crashreport
# is gb_Helper_optional(BREAKPAD, ...) and excluded (LIBRESTUDIO_ENABLE_BREAKPAD
# is not defined); recoveryui applies unconditionally since
# ENABLE_WASM_STRIP_RECOVERYUI is unset.
librestudio_add_component(svx
    COMPONENT      "${CMAKE_SOURCE_DIR}/svx/util/svx.component"
    IMPLEMENTATION "${CMAKE_SOURCE_DIR}/svx/util/svx.component.recoveryui"
    LIBRARY        svx
    LAYER          OOO)

librestudio_set_library_output_name(svx svx OOO)

install(TARGETS svx LIBRARY DESTINATION program)
