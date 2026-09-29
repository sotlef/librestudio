# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# extensions — of the module's 14 Library_*.mk files, 8 have a dependency
# closure that is fully satisfiable on this Linux CMake port and are built
# below; the other 6 need externals or platforms that are not available here
# and are skipped for now (see connectivity/CMakeLists.txt and
# filter/CMakeLists.txt for the same scoping approach elsewhere in this
# port):
#
# Included:
#  - pcr    (Library_pcr.mk)    — the property browser/inspector ("Property
#    Browser Controller") used by the form-control property dialogs across
#    the whole suite. Depends only on svx, svxcore, editeng, sfx, svt, tk,
#    vcl, svl, utl, tl, dbtools, i18nlangtag, comphelper, cppuhelper, cppu,
#    sal, salhelper and boost_headers — all already ported.
#  - abp    (Library_abp.mk)    — the address book source pilot/wizard
#    (extensions/source/abpilot). Same kind of dependency set as pcr, all
#    ported (comphelper cppuhelper cppu sal sfx svl svt svxcore svx tk tl
#    utl vcl i18nlangtag, boost_headers).
#  - bib    (Library_bib.mk)    — the bibliography database component
#    (extensions/source/bibliography). Depends on dbtools sfx sot svt svl
#    tk vcl utl tl comphelper cppuhelper cppu sal salhelper i18nlangtag —
#    sot is ported (see sot/CMakeLists.txt) along with the rest.
#  - dbp    (Library_dbp.mk)    — the database form-control pilots/wizards
#    (extensions/source/dbpilots: grid/group box/list-combo autopilots).
#    Depends on svt vcl tl tk svl sfx dbtools utl comphelper cppuhelper
#    cppu sal salhelper i18nlangtag — all ported.
#  - log    (Library_log.mk)    — the com.sun.star.logging UNO service
#    implementation (extensions/source/logging). Trivial dependency set:
#    comphelper cppuhelper cppu sal tl, plus boost_headers.
#  - updatefeed (Library_updatefeed.mk) — parses the Atom/RSS update-
#    information feed (extensions/source/update/feed). Depends only on
#    cppuhelper cppu sal vcl, plus boost_headers.
#  - updchk (Library_updchk.mk) — the online update-check UNO service
#    (extensions/source/update/check). Depends on comphelper cppuhelper
#    cppu sal salhelper tk utl vcl (all ported) and the externals
#    boost_headers and curl. curl is already wired at the top level
#    (CURL::libcurl / find_package(CURL REQUIRED) in the root
#    CMakeLists.txt, already used by svl, linguistic and ucb/ucpdav1), so
#    this one is portable. Only the OS-neutral exception objects are built
#    (download, updatecheck, updatecheckconfig, updatecheckjob, updatehdl,
#    updateprotocol); onlinecheck.cxx is guarded by
#    "ifeq($(OS),WNT)"/"#ifdef _WIN32" in both the .mk and the source
#    itself (it talks to wininet.h) and is excluded on this Linux build.
#    download.cxx pulls in include/systools/curlinit.hxx, which itself
#    needs the generated officecfg::Office::Security schema header —
#    matching Library_updchk.mk's own
#    gb_Library_use_custom_headers(updchk, officecfg/registry) — so
#    officecfg_headers/OFFICECFG_INCLUDE_DIR are wired in for this library
#    only (ucb/CMakeLists.txt's ucpdav1 target does the same for the same
#    reason).
#  - updatecheckui (Library_updatecheckui.mk) — the "new version available"
#    notification dialog (extensions/source/update/ui). Depends only on
#    vcl tl utl comphelper cppuhelper cppu sal i18nlangtag, plus
#    boost_headers. Its componentfile is named updchk.component (not
#    updatecheckui.component) in extensions/source/update/ui — transcribed
#    as-is from Library_updatecheckui.mk.
#
# Excluded (dependency closure not satisfiable on this Linux CMake port, or
# platform-specific):
#  - scn (Library_scn.mk) — the scanner UI. On non-Windows it needs the
#    "sane_headers" external (SANE scanner API headers); `pkg-config
#    --exists sane-backends` fails and no sane*.h exists under
#    /usr/include on this system, so it is out of scope until SANE
#    development headers are installed.
#  - ldapbe2 (Library_ldapbe2.mk) — the LDAP user-profile backend. On
#    non-Windows it needs the "openldap" and "openssl" externals; `pkg-
#    config --exists ldap` fails and no ldap*.h exists under
#    /usr/include, so it is out of scope until OpenLDAP development
#    headers are installed (openssl itself IS available, but that alone
#    doesn't help without the LDAP client headers).
#  - WinUserInfoBe (Library_WinUserInfoBe.mk) — Windows-only: its
#    Library_*.mk is only added to the module under
#    "$(if $(filter WNT,$(OS)), Library_WinUserInfoBe ...)" in
#    Module_extensions.mk, and it links gb_Library_use_system_win32_libs
#    (Ole32, OleAut32, Activeds, Adsiid, Secur32) — no Linux equivalent.
#  - oleautobridge (Library_oleautobridge.mk) — Windows-only: an OLE
#    Automation<->UNO bridge; gb_Library_use_system_win32_libs(advapi32,
#    ole32, oleaut32, uuid) and ATL includes/libs, only built under
#    "ifeq($(OS),WNT)" in Module_extensions.mk.
#  - so_activex (Library_so_activex.mk) — Windows-only ActiveX control;
#    only built under "ifeq($(COM),MSC)" (inside the WNT block) in
#    Module_extensions.mk, links system32/ATL Windows libraries
#    exclusively.
#  - OOoSpotlightImporter (Library_OOoSpotlightImporter.mk) — macOS-only
#    Spotlight metadata importer; a gb_Library_Bundle using Objective-C
#    exception objects (gb_Library_add_objcobjects) and
#    gb_Library_use_system_darwin_frameworks(AppKit), only built under
#    "ifeq($(OS),MACOSX)" in Module_extensions.mk.
#
# Every included library is registered under OOOLIBS in Repository.mk
# (base: abp, dbp; onlineupdate: updatecheckui, updchk; ooo: pcr, bib, log,
# updatefeed), so all eight use LAYER OOO here, matching msfilter/dbtools/
# frm elsewhere in this port. None of the eight .component files use
# <optional/> implementations, so no IMPLEMENTATION filtering is needed in
# the librestudio_add_component() calls below.
#
# Source lists, includes, defines and library sets are transcribed from
# each Library_*.mk.
#
# updchk and updatecheckui specifically are additionally gated behind
# ENABLE_ONLINE_UPDATE (cmake/LibreStudioConfigHeaders.cmake, default OFF):
# Module_extensions.mk nests both Library_updatecheckui and Library_updchk
# (plus Configuration_updchk) inside
# "ifeq ($(ENABLE_ONLINE_UPDATE),TRUE)" -- itself inside the "DESKTOP
# BUILD_TYPE and !ENABLE_WASM_STRIP_BASIC_DRAW_MATH_IMPRESS" block that
# updatefeed alone sits in (both of those resolve true unconditionally on
# this port -- DESKTOP is this port's only BUILD_TYPE and
# ENABLE_WASM_STRIP_BASIC_DRAW_MATH_IMPRESS is always 0, see cmake/
# LibreStudioConfigHeaders.cmake -- so they need no CMake option of their
# own, matching this port's established "resolve an always-true filter
# unconditionally" precedent for BUILD_TYPE/WASM_STRIP). ENABLE_ONLINE_UPDATE
# itself is different: configure.ac's --enable-online-update has no bare-
# configure default of "yes" the way --enable-scripting/--enable-database-
# connectivity do (see that option's own comment in LibreStudioConfigHeaders.
# cmake) -- a plain ./configure leaves it off -- so it gets a real, testable
# OFF-by-default switch, the same category as ENABLE_ODK/ENABLE_PCH rather
# than ENABLE_SCRIPTING/ENABLE_DBCONNECTIVITY. updatefeed itself is NOT
# gated on it (Module_extensions.mk's ENABLE_ONLINE_UPDATE ifeq wraps only
# updatecheckui/updchk/Configuration_updchk, not updatefeed), so it stays
# unconditional below, matching upstream exactly.

# --- pcr --------------------------------------------------------------
# gb_Module_add_targets(extensions, ... Library_pcr ...) is inside
# Module_extensions.mk's "ifneq (,$(filter DBCONNECTIVITY,$(BUILD_TYPE)))"
# block, so the whole target is gated.

if (ENABLE_DBCONNECTIVITY)

set(PCR_SOURCES
    extensions/source/propctrlr/MasterDetailLinkDialog
    extensions/source/propctrlr/browserline
    extensions/source/propctrlr/browserlistbox
    extensions/source/propctrlr/browserpage
    extensions/source/propctrlr/browserview
    extensions/source/propctrlr/buttonnavigationhandler
    extensions/source/propctrlr/cellbindinghandler
    extensions/source/propctrlr/cellbindinghelper
    extensions/source/propctrlr/commoncontrol
    extensions/source/propctrlr/composeduiupdate
    extensions/source/propctrlr/controlfontdialog
    extensions/source/propctrlr/defaultforminspection
    extensions/source/propctrlr/defaulthelpprovider
    extensions/source/propctrlr/editpropertyhandler
    extensions/source/propctrlr/eformshelper
    extensions/source/propctrlr/eformspropertyhandler
    extensions/source/propctrlr/eventhandler
    extensions/source/propctrlr/fontdialog
    extensions/source/propctrlr/formbrowsertools
    extensions/source/propctrlr/formcomponenthandler
    extensions/source/propctrlr/formcontroller
    extensions/source/propctrlr/formgeometryhandler
    extensions/source/propctrlr/formlinkdialog
    extensions/source/propctrlr/formmetadata
    extensions/source/propctrlr/genericpropertyhandler
    extensions/source/propctrlr/handlerhelper
    extensions/source/propctrlr/inspectorhelpwindow
    extensions/source/propctrlr/inspectormodelbase
    extensions/source/propctrlr/listselectiondlg
    extensions/source/propctrlr/modulepcr
    extensions/source/propctrlr/newdatatype
    extensions/source/propctrlr/objectinspectormodel
    extensions/source/propctrlr/pcrcommon
    extensions/source/propctrlr/pcrunodialogs
    extensions/source/propctrlr/propcontroller
    extensions/source/propctrlr/propertycomposer
    extensions/source/propctrlr/propertycontrolextender
    extensions/source/propctrlr/propertyeditor
    extensions/source/propctrlr/propertyhandler
    extensions/source/propctrlr/propeventtranslation
    extensions/source/propctrlr/pushbuttonnavigation
    extensions/source/propctrlr/selectlabeldialog
    extensions/source/propctrlr/sqlcommanddesign
    extensions/source/propctrlr/standardcontrol
    extensions/source/propctrlr/stringrepresentation
    extensions/source/propctrlr/submissionhandler
    extensions/source/propctrlr/taborder
    extensions/source/propctrlr/unourl
    extensions/source/propctrlr/usercontrol
    extensions/source/propctrlr/xsddatatypes
    extensions/source/propctrlr/xsdvalidationhelper
    extensions/source/propctrlr/xsdvalidationpropertyhandler
)

librestudio_expand_cxx(PCR_SOURCES)
add_library(pcr SHARED ${PCR_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(pcr REUSE_FROM precompiled_system)
endif ()

target_include_directories(pcr PRIVATE
    "${CMAKE_SOURCE_DIR}/extensions/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(pcr udkapi_normal_headers offapi_normal_headers)

target_link_libraries(pcr PRIVATE
    svx svxcore editeng sfx svt tk vcl svl utl tl dbtools i18nlangtag
    comphelper cppuhelper cppu sal salhelper
    Boost::boost)

librestudio_add_component(pcr
    COMPONENT "${CMAKE_SOURCE_DIR}/extensions/source/propctrlr/pcr.component"
    LIBRARY   pcr
    LAYER     OOO)

librestudio_set_library_output_name(pcr pcr OOO)

install(TARGETS pcr LIBRARY DESTINATION program)

endif (ENABLE_DBCONNECTIVITY)

# --- abp --------------------------------------------------------------

set(ABP_SOURCES
    extensions/source/abpilot/abpfinalpage
    extensions/source/abpilot/abspage
    extensions/source/abpilot/abspilot
    extensions/source/abpilot/admininvokationimpl
    extensions/source/abpilot/admininvokationpage
    extensions/source/abpilot/datasourcehandling
    extensions/source/abpilot/fieldmappingimpl
    extensions/source/abpilot/fieldmappingpage
    extensions/source/abpilot/moduleabp
    extensions/source/abpilot/tableselectionpage
    extensions/source/abpilot/typeselectionpage
    extensions/source/abpilot/unodialogabp
)

librestudio_expand_cxx(ABP_SOURCES)
add_library(abp SHARED ${ABP_SOURCES})

target_include_directories(abp PRIVATE
    "${CMAKE_SOURCE_DIR}/extensions/inc"
    "${CMAKE_SOURCE_DIR}/extensions/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(abp udkapi_normal_headers offapi_normal_headers)

target_link_libraries(abp PRIVATE
    comphelper cppuhelper cppu sal sfx svl svt svxcore svx tk tl utl vcl
    i18nlangtag
    Boost::boost)

librestudio_add_component(abp
    COMPONENT "${CMAKE_SOURCE_DIR}/extensions/source/abpilot/abp.component"
    LIBRARY   abp
    LAYER     OOO)

librestudio_set_library_output_name(abp abp OOO)

install(TARGETS abp LIBRARY DESTINATION program)

# --- bib --------------------------------------------------------------

set(BIB_SOURCES
    extensions/source/bibliography/bibbeam
    extensions/source/bibliography/bibconfig
    extensions/source/bibliography/bibcont
    extensions/source/bibliography/bibload
    extensions/source/bibliography/bibmod
    extensions/source/bibliography/bibview
    extensions/source/bibliography/datman
    extensions/source/bibliography/formcontrolcontainer
    extensions/source/bibliography/framectr
    extensions/source/bibliography/general
    extensions/source/bibliography/loadlisteneradapter
    extensions/source/bibliography/toolbar
)

librestudio_expand_cxx(BIB_SOURCES)
add_library(bib SHARED ${BIB_SOURCES})

target_include_directories(bib PRIVATE
    "${CMAKE_SOURCE_DIR}/extensions/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(bib udkapi_normal_headers offapi_normal_headers)

target_link_libraries(bib PRIVATE
    dbtools sfx sot svt svl tk vcl utl tl comphelper cppuhelper cppu sal
    salhelper i18nlangtag
    Boost::boost)

librestudio_add_component(bib
    COMPONENT "${CMAKE_SOURCE_DIR}/extensions/source/bibliography/bib.component"
    LIBRARY   bib
    LAYER     OOO)

librestudio_set_library_output_name(bib bib OOO)

install(TARGETS bib LIBRARY DESTINATION program)

# --- dbp --------------------------------------------------------------
# gb_Module_add_targets(extensions, ... Library_dbp ...) is inside the same
# DBCONNECTIVITY filter block as Library_pcr above.

if (ENABLE_DBCONNECTIVITY)

set(DBP_SOURCES
    extensions/source/dbpilots/commonpagesdbp
    extensions/source/dbpilots/controlwizard
    extensions/source/dbpilots/dbptools
    extensions/source/dbpilots/gridwizard
    extensions/source/dbpilots/groupboxwiz
    extensions/source/dbpilots/listcombowizard
    extensions/source/dbpilots/moduledbp
    extensions/source/dbpilots/optiongrouplayouter
    extensions/source/dbpilots/wizardservices
)

librestudio_expand_cxx(DBP_SOURCES)
add_library(dbp SHARED ${DBP_SOURCES})

target_include_directories(dbp PRIVATE
    "${CMAKE_SOURCE_DIR}/extensions/inc"
    "${CMAKE_SOURCE_DIR}/extensions/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(dbp udkapi_normal_headers offapi_normal_headers)

target_link_libraries(dbp PRIVATE
    svt vcl tl tk svl sfx dbtools utl comphelper cppuhelper cppu sal
    salhelper i18nlangtag
    Boost::boost)

librestudio_add_component(dbp
    COMPONENT "${CMAKE_SOURCE_DIR}/extensions/source/dbpilots/dbp.component"
    LIBRARY   dbp
    LAYER     OOO)

librestudio_set_library_output_name(dbp dbp OOO)

install(TARGETS dbp LIBRARY DESTINATION program)

endif (ENABLE_DBCONNECTIVITY)

# --- log ----------------------------------------------------------------

set(LOG_SOURCES
    extensions/source/logging/consolehandler
    extensions/source/logging/csvformatter
    extensions/source/logging/filehandler
    extensions/source/logging/logger
    extensions/source/logging/loggerconfig
    extensions/source/logging/loghandler
    extensions/source/logging/logrecord
    extensions/source/logging/plaintextformatter
    extensions/source/logging/simpletextformatter
)

librestudio_expand_cxx(LOG_SOURCES)
add_library(log SHARED ${LOG_SOURCES})

target_include_directories(log PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(log udkapi_normal_headers offapi_normal_headers)

target_link_libraries(log PRIVATE
    comphelper cppuhelper cppu sal tl
    Boost::boost)

librestudio_add_component(log
    COMPONENT "${CMAKE_SOURCE_DIR}/extensions/source/logging/log.component"
    LIBRARY   log
    LAYER     OOO)

librestudio_set_library_output_name(log log OOO)

install(TARGETS log LIBRARY DESTINATION program)

# --- updatefeed -----------------------------------------------------------

set(UPDATEFEED_SOURCES
    extensions/source/update/feed/updatefeed
)

librestudio_expand_cxx(UPDATEFEED_SOURCES)
add_library(updatefeed SHARED ${UPDATEFEED_SOURCES})

target_include_directories(updatefeed PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(updatefeed udkapi_normal_headers offapi_normal_headers)

target_link_libraries(updatefeed PRIVATE
    cppuhelper cppu sal vcl
    Boost::boost)

librestudio_add_component(updatefeed
    COMPONENT "${CMAKE_SOURCE_DIR}/extensions/source/update/feed/updatefeed.component"
    LIBRARY   updatefeed
    LAYER     OOO)

librestudio_set_library_output_name(updatefeed updatefeed OOO)

install(TARGETS updatefeed LIBRARY DESTINATION program)

# --- updchk -----------------------------------------------------------

if (ENABLE_ONLINE_UPDATE)

# Only the OS-neutral exception objects from Library_updchk.mk are built;
# onlinecheck.cxx is Windows-only (gated by "ifeq($(OS),WNT)" in the .mk and
# by "#ifdef _WIN32" around its wininet.h use in the source itself) and is
# excluded on this Linux build.
set(UPDCHK_SOURCES
    extensions/source/update/check/download
    extensions/source/update/check/updatecheck
    extensions/source/update/check/updatecheckconfig
    extensions/source/update/check/updatecheckjob
    extensions/source/update/check/updatehdl
    extensions/source/update/check/updateprotocol
)

librestudio_expand_cxx(UPDCHK_SOURCES)

# Propagated to the root CMakeLists.txt's own scope (add_subdirectory()
# nesting is exactly one level here) so extensions/CppunitTest_deferred.cmake
# -- which runs in that scope, include()'d well after this add_subdirectory()
# returns -- can recompile the same list into CppunitTest_extensions_
# test_update.mk's own gb_CppunitTest_use_library_objects(extensions_
# test_update,updchk) call, the same technique sal/CMakeLists.txt's own
# sal_osl_file_details uses for its use_library_objects(sal): that test's
# extensions/qa/update/test_update.cxx includes "../../source/update/check/
# updatecheck.hxx" and "updateprotocol.hxx" directly -- internal headers, not
# part of updchk's public UNO API -- so its internal symbols are not
# reachable by linking against the updchk *library*.
set(UPDCHK_SOURCES "${UPDCHK_SOURCES}" PARENT_SCOPE)

add_library(updchk SHARED ${UPDCHK_SOURCES})

target_include_directories(updchk PRIVATE
    "${CMAKE_SOURCE_DIR}/extensions/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(updchk officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(updchk PRIVATE
    comphelper cppuhelper cppu sal salhelper tk utl vcl
    Boost::boost
    CURL::libcurl)

# Library_updchk.mk:36-46 -- ifeq ($(OS),WNT): use_system_win32_libs(ole32,
# shell32, wininet) plus the WNT-only onlinecheck.cxx exception object (its
# own exclusion is already noted above). Structural-but-inert like sal's own
# "sal_comtools" WIN32 block (sal/CMakeLists.txt) -- never executes on this
# Linux build, kept so the Windows link surface is visible without an MSVC
# toolchain here to actually confirm it.
if (WIN32)
    target_link_libraries(updchk PRIVATE ole32 shell32 wininet)
endif ()

librestudio_add_component(updchk
    COMPONENT "${CMAKE_SOURCE_DIR}/extensions/source/update/check/updchk.uno.component"
    LIBRARY   updchk
    LAYER     OOO)

librestudio_set_library_output_name(updchk updchk OOO)

install(TARGETS updchk LIBRARY DESTINATION program)

endif (ENABLE_ONLINE_UPDATE)

# --- updatecheckui ----------------------------------------------------

if (ENABLE_ONLINE_UPDATE)

# Its componentfile is named updchk.component (not updatecheckui.component)
# under extensions/source/update/ui -- transcribed as-is from
# Library_updatecheckui.mk's gb_Library_set_componentfile call.
set(UPDATECHECKUI_SOURCES
    extensions/source/update/ui/updatecheckui
)

librestudio_expand_cxx(UPDATECHECKUI_SOURCES)
add_library(updatecheckui SHARED ${UPDATECHECKUI_SOURCES})

target_include_directories(updatecheckui PRIVATE
    "${CMAKE_SOURCE_DIR}/extensions/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(updatecheckui udkapi_normal_headers offapi_normal_headers)

target_link_libraries(updatecheckui PRIVATE
    vcl tl utl comphelper cppuhelper cppu sal i18nlangtag
    Boost::boost)

librestudio_add_component(updatecheckui
    COMPONENT "${CMAKE_SOURCE_DIR}/extensions/source/update/ui/updchk.component"
    LIBRARY   updatecheckui
    LAYER     OOO)

librestudio_set_library_output_name(updatecheckui updatecheckui OOO)

install(TARGETS updatecheckui LIBRARY DESTINATION program)

endif (ENABLE_ONLINE_UPDATE)

# --- Configuration_updchk: documented gap, not an oversight ------------

# Configuration_updchk.mk (extensions/source/update/check/org/openoffice/
# Office/{Jobs,Addons}-onlineupdate.xcu spooled + Addons.xcu localized-data
# merge) is gb_Configuration_add_spool_modules + add_localized_datas -- the
# exact same mechanism connectivity/CMakeLists.txt's own header comment
# already documents as not portable here: the spool step produces an
# intermediate .xcu that only postprocess/CustomTarget_registry.mk (never
# add_subdirectory()'d anywhere in this tree) folds into a real .xcd, and
# reaching into officecfg/CMakeLists.txt's own already-built main.xcd target
# from here would mean reopening a sibling module's finished target. Left
# unbuilt for the identical reason, whether or not ENABLE_ONLINE_UPDATE is
# ever turned on.

# --- UIConfig_sabpilot / UIConfig_scanner -------------------------------

# Both sit in Module_extensions.mk's "ifeq (,$(DISABLE_DYNLOADING))" block
# alongside Library_abp/Library_scn -- DISABLE_DYNLOADING is never set
# anywhere in this port (confirmed elsewhere in this tree, e.g. codemaker/
# CMakeLists.txt, cpputools/CMakeLists.txt), so that filter resolves
# unconditionally true here, matching abp's own unconditional build above.
# Neither UIConfig depends on Library_scn actually building (scn itself
# stays excluded, see the "Excluded" list above) -- both are pure .ui
# packaging, same gb_UIConfig_DIR_FOR-derived install(FILES ...) mechanism as
# UIConfig_svt/UIConfig_vcl (svtools/CMakeLists.txt, vcl/CMakeLists.txt). No
# CppunitTest in this module calls use_uiconfigs on either, so no real
# build-time ALL-target mirror is needed here (unlike UIConfig_svt's own
# svt_uiconfig, added only because svtools_dialogs_test needed something to
# depend on).
install(FILES
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sabpilot/ui/contentfieldpage.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sabpilot/ui/contenttablepage.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sabpilot/ui/datasourcepage.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sabpilot/ui/defaultfieldselectionpage.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sabpilot/ui/fieldassignpage.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sabpilot/ui/fieldlinkpage.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sabpilot/ui/gridfieldsselectionpage.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sabpilot/ui/groupradioselectionpage.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sabpilot/ui/invokeadminpage.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sabpilot/ui/optiondbfieldpage.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sabpilot/ui/optionsfinalpage.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sabpilot/ui/optionvaluespage.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sabpilot/ui/selecttablepage.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sabpilot/ui/selecttypepage.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sabpilot/ui/tableselectionpage.ui"
    DESTINATION share/config/soffice.cfg/modules/sabpilot/ui)

install(FILES
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/scanner/ui/sanedialog.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/scanner/ui/griddialog.ui"
    DESTINATION share/config/soffice.cfg/modules/scanner/ui)

# --- UIConfig_sbibliography / UIConfig_spropctrlr -----------------------

# Both sit in Module_extensions.mk's "ifneq (,$(filter DBCONNECTIVITY,
# $(BUILD_TYPE)))" block alongside Library_dbp/Library_pcr, so they are
# gated on ENABLE_DBCONNECTIVITY the same way those two libraries are above.
if (ENABLE_DBCONNECTIVITY)

install(FILES
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sbibliography/menubar/menubar.xml"
    DESTINATION share/config/soffice.cfg/modules/sbibliography/menubar)

install(FILES
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sbibliography/ui/autofiltermenu.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sbibliography/ui/choosedatasourcedialog.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sbibliography/ui/combobox.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sbibliography/ui/editbox.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sbibliography/ui/generalpage.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sbibliography/ui/querydialog.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sbibliography/ui/mappingdialog.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/sbibliography/ui/toolbar.ui"
    DESTINATION share/config/soffice.cfg/modules/sbibliography/ui)

install(FILES
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/browserline.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/browserpage.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/colorlistbox.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/combobox.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/controlfontdialog.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/datefield.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/datatypedialog.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/datetimefield.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/formlinksdialog.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/formattedcontrol.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/formattedsample.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/formproperties.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/hyperlinkfield.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/labelselectiondialog.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/listbox.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/listselectdialog.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/multiline.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/numericfield.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/taborder.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/textfield.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/timefield.ui"
    "${CMAKE_SOURCE_DIR}/extensions/uiconfig/spropctrlr/ui/urlcontrol.ui"
    DESTINATION share/config/soffice.cfg/modules/spropctrlr/ui)

endif (ENABLE_DBCONNECTIVITY)

# --- Other Module_extensions.mk targets: documented gaps ----------------
#
# Re-checked directly against Module_extensions.mk in full, alongside every
# Library_*.mk/Executable_*.mk/WinResTarget_*.mk/CustomTarget_*.mk/
# Package_*.mk/StaticLibrary_*.mk/JunitTest_*.mk it names, so these are
# flagged deliberately, not missed:
#  - AllLangMoTarget_pcr (l10n .po -> .mo): not ported, matching this port's
#    established blanket exclusion of every AllLangMoTarget_* (see e.g.
#    uui/CMakeLists.txt, desktop/CMakeLists.txt's own notes on their own
#    module's AllLangMoTarget).
#  - JunitTest_extensions_unoapi (gb_Module_add_subsequentcheck_targets): a
#    Java UNO-API conformance test (gb_JunitTest_set_unoapi_test_defaults) --
#    out of scope like every other JunitTest in this port (see bean/
#    CMakeLists.txt, javaunohelper/CMakeLists.txt, unoxml/CMakeLists.txt for
#    the same call on their own modules' JunitTests).
#  - WinResTarget_activex, Library_so_activex, CustomTarget_so_activex_idl
#    (ifeq($(OS),WNT) -> ifeq($(COM),MSC) block): the ActiveX control's
#    Windows .rc resource, the ActiveX DLL itself, and the MIDL-generated
#    .tlb/.h/.c stubs (CustomTarget_so_activex_idl.mk invokes midl.exe
#    directly) -- MSVC/Windows-only, no Linux equivalent, same verdict as
#    Library_so_activex's own exclusion above.
#  - Executable_twain32shim ($(if $(filter WNT,$(OS)), $(if
#    $(CXX_X86_BINARY),Executable_twain32shim))): a Windows x86 TWAIN-to-
#    scanner-API shim process, needs both WNT and sane_headers -- out of
#    scope on both counts (WNT-only, and sane_headers is already excluded
#    for Library_scn above).
#  - StaticLibrary_extensions_macosx_common, Library_OOoSpotlightImporter,
#    Executable_QuickLookPreview, Executable_QuickLookThumbnail,
#    Package_mdibundle, Package_OOoSpotlightImporter,
#    Package_quicklookpreviewappex, Package_QuickLookPreview,
#    Package_quicklookthumbnailappex, Package_QuickLookThumbnail (all under
#    "ifeq($(OS),MACOSX)"): macOS Spotlight/QuickLook metadata extensions --
#    Objective-C sources, Darwin frameworks (AppKit/Quartz/
#    QuickLookThumbnailing) and .mdimporter/.appex bundle packaging, all
#    macOS-only, no Linux equivalent, same verdict as Library_
#    OOoSpotlightImporter's own exclusion above.

# extensions/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/extensions/CppunitTest_deferred.cmake")
