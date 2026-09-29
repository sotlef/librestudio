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
#  - ENABLE_AVMEDIA (cmake/LibreStudioConfigHeaders.cmake, default ON --
#    configure.ac:1648-1651/3513-3515) gates the two gb_Helper_optional(AVMEDIA,
#    ...) entries each .mk has: svxcore's sdrmediawindow
#    (Library_svxcore.mk:263) and svx's MediaPlaybackPanel sidebar source
#    (Library_svx.mk:249) plus svx's own direct link to avmedia
#    (Library_svx.mk:49). svxcore links avmedia unconditionally regardless
#    (Library_svxcore.mk:57, not gated) for the media item itself, same
#    reasoning as editeng -> avmedia.
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

# gb_Helper_optional(AVMEDIA, svx/source/sdr/contact/sdrmediawindow) from
# Library_svxcore.mk:263.
if (ENABLE_AVMEDIA)
    list(APPEND SVXCORE_SOURCES svx/source/sdr/contact/sdrmediawindow)
endif (ENABLE_AVMEDIA)

librestudio_expand_cxx(SVXCORE_SOURCES)
add_library(svxcore SHARED ${SVXCORE_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(svxcore PRIVATE
        "${CMAKE_SOURCE_DIR}/svx/inc/pch/precompiled_svxcore.hxx")
    target_compile_definitions(svxcore PRIVATE PCH_LEVEL=2)
endif ()
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
    avmedia basegfx sb comphelper cppuhelper cppu docmodel
    drawinglayercore drawinglayer editeng fwk i18nlangtag i18nutil lng sal
    salhelper sax sfx sot svl svt tk tl ucbhelper utl vcl xo
    Boost::boost
    ICU::uc
    LibXml2::LibXml2)

# $(call gb_Helper_optional,DBCONNECTIVITY,dbtools) from Library_svxcore.mk's
# use_libraries.
if (ENABLE_DBCONNECTIVITY)
    target_link_libraries(svxcore PRIVATE dbtools)
endif (ENABLE_DBCONNECTIVITY)

# Library_svxcore.mk's ifeq ($(OS),MACOSX) branch (lines 96-106, re-checked
# directly against the live file): gb_Library_add_cxxflags(svxcore,
# $(gb_OBJCXXFLAGS)) + gb_Library_use_system_darwin_frameworks(svxcore,
# Foundation) -- no -lobjc here, unlike toolkit's own tk branch (Library_tk.mk
# additionally does gb_Library_add_libs(tk,-lobjc); svxcore's branch does
# not). gb_OBJCXXFLAGS is macosx.mk's own definition specifically (solenv/
# gbuild/platform/macosx.mk:59: "-x objective-c++ -fobjc-exceptions",
# confirmed directly), not iOS.mk's plain "-x objective-c++" (iOS.mk:60) --
# this .mk's guard is ifeq($(OS),MACOSX), never iOS. Same distinction this
# session already established for toolkit/CMakeLists.txt's tk (real macOS
# branch, CMAKE_SYSTEM_NAME STREQUAL "Darwin") vs cppuhelper's separate iOS
# branch (CMAKE_SYSTEM_NAME STREQUAL "iOS") -- CMAKE_SYSTEM_NAME STREQUAL
# "Darwin" is used here for the same reason: bare if(APPLE) is also true for
# iOS, which is the wrong branch for this exact ifeq($(OS),MACOSX) guard.
# Structurally present but inert on this Linux build, same status as every
# other iOS/Android/Windows branch in this port (see sal/CMakeLists.txt's own
# header comment).
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_options(svxcore PRIVATE -x objective-c++ -fobjc-exceptions)
    target_link_libraries(svxcore PRIVATE "-framework Foundation")
endif ()

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

# $(call gb_Helper_optional,DBCONNECTIVITY,svx/source/form/filtnav) from
# Library_svx.mk's add_exception_objects.
if (ENABLE_DBCONNECTIVITY)
    list(APPEND SVX_SOURCES svx/source/form/filtnav)
endif (ENABLE_DBCONNECTIVITY)

# $(call gb_Helper_optional,AVMEDIA,svx/source/sidebar/media/MediaPlaybackPanel)
# from Library_svx.mk:249's add_exception_objects.
if (ENABLE_AVMEDIA)
    list(APPEND SVX_SOURCES svx/source/sidebar/media/MediaPlaybackPanel)
endif (ENABLE_AVMEDIA)

librestudio_expand_cxx(SVX_SOURCES)
add_library(svx SHARED ${SVX_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(svx PRIVATE
        "${CMAKE_SOURCE_DIR}/svx/inc/pch/precompiled_svx.hxx")
    target_compile_definitions(svx PRIVATE PCH_LEVEL=2)
endif ()
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
    basegfx sb comphelper cppuhelper cppu docmodel drawinglayercore
    drawinglayer editeng fwk i18nlangtag i18nutil sal salhelper sfx sot svl
    svt svxcore tk tl ucbhelper utl vcl xo xmlscript sax
    Boost::boost
    ICU::uc)

# $(call gb_Helper_optional,DBCONNECTIVITY,dbtools) from Library_svx.mk's
# use_libraries.
if (ENABLE_DBCONNECTIVITY)
    target_link_libraries(svx PRIVATE dbtools)
endif (ENABLE_DBCONNECTIVITY)

# gb_Helper_optional(AVMEDIA, avmedia) from Library_svx.mk:49's
# use_libraries -- svx (unlike svxcore, which links avmedia unconditionally)
# only links avmedia directly when AVMEDIA is enabled.
if (ENABLE_AVMEDIA)
    target_link_libraries(svx PRIVATE avmedia)
endif (ENABLE_AVMEDIA)

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

# textconversiondlgs (Library_textconversiondlgs.mk) -- the Chinese simplified/
# traditional dictionary and translation dialogs. Instantiated directly by
# sc/sd/sw (rtl::Reference<textconversiondlgs::ChineseTranslation_UnoDialog>
# constructed straight from the header, no XMultiComponentFactory lookup), so
# unlike svxcore/svx it registers no UNO service and needs no .component file
# -- confirmed by grepping for a component_getFactory/component export in its
# three sources (none) and for any *.component referencing it anywhere in the
# tree (none).
set(TEXTCONVERSIONDLGS_SOURCES
    svx/source/unodialogs/textconversiondlgs/chinese_dictionarydialog
    svx/source/unodialogs/textconversiondlgs/chinese_translationdialog
    svx/source/unodialogs/textconversiondlgs/chinese_translation_unodialog
)

librestudio_expand_cxx(TEXTCONVERSIONDLGS_SOURCES)
add_library(textconversiondlgs SHARED ${TEXTCONVERSIONDLGS_SOURCES})

target_include_directories(textconversiondlgs PRIVATE
    "${CMAKE_SOURCE_DIR}/svx/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(textconversiondlgs udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(textconversiondlgs PRIVATE TEXTCONVERSIONDLGS_DLLIMPLEMENTATION)

target_link_libraries(textconversiondlgs PRIVATE
    comphelper cppuhelper cppu sal i18nlangtag svl svt tk tl utl vcl
    Boost::boost)

librestudio_set_library_output_name(textconversiondlgs textconversiondlgs OOO)

install(TARGETS textconversiondlgs LIBRARY DESTINATION program)

# --- Package_document_themes ----------------------------------------------------
#
# Package_document_themes.mk: 7 .theme files under svx/uiconfig/themes/
# packaged into $(LIBO_SHARE_FOLDER)/themes -- LIBO_SHARE_FOLDER is plain
# "share" on this port (see UIConfig_svt's own comment in svtools/
# CMakeLists.txt, and framework/CMakeLists.txt's/sfx2/CMakeLists.txt's own
# "share/..." installs, for the same established value). Plain install(FILES
# ...), same mechanism as every other Package_*.mk ported so far.
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/themes/Beach.theme"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/themes/Breeze.theme"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/themes/Forest.theme"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/themes/Libreoffice.theme"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/themes/Ocean.theme"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/themes/Rainbow.theme"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/themes/Sunset.theme"
    DESTINATION share/themes)

# --- Package_xml -----------------------------------------------------------------
#
# Package_xml.mk: 1 file (svx/source/table/tablestyles.xml) packaged into
# $(LIBO_SHARE_FOLDER)/svx -- same "share" root as above.
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/source/table/tablestyles.xml"
    DESTINATION share/svx)

# --- UIConfig_svx ------------------------------------------------------------
#
# UIConfig_svx.mk: 143 .ui files under svx/uiconfig/ui/ packaged into
# share/config/soffice.cfg/svx/ui -- the same gb_UIConfig_DIR_FOR-derived
# destination convention already established for UIConfig_svt (svtools/
# CMakeLists.txt) and UIConfig_vcl (vcl/CMakeLists.txt): a plain install(FILES
# ...) naming every .ui file directly, DESTINATION swapped to .../svx/ui.
# Verified against the live UIConfig_svx.mk: exactly 143 add_uifiles entries,
# and `find svx/uiconfig/ui -name "*.ui" | wc -l` also returns 143 -- every
# named file exists on disk, nothing invented.
#
# SVX_UICONFIG_UI_FILES holds the list once so both the packaging install()
# below and the real build-time mirror right after it stay in sync by
# construction -- no prior variable existed here to reuse (this block used to
# inline the list straight into install(FILES ...)); introduced now the same
# way vcl/CMakeLists.txt's own UIConfig_vcl block introduces
# VCL_UICONFIG_UI_FILES for the identical reason (see that file's comment).
set(SVX_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/absrecbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/acceptrejectchangesdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/accessibilitycheckentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/accessibilitychecklevel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/addconditiondialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/adddataitemdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/addinstancedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/addmodeldialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/addnamespacedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/addsubmissiondialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/asianphoneticguidedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/applystylebox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/cellmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/charsetmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/checkbuttonbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/chineseconversiondialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/chinesedictionary.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/classificationdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/clipboardmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/colorwindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/currencywindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/colsmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/columnswindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/compressgraphicdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/crashreportdlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/datanavigator.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/defaultshapespanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/deleteheaderdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/deletefooterdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/depthwindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/directionwindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/docking3deffects.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dockingcolorreplace.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dockingcolorwindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dockingfontwork.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/docrecoverybrokendialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/docrecoveryprogressdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/docrecoveryrecoverdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/docrecoverysavedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/extrustiondepthdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/fillctrlbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/filtermenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/filternavigator.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/findbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/findreplacedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/floatingareastyle.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/floatingcontour.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/floatingframeborder.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/floatinglineend.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/floatinglineproperty.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/floatinglinestyle.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/floatingundoredo.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/fontworkalignmentcontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/fontworkcharacterspacingcontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/fontworkgallerydialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/fontworkspacingdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/fontsizebox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/fontnamebox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/formdatamenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/formfielddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/formlinkwarndialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/formnavigator.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/formnavimenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/formpropertydialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/functionmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/gallerymenu1.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/gallerymenu2.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/genericcheckdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/genericcheckentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/gotopagedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/grafctrlbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/grafmodebox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/headfootformatpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/imapdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/imapmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/inspectortextpanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/labelbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/lightingwindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/linkwarndialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/measurewidthbar.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/medialine.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/mediaplayback.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/mediawindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/metricfieldbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/namespacedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/navigationbar.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/numberingwindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/optgridpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/paralinespacingcontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/paralrspacing.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/paraulspacing.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/passwd.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/presetmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/fileexporteddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querydeletecontourdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querydeleteobjectdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querydeletethemedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querymodifyimagemapchangesdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querynewcontourdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querysavecontchangesdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querysaveimagemapchangesdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/queryunlinkgraphicsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/redlinecontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/redlinefilterpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/redlineviewpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/rowsmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/rulermenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/safemodedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/savemodifieddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/selectionmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebararea.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebarempty.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebareffect.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebartexteffect.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebarfontwork.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebarshadow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebargallery.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebargraphic.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebarline.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebarparagraph.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebarlists.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebarpossize.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebarstylespanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebartextcolumnspanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/sidebartextpanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/stylemenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/surfacewindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tablewindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/stylespreview.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/textcharacterspacingcontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/textcontrolchardialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/textcontrolparadialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/textunderlinecontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/themedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/themecoloreditdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/themeselectorpanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/toolbarpopover.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/xmlsecstatmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/xformspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/zoommenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tableautofmtdlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tablestylesdlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/stringinput.ui"
)

install(FILES ${SVX_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/svx/ui)

# Real build-time mirror of the install() rule above, alongside it (not
# instead of it) -- same rationale, and the same per-file add_custom_command
# + one add_custom_target(... ALL DEPENDS ...) shape, as vcl/CMakeLists.txt's
# own UIConfig_vcl block; see that file's comment for the full reasoning this
# is not repeated here.
set(_svx_uiconfig_outputs "")
foreach (_svx_uiconfig_src IN LISTS SVX_UICONFIG_UI_FILES)
    get_filename_component(_svx_uiconfig_name "${_svx_uiconfig_src}" NAME)
    set(_svx_uiconfig_dest "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/svx/ui/${_svx_uiconfig_name}")
    add_custom_command(
        OUTPUT "${_svx_uiconfig_dest}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/svx/ui"
        COMMAND ${CMAKE_COMMAND} -E copy "${_svx_uiconfig_src}" "${_svx_uiconfig_dest}"
        DEPENDS "${_svx_uiconfig_src}"
        COMMENT "Copying ${_svx_uiconfig_name} into build-tree soffice.cfg/svx/ui"
        VERBATIM)
    list(APPEND _svx_uiconfig_outputs "${_svx_uiconfig_dest}")
endforeach ()
unset(_svx_uiconfig_src)
unset(_svx_uiconfig_name)
unset(_svx_uiconfig_dest)

add_custom_target(svx_uiconfig ALL DEPENDS ${_svx_uiconfig_outputs})
unset(_svx_uiconfig_outputs)

# --- Executable_gengal + Package_gengal -----------------------------------------
#
# Module_svx.mk's exact gating (re-checked directly against the live file):
#   ifneq (,$(filter DESKTOP,$(BUILD_TYPE)))
#   ifeq (,$(DISABLE_DYNLOADING))
#     Executable_gengal + $(if $(filter-out MACOSX WNT,$(OS)), Package_gengal)
#   else ($(WITH_GALLERY_BUILD),TRUE): Executable_gengal, build-side only
#   endif
# BUILD_TYPE contains DESKTOP unconditionally on this port (HAVE_FEATURE_
# DESKTOP, same reasoning cpputools/CMakeLists.txt's own uno.sh install
# already applies -- "gb_CondExeUno ... both hold unconditionally on this
# port"), and DISABLE_DYNLOADING is never defined anywhere in this port's
# config surface (grepped; same "never set" conclusion unoidl/CMakeLists.txt
# already reached for its own DISABLE_DYNLOADING reasoning). So the first
# branch applies unconditionally: Executable_gengal AND (OS is Linux, matched
# by $(filter-out MACOSX WNT,$(OS))) Package_gengal both apply, always, on
# this port -- the $(WITH_GALLERY_BUILD) build-side-only branch never
# applies here.
#
# Executable_gengal.mk, re-checked directly: 1 source (svx/source/gengal/
# gengal), use_external(boost_headers), use_sdk_api (normal udkapi+offapi),
# use_libraries(basegfx sal tl sfx svl svt comphelper cppu cppuhelper utl
# svxcore vcl), use_vclmain, ifeq($(OS),WNT) use_system_win32_libs(kernel32
# msvcrt oldnames user32) -- kept structurally present but inert, same
# if(WIN32) pattern already used for cpputools' sp2bv and i18nlangtag's own
# kernel32 branch -- and add_default_nativeres (a Windows icon/version
# resource, omitted outright, same as opencl/CMakeLists.txt's opencltest:
# "not applicable on this Linux-only port, so omitted").
#
# gengal.cxx calls ::cppu::defaultBootstrap_InitialComponentContext()
# directly, so it dlopens the UNO bridge at run time the same way cpputools'
# uno / vcl's icontest and visualbackendtest do -- gcc3_uno is a real
# add_dependencies() build-order prerequisite here for the same reason (see
# bridges/CMakeLists.txt's own note on this dlopen-not-link pattern).
# vclmain is vcl/CMakeLists.txt's real STATIC library target (vcl is
# add_subdirectory()'d before svx), providing the SAL_IMPLEMENT_MAIN() entry
# point gengal's own gengal.cxx relies on instead of defining its own main().
set(GENGAL_SOURCES
    svx/source/gengal/gengal
)
librestudio_expand_cxx(GENGAL_SOURCES)
add_executable(gengal ${GENGAL_SOURCES})

target_include_directories(gengal PRIVATE
    "${CMAKE_SOURCE_DIR}/svx/inc"
    "${CMAKE_SOURCE_DIR}/svx/inc/pch"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(gengal udkapi_normal_headers offapi_normal_headers gcc3_uno)

target_link_libraries(gengal PRIVATE
    basegfx sal tl sfx svl svt comphelper cppu cppuhelper utl svxcore vcl
    vclmain
    Boost::boost)

# ifeq ($(OS),WNT) gb_Executable_use_system_win32_libs(gengal,kernel32
# msvcrt oldnames user32) -- kept structurally present but inert, per this
# port's policy of not silently dropping Windows-only gbuild targets (see
# cpputools/CMakeLists.txt's sp2bv and i18nlangtag's kernel32 branch for the
# same pattern). Never exercised on this Linux build.
if (WIN32)
    target_link_libraries(gengal PRIVATE kernel32 msvcrt oldnames user32)
endif ()

install(TARGETS gengal RUNTIME DESTINATION program)

# Package_gengal.mk: installs svx/source/gengal/gengal.sh, renamed to
# "gengal", into $(LIBO_BIN_FOLDER) (program/) -- gated the same way as the
# Executable above ($(filter-out MACOSX WNT,$(OS)), true on this Linux port).
# Same shape as cpputools/CMakeLists.txt's own uno.sh -> "uno" install
# (install(PROGRAMS ... RENAME <name>)), the one existing precedent in this
# port for a gbuild Executable+wrapper-script pair; gengal.sh's own exec line
# ("exec \"$0.bin\" \"$@\" ...") expects the real compiled binary to be found
# as "<self>.bin" once invoked as the renamed wrapper -- exactly the same
# assumption cpputools' uno.sh makes for its own "$0.bin" exec, and which
# that existing CMakeLists.txt does not (yet) reconcile with the plain
# install(TARGETS uno RUNTIME DESTINATION program) sitting right next to it
# (both install to program/uno, unrenamed): whichever install() rule runs
# last wins that path, so the wrapper's own "$0.bin" indirection is not
# actually load-bearing as either module is wired today. Reproduced here
# unchanged, for consistency with that one precedent, rather than inventing
# a different, untested convention just for gengal; not silently presented
# as verified end-to-end (this port has never run either wrapper script).
install(PROGRAMS "${CMAKE_SOURCE_DIR}/svx/source/gengal/gengal.sh"
    DESTINATION program RENAME gengal)

# --- CppunitTest_svx_removewhichrange --------------------------------------------
#
# CppunitTest_svx_removewhichrange.mk, re-checked directly: 1 source (svx/qa/
# unit/removewhichrange), use_externals(boost_headers icuuc libxml2),
# use_sdk_api (normal udkapi+offapi), gb_CppunitTest_use_library_objects(
# svx_removewhichrange, svxcore) -- NOT use_libraries(svxcore) -- plus a full
# use_libraries list matching svxcore's own transitive deps, plus its own
# ifeq($(OS),MACOSX) use_system_darwin_frameworks(Foundation) block mirroring
# svxcore's own. No use_ure/use_vcl/use_rdb/use_configuration anywhere in
# this .mk, so no PROTECTORS/CONFIGURATION and no ordering constraint against
# any module add_subdirectory()'d after svx -- wired directly here.
#
# use_library_objects(svx_removewhichrange, svxcore) links svxcore's already-
# compiled object files directly into the test binary rather than dynamically
# against libsvxcore.so. Upstream needs this because the one symbol the test
# calls, RemoveWhichRange (declared in include/svx/svdetc.hxx with no
# SVXCORE_DLLPUBLIC export marker -- confirmed by reading the header
# directly), is not part of svxcore's exported ABI there. sal/CMakeLists.txt's
# own sal_osl_file_details hit the identical gb_CppunitTest_use_library_objects
# shape and modelled it by recompiling the WHOLE target library's source list
# a second time directly into the test binary (see that comment) -- but that
# was necessary there because sal really does restrict its exported symbols
# with a linker version script on this port too
# (-Wl,--version-script=sal/util/sal.map, see sal/CMakeLists.txt), so
# isForbidden() genuinely is unreachable through libsal.so as this port
# builds it.
#
# svxcore has no equivalent restriction: no svx/util/*.map (or any *.map)
# file exists anywhere under svx/, target_link_libraries(svxcore ...) applies
# no -Wl,--version-script (grepped svx/CMakeLists.txt directly, confirmed
# above in svxcore's own block), and this port sets no -fvisibility=hidden
# anywhere at all (grepped cmake/*.cmake and the root CMakeLists.txt, no
# matches) -- GCC's plain default visibility applies, which exports every
# global symbol, RemoveWhichRange included, regardless of the missing
# SVXCORE_DLLPUBLIC marker upstream's own stricter Windows-style export
# list relies on. So unlike sal_osl_file_details, this test's one missing
# symbol IS already reachable by linking against the real svxcore *library*
# as this port actually builds it -- recompiling svxcore's ~400-file source
# list a second time into this one small test would be real, unnecessary
# extra build cost with no functional benefit here. Modelled as a plain
# LIBRARIES svxcore instead; flagged here rather than silently presented as
# identical to sal_osl_file_details's heavier precedent.
#
# Fix (first real `cmake --build`, 2026-09-27): this LIBRARIES list was
# missing basegfx/comphelper/cppu/cppuhelper/docmodel/drawinglayer/
# drawinglayercore/editeng/fwk/i18nlangtag/i18nutil/sal/salhelper/sax/sb/
# sfx/sot/svl/svt/tk/tl/ucbhelper/utl/vcl/xo -- the comment above claimed "a
# full use_libraries list matching svxcore's own transitive deps" but the
# code only ever had "svxcore" alone. svxcore links all of these PRIVATE
# (svx/CMakeLists.txt), which does not propagate to a consumer that only
# names the shared library itself -- svl's WhichRangesContainer (include/
# svl/whichranges.hxx, the very type this test exercises) was consequently
# "undefined reference" at link time despite being reachable at runtime
# through svxcore's own dependency chain. Also adds avmedia/dbtools, the
# .mk's own gb_Helper_optional(AVMEDIA,...)/gb_Helper_optional(
# DBCONNECTIVITY,...) entries (both ENABLE_AVMEDIA and ENABLE_DBCONNECTIVITY
# default ON in this port).
librestudio_add_cppunit_test(svx_removewhichrange
    SOURCES
        svx/qa/unit/removewhichrange
    LIBRARIES
        svxcore basegfx comphelper cppu cppuhelper docmodel drawinglayer
        drawinglayercore editeng fwk i18nlangtag i18nutil sal salhelper sax
        sb sfx sot svl svt tk tl ucbhelper utl vcl xo
        Boost::boost ICU::uc LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/svx/source/inc"
        "${CMAKE_SOURCE_DIR}/svx/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers)

if (ENABLE_AVMEDIA)
    target_link_libraries(svx_removewhichrange PRIVATE avmedia)
endif ()
if (ENABLE_DBCONNECTIVITY)
    target_link_libraries(svx_removewhichrange PRIVATE dbtools)
endif ()

# ifeq ($(OS),MACOSX) gb_CppunitTest_use_system_darwin_frameworks(
# svx_removewhichrange, Foundation) -- mirrors svxcore's own macOS branch
# above; structurally present but inert on this Linux build.
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(svx_removewhichrange PRIVATE "-framework Foundation")
endif ()

# CppunitTest_svx_unit, CppunitTest_svx_gallery_test and CppunitTest_svx_core
# all need unotest and/or test_fixtures and/or subsequenttest (test/
# CMakeLists.txt, unotest/CMakeLists.txt), both add_subdirectory()'d AFTER
# svx in the root CMakeLists.txt, plus use_rdb(services) needs
# program_services_rdb (defined much later still, right after
# librestudio_finalize_components(services)) -- so all three are deferred
# into svx/CppunitTest_deferred.cmake, include()'d from the root
# CMakeLists.txt once program_services_rdb exists, the same point scaddins_
# analysis/xmlscript_cppunit/vcl's/svtools_graphic's own use_rdb(services)
# tests are already deferred to. See that file for the full per-test detail.
#
# CppunitTest_svx_dialogs_test (a gb_CppunitTest_CppunitScreenShot screenshot
# target calling use_vcl_non_headless_with_windows) is RESOLVED: previously
# NOT wired here, or anywhere, because UIConfig_svx existing as an
# install(FILES ...) rule alone did not unblock it -- the real gap was
# librestudio_add_cppunit_test() having no HEADLESS-toggle/use_uiconfigs
# equivalent at all (see svx/CppunitTest_deferred.cmake's own now-updated
# header comment for the full detail). cmake/LibreStudioTest.cmake's
# NONHEADLESS and UICONFIG options close that gap, and UIConfig_svx above now
# also defines a real build-time mirror (svx_uiconfig) for UICONFIG to depend
# on, so this test is wired in svx/CppunitTest_deferred.cmake for real.

# svx/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/svx/CppunitTest_deferred.cmake")
