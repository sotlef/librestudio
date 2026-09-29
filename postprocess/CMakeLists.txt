# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# postprocess -- Module_postprocess.mk, read in full, adds (on this Linux,
# non-Windows-signing, PKGFORMAT-unset, USING_X11 build):
#
#   $(if $(ENABLE_CUSTOMTARGET_COMPONENTS),CustomTarget_components)  -- inactive, see below
#   CustomTarget_registry                                           -- PARTIALLY covered, see below
#   Rdb_services                                                    -- covered, see below
#   CustomTarget_images + Package_images        (gb_Side=host)      -- implemented, see below
#   CustomTarget_fontconfig + Package_fontconfig (USING_X11 branch) -- implemented for real, below
#   Package_registry                            (l10n target)       -- blocked on CustomTarget_registry gap
#   CppunitTest_singletons, CppunitTest_services (MPL_SUBSET != TRUE) -- ported for real, below (see the
#                                                                        "--- CppunitTest_singletons /
#                                                                        CppunitTest_services ---" section)
#
# CustomTarget_signing (WINDOWS_BUILD_SIGNING) and CustomTarget_check_dynamic_
# objects (OS=LINUX + PKGFORMAT set) are both inert on this build: the former
# is Windows-only, the latter needs PKGFORMAT (rpm/deb/...) which this
# configure-less CMake port never sets (grepped: PKGFORMAT appears nowhere in
# cmake/*.cmake or the root CMakeLists.txt) -- nothing to transcribe for
# either.

# --- Rdb_services: confirmed covered by librestudio_finalize_components() ---
#
# postprocess/Rdb_services.mk, read in full:
#
#   $(eval $(call gb_Rdb_Rdb_install,services))
#
# gb_Rdb_Rdb_install's single-argument form (solenv/gbuild/Rdb.mk) installs to
# gb_Rdb__get_install_target = $(INSTROOT)/$(LIBO_ETC_FOLDER)/services/$(1).rdb
# -- i.e., upstream, this would be program/services/services.rdb, collecting
# the ~188 gb_Library_set_componentfile(...,services) registrations (every
# module *except* the dozen or so low-level bootstrap libraries that instead
# target the separate "ure/services" Rdb -- see ure/CMakeLists.txt for that
# half of the same investigation).
#
# This port does not reproduce that split. Every ported library's
# librestudio_add_component() call -- regardless of whether its upstream
# Library_*.mk named "services" or "ure/services" as the componentfile
# target -- feeds one single global property, packed once by the root
# CMakeLists.txt's:
#
#   librestudio_finalize_components(services)
#   add_custom_command(OUTPUT "${LIBRESTUDIO_PROGRAM_DIR}/services.rdb" ...)
#   add_custom_target(program_services_rdb ALL DEPENDS "${LIBRESTUDIO_PROGRAM_DIR}/services.rdb")
#
# installed at program/services.rdb -- the path unorc's own built-in default
# UNO_SERVICES value already resolves to (root CMakeLists.txt's own comment
# beside program_services_rdb's dependents says so explicitly), not the
# split program/services/services.rdb path upstream's plain "services" Rdb
# would use on its own. So this is not "the same target renamed": this port
# deliberately unifies the two upstream registries (ure/services + services)
# into the one file the runtime bootstrap already looks for by default, and
# every component either registry would have held is present in it. Nothing
# is missing; re-declaring a second, upstream-shaped Rdb_services target here
# would either duplicate librestudio_finalize_components()'s work or produce
# a second, unconsulted services.rdb copy.

# --- CustomTarget_registry: PARTIALLY covered -- correcting an overstated
#     claim in PORTING_STATUS.md ---------------------------------------------
#
# PORTING_STATUS.md's postprocess row currently reads: "функциональность уже
# покрыта librestudio_finalize_components()+officecfg напрямую" (the
# functionality is already covered by librestudio_finalize_components() +
# officecfg directly). Checked directly against postprocess/CustomTarget_
# registry.mk (676 lines) and officecfg/CMakeLists.txt: this is only half
# true, and officecfg/CMakeLists.txt already says so itself.
#
# postprocess_XCDS (CustomTarget_registry.mk) lists twelve registry files:
#   base calc cjk ctl ctlseqcheck draw graphicfilter impress lingucomponent
#   main math writer xsltfilter        (all suffixed .xcd)
# each assembled the same way: an XcsTarget schema pass, an XcuDataTarget/
# XcuModuleTarget data pass, and a packregistry.xslt merge into one .xcd.
#
# officecfg/CMakeLists.txt (its own "--- main.xcd ---" section, and its
# explicit comment a few hundred lines earlier: "the ones main.xcd does not
# take are the schemas belonging to the per-module .xcd files -- calc.xcd,
# writer.xcd and the rest -- which this build does not produce yet") builds
# exactly ONE of these twelve: main.xcd, via
#
#   add_custom_command(OUTPUT "${LIBRESTUDIO_REGISTRY_DIR}/main.xcd" ...
#       COMMAND sh -c "${XSLTPROC_EXECUTABLE} --nonet '${OFFICECFG_PACKREGISTRY}' \
#           '${OFFICECFG_MAIN_LIST}' | sed -f '${OFFICECFG_MAIN_SED}' > ... main.xcd")
#
# The other eleven -- base.xcd, calc.xcd, cjk.xcd, ctl.xcd, ctlseqcheck.xcd,
# draw.xcd, graphicfilter.xcd, impress.xcd, lingucomponent.xcd, math.xcd,
# writer.xcd, xsltfilter.xcd -- are not produced anywhere in this tree yet.
# This is a real, currently-undocumented-elsewhere gap, not something already
# covered: officecfg's own module docstring already flags it, but
# PORTING_STATUS.md's postprocess row reads as if the whole of
# CustomTarget_registry.mk were subsumed, which overstates it.
#
# Consequence for Package_registry (below): it packages exactly these twelve
# .xcd files (plus per-language Langpack-*.xcd/cjk_*.xcd/ctl_*.xcd/
# ctlseqcheck_*.xcd/fcfg_langpack_*.xcd/registry_*.xcd variants -- see
# postprocess/Package_registry.mk), so it is blocked on the same gap for
# eleven twelfths of its payload; main.xcd alone is not enough to satisfy it.
#
# Not attempted here: building the other eleven .xcd files would mean
# transcribing eleven more postprocess_FILES_<theme>/postprocess_DEPS_<theme>
# variable groups (each naming its own XcsTarget/XcuDataTarget/
# XcuFilterFiltersTarget/XcuFilterTypesTarget/XcuFilterOthersTarget inputs) and
# extending officecfg's packregistry.xslt/XSLTPROC_EXECUTABLE mechanism to
# each -- a substantial, self-contained porting task in its own right, and out
# of scope for a documentation-only postprocess/CMakeLists.txt. Left as an
# honest gap, correcting the tracking table's overstatement above rather than
# perpetuating it.

# --- Package_registry: blocked on the CustomTarget_registry gap above -------
#
# postprocess/Package_registry.mk installs postprocess_XCDS (all twelve .xcd
# files above) plus per-language variants keyed off gb_Configuration_LANGS/
# gb_CJK_LANGS/gb_CTL_LANGS/gb_CTLSEQCHECK_LANGS -- none of which this
# configure-less CMake port defines (there is no localized-build concept
# wired anywhere in this tree, matching this port's general en-US-only
# stance). Even a single-language (en-US-only) instance of this Package still
# needs the eleven non-main .xcd files above, which do not exist yet.
# Deferred alongside CustomTarget_registry.

# --- CustomTarget_fontconfig + Package_fontconfig: implemented for real -----
#
# postprocess/CustomTarget_fontconfig.mk, read in full, is small and entirely
# static-data-driven once BUILD_TYPE does not contain MORE_FONTS (grepped:
# MORE_FONTS/external/more_fonts appear nowhere as an active flag in this
# port) and OS is not EMSCRIPTEN (this is a native Linux build): it just
# wraps two checked-in snippet files in a <fontconfig> XML envelope --
#
#   printf '<?xml version="1.0"?>\n<!DOCTYPE fontconfig SYSTEM "/etc/fonts/conf.d/fonts.dtd">\n<fontconfig>\n'
#   cat extras/source/truetype/symbol/fc_local.snippet
#   cat postprocess/fontconfig/fc_local.snippet
#   printf '</fontconfig>\n'
#
# Both source snippets exist on disk (verified) and the module gate itself
# -- `ifneq ($(USE_HEADLESS_CODE)$(USING_X11)$(DISABLE_GUI)$(filter ANDROID
# EMSCRIPTEN,$(OS)),)` -- holds on this build (USING_X11 is TRUE by default,
# cmake/LibreStudioConfigHeaders.cmake), so this is real, always-active
# content, not a deferred/optional one. Package_fontconfig.mk's destination
# ($(LIBO_SHARE_FOLDER)/fonts/truetype, i.e. share/fonts/truetype on this
# platform) is reproduced directly.
set(POSTPROCESS_FC_LOCAL_CONF "${CMAKE_CURRENT_BINARY_DIR}/fc_local.conf")

add_custom_command(
    OUTPUT "${POSTPROCESS_FC_LOCAL_CONF}"
    COMMAND sh -c "printf '%s\\n%s\\n%s\\n' \
            '<?xml version=\"1.0\"?>' \
            '<!DOCTYPE fontconfig SYSTEM \"/etc/fonts/conf.d/fonts.dtd\">' \
            '<fontconfig>' > '${POSTPROCESS_FC_LOCAL_CONF}' && \
            cat '${CMAKE_SOURCE_DIR}/extras/source/truetype/symbol/fc_local.snippet' \
                '${CMAKE_SOURCE_DIR}/postprocess/fontconfig/fc_local.snippet' \
                >> '${POSTPROCESS_FC_LOCAL_CONF}' && \
            printf '</fontconfig>\\n' >> '${POSTPROCESS_FC_LOCAL_CONF}'"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/extras/source/truetype/symbol/fc_local.snippet"
        "${CMAKE_SOURCE_DIR}/postprocess/fontconfig/fc_local.snippet"
    COMMENT "postprocess: assembling fc_local.conf (CustomTarget_fontconfig.mk)"
    VERBATIM)
add_custom_target(postprocess_fontconfig ALL DEPENDS "${POSTPROCESS_FC_LOCAL_CONF}")

install(FILES "${POSTPROCESS_FC_LOCAL_CONF}" DESTINATION share/fonts/truetype)

# --- CustomTarget_components: confirmed inactive ----------------------------
#
# Gated in Module_postprocess.mk by `$(if $(ENABLE_CUSTOMTARGET_COMPONENTS),
# CustomTarget_components)`. As already established for static/Module_
# static.mk (see static/CMakeLists.txt): ENABLE_CUSTOMTARGET_COMPONENTS is
# grepped nowhere in this tree, so it is unset, matching gbuild's own
# opt-in-only default. CustomTarget_components.mk's own body additionally
# only does anything `ifeq (,$(gb_PARTIAL_BUILD))` and produces a
# services_constructors.list for statically-linked-components builds (the
# same single-binary/WASM path static/CMakeLists.txt documents as not
# applicable here). Nothing to port.

# --- CustomTarget_images + Package_images: implemented ----------------------
#
# postprocess/CustomTarget_images.mk (133 lines, read in full) drives
# solenv/bin/pack_images.py to assemble images_<theme>.zip per configured
# icon theme (WITH_THEMES). Real, working upstream tools are reused directly
# (pack_images.py itself, plus solenv/bin/uiimagelist.xsl via xsltproc) --
# same "orchestrate the real generator from CMake" principle already applied
# to every other Python/XSLT-driven CustomTarget in this port (oox_generated,
# sw_writerfilter_generated, officecfg's main.xcd, etc.), not a
# reimplementation.
#
# Two real findings from reading the .mk and solenv/bin/pack_images.py in
# full, both affecting the design below:
#
#   1. `$(packimages_DIR)/images.zip` (no theme suffix) sets
#      `DEFAULT_THEME := $(true)`, but "images.zip" is never actually listed
#      in this CustomTarget's own gb_CustomTarget_register_targets call
#      (only `images_$(theme).zip` per WITH_THEMES entry is), and
#      Package_images.mk only ever installs the per-theme names too -- this
#      looks like another never-triggered upstream artifact (the same shape
#      as sw_writerfilter's own OOXMLFactory_values.hxx finding). Practical
#      consequence: DEFAULT_THEME is always false for every real zip that
#      actually gets built, so every real invocation uses the SAME directory
#      for -g/-m/-c (icon-themes/<theme> itself) -- no separate global/
#      module/custom distinction ever applies in practice, which is exactly
#      what is reproduced below (one directory, three flags).
#   2. `-s/--sort-file` is `required=True` in pack_images.py's own argparse,
#      but `optimize_zip_layout()` (read in full) falls back to
#      `sorted(zip_list.items())` -- plain alphabetical order -- for every
#      entry an (empty, or non-matching) sort file does not explicitly
#      place. The REAL sort file (`sorted.lst`, generated by solenv/bin/
#      image-sort.py) needs `gb_Postprocess_get_target,AllUIConfigs` --
#      i.e. the ENTIRE installed UI-config tree across all ~39 upstream-
#      registered UIConfig modules -- to regex-scan for icon load order.
#      This port has only ported UIConfig for ~9 modules so far; fully
#      reproducing image-sort.py would need installing all 39 first, a
#      separate, much larger task. sorted.lst is therefore written as an
#      EMPTY file below -- a real, honest, narrow fidelity gap (the
#      resulting zip is byte-for-byte CORRECT in *content*, just ordered
#      alphabetically instead of load-time-optimized) rather than a
#      fabrication or a blocker.
#
# The per-.ui-file image-list extraction (gb_Helper_get_imagelists's own
# per-UIConfig-module aggregation, driven by solenv/bin/uiimagelist.xsl) is
# reproduced as ONE tree-wide pass below (glob every real .ui file under any
# module's uiconfig/*/ui/ directory, 606 files in this checked-out tree, and
# run xsltproc once per file) rather than touching each of the ~9 already-
# ported *_uiconfig targets individually -- uiimagelist.xsl (read in full)
# only ever matches `<property name="icon_name"|"icon-name"|"pixbuf">`
# inside GtkBuilder .ui files; toolbar/menubar/statusbar/popupmenu XML files
# play no part in determining zip CONTENTS (they only feed image-sort.py's
# already-simplified-away ordering pass), so they are correctly not scanned
# here at all.
find_program(XSLTPROC_EXECUTABLE xsltproc REQUIRED)
find_program(PYTHON3_EXECUTABLE NAMES python3 REQUIRED)

set(WITH_THEMES
    breeze breeze_dark breeze_dark_svg breeze_svg
    colibre colibre_dark colibre_dark_svg colibre_svg
    elementary elementary_svg
    karasa_jaga karasa_jaga_svg
    sifr sifr_dark sifr_dark_svg sifr_svg
    sukapura sukapura_dark sukapura_dark_svg sukapura_svg
    CACHE STRING "Icon themes to pack into images_<theme>.zip (postprocess_images). Matches configure.ac's own --with-theme default (every theme) when unspecified.")

set(POSTPROCESS_IMAGES_DIR "${CMAKE_CURRENT_BINARY_DIR}/images")
file(MAKE_DIRECTORY "${POSTPROCESS_IMAGES_DIR}")

# --- sourceimagelist.ilst: grep/cut/sed pipeline over 22 fixed bitmaps.hlst
# files, transcribed literally from CustomTarget_images.mk's own recipe.
set(POSTPROCESS_BITMAPS_HLST
    "${CMAKE_SOURCE_DIR}/avmedia/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/basctl/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/connectivity/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/cui/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/chart2/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/dbaccess/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/desktop/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/extensions/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/formula/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/fpicker/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/framework/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/include/vcl/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/reportdesign/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/sc/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/sd/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/sdext/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/sfx2/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/svtools/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/svx/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/sw/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/vcl/inc/bitmaps.hlst"
    "${CMAKE_SOURCE_DIR}/xmlsecurity/inc/bitmaps.hlst")
string(REPLACE ";" " " POSTPROCESS_BITMAPS_HLST_SH "${POSTPROCESS_BITMAPS_HLST}")

set(POSTPROCESS_SOURCEIMAGELIST "${POSTPROCESS_IMAGES_DIR}/sourceimagelist.ilst")
add_custom_command(
    OUTPUT "${POSTPROCESS_SOURCEIMAGELIST}"
    COMMAND sh -c "grep res ${POSTPROCESS_BITMAPS_HLST_SH} | cut -d'\"' -f2 | sed 's/^/%MODULE%\\//' | sed 's/%MODULE%.res/%GLOBALRES%/g' > '${POSTPROCESS_SOURCEIMAGELIST}.png' && sed 's/\\.png/\\.svg/g' '${POSTPROCESS_SOURCEIMAGELIST}.png' > '${POSTPROCESS_SOURCEIMAGELIST}.svg' && cat '${POSTPROCESS_SOURCEIMAGELIST}.png' '${POSTPROCESS_SOURCEIMAGELIST}.svg' > '${POSTPROCESS_SOURCEIMAGELIST}'"
    DEPENDS ${POSTPROCESS_BITMAPS_HLST}
    COMMENT "pack_images: sourceimagelist.ilst"
    VERBATIM)

# --- commandimagelist.ilst: recursive glob over every icon-themes/<theme>/
# cmd/ file, path-normalized to %MODULE%/<relative-path>, de-duplicated.
# Computed at CMake CONFIGURE time (file(GLOB_RECURSE) + file(WRITE)) --
# this mirrors, not deviates from, upstream's own two-phase nature: gbuild
# itself computes packimages_cmd_images via $(shell $(FIND) ...) at
# MAKEFILE-PARSE time (the Make equivalent of CMake's configure step), and
# only the final write happens in the recipe body.
set(_pp_cmd_entries "")
foreach (_pp_theme IN LISTS WITH_THEMES)
    set(_pp_theme_dir "${CMAKE_SOURCE_DIR}/icon-themes/${_pp_theme}")
    if (IS_DIRECTORY "${_pp_theme_dir}/cmd")
        file(GLOB_RECURSE _pp_theme_cmd_files
            RELATIVE "${_pp_theme_dir}"
            "${_pp_theme_dir}/cmd/*.png" "${_pp_theme_dir}/cmd/*.svg")
        foreach (_pp_f IN LISTS _pp_theme_cmd_files)
            list(APPEND _pp_cmd_entries "%MODULE%/${_pp_f}")
        endforeach ()
    endif ()
endforeach ()
list(REMOVE_DUPLICATES _pp_cmd_entries)
list(SORT _pp_cmd_entries)
list(JOIN _pp_cmd_entries "\n" _pp_cmd_content)
set(POSTPROCESS_COMMANDIMAGELIST "${POSTPROCESS_IMAGES_DIR}/commandimagelist.ilst")
file(WRITE "${POSTPROCESS_COMMANDIMAGELIST}" "${_pp_cmd_content}\n")
unset(_pp_cmd_entries)
unset(_pp_cmd_content)

# --- wizards/source/imagelists/imagelists.ilst: static, checked in as-is.
set(POSTPROCESS_WIZARDS_ILST "${CMAKE_SOURCE_DIR}/wizards/source/imagelists/imagelists.ilst")

# --- per-.ui-file image lists (the one genuinely new subsystem) -------------
# solenv/bin/uiimagelist.xsl via xsltproc, one pass per real .ui file
# (gb_UIImageListTarget__command's own exact recipe), concatenated into a
# single combined .ilst -- pack_images.py's own -l manifest just unions every
# listed .ilst's entries regardless of how many files they are split across,
# so one combined file has the identical net effect to gbuild's own N
# separate per-UIConfig .ilst entries, for far less manifest bookkeeping.
file(GLOB_RECURSE POSTPROCESS_UI_FILES "${CMAKE_SOURCE_DIR}/*/uiconfig/*/ui/*.ui")
list(SORT POSTPROCESS_UI_FILES)

set(POSTPROCESS_UI_ILST_DIR "${POSTPROCESS_IMAGES_DIR}/uiimagelists")
set(_pp_ui_ilst_files "")
foreach (_pp_ui_file IN LISTS POSTPROCESS_UI_FILES)
    file(RELATIVE_PATH _pp_ui_rel "${CMAKE_SOURCE_DIR}" "${_pp_ui_file}")
    string(REPLACE "/" "_" _pp_ui_flat "${_pp_ui_rel}")
    set(_pp_ui_ilst "${POSTPROCESS_UI_ILST_DIR}/${_pp_ui_flat}.ilst")
    add_custom_command(
        OUTPUT "${_pp_ui_ilst}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${POSTPROCESS_UI_ILST_DIR}"
        COMMAND "${XSLTPROC_EXECUTABLE}" --nonet -o "${_pp_ui_ilst}"
                "${CMAKE_SOURCE_DIR}/solenv/bin/uiimagelist.xsl" "${_pp_ui_file}"
        # UIConfig.mk's own gb_UIImageListTarget__command, verbatim: "for some
        # reason xsltproc does not produce any file if there is no output, so
        # we touch the target to make sure it exists" -- real, reproduced
        # here via a `${CMAKE_COMMAND} -E touch` step (confirmed empirically
        # on the remote test build: xsltproc exits 0 but leaves no file on
        # disk for any .ui with zero icon_name/icon-name/pixbuf properties,
        # e.g. basctl/uiconfig/basicide/ui/basicmacrodialog.ui).
        COMMAND ${CMAKE_COMMAND} -E touch "${_pp_ui_ilst}"
        DEPENDS "${_pp_ui_file}" "${CMAKE_SOURCE_DIR}/solenv/bin/uiimagelist.xsl"
        COMMENT "uiimagelist.xsl: ${_pp_ui_rel}"
        VERBATIM)
    list(APPEND _pp_ui_ilst_files "${_pp_ui_ilst}")
endforeach ()
unset(_pp_ui_file)
unset(_pp_ui_rel)
unset(_pp_ui_flat)
unset(_pp_ui_ilst)

set(POSTPROCESS_UIIMAGELIST_ALL "${POSTPROCESS_IMAGES_DIR}/uiimagelist_all.ilst")
add_custom_command(
    OUTPUT "${POSTPROCESS_UIIMAGELIST_ALL}"
    COMMAND ${CMAKE_COMMAND} -E cat ${_pp_ui_ilst_files} > "${POSTPROCESS_UIIMAGELIST_ALL}"
    DEPENDS ${_pp_ui_ilst_files}
    COMMENT "pack_images: combined per-.ui image list"
    VERBATIM)

# --- the -l manifest (pack_images.py's own "file listing other .ilst files")
set(POSTPROCESS_IMAGELIST_MANIFEST "${POSTPROCESS_IMAGES_DIR}/imagelist_manifest.txt")
file(WRITE "${POSTPROCESS_IMAGELIST_MANIFEST}"
    "${POSTPROCESS_SOURCEIMAGELIST}\n${POSTPROCESS_COMMANDIMAGELIST}\n${POSTPROCESS_WIZARDS_ILST}\n${POSTPROCESS_UIIMAGELIST_ALL}\n")

# --- sorted.lst: empty placeholder -- see the header comment's finding #2.
set(POSTPROCESS_SORTED_LST "${POSTPROCESS_IMAGES_DIR}/sorted.lst")
file(WRITE "${POSTPROCESS_SORTED_LST}" "")

# --- per-theme <theme>_links.txt: literal transcription of the real
# recipe's three sequential (non-exclusive -- the second and third checks
# can each overwrite what came before) shell branches.
foreach (_pp_theme IN LISTS WITH_THEMES)
    set(_pp_links_out "${POSTPROCESS_IMAGES_DIR}/${_pp_theme}_links.txt")
    string(REPLACE "_svg" "" _pp_theme_no_svg "${_pp_theme}")
    string(REPLACE "_dark" "" _pp_theme_no_dark "${_pp_theme}")
    add_custom_command(
        OUTPUT "${_pp_links_out}"
        COMMAND sh -c "\
if test -f '${CMAKE_SOURCE_DIR}/icon-themes/${_pp_theme}/links.txt'; then \
  cp '${CMAKE_SOURCE_DIR}/icon-themes/${_pp_theme}/links.txt' '${_pp_links_out}.tmp'; \
else \
  if test -f '${CMAKE_SOURCE_DIR}/icon-themes/${_pp_theme_no_svg}/links.txt'; then \
    sed 's/\\.png/\\.svg/g' '${CMAKE_SOURCE_DIR}/icon-themes/${_pp_theme_no_svg}/links.txt' > '${_pp_links_out}.tmp'; \
  fi; \
  if test -f '${CMAKE_SOURCE_DIR}/icon-themes/${_pp_theme_no_dark}/links.txt'; then \
    cp '${CMAKE_SOURCE_DIR}/icon-themes/${_pp_theme_no_dark}/links.txt' '${_pp_links_out}.tmp'; \
  fi; \
fi; \
mv '${_pp_links_out}.tmp' '${_pp_links_out}'"
        COMMENT "postprocess: ${_pp_theme}_links.txt"
        VERBATIM)
endforeach ()
unset(_pp_theme_no_svg)
unset(_pp_theme_no_dark)
unset(_pp_links_out)

# --- images_<theme>.zip: the real pack_images.py invocation, once per theme.
# DEFAULT_THEME is always false here (see header comment finding #1), so
# -g/-m/-c all point at the same icon-themes/<theme> directory, matching
# every real (non-dead-code) invocation upstream ever makes.
set(_pp_all_zips "")
foreach (_pp_theme IN LISTS WITH_THEMES)
    set(_pp_theme_dir "${CMAKE_SOURCE_DIR}/icon-themes/${_pp_theme}")
    set(_pp_links_file "${POSTPROCESS_IMAGES_DIR}/${_pp_theme}_links.txt")
    set(_pp_zip_out "${LIBRESTUDIO_SHARE_DIR}/config/images_${_pp_theme}.zip")
    add_custom_command(
        OUTPUT "${_pp_zip_out}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${LIBRESTUDIO_SHARE_DIR}/config"
        COMMAND "${PYTHON3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/solenv/bin/pack_images.py"
                -g "${_pp_theme_dir}" -m "${_pp_theme_dir}" -c "${_pp_theme_dir}"
                -l "${POSTPROCESS_IMAGELIST_MANIFEST}"
                -L "${_pp_links_file}"
                -s "${POSTPROCESS_SORTED_LST}"
                -o "${_pp_zip_out}"
        DEPENDS
            "${POSTPROCESS_SOURCEIMAGELIST}" "${POSTPROCESS_COMMANDIMAGELIST}"
            "${POSTPROCESS_UIIMAGELIST_ALL}" "${POSTPROCESS_WIZARDS_ILST}"
            "${_pp_links_file}" "${POSTPROCESS_SORTED_LST}"
        COMMENT "pack_images.py: images_${_pp_theme}.zip"
        VERBATIM)
    list(APPEND _pp_all_zips "${_pp_zip_out}")
endforeach ()
unset(_pp_theme)
unset(_pp_theme_dir)
unset(_pp_links_file)
unset(_pp_zip_out)

add_custom_target(postprocess_images DEPENDS ${_pp_all_zips})
unset(_pp_all_zips)

# --- CppunitTest_singletons / CppunitTest_services: ported for real --------
#
# Both are gb_Module_add_check_targets test infrastructure (the latter
# additionally gated `ifneq ($(MPL_SUBSET),TRUE)` -- MPL_SUBSET is grepped
# nowhere in this tree, so that gate is inactive, matching gbuild's own
# default). Unlike most other CppunitTests in this port, neither needs a
# CppunitTest_deferred.cmake file: postprocess is the LAST module
# add_subdirectory()'d in the root CMakeLists.txt (line ~1459), well after
# both officecfg (~719), unotest/test (~1017-1018) and program_services_rdb
# (~1214, right after librestudio_finalize_components(services)) -- every
# ordering constraint either test could have is already satisfied by the
# time this file itself runs, so both are registered directly here.
#
# postprocess/qa/singletons.cxx and postprocess/qa/services.cxx were read in
# full: both are test::BootstrapFixture-derived CppunitTest suites that only
# walk the live UNO ComponentContext/type-description-manager/service-
# manager (singletons.cxx: every interface-based singleton is resolvable via
# the theTypeDescriptionManager; services.cxx: every UNO service
# implementation reachable via the service manager can actually be
# instantiated and reports a consistent XServiceInfo) -- neither #includes
# anything pyuno/oovbaapi-specific directly, but both .mk files still declare
# gb_CppunitTest_use_api(<name>,oovbaapi), transcribed faithfully as declared
# (same "faithful to what the .mk declares, not second-guessed from one
# file's own #include list" posture as officecfg_accelerator_key_checker_
# test's own identical use_api(oovbaapi) case, officecfg/CppunitTest_
# deferred.cmake) via oovbaapi_normal_HEADERS_INCLUDE_DIR /
# oovbaapi_normal_headers (oovbaapi/CMakeLists.txt's own
# librestudio_uno_api_headers(oovbaapi_normal ...) call, add_subdirectory()'d
# at root CMakeLists.txt line ~718, long before this file runs).
#
# Both .mk files: use_sdk_api (normal udkapi+offapi headers), use_libraries
# comphelper cppu cppuhelper sal test unotest vcl ("test" -> test_fixtures,
# this port's established rename), use_ure + use_vcl -> 3 protector pairs
# (same precedent as sax_parser/hwpfilter_test_hwpfilter/svx_unit),
# use_rdb(services[,services]) -> DEPENDS program_services_rdb, no
# COMPONENTS (see scaddins/CppunitTest_deferred.cmake's own header comment
# for the full ini-fallback reasoning), use_configuration -> CONFIGURATION.
# CppunitTest_services.mk additionally declares use_externals(boost_headers)
# (services.cxx uses std::vector/std::map/std::set directly, boost_headers
# is still transcribed faithfully as declared).
#
# CppunitTest_services.mk's remaining pieces, NOT reproduced here, each a
# genuine, verified gap rather than an oversight:
#   - `ifneq ($(DISABLE_PYTHON),TRUE)` block (use_python_ure,
#     use_rdb(services,pyuno), use_packages(pyuno_pythonloader_ini), and an
#     order-dep on Pyuno_get_target(commonwizards)): this port defines no
#     DISABLE_PYTHON switch (pyuno/CMakeLists.txt's own header comment says
#     so explicitly), so upstream's guard would always be active here --
#     but none of pyuno/CMakeLists.txt's own "not yet ported" list
#     (CustomTarget_pyuno_pythonloader_ini/Package_pyuno_pythonloader_ini,
#     Rdb_pyuno) exists as a real target in this tree, and there is no
#     "commonwizards" Pyuno_* target anywhere either -- none of these are
#     compile/link inputs to services.cxx (only extra runtime UNO_SERVICES
#     entries / staged resources / an order-dep), so the test still builds
#     without them; if it runs, it will simply see fewer python-backed UNO
#     services than upstream would, a pre-existing, documented pyuno gap
#     surfacing here rather than a new one.
#   - `ifeq ($(ENABLE_JAVA),TRUE)` use_jars(ScriptProviderForJava,smoketest):
#     LIBRESTUDIO_ENABLE_JAVA defaults OFF in this port (root CMakeLists.txt),
#     matching gbuild's own bare-configure default -- inactive, same posture
#     as reportdesign_basic_test's own ENABLE_JAVA gate (reportdesign/
#     CMakeLists.txt).
#   - use_packages(autotextshare_en-US, instsetoo_native_setup), unconditional
#     at the end of the .mk: both are pure runtime-staging order-deps (a
#     packaged AutoText tree, a packaged instdir setup fragment), not
#     compile/link inputs. Neither exists as a target here: extras/
#     CMakeLists.txt's own AllLangPackage_autotextshare note already
#     documents the l10n AutoText package as skipped, and instsetoo_native's
#     own setup packages are documented as not built (unotest/CMakeLists.txt).
#     Dropped, matching those modules' own already-accepted posture, not a
#     new gap introduced here.
librestudio_add_cppunit_test(singletons
    SOURCES
        postprocess/qa/singletons
    LIBRARIES
        comphelper cppu cppuhelper sal test_fixtures unotest vcl
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(services
    SOURCES
        postprocess/qa/services
    LIBRARIES
        comphelper cppu cppuhelper sal test_fixtures unotest vcl
        Boost::boost
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

