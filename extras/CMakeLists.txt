# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# extras -- Module_extras.mk, read in full: almost entirely Package_* targets
# installing real DATA (templates, gallery content, autocorrect/autotext,
# palettes, wordbook, ...), plus a handful of CustomTarget_* that actually
# *build* some of that data (zip/xsltproc assembly of ODF containers, or a
# font) before it is packaged.
#
# Every source path referenced below was verified present on disk (ls/find)
# before being transcribed; every destination prefix (LIBO_SHARE_FOLDER=
# share, LIBO_SHARE_PRESETS_FOLDER=presets -- both top-level, not share/-
# nested; LIBO_SHARE_RESOURCE_FOLDER=program/resource) comes from
# configure.ac's non-Darwin/non-WNT branch, cross-checked against this port's
# own DESTINATION conventions already used elsewhere (e.g. udkapi/
# CMakeLists.txt's `DESTINATION program`, io/CMakeLists.txt's
# `DESTINATION program`).

find_program(XSLTPROC_EXECUTABLE xsltproc REQUIRED)
find_program(ZIP_EXECUTABLE zip REQUIRED)

# extras_pack_zip(<out_file> <src_dir> [MIMETYPE_EXISTS])
#
# Assembles one ODF-style zip container the same way upstream's run_
# zip_template_recipe / autocorr's acor_%.dat rule / autotextuser's %.bau
# rule all do: "mimetype" stored uncompressed first (so a reader can find the
# media type at a fixed offset without inflating), then every other member
# deflated on top -- `zip -q0X --filesync --must-match ... mimetype` followed
# by `zip -qrX --must-match ...` for the rest.
#
# When MIMETYPE_EXISTS is not passed, <src_dir> has no "mimetype" file of its
# own (true for extras/source/autocorr/lang/<lang>/ and extras/source/
# autotext/mytexts/ -- upstream's own CustomTarget rules `touch` it into
# existence in the customtarget workdir), so this copies <src_dir> into a
# private staging directory under CMAKE_CURRENT_BINARY_DIR and creates an
# empty one there instead of writing into the checked-in source tree.
#
# Deliberately NOT reproduced: upstream additionally runs every .xml member
# through xsltproc + extras/util/compact.xsl before zipping (whitespace-only
# minification -- same document, smaller file). Skipping it packages the raw,
# uncompacted XML verbatim instead; semantically identical, just not
# byte-identical to what gbuild would produce.
function(extras_pack_zip out_file src_dir)
    cmake_parse_arguments(_pz "MIMETYPE_EXISTS" "" "" ${ARGN})

    if (_pz_MIMETYPE_EXISTS)
        set(_zip_srcdir "${src_dir}")
        file(GLOB_RECURSE _members RELATIVE "${src_dir}" CONFIGURE_DEPENDS "${src_dir}/*")
        file(GLOB_RECURSE _members_abs CONFIGURE_DEPENDS "${src_dir}/*")
        list(REMOVE_ITEM _members "mimetype")
    else ()
        get_filename_component(_stem "${out_file}" NAME_WE)
        set(_zip_srcdir "${CMAKE_CURRENT_BINARY_DIR}/_zipstage/${_stem}")
        file(GLOB_RECURSE _members RELATIVE "${src_dir}" CONFIGURE_DEPENDS "${src_dir}/*")
        file(GLOB_RECURSE _members_abs CONFIGURE_DEPENDS "${src_dir}/*")
    endif ()

    set(_members_sh "")
    foreach (_m IN LISTS _members)
        string(APPEND _members_sh " '${_m}'")
    endforeach ()

    get_filename_component(_out_dir "${out_file}" DIRECTORY)

    if (_pz_MIMETYPE_EXISTS)
        add_custom_command(
            OUTPUT "${out_file}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_out_dir}"
            COMMAND sh -c "cd '${_zip_srcdir}' && rm -f '${out_file}' && \
                '${ZIP_EXECUTABLE}' -q0X --filesync --must-match '${out_file}' mimetype && \
                '${ZIP_EXECUTABLE}' -qrX --must-match '${out_file}'${_members_sh}"
            DEPENDS ${_members_abs}
            COMMENT "extras: packing ${out_file}"
            VERBATIM)
    else ()
        add_custom_command(
            OUTPUT "${out_file}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_out_dir}"
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${_zip_srcdir}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_zip_srcdir}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${src_dir}" "${_zip_srcdir}"
            COMMAND ${CMAKE_COMMAND} -E touch "${_zip_srcdir}/mimetype"
            COMMAND sh -c "cd '${_zip_srcdir}' && rm -f '${out_file}' && \
                '${ZIP_EXECUTABLE}' -q0X --filesync --must-match '${out_file}' mimetype && \
                '${ZIP_EXECUTABLE}' -qrX --must-match '${out_file}'${_members_sh}"
            DEPENDS ${_members_abs}
            COMMENT "extras: packing ${out_file}"
            VERBATIM)
    endif ()
endfunction()

# =============================================================================
# Static data packages (SRCDIR-sourced, no build step) -- Package_cfgsrvnolang
# .mk, Package_cfgusr.mk, Package_database.mk, Package_databasebiblio.mk,
# Package_gallbullets.mk, Package_gallmytheme.mk, Package_gallroot.mk,
# Package_gallsystem.mk, Package_labels.mk, Package_palettes.mk,
# Package_tplwizbitmap.mk, Package_wordbook.mk, Personas.mk
# =============================================================================

# Package_cfgsrvnolang.mk (gb_Package_add_files_with_dir keeps the wizard/
# form/styles/ subdir)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/source/misc_config/wizard/form/styles/beige.css"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/misc_config/wizard/form/styles/bgr.css"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/misc_config/wizard/form/styles/dark.css"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/misc_config/wizard/form/styles/grey.css"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/misc_config/wizard/form/styles/ibg.css"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/misc_config/wizard/form/styles/ice.css"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/misc_config/wizard/form/styles/orange.css"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/misc_config/wizard/form/styles/red.css"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/misc_config/wizard/form/styles/violet.css"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/misc_config/wizard/form/styles/water.css"
    DESTINATION share/config/wizard/form/styles)

# Package_cfgusr.mk
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/source/misc_config/autotbl.fmt" DESTINATION presets/config)

# Package_database.mk
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/source/database/biblio.odb" "${CMAKE_CURRENT_SOURCE_DIR}/source/database/evolocal.odb" DESTINATION presets/database)

# Package_databasebiblio.mk
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/source/database/biblio.dbf" "${CMAKE_CURRENT_SOURCE_DIR}/source/database/biblio.dbt" DESTINATION presets/database/biblio)

# Package_gallbullets.mk
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/check.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/circle-blue.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/circle-green.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/circle-yellow.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/circle.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/cross-red.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/cross.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/flag-blue.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/flag-green.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/flag-yellow.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/flag.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/info.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/pin-blue.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/pin-green.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/pin-yellow.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/pin.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/redo.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/stars-empty.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/stars.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/triangle-blue.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/triangle-green.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/triangle-yellow.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/triangle.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/undo.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bullets/warning.svg"
    DESTINATION share/gallery/bullets)

# Package_gallmytheme.mk
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/gallery_mytheme/sg30.sdv" "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/gallery_mytheme/sg30.thm"
    DESTINATION presets/gallery)

# Package_gallroot.mk
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/apples.gif"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/bigapple.gif"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/flower.gif"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/flowers.gif"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/sky.gif"
    DESTINATION share/gallery)

# Package_gallsystem.mk -- system_galleries := arrows bpmn bullets diagrams
# flowchart fontwork icons network shapes symbolshapes, each with .sdg/.sdv/
# .thm; all thirty files verified present in source/gallery/gallery_system/.
set(EXTRAS_SYSTEM_GALLERIES arrows bpmn bullets diagrams flowchart fontwork icons network shapes symbolshapes)
set(EXTRAS_GALLSYSTEM_FILES "")
foreach (_gallery IN LISTS EXTRAS_SYSTEM_GALLERIES)
    list(APPEND EXTRAS_GALLSYSTEM_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/gallery_system/${_gallery}.sdg"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/gallery_system/${_gallery}.sdv"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/gallery_system/${_gallery}.thm")
endforeach ()
unset(_gallery)
install(FILES ${EXTRAS_GALLSYSTEM_FILES} DESTINATION share/gallery)

# Package_labels.mk
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/source/labels/labels.xml" DESTINATION share/labels)

# Package_palettes.mk
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/arrowhd.soe"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/chart-palettes.soc"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/classic.sog"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/freecolour-hlc.soc"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/hatching.soh"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/html.soc"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/libreoffice.soc"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/material.soc"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/modern.sog"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/compatibility.soc"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/standard.sob"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/standard.soc"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/standard.sod"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/standard.soe"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/standard.sog"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/standard.soh"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/standard.sop"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/styles.sod"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/tonal.soc"
    DESTINATION share/palette)

# librestudio_add_package_mirror(): the real, named "extras_palettes"
# build-time target gb_CppunitTest_use_packages(chart2_dialogs_test,
# extras_palettes) needs (see chart2/CppunitTest_deferred.cmake's own
# chart2_dialogs_test section) -- same generic helper (cmake/
# LibreStudioPackage.cmake) already used for wizards'/scripting's own
# PACKAGES targets, applied here to the install(FILES...) block just above
# (same 19 files, same destination).
librestudio_add_package_mirror(extras_palettes
    PAIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/arrowhd.soe:${LIBRESTUDIO_SHARE_DIR}/palette"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/chart-palettes.soc:${LIBRESTUDIO_SHARE_DIR}/palette"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/classic.sog:${LIBRESTUDIO_SHARE_DIR}/palette"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/freecolour-hlc.soc:${LIBRESTUDIO_SHARE_DIR}/palette"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/hatching.soh:${LIBRESTUDIO_SHARE_DIR}/palette"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/html.soc:${LIBRESTUDIO_SHARE_DIR}/palette"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/libreoffice.soc:${LIBRESTUDIO_SHARE_DIR}/palette"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/material.soc:${LIBRESTUDIO_SHARE_DIR}/palette"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/modern.sog:${LIBRESTUDIO_SHARE_DIR}/palette"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/compatibility.soc:${LIBRESTUDIO_SHARE_DIR}/palette"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/standard.sob:${LIBRESTUDIO_SHARE_DIR}/palette"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/standard.soc:${LIBRESTUDIO_SHARE_DIR}/palette"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/standard.sod:${LIBRESTUDIO_SHARE_DIR}/palette"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/standard.soe:${LIBRESTUDIO_SHARE_DIR}/palette"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/standard.sog:${LIBRESTUDIO_SHARE_DIR}/palette"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/standard.soh:${LIBRESTUDIO_SHARE_DIR}/palette"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/standard.sop:${LIBRESTUDIO_SHARE_DIR}/palette"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/styles.sod:${LIBRESTUDIO_SHARE_DIR}/palette"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/palettes/tonal.soc:${LIBRESTUDIO_SHARE_DIR}/palette")

# Package_tplwizbitmap.mk
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/source/templates/wizard/bitmap/Import_1.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/templates/wizard/bitmap/Import_3.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/templates/wizard/bitmap/Import_4.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/templates/wizard/bitmap/maximize.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/templates/wizard/bitmap/minimize.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/templates/wizard/bitmap/MS-Import_2-1.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/templates/wizard/bitmap/MS-Import_2-2.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/templates/wizard/bitmap/MS-Import_2-3.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/templates/wizard/bitmap/tutorial_background.gif"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/templates/wizard/bitmap/XML-Import_2-1.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/templates/wizard/bitmap/XML-Import_2-2.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/templates/wizard/bitmap/XML-Import_2-3.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/templates/wizard/bitmap/XML-Import_2-4.png"
    DESTINATION share/template/wizard/bitmap)

# Package_wordbook.mk
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/source/wordbook/en-GB.dic"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/wordbook/en-US.dic"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/wordbook/sl.dic"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/wordbook/technical.dic"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/wordbook/hu_AkH11.dic"
    DESTINATION share/wordbook)

# Package_autotext.mk -- gb_Package_add_empty_directory only: no files, just
# the directory itself. install(DIRECTORY) with no source tree reproduces an
# empty-directory install the same way.
install(DIRECTORY DESTINATION share/autotext/common)

# Personas.mk -- six per-color Packages (footer/header/preview.png, all
# verified present) plus one flat personas_list.txt; the .mk's own "TODO:
# generate this" comment confirms personas_list.txt is meant to be static too.
foreach (_persona dark gray green pink sand white)
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/personas/${_persona}/footer.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/personas/${_persona}/header.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/personas/${_persona}/preview.png"
        DESTINATION "share/gallery/personas/${_persona}")
endforeach ()
unset(_persona)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/source/gallery/personas/personas_list.txt" DESTINATION share/gallery/personas)

# =============================================================================
# CustomTarget-built data: implemented for real
# =============================================================================

# --- CustomTarget_glade + Package_glade -------------------------------------
#
# A single xsltproc transform: makewidgetgroup.xslt applied to libreoffice-
# catalog.xml.in produces libreoffice-catalog.xml (a GtkBuilder/Glade
# widget-catalog description for LibreOffice's own custom widgets). Both
# source files verified present.
set(EXTRAS_GLADE_CATALOG "${CMAKE_CURRENT_BINARY_DIR}/glade/libreoffice-catalog.xml")
add_custom_command(
    OUTPUT "${EXTRAS_GLADE_CATALOG}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/glade"
    COMMAND "${XSLTPROC_EXECUTABLE}" --nonet
            -o "${EXTRAS_GLADE_CATALOG}"
            "${CMAKE_CURRENT_SOURCE_DIR}/source/glade/makewidgetgroup.xslt"
            "${CMAKE_CURRENT_SOURCE_DIR}/source/glade/libreoffice-catalog.xml.in"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/glade/makewidgetgroup.xslt"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/glade/libreoffice-catalog.xml.in"
    COMMENT "extras: libreoffice-catalog.xml (CustomTarget_glade.mk)"
    VERBATIM)
add_custom_target(extras_glade_catalog ALL DEPENDS "${EXTRAS_GLADE_CATALOG}")
install(FILES "${EXTRAS_GLADE_CATALOG}" DESTINATION share/glade)

# --- CustomTarget_autotextuser + Package_autotextuser -----------------------
#
# mytexts.bau: mimetype (generated, touched empty) + mytexts/BlockList.xml +
# mytexts/META-INF/manifest.xml, both verified present, zipped together.
set(EXTRAS_AUTOTEXTUSER_BAU "${CMAKE_CURRENT_BINARY_DIR}/autotext_user/mytexts.bau")
extras_pack_zip("${EXTRAS_AUTOTEXTUSER_BAU}" "${CMAKE_CURRENT_SOURCE_DIR}/source/autotext/mytexts")
add_custom_target(extras_autotextuser ALL DEPENDS "${EXTRAS_AUTOTEXTUSER_BAU}")
install(FILES "${EXTRAS_AUTOTEXTUSER_BAU}" DESTINATION presets/autotext)

# --- CustomTarget_autocorr + Package_autocorr -------------------------------
#
# 49 per-language acor_<longlang>.dat containers, each zipping whichever of
# DocumentList.xml / META-INF/manifest.xml / SentenceExceptList.xml /
# WordExceptList.xml really exist for that language (not every language has
# all four -- extras_pack_zip's own file(GLOB_RECURSE) picks up exactly the
# real members per directory, the same selection extras_AUTOCORR_XMLFILES_LANG
# makes upstream) plus a generated empty mimetype. shortlang:longlang pairs
# transcribed verbatim from extras_AUTOCORR_LANGS in CustomTarget_autocorr.mk;
# every extras/source/autocorr/lang/<shortlang>/ directory verified present.
#
# Deliberately not reproduced: bin/check-autocorr.py, a DocumentList.xml
# sanity check run before zipping upstream -- validates input that is already
# static and unchanged from upstream, not something that alters the output.
set(EXTRAS_AUTOCORR_LANGS
    "af-ZA:af-ZA" "bg:bg-BG" "ca:ca-ES" "cs:cs-CZ" "da:da-DK" "de:de" "dsb:dsb"
    "el:el-GR" "en-AU:en-AU" "en-GB:en-GB" "en-US:en-US" "en-ZA:en-ZA" "es:es"
    "fa:fa-IR" "fi:fi-FI" "fr:fr" "ga-IE:ga-IE" "hr:hr-HR" "hsb:hsb" "hu:hu-HU"
    "is:is-IS" "it:it" "ja:ja-JP" "ko:ko-KR" "lb-LU:lb-LU" "lt:lt-LT" "mn:mn-MN"
    "nl:nl-NL" "nl-BE:nl-BE" "pl:pl-PL" "pt:pt-PT" "pt-BR:pt-BR" "ro:ro-RO"
    "ru:ru-RU" "sk:sk-SK" "sl:sl-SI" "sr-CS:sr-CS" "sr-Latn-CS:sr-Latn-CS"
    "sr-Latn-ME:sr-Latn-ME" "sr-Latn-RS:sr-Latn-RS" "sr-ME:sr-ME" "sr-RS:sr-RS"
    "sv:sv-SE" "th:th-TH" "tr:tr-TR" "vi:vi-VN" "vro:vro-EE" "zh-CN:zh-CN"
    "zh-TW:zh-TW")

set(EXTRAS_AUTOCORR_DAT_FILES "")
foreach (_pair IN LISTS EXTRAS_AUTOCORR_LANGS)
    string(REPLACE ":" ";" _pair_list "${_pair}")
    list(GET _pair_list 0 _shortlang)
    list(GET _pair_list 1 _longlang)
    set(_dat "${CMAKE_CURRENT_BINARY_DIR}/autocorr/acor_${_longlang}.dat")
    extras_pack_zip("${_dat}" "${CMAKE_CURRENT_SOURCE_DIR}/source/autocorr/lang/${_shortlang}")
    list(APPEND EXTRAS_AUTOCORR_DAT_FILES "${_dat}")
endforeach ()
unset(_pair)
unset(_pair_list)
unset(_shortlang)
unset(_longlang)
unset(_dat)

add_custom_target(extras_autocorr ALL DEPENDS ${EXTRAS_AUTOCORR_DAT_FILES})
install(FILES ${EXTRAS_AUTOCORR_DAT_FILES} DESTINATION share/autocorr)

# --- CustomTarget_templates + Package_templates/tplpresnt/tplwizard/
#     tplwizdesktop: ODF template containers ----------------------------------
#
# Every template's source directory already contains its own "mimetype"
# (verified: extras/source/templates/officorr/Modern_business_letter_sans_
# serif/mimetype and siblings exist for real), so these use extras_pack_zip's
# MIMETYPE_EXISTS path -- no staging copy needed, zip reads straight from the
# checked-in source directory. Each directory's own file(GLOB_RECURSE) in
# extras_pack_zip naturally reproduces template_files.mk's per-template
# extra_files_* lists (Thumbnails/, Pictures/, settings.xml, manifest.rdf,
# ...): every member template_files.mk names is a real file already sitting
# in that same source directory, so recursively globbing the directory yields
# exactly that set without hand-transcribing template_files.mk's 280-line
# variable list.
#
# WITH_TEMPLATES/WITH_GALLERY_BUILD default TRUE upstream (configure.ac,
# unconditionally, absent an explicit --without-templates/--without-galleries)
# -- this build has no configure step to turn them off, so tplpresnt/tplwizard/
# tplwizdesktop are unconditionally in scope, matching Package_templates.mk
# itself (never gated at all).

# templates: dest share/template/common, subdir preserved (gb_Package_add_
# files_with_dir)
set(EXTRAS_TEMPLATES_COMMON
    "officorr/Modern_business_letter_sans_serif.ott"
    "officorr/Modern_business_letter_serif.ott"
    "offimisc/Businesscard-with-logo.ott"
    "personal/CV.ott"
    "personal/Resume1page.ott"
    "styles/Default.ott"
    "styles/Modern.ott"
    "styles/Simple.ott"
    "draw/bpmn.otg"
    "l10n/zh_CN_ott_normal.ott"
    "l10n/ja_ott_normal.ott")

# tplpresnt: dest share/template/common/presnt, flat (gb_Package_add_files)
set(EXTRAS_TEMPLATES_PRESNT
    "presnt/Candy.otp" "presnt/DNA.otp" "presnt/Inspiration.otp" "presnt/Metropolis.otp"
    "presnt/Vintage.otp" "presnt/Focus.otp" "presnt/Grey_Elegant.otp" "presnt/Growing_Liberty.otp"
    "presnt/Freshes.otp" "presnt/Midnightblue.otp" "presnt/Vivid.otp" "presnt/Pencil.otp"
    "presnt/Blueprint_Plans.otp" "presnt/Portfolio.otp" "presnt/Progress.otp" "presnt/Beehive.otp"
    "presnt/Blue_Curve.otp" "presnt/Forestbird.otp" "presnt/Lights.otp" "presnt/Nature_Illustration.otp"
    "presnt/Piano.otp" "presnt/Sunset.otp" "presnt/Yellow_Idea.otp")

# tplwizard: dest share/template/common/wizard, subdir preserved
set(EXTRAS_TEMPLATES_WIZARD
    "wizard/agenda/10grey.ott" "wizard/agenda/1simple.ott" "wizard/agenda/2elegant.ott"
    "wizard/agenda/3modern.ott" "wizard/agenda/4classic.ott" "wizard/agenda/5blue.ott"
    "wizard/agenda/6orange.ott" "wizard/agenda/7red.ott" "wizard/agenda/8green.ott"
    "wizard/agenda/9colorful.ott" "wizard/agenda/aw-10grey.ott" "wizard/agenda/aw-1simple.ott"
    "wizard/agenda/aw-2elegant.ott" "wizard/agenda/aw-3modern.ott" "wizard/agenda/aw-4classic.ott"
    "wizard/agenda/aw-5blue.ott" "wizard/agenda/aw-6orange.ott" "wizard/agenda/aw-7red.ott"
    "wizard/agenda/aw-8green.ott" "wizard/agenda/aw-9colorful.ott"
    "wizard/fax/bus-classic_f.ott" "wizard/fax/bus-classic-pri_f.ott" "wizard/fax/bus-modern_f.ott"
    "wizard/fax/bus-modern-pri_f.ott" "wizard/fax/pri-bottle_f.ott" "wizard/fax/pri-fax_f.ott"
    "wizard/fax/pri-lines_f.ott" "wizard/fax/pri-marine_f.ott"
    "wizard/letter/bus-elegant_l.ott" "wizard/letter/bus-modern_l.ott" "wizard/letter/bus-office_l.ott"
    "wizard/letter/off-elegant_l.ott" "wizard/letter/off-modern_l.ott" "wizard/letter/off-office_l.ott"
    "wizard/letter/pri-bottle_l.ott" "wizard/letter/pri-mail_l.ott" "wizard/letter/pri-marine_l.ott"
    "wizard/letter/pri-redline_l.ott"
    "wizard/report/cnt-011.ott" "wizard/report/cnt-012.ott" "wizard/report/cnt-01.ott"
    "wizard/report/cnt-021.ott" "wizard/report/cnt-022.ott" "wizard/report/cnt-02.ott"
    "wizard/report/cnt-031.ott" "wizard/report/cnt-032.ott" "wizard/report/cnt-03.ott"
    "wizard/report/cnt-041.ott" "wizard/report/cnt-042.ott" "wizard/report/cnt-04.ott"
    "wizard/report/cnt-051.ott" "wizard/report/cnt-052.ott" "wizard/report/cnt-05.ott"
    "wizard/report/cnt-061.ott" "wizard/report/cnt-062.ott" "wizard/report/cnt-06.ott"
    "wizard/report/cnt-default.ott"
    "wizard/report/stl-01.ott" "wizard/report/stl-02.ott" "wizard/report/stl-03.ott"
    "wizard/report/stl-04.ott" "wizard/report/stl-05.ott" "wizard/report/stl-06.ott"
    "wizard/report/stl-07.ott" "wizard/report/stl-08.ott" "wizard/report/stl-09.ott"
    "wizard/report/stl-default.ott"
    "wizard/styles/blackberry.ots" "wizard/styles/black_white.ots" "wizard/styles/default.ots"
    "wizard/styles/diner.ots" "wizard/styles/fall.ots" "wizard/styles/glacier.ots"
    "wizard/styles/green_grapes.ots" "wizard/styles/jeans.ots" "wizard/styles/marine.ots"
    "wizard/styles/millennium.ots" "wizard/styles/nature.ots" "wizard/styles/neon.ots"
    "wizard/styles/night.ots" "wizard/styles/nostalgic.ots" "wizard/styles/pastell.ots"
    "wizard/styles/pool.ots" "wizard/styles/pumpkin.ots" "wizard/styles/xos.ots")

# tplwizdesktop: dest share/template/common/internal, subdir preserved --
# source subdirs are wizard/desktop/html and wizard/desktop/idxexample, but
# (per Package_tplwizdesktop.mk) the *installed* names are flat: html.oth,
# idxexample.odt.
set(EXTRAS_TEMPLATES_WIZDESKTOP
    "wizard/desktop/html.oth"
    "wizard/desktop/idxexample.odt")

# FLATTEN distinguishes gb_Package_add_files_with_dir (subdir preserved in
# the destination -- templates, tplwizard) from plain gb_Package_add_files
# (always flat, regardless of the source subdir the item's zip is built from
# -- tplpresnt's presnt/<Name>.otp and tplwizdesktop's wizard/desktop/<name>.
# <ext>, both of which need their source-only leading subdir stripped from
# the *destination*, not just the source lookup).
function(extras_install_odf_templates out_files_var dest_subdir)
    cmake_parse_arguments(_iot "FLATTEN" "" "ITEMS" ${ARGN})
    set(_out_files "")
    foreach (_rel IN LISTS _iot_ITEMS)
        string(REGEX REPLACE "\\.[^.]+$" "" _rel_noext "${_rel}")
        set(_src "${CMAKE_CURRENT_SOURCE_DIR}/source/templates/${_rel_noext}")
        get_filename_component(_out_reldir "${_rel}" DIRECTORY)
        set(_out "${CMAKE_CURRENT_BINARY_DIR}/templates/${_rel}")
        extras_pack_zip("${_out}" "${_src}" MIMETYPE_EXISTS)
        list(APPEND _out_files "${_out}")
        if (_iot_FLATTEN OR NOT _out_reldir)
            install(FILES "${_out}" DESTINATION "share/template/${dest_subdir}")
        else ()
            # _out_reldir carries a leading "wizard/" for EXTRAS_TEMPLATES_WIZARD
            # (needed above only to resolve _src under source/templates/wizard/...);
            # dest_subdir already supplies "common/wizard", so strip that one
            # source-only leading segment before it becomes part of the
            # destination (upstream's own Package_tplwizard.mk file list has no
            # "wizard/" prefix either -- its SOURCEDIR is already .../templates/
            # wizard). A no-op for EXTRAS_TEMPLATES_COMMON, whose entries never
            # start with "wizard/".
            string(REGEX REPLACE "^wizard/" "" _out_reldir "${_out_reldir}")
            install(FILES "${_out}" DESTINATION "share/template/${dest_subdir}/${_out_reldir}")
        endif ()
    endforeach ()
    set(${out_files_var} "${_out_files}" PARENT_SCOPE)
endfunction()

extras_install_odf_templates(EXTRAS_TEMPLATES_COMMON_OUT common ITEMS ${EXTRAS_TEMPLATES_COMMON})
extras_install_odf_templates(EXTRAS_TEMPLATES_PRESNT_OUT common/presnt FLATTEN ITEMS ${EXTRAS_TEMPLATES_PRESNT})
extras_install_odf_templates(EXTRAS_TEMPLATES_WIZARD_OUT common/wizard ITEMS ${EXTRAS_TEMPLATES_WIZARD})
extras_install_odf_templates(EXTRAS_TEMPLATES_WIZDESKTOP_OUT common/internal FLATTEN ITEMS ${EXTRAS_TEMPLATES_WIZDESKTOP})

add_custom_target(extras_templates ALL DEPENDS
    ${EXTRAS_TEMPLATES_COMMON_OUT}
    ${EXTRAS_TEMPLATES_PRESNT_OUT}
    ${EXTRAS_TEMPLATES_WIZARD_OUT}
    ${EXTRAS_TEMPLATES_WIZDESKTOP_OUT})

# =============================================================================
# Real gaps, honestly documented (not silently skipped)
# =============================================================================

# --- Package_fonts / Package_resource_fonts (CustomTarget_opensymbol) ------
#
# Both packages install opens___.ttf (the OpenSymbol bullet/dingbat font),
# built by CustomTarget_opensymbol.mk from extras/source/truetype/symbol/
# OpenSymbol.sfd via `$(FONTFORGE) -lang=ff -c 'Open($1); Generate($2)'` when
# FONTFORGE is found, or otherwise copied from a pre-built tarball named in
# download.lst (OPENSYMBOL_TTF := f543e6e2...-opens___.ttf, TARFILE_LOCATION-
# relative). Checked directly: `fontforge` is not installed on this machine
# (`which fontforge` fails), and this port's root CMakeLists.txt does not
# vendor/fetch OPENSYMBOL_TTF anywhere (grepped every librestudio_fetch_
# tarball() call in the root CMakeLists.txt -- harfbuzz/orcus/mdds/etc. are
# fetched that way, OpenSymbol is not). Neither upstream code path is
# available, so opens___.ttf cannot be produced here. Real gap: OpenSymbol
# (used for some bullet/list glyphs) is missing; not worked around with a
# substitute font, since that would silently change rendered output. Fixing
# this needs either an fontforge install or a librestudio_fetch_tarball(...)
# call for OPENSYMBOL_TTF -- either is a reasonable follow-up, neither
# attempted here.
message(STATUS
    "extras: Package_fonts/Package_resource_fonts (OpenSymbol, opens___.ttf) "
    "skipped -- fontforge is not installed and OPENSYMBOL_TTF is not vendored "
    "(see extras/CMakeLists.txt).")

# --- Package_gallsystemstr + CustomTarget_gallsystem ------------------------
#
# Localized display names for the seven non-icon-set system gallery themes
# (arrows/bpmn/diagrams/flowchart/icons/network/shapes -- bullets/fontwork/
# symbolshapes excluded per the .mk's own comment). CustomTarget_gallsystem.mk
# merges extras/source/gallery/share/gallery_names.ulf against $(gb_TRANS_
# LANGS)'s .po files via gb_CustomTarget_ulfex_rule, then runs solenv/bin/
# desktop-translate.py over the result to emit one <name>.str per gallery
# (a multi-language "Name[xx]=..." style file, seeded from extras/source/
# gallery/gallery_system/dummy.str). This needs the same ulf/po/gb_TRANS_LANGS
# translation-catalog machinery this port has nowhere else either (see
# sysui/CMakeLists.txt and wizards/CMakeLists.txt's AllLangMoTarget_wiz note
# for the same gap in other modules) -- not attempted here.
message(STATUS
    "extras: Package_gallsystemstr (localized gallery theme names) skipped -- "
    "needs the ulf/po/desktop-translate.py translation pipeline, which this "
    "port has not wired anywhere (see extras/CMakeLists.txt).")

# --- AllLangPackage_autotextshare + CustomTarget_autotextshare --------------
#
# gb_Module_add_l10n_targets(extras, CustomTarget_autotextshare AllLangPackage_
# autotextshare) -- over a hundred per-language standard.bau/template.bau/
# crdbus5x.bau AutoText packages, each built the same zip way as Package_
# autotextuser above but keyed off gb_WITH_LANG (this build has no --with-lang
# equivalent, so there is no configured language list to iterate). Unlike
# Package_autocorr (main gb_Module_add_targets, unconditional), this is
# specifically in the l10n-only target group; deferred like every other l10n-
# gated target in this pass.
message(STATUS
    "extras: AllLangPackage_autotextshare (per-language AutoText) skipped -- "
    "l10n-only target, no gb_WITH_LANG equivalent in this port.")

# --- Gallery_backgrounds + Gallery_sound (WITH_GALLERY_BUILD, default TRUE) -
#
# gb_Gallery_Gallery/gb_Gallery_add_files feed raw .jpg/.wav media through the
# gengal tool (svx/source/gengal, already ported for real -- see svx/
# CMakeLists.txt's own install(PROGRAMS gengal.sh)) to produce a gallery
# theme's .sdg/.sdv/.thm triplet. Unlike gengal's own *invocation* (a plain
# executable this port already builds), what actually drives it at gallery-
# build time is upstream's gengal.sh wrapper bootstrapping a throwaway
# --headless soffice instance against these raw media files -- a full office-
# runtime dependency at build time, not just a native tool. Wiring a build-
# time soffice invocation into this CMake graph is a meaningfully different,
# heavier undertaking than everything else in this file (all of which is
# xsltproc/zip/sed -- no dependency on the office runtime it is itself
# building) and was not attempted in this pass. Gallery_sound.mk is further
# gated on AVMEDIA (gb_Helper_optional(AVMEDIA,Gallery_sound)); avmedia is
# ported in this tree, so that gate alone would not stop it.
message(STATUS
    "extras: Gallery_backgrounds/Gallery_sound skipped -- gengal theme "
    "generation needs a build-time --headless soffice invocation, not just "
    "the gengal executable itself (see extras/CMakeLists.txt).")

# --- Package_newfiles: WNT-only, inert on this platform ---------------------
#
# Module_extras.mk: `$(if $(filter WNT,$(OS)),Package_newfiles)` -- never
# added on this Linux-only port. (It installs the Windows Explorer "New"
# shell-integration document templates -- share/template/shellnew/soffice.
# {odg,odp,ods,odt} -- meaningless without the Windows shell integration it
# serves.) Nothing to do here, matching cpputools/CMakeLists.txt's own
# `if (WIN32)`-gated sp2bv precedent for the same kind of inert branch.

