# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# scripting — the macro/scripting-provider framework (Tools > Macros): the
# generic ScriptProvider/BrowseNodeFactory machinery (scriptframe), the
# single-script-file provider used by pyprov/jsprov (singleprov), the
# "vnd.sun.star.script:" URL dispatcher (protocolhandler), the dialog
# .properties string-resource reader used by the Basic IDE dialog editor
# (stringresource), and the QuickJS-based JavaScript script provider
# (jsprov, Library_jsprov.mk) -- despite its name this needs no JDK: jsuno,
# which it links, embeds QuickJS rather than a JVM/Rhino (see
# jsuno/CMakeLists.txt), so jsprov is included below like any other library.
#
# All 8 of the module's gb_Library targets satisfy the "every
# gb_Library_use_libraries/use_external entry already ported" scoping rule
# used elsewhere in this port (see connectivity/CMakeLists.txt for the
# precedent).
#
# vbaevents (Library_vbaevents.mk) needs msfilter, which IS now ported (the
# "msfilter" library lives under the filter/ module directory, see
# filter/CMakeLists.txt) — so vbaevents is included below, linking msfilter,
# sb, sfx and the oovbaapi private API headers the same way basic/
# CMakeLists.txt already consumes them for sb.
#
# dlgprov (Library_dlgprov.mk) and basprov (Library_basprov.mk) both
# gb_Library_use_libraries ... vbahelper ...; vbahelper has now been ported
# (see vbahelper/CMakeLists.txt), so both are included below too. dlgprov
# also links xmlscript (now ported, see xmlscript/CMakeLists.txt) and uses
# the oovbaapi private API the same way vbaevents does; basprov only uses
# gb_Library_use_sdk_api (offapi + udkapi, no oovbaapi). Both are registered
# in the OOO layer, matching Repository.mk's
# gb_Helper_register_libraries_for_install(OOOLIBS,ooo,...) (both listed
# there under gb_Helper_optional(SCRIPTING,...)).
#
# Source lists, includes and library sets below are transcribed from each
# library's Library_*.mk. Only protocolhandler (Library_protocolhandler.mk)
# actually calls gb_Library_use_custom_headers(...,officecfg/registry), so it
# is the only library here wiring OFFICECFG_INCLUDE_DIR/officecfg_headers;
# none of the other seven libraries' .mk files call it.
#
# Module_scripting.mk wraps its ENTIRE gb_Module_add_targets list (all 8
# libraries above, jsprov included -- it sits inside the same
# `ifneq ($(filter SCRIPTING,$(BUILD_TYPE)),)` block as the rest, ENABLE_QUICKJS
# is a separate, narrower gate within it) in that one SCRIPTING check. So the
# whole body below is wrapped in `if (ENABLE_SCRIPTING)`, which defaults ON.
if (ENABLE_SCRIPTING)

set(SCRIPTFRAME_SOURCES
    scripting/source/provider/ActiveMSPList
    scripting/source/provider/BrowseNodeFactoryImpl
    scripting/source/provider/MasterScriptProvider
    scripting/source/provider/MasterScriptProviderFactory
    scripting/source/provider/ProviderCache
    scripting/source/provider/URIHelper
)

librestudio_expand_cxx(SCRIPTFRAME_SOURCES)
add_library(scriptframe SHARED ${SCRIPTFRAME_SOURCES})

target_include_directories(scriptframe PRIVATE
    "${CMAKE_SOURCE_DIR}/scripting/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(scriptframe udkapi_normal_headers offapi_normal_headers)

target_link_libraries(scriptframe PRIVATE
    comphelper cppu cppuhelper sal tl ucbhelper utl
    Boost::boost)

librestudio_add_component(scriptframe
    COMPONENT "${CMAKE_SOURCE_DIR}/scripting/util/scriptframe.component"
    LIBRARY   scriptframe
    LAYER     OOO)

librestudio_set_library_output_name(scriptframe scriptframe OOO)

install(TARGETS scriptframe LIBRARY DESTINATION program)

# singleprov has no gb_Library_set_componentfile call in Library_singleprov.mk
# -- it is not a UNO service on its own, only a helper library linked by
# jsprov/pyprov's script providers -- so no .component is registered for it.
# gb_Library_use_system_win32_libs (shell32, shlwapi) is Windows-only and does
# not apply on this Linux build.
set(SINGLEPROV_SOURCES
    scripting/source/singleprov/externaledit
    scripting/source/singleprov/provcontext
    scripting/source/singleprov/scriptdir
    scripting/source/singleprov/scriptfile
    scripting/source/singleprov/scriptmacro
    scripting/source/singleprov/scriptprovider
)

librestudio_expand_cxx(SINGLEPROV_SOURCES)
add_library(singleprov SHARED ${SINGLEPROV_SOURCES})

target_include_directories(singleprov PRIVATE
    "${CMAKE_SOURCE_DIR}/scripting/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(singleprov udkapi_normal_headers offapi_normal_headers)

target_link_libraries(singleprov PRIVATE
    comphelper cppu cppuhelper sal)

librestudio_set_library_output_name(singleprov singleprov OOO)

install(TARGETS singleprov LIBRARY DESTINATION program)

set(PROTOCOLHANDLER_SOURCES
    scripting/source/protocolhandler/scripthandler
)

librestudio_expand_cxx(PROTOCOLHANDLER_SOURCES)
add_library(protocolhandler SHARED ${PROTOCOLHANDLER_SOURCES})

target_include_directories(protocolhandler PRIVATE
    "${CMAKE_SOURCE_DIR}/scripting/source/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(protocolhandler officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(protocolhandler PRIVATE
    comphelper cppu cppuhelper fwk sal sfx tl utl vcl
    Boost::boost)

librestudio_add_component(protocolhandler
    COMPONENT "${CMAKE_SOURCE_DIR}/scripting/source/protocolhandler/protocolhandler.component"
    LIBRARY   protocolhandler
    LAYER     OOO)

librestudio_set_library_output_name(protocolhandler protocolhandler OOO)

install(TARGETS protocolhandler LIBRARY DESTINATION program)

set(STRINGRESOURCE_SOURCES
    scripting/source/stringresource/stringresource
)

librestudio_expand_cxx(STRINGRESOURCE_SOURCES)
add_library(stringresource SHARED ${STRINGRESOURCE_SOURCES})

target_include_directories(stringresource PRIVATE
    "${CMAKE_SOURCE_DIR}/scripting/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(stringresource udkapi_normal_headers offapi_normal_headers)

target_link_libraries(stringresource PRIVATE
    comphelper cppu cppuhelper sal utl tl i18nlangtag
    Boost::boost)

librestudio_add_component(stringresource
    COMPONENT "${CMAKE_SOURCE_DIR}/scripting/source/stringresource/stringresource.component"
    LIBRARY   stringresource
    LAYER     OOO)

librestudio_set_library_output_name(stringresource stringresource OOO)

install(TARGETS stringresource LIBRARY DESTINATION program)

set(VBAEVENTS_SOURCES
    scripting/source/vbaevents/eventhelper
)

librestudio_expand_cxx(VBAEVENTS_SOURCES)
add_library(vbaevents SHARED ${VBAEVENTS_SOURCES})

target_include_directories(vbaevents PRIVATE
    "${CMAKE_SOURCE_DIR}/scripting/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(vbaevents
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers)

target_link_libraries(vbaevents PRIVATE
    comphelper cppu cppuhelper msfilter sal sb sfx tl
    Boost::boost)

librestudio_add_component(vbaevents
    COMPONENT "${CMAKE_SOURCE_DIR}/scripting/source/vbaevents/vbaevents.component"
    LIBRARY   vbaevents
    LAYER     OOO)

librestudio_set_library_output_name(vbaevents vbaevents OOO)

install(TARGETS vbaevents LIBRARY DESTINATION program)

set(DLGPROV_SOURCES
    scripting/source/dlgprov/DialogModelProvider
    scripting/source/dlgprov/dlgevtatt
    scripting/source/dlgprov/dlgprov
)

librestudio_expand_cxx(DLGPROV_SOURCES)
add_library(dlgprov SHARED ${DLGPROV_SOURCES})

target_include_directories(dlgprov PRIVATE
    "${CMAKE_SOURCE_DIR}/scripting/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(dlgprov
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers)

target_link_libraries(dlgprov PRIVATE
    comphelper cppu cppuhelper sal i18nlangtag sb sfx tl ucbhelper utl vbahelper vcl xmlscript
    Boost::boost)

librestudio_add_component(dlgprov
    COMPONENT "${CMAKE_SOURCE_DIR}/scripting/source/dlgprov/dlgprov.component"
    LIBRARY   dlgprov
    LAYER     OOO)

librestudio_set_library_output_name(dlgprov dlgprov OOO)

install(TARGETS dlgprov LIBRARY DESTINATION program)

set(BASPROV_SOURCES
    scripting/source/basprov/baslibnode
    scripting/source/basprov/basmethnode
    scripting/source/basprov/basmodnode
    scripting/source/basprov/basprov
    scripting/source/basprov/basscript
)

librestudio_expand_cxx(BASPROV_SOURCES)
add_library(basprov SHARED ${BASPROV_SOURCES})

target_include_directories(basprov PRIVATE
    "${CMAKE_SOURCE_DIR}/scripting/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(basprov udkapi_normal_headers offapi_normal_headers)

target_link_libraries(basprov PRIVATE
    comphelper cppu cppuhelper sal sb sfx svl tl ucbhelper vcl
    Boost::boost)

librestudio_add_component(basprov
    COMPONENT "${CMAKE_SOURCE_DIR}/scripting/source/basprov/basprov.component"
    LIBRARY   basprov
    LAYER     OOO)

librestudio_set_library_output_name(basprov basprov OOO)

install(TARGETS basprov LIBRARY DESTINATION program)

set(JSPROV_SOURCES
    scripting/source/jsprov/loader
)

librestudio_expand_cxx(JSPROV_SOURCES)
add_library(jsprov SHARED ${JSPROV_SOURCES})

target_include_directories(jsprov PRIVATE
    "${CMAKE_SOURCE_DIR}/scripting/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(jsprov udkapi_normal_headers offapi_normal_headers)

target_link_libraries(jsprov PRIVATE
    comphelper cppu cppuhelper jsuno sal singleprov)

librestudio_add_component(jsprov
    COMPONENT "${CMAKE_SOURCE_DIR}/scripting/source/jsprov/jsprov.component"
    LIBRARY   jsprov
    LAYER     OOO)

librestudio_set_library_output_name(jsprov jsprov OOO)

install(TARGETS jsprov LIBRARY DESTINATION program)

# --- Package_scriptbindinglib -----------------------------------------------
#
# scripting/Package_scriptbindinglib.mk, read in full: gb_Package_Package(
# scriptbindinglib,$(SRCDIR)/scripting/workben/bindings) + gb_Package_
# add_files(...,$(LIBO_SHARE_FOLDER)/basic/ScriptBindingLibrary, Highlight.xdl
# MacroEditor.xdl dialog.xlb script.xlb) -- 4 plain, checked-in files (all 4
# verified present on disk), unconditional within Module_scripting.mk's own
# Package_scriptbindinglib entry (listed outside the `$(if $(ENABLE_JAVA),
# ...)` block that entry sits next to, so this module builds it whenever
# scripting itself is included at all -- matching this port's own
# `if (ENABLE_SCRIPTING)` gate around this whole file, not a narrower one).
# CppunitTest_desktop_lib.mk's own use_packages list names this
# "scripting_scriptbindinglib" (gbuild's own <module>_<packagename>
# convention -- the "scriptbindinglib" argument to gb_Package_Package is
# just this package's own second half); previously zero install() rule
# existed for this content anywhere in this port (confirmed by grep before
# adding this section) -- ported here for real now, giving PACKAGES a real
# build-time target under the exact name desktop_lib needs.
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/workben/bindings/Highlight.xdl"
    "${CMAKE_CURRENT_SOURCE_DIR}/workben/bindings/MacroEditor.xdl"
    "${CMAKE_CURRENT_SOURCE_DIR}/workben/bindings/dialog.xlb"
    "${CMAKE_CURRENT_SOURCE_DIR}/workben/bindings/script.xlb"
    DESTINATION share/basic/ScriptBindingLibrary)

librestudio_add_package_mirror(scripting_scriptbindinglib
    PAIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/workben/bindings/Highlight.xdl:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptBindingLibrary"
        "${CMAKE_CURRENT_SOURCE_DIR}/workben/bindings/MacroEditor.xdl:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptBindingLibrary"
        "${CMAKE_CURRENT_SOURCE_DIR}/workben/bindings/dialog.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptBindingLibrary"
        "${CMAKE_CURRENT_SOURCE_DIR}/workben/bindings/script.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptBindingLibrary")

# --- Jar_ScriptFramework / Jar_ScriptProviderForBeanShell / Rdb_
# scriptproviderforbeanshell (2026-09-28) --------------------------------
#
# Module_scripting.mk's own Java-gated target list, re-checked directly:
# `$(if $(ENABLE_JAVA), Jar_HelloWorld Jar_Highlight Jar_MemoryUsage
# Jar_ScriptFramework $(if $(ENABLE_SCRIPTING_BEANSHELL), Jar_
# ScriptProviderForBeanShell Rdb_scriptproviderforbeanshell) Jar_
# ScriptProviderForJava ...)`. Only ScriptFramework and the BeanShell pair
# are ported here (what CppunitTest_scripting_beanshell.mk itself needs);
# Jar_HelloWorld/Jar_Highlight/Jar_MemoryUsage/Jar_ScriptProviderForJava are
# a separate, not-yet-attempted to-do, left alone. ENABLE_SCRIPTING_BEANSHELL
# itself is not modelled as a separate CMake option: configure.ac's own
# default (`test "${enable_scripting_beanshell}" != "no" -a "x$with_java" !=
# "xno"`) is unconditionally TRUE whenever Java is enabled at all, unless
# explicitly disabled -- the same "defaults on together with its parent
# flag, this port ties it directly rather than adding a second always-same-
# value toggle" simplification already applied to ENABLE_SCRIPTING/
# ENABLE_DBCONNECTIVITY/ENABLE_XMLHELP elsewhere in this port.
if (LIBRESTUDIO_ENABLE_JAVA)
    # Jar_ScriptFramework.mk, re-checked directly: 28 sourcefiles (browse/
    # container/io/log/provider packages, plus Framework/.../SecurityDialog),
    # use_jars(libreoffice) -> CLASSPATH libreoffice_JAR (ridljar/
    # CMakeLists.txt's own librestudio_add_uno_jar(libreoffice ...), already
    # add_subdirectory()'d before scripting's own position -- no deferral
    # needed), set_manifest(scripting/java/Framework/MANIFEST.MF),
    # set_componentfile(scripting/java/ScriptFramework, OOO, services) --
    # this one DOES register into the ordinary "services" Rdb (unlike the
    # BeanShell provider below), set_packageroot(com).
    librestudio_add_uno_jar(ScriptFramework
        PACKAGEROOT com
        CLASSPATH "${libreoffice_JAR}"
        DEPENDS libreoffice_jar
        MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/java/Framework/MANIFEST.MF"
        SOURCES
            scripting/java/com/sun/star/script/framework/browse/DialogFactory
            scripting/java/com/sun/star/script/framework/browse/ParcelBrowseNode
            scripting/java/com/sun/star/script/framework/browse/PkgProviderBrowseNode
            scripting/java/com/sun/star/script/framework/browse/ProviderBrowseNode
            scripting/java/com/sun/star/script/framework/browse/ScriptBrowseNode
            scripting/java/com/sun/star/script/framework/container/DeployedUnoPackagesDB
            scripting/java/com/sun/star/script/framework/container/ParcelContainer
            scripting/java/com/sun/star/script/framework/container/ParcelDescriptor
            scripting/java/com/sun/star/script/framework/container/Parcel
            scripting/java/com/sun/star/script/framework/container/ParsedScriptUri
            scripting/java/com/sun/star/script/framework/container/ScriptEntry
            scripting/java/com/sun/star/script/framework/container/ScriptMetaData
            scripting/java/com/sun/star/script/framework/container/UnoPkgContainer
            scripting/java/com/sun/star/script/framework/container/XMLParserFactory
            scripting/java/com/sun/star/script/framework/container/XMLParser
            scripting/java/com/sun/star/script/framework/io/UCBStreamHandler
            scripting/java/com/sun/star/script/framework/io/XInputStreamImpl
            scripting/java/com/sun/star/script/framework/io/XInputStreamWrapper
            scripting/java/com/sun/star/script/framework/io/XOutputStreamWrapper
            scripting/java/com/sun/star/script/framework/io/XStorageHelper
            scripting/java/com/sun/star/script/framework/log/LogUtils
            scripting/java/com/sun/star/script/framework/provider/ClassLoaderFactory
            scripting/java/com/sun/star/script/framework/provider/EditorScriptContext
            scripting/java/com/sun/star/script/framework/provider/PathUtils
            scripting/java/com/sun/star/script/framework/provider/ScriptContext
            scripting/java/com/sun/star/script/framework/provider/ScriptEditor
            scripting/java/com/sun/star/script/framework/provider/ScriptEditorBase
            scripting/java/com/sun/star/script/framework/provider/ScriptProvider
            scripting/java/com/sun/star/script/framework/provider/SwingInvocation
            scripting/java/Framework/com/sun/star/script/framework/security/SecurityDialog)

    librestudio_add_java_component(ScriptFramework
        COMPONENT "${CMAKE_CURRENT_SOURCE_DIR}/java/ScriptFramework.component"
        JARFILE   "${ScriptFramework_JAR}"
        LAYER     OOO)

    # Jar_ScriptProviderForBeanShell.mk, re-checked directly: 6 sourcefiles,
    # use_jars(libreoffice, ScriptFramework) -> CLASSPATH both, use_externals
    # (bsh) -> CLASSPATH the vendored/system bsh.jar (BEANSHELL_JAR, root
    # CMakeLists.txt's own "--- beanshell ---" section, only defined under
    # this same LIBRESTUDIO_ENABLE_JAVA gate), set_manifest(scripting/java/
    # com/.../beanshell/MANIFEST.MF), set_componentfile(scripting/java/
    # ScriptProviderForBeanShell, OOO, scriptproviderforbeanshell) -- unlike
    # ScriptFramework above, this one names a SEPARATE, dedicated Rdb
    # ("scriptproviderforbeanshell", not "services") via librestudio_add_
    # component()'s own RDB parameter (cmake/LibreStudioComponent.cmake) --
    # set_packageroot(com), add_packagefile(template.bsh) -> PACKAGEDIRS
    # equivalent: template.bsh is a plain resource file, not compiled, so it
    # is copied into the jar's own classdir before the jar step via a tiny
    # dedicated copy target rather than PACKAGEDIRS (which expects a whole
    # pre-built directory, not one loose file).
    set(_scriptproviderforbeanshell_template_dest
        "${CMAKE_CURRENT_BINARY_DIR}/ScriptProviderForBeanShell-classes/com/sun/star/script/framework/provider/beanshell/template.bsh")
    add_custom_command(
        OUTPUT "${_scriptproviderforbeanshell_template_dest}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${CMAKE_CURRENT_BINARY_DIR}/ScriptProviderForBeanShell-classes/com/sun/star/script/framework/provider/beanshell"
        COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_CURRENT_SOURCE_DIR}/java/com/sun/star/script/framework/provider/beanshell/template.bsh"
                "${_scriptproviderforbeanshell_template_dest}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/java/com/sun/star/script/framework/provider/beanshell/template.bsh"
        COMMENT "Copying template.bsh into ScriptProviderForBeanShell's own classdir")

    librestudio_add_uno_jar(ScriptProviderForBeanShell
        PACKAGEROOT com
        CLASSPATH "${libreoffice_JAR}" "${ScriptFramework_JAR}" "${BEANSHELL_JAR}"
        MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/java/com/sun/star/script/framework/provider/beanshell/MANIFEST.MF"
        DEPENDS libreoffice_jar ScriptFramework_jar "${BEANSHELL_JAR}"
                "${_scriptproviderforbeanshell_template_dest}"
        SOURCES
            scripting/java/com/sun/star/script/framework/provider/beanshell/PlainSourceView
            scripting/java/com/sun/star/script/framework/provider/beanshell/ScriptEditorForBeanShell
            scripting/java/com/sun/star/script/framework/provider/beanshell/ScriptProviderForBeanShell
            scripting/java/com/sun/star/script/framework/provider/beanshell/ScriptSourceModel
            scripting/java/com/sun/star/script/framework/provider/beanshell/ScriptSourceView
            scripting/java/com/sun/star/script/framework/provider/beanshell/UnsavedChangesListener)

    # librestudio_add_uno_jar() only compiles+jars SOURCES; it has no
    # PACKAGEDIRS-equivalent of its own PACKAGEDIRS parameter actually
    # copying template.bsh's own directory into the jar after the fact
    # (unlike javamaker output, template.bsh is a single loose file, not a
    # whole pre-built tree) -- re-open the already-built jar and add it
    # directly, mirroring gb_Jar_add_packagefile's own effect (the file ends
    # up inside the jar at the given in-jar path) without needing a new
    # parameter on the shared helper for a single one-off call site.
    add_custom_command(
        OUTPUT "${ScriptProviderForBeanShell_JAR}.stamp"
        COMMAND "${Java_JAR_EXECUTABLE}" -uf "${ScriptProviderForBeanShell_JAR}"
                -C "${CMAKE_CURRENT_BINARY_DIR}/ScriptProviderForBeanShell-classes"
                com/sun/star/script/framework/provider/beanshell/template.bsh
        COMMAND ${CMAKE_COMMAND} -E touch "${ScriptProviderForBeanShell_JAR}.stamp"
        DEPENDS ScriptProviderForBeanShell_jar "${_scriptproviderforbeanshell_template_dest}"
        COMMENT "Adding template.bsh to ScriptProviderForBeanShell.jar")
    add_custom_target(ScriptProviderForBeanShell_packagefiles ALL
        DEPENDS "${ScriptProviderForBeanShell_JAR}.stamp")

    librestudio_add_java_component(ScriptProviderForBeanShell
        COMPONENT "${CMAKE_CURRENT_SOURCE_DIR}/java/ScriptProviderForBeanShell.component"
        JARFILE   "${ScriptProviderForBeanShell_JAR}"
        LAYER     OOO
        RDB       scriptproviderforbeanshell)
    add_dependencies(ScriptProviderForBeanShell_component ScriptProviderForBeanShell_packagefiles)

    # gb_Helper_register_jars_for_install(OOO,ooo,[ScriptFramework ...]) and
    # (OOO,extensions_bsh,[ScriptProviderForBeanShell]) in Repository.mk --
    # both real jars, both resolve to program/classes on this single-layer
    # Linux port. Found missing (a real, pre-existing gap, not introduced
    # today) while wiring scripting_beanshell's own CppunitTest, which needs
    # ScriptProviderForBeanShell.jar to actually be on disk at run time.
    install(FILES "${ScriptFramework_JAR}" "${ScriptProviderForBeanShell_JAR}"
        DESTINATION program/classes)

    # Rdb_scriptproviderforbeanshell.mk, re-checked directly: a bare
    # gb_Rdb_Rdb_install call, no gb_Rdb_add_component of its own -- the
    # component routing happens entirely through the Jar's own set_
    # componentfile 4th argument above (upstream's gb_ComponentTarget_
    # ComponentTarget resolves the Rdb name from there and calls gb_Rdb_
    # add_component itself, the same relationship librestudio_add_component(
    # ... RDB ...) reproduces directly). This is the ONLY component this
    # Rdb ever gets (re-confirmed: grepping the whole tree for
    # "scriptproviderforbeanshell" finds no other producer), so it is
    # finalized right here, in the one module that owns it -- cmake/
    # LibreStudioComponent.cmake's own librestudio_finalize_components()
    # header comment explicitly names this as the right place for a
    # private, single-owner registry (as opposed to "services", finalized
    # once at the very end of the root CMakeLists.txt after every module
    # has had its say).
    librestudio_finalize_components(scriptproviderforbeanshell)
endif ()

# --- CppunitTests --------------------------------------------------------------
#
# scripting has 4 CppunitTest_*.mk files, each read in full and checked
# directly against the live file, all 4 now wired in scripting/CppunitTest_
# deferred.cmake -- deferred, not wired here directly, because all four call
# gb_CppunitTest_use_configuration (and either gb_CppunitTest_use_rdb or
# gb_CppunitTest_use_rdbs), needing program_services_rdb/services_rdb and
# test/CMakeLists.txt's own LIBRESTUDIO_TEST_UNITTEST_DIR, both only
# available much later in the root CMakeLists.txt than scripting's own
# add_subdirectory() position -- the same ordering constraint already
# documented for scaddins_analysis/xmlscript_cppunit/svx's three tests/
# sfx2's four tests before this one. See that file for the full per-test
# detail.
#
# scripting_beanshell (CppunitTest_scripting_beanshell.mk) -- 2026-09-28:
# wired for real, the last of the 4. Every dependency an earlier pass of
# this comment flagged as missing is now real: BeanShell is vendored and
# builds (root CMakeLists.txt's own "--- beanshell ---" section), Jar_
# ScriptProviderForBeanShell/Jar_ScriptFramework compile+link+jar (this
# file's own "--- Jar_ScriptFramework / Jar_ScriptProviderForBeanShell /
# Rdb_scriptproviderforbeanshell ---" section above), librestudio_add_
# cppunit_test() has a real RDBS parameter (cmake/LibreStudioTest.cmake) for
# gb_CppunitTest_use_rdbs's own two-registry ("services"
# "scriptproviderforbeanshell") need, and -- the last, deepest piece --
# Jar_java_uno/Library_java_uno are now ported too (bridges/CMakeLists.txt's
# own "--- java_uno ---" section): beanshelltest.cxx (read in full) calls
# createInstanceWithArgumentsAndContext("...ScriptProviderForBeanShell...")
# and then real UNO methods on the result (createNode, renameNode,
# getChildNodes, getName, getPropertyValue) across 2 test cases, each one
# crossing the JNI/java_uno bridge for real -- the first actual exerciser of
# that bridge anywhere in this tree. See scripting/CppunitTest_deferred.
# cmake's own scripting_beanshell section for the exact wiring, and this
# module's CMakeLists.txt's own end-to-end verification note (ctest result)
# once run.
#
# Real, incidental bugs found and fixed while verifying the above end-to-end
# (none of them in code touched by this pass before today, all pre-existing
# and never previously build-tested since LIBRESTUDIO_ENABLE_JAVA defaults
# OFF): ridljar/CMakeLists.txt's own librestudio_add_uno_jar(libreoffice...)
# call named a DEPENDS target "java_websocket_ext" that has never existed
# (librestudio_add_uno_jar(java_websocket...) only ever creates "java_
# websocket_jar") and a CLASSPATH variable "JAVA_WEBSOCKET_JAR" that was
# never set anywhere (the real CACHE variable is "java_websocket_JAR",
# lowercase-with-underscores per librestudio_add_uno_jar()'s own "${name}_
# JAR" convention) -- both silently meant libreoffice.jar had never
# successfully linked WebsocketConnection.java against java_websocket's own
# WebSocketClient class before now. Both fixed directly in ridljar/
# CMakeLists.txt.
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/scripting/CppunitTest_deferred.cmake")

endif ()
