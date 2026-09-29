# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sysui -- Module_sysui.mk, read in full:
#
#   ifneq ($(OS),WNT)
#   ifneq ($(OS),MACOSX)
#     CustomTarget_share, Package_share,
#     $(if $(filter rpm,$(PKGFORMAT)),CustomTarget_rpm),
#     $(if $(filter deb,$(PKGFORMAT)),CustomTarget_deb),
#     $(if $(filter SOLARIS,$(OS)),CustomTarget_solaris)
#   else # OS=MACOSX
#     Package_osxicons, CustomTarget_infoplist, Package_infoplist
#   endif
#
# On this Linux port the `else # OS=MACOSX` branch (Package_osxicons/
# CustomTarget_infoplist/Package_infoplist -- macOS .app bundle Info.plist
# generation and icon packaging) never applies; CustomTarget_solaris likewise
# never applies (OS=SOLARIS false); CustomTarget_rpm/CustomTarget_deb are
# gated on PKGFORMAT, which this configure-less CMake port never sets
# (grepped: PKGFORMAT appears nowhere in cmake/*.cmake or the root
# CMakeLists.txt -- this build produces an instdir tree, not .rpm/.deb
# packages, matching this port's general stance so far). That leaves
# CustomTarget_share + Package_share: the eight .desktop launcher entries
# (writer/calc/draw/impress/math/base/startcenter/xsltfilter) this build
# would ship at share/xdg/*.desktop.
#
# This is a documented gap, not a silent skip. CustomTarget_share.mk's real
# payload is not the checked-in sysui/desktop/menus/*.desktop templates
# themselves (those are only *inputs*): the actual installed files are
# produced by a three-stage pipeline this port has no infrastructure for yet:
#
# 1. sysui/desktop/share/brand.pl rewrites each menus/<app>.desktop template,
#    substituting %PRODUCTNAME, %%FILE%%, and an --iconprefix built from
#    $(UNIXBASISROOTNAME) (sysui/productlist.mk: PRODUCTNAME.libreoffice,
#    UNIXFILENAME.libreoffice, PKGVERSION/PKGVERSIONSHORT derived from
#    LIBO_VERSION_MAJOR/MINOR/MICRO). None of PRODUCTNAME, PKGVERSION,
#    PKGVERSIONSHORT, or UNIXFILENAME.<product> exist anywhere in this CMake
#    port yet (grepped: zero hits), and the CustomTarget_share.mk invocation
#    of brand.pl itself passes `--iconprefix '$${UNIXBASISROOTNAME}-'` with
#    make's own `$$` collapsed to a single, *shell-single-quoted* `$`
#    (preserved literally into the .desktop file's Icon= line for a later,
#    still-unidentified substitution stage -- not resolved by brand.pl
#    itself), which this investigation could not pin down with the same
#    confidence as the rest of this file's other citations within the time
#    available for this pass.
# 2. solenv/bin/desktop-translate.py adds one "Name[<lang>]=" entry per
#    configured UI language, reading $(gb_TRANS_LANGS)/$(gb_POLOCATION)'s .po
#    files. This port has no localized-build (gb_WITH_LANG) concept wired at
#    all (see extras/CMakeLists.txt's AllLangPackage_autotextshare note for
#    the same gap elsewhere), so even if brand.pl's output were reproduced,
#    this second stage would need its own new infrastructure.
# 3. sysui/desktop/share/create_mime_xml.pl (openoffice.org.xml, the MIME
#    type registration) and the RPM/DEB spec/control templates are packaging-
#    time artifacts consumed by rpm/create_tree.sh, itself gated on PKGFORMAT
#    as noted above -- moot without it.
#
# Reproducing stage 1 alone would require this module to invent PRODUCTNAME/
# PKGVERSION/UNIXFILENAME values not established anywhere else in this port
# (a branding decision, not a build-system transcription one), and stage 1's
# own icon-prefix substitution semantics were not conclusively pinned down
# (see the note above) -- both good reasons not to guess at a working
# implementation here. Left as an honest, fully-cited gap: OS desktop
# integration (menu entries under share/xdg/) does not build in this port,
# but nothing about running the office suite itself depends on it.
#
# --- Static content that exists but is not install()able from here ---------
#
# sysui/desktop/icons/hicolor/**/*.{png,svg} are real, checked-in icon files,
# but they are not part of any gb_Package this module declares: gbuild's own
# create_tree.sh (sysui/desktop/share/create_tree.sh) copies them straight
# from $ICON_SOURCE_DIR into the *system* icon theme directory
# ($DESTDIR/$KDEMAINDIR/share/icons/...) as an RPM/DEB post-install step, not
# into gbuild's own instdir tree via any gb_Package_add_files call -- so
# there is no Package_*.mk destination for them to mirror here, and
# packaging is out of scope per PKGFORMAT above regardless.

