# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# wizards -- Module_wizards.mk, read in full, is a genuinely split module:
#
#  - Package_* (Basic/.xba-.xdl-.xlb macro libraries, StarBasic-implemented
#    wizards/services): real DATA, installed for real below. This port's
#    Basic interpreter and dialog runtime (basic/, basctl/, vcl's dialog
#    layer) are already ported and working, so this content is genuinely
#    reachable at runtime, not just inert files -- unlike extras' font/
#    l10n-pipeline gaps.
#  - Pyuno_* (Python-implemented wizard services): the .py payload installs
#    for real; the UNO *service registration* half is a documented gap (see
#    below -- it needs infrastructure this port's shared component-registry
#    helper does not have yet, not something wizards/CMakeLists.txt can fix
#    on its own without touching cmake/LibreStudioComponent.cmake, which is
#    outside this module).
#  - Jar_* (Java-implemented Base wizard classes: form/query/report/
#    reportbuilder/table + their common/commonwizards support jar): gated
#    behind LIBRESTUDIO_ENABLE_JAVA, matching Module_wizards.mk's own `ifeq
#    ($(ENABLE_JAVA),TRUE)` gate. 2026-09-28: now ported for real (all 6 jars
#    build), deferred into wizards/Jar_wizards_deferred.cmake -- see the
#    dedicated section below for why deferred, and for the one real
#    fidelity gap this creates (form/query/report/table's own UNO components
#    land in a dedicated, non-live "wizards" registry rather than the shared
#    "services" one upstream's own gb_Jar_set_componentfile targets).
#  - AllLangMoTarget_wiz (gb_Module_add_l10n_targets): translation .mo
#    catalogs, out of scope like every other l10n-only target elsewhere in
#    this port (this build has no gb_WITH_LANG equivalent).

# --- CustomTarget_share + Package_share: dialog.xlc / script.xlc -----------
#
# CustomTarget_share.mk, read in full, is a single sed substitution per file:
#   sed -e 's/@LIBO_SHARE_FOLDER@/$(LIBO_SHARE_FOLDER)/' <in >out
# against wizards/source/configshare/{dialog,script}.xlc (both verified
# present). LIBO_SHARE_FOLDER=share on this platform (configure.ac). Package_
# share.mk installs the two results to $(LIBO_SHARE_FOLDER)/basic, i.e.
# share/basic.
set(WIZARDS_CONFIGSHARE_SRCDIR "${CMAKE_CURRENT_SOURCE_DIR}/source/configshare")
set(WIZARDS_CONFIGSHARE_BINDIR "${CMAKE_CURRENT_BINARY_DIR}/share")

foreach (_xlc dialog script)
    set(_in "${WIZARDS_CONFIGSHARE_SRCDIR}/${_xlc}.xlc")
    set(_out "${WIZARDS_CONFIGSHARE_BINDIR}/${_xlc}.xlc")
    add_custom_command(
        OUTPUT "${_out}"
        COMMAND sh -c "sed -e 's/@LIBO_SHARE_FOLDER@/share/' '${_in}' > '${_out}'"
        DEPENDS "${_in}"
        COMMENT "wizards: ${_xlc}.xlc (CustomTarget_share.mk)"
        VERBATIM)
    list(APPEND WIZARDS_CONFIGSHARE_OUT "${_out}")
endforeach ()
unset(_xlc)
unset(_in)
unset(_out)

add_custom_target(wizards_basicshare_files ALL DEPENDS ${WIZARDS_CONFIGSHARE_OUT})

install(FILES ${WIZARDS_CONFIGSHARE_OUT} DESTINATION share/basic)

# gb_Package_Package(wizards_basicshare,$(gb_CustomTarget_workdir)/wizards/
# share) + gb_Package_add_files(wizards_basicshare,$(LIBO_SHARE_FOLDER)/basic,
# dialog.xlc script.xlc): the actual named package CppunitTest_desktop_lib.mk
# needs (its own use_packages list names "wizards_basicshare", not
# "wizards_basicshare_files" above -- that target is this port's own
# CustomTarget_share.mk-equivalent staging step, one level earlier than the
# real Package_share.mk target upstream separately names). Mirrors the two
# sed-substituted files from that staging step into share/basic directly (no
# per-library subdirectory, unlike every wizards_basicsrv* package below --
# gb_Package_add_files's own destdir argument here is $(LIBO_SHARE_FOLDER)/
# basic itself, verified against the live Package_share.mk), giving PACKAGES
# a real build-time target under the exact name gbuild uses.
librestudio_add_package_mirror(wizards_basicshare
    PAIRS
        "${WIZARDS_CONFIGSHARE_BINDIR}/dialog.xlc:${LIBRESTUDIO_SHARE_DIR}/basic"
        "${WIZARDS_CONFIGSHARE_BINDIR}/script.xlc:${LIBRESTUDIO_SHARE_DIR}/basic")
add_dependencies(wizards_basicshare wizards_basicshare_files)

# --- CustomTarget_wizards (locproperties): confirmed zero real targets ------
#
# CustomTarget_wizards.mk registers its resources_<lang>.properties recipe
# generation `ifneq ($(strip $(gb_WITH_LANG)),)` only -- wizards_LANGS :=
# $(filter-out en-US,$(gb_WITH_LANG)), and the loop that would call
# wizards_Properties_Properties for each language is itself inside that same
# guard. gb_WITH_LANG is unset in this port (no localized-build
# infrastructure at all, per this file's own module-level note above), so
# wizards_LANGS is empty and the foreach loop runs zero times: this
# CustomTarget produces no real recipes on this port's (or any WITH_LANG=
# unset) configuration -- verified directly from the .mk, not assumed, same
# kind of zero-target-on-this-config finding as static/Module_static.mk.
#
# --- Package_wizards_properties: consequently empty -------------------------
#
# Package_wizards_properties.mk installs
# $(foreach lang,$(subst -,_,$(wizards_LANGS)),resources_$(lang).properties)
# -- with wizards_LANGS empty, this expands to nothing. Nothing to install()
# here either; the base-language resources file is handled by Package_
# wizards below instead.

# --- Package_wizards: resources_en_US.properties -----------------------------
#
# wizards/source/resources/resources_en_US.properties (verified present),
# installed to $(LIBO_SHARE_FOLDER)/wizards = share/wizards.
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/source/resources/resources_en_US.properties"
    DESTINATION share/wizards)

# --- Basic-macro wizard content (Package_access2base .. Package_usr) -------
#
# All of the following are gb_Package_Package(name,$(SRCDIR)/wizards/source/
# <dir>) + gb_Package_add_files(name,<dest>,<flat file list>) -- a plain,
# un-nested copy of real, checked-in .xba/.xdl/.xlb/.py files (StarBasic
# source, dialog definitions, library indexes) into share/basic/<LibraryName>
# (or presets/basic/<LibraryName> for the two libraries gbuild ships under
# LIBO_SHARE_PRESETS_FOLDER instead). Every source directory below was
# verified present on disk (ls) before transcribing its file list.

# Package_access2base.mk
install(FILES
    source/access2base/_License.xba
    source/access2base/acConstants.xba
    source/access2base/Application.xba
    source/access2base/Collect.xba
    source/access2base/CommandBar.xba
    source/access2base/CommandBarControl.xba
    source/access2base/Control.xba
    source/access2base/Database.xba
    source/access2base/DataDef.xba
    source/access2base/Dialog.xba
    source/access2base/dialog.xlb
    source/access2base/dlgFormat.xdl
    source/access2base/dlgTrace.xdl
    source/access2base/DoCmd.xba
    source/access2base/Event.xba
    source/access2base/Field.xba
    source/access2base/Form.xba
    source/access2base/L10N.xba
    source/access2base/Methods.xba
    source/access2base/Module.xba
    source/access2base/OptionGroup.xba
    source/access2base/PropertiesGet.xba
    source/access2base/PropertiesSet.xba
    source/access2base/Property.xba
    source/access2base/Python.xba
    source/access2base/Recordset.xba
    source/access2base/Root_.xba
    source/access2base/script.xlb
    source/access2base/SubForm.xba
    source/access2base/TempVar.xba
    source/access2base/Test.xba
    source/access2base/Trace.xba
    source/access2base/UtilProperty.xba
    source/access2base/Utils.xba
    DESTINATION share/basic/Access2Base)
install(FILES source/access2base/access2base.py DESTINATION program)
librestudio_add_package_mirror(wizards_basicsrvaccess2base
    PAIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/_License.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/acConstants.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/Application.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/Collect.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/CommandBar.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/CommandBarControl.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/Control.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/Database.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/DataDef.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/Dialog.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/dialog.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/dlgFormat.xdl:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/dlgTrace.xdl:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/DoCmd.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/Event.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/Field.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/Form.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/L10N.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/Methods.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/Module.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/OptionGroup.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/PropertiesGet.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/PropertiesSet.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/Property.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/Python.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/Recordset.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/Root_.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/script.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/SubForm.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/TempVar.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/Test.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/Trace.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/UtilProperty.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/Utils.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Access2Base"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/access2base/access2base.py:${LIBRESTUDIO_PROGRAM_DIR}")


# Package_depot.mk
install(FILES
    source/depot/CommonLang.xba
    source/depot/Currency.xba
    source/depot/Depot.xba
    source/depot/dialog.xlb
    source/depot/Dialog2.xdl
    source/depot/Dialog3.xdl
    source/depot/Dialog4.xdl
    source/depot/Internet.xba
    source/depot/Lang_de.xba
    source/depot/Lang_en.xba
    source/depot/Lang_es.xba
    source/depot/Lang_fr.xba
    source/depot/Lang_it.xba
    source/depot/Lang_ja.xba
    source/depot/Lang_ko.xba
    source/depot/Lang_sv.xba
    source/depot/Lang_tw.xba
    source/depot/Lang_zh.xba
    source/depot/script.xlb
    source/depot/tools.xba
    DESTINATION share/basic/Depot)
librestudio_add_package_mirror(wizards_basicsrvdepot
    PAIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/CommonLang.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/Currency.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/Depot.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/dialog.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/Dialog2.xdl:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/Dialog3.xdl:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/Dialog4.xdl:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/Internet.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/Lang_de.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/Lang_en.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/Lang_es.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/Lang_fr.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/Lang_it.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/Lang_ja.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/Lang_ko.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/Lang_sv.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/Lang_tw.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/Lang_zh.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/script.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/Depot"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/depot/tools.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Depot")


# Package_form.mk
install(FILES
    source/formwizard/DBMeta.xba
    source/formwizard/develop.xba
    source/formwizard/dialog.xlb
    source/formwizard/DlgFormDB.xdl
    source/formwizard/FormWizard.xba
    source/formwizard/Language.xba
    source/formwizard/Layouter.xba
    source/formwizard/script.xlb
    source/formwizard/tools.xba
    DESTINATION share/basic/FormWizard)
librestudio_add_package_mirror(wizards_basicsrvform
    PAIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/formwizard/DBMeta.xba:${LIBRESTUDIO_SHARE_DIR}/basic/FormWizard"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/formwizard/develop.xba:${LIBRESTUDIO_SHARE_DIR}/basic/FormWizard"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/formwizard/dialog.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/FormWizard"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/formwizard/DlgFormDB.xdl:${LIBRESTUDIO_SHARE_DIR}/basic/FormWizard"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/formwizard/FormWizard.xba:${LIBRESTUDIO_SHARE_DIR}/basic/FormWizard"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/formwizard/Language.xba:${LIBRESTUDIO_SHARE_DIR}/basic/FormWizard"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/formwizard/Layouter.xba:${LIBRESTUDIO_SHARE_DIR}/basic/FormWizard"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/formwizard/script.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/FormWizard"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/formwizard/tools.xba:${LIBRESTUDIO_SHARE_DIR}/basic/FormWizard")


# Package_gimmicks.mk
install(FILES
    source/gimmicks/AutoText.xba
    source/gimmicks/ChangeAllChars.xba
    source/gimmicks/dialog.xlb
    source/gimmicks/GetTexts.xba
    source/gimmicks/ReadDir.xba
    source/gimmicks/ReadFolderDlg.xdl
    source/gimmicks/script.xlb
    source/gimmicks/UserfieldDlg.xdl
    source/gimmicks/Userfields.xba
    DESTINATION share/basic/Gimmicks)
librestudio_add_package_mirror(wizards_basicsrvgimmicks
    PAIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/gimmicks/AutoText.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Gimmicks"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/gimmicks/ChangeAllChars.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Gimmicks"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/gimmicks/dialog.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/Gimmicks"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/gimmicks/GetTexts.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Gimmicks"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/gimmicks/ReadDir.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Gimmicks"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/gimmicks/ReadFolderDlg.xdl:${LIBRESTUDIO_SHARE_DIR}/basic/Gimmicks"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/gimmicks/script.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/Gimmicks"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/gimmicks/UserfieldDlg.xdl:${LIBRESTUDIO_SHARE_DIR}/basic/Gimmicks"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/gimmicks/Userfields.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Gimmicks")


# Package_import.mk
install(FILES
    source/importwizard/API.xba
    source/importwizard/dialog.xlb
    source/importwizard/DialogModul.xba
    source/importwizard/FilesModul.xba
    source/importwizard/ImportDialog.xdl
    source/importwizard/Language.xba
    source/importwizard/Main.xba
    source/importwizard/script.xlb
    DESTINATION share/basic/ImportWizard)
librestudio_add_package_mirror(wizards_basicsrvimport
    PAIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/importwizard/API.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ImportWizard"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/importwizard/dialog.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/ImportWizard"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/importwizard/DialogModul.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ImportWizard"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/importwizard/FilesModul.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ImportWizard"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/importwizard/ImportDialog.xdl:${LIBRESTUDIO_SHARE_DIR}/basic/ImportWizard"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/importwizard/Language.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ImportWizard"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/importwizard/Main.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ImportWizard"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/importwizard/script.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/ImportWizard")

# Package_scriptforge.mk (three destinations: the .xba/.xdl/.xlb library
# itself, its Python helper under Scripts/python, its po/ translation
# templates alongside the library, and scriptforge.py/.pyi under program/).
install(FILES
    source/scriptforge/SF_Array.xba
    source/scriptforge/SF_Dictionary.xba
    source/scriptforge/SF_Exception.xba
    source/scriptforge/SF_FileSystem.xba
    source/scriptforge/SF_L10N.xba
    source/scriptforge/SF_Platform.xba
    source/scriptforge/SF_PythonHelper.xba
    source/scriptforge/SF_Region.xba
    source/scriptforge/SF_Root.xba
    source/scriptforge/SF_Services.xba
    source/scriptforge/SF_Session.xba
    source/scriptforge/SF_SharedMemory.xba
    source/scriptforge/SF_String.xba
    source/scriptforge/SF_TextStream.xba
    source/scriptforge/SF_Timer.xba
    source/scriptforge/SF_UI.xba
    source/scriptforge/SF_Utils.xba
    source/scriptforge/_CodingConventions.xba
    source/scriptforge/_ModuleModel.xba
    source/scriptforge/__License.xba
    source/scriptforge/dialog.xlb
    source/scriptforge/dlgConsole.xdl
    source/scriptforge/dlgProgress.xdl
    source/scriptforge/script.xlb
    DESTINATION share/basic/ScriptForge)
install(FILES source/scriptforge/python/ScriptForgeHelper.py DESTINATION share/Scripts/python)
install(FILES
    source/scriptforge/po/ScriptForge.pot
    source/scriptforge/po/en.po
    source/scriptforge/po/es.po
    source/scriptforge/po/fr.po
    source/scriptforge/po/pt.po
    DESTINATION share/basic/ScriptForge/po)
install(FILES
    source/scriptforge/python/scriptforge.py
    source/scriptforge/python/scriptforge.pyi
    DESTINATION program)
librestudio_add_package_mirror(wizards_basicsrvscriptforge
    PAIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/SF_Array.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/SF_Dictionary.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/SF_Exception.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/SF_FileSystem.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/SF_L10N.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/SF_Platform.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/SF_PythonHelper.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/SF_Region.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/SF_Root.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/SF_Services.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/SF_Session.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/SF_SharedMemory.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/SF_String.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/SF_TextStream.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/SF_Timer.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/SF_UI.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/SF_Utils.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/_CodingConventions.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/_ModuleModel.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/__License.xba:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/dialog.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/dlgConsole.xdl:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/dlgProgress.xdl:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/script.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/python/ScriptForgeHelper.py:${LIBRESTUDIO_SHARE_DIR}/Scripts/python"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/po/ScriptForge.pot:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge/po"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/po/en.po:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge/po"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/po/es.po:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge/po"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/po/fr.po:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge/po"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/po/pt.po:${LIBRESTUDIO_SHARE_DIR}/basic/ScriptForge/po"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/python/scriptforge.py:${LIBRESTUDIO_PROGRAM_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/scriptforge/python/scriptforge.pyi:${LIBRESTUDIO_PROGRAM_DIR}")


# Package_sfdatabases.mk
install(FILES
    source/sfdatabases/SF_Database.xba
    source/sfdatabases/SF_Dataset.xba
    source/sfdatabases/SF_Datasheet.xba
    source/sfdatabases/SF_Register.xba
    source/sfdatabases/__License.xba
    source/sfdatabases/dialog.xlb
    source/sfdatabases/script.xlb
    DESTINATION share/basic/SFDatabases)
librestudio_add_package_mirror(wizards_basicsrvsfdatabases
    PAIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdatabases/SF_Database.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDatabases"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdatabases/SF_Dataset.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDatabases"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdatabases/SF_Datasheet.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDatabases"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdatabases/SF_Register.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDatabases"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdatabases/__License.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDatabases"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdatabases/dialog.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/SFDatabases"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdatabases/script.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/SFDatabases")


# Package_sfdialogs.mk
install(FILES
    source/sfdialogs/SF_Dialog.xba
    source/sfdialogs/SF_DialogControl.xba
    source/sfdialogs/SF_DialogListener.xba
    source/sfdialogs/SF_DialogUtils.xba
    source/sfdialogs/SF_Register.xba
    source/sfdialogs/__License.xba
    source/sfdialogs/dialog.xlb
    source/sfdialogs/script.xlb
    DESTINATION share/basic/SFDialogs)
librestudio_add_package_mirror(wizards_basicsrvsfdialogs
    PAIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdialogs/SF_Dialog.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDialogs"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdialogs/SF_DialogControl.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDialogs"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdialogs/SF_DialogListener.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDialogs"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdialogs/SF_DialogUtils.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDialogs"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdialogs/SF_Register.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDialogs"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdialogs/__License.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDialogs"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdialogs/dialog.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/SFDialogs"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdialogs/script.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/SFDialogs")


# Package_sfdocuments.mk
install(FILES
    source/sfdocuments/SF_Base.xba
    source/sfdocuments/SF_Calc.xba
    source/sfdocuments/SF_Chart.xba
    source/sfdocuments/SF_Document.xba
    source/sfdocuments/SF_DocumentListener.xba
    source/sfdocuments/SF_Form.xba
    source/sfdocuments/SF_FormControl.xba
    source/sfdocuments/SF_FormDocument.xba
    source/sfdocuments/SF_Register.xba
    source/sfdocuments/SF_Shape.xba
    source/sfdocuments/SF_Writer.xba
    source/sfdocuments/__License.xba
    source/sfdocuments/dialog.xlb
    source/sfdocuments/script.xlb
    DESTINATION share/basic/SFDocuments)
librestudio_add_package_mirror(wizards_basicsrvsfdocuments
    PAIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdocuments/SF_Base.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDocuments"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdocuments/SF_Calc.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDocuments"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdocuments/SF_Chart.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDocuments"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdocuments/SF_Document.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDocuments"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdocuments/SF_DocumentListener.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDocuments"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdocuments/SF_Form.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDocuments"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdocuments/SF_FormControl.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDocuments"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdocuments/SF_FormDocument.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDocuments"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdocuments/SF_Register.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDocuments"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdocuments/SF_Shape.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDocuments"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdocuments/SF_Writer.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDocuments"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdocuments/__License.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFDocuments"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdocuments/dialog.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/SFDocuments"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfdocuments/script.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/SFDocuments")


# Package_sfunittests.mk
install(FILES
    source/sfunittests/SF_Register.xba
    source/sfunittests/SF_UnitTest.xba
    source/sfunittests/__License.xba
    source/sfunittests/dialog.xlb
    source/sfunittests/script.xlb
    DESTINATION share/basic/SFUnitTests)
librestudio_add_package_mirror(wizards_basicsrvsfunittests
    PAIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfunittests/SF_Register.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFUnitTests"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfunittests/SF_UnitTest.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFUnitTests"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfunittests/__License.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFUnitTests"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfunittests/dialog.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/SFUnitTests"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfunittests/script.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/SFUnitTests")


# Package_sfwidgets.mk
install(FILES
    source/sfwidgets/SF_Menu.xba
    source/sfwidgets/SF_MenuListener.xba
    source/sfwidgets/SF_ContextMenu.xba
    source/sfwidgets/SF_PopupMenu.xba
    source/sfwidgets/SF_Register.xba
    source/sfwidgets/SF_Toolbar.xba
    source/sfwidgets/SF_ToolbarButton.xba
    source/sfwidgets/__License.xba
    source/sfwidgets/dialog.xlb
    source/sfwidgets/script.xlb
    DESTINATION share/basic/SFWidgets)
librestudio_add_package_mirror(wizards_basicsrvsfwidgets
    PAIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfwidgets/SF_Menu.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFWidgets"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfwidgets/SF_MenuListener.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFWidgets"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfwidgets/SF_ContextMenu.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFWidgets"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfwidgets/SF_PopupMenu.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFWidgets"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfwidgets/SF_Register.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFWidgets"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfwidgets/SF_Toolbar.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFWidgets"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfwidgets/SF_ToolbarButton.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFWidgets"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfwidgets/__License.xba:${LIBRESTUDIO_SHARE_DIR}/basic/SFWidgets"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfwidgets/dialog.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/SFWidgets"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/sfwidgets/script.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/SFWidgets")


# Package_standard.mk -- note LIBO_SHARE_PRESETS_FOLDER (presets/), not
# LIBO_SHARE_FOLDER (share/): this is the seed for a *new user's* default
# Standard Basic library, copied into each new profile, not shared runtime
# data.
install(FILES
    source/standard/dialog.xlb
    source/standard/Module1.xba
    source/standard/script.xlb
    DESTINATION presets/basic/Standard)

# Package_template.mk
install(FILES
    source/template/Autotext.xba
    source/template/Correspondence.xba
    source/template/dialog.xlb
    source/template/DialogStyles.xdl
    source/template/ModuleAgenda.xba
    source/template/Samples.xba
    source/template/script.xlb
    source/template/TemplateDialog.xdl
    DESTINATION share/basic/Template)
librestudio_add_package_mirror(wizards_basicsrvtemplate
    PAIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/template/Autotext.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Template"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/template/Correspondence.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Template"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/template/dialog.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/Template"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/template/DialogStyles.xdl:${LIBRESTUDIO_SHARE_DIR}/basic/Template"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/template/ModuleAgenda.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Template"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/template/Samples.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Template"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/template/script.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/Template"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/template/TemplateDialog.xdl:${LIBRESTUDIO_SHARE_DIR}/basic/Template")


# Package_tools.mk
install(FILES
    source/tools/Debug.xba
    source/tools/dialog.xlb
    source/tools/DlgOverwriteAll.xdl
    source/tools/Listbox.xba
    source/tools/Misc.xba
    source/tools/ModuleControls.xba
    source/tools/script.xlb
    source/tools/Strings.xba
    source/tools/UCB.xba
    DESTINATION share/basic/Tools)
librestudio_add_package_mirror(wizards_basicsrvtools
    PAIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/tools/Debug.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Tools"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/tools/dialog.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/Tools"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/tools/DlgOverwriteAll.xdl:${LIBRESTUDIO_SHARE_DIR}/basic/Tools"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/tools/Listbox.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Tools"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/tools/Misc.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Tools"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/tools/ModuleControls.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Tools"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/tools/script.xlb:${LIBRESTUDIO_SHARE_DIR}/basic/Tools"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/tools/Strings.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Tools"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/tools/UCB.xba:${LIBRESTUDIO_SHARE_DIR}/basic/Tools")


# Package_tutorials.mk
install(FILES
    source/tutorials/dialog.xlb
    source/tutorials/Functions.xba
    source/tutorials/RoadMap.xba
    source/tutorials/script.xlb
    source/tutorials/ShowInfoDialog.xba
    source/tutorials/TutorialClose.xba
    source/tutorials/TutorialCloseDialog.xdl
    source/tutorials/TutorialCreator.xba
    source/tutorials/TutorialOpen.xba
    source/tutorials/TutorialOpenDialog.xdl
    source/tutorials/TutorialsDialog.xdl
    DESTINATION share/basic/Tutorials)

# Package_usr.mk -- the *default* dialog.xlc/script.xlc a fresh user profile
# starts from (as opposed to CustomTarget_share/Package_share's own two
# files above, which seed the shared program-wide basic/ tree). Same
# LIBO_SHARE_PRESETS_FOLDER convention as Package_standard.mk above.
install(FILES
    source/config/dialog.xlc
    source/config/script.xlc
    DESTINATION presets/basic)

# --- Pyuno_fax / Pyuno_letter / Pyuno_agenda / Pyuno_commonwizards ----------
#
# gb_Pyuno_add_files(component,destdir,files) installs plain .py files to
# $(LIBO_LIB_PYUNO_FOLDER)/<destdir>/<file> -- LIBO_LIB_PYUNO_FOLDER=program
# on this platform -- with no compile step (unlike Jar_*, below). All four
# source directories and every file below were verified present.
install(FILES
    com/sun/star/wizards/fax/__init__.py
    com/sun/star/wizards/fax/CallWizard.py
    com/sun/star/wizards/fax/CGFax.py
    com/sun/star/wizards/fax/CGFaxWizard.py
    com/sun/star/wizards/fax/FaxDocument.py
    com/sun/star/wizards/fax/FaxWizardDialogConst.py
    com/sun/star/wizards/fax/FaxWizardDialogImpl.py
    com/sun/star/wizards/fax/FaxWizardDialog.py
    com/sun/star/wizards/fax/FaxWizardDialogResources.py
    DESTINATION program/wizards/fax)

install(FILES
    com/sun/star/wizards/letter/CallWizard.py
    com/sun/star/wizards/letter/CGLetter.py
    com/sun/star/wizards/letter/CGLetterWizard.py
    com/sun/star/wizards/letter/CGPaperElementLocation.py
    com/sun/star/wizards/letter/__init__.py
    com/sun/star/wizards/letter/LetterDocument.py
    com/sun/star/wizards/letter/LetterWizardDialogConst.py
    com/sun/star/wizards/letter/LetterWizardDialogImpl.py
    com/sun/star/wizards/letter/LetterWizardDialog.py
    com/sun/star/wizards/letter/LetterWizardDialogResources.py
    DESTINATION program/wizards/letter)

install(FILES
    com/sun/star/wizards/agenda/AgendaDocument.py
    com/sun/star/wizards/agenda/AgendaWizardDialogConst.py
    com/sun/star/wizards/agenda/AgendaWizardDialogImpl.py
    com/sun/star/wizards/agenda/AgendaWizardDialog.py
    com/sun/star/wizards/agenda/AgendaWizardDialogResources.py
    com/sun/star/wizards/agenda/CallWizard.py
    com/sun/star/wizards/agenda/CGAgenda.py
    com/sun/star/wizards/agenda/CGTopic.py
    com/sun/star/wizards/agenda/__init__.py
    com/sun/star/wizards/agenda/TemplateConsts.py
    com/sun/star/wizards/agenda/TopicsControl.py
    DESTINATION program/wizards/agenda)

install(FILES
    com/sun/star/wizards/__init__.py
    com/sun/star/wizards/common/strings.hrc
    com/sun/star/wizards/common/ConfigGroup.py
    com/sun/star/wizards/common/ConfigSet.py
    com/sun/star/wizards/common/Configuration.py
    com/sun/star/wizards/common/Desktop.py
    com/sun/star/wizards/common/FileAccess.py
    com/sun/star/wizards/common/HelpIds.py
    com/sun/star/wizards/common/NoValidPathException.py
    com/sun/star/wizards/common/NumberFormatter.py
    com/sun/star/wizards/common/Properties.py
    com/sun/star/wizards/common/PropertyNames.py
    com/sun/star/wizards/common/SystemDialog.py
    com/sun/star/wizards/common/IRenderer.py
    com/sun/star/wizards/common/UCB.py
    com/sun/star/wizards/common/ListModel.py
    com/sun/star/wizards/common/__init__.py
    DESTINATION program/wizards/common)
install(FILES
    com/sun/star/wizards/document/OfficeDocument.py
    com/sun/star/wizards/document/__init__.py
    DESTINATION program/wizards/document)
install(FILES
    com/sun/star/wizards/text/TextDocument.py
    com/sun/star/wizards/text/TextElement.py
    com/sun/star/wizards/text/TextFieldHandler.py
    com/sun/star/wizards/text/TextSectionHandler.py
    com/sun/star/wizards/text/__init__.py
    DESTINATION program/wizards/text)
install(FILES
    com/sun/star/wizards/ui/ControlScroller.py
    com/sun/star/wizards/ui/DocumentPreview.py
    com/sun/star/wizards/ui/PathSelection.py
    com/sun/star/wizards/ui/PeerConfig.py
    com/sun/star/wizards/ui/UIConsts.py
    com/sun/star/wizards/ui/UnoDialog.py
    com/sun/star/wizards/ui/UnoDialog2.py
    com/sun/star/wizards/ui/WizardDialog.py
    com/sun/star/wizards/ui/__init__.py
    DESTINATION program/wizards/ui)
install(FILES
    com/sun/star/wizards/ui/event/CommonListener.py
    com/sun/star/wizards/ui/event/DataAware.py
    com/sun/star/wizards/ui/event/ListModelBinder.py
    com/sun/star/wizards/ui/event/RadioDataAware.py
    com/sun/star/wizards/ui/event/UnoDataAware.py
    com/sun/star/wizards/ui/event/Task.py
    com/sun/star/wizards/ui/event/TaskEvent.py
    com/sun/star/wizards/ui/event/TaskListener.py
    com/sun/star/wizards/ui/event/EventListenerList.py
    com/sun/star/wizards/ui/event/__init__.py
    DESTINATION program/wizards/ui/event)

# --- Pyuno UNO service registration: documented gap -------------------------
#
# gb_Pyuno_set_componentfile_full(fax,wizards/com/sun/star/wizards/fax/fax,
# vnd.openoffice.pymodule:wizards.fax,.CallWizard,services) -- and the
# equivalent calls for letter/agenda -- register each *.component file (real,
# checked-in XML naming a com.sun.star.loader.Python-loaded implementation,
# e.g. wizards/com/sun/star/wizards/fax/fax.component) into the "services"
# Rdb, the same one librestudio_finalize_components(services) packs (see
# ure/CMakeLists.txt and postprocess/CMakeLists.txt for that mechanism in
# depth).
#
# This port's own librestudio_add_component() helper (cmake/
# LibreStudioComponent.cmake), however, only knows how to build a component's
# uri from a compiled CMake LIBRARY target, via one of two fixed prefixes
# (vnd.sun.star.expand:$URE_INTERNAL_LIB_DIR/ or vnd.sun.star.expand:
# $LO_LIB_DIR/ -- see LIBRESTUDIO_COMPONENT_PREFIX_URELIB/_OOO in that file).
# A Python-loaded component's uri is the fixed, non-path string
# vnd.openoffice.pymodule:wizards.fax (com.sun.star.loader.Python resolves it
# by dotted Python module name, not by library path) -- there is no LIBRARY
# to pass, and no PREFIX/LAYER combination in librestudio_add_component()
# produces that scheme. Extending it to support a pymodule-style uri is a
# change to shared cmake/ infrastructure outside this module's scope (and
# outside the 6 modules this pass is authorized to touch).
#
# Net effect: the three wizards (fax/letter/agenda) install their .py
# payload for real (above) and are reachable by direct Basic/Python
# scripting, but are not yet registered as discoverable
# com.sun.star.task.Job UNO services the way upstream's packed services.rdb
# would make them (e.g. for the File > Wizards menu's automatic dispatch).
# Pyuno_commonwizards has no componentfile of its own (it is a shared library
# of .py modules the other three import), so it is unaffected by this gap.

# --- Jar_commonwizards / Jar_form / Jar_query / Jar_reportbuilder /
#     Jar_report / Jar_table -- gated behind LIBRESTUDIO_ENABLE_JAVA --------
#
# Module_wizards.mk: `ifeq ($(ENABLE_JAVA),TRUE) ... Jar_commonwizards Jar_
# form Jar_query Jar_report Jar_reportbuilder Jar_table endif`. Every one of
# these six Jars declares `gb_Jar_use_jars(<name>,libreoffice java_uno ...)`
# (Jar_commonwizards.mk and the rest, verified individually by reading each
# file in full), i.e. a hard *compile-classpath* dependency on java_uno.jar
# (bridges/CMakeLists.txt's own "--- java_uno ---" section,
# bridges/Jar_java_uno_deferred.cmake).
#
# 2026-09-28: ported for real, in wizards/Jar_wizards_deferred.cmake -- see
# that file's own header comment for the full dependency graph
# (commonwizards -> form/query/table, and commonwizards -> report ->
# reportbuilderwizard) and sourcefile transcription. Deferred rather than
# called directly here, for the same reason bridges/Jar_java_uno_deferred.
# cmake is itself deferred: every one of these six jars needs
# ${java_uno_JAR}, which is only set once THAT deferred call has already
# fired -- and deferred calls against the same DIRECTORY run in scheduling
# order, so scheduling this file's own DEFER call here (textually after
# bridges/CMakeLists.txt's own add_subdirectory()) guarantees java_uno_JAR
# is ready by the time it runs.
#
# One real, deliberate fidelity gap this creates: form/query/report/table's
# own gb_Jar_set_componentfile(...,OOO,services) calls upstream route into
# the shared "services" registry -- but librestudio_finalize_components(
# services) (root CMakeLists.txt) runs synchronously, at a fixed point in
# the plain, un-deferred pass over the root CMakeLists.txt, well before ANY
# deferred call in the whole tree (this file included) ever fires. A
# component registered from inside this file would therefore silently never
# reach services.rdb -- finalize_components() is a one-shot pack-what's-
# there-now operation, calling it a second time for the same name is a real
# CMake error (two rules for the same output file), so there is no way to
# "add to it later" either. Rather than leave these four components
# unregistered outright, they are routed to a dedicated "wizards" registry
# instead (wizards/Jar_wizards_deferred.cmake's own librestudio_finalize_
# components(wizards) call) -- the same accepted trade-off already used for
# scripting's own scriptproviderforbeanshell registry: a real, correctly
# packed .rdb, just not one the live application's own service manager ever
# loads. Fixing this for real would mean restructuring when "services"
# itself gets finalized (a much larger, riskier change touching every other
# already-working component in the tree), not something owned by wizards/
# CMakeLists.txt alone.
if (LIBRESTUDIO_ENABLE_JAVA)
    librestudio_include_deferred("${CMAKE_SOURCE_DIR}/wizards/Jar_wizards_deferred.cmake")
endif ()

# --- AllLangMoTarget_wiz: out of scope ---------------------------------------
#
# gb_Module_add_l10n_targets(wizards, AllLangMoTarget_wiz) -- compiles
# wizards' .po translations into .mo catalogs for every configured UI
# language. Like every other l10n-only target noted elsewhere in this pass
# (extras' AllLangPackage_autotextshare, sysui's desktop-translate.py stage),
# this port has no gb_WITH_LANG/translation-catalog infrastructure at all;
# nothing to build here for the default, single-language configuration this
# port targets.

