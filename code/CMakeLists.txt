# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# "Code" -- a future Visual-Studio-like IDE app, recorded in memory as
# project_code_ide. This is original LibreStudio code, not a port of any
# upstream gbuild module -- there is no Module_code.mk anywhere to
# transcribe, so it has no row in PORTING_STATUS.md.
#
# v2: real sfx2/framework shell (SfxObjectShell/SfxViewShell pair, declarative
# menubar/toolbar/statusbar from uiconfig XML, dispatched through a real
# SfxDispatcher via a compiled .sdi slot table) instead of v1's hand-coded
# VCL MenuBar/ToolBox/StatusBar. Modelled directly on basctl -- the only
# other SFX app in this tree -- but simplified: code::DocShell is
# constructed directly (not via a dlopen()able UNO component) and handed to
# the FrameLoader with an already-built model, the same way basctl's own
# SID_BASICIDE_APPEAR does (sfx2/source/appl/appserv.cxx), so "code" stays a
# plain add_executable() -- no librestudio_add_component() needed, it is not
# a UNO service. Confirmed empirically feasible outside soffice.bin by
# building and running svx's own gengal executable, which does the same
# "link sfx directly, call SfxApplication::GetOrCreate() from main()" thing.

librestudio_add_sdi_target(code_sdi
    SDI        "${CMAKE_SOURCE_DIR}/code/sdi/codeslots.sdi"
    EXPORTS    "${CMAKE_SOURCE_DIR}/code/sdi/code.sdi"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/SdiTarget/code/sdi"
    INCLUDES   "${CMAKE_SOURCE_DIR}/code/inc" "${CMAKE_SOURCE_DIR}/svx/sdi" "${CMAKE_SOURCE_DIR}/sfx2/sdi")

add_executable(code
    "${CMAKE_SOURCE_DIR}/code/source/codeapp.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/configurationcontrol.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/configureprojectdialog.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/codedocshell.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/codeviewshell.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/editor/codeeditview.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/editormdipanel.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/finddialog.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/gitbranchespopover.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/gotosymboldialog.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/optionsdialog.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/optionspages.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/tasksstatusbarcontrol.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/recentprojects.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/uibridge.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/vcsstatusbarcontrol.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/workbench.cxx"
    # The Output dock's own SfxChildWindow/SfxDockingWindow/IDeckHost
    # trio -- unlike codesidebar's panel factory below, these are
    # constructed directly (RegisterChildWindow/ShowChildWindow in
    # codeapp.cxx), not UNO-discovered, so they link straight into the
    # executable the same way codeviewshell.cxx/editorarea.cxx do.
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/outputchildwindow.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/outputcontroller.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/outputdockingwindow.cxx")

target_compile_definitions(code PRIVATE VCL_INTERNALS)

target_include_directories(code PRIVATE
    "${CMAKE_SOURCE_DIR}/code/source/services"
    "${CMAKE_SOURCE_DIR}/code/inc"
    "${CMAKE_SOURCE_DIR}/code/source"
    "${CMAKE_SOURCE_DIR}/code/sdi"
    "${CMAKE_SOURCE_DIR}/vcl/inc"
    "${code_sdi_INCLUDE_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${codeapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(code code_sdi udkapi_normal_headers offapi_normal_headers codeapi_normal_headers gcc3_uno officecfg_registry officecfg_headers program_services_rdb
    # Runtime-only UNO components/build-tree assets "code" never links
    # against but silently needs to actually run without crashing or
    # missing content -- each one found and built ad hoc during a single
    # debugging session by reproducing the crash and reading the resulting
    # exception/log. Wiring them here means a plain `cmake --build . --
    # target code` from a fresh configure rebuilds all of them too, instead
    # of needing every one of those crashes to be rediscovered by hand
    # again next time (see project memory for the full list of how each
    # one was found):
    #  - uui:           "Open" dialog's InteractionHandler
    #  - filterconfig:  "Open" dialog's type/filter detection
    #  - i18nsearch:    toolbar Customize dialog's search box
    #  - vcl_uiconfig:  vcl's own generic .ui files (combobox.ui et al.),
    #                   used internally by ordinary widgets, not just code's
    #                   own uiconfig
    #  - package2:      ZipFileAccess, which vcl's own ImageTree needs to
    #                   read *any* icon out of *any* icon-theme zip -- so
    #                   this one was silently breaking every icon in the
    #                   app, not just the sidebar's, the whole session
    #  - cui:           Tools > Options (and every other cui-hosted dialog)
    #                   -- dlopened on first use; found via a crash after a
    #                   weld::Builder vtable change (new weld_mdi() slot)
    #                   left a stale cui mis-dispatching virtual calls,
    #                   because `ninja code` never rebuilt it
    #  - vclplug_gtk3/  the actual GUI backends: dlopened by vcl at startup
    #    vclplug_qt6:   based on SAL_USE_VCLPLUGIN/desktop detection, never
    #                   linked -- without them here, `ninja code` happily
    #                   reports "no work to do" after an edit inside
    #                   vcl/qt5/ or vcl/unx/gtk3/
    uui filterconfig i18nsearch vcl_uiconfig package2 cui)

# What the generated unorc points UNO_TYPES/UNO_SERVICES at (cmake/
# LibreStudioUnorc.cmake): without these registries the executable links but
# stops in the bootstrap.
add_dependencies(code program_types_rdb program_offapi_rdb program_oovbaapi_rdb
    program_codeapi_rdb program_services_rdb)

if (ENABLE_GTK3)
    add_dependencies(code vclplug_gtk3)
endif()
if (ENABLE_QT6)
    add_dependencies(code vclplug_qt6)
endif()
if (ENABLE_WIN)
    add_dependencies(code vclplug_win)
endif()

target_link_libraries(code PRIVATE Boost::boost ICU::uc ICU::i18n
    comphelper cppu cppuhelper sal sfx fwk svxcore svx svl svt tk tl ucbhelper utl vcl vclmain)

# Same reason every vcl workbench program above gets one: UNO/configmgr
# bootstrap reads CONFIGURATION_LAYERS from "<program name>rc", not unorc.
librestudio_generate_unorc("${LIBRESTUDIO_PROGRAM_DIR}/coderc")

install(TARGETS code RUNTIME DESTINATION program)

# --- codesidebar -----------------------------------------------------------
#
# The real Sidebar's ResourceManager (sfx2/source/sidebar/ResourceManager.cxx)
# instantiates a panel's content dynamically, by UNO service name, the
# moment the user actually shows that panel -- unlike code::DocShell
# (constructed directly in codeapp.cxx, never UNO-discovered), a panel
# factory has to be a real, dlopen()able UNO component, so unlike "code"
# itself this is a normal librestudio_add_component()-registered SHARED
# library, discovered at runtime via services.rdb/main.xcd exactly like
# uui/filterconfig -- "code" (the executable) does not link it.
add_library(codesidebar SHARED
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/codepanelfactory.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/errorlistpanel.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/findresultspanel.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/gitchangespanel.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/outputlogpanel.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/recentprojects.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/recentprojectsmenucontroller.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/folderviewpanel.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/projectpropertiesdialog.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/projecttreepanel.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/sidebar/solutionpanel.cxx"
    # The panels listen to the services through the same UI bridge the exe
    # uses; a second copy here (rather than a shared library) because it is
    # one small file and the two sides never exchange its objects.
    "${CMAKE_SOURCE_DIR}/code/source/uibridge.cxx")

target_include_directories(codesidebar PRIVATE
    "${CMAKE_SOURCE_DIR}/code/source"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${codeapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(codesidebar udkapi_normal_headers offapi_normal_headers codeapi_normal_headers)

target_link_libraries(codesidebar PRIVATE Boost::boost
    comphelper cppu cppuhelper sal sfx svl svt tk tl ucbhelper vcl)

librestudio_add_component(codesidebar
    COMPONENT "${CMAKE_SOURCE_DIR}/code/util/codesidebar.component"
    LIBRARY   codesidebar
    LAYER     OOO)

librestudio_set_library_output_name(codesidebar codesidebar OOO)

install(TARGETS codesidebar LIBRARY DESTINATION program)

# Sibling runtime-only dependency (see the long comment on "code"'s own
# add_dependencies() above) -- not something "code" links against, but
# building it is what makes the Solution sidebar panel actually work
# instead of silently doing nothing.
add_dependencies(code codesidebar)

# --- codeservices ----------------------------------------------------------
#
# The backend of "Code": the singletons of the com.sun.star.code.* API
# (codeapi) -- project, document, output, task, job registry -- and the
# first-wave plugins that attach to them (compiler-output filter, "open in
# editor" task handler). One SHARED component library for all of them: they
# are small, share common.cxx, and unlike deepcode's library-per-service
# layout nothing here is meant to be loaded on its own. Like codesidebar it is
# discovered through services.rdb, never linked by "code".
# The helpers every backend library needs (URL handling, MIME types, the
# folder project): one static library linked into each, so that the format
# plugins can live in libraries of their own.
add_library(codecommon STATIC
    "${CMAKE_SOURCE_DIR}/code/source/services/backgroundjob.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/common.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/folderproject.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/process.cxx")
set_target_properties(codecommon PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(codecommon PUBLIC
    "${CMAKE_SOURCE_DIR}/code/source/services"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${codeapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(codecommon udkapi_normal_headers offapi_normal_headers codeapi_normal_headers)
target_link_libraries(codecommon PUBLIC comphelper cppu cppuhelper sal)

# The descriptions that come with Code, and the schema of their format.
set(CODE_TOOL_DESCRIPTIONS
    "${CMAKE_CURRENT_SOURCE_DIR}/tooldescriptions/clang.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/tooldescriptions/code.tool-1.xsd"
    "${CMAKE_CURRENT_SOURCE_DIR}/tooldescriptions/gcc.xml")
install(FILES ${CODE_TOOL_DESCRIPTIONS} DESTINATION share/code/tooldescriptions)

add_library(codeservices SHARED
    "${CMAKE_SOURCE_DIR}/code/source/services/buildcontroller.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/code.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/compilecommands.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/compileroutputfilter.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/documentmanager.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/editoropenhandler.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/finder.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/gitvcs.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/jobregistry.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/outputcontroller.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/projectconfigstore.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/projectformatregistry.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/projectmanager.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/semantictokencontroller.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/symbolindex.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/symbolstore.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/taskhub.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/tooldescription.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/tooldescriptions.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/services/vcsmanager.cxx")

target_include_directories(codeservices PRIVATE
    "${CMAKE_SOURCE_DIR}/code/source/services"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${codeapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(codeservices udkapi_normal_headers offapi_normal_headers
    codeapi_normal_headers)

target_link_libraries(codeservices PRIVATE codecommon comphelper cppu cppuhelper sal ICU::uc ICU::i18n)

librestudio_add_component(codeservices
    COMPONENT "${CMAKE_SOURCE_DIR}/code/util/codeservices.component"
    LIBRARY   codeservices
    LAYER     OOO)

librestudio_set_library_output_name(codeservices codeservices OOO)

install(TARGETS codeservices LIBRARY DESTINATION program)

# Runtime-only, like codesidebar: building "code" must also build the services
# it asks for by name, and install the registry that names their types.
add_dependencies(code codeservices program_codeapi_rdb)

# --- cmakeproj -------------------------------------------------------------
#
# Everything that is CMake in "Code", in one library: the project-format
# reader (a plugin that registers itself with the format registry) and the
# panel in which a CMake project is configured. Kept out of codeservices
# because the panel is VCL user interface, and codeservices has no GUI
# dependency (the headless tests use it). Like codesidebar this is discovered
# through services.rdb, never linked by "code". Autotools will get a library
# of its own the same way.
add_library(cmakeproj SHARED
    "${CMAKE_SOURCE_DIR}/code/source/cmake/cmakebuild.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/cmake/cmakeconfigpanel.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/cmake/cmakeformatreader.cxx"
    "${CMAKE_SOURCE_DIR}/code/source/cmake/cmakeproject.cxx")

target_include_directories(cmakeproj PRIVATE "${CMAKE_SOURCE_DIR}/code/source/cmake")
target_link_libraries(cmakeproj PRIVATE Boost::boost codecommon comphelper cppu cppuhelper sal tl vcl)

librestudio_add_component(cmakeproj
    COMPONENT "${CMAKE_SOURCE_DIR}/code/util/cmakeproj.component"
    LIBRARY   cmakeproj
    LAYER     OOO)

librestudio_set_library_output_name(cmakeproj cmakeproj OOO)

install(TARGETS cmakeproj LIBRARY DESTINATION program)

add_dependencies(code cmakeproj)

# --- codeclang -------------------------------------------------------------
#
# The semantic-token and symbol providers for C and C++, on libclang.  A library of its own
# so that libclang is a dependency of this one plugin only, and optional: where
# it is not found Code simply has no highlighting for C and C++.  Discovered
# through services.rdb, like cmakeproj.
file(GLOB _llvm_roots LIST_DIRECTORIES true "/usr/lib/llvm-*" "/usr/lib64/llvm*")
find_path(LIBCLANG_INCLUDE_DIR clang-c/Index.h
    HINTS ${_llvm_roots}
    PATH_SUFFIXES include)
find_library(LIBCLANG_LIBRARY NAMES clang libclang.so.19.1 libclang.so.20.1 libclang.so.21.1
    HINTS ${_llvm_roots}
    PATH_SUFFIXES lib lib64)
if (LIBCLANG_INCLUDE_DIR AND LIBCLANG_LIBRARY)
    add_library(codeclang SHARED
        "${CMAKE_SOURCE_DIR}/code/source/clang/clangsemantictokens.cxx"
        "${CMAKE_SOURCE_DIR}/code/source/clang/clangsymbols.cxx")
    target_include_directories(codeclang PRIVATE
        "${LIBCLANG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${codeapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(codeclang udkapi_normal_headers offapi_normal_headers codeapi_normal_headers)
    target_link_libraries(codeclang PRIVATE "${LIBCLANG_LIBRARY}" comphelper cppu cppuhelper sal)

    librestudio_add_component(codeclang
        COMPONENT "${CMAKE_SOURCE_DIR}/code/util/codeclang.component"
        LIBRARY   codeclang
        LAYER     OOO)

    librestudio_set_library_output_name(codeclang codeclang OOO)

    install(TARGETS codeclang LIBRARY DESTINATION program)

    add_dependencies(code codeclang)
else()
    message(STATUS "libclang not found: Code will not highlight C and C++")
endif()

# --- UIConfig_code -------------------------------------------------------
#
# Mirrors basctl/CMakeLists.txt's own UIConfig_basicide block: installs
# code/uiconfig/code/{menubar,toolbar,statusbar}/*.xml into
# share/config/soffice.cfg/modules/Code/{menubar,toolbar,statusbar} -- the
# exact directory framework's PresetHandler resolves at runtime from
# ooSetupFactoryShortName=Code (officecfg/registry/data/org/openoffice/
# Setup.xcu's own new "com.sun.star.code.CodeDocument" node).
set(CODE_UICONFIG_MENUBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/menubar/menubar.xml"
)
set(CODE_UICONFIG_TOOLBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/toolbar/standardbar.xml"
)
set(CODE_UICONFIG_STATUSBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/statusbar/statusbar.xml"
)

install(FILES ${CODE_UICONFIG_MENUBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/Code/menubar)
install(FILES ${CODE_UICONFIG_TOOLBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/Code/toolbar)
install(FILES ${CODE_UICONFIG_STATUSBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/Code/statusbar)

set(_code_uiconfig_outputs "")
foreach (_code_uiconfig_pair
        "CODE_UICONFIG_MENUBAR_FILES:menubar"
        "CODE_UICONFIG_TOOLBAR_FILES:toolbar"
        "CODE_UICONFIG_STATUSBAR_FILES:statusbar")
    string(REPLACE ":" ";" _code_uiconfig_pair "${_code_uiconfig_pair}")
    list(GET _code_uiconfig_pair 0 _code_uiconfig_var)
    list(GET _code_uiconfig_pair 1 _code_uiconfig_category)
    foreach (_code_uiconfig_src IN LISTS ${_code_uiconfig_var})
        get_filename_component(_code_uiconfig_name "${_code_uiconfig_src}" NAME)
        set(_code_uiconfig_dest
            "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/Code/${_code_uiconfig_category}/${_code_uiconfig_name}")
        add_custom_command(
            OUTPUT "${_code_uiconfig_dest}"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/Code/${_code_uiconfig_category}"
            COMMAND ${CMAKE_COMMAND} -E copy "${_code_uiconfig_src}" "${_code_uiconfig_dest}"
            DEPENDS "${_code_uiconfig_src}"
            COMMENT "Copying ${_code_uiconfig_name} into build-tree soffice.cfg/modules/Code/${_code_uiconfig_category}"
            VERBATIM)
        list(APPEND _code_uiconfig_outputs "${_code_uiconfig_dest}")
    endforeach ()
endforeach ()

# solutionpanel.ui isn't module uiconfig (menubar/toolbar/statusbar under
# .../modules/Code/) -- it's a weld .ui a sidebar panel loads by a plain
# "code/ui/<file>.ui" path (PanelLayout's rUIXMLDescription argument,
# code/source/sidebar/solutionpanel.cxx), resolved by AllSettings::
# GetUIRootDir() (vcl/source/window/builder.cxx) straight against
# share/config/soffice.cfg/ -- same "<module>/ui/" convention every other
# module's own weld .ui files use (e.g. svx/ui/sidebarempty.ui).
set(CODE_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/branchespopover.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/cmakeconfigpanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/configurationbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/configureprojectdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/editordocumentcontent.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/editormdi.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/errorlistpanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/folderviewpanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/finddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/findresultspanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/findresultspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/findpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/gitchangesmenus.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/gitchangespanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/gotosymboldialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/newbranchdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/optionsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/optionspage_editorfont.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/optionspage_editorgeneral.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/outputpanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/projectpropertiesdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/pushdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/solutionpanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/code/ui/taskspopover.ui"
)
install(FILES ${CODE_UI_FILES} DESTINATION share/config/soffice.cfg/code/ui)
foreach (_code_ui_src IN LISTS CODE_UI_FILES)
    get_filename_component(_code_ui_name "${_code_ui_src}" NAME)
    set(_code_ui_dest "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/code/ui/${_code_ui_name}")
    add_custom_command(
        OUTPUT "${_code_ui_dest}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/code/ui"
        COMMAND ${CMAKE_COMMAND} -E copy "${_code_ui_src}" "${_code_ui_dest}"
        DEPENDS "${_code_ui_src}"
        COMMENT "Copying ${_code_ui_name} into build-tree soffice.cfg/code/ui"
        VERBATIM)
    list(APPEND _code_uiconfig_outputs "${_code_ui_dest}")
endforeach ()

# the tool descriptions, where the services look for them: share/code/tooldescriptions
foreach (_code_tool_src IN LISTS CODE_TOOL_DESCRIPTIONS)
    get_filename_component(_code_tool_name "${_code_tool_src}" NAME)
    set(_code_tool_dest "${LIBRESTUDIO_SHARE_DIR}/code/tooldescriptions/${_code_tool_name}")
    add_custom_command(
        OUTPUT "${_code_tool_dest}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${LIBRESTUDIO_SHARE_DIR}/code/tooldescriptions"
        COMMAND ${CMAKE_COMMAND} -E copy "${_code_tool_src}" "${_code_tool_dest}"
        DEPENDS "${_code_tool_src}"
        COMMENT "Copying ${_code_tool_name} into build-tree share/code/tooldescriptions"
        VERBATIM)
    list(APPEND _code_uiconfig_outputs "${_code_tool_dest}")
endforeach ()

add_custom_target(code_uiconfig ALL DEPENDS ${_code_uiconfig_outputs})

# The tests need program_services_rdb, which exists only after the whole tree
# has been processed.
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/code/CppunitTest_deferred.cmake")
