# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# CppunitTest: gbuild's per-test "shared library run by a generic runner"
# model, transcribed generically enough for every future module's
# CppunitTest_*.mk to reuse. sal is the first consumer (see sal/CMakeLists.txt
# -- it is the bottom of the whole dependency graph, so its CppunitTests are
# the first this port ever ports), but nothing below is sal-specific.
#
# Upstream shape (solenv/gbuild/CppunitTest.mk):
#   - Each CppunitTest_<name> is built as a *shared library*
#     (gb_LinkTarget_set_targettype(...,CppunitTest)), never an executable --
#     CppUnit::PlugInManager::load() dlopen()s it at test run time
#     (CppunitTest.mk's gb_CppunitTest__CppunitTest_impl, line ~208).
#   - Every CppunitTest links $(gb_STDLIBS) unconditionally and the cppunit
#     external, and gets
#     -DCPPUNIT_PLUGIN_EXPORT='extern "C" SAL_DLLPUBLIC_EXPORT' so
#     CPPUNIT_PLUGIN_IMPLEMENT's exported entry point has C linkage for
#     dlopen (CppunitTest.mk lines 210-222). gb_STDLIBS has no definition for
#     the unxgcc.mk platform this port targets (only OpenBSD/Solaris/MSVC/
#     Android define it), so nothing else needs to be added here for it.
#   - All CppunitTest targets are dlopened by one shared runner executable,
#     cppunittester (sal/Executable_cppunittester.mk,
#     sal/cppunittester/cppunittester.cxx -- ported in sal/CMakeLists.txt,
#     since Executable_cppunittester.mk belongs to sal), which:
#       * takes zero or more "--protector <shared-library-path>
#         <function-symbol>" triples (cppunit "Protector" plug-ins that know
#         how to catch and report UNO/VCL-specific exceptions before the
#         process dies) followed by the test shared library's own path;
#       * dlopens the protector libraries and the test library via
#         osl::Module::getFunctionSymbol / CppUnit::PlugInManager;
#       * runs the suite through CppUnit::TestRunner, honouring the
#         CPPUNIT_TEST_NAME env var to filter to a subset.
#
# Protectors are opt-in per test (gb_CppunitTest_use_ure / gb_CppunitTest_
# use_vcl*, CppunitTest.mk lines 250-302) -- NOT unconditional. sal's own 8
# CppunitTest_*.mk files were each checked individually (grep for use_ure /
# use_vcl / use_vcl_non_headless* across every sal/CppunitTest_*.mk; see
# sal/CMakeLists.txt for the per-target notes) and none of them call any of
# those, so this port does not implement unobootstrapprotector /
# unoexceptionprotector / vclbootstrapprotector here -- that matches unotest/
# CMakeLists.txt and test/CMakeLists.txt, which both already note their
# Library_uno*protector.mk / Library_vclbootstrapprotector.mk are not yet
# ported. librestudio_add_cppunit_test() still accepts a PROTECTORS argument
# (functional, just unused by every call site today) so a later wave that
# *does* need protectors -- once those Library_*protector.mk targets exist --
# can wire them in without reshaping this function again.
#
# gb_CppunitTest_use_components (CppunitTest.mk, wired through to
# gb_CppunitTest__make_args's UNO_SERVICES handling, lines ~119-120) registers
# named .component fragments -- built by other modules' own gb_Library_
# set_componentfile / librestudio_add_component() calls -- into the test's
# UNO_SERVICES list, so the ComponentContext the test bootstraps can
# instantiate those services. Upstream joins every entry with a single space
# into ONE "-env:UNO_SERVICES=<url> <url> ..." argument (not one "-env:" flag
# per entry) and gates the whole thing behind $(if $(URE),...) -- this port
# does not model the URE gate (no use_ure-equivalent parameter exists here
# either, see below), so librestudio_add_cppunit_test() emits the argument
# unconditionally whenever COMPONENTS is non-empty.
#
# Each name passed to COMPONENTS is looked up as the GLOBAL property
# "LIBRESTUDIO_COMPONENT_FILE_<name>" that librestudio_add_component()
# records (cmake/LibreStudioComponent.cmake) -- the same <name> given as that
# function's own first argument, which is this port's CMake-side identifier
# for a component and is NOT always the same string as gbuild's own
# use_components spelling (a path like "i18npool/util/i18npool"): e.g.
# i18npool/CMakeLists.txt registers its component under the plain name
# "i18npool", not the gbuild path, so COMPONENTS must be given "i18npool"
# there, verified per call site against the module that owns each name.
#
# Because get_property(GLOBAL ...) reads whatever has been set by the exact
# point it runs, this is a real ordering constraint, not just a lookup
# detail: the owning module's add_subdirectory() -- and with it, the
# librestudio_add_component() call inside it -- must have already run by the
# time a librestudio_add_cppunit_test() call names it in COMPONENTS.
# comphelper_test's dependency on i18npool, and sax_parser/sax_xmlimport's on
# configmgr/framework/i18npool/sfx2/ucb/uui, all hit this: those owning
# modules' add_subdirectory() calls sit textually AFTER comphelper's and
# sax's own in the root CMakeLists.txt (comphelper and sax are themselves
# depended on by many earlier-consumed modules, so reordering them instead
# would be the much larger, riskier change). The fix used here is to defer
# just these specific librestudio_add_cppunit_test() calls -- moved into
# their own comphelper/CppunitTest_deferred.cmake and sax/CppunitTest_
# deferred.cmake files -- to an include() from the root CMakeLists.txt
# positioned after every module they reference has already been
# add_subdirectory()'d; see those files and their include() call sites for
# the specifics, including the further directory-scope wrinkle (a directory-
# scoped IMPORTED_TARGET like PkgConfig::NSS is not visible from the root
# scope a deferred file executes in) that comphelper's deferred file also has
# to work around.
#
# gb_CppunitTest__make_args (CppunitTest.mk lines 108-131) unconditionally
# passes "-env:UserInstallation=<file-url>" to every test, regardless of
# use_ure/use_vcl*. This turns out to be genuinely load-bearing, not inert
# boilerplate: sal/qa/rtl/bootstrap/expand.cxx's testThreeSegments calls
# rtl::Bootstrap::get(u"UserInstallation", url) and CPPUNIT_ASSERTs that it
# succeeds, and rtl::Bootstrap resolves that key straight from the process's
# own "-env:" command-line arguments (sal/rtl/bootstrap.cxx,
# getFromCommandLineArgs(), consulted by getValue()/getAmbienceValue()) --
# there is no rc/ini file fallback for a key nothing ever wrote to one. So
# every test registered here gets a fresh "<name>.user" directory (created by
# a CTest fixture, so it exists before the test runs) and that same
# "-env:UserInstallation=file://..." argument. This is a *minimal* version of
# gb_CppunitTest__make_args: gbuild also does `rm -fr $@.user && cp -r
# $(WORKDIR)/unittest $@.user` (recreate from a template package) and adds
# BRAND_BASE_DIR / UNO_SERVICES / UNO_TYPES / CONFIGURATION_LAYERS / real
# --protector args -- all of that is only meaningful once $(URE) is true
# (i.e. once a test calls use_ure), which is not the case for any of sal's 8
# tests, and there is no $(WORKDIR)/unittest template package ported yet
# either. A later wave whose tests do call use_ure has to extend this
# function for that, the same way PROTECTORS was left in ahead of need.
#
# gb_CppunitTest_use_configuration (CppunitTest.mk lines ~376-408) is that
# later wave, for the CONFIGURATION_LAYERS piece specifically. Upstream
# expands it to three sub-calls, each appending one "xcsxcu:<url>" entry to a
# per-test CONFIGURATION_LAYERS make variable, in this fixed order (later
# layers override earlier ones for the same setting at run time):
#   gb_CppunitTest_use_instdir_configuration:
#     xcsxcu:$(INSTROOT)/$(LIBO_SHARE_FOLDER)/registry              (+ order-
#     dep on Package_registry, postprocess's packed instdir registry)
#   gb_CppunitTest_use_common_configuration:
#     xcsxcu:$(WORKDIR)/unittest/registry-common                    (+ order-
#     dep on Package_unittest, test/user-template's staged copy)
#   gb_CppunitTest_use_user_ui_configuration:
#     xcsxcu:$(WORKDIR)/unittest/registry-user-ui                   (+ same
#     Package_unittest order-dep)
# gb_CppunitTest__make_args then joins the whole CONFIGURATION_LAYERS list
# with a single space into ONE "-env:CONFIGURATION_LAYERS=..." argument --
# same one-argument-for-the-whole-list shape as UNO_SERVICES above, not one
# "-env:" flag per layer.
#
# This port's equivalents for the two order-deps: the instdir layer is
# exactly officecfg/CMakeLists.txt's already-built "the registry" section --
# officecfg_registry (a real ALL target, depended on directly below) produces
# ${LIBRESTUDIO_REGISTRY_DIR}/main.xcd, the identical registry the production
# unorc points CONFIGURATION_LAYERS at (cmake/LibreStudioUnorc.cmake), so it
# is reused as-is, no copy needed (it is read-only static build output, safe
# to share across every test that asks for it). The common/user-ui layers
# have no equivalent yet: test/Package_unittest.mk packages the whole of
# test/user-template/ (registry-common/modifications.xcd, registry-user-ui/
# user-ui.xcd, plus the full synthetic user profile -- user/database,
# user/wordbook, user/autotext, user/autocorr, user/config/soffice.cfg/... --
# that gb_CppunitTest__make_args's own generic "cp -r $(WORKDIR)/unittest
# $@.user" step also copies into *every* URE test's .user dir, not just
# CONFIGURATION ones) into $(WORKDIR)/unittest. test/CMakeLists.txt now
# stages that same source directory into LIBRESTUDIO_TEST_UNITTEST_DIR
# (${CMAKE_BINARY_DIR}/test/unittest, mirroring $(WORKDIR)/unittest) with a
# single file(COPY ...) at configure time -- test/user-template is static
# source content, not generated, so there is nothing a build-time custom
# command would add; see test/CMakeLists.txt's own comment on this staging
# step, including the "reconfigure to see an edit" caveat that follows from
# doing it at configure time.
#
# CONFIGURATION below (a cmake_parse_arguments option, not a multi-value
# argument like COMPONENTS -- gbuild's use_configuration takes no parameters
# either) does three things when given:
#   1. add_dependencies(${name} officecfg_registry) -- the instdir layer's
#      order-dep, same role DEPENDS already plays for other targets.
#   2. Joins all three xcsxcu:<url> entries into one -env:CONFIGURATION_LAYERS
#      argument, in the fixed order above, appended to the cppunittester
#      command line below (right after UNO_SERVICES, right before
#      UserInstallation -- matching gb_CppunitTest__make_args's own argument
#      order).
#   3. Upgrades the "<name>.user" FIXTURES_SETUP step from the plain
#      make_directory every other test still gets to gbuild's real
#      "rm -fr $@.user && cp -r $(WORKDIR)/unittest $@.user" -- i.e. every
#      CONFIGURATION test's .user directory is now a fresh copy of the
#      staged test/user-template tree (registry-common/registry-user-ui
#      included, even though those two are *also* read directly via
#      CONFIGURATION_LAYERS above -- that duplication is upstream's own
#      shape, not introduced here: gb_CppunitTest__make_args's .user copy is
#      unconditional for every use_ure test regardless of use_configuration,
#      while CONFIGURATION_LAYERS is the separate, opt-in mechanism that
#      points configmgr at the same files directly by URL). Tests that do
#      NOT pass CONFIGURATION are entirely unaffected -- this is purely
#      additive, the same posture PROTECTORS/COMPONENTS already established.
#
# What is still NOT modelled, same honesty this file already applies to
# use_ure's other pieces: gb_CppunitTest_use_rdb's separate BRAND_BASE_DIR /
# real instdir Package_registry order-dep (officecfg_registry is this port's
# own from-source equivalent, not upstream's packaged instdir copy, but
# produces the identical main.xcd) / the extension and user-installation
# registry layers upstream's fundamentalrc carries beyond what unorc's own
# CONFIGURATION_LAYERS default already gives every test for free (see
# LibreStudioUnorc.cmake) -- none of those are reproduced, only the three
# xcsxcu layers gb_CppunitTest_use_configuration itself adds.
#
# NONHEADLESS and UICONFIG (added together, for the same three "*_dialogs_
# test" gb_CppunitTest_CppunitScreenShot targets -- vcl_dialogs_test,
# svtools_dialogs_test, svx_dialogs_test -- that motivated both): gbuild's
# gb_CppunitTest_use_vcl_non_headless_with_windows and gb_CppunitTest_
# use_uiconfigs (solenv/gbuild/CppunitTest.mk lines 297-302 and 472-480).
#
# gb_CppunitTest_use_vcl_non_headless_with_windows(name) does three things
# upstream: clears the per-test HEADLESS make variable (CppunitTest.mk:233
# defaults it to the bare cppunittester CLI flag "--headless"; line 298 here
# sets it back to empty), sets gb_CppunitTest__vcl_no_svp := true (line 299;
# the target rule at CppunitTest.mk:158-159 uses this to filter
# "SAL_USE_VCLPLUGIN=svp" out of gb_TEST_ENV_VARS for this one test only),
# and calls gb_CppunitTest__use_vcl(name,$(false)) (line 300), which -- unlike
# gb_CppunitTest_use_vcl's own $(true) headless argument -- makes the real
# platform VCL plugin libraries (vclplug_gen and friends under USING_X11,
# CppunitTest.mk:264-271) build-order dependencies of the test, not just
# vclbootstrapprotector.
#
# Re-verified directly against this file before adding NONHEADLESS: neither
# "--headless" nor "SAL_USE_VCLPLUGIN=svp" (nor any other spelling of
# "headless"/"HEADLESS"/"SAL_USE_VCLPLUGIN") appears anywhere in this file,
# in any add_test() this function has ever emitted, or anywhere else in this
# port's CMake side. That means every CppunitTest registered through this
# function today is "headless" only by omission -- nothing here ever forces
# a real backend NOR explicitly requests the SVP one -- unlike upstream,
# where gb_TEST_ENV_VARS actively sets SAL_USE_VCLPLUGIN=svp for every test
# and gb_CppunitTest_use_vcl_non_headless_with_windows is what has to
# actively undo that. NONHEADLESS therefore does not need an "undo" step
# (there is nothing forced to undo); it only needs to (a) make the real
# backend an actual build dependency, so it exists to dlopen, and (b) pin
# SAL_USE_VCLPLUGIN=gen explicitly via the same "-env:"-argument mechanism
# COMPONENTS/CONFIGURATION already use, so the real backend is guaranteed
# over autodetection rather than left to it. vclplug_gen specifically (not
# the GTK/Qt backends CppunitTest.mk also lists under USING_X11) because it
# is the one this port actually builds unconditionally by default (vcl/
# CMakeLists.txt: "if (ENABLE_GEN)", ENABLE_GEN defaulting ON) -- see the
# NONHEADLESS option's own comment at its call site below for the FATAL_ERROR
# this triggers when that assumption does not hold (e.g. ENABLE_GEN=OFF).
#
# gb_CppunitTest_use_uiconfigs(name, module...) (CppunitTest.mk:472-480) adds
# one order-only dependency per module on gb_UIConfig_get_target(module) --
# upstream's own UIConfig_<module> packaging target. This port's UIConfig_vcl/
# UIConfig_svt/UIConfig_svx equivalents (vcl/svtools/svx CMakeLists.txt) were,
# until this change, install(FILES ...) rules only -- real for `cmake
# --install`, but producing nothing a `cmake --build` ever touches, so there
# was nothing to depend on here. Each of those three files now also defines a
# real build-time mirror -- an ALL target named "<module>_uiconfig" (vcl_
# uiconfig, svt_uiconfig, svx_uiconfig) that add_custom_command()-copies the
# same .ui files into the build tree directly (see e.g. vcl/CMakeLists.txt's
# "--- UIConfig_vcl ---" block for the exact mechanism) -- so UICONFIG below
# has a real target to depend on. UICONFIG takes a single module name (not a
# list like upstream's use_uiconfigs) because no call site in this port needs
# more than one module's .ui files.

# librestudio_add_cppunit_test(<name>
#     SOURCES <files...>
#     [LIBRARIES <cmake-target>...]
#     [INCLUDE <dir>...]
#     [DEFINITIONS <def>...]
#     [DEPENDS <target>...]
#     [PROTECTORS <cmake-target> <symbol> ...]
#     [COMPONENTS <name>...]
#     [CONFIGURATION]
#     [NONHEADLESS]
#     [UICONFIG <module-name>]
#     [EXTRA_ARGS <arg>...])
#
# SOURCES: gb_CppunitTest_add_exception_objects's own spelling -- a
#   module-relative path without the .cxx extension, expanded the same way
#   as every other source list in this port (librestudio_expand_cxx, see
#   cmake/LibreStudioPlatform.cmake) -- or an already-absolute path, which
#   passes through unchanged (used by sal_osl_file_details to recompile some
#   of sal's own sources directly into the test; see the comment at its call
#   site in sal/CMakeLists.txt for why).
# LIBRARIES: forwarded to target_link_libraries(<name> PRIVATE ...), same as
#   gb_CppunitTest_use_libraries.
# INCLUDE / DEFINITIONS: forwarded to target_include_directories /
#   target_compile_definitions, same as gb_CppunitTest_set_include /
#   gb_CppunitTest_add_defs.
# DEPENDS: build-order-only dependencies (add_dependencies), for gbuild's
#   gb_CppunitTest_use_executable and the like -- something the test needs to
#   exist as a *file* next to itself at run time (dlopened or spawned by the
#   test's own code, e.g. osl_process_child, or another CppunitTest's shared
#   library, e.g. Module_DLL) but never actually links against.
# PROTECTORS: pairs of <cmake-target> <symbol>, one pair per protector
#   plug-in; turned into "--protector $<TARGET_FILE:target> symbol" arguments
#   ahead of the CTest command. Not used by any call site yet -- see the file
#   header comment for why -- but implemented for real (not a stub) since a
#   later wave is expected to need it.
# COMPONENTS: component names (librestudio_add_component()'s own <name>
#   argument, not necessarily gbuild's use_components spelling -- see the
#   file header comment), gb_CppunitTest_use_components's equivalent. Each
#   is resolved via the GLOBAL property librestudio_add_component() records,
#   added as a build-order dependency (on that call's own "<name>_component"
#   custom target), and its built .component file joined into a single
#   "-env:UNO_SERVICES=file://... file://..." argument. FATAL_ERRORs if a
#   name was never registered -- see the file header comment for the real
#   ordering constraint this implies.
# CONFIGURATION: a bare option (present or absent, no value -- gb_CppunitTest_
#   use_configuration's equivalent, takes no parameters either). See the file
#   header comment for the full mechanism; requires add_subdirectory(officecfg)
#   and add_subdirectory(test) to have already run (FATAL_ERRORs, naming which
#   is missing, if not -- the same style as the cppunittester check below).
# NONHEADLESS: a bare option (present or absent, no value -- gb_CppunitTest_
#   use_vcl_non_headless_with_windows's equivalent, takes no parameters
#   either). See the file header comment for the full mechanism and why it
#   does not need to "undo" a forced headless/SVP setting in this port (there
#   is none to undo). Adds a build-order dependency on the vclplug_gen target
#   (vcl/CMakeLists.txt's ENABLE_GEN block) and appends
#   "-env:SAL_USE_VCLPLUGIN=gen" to the cppunittester command line, ahead of
#   UNO_SERVICES/CONFIGURATION_LAYERS/UserInstallation. FATAL_ERRORs, naming
#   the missing target, if vclplug_gen does not exist (e.g. built with
#   ENABLE_GEN=OFF) -- the same style COMPONENTS/CONFIGURATION already use.
# UICONFIG: a single module name (gb_CppunitTest_use_uiconfigs's equivalent,
#   restricted to one module -- see the file header comment for why a list
#   was not needed). Adds add_dependencies(<name> <module-name>_uiconfig),
#   the real build-time ALL target vcl/svtools/svx's own UIConfig_* blocks
#   now define (see the file header comment). FATAL_ERRORs, naming the
#   module, if "<module-name>_uiconfig" does not exist yet -- same style as
#   COMPONENTS/CONFIGURATION/NONHEADLESS.
# EXTRA_ARGS: gb_CppunitTest_add_arguments's equivalent -- raw extra
#   arguments appended, verbatim and last, to the cppunittester COMMAND
#   (gb_CppunitTest__make_args appends its own $(ARGS) last too). Purely
#   additive: defaults to empty, so every call site that does not pass it
#   (every one as of this writing) is unaffected. Added for
#   CppunitTest_smoketest.mk's bespoke "-env:arg-soffice=" / "-env:arg-user="
#   / "-env:arg-env=" / "-env:arg-testarg.smoketest.doc=" arguments (see
#   smoketest/CMakeLists.txt) rather than hand-rolling that test's add_test()
#   call outright: those four are plain string-valued "-env:" flags with no
#   interaction with anything else this function already builds (protectors,
#   UNO_SERVICES, the UserInstallation fixture), so passing them through
#   unchanged is low-risk and keeps this one bespoke test going through the
#   same generic machinery -- and the same FIXTURES_REQUIRED/CTest plumbing
#   -- as every other CppunitTest in this port.
function(librestudio_add_cppunit_test name)
    cmake_parse_arguments(_cut "CONFIGURATION;NONHEADLESS" "UICONFIG" "SOURCES;LIBRARIES;INCLUDE;DEFINITIONS;DEPENDS;PROTECTORS;COMPONENTS;EXTRA_ARGS" ${ARGN})

    if (NOT _cut_SOURCES)
        message(FATAL_ERROR "librestudio_add_cppunit_test(${name}): SOURCES is required")
    endif ()
    if (_cut_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "librestudio_add_cppunit_test(${name}): unknown arguments: ${_cut_UNPARSED_ARGUMENTS}")
    endif ()
    list(LENGTH _cut_PROTECTORS _cut_protector_count)
    math(EXPR _cut_protector_remainder "${_cut_protector_count} % 2")
    if (NOT _cut_protector_remainder EQUAL 0)
        message(FATAL_ERROR
            "librestudio_add_cppunit_test(${name}): PROTECTORS must be pairs of <cmake-target> <symbol>")
    endif ()

    librestudio_expand_cxx(_cut_SOURCES)

    # gb_LinkTarget_set_targettype(...,CppunitTest): the test is a shared
    # object dlopened by cppunittester, never linked against by anything else
    # -- CMake's MODULE type says exactly that (as opposed to SHARED, which
    # advertises "other targets may link against this").
    add_library(${name} MODULE ${_cut_SOURCES})

    # gb_CppunitTest_get_filename / get_ilibfilename (solenv/gbuild/platform/
    # unxgcc.mk): every CppunitTest's file is named "libtest_<name>.so" on
    # Linux. Not just cosmetic: sal_osl's own osl_Module test (the Module_DLL
    # CppunitTest) hardcodes the filename "libtest_Module_DLL.so" and expects
    # to find it next to itself (sal/qa/osl/module/osl_Module.cxx,
    # getDllURL()), so every test target here needs this rename, not only
    # Module_DLL.
    set_target_properties(${name} PROPERTIES OUTPUT_NAME "test_${name}")

    # cppunit: the same external unotest/CMakeLists.txt and test/CMakeLists.txt
    # use (pkg_check_modules(CPPUNIT REQUIRED IMPORTED_TARGET cppunit)),
    # reused rather than re-derived. An IMPORTED_TARGET from pkg_check_modules
    # is only visible in the directory it was created in and below (see test/
    # CMakeLists.txt's own comment on this very point), so this is guarded
    # and re-run here rather than assumed to already exist -- it makes the
    # function usable from any calling directory, current or future.
    if (NOT TARGET PkgConfig::CPPUNIT)
        pkg_check_modules(CPPUNIT REQUIRED IMPORTED_TARGET cppunit)
    endif ()

    target_link_libraries(${name} PRIVATE PkgConfig::CPPUNIT ${_cut_LIBRARIES})

    # -DCPPUNIT_PLUGIN_EXPORT='extern "C" SAL_DLLPUBLIC_EXPORT'
    # (CppunitTest.mk line 221, quoted in full in the file header comment
    # above).
    target_compile_definitions(${name} PRIVATE
        "CPPUNIT_PLUGIN_EXPORT=extern \"C\" SAL_DLLPUBLIC_EXPORT"
        ${_cut_DEFINITIONS})

    if (_cut_INCLUDE)
        target_include_directories(${name} PRIVATE ${_cut_INCLUDE})
    endif ()

    if (_cut_DEPENDS)
        add_dependencies(${name} ${_cut_DEPENDS})
    endif ()

    # UICONFIG -- gb_CppunitTest_use_uiconfigs's equivalent, restricted to a
    # single module name (see the file header comment for why). Resolved as
    # a build-order dependency on "<module-name>_uiconfig", the real ALL
    # target vcl/svtools/svx's own UIConfig_* blocks now define alongside
    # their install(FILES ...) packaging rule.
    if (_cut_UICONFIG)
        if (NOT TARGET "${_cut_UICONFIG}_uiconfig")
            message(FATAL_ERROR
                "librestudio_add_cppunit_test(${name}): UICONFIG names "
                "'${_cut_UICONFIG}', but no '${_cut_UICONFIG}_uiconfig' target exists -- "
                "that module's own UIConfig_* build-time mirror (see e.g. vcl/CMakeLists.txt's "
                "UIConfig_vcl block) must be defined, and its add_subdirectory() must have "
                "already run, before this librestudio_add_cppunit_test() call is reached")
        endif ()
        add_dependencies(${name} "${_cut_UICONFIG}_uiconfig")
    endif ()

    # COMPONENTS -- gb_CppunitTest_use_components's equivalent. See the file
    # header comment for the lookup key's exact semantics and the ordering
    # constraint this implies.
    set(_cut_component_urls "")
    foreach (_cut_c IN LISTS _cut_COMPONENTS)
        get_property(_cut_component_file GLOBAL PROPERTY "LIBRESTUDIO_COMPONENT_FILE_${_cut_c}")
        if (NOT _cut_component_file)
            message(FATAL_ERROR
                "librestudio_add_cppunit_test(${name}): COMPONENTS names "
                "'${_cut_c}', but no librestudio_add_component(${_cut_c} ...) "
                "has run yet -- that component's owning module's "
                "add_subdirectory() must run, registering it, before this "
                "librestudio_add_cppunit_test() call is reached")
        endif ()
        # "<name>_component": librestudio_add_component()'s own custom
        # target for this component (cmake/LibreStudioComponent.cmake) --
        # depended on the same way DEPENDS above does for plain targets, a
        # build-order dependency only, never a link. Guaranteed to already
        # exist here: it is created in the very same call that set the
        # GLOBAL property just checked above.
        add_dependencies(${name} "${_cut_c}_component")
        list(APPEND _cut_component_urls "file://${_cut_component_file}")
        unset(_cut_component_file)
    endforeach ()

    # gb_CppunitTest__make_args (CppunitTest.mk ~line 119-120): every
    # UNO_SERVICES entry space-joined into ONE "-env:UNO_SERVICES=..."
    # argument, not repeated "-env:" flags -- see the file header comment.
    set(_cut_uno_services_arg "")
    if (_cut_component_urls)
        string(REPLACE ";" " " _cut_uno_services_joined "${_cut_component_urls}")
        set(_cut_uno_services_arg "-env:UNO_SERVICES=${_cut_uno_services_joined}")
    endif ()

    # CONFIGURATION -- gb_CppunitTest_use_configuration's equivalent. See the
    # file header comment for the full mechanism (the three xcsxcu layers,
    # their fixed order, and what each order-dep below stands in for).
    set(_cut_configuration_layers_arg "")
    if (_cut_CONFIGURATION)
        if (NOT TARGET officecfg_registry)
            message(FATAL_ERROR
                "librestudio_add_cppunit_test(${name}): CONFIGURATION requires the "
                "officecfg_registry target (officecfg/CMakeLists.txt's instdir registry) -- "
                "add_subdirectory(officecfg) must run before this call")
        endif ()
        if (NOT LIBRESTUDIO_TEST_UNITTEST_DIR)
            message(FATAL_ERROR
                "librestudio_add_cppunit_test(${name}): CONFIGURATION requires "
                "LIBRESTUDIO_TEST_UNITTEST_DIR (test/CMakeLists.txt's staged copy of "
                "test/user-template) -- add_subdirectory(test) must run before this call")
        endif ()
        add_dependencies(${name} officecfg_registry)
        set(_cut_configuration_layers_arg
            "-env:CONFIGURATION_LAYERS=xcsxcu:file://${LIBRESTUDIO_REGISTRY_DIR} xcsxcu:file://${LIBRESTUDIO_TEST_UNITTEST_DIR}/registry-common xcsxcu:file://${LIBRESTUDIO_TEST_UNITTEST_DIR}/registry-user-ui")
    endif ()

    # NONHEADLESS -- gb_CppunitTest_use_vcl_non_headless_with_windows's
    # equivalent, restricted to what this port actually needs -- see the file
    # header comment for the full "nothing here forces headless/SVP in the
    # first place, so there is nothing to undo" reasoning. Makes the real
    # platform VCL backend (vclplug_gen) a build-order dependency and pins
    # SAL_USE_VCLPLUGIN=gen explicitly, ahead of the other -env: arguments
    # below, rather than leaving backend selection to autodetection.
    set(_cut_nonheadless_arg "")
    if (_cut_NONHEADLESS)
        if (NOT TARGET vclplug_gen)
            message(FATAL_ERROR
                "librestudio_add_cppunit_test(${name}): NONHEADLESS requires the vclplug_gen "
                "target (vcl/CMakeLists.txt's ENABLE_GEN block, default ON) -- "
                "add_subdirectory(vcl) must run, with ENABLE_GEN not turned off, before this call")
        endif ()
        add_dependencies(${name} vclplug_gen)
        set(_cut_nonheadless_arg "-env:SAL_USE_VCLPLUGIN=gen")
    endif ()

    if (NOT TARGET cppunittester)
        message(FATAL_ERROR
            "librestudio_add_cppunit_test(${name}): the cppunittester target does not exist yet "
            "-- add_subdirectory(sal) must run before this call (cppunittester is defined in "
            "sal/CMakeLists.txt, since Executable_cppunittester.mk belongs to sal upstream too)")
    endif ()

    # --protector <path> <symbol> pairs, gb_CppunitTest__make_args's order:
    # they come after the test library's own path on the command line (see
    # the add_test() call below), which is also where cppunittester's own
    # argument parser (sal/cppunittester/cppunittester.cxx, main2()) expects
    # them -- it treats the first bare, non-"-env:", non-"--protector"
    # argument as the test library path, and "--protector" specially
    # wherever it appears afterwards.
    set(_cut_protector_args "")
    if (_cut_protector_count GREATER 0)
        math(EXPR _cut_last_pair "${_cut_protector_count} / 2 - 1")
        foreach (_cut_i RANGE 0 ${_cut_last_pair})
            math(EXPR _cut_lib_idx "${_cut_i} * 2")
            math(EXPR _cut_sym_idx "${_cut_lib_idx} + 1")
            list(GET _cut_PROTECTORS ${_cut_lib_idx} _cut_protector_lib)
            list(GET _cut_PROTECTORS ${_cut_sym_idx} _cut_protector_sym)
            list(APPEND _cut_protector_args
                --protector "$<TARGET_FILE:${_cut_protector_lib}>" "${_cut_protector_sym}")
        endforeach ()
    endif ()

    # Minimal version of gb_CppunitTest__make_args's UserInstallation
    # bootstrap profile -- see the file header comment for the full
    # rationale and what is deliberately not implemented here.
    #
    # This does not clear stale content between reruns the way gbuild's own
    # `rm -fr $@.user && cp -r $(WORKDIR)/unittest $@.user` does; none of
    # sal's own tests need a pristine profile (they only need the directory
    # to exist and be writable), so plain make_directory is an accepted
    # simplification here, not a silent omission. CONFIGURATION tests get the
    # real gbuild behaviour instead (see the file header comment, point 3):
    # a fresh copy of the staged test/user-template tree every run.
    set(_cut_user_dir "${CMAKE_CURRENT_BINARY_DIR}/${name}.user")
    if (_cut_CONFIGURATION)
        add_test(NAME "${name}_user_installation"
            COMMAND sh -c "rm -rf '${_cut_user_dir}' && cp -r '${LIBRESTUDIO_TEST_UNITTEST_DIR}' '${_cut_user_dir}'")
    else ()
        add_test(NAME "${name}_user_installation"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${_cut_user_dir}")
    endif ()
    set_tests_properties("${name}_user_installation" PROPERTIES
        FIXTURES_SETUP "${name}_user_installation")

    add_test(NAME ${name}
        COMMAND cppunittester "$<TARGET_FILE:${name}>" ${_cut_protector_args}
                ${_cut_nonheadless_arg}
                ${_cut_uno_services_arg}
                ${_cut_configuration_layers_arg}
                "-env:UserInstallation=file://${_cut_user_dir}"
                ${_cut_EXTRA_ARGS})
    set_tests_properties(${name} PROPERTIES
        FIXTURES_REQUIRED "${name}_user_installation")
endfunction()
