# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# desktop -- the module that produces soffice.bin, the office application
# binary itself, plus the extension-manager/deployment machinery and the
# unopkg command-line tool that drives it. Twelve targets, in dependency
# order (each library before the things that link it):
#
#   deploymentmisc  (Library_deploymentmisc.mk)  -- shared helpers, no .component
#   deployment      (Library_deployment.mk)      -- the ExtensionManager UNO services
#   deploymentgui   (Library_deploymentgui.mk)   -- the extension manager's dialogs
#   offacc          (Library_offacc.mk)          -- the office Acceptor service
#   sofficeapp      (Library_sofficeapp.mk)       -- soffice.bin's application logic
#   spl             (Library_spl.mk)             -- the splash screen services
#   migrationoo2    (Library_migrationoo2.mk)    -- OOo2 basic/wordbook migration
#   migrationoo3    (Library_migrationoo3.mk)    -- OOo3 extension migration
#   unopkgapp       (Library_unopkgapp.mk)       -- unopkg's application logic
#   soffice_bin     (Executable_soffice_bin.mk)  -- THE application binary
#   unopkg_bin      (Executable_unopkg_bin.mk)   -- the unopkg command-line tool
#   oosplash        (Executable_oosplash.mk)     -- the process that shows the
#                                                    splash screen and execs
#                                                    soffice.bin
#
# Module_desktop.mk resolves, for this port (DESKTOP in BUILD_TYPE, Linux,
# USING_X11=TRUE, DISABLE_GUI=FALSE, gb_CondExeUnopkg true since
# DISABLE_DYNLOADING is unset):
#
#   Library_deployment, Library_deploymentgui, Library_deploymentmisc,
#   Library_offacc, Library_sofficeapp, Library_spl (ENABLE_WASM_STRIP_SPLASH
#   and DISABLE_GUI are both off, so spl is not skipped),
#   Executable_soffice_bin, Executable_unopkg_bin, Library_migrationoo2,
#   Library_migrationoo3, Library_unopkgapp, Executable_oosplash.
#
# Excluded, and why:
#   - Library_crashreport, Executable_minidump_upload: gb_Helper_optional/
#     $(if $(ENABLE_BREAKPAD),...) guarded; ENABLE_BREAKPAD is not tracked in
#     this port (untracked -> off).
#   - CustomTarget_soffice: MACOSX-only. CustomTarget_soffice_bin-emscripten-
#     exports: EMSCRIPTEN-only. Neither applies to this Linux port.
#   - Package_branding, Package_branding_custom (CUSTOM_BRAND_DIR unset),
#     CustomTarget_desktop_unopackages_install,
#     GeneratedPackage_desktop_unopackages_install: pure file-copy/packaging
#     targets (gb_Package_*/gb_CustomTarget_* mechanics), no compiled code --
#     this port never reproduces those, matching every other module (see
#     e.g. xmlsecurity/CMakeLists.txt).
#   - Package_soffice_sh and Package_{sbase,scalc,sdraw,simpress,smath,
#     swriter}_sh are, by contrast, real and now wired below: each is
#     nothing but `gb_Package_add_file(desktop_<name>_sh,
#     $(LIBO_BIN_FOLDER)/<name>, <name>.sh)` -- a single verbatim copy of
#     desktop/scripts/<name>.sh to program/<name> (no .sh suffix on the
#     installed name), the same gb_Package_add_file + plain
#     install(PROGRAMS ... RENAME ...) convention cpputools/CMakeLists.txt
#     already established for Package_uno_sh.mk's uno.sh. soffice.bin itself
#     stays directly launchable regardless of these wrappers -- like the
#     vcl/workben programs, it gets its own bootstrap rc file (see the
#     librestudio_generate_unorc() call below) -- but these 7 wrapper
#     scripts are upstream's own documented entry points and are cheap
#     enough to install for real rather than leave undone.
#   - UIConfig_deployment is now ported for real (2026-09-27) -- see this
#     file's own "--- UIConfig_deployment ---" section near the end. The old
#     "UI config = out of scope, established port-wide policy" note was
#     accurate only before this session's UIConfig-porting precedent (vcl/
#     svtools/svx/cui/basctl/reportdesign/xmlsecurity/filter/...) existed;
#     needed for real by desktop_dialogs_test's own use_uiconfigs(desktop)
#     call. AllLangMoTarget_dkt (localization .mo) remains out of scope,
#     matching every other AllLangMoTarget_* in this port.
#   - Pagein_* targets: $(if $(ENABLE_PAGEIN),...) guarded; ENABLE_PAGEIN is
#     not tracked in this port.
#   - Extension_test-active/-passive/-crashextension and their Jar_*/
#     Library_active_native/Library_passive_native/Pyuno_passive_python/
#     Rdb_passive_*/Library_crashextension/Rdb_crashextension satellites:
#     all MAKECMDGOALS-gated (only built when explicitly requested as a
#     make goal), i.e. test infrastructure, not production build output.
#   - CppunitTest_desktop_app, CppunitTest_desktop_version,
#     CppunitTest_desktop_lib, CppunitTest_desktop_lokinit,
#     CppunitTest_desktop_dialogs_test (screenshot target): tests.
#   - StaticLibrary_winloader/winlauncher, Executable_quickstart/sbase/scalc/
#     sdraw/simpress/smath/soffice_exe/soffice_com/soffice_safe/sweb/
#     swriter/unoinfo/unopkg/unopkg_com, WinResTarget_*: all under
#     Module_desktop.mk's `ifeq ($(OS),WNT)` branch -- Windows-only.
#
# Every source path below, and every use_libraries/use_externals/
# use_sdk_api/use_custom_headers/componentfile entry, was read directly out
# of the corresponding Library_*.mk/Executable_*.mk and cross-checked
# against the real .component files under desktop/.

# ---------------------------------------------------------------------------
# deploymentmisc -- shared helpers (dp_misc, dp_update, lockfile, ...) used
# by deployment, deploymentgui and unopkgapp below. No .component file:
# Library_deploymentmisc.mk has no gb_Library_set_componentfile call, and
# `find desktop -iname "*.component"` confirms no deploymentmisc.component
# exists anywhere in the tree.
# ---------------------------------------------------------------------------

set(DEPLOYMENTMISC_SOURCES
    desktop/source/deployment/misc/dp_dependencies
    desktop/source/deployment/misc/dp_descriptioninfoset
    desktop/source/deployment/misc/dp_identifier
    desktop/source/deployment/misc/dp_interact
    desktop/source/deployment/misc/dp_misc
    desktop/source/deployment/misc/dp_platform
    desktop/source/deployment/misc/dp_resource
    desktop/source/deployment/misc/dp_ucb
    desktop/source/deployment/misc/dp_update
    desktop/source/deployment/misc/dp_version
    desktop/source/deployment/misc/lockfile
)

librestudio_expand_cxx(DEPLOYMENTMISC_SOURCES)
add_library(deploymentmisc SHARED ${DEPLOYMENTMISC_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(deploymentmisc PRIVATE
        "${CMAKE_SOURCE_DIR}/desktop/inc/pch/precompiled_deploymentmisc.hxx")
    target_compile_definitions(deploymentmisc PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(deploymentmisc PRIVATE
    "${CMAKE_SOURCE_DIR}/desktop/inc"
    "${CMAKE_SOURCE_DIR}/desktop/source/deployment/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(deploymentmisc udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(deploymentmisc PRIVATE
    DESKTOP_DEPLOYMENTMISC_DLLIMPLEMENTATION)

target_link_libraries(deploymentmisc PRIVATE
    comphelper cppu cppuhelper sal tl ucbhelper utl vcl xmlscript i18nlangtag
    Boost::boost)

librestudio_set_library_output_name(deploymentmisc deploymentmisc OOO)

install(TARGETS deploymentmisc LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# deployment -- the extension manager's UNO services (ExtensionManager,
# PackageManagerFactory, the per-kind PackageRegistryBackends, ...). 28
# sources from Library_deployment.mk's add_exception_objects list.
# ---------------------------------------------------------------------------

set(DEPLOYMENT_SOURCES
    desktop/source/deployment/dp_log
    desktop/source/deployment/dp_persmap
    desktop/source/deployment/dp_xml
    desktop/source/deployment/manager/dp_activepackages
    desktop/source/deployment/manager/dp_commandenvironments
    desktop/source/deployment/manager/dp_extensionmanager
    desktop/source/deployment/manager/dp_informationprovider
    desktop/source/deployment/manager/dp_manager
    desktop/source/deployment/manager/dp_managerfac
    desktop/source/deployment/manager/dp_properties
    desktop/source/deployment/registry/component/dp_compbackenddb
    desktop/source/deployment/registry/component/dp_component
    desktop/source/deployment/registry/configuration/dp_configuration
    desktop/source/deployment/registry/configuration/dp_configurationbackenddb
    desktop/source/deployment/registry/dp_backend
    desktop/source/deployment/registry/dp_backenddb
    desktop/source/deployment/registry/dp_registry
    desktop/source/deployment/registry/executable/dp_executable
    desktop/source/deployment/registry/executable/dp_executablebackenddb
    desktop/source/deployment/registry/help/dp_help
    desktop/source/deployment/registry/help/dp_helpbackenddb
    desktop/source/deployment/registry/package/dp_extbackenddb
    desktop/source/deployment/registry/package/dp_package
    desktop/source/deployment/registry/script/dp_lib_container
    desktop/source/deployment/registry/script/dp_script
    desktop/source/deployment/registry/script/dp_scriptbackenddb
    desktop/source/deployment/registry/sfwk/dp_parceldesc
    desktop/source/deployment/registry/sfwk/dp_sfwk
)

librestudio_expand_cxx(DEPLOYMENT_SOURCES)
add_library(deployment SHARED ${DEPLOYMENT_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(deployment PRIVATE
        "${CMAKE_SOURCE_DIR}/desktop/inc/pch/precompiled_deployment.hxx")
    target_compile_definitions(deployment PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(deployment PRIVATE
    "${CMAKE_SOURCE_DIR}/desktop/inc"
    "${CMAKE_SOURCE_DIR}/desktop/source/deployment/inc"
    "${CMAKE_SOURCE_DIR}/desktop/source/deployment/registry/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(deployment udkapi_normal_headers offapi_normal_headers)

target_link_libraries(deployment PRIVATE
    comphelper cppu cppuhelper deploymentmisc sal svl tl ucbhelper utl vcl
    xmlscript i18nlangtag
    Boost::boost)

# `ifneq (,$(filter XMLHELP,$(BUILD_TYPE)))` in Library_deployment.mk: the
# extension manager's help-registry backend (dp_help/dp_helpbackenddb above)
# needs helplinker (helpcompiler/CMakeLists.txt) to link and re-index
# installed help content packages. This port tracks that switch as the real
# ENABLE_XMLHELP CACHE BOOL (cmake/LibreStudioConfigHeaders.cmake, default
# ON), so the link is a genuine conditional rather than a hardcoded default.
if (ENABLE_XMLHELP)
    target_link_libraries(deployment PRIVATE helplinker)
endif ()

librestudio_add_component(deployment
    COMPONENT "${CMAKE_SOURCE_DIR}/desktop/source/deployment/deployment.component"
    LIBRARY   deployment
    LAYER     OOO)
librestudio_set_library_output_name(deployment deployment OOO)

install(TARGETS deployment LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# deploymentgui -- the extension manager's dialogs (PackageManagerDialog,
# LicenseDialog, UpdateRequiredDialog). 10 sources.
# ---------------------------------------------------------------------------

set(DEPLOYMENTGUI_SOURCES
    desktop/source/deployment/gui/dp_gui_dependencydialog
    desktop/source/deployment/gui/dp_gui_dialog2
    desktop/source/deployment/gui/dp_gui_extensionboxwithbuttons
    desktop/source/deployment/gui/dp_gui_extensioncmdqueue
    desktop/source/deployment/gui/dp_gui_extlistbox
    desktop/source/deployment/gui/dp_gui_service
    desktop/source/deployment/gui/dp_gui_theextmgr
    desktop/source/deployment/gui/dp_gui_updatedialog
    desktop/source/deployment/gui/dp_gui_updateinstalldialog
    desktop/source/deployment/gui/license_dialog
)

librestudio_expand_cxx(DEPLOYMENTGUI_SOURCES)
add_library(deploymentgui SHARED ${DEPLOYMENTGUI_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(deploymentgui PRIVATE
        "${CMAKE_SOURCE_DIR}/desktop/inc/pch/precompiled_deploymentgui.hxx")
    target_compile_definitions(deploymentgui PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(deploymentgui PRIVATE
    "${CMAKE_SOURCE_DIR}/desktop/inc"
    "${CMAKE_SOURCE_DIR}/desktop/source/deployment/inc"
    "${CMAKE_SOURCE_DIR}/desktop/source/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(deploymentgui
    officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(deploymentgui PRIVATE
    comphelper cppu cppuhelper deploymentmisc i18nlangtag sal salhelper sfx
    svl svt svxcore tk tl ucbhelper utl vcl
    Boost::boost)

librestudio_add_component(deploymentgui
    COMPONENT "${CMAKE_SOURCE_DIR}/desktop/source/deployment/gui/deploymentgui.component"
    LIBRARY   deploymentgui
    LAYER     OOO)
librestudio_set_library_output_name(deploymentgui deploymentgui OOO)

install(TARGETS deploymentgui LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# offacc -- the office Acceptor UNO service (com.sun.star.office.Acceptor),
# used by the --accept command-line switch. 1 source.
# ---------------------------------------------------------------------------

set(OFFACC_SOURCES
    desktop/source/offacc/acceptor
)

librestudio_expand_cxx(OFFACC_SOURCES)
add_library(offacc SHARED ${OFFACC_SOURCES})

target_include_directories(offacc PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(offacc officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(offacc PRIVATE
    comphelper cppu cppuhelper sal tl)

librestudio_add_component(offacc
    COMPONENT "${CMAKE_SOURCE_DIR}/desktop/source/offacc/offacc.component"
    LIBRARY   offacc
    LAYER     OOO)
librestudio_set_library_output_name(offacc offacc OOO)

install(TARGETS offacc LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# sofficeapp -- soffice.bin's actual application logic: startup/shutdown,
# command-line handling, the IPC pipe that routes a second invocation into
# the running instance, user-profile migration, and (since USING_X11=TRUE
# resolves the LibreOfficeKit branch further down in Library_sofficeapp.mk)
# the LibreOfficeKit entry points. 18 sources: 15 unconditional + 3 LOK.
# ---------------------------------------------------------------------------

# dbus is used unconditionally by Library_sofficeapp.mk's use_externals
# (officeipcthread's desktop presence/activation handling); declared locally
# per this port's IMPORTED_TARGET convention (see e.g. vcl, sd, shell).
pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)

set(SOFFICEAPP_SOURCES
    desktop/source/app/app
    desktop/source/app/appinit
    desktop/source/app/check_ext_deps
    desktop/source/app/cmdlineargs
    desktop/source/app/cmdlinehelp
    desktop/source/app/desktopcontext
    desktop/source/app/dispatchwatcher
    desktop/source/app/initjsunoscripting
    desktop/source/app/langselect
    desktop/source/app/lockfile2
    desktop/source/app/officeipcthread
    desktop/source/app/opencl
    desktop/source/app/sofficemain
    desktop/source/app/userinstall
    desktop/source/migration/migration

    # LibreOfficeKit bits (desktop/lokclipboard.component's implementation
    # lives in lokclipboard.cxx below). Library_sofficeapp.mk includes this
    # group whenever OS is one of ANDROID/iOS/MACOSX/WNT, or -- the branch
    # that applies on this Linux, non-Android build -- whenever
    # $(filter TRUE,$(USING_X11) $(DISABLE_GUI)) is non-empty; USING_X11 is
    # TRUE in this port, so the group is included unconditionally here.
    desktop/source/lib/init
    desktop/source/lib/lokinteractionhandler
    desktop/source/lib/lokclipboard
)

librestudio_expand_cxx(SOFFICEAPP_SOURCES)
add_library(sofficeapp SHARED ${SOFFICEAPP_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(sofficeapp PRIVATE
        "${CMAKE_SOURCE_DIR}/desktop/inc/pch/precompiled_sofficeapp.hxx")
    target_compile_definitions(sofficeapp PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(sofficeapp PRIVATE
    "${CMAKE_SOURCE_DIR}/desktop/inc"
    "${CMAKE_SOURCE_DIR}/desktop/source/inc"
    "${CMAKE_SOURCE_DIR}/desktop/source/deployment/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(sofficeapp
    officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(sofficeapp PRIVATE DESKTOP_DLLIMPLEMENTATION)

target_link_libraries(sofficeapp PRIVATE
    comphelper cppu cppuhelper deploymentmisc editeng fwk i18nlangtag sal
    salhelper sb sfx svl svx svxcore svt tk tl ucbhelper utl vcl
    Boost::boost
    ICU::uc ICU::i18n
    PkgConfig::DBUS)

# Fix (first real `cmake --build`, 2026-09-27): Library_sofficeapp.mk links
# both clew and opencl under "$(if $(filter OPENCL,$(BUILD_TYPE)),...)"
# (lines 24, 70) -- missing here, the same "OPENCL BUILD_TYPE vs
# ENABLE_OPENCL" mismatch already found and fixed for vcl and scui (see
# vcl/CMakeLists.txt's own comment on this exact class of gap):
# desktop/source/app/opencl.cxx includes <opencl/openclwrapper.hxx> (a
# public, global header), which itself needs <clew/clew.h> -- "fatal error:
# clew/clew.h: No such file" without this. clew's own
# target_include_directories(clew PUBLIC ...) (external/clew/CMakeLists.txt)
# propagates the include path automatically once linked directly (opencl
# links clew PRIVATE, so linking opencl alone would not be enough -- both
# are linked here, matching the real .mk's own list).
if (ENABLE_OPENCL)
    target_link_libraries(sofficeapp PRIVATE clew opencl)
endif ()

librestudio_add_component(sofficeapp
    COMPONENT "${CMAKE_SOURCE_DIR}/desktop/lokclipboard.component"
    LIBRARY   sofficeapp
    LAYER     OOO)
# 2026-09-29, found via a real ctest run: Repository.mk registers sofficeapp
# under PLAINLIBS_OOO (line ~671), not OOOLIBS -- despite the "OOO" in that
# list's own name, PLAINLIBS_OOO/PLAINLIBS_NONE are both "plain" filename
# groups (no gb_Library_DLLPOSTFIX "lo" suffix), the same distinction
# unotest/CMakeLists.txt's own header comment already worked out in detail
# for unotest/unobootstrapprotector/unoexceptionprotector. OOO here wrongly
# produced libsofficeapplo.so; LibreOfficeKitInit.h's lok_dlopen() hardcodes
# the bare "libsofficeapp.so" name (falling back to libmergedlo.so, this
# port's own --enable-mergedlibs equivalent, only ported), so every real LOK
# consumer (libreofficekit_tiledrendering, desktop_lib) failed to find it --
# confirmed live: "failed to open library '.../libmergedlo.so': ... No such
# file", the exact fallback path lok_dlopen takes when libsofficeapp.so is
# missing.
librestudio_set_library_output_name(sofficeapp sofficeapp PLAIN)

install(TARGETS sofficeapp LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# spl -- the splash screen UNO services (SplashScreen, PipeSplashScreen). 2
# sources.
# ---------------------------------------------------------------------------

set(SPL_SOURCES
    desktop/source/splash/splash
    desktop/source/splash/unxsplash
)

librestudio_expand_cxx(SPL_SOURCES)
add_library(spl SHARED ${SPL_SOURCES})

target_include_directories(spl PRIVATE
    "${CMAKE_SOURCE_DIR}/desktop/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(spl udkapi_normal_headers offapi_normal_headers)

target_link_libraries(spl PRIVATE
    comphelper cppu cppuhelper sal tl ucbhelper utl vcl
    Boost::boost)

librestudio_add_component(spl
    COMPONENT "${CMAKE_SOURCE_DIR}/desktop/source/splash/spl.component"
    LIBRARY   spl
    LAYER     OOO)
librestudio_set_library_output_name(spl spl OOO)

install(TARGETS spl LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# migrationoo2 -- OOo2-generation user-profile migration (Basic macros,
# custom spellchecking wordbooks, and -- under LIBRESTUDIO_ENABLE_JAVA --
# JVM configuration). 2 sources unconditionally, 1 more under
# LIBRESTUDIO_ENABLE_JAVA.
# ---------------------------------------------------------------------------

set(MIGRATIONOO2_SOURCES
    desktop/source/migration/services/basicmigration
    desktop/source/migration/services/wordbookmigration
)

# `$(if $(ENABLE_JAVA), desktop/source/migration/services/jvmfwk)` in
# Library_migrationoo2.mk's add_exception_objects. This port's equivalent
# switch is the real LIBRESTUDIO_ENABLE_JAVA option (root CMakeLists.txt,
# default OFF), so the source and its matching library link (further below)
# are wrapped in a real conditional rather than silently dropped -- matching
# the precedent set by sc/CMakeLists.txt's `if (ENABLE_SCRIPTING)` treatment
# of vbaobj.
if (LIBRESTUDIO_ENABLE_JAVA)
    list(APPEND MIGRATIONOO2_SOURCES
        desktop/source/migration/services/jvmfwk)
endif ()

librestudio_expand_cxx(MIGRATIONOO2_SOURCES)
add_library(migrationoo2 SHARED ${MIGRATIONOO2_SOURCES})

target_include_directories(migrationoo2 PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(migrationoo2 udkapi_normal_headers offapi_normal_headers)

target_link_libraries(migrationoo2 PRIVATE
    cppu cppuhelper i18nlangtag sal tl utl
    Boost::boost)

if (LIBRESTUDIO_ENABLE_JAVA)
    target_link_libraries(migrationoo2 PRIVATE jvmfwk)
endif ()

librestudio_add_component(migrationoo2
    COMPONENT "${CMAKE_SOURCE_DIR}/desktop/source/migration/services/migrationoo2.component"
    LIBRARY   migrationoo2
    LAYER     OOO)
librestudio_set_library_output_name(migrationoo2 migrationoo2 OOO)

install(TARGETS migrationoo2 LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# migrationoo3 -- OOo3 extension migration. 1 source.
# ---------------------------------------------------------------------------

set(MIGRATIONOO3_SOURCES
    desktop/source/migration/services/oo3extensionmigration
)

librestudio_expand_cxx(MIGRATIONOO3_SOURCES)
add_library(migrationoo3 SHARED ${MIGRATIONOO3_SOURCES})

target_include_directories(migrationoo3 PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(migrationoo3 udkapi_normal_headers offapi_normal_headers)

target_link_libraries(migrationoo3 PRIVATE
    comphelper cppu cppuhelper sal utl)

librestudio_add_component(migrationoo3
    COMPONENT "${CMAKE_SOURCE_DIR}/desktop/source/migration/services/migrationoo3.component"
    LIBRARY   migrationoo3
    LAYER     OOO)
librestudio_set_library_output_name(migrationoo3 migrationoo3 OOO)

install(TARGETS migrationoo3 LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# unopkgapp -- unopkg_bin's application logic (argument parsing, the
# command-line ConsoleInteractionHandler, and the bootstrap/shutdown glue).
# 3 sources. No .component file: Library_unopkgapp.mk has no
# gb_Library_set_componentfile call, confirmed against the .component
# listing above (no unopkgapp.component anywhere in the tree).
# ---------------------------------------------------------------------------

set(UNOPKGAPP_SOURCES
    desktop/source/pkgchk/unopkg/unopkg_app
    desktop/source/pkgchk/unopkg/unopkg_cmdenv
    desktop/source/pkgchk/unopkg/unopkg_misc
)

librestudio_expand_cxx(UNOPKGAPP_SOURCES)
add_library(unopkgapp SHARED ${UNOPKGAPP_SOURCES})

target_include_directories(unopkgapp PRIVATE
    "${CMAKE_SOURCE_DIR}/desktop/inc"
    "${CMAKE_SOURCE_DIR}/desktop/source/deployment/inc"
    "${CMAKE_SOURCE_DIR}/desktop/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(unopkgapp udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(unopkgapp PRIVATE DESKTOP_DLLIMPLEMENTATION)

target_link_libraries(unopkgapp PRIVATE
    comphelper cppu cppuhelper deploymentmisc sal tl ucbhelper utl vcl
    i18nlangtag
    Boost::boost)

librestudio_set_library_output_name(unopkgapp unopkgapp OOO)

install(TARGETS unopkgapp LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# soffice_bin -- THE application binary. Executable_soffice_bin.mk: one C
# source (main.c, a thin argv/argc shim into sofficeapp's SoDllMain-style
# entry point) linked against sal and sofficeapp.
#
# On-disk filename: RepositoryFixes.mk (line 40, the non-MACOSX/non-
# EMSCRIPTEN branch) renames soffice_bin -> soffice.bin; desktop/scripts/
# soffice.sh execs "$sd_prog/soffice.bin" directly, and
# desktop/unx/source/start.cxx (line 108) builds the same "soffice.bin"
# string when it looks for the binary to exec after showing the splash
# screen -- both confirm the required OUTPUT_NAME.
#
# Runtime bootstrap: appinit.cxx (compiled into sofficeapp, linked in here)
# calls cppu::defaultBootstrap_InitialComponentContext(), which -- per
# cppuhelper/source/paths.cxx -- looks for unorc beside libcppuhelpergcc3.so,
# and separately, sal/rtl/bootstrap.cxx's getIniFileName() resolves the
# process-wide default ini as "<program dir>/<executable name without
# extension>rc", i.e. "sofficerc" for an executable named soffice.bin. This
# is the same two-file split documented at length in vcl/CMakeLists.txt
# above svptest, and the same fix: give soffice_bin its own sofficerc via
# librestudio_generate_unorc(), matching real upstream LibreOffice installs
# (which ship program/sofficerc).
# ---------------------------------------------------------------------------

add_executable(soffice_bin
    "${CMAKE_SOURCE_DIR}/desktop/source/app/main.c")
set_target_properties(soffice_bin PROPERTIES OUTPUT_NAME "soffice.bin")

target_include_directories(soffice_bin PRIVATE
    "${CMAKE_SOURCE_DIR}/desktop/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(soffice_bin
    udkapi_normal_headers offapi_normal_headers gcc3_uno officecfg_registry)

target_link_libraries(soffice_bin PRIVATE sal sofficeapp)

librestudio_generate_unorc("${LIBRESTUDIO_PROGRAM_DIR}/sofficerc")

install(TARGETS soffice_bin RUNTIME DESTINATION program)

# ---------------------------------------------------------------------------
# unopkg_bin -- the unopkg command-line extension manager tool.
# Executable_unopkg_bin.mk: one C source (unopkg_main.c) linked against
# comphelper, sal, tl and unopkgapp.
#
# On-disk filename: RepositoryFixes.mk line 28 renames unopkg_bin ->
# unopkg.bin on every OS but MACOSX, which this Linux port is not.
#
# Runtime bootstrap: unopkg_misc.cxx (compiled into unopkgapp, linked in
# here) also calls cppu::defaultBootstrap_InitialComponentContext(), so this
# executable needs the same treatment as soffice_bin above. Executable name
# without extension is "unopkg" -> "unopkgrc".
# ---------------------------------------------------------------------------

add_executable(unopkg_bin
    "${CMAKE_SOURCE_DIR}/desktop/source/pkgchk/unopkg/unopkg_main.c")
set_target_properties(unopkg_bin PROPERTIES OUTPUT_NAME "unopkg.bin")

target_include_directories(unopkg_bin PRIVATE
    "${CMAKE_SOURCE_DIR}/desktop/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(unopkg_bin
    udkapi_normal_headers offapi_normal_headers gcc3_uno officecfg_registry)

target_link_libraries(unopkg_bin PRIVATE comphelper sal tl unopkgapp)

librestudio_generate_unorc("${LIBRESTUDIO_PROGRAM_DIR}/unopkgrc")

install(TARGETS unopkg_bin RUNTIME DESTINATION program)

# ---------------------------------------------------------------------------
# oosplash -- the small, dependency-light launcher that shows the splash
# bitmap and execs soffice.bin. Executable_oosplash.mk: 3 C sources
# (args, file_image_unx, splashx -- pagein.c is excluded, gb_Helper_optional
# under ENABLE_PAGEIN, which this port does not track) plus 1 C++ source
# (start, through librestudio_expand_cxx since add_cxxobjects is a bare
# path).
#
# start.cxx (line 261/265) reads its own bootstrap configuration via
# SAL_CONFIGFILE("bootstrap") + rtl_bootstrap_args_open(), i.e. an
# explicitly-named "bootstraprc" -- not the implicit
# defaultBootstrap_InitialComponentContext()/getIniFileName() resolution
# soffice_bin and unopkg_bin rely on -- and oosplash never calls
# defaultBootstrap_InitialComponentContext() itself (that grep comes back
# empty for this file), so it needs neither a .component file nor a
# librestudio_generate_unorc() call.
# ---------------------------------------------------------------------------

add_executable(oosplash
    "${CMAKE_SOURCE_DIR}/desktop/unx/source/args.c"
    "${CMAKE_SOURCE_DIR}/desktop/unx/source/file_image_unx.c"
    "${CMAKE_SOURCE_DIR}/desktop/unx/source/splashx.c"
    "${CMAKE_SOURCE_DIR}/desktop/unx/source/start.cxx")

target_link_libraries(oosplash PRIVATE comphelper sal m)

# `ifneq ($(USING_X11),)` in Executable_oosplash.mk.
if (USING_X11)
    pkg_check_modules(XINERAMA REQUIRED IMPORTED_TARGET xinerama)
    target_link_libraries(oosplash PRIVATE PkgConfig::XINERAMA)
endif ()

# `ifneq ($(DISABLE_GUI),TRUE)` in Executable_oosplash.mk: x11, the
# ENABLE_QUICKSTART_LIBPNG define, and libpng.
if (NOT DISABLE_GUI)
    pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)
    target_link_libraries(oosplash PRIVATE PkgConfig::X11)

    # png, resolved via librestudio_find_package() (cmake/LibreStudioTarballs.cmake).
    librestudio_find_package(NAME PNG)

    target_compile_definitions(oosplash PRIVATE ENABLE_QUICKSTART_LIBPNG)
    target_link_libraries(oosplash PRIVATE ${LIBRESTUDIO_PNG_TARGET})
endif ()

install(TARGETS oosplash RUNTIME DESTINATION program)

# --- Package_soffice_sh, Package_{sbase,scalc,sdraw,simpress,smath,swriter}_sh -
#
# See the module-level comment at the top of this file for the .mk citation.
foreach (_sh_name soffice sbase scalc sdraw simpress smath swriter)
    install(PROGRAMS "${CMAKE_SOURCE_DIR}/desktop/scripts/${_sh_name}.sh"
        DESTINATION program RENAME ${_sh_name})
endforeach ()

# --- UIConfig_deployment (registers internally as "desktop") -----------------
#
# desktop/UIConfig_deployment.mk: gb_UIConfig_UIConfig(desktop) -- the .mk
# FILENAME says "deployment", but the UIConfig it registers is named
# "desktop" (verified directly: `gb_UIConfig_UIConfig,desktop`), matching
# CppunitTest_desktop_dialogs_test.mk's own use_uiconfigs(desktop_dialogs_test,
# desktop) argument. 9 .ui files under desktop/uiconfig/ui/, unconditional
# (no ifeq/ifneq gates in the live .mk), all verified present on disk. Same
# "<module>_uiconfig" build-time ALL target convention as every other
# *_dialogs_test module.
set(DESKTOP_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/dependenciesdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/extensionmanager.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/extensionmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/installforalldialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/licensedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/showlicensedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/updatedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/updateinstalldialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/updaterequireddialog.ui")

install(FILES ${DESKTOP_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/desktop/ui)

set(_desktop_uiconfig_outputs "")
foreach (_desktop_uiconfig_src IN LISTS DESKTOP_UICONFIG_UI_FILES)
    get_filename_component(_desktop_uiconfig_name "${_desktop_uiconfig_src}" NAME)
    set(_desktop_uiconfig_dest "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/desktop/ui/${_desktop_uiconfig_name}")
    add_custom_command(
        OUTPUT "${_desktop_uiconfig_dest}"
        COMMAND ${CMAKE_COMMAND} -E copy "${_desktop_uiconfig_src}" "${_desktop_uiconfig_dest}"
        DEPENDS "${_desktop_uiconfig_src}"
        COMMENT "Copying ${_desktop_uiconfig_name} into build-tree soffice.cfg/desktop/ui")
    list(APPEND _desktop_uiconfig_outputs "${_desktop_uiconfig_dest}")
endforeach ()
unset(_desktop_uiconfig_src)
unset(_desktop_uiconfig_name)
unset(_desktop_uiconfig_dest)

add_custom_target(desktop_uiconfig ALL DEPENDS ${_desktop_uiconfig_outputs})
unset(_desktop_uiconfig_outputs)

# --- desktop_version (CppunitTest_desktop_version.mk) -------------------------
#
# Re-checked directly: 1 source (desktop/qa/deployment_misc/test_dp_version),
# use_libraries deploymentmisc sal, use_udk_api (udkapi only, "normal"
# variant). No use_ure/use_vcl/use_configuration/use_rdb at all in this .mk
# -- confirmed by reading the whole file -- so no PROTECTORS, no
# CONFIGURATION, and no program_services_rdb dependency: this test can be
# registered right here, inline, no deferral needed (officecfg/test are not
# even referenced).
librestudio_add_cppunit_test(desktop_version
    SOURCES
        desktop/qa/deployment_misc/test_dp_version
    LIBRARIES
        deploymentmisc sal
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers)

# --- desktop_app (CppunitTest_desktop_app.mk) ---------------------------------
#
# Re-checked directly: 1 source (desktop/qa/desktop_app/test_desktop_app),
# use_externals (ENABLE_BREAKPAD-gated breakpad -> off/untracked, dbus,
# icu_headers/icui18n/icuuc -> ICU::uc+ICU::i18n, ENABLE_ONLINE_UPDATE_MAR-
# gated curl/orcus-parser/orcus -> this port has no ENABLE_ONLINE_UPDATE_MAR
# switch, matching upstream's own bare-configure default of unset/off, so
# that whole branch is empty here too), use_libraries comphelper cppu
# cppuhelper (ENABLE_BREAKPAD-gated crashreport -> off) deploymentmisc
# editeng fwk i18nlangtag (OPENCL BUILD_TYPE-gated opencl -> this port's own
# ENABLE_OPENCL is a real, independent switch, default ON, so opencl IS
# linked here) sal salhelper sb sfx svl svx svxcore svt tk tl ucbhelper utl
# vcl, use_library_objects(sofficeapp) -- RECOMPILES sofficeapp's own
# sources directly into this test rather than linking the sofficeapp
# library (same technique already established for editeng_core), reusing
# desktop/CMakeLists.txt's own SOFFICEAPP_SOURCES variable (set earlier in
# this same file) rather than re-deriving the list, use_external(
# boost_headers), use_sdk_api. No use_ure call anywhere in this .mk
# (confirmed) -- no PROTECTORS. No use_configuration either -- no
# CONFIGURATION, no program_services_rdb dependency: wireable right here,
# inline.
# PkgConfig::DBUS is created without GLOBAL by sofficeapp's own
# pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1) above, scoped to
# this directory + children -- not visible from CppunitTest_deferred.cmake's
# own root-directory scope (same re-guard pattern already used throughout
# this tree for PkgConfig::CPPUNIT and friends). Found missing via the real
# remote build: officeipcthread.cxx (part of ${SOFFICEAPP_SOURCES}, recompiled
# here) needs it directly -- "fatal error: dbus/dbus.h: No such file".
if (NOT TARGET PkgConfig::DBUS)
    pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)
endif ()

librestudio_add_cppunit_test(desktop_app
    SOURCES
        desktop/qa/desktop_app/test_desktop_app
        ${SOFFICEAPP_SOURCES}
    LIBRARIES
        comphelper cppu cppuhelper deploymentmisc editeng fwk i18nlangtag
        sal salhelper sb sfx svl svx svxcore svt tk tl ucbhelper utl vcl
        ICU::uc ICU::i18n Boost::boost PkgConfig::DBUS
    INCLUDE
        # 2026-09-29, found via the real remote build: use_library_objects(
        # sofficeapp) recompiles sofficeapp's OWN sources here, so it needs
        # sofficeapp's own PRIVATE include dirs too (target_include_
        # directories(sofficeapp PRIVATE ...) just above), not only its
        # linked LIBRARIES -- missed originally, causing real "fatal error:
        # app.hxx/migration.hxx/dp_shared.hxx/officecfg/Setup.hxx: No such
        # file or directory" (all 4 real headers these recompiled sources
        # reach, confirmed by grepping each's real location).
        "${CMAKE_SOURCE_DIR}/desktop/inc"
        "${CMAKE_SOURCE_DIR}/desktop/source/inc"
        "${CMAKE_SOURCE_DIR}/desktop/source/deployment/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEFINITIONS
        DESKTOP_DLLIMPLEMENTATION
    DEPENDS
        officecfg_headers udkapi_normal_headers offapi_normal_headers)
if (ENABLE_OPENCL)
    target_link_libraries(desktop_app PRIVATE clew opencl)
endif ()

# --- desktop_lok_init (CppunitTest_desktop_lokinit.mk) ------------------------
#
# Re-checked directly: 1 source (desktop/qa/unit/desktop-lok-init),
# use_external(boost_headers), use_sdk_api, use_ure -> 2 protector pairs (no
# use_vcl call in this .mk -- confirmed, so no vclbootstrapprotector),
# set_include(-Idesktop/source/inc -Idesktop/inc), use_libraries comphelper
# cppu sal sofficeapp vcl ($(gb_UWINAPI) is empty on this Linux port -- a
# Windows-only linker-flag macro, confirmed by grepping solenv/gbuild for
# its Linux-platform definition, which is blank), Linux-only add_libs(-lm
# -ldl), use_configuration -> CONFIGURATION. CONFIGURATION needs officecfg_
# registry (add_subdirectory()'d well before desktop's own position) and
# LIBRESTUDIO_TEST_UNITTEST_DIR (test/CMakeLists.txt, also well before) --
# both satisfied already, no program_services_rdb dependency (no use_rdb
# call in this .mk), so no deferral needed here either.
librestudio_add_cppunit_test(desktop_lok_init
    SOURCES
        desktop/qa/unit/desktop-lok-init
    LIBRARIES
        comphelper cppu sal sofficeapp vcl m dl Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/desktop/source/inc"
        "${CMAKE_SOURCE_DIR}/desktop/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
    CONFIGURATION)

# desktop/CppunitTest_deferred.cmake needs data (program_services_rdb) from a
# module add_subdirectory()'d later than this one -- scheduled here rather
# than included directly, so it runs only once every add_subdirectory() in
# the whole tree has completed. See that file's own header comment for the
# specific constraint, and cmake/LibreStudioTest.cmake's header comment for
# why CMAKE_SOURCE_DIR (not CMAKE_CURRENT_SOURCE_DIR) is required in the
# path argument below.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/desktop/CppunitTest_deferred.cmake")
