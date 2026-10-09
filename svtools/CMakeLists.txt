# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# svtools — the shared GUI helper layer built on top of vcl and tk: the
# browse-box grid, colour/HTML/accelerator configuration, the RTF/HTML mini
# parsers, and a grab-bag of UNO services (file picker, popup menu and status
# bar controllers, the generic wizard shell) that other UI modules build on.
# Source list from Library_svt.mk. Java-only sources (javacontext,
# javainteractionhandler) are skipped: LIBRESTUDIO_ENABLE_JAVA is OFF. Also
# ported here: UIConfig_svt's .ui packaging, Executable_langsupport, and
# CppunitTest_svtools_html; CppunitTest_svtools_graphic is deferred to
# svtools/CppunitTest_deferred.cmake, and CppunitTest_svtools_dialogs_test
# (a screenshot target) is not ported at all -- see the comments at each
# target below.

set(SVTOOLS_SOURCES
    svtools/source/brwbox/accessibleeditbrowseboxcell
    svtools/source/brwbox/brwbox1
    svtools/source/brwbox/brwbox2
    svtools/source/brwbox/brwbox3
    svtools/source/brwbox/brwhead
    svtools/source/brwbox/datwin
    svtools/source/brwbox/ebbcontrols
    svtools/source/brwbox/editbrowsebox
    svtools/source/brwbox/editbrowsebox2
    svtools/source/brwbox/recorditemwindow
    svtools/source/config/accessibilityoptions
    svtools/source/config/apearcfg
    svtools/source/config/colorcfg
    svtools/source/config/extcolorcfg
    svtools/source/config/fontsubstconfig
    svtools/source/config/htmlcfg
    svtools/source/config/itemholder2
    svtools/source/config/miscopt
    svtools/source/config/optionsdrawinglayer
    svtools/source/config/printoptions
    svtools/source/contnr/querydelete
    svtools/source/config/viewoptions
    svtools/source/control/accessibleruler
    svtools/source/control/accessibletabbar
    svtools/source/control/accessibletabbarbase
    svtools/source/control/accessibletabbarpage
    svtools/source/control/accessibletabbarpagelist
    svtools/source/control/asynclink
    svtools/source/control/collatorres
    svtools/source/control/ctrlbox
    svtools/source/control/ctrltool
    svtools/source/control/indexentryres
    svtools/source/control/inettbc
    svtools/source/control/ruler
    svtools/source/control/scriptedtext
    svtools/source/control/scrolladaptor
    svtools/source/control/tabbar
    svtools/source/control/toolbarmenu
    svtools/source/control/valueacc
    svtools/source/control/valueset
    svtools/source/dialogs/addresstemplate
    svtools/source/dialogs/dlgname
    svtools/source/dialogs/insdlg
    svtools/source/dialogs/PlaceEditDialog
    svtools/source/dialogs/prnsetup
    svtools/source/dialogs/querydialog
    svtools/source/dialogs/restartdialog
    svtools/source/dialogs/ServerDetailsControls
    svtools/source/filter/SvFilterOptionsDialog
    svtools/source/filter/DocumentToGraphicRenderer
    svtools/source/filter/exportdialog
    svtools/source/graphic/renderer
    svtools/source/misc/acceleratorexecute
    svtools/source/misc/bindablecontrolhelper
    svtools/source/misc/cliplistener
    svtools/source/misc/dialogclosedlistener
    svtools/source/misc/ehdl
    svtools/source/misc/embedhlp
    svtools/source/misc/embedtransfer
    svtools/source/misc/filechangedchecker
    svtools/source/misc/imagemgr
    svtools/source/misc/imageresourceaccess
    svtools/source/misc/langtab
    svtools/source/misc/langhelp
    svtools/source/misc/openfiledroptargetlistener
    svtools/source/misc/sampletext
    svtools/source/misc/stringtransfer
    svtools/source/misc/svtresid
    svtools/source/misc/templatefoldercache
    svtools/source/misc/unitconv
    svtools/source/svhtml/htmlkywd
    svtools/source/svhtml/htmlout
    svtools/source/svhtml/htmlsupp
    svtools/source/svhtml/HtmlWriter
    svtools/source/svhtml/parhtml
    svtools/source/svrtf/parrtf
    svtools/source/svrtf/rtfkeywd
    svtools/source/svrtf/rtfout
    svtools/source/svrtf/svparser
    svtools/source/uno/addrtempuno
    svtools/source/uno/fpicker
    svtools/source/uno/framestatuslistener
    svtools/source/uno/genericunodialog
    svtools/source/uno/miscservices
    svtools/source/uno/popupmenucontrollerbase
    svtools/source/uno/popupwindowcontroller
    svtools/source/uno/statusbarcontroller
    svtools/source/uno/toolboxcontroller
    svtools/source/uno/unoevent
    svtools/source/uno/unoimap
    svtools/source/uno/wizard/unowizard
    svtools/source/uno/wizard/wizardpagecontroller
    svtools/source/uno/wizard/wizardshell
    svtools/source/uitest/uiobject
)

librestudio_expand_cxx(SVTOOLS_SOURCES)
add_library(svt SHARED ${SVTOOLS_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(svt PRIVATE
        "${CMAKE_SOURCE_DIR}/svtools/inc/pch/precompiled_svt.hxx")
    target_compile_definitions(svt PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(svt PRIVATE
    "${CMAKE_SOURCE_DIR}/svtools/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(svt officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(svt PRIVATE SVT_DLLIMPLEMENTATION)
target_link_libraries(svt PRIVATE
    basegfx comphelper cppu cppuhelper drawinglayercore i18nlangtag i18nutil
    salhelper sal sot svl tk tl ucbhelper utl vcl
    Boost::boost
    ICU::uc ICU::i18n
    LibXml2::LibXml2)

# Library_svt.mk's platform tail (verified against the live file):
#   ifeq ($(OS),WNT)
#   $(eval $(call gb_Library_use_system_win32_libs,svt,\
#       advapi32 \
#       gdi32 \
#       ole32 \
#       oleaut32 \
#       uuid \
#   ))
#   endif
# Not exercised on this Linux port; wired for completeness only.
if (WIN32)
    target_link_libraries(svt PRIVATE advapi32 gdi32 ole32 oleaut32 uuid)
endif ()

librestudio_add_component(svt
    COMPONENT "${CMAKE_SOURCE_DIR}/svtools/util/svt.component"
    LIBRARY   svt
    LAYER     OOO)

librestudio_set_library_output_name(svt svt OOO)
install(TARGETS svt LIBRARY DESTINATION program)

# --- UIConfig_svt ------------------------------------------------------------
#
# UIConfig_svt.mk: 30 .ui files under svtools/uiconfig/ui packaged into
# share/config/soffice.cfg/svt/ui -- gb_UIConfig_UIConfig's own
# destination convention (solenv/gbuild/UIConfig.mk: gb_UIConfig_DIR_FOR
# expands to $(INSTROOT)/$(LIBO_SHARE_FOLDER)/config/soffice.cfg/$(2)/ui,
# LIBO_SHARE_FOLDER being plain "share" on this port -- see officecfg/
# CMakeLists.txt's OFFICECFG_SHARE_FOLDER for the same "share" value already
# established here). No prior UIConfig_*/soffice.cfg packaging precedent
# exists yet anywhere else in this tree (checked: no other install(FILES ...)
# targets a soffice.cfg destination), so this is the first, following that
# same gb_UIConfig_DIR_FOR-derived pattern directly.
#
# SVTOOLS_UICONFIG_UI_FILES holds the list once so both the packaging
# install() below and the real build-time mirror right after it stay in sync
# by construction -- no prior variable existed here to reuse (this block used
# to inline the list straight into install(FILES ...)); introduced now the
# same way vcl/CMakeLists.txt's own UIConfig_vcl block introduces
# VCL_UICONFIG_UI_FILES for the identical reason (see that file's comment).
set(SVTOOLS_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/addresstemplatedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/calendar.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/checkboxcontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/combocontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/datewindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/editcontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/emptypage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/fileviewmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/fixedimagecontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/fixedtextcontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/graphicexport.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/interimparent.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/inputbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/javadisableddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/listcontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/linewindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/managedtoolbar.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/placeedit.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/printersetupdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querydeletedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/querydialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/restartdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/scrollbars.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/spinfieldcontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/subtoolbar.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tabbuttons.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tabbuttonsmirrored.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/tabbaredit.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/textviewcontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/thineditcontrol.ui"
)

install(FILES ${SVTOOLS_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/svt/ui)

# Real build-time mirror of the install() rule above, alongside it (not
# instead of it) -- same rationale, and the same per-file add_custom_command
# + one add_custom_target(... ALL DEPENDS ...) shape, as vcl/CMakeLists.txt's
# own UIConfig_vcl block; see that file's comment for the full reasoning this
# is not repeated here. Named svt_uiconfig, not svtools_uiconfig, to match
# the module name CppunitTest_svtools_dialogs_test.mk's own
# gb_CppunitTest_use_uiconfigs(svtools_dialogs_test,svt) call passes -- "svt"
# is this module's real name throughout this port (its production library
# target itself is "svt", not "svtools"; see librestudio_set_library_output_
# name(svt svt OOO) above).
set(_svt_uiconfig_outputs "")
foreach (_svt_uiconfig_src IN LISTS SVTOOLS_UICONFIG_UI_FILES)
    get_filename_component(_svt_uiconfig_name "${_svt_uiconfig_src}" NAME)
    set(_svt_uiconfig_dest "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/svt/ui/${_svt_uiconfig_name}")
    add_custom_command(
        OUTPUT "${_svt_uiconfig_dest}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/svt/ui"
        COMMAND ${CMAKE_COMMAND} -E copy "${_svt_uiconfig_src}" "${_svt_uiconfig_dest}"
        DEPENDS "${_svt_uiconfig_src}"
        COMMENT "Copying ${_svt_uiconfig_name} into build-tree soffice.cfg/svt/ui"
        VERBATIM)
    list(APPEND _svt_uiconfig_outputs "${_svt_uiconfig_dest}")
endforeach ()
unset(_svt_uiconfig_src)
unset(_svt_uiconfig_name)
unset(_svt_uiconfig_dest)

add_custom_target(svt_uiconfig ALL DEPENDS ${_svt_uiconfig_outputs})
unset(_svt_uiconfig_outputs)

# --- Executable_langsupport ---------------------------------------------------
#
# Module_svtools.mk: ifeq ($(CROSS_COMPILING)$(DISABLE_DYNLOADING),) -- both
# CROSS_COMPILING and DISABLE_DYNLOADING are unset on this port (neither
# variable is ever set anywhere in this tree, so the make ifeq's concatenated
# test is the empty string, unconditionally true here), and
# ifneq ($(OS),WNT) also holds unconditionally on this Linux-only port -- so
# Executable_langsupport.mk is built unconditionally here too.
#
# Executable_langsupport.mk: 1 source (svtools/langsupport/langsupport),
# use_sdk_api (udkapi+offapi normal headers), use_external(boost_headers),
# use_libraries comphelper cppu cppuhelper i18nlangtag sal svt tl ucbhelper
# vcl. set_include adds svtools/inc, already covered by target_include_
# directories below (mirrors svt's own PRIVATE include, safe to repeat here
# since this executable's own translation unit needs the same headers).
add_executable(langsupport
    "${CMAKE_CURRENT_SOURCE_DIR}/langsupport/langsupport.cxx")

target_include_directories(langsupport PRIVATE
    "${CMAKE_SOURCE_DIR}/svtools/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(langsupport udkapi_normal_headers offapi_normal_headers)

target_link_libraries(langsupport PRIVATE
    comphelper cppu cppuhelper i18nlangtag sal svt tl ucbhelper vcl
    Boost::boost)

install(TARGETS langsupport RUNTIME DESTINATION program)

# --- CppunitTest_svtools_html --------------------------------------------------
#
# CppunitTest_svtools_html.mk: 2 sources (svtools/qa/unit/testHtmlReader,
# testHtmlWriter), use_external(boost_headers), use_sdk_api (udkapi+offapi
# normal headers), use_libraries comphelper cppu cppuhelper tl sal svt -- no
# use_ure/use_vcl*/use_rdb/use_configuration, so no PROTECTORS/CONFIGURATION,
# and no ordering constraint against any module add_subdirectory()'d after
# svtools -- wired directly here rather than deferred.
librestudio_add_cppunit_test(svtools_html
    SOURCES
        svtools/qa/unit/testHtmlReader
        svtools/qa/unit/testHtmlWriter
    LIBRARIES comphelper cppu cppuhelper tl sal svt Boost::boost
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers)

# CppunitTest_svtools_graphic (needs sw and program_services_rdb, both
# add_subdirectory()'d/defined well after svtools) and CppunitTest_svtools_
# dialogs_test (a screenshot target -- RESOLVED, see below) are in svtools/
# CppunitTest_deferred.cmake, include()'d from the root CMakeLists.txt after
# those dependencies exist.
#
# CppunitTest_svtools_dialogs_test.mk RESOLVED: previously left unwired here
# ("a screenshot target, deliberately not ported", the same category of gap
# as every other module's own "*_dialogs_test screenshot target" -- uui,
# fpicker, desktop, vcl) because it is a gb_CppunitTest_CppunitScreenShot
# calling use_vcl_non_headless_with_windows, real non-headless windowing this
# port's librestudio_add_cppunit_test() had no equivalent for. It now does
# (NONHEADLESS + UICONFIG, cmake/LibreStudioTest.cmake), and this module's
# own UIConfig_svt block just above now also has a real build-time mirror
# (svt_uiconfig) for UICONFIG to depend on -- see svtools/CppunitTest_
# deferred.cmake's own header comment for the full per-test wiring.

# svtools/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/svtools/CppunitTest_deferred.cmake")
