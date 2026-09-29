# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# setup_native -- native-installer glue: platform-specific packaging scripts
# (RPM/DMG/MSI) and the small number of genuinely compiled Windows DLLs that
# implement MSI custom actions. Most of this module belongs to the *installer
# build* (CustomTarget_instsetoo_native + scp2, neither of which has a
# CMakeLists.txt in this port -- `find scp2 -iname CMakeLists.txt` and the
# same for instsetoo_native both come back empty), not to the *product
# build* this CMake tree otherwise produces. Read in full for this pass:
# Module_setup_native.mk and every Library_*/StaticLibrary_*/CustomTarget_*/
# Package_*.mk it names.
#
# Module_setup_native.mk, read in full:
#
#   $(eval $(call gb_Module_Module,setup_native))
#   $(eval $(call gb_Module_add_targets,setup_native,\
#       $(if $(filter MACOSX,$(OS)),CustomTarget_mac) \
#       CustomTarget_spell \
#       $(if $(filter WNT,$(OS)),Package_misc) \
#       Package_packinfo \
#       $(if $(filter LINUX SOLARIS,$(OS)), \
#           CustomTarget_scripts \
#           $(if $(ENABLE_ONLINE_UPDATE),Package_scripts) \
#       ) \
#   ))
#   ifeq ($(OS),WNT)
#   $(eval $(call gb_Module_add_targets,setup_native,\
#       Library_instooofiltmsi Library_qslnkmsi Library_reg_dlls \
#       Library_reg4allmsdoc Library_regactivex Library_sdqsmsi \
#       Library_sellangmsi Library_shlxtmsi Library_sn_tools \
#       StaticLibrary_quickstarter StaticLibrary_seterror \
#   ))
#   endif
#
# =============================================================================
# IMPORTANT CORRECTION relative to this module's task description
# =============================================================================
#
# This module's assignment described setup_native/CustomTarget_scripts.mk as
# "generating the soffice/scalc/swriter/etc. shell-wrapper scripts" that
# desktop/CMakeLists.txt's own header comment flags as missing. Having read
# CustomTarget_scripts.mk, setup_native/scripts/install_linux.sh,
# uninstall_linux.sh and install_create.pl in full, that is NOT what this
# target does, and the two are unrelated:
#
#   - The actual soffice/scalc/sdraw/simpress/smath/swriter/sbase
#     shell-wrapper scripts desktop/CMakeLists.txt's comment refers to are
#     produced by seven *separate* targets that live in a *different*
#     module: desktop/Package_soffice_sh.mk, desktop/Package_scalc_sh.mk,
#     desktop/Package_sdraw_sh.mk, desktop/Package_simpress_sh.mk,
#     desktop/Package_smath_sh.mk, desktop/Package_swriter_sh.mk,
#     desktop/Package_sbase_sh.mk (all read directly for this pass). Every
#     one of them is a single gb_Package_add_file call copying one static,
#     already-complete script from desktop/scripts/*.sh straight into
#     program/ under a new name (e.g. `gb_Package_add_file,desktop_soffice_sh,
#     $(LIBO_BIN_FOLDER)/soffice,soffice.sh` -- no templating, no
#     generation, no CustomTarget involved at all). Those targets are
#     Module_desktop.mk's, not Module_setup_native.mk's, and desktop/
#     CMakeLists.txt is explicitly off limits for this task -- so this gap
#     genuinely cannot be closed from here. Whoever next edits desktop/
#     CMakeLists.txt can close it directly with seven
#     `install(PROGRAMS "${CMAKE_SOURCE_DIR}/desktop/scripts/<x>.sh"
#     DESTINATION program RENAME <x>)` calls (the exact pattern cpputools/
#     CMakeLists.txt already uses for uno.sh -> uno); no setup_native
#     involvement needed.
#   - setup_native/CustomTarget_scripts.mk instead builds the *old-style
#     self-extracting userland RPM installer* used by
#     setup_native/scripts/install_linux.sh: a "fake" RPM (built from
#     scripts/fake-db.spec via `rpm -bb`, used only to seed a private,
#     non-root RPM database) plus install_create.pl, which concatenates
#     install_linux.sh's own header with that fake .rpm's bytes into one
#     self-extracting shell script. It is gated `ifeq ($(OS),LINUX)` +
#     `ifneq ($(filter rpm,$(PKGFORMAT)),)` -- a legacy Linux *packaging*
#     mechanism (predating system package managers being assumed present),
#     unrelated to how soffice.bin itself is launched. Transcribed for real
#     below anyway, per this task's explicit instruction to make a genuine
#     attempt at it, but it should not be read as closing the wrapper-
#     script gap -- that gap remains open, and is out of this module's
#     reach.
# =============================================================================

# -----------------------------------------------------------------------
# CustomTarget_mac.mk -- ULF-translated (localized) wizard strings for the
# macOS .pkg installer UI (macinstall.ulf -> per-language .po substitution
# via gb_CustomTarget_ulfex_rule). Pure localization/packaging text, no
# compiled code, consumed only by the not-ported macOS installer build
# (CustomTarget_instsetoo_native) -- same "UI config / localization, out of
# scope" verdict this port already applies elsewhere (see e.g. desktop/
# CMakeLists.txt's own UIConfig_deployment/AllLangMoTarget_dkt exclusion
# note). Not reproduced; APPLE-gated in gbuild, so inert on this Linux
# port either way.
# -----------------------------------------------------------------------

# -----------------------------------------------------------------------
# CustomTarget_spell.mk -- generates spellchecker_selection.hxx (a C++
# header of per-language default-dictionary tables) from
# source/packinfo/spellchecker_selection.txt via a small, self-contained
# perl script (spellchecker_selection.pl, stdin -> stdout, no other
# inputs) -- NOT OS-gated in Module_setup_native.mk, so nominally this
# runs on every platform. Its only in-tree consumer, though, is
# setup_native's own Library_sellangmsi.mk (gb_Library_use_custom_headers,
# sellangmsi,setup_native/spell) -- a Windows-only MSI custom-action DLL
# (built below, behind if (WIN32)) -- and scp2's installer-definition
# language tables, which are not ported to CMake in this port at all
# (`find scp2 -iname CMakeLists.txt` is empty). Since this is real,
# self-contained, cheap-to-run generation (unlike CustomTarget_mac/
# Package_packinfo below), it is implemented for real rather than just
# documented, gated only on perl actually being found -- so sellangmsi
# below has a genuine generated header to point at instead of a dangling
# reference.
# -----------------------------------------------------------------------
find_program(SETUP_NATIVE_PERL_EXECUTABLE perl)

set(SETUP_NATIVE_SPELL_DIR "${CMAKE_CURRENT_BINARY_DIR}/spell")
set(SETUP_NATIVE_SPELL_HEADER "${SETUP_NATIVE_SPELL_DIR}/spellchecker_selection.hxx")

if (SETUP_NATIVE_PERL_EXECUTABLE)
    file(MAKE_DIRECTORY "${SETUP_NATIVE_SPELL_DIR}")
    add_custom_command(
        OUTPUT "${SETUP_NATIVE_SPELL_HEADER}"
        COMMAND "${SETUP_NATIVE_PERL_EXECUTABLE}" -w
                "${CMAKE_SOURCE_DIR}/setup_native/source/packinfo/spellchecker_selection.pl"
                < "${CMAKE_SOURCE_DIR}/setup_native/source/packinfo/spellchecker_selection.txt"
                > "${SETUP_NATIVE_SPELL_HEADER}"
        DEPENDS
            "${CMAKE_SOURCE_DIR}/setup_native/source/packinfo/spellchecker_selection.txt"
            "${CMAKE_SOURCE_DIR}/setup_native/source/packinfo/spellchecker_selection.pl"
        COMMENT "perl: setup_native/spell/spellchecker_selection.hxx"
        VERBATIM)
    add_custom_target(setup_native_spell DEPENDS "${SETUP_NATIVE_SPELL_HEADER}")
    # Not attached to ALL: nothing in this port's own add_subdirectory graph
    # consumes this header today (its one real consumer, sellangmsi, only
    # exists behind if (WIN32) below) -- build it on demand
    # (`cmake --build . --target setup_native_spell`) or let sellangmsi's own
    # add_dependencies() below pull it in once that branch is active.
endif ()

# -----------------------------------------------------------------------
# Package_misc.mk -- WNT-only (module-level gate). One file: win32/
# desktophelper.txt, copied verbatim to program/desktophelper.txt. A real,
# if trivial, one-line static-file install, reproduced directly (the same
# "small enough that pure-copy is not worth skipping" call this port's own
# cpputools/CMakeLists.txt already made for uno.sh -> uno).
# -----------------------------------------------------------------------
if (WIN32)
    install(FILES "${CMAKE_SOURCE_DIR}/setup_native/source/win32/desktophelper.txt"
        DESTINATION program)
endif ()

# -----------------------------------------------------------------------
# Package_packinfo.mk -- packages the *entire* setup_native/source
# directory (gb_Package_Package,setup_native_packinfo,$(SRCDIR)) for the
# not-ported native-installer build, adding one conditional empty
# extensions/ directory placeholder when no bundled extensions/dictionaries
# are configured. This is installer-packaging plumbing with no compiled
# code and no consumer anywhere in this port's own CMake graph (its
# consumer, CustomTarget_instsetoo_native, is not ported) -- not
# reproduced, matching this port's established treatment of pure
# gb_Package_*/gb_CustomTarget_* packaging targets with no in-tree
# consumer (see desktop/CMakeLists.txt's own Package_branding/
# Package_scripts exclusion note for the identical reasoning applied to
# its own module).
# -----------------------------------------------------------------------

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # -----------------------------------------------------------------------
    # Package_scripts.mk -- LINUX/SOLARIS + ENABLE_ONLINE_UPDATE (this
    # port's own already-tracked, real CACHE BOOL, cmake/
    # LibreStudioConfigHeaders.cmake, default OFF -- see that option's own
    # comment for the extensions/updchk feature it gates elsewhere in this
    # port). One file: scripts/unpack_update.sh -> program/unpack_update,
    # the script the online-update UNO service (extensions' updchk, when
    # ENABLE_ONLINE_UPDATE is ever turned on) execs to apply a downloaded
    # update. Real, trivial, one-line static-file install -- reproduced
    # directly, same call as Package_misc above.
    # -----------------------------------------------------------------------
    if (ENABLE_ONLINE_UPDATE)
        install(PROGRAMS "${CMAKE_SOURCE_DIR}/setup_native/scripts/unpack_update.sh"
            DESTINATION program RENAME unpack_update)
    endif ()

    # -----------------------------------------------------------------------
    # CustomTarget_scripts.mk -- see the "IMPORTANT CORRECTION" block above
    # for what this actually is (the legacy self-extracting userland RPM
    # installer, NOT the soffice/scalc/... wrapper scripts). Real gate:
    # `ifeq ($(OS),LINUX)` + `ifneq ($(filter rpm,$(PKGFORMAT)),)`.
    # PKGFORMAT itself is not a switch this configure-less CMake port
    # tracks anywhere (no module so far reads it) -- treated the same way
    # this port treats every other untracked-configure-switch (e.g.
    # ENABLE_BREAKPAD, ENABLE_PAGEIN: "untracked -> off" by default), via a
    # local option below rather than silently assuming it on or off without
    # a way to flip it.
    #
    # Three real recipes, transcribed directly from the .mk:
    #   1. noarch/fake-db-1.0-0.noarch.rpm: `rpm --define "_builddir ..."
    #      --define "_rpmdir ..." -bb fake-db.spec` -- an empty placeholder
    #      RPM used only to seed a private RPM database (see
    #      install_linux.sh's own "populate the private rpm database"
    #      step), read in full to confirm it installs no files of its own.
    #   2. install: `perl -w install_create.pl install_linux.sh
    #      <fake-db.rpm> install` -- concatenates install_linux.sh's own
    #      text with the fake .rpm's bytes appended (install_create.pl
    #      rewrites install_linux.sh's own `linenum=` marker first, so the
    #      script can `tail -n +$linenum $0` its own trailing binary
    #      payload at run time -- verified by reading install_linux.sh
    #      line ~245, "linenum=???" / "tail -n +$linenum $0 > $FAKEDBRPM").
    #   3. uninstall: a straight CRLF-stripped copy of uninstall_linux.sh
    #      (`tr -d "\015"`).
    # -----------------------------------------------------------------------
    option(SETUP_NATIVE_BUILD_RPM_USERLAND_INSTALLER
        "Build the legacy self-extracting RPM userland install/uninstall scripts (setup_native/CustomTarget_scripts.mk; upstream gate: OS=LINUX and PKGFORMAT contains \"rpm\", neither of which this configure-less port auto-detects)"
        OFF)

    if (SETUP_NATIVE_BUILD_RPM_USERLAND_INSTALLER)
        find_program(SETUP_NATIVE_RPM_EXECUTABLE rpm REQUIRED)
        if (NOT SETUP_NATIVE_PERL_EXECUTABLE)
            message(FATAL_ERROR
                "setup_native: SETUP_NATIVE_BUILD_RPM_USERLAND_INSTALLER needs perl (install_create.pl), but perl was not found")
        endif ()

        set(SETUP_NATIVE_SCRIPTS_DIR "${CMAKE_CURRENT_BINARY_DIR}/scripts")
        file(MAKE_DIRECTORY "${SETUP_NATIVE_SCRIPTS_DIR}/fake-db-root")

        set(SETUP_NATIVE_FAKEDB_RPM "${SETUP_NATIVE_SCRIPTS_DIR}/noarch/fake-db-1.0-0.noarch.rpm")
        add_custom_command(
            OUTPUT "${SETUP_NATIVE_FAKEDB_RPM}"
            COMMAND "${SETUP_NATIVE_RPM_EXECUTABLE}"
                    --define "_builddir ${SETUP_NATIVE_SCRIPTS_DIR}/fake-db-root"
                    --define "_rpmdir ${SETUP_NATIVE_SCRIPTS_DIR}"
                    -bb "${CMAKE_SOURCE_DIR}/setup_native/scripts/fake-db.spec"
            DEPENDS "${CMAKE_SOURCE_DIR}/setup_native/scripts/fake-db.spec"
            COMMENT "rpm: fake-db-1.0-0.noarch.rpm"
            VERBATIM)

        set(SETUP_NATIVE_RPM_INSTALL_SCRIPT "${SETUP_NATIVE_SCRIPTS_DIR}/install")
        add_custom_command(
            OUTPUT "${SETUP_NATIVE_RPM_INSTALL_SCRIPT}"
            COMMAND "${SETUP_NATIVE_PERL_EXECUTABLE}" -w
                    "${CMAKE_SOURCE_DIR}/setup_native/scripts/install_create.pl"
                    "${CMAKE_SOURCE_DIR}/setup_native/scripts/install_linux.sh"
                    "${SETUP_NATIVE_FAKEDB_RPM}"
                    "${SETUP_NATIVE_RPM_INSTALL_SCRIPT}"
            COMMAND ${CMAKE_COMMAND} -E chmod 775 "${SETUP_NATIVE_RPM_INSTALL_SCRIPT}"
            DEPENDS
                "${CMAKE_SOURCE_DIR}/setup_native/scripts/install_linux.sh"
                "${CMAKE_SOURCE_DIR}/setup_native/scripts/install_create.pl"
                "${SETUP_NATIVE_FAKEDB_RPM}"
            COMMENT "perl: setup_native RPM userland installer script"
            VERBATIM)

        set(SETUP_NATIVE_RPM_UNINSTALL_SCRIPT "${SETUP_NATIVE_SCRIPTS_DIR}/uninstall")
        add_custom_command(
            OUTPUT "${SETUP_NATIVE_RPM_UNINSTALL_SCRIPT}"
            # `cat $< | tr -d "\015" > $@; chmod 775 $@` -- CRLF-strip copy.
            COMMAND sh -c "tr -d '\\015' < '${CMAKE_SOURCE_DIR}/setup_native/scripts/uninstall_linux.sh' > '${SETUP_NATIVE_RPM_UNINSTALL_SCRIPT}'"
            COMMAND ${CMAKE_COMMAND} -E chmod 775 "${SETUP_NATIVE_RPM_UNINSTALL_SCRIPT}"
            DEPENDS "${CMAKE_SOURCE_DIR}/setup_native/scripts/uninstall_linux.sh"
            COMMENT "generate: setup_native RPM userland uninstaller script"
            VERBATIM)

        add_custom_target(setup_native_rpm_userland_installer ALL DEPENDS
            "${SETUP_NATIVE_RPM_INSTALL_SCRIPT}"
            "${SETUP_NATIVE_RPM_UNINSTALL_SCRIPT}")

        # Neither script is installed anywhere by the .mk itself (they are
        # build products of the *installer* build, picked up by
        # instsetoo_native's own packaging step, not by `make install`) --
        # left as build-tree artifacts only, matching upstream's own scope.
    endif ()
endif ()

# =============================================================================
# Windows-only targets (module-level `ifeq ($(OS),WNT)` block): ten MSI
# custom-action DLLs/static libs. Every one of these is small, self-
# contained Win32 code (no UNO/ATL, no udkapi/offapi) built with the classic
# "/MT, -U_DLL, /NODEFAULTLIB, linked via its own .def" pattern already seen
# in cpputools' sp2bv and embedserv above -- transcribed for real behind
# if (WIN32), per this port's policy of not silently dropping Windows-only
# gbuild targets. Inert on this Linux build.
# =============================================================================
if (WIN32)
    # --- StaticLibrary_quickstarter / StaticLibrary_seterror ---------------
    # Two tiny static helper libraries, each wrapping exactly one .cxx,
    # consumed by several of the DLLs below.
    add_library(quickstarter STATIC
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/quickstarter/quickstarter.cxx")
    if (MSVC)
        target_compile_definitions(quickstarter PRIVATE -U_DLL)
        target_compile_options(quickstarter PRIVATE
            $<IF:$<BOOL:${MSVC_USE_DEBUG_RUNTIME}>,/MTd,/MT>)
    endif ()

    add_library(seterror STATIC
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/tools/seterror.cxx")
    if (MSVC)
        target_compile_definitions(seterror PRIVATE -U_DLL)
        target_compile_options(seterror PRIVATE
            $<IF:$<BOOL:${MSVC_USE_DEBUG_RUNTIME}>,/MTd,/MT>)
    endif ()

    # Small helper: every MSI custom-action DLL below follows the exact
    # same "/MT(d), -U_DLL, /DEF:<...>.def, /NODEFAULTLIB" shape, differing
    # only in sources/link libs -- factored once here instead of repeating
    # the four identical lines ten times.
    function(setup_native_msi_dll _name _def)
        if (MSVC)
            target_compile_definitions(${_name} PRIVATE -U_DLL)
            target_compile_options(${_name} PRIVATE
                $<IF:$<BOOL:${MSVC_USE_DEBUG_RUNTIME}>,/MTd,/MT>)
            set_target_properties(${_name} PROPERTIES
                LINK_FLAGS "/DEF:${_def} /NODEFAULTLIB")
        endif ()
    endfunction()

    # --- Library_instooofiltmsi ---------------------------------------------
    add_library(instooofiltmsi SHARED
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/indexingfilter/restartindexingservice.cxx")
    setup_native_msi_dll(instooofiltmsi
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/indexingfilter/instooofiltmsi.def")
    target_link_libraries(instooofiltmsi PRIVATE kernel32 advapi32)

    # --- Library_qslnkmsi ----------------------------------------------------
    add_library(qslnkmsi SHARED
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/quickstarter/remove_quickstart_link.cxx")
    setup_native_msi_dll(qslnkmsi
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/quickstarter/qslnkmsi.def")
    target_link_libraries(qslnkmsi PRIVATE quickstarter msi kernel32 user32 advapi32 shell32)

    # --- Library_reg4allmsdoc -------------------------------------------------
    add_library(reg4allmsdoc SHARED
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/reg4allmsdoc/reg4allmsi.cxx")
    setup_native_msi_dll(reg4allmsdoc
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/reg4allmsdoc/reg4allmsdoc.def")
    target_link_libraries(reg4allmsdoc PRIVATE msi kernel32 user32 advapi32 shell32 shlwapi)

    # --- Library_regactivex ----------------------------------------------------
    add_library(regactivex SHARED
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/regactivex/regactivex.cxx")
    setup_native_msi_dll(regactivex
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/regactivex/regactivex.def")
    target_link_libraries(regactivex PRIVATE kernel32 msi)

    # --- Library_reg_dlls -------------------------------------------------------
    add_library(reg_dlls SHARED
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/reg_dlls/reg_dlls.cxx")
    setup_native_msi_dll(reg_dlls
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/reg_dlls/reg_dlls.def")
    target_link_libraries(reg_dlls PRIVATE kernel32 ole32 shell32 msi)

    # --- Library_sdqsmsi -------------------------------------------------------
    add_library(sdqsmsi SHARED
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/quickstarter/shutdown_quickstart.cxx")
    setup_native_msi_dll(sdqsmsi
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/quickstarter/sdqsmsi.def")
    target_link_libraries(sdqsmsi PRIVATE quickstarter msi kernel32 user32 advapi32 shell32)

    # --- Library_sellangmsi ------------------------------------------------------
    # use_custom_headers,sellangmsi,setup_native/spell -- the
    # spellchecker_selection.hxx generated by CustomTarget_spell above.
    add_library(sellangmsi SHARED
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/sellang/sellang.cxx"
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/sellang/sorttree.cxx")
    target_include_directories(sellangmsi PRIVATE "${SETUP_NATIVE_SPELL_DIR}")
    if (TARGET setup_native_spell)
        add_dependencies(sellangmsi setup_native_spell)
    endif ()
    setup_native_msi_dll(sellangmsi
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/sellang/sellang.def")
    target_link_libraries(sellangmsi PRIVATE msi kernel32 user32 advapi32 shell32)

    # --- Library_shlxtmsi -------------------------------------------------------
    add_library(shlxtmsi SHARED
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/shellextensions/startmenuicon.cxx"
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/shellextensions/upgrade.cxx"
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/shellextensions/migrateinstallpath.cxx"
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/shellextensions/completeinstallpath.cxx"
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/shellextensions/checkdirectory.cxx"
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/shellextensions/checkpatches.cxx")
    target_link_libraries(shlxtmsi PRIVATE Boost::boost)
    setup_native_msi_dll(shlxtmsi
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/shellextensions/shlxtmsi.def")
    target_link_libraries(shlxtmsi PRIVATE seterror msi kernel32 advapi32 shell32 psapi)

    # --- Library_sn_tools --------------------------------------------------------
    add_library(sn_tools SHARED
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/tools/checkversion.cxx")
    setup_native_msi_dll(sn_tools
        "${CMAKE_SOURCE_DIR}/setup_native/source/win32/customactions/tools/sn_tools.def")
    target_link_libraries(sn_tools PRIVATE seterror kernel32 msi advapi32)
endif ()
