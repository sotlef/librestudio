# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# readlicense_oo -- Module_readlicense_oo.mk, read in full, lists:
#   CustomTarget_readme, CustomTarget_license, Package_files, Package_license,
#   Package_readlicense_oo_readmes
# All five are real, install-time license/readme text; unlike extras/sysui
# there is no OS-integration or translation-pipeline dependency blocking this
# one -- xsltproc (already used by officecfg/CMakeLists.txt and
# root CMakeLists.txt) is the only external tool CustomTarget_license.mk and
# CustomTarget_readme.mk need on the (default, gb_WITH_LANG unset) code path.
#
# This port has no configure.ac-equivalent PRODUCTNAME/BUILD_TYPE/WITH_THEMES
# variables yet (grepped: none exist anywhere in cmake/*.cmake or the root
# CMakeLists.txt), so this module defines the handful of xsltproc stringparams
# license_html.xsl/license_plain_text.xsl/readme.xsl need as local defaults,
# scoped to this file only -- not a claim that these belong centrally yet.
# PRODUCTNAME follows this fork's own name (used throughout this port's own
# file headers, "This file is part of the LibreStudio project"); the others
# are informational-only text substitutions with no functional effect on the
# built office suite itself.
find_program(XSLTPROC_EXECUTABLE xsltproc REQUIRED)

set(READLICENSE_OO_PRODUCTNAME "LibreStudio")
set(READLICENSE_OO_PRODUCTVERSION "${LIBO_VERSION_MAJOR}.${LIBO_VERSION_MINOR}")
# CustomTarget_readme.mk's readme_SED additionally substitutes ${LCPRODUCTNAME}
# (lower-cased PRODUCTNAME) into readme.xsl's own output via a trailing sed
# pass; reproduced the same way below.
string(TOLOWER "${READLICENSE_OO_PRODUCTNAME}" READLICENSE_OO_LCPRODUCTNAME)

set(READLICENSE_OO_WORKDIR "${CMAKE_CURRENT_BINARY_DIR}")

# --- CustomTarget_license: LICENSE.html + LICENSE ---------------------------
#
# CustomTarget_license.mk's two xsltproc invocations, --stringparam for
# --stringparam, against readlicense_oo/license/license.xml. build_type/os/
# themes are this build's stand-ins for BUILD_TYPE/OS/WITH_THEMES (see above);
# no_mpl_subset is passed because MPL_SUBSET is unset in this port (confirmed:
# grepped nowhere in configure.ac's default path or this port's CMake --
# postprocess/CMakeLists.txt's own CppunitTest_services note relies on the
# same fact), matching upstream's own `$(if $(MPL_SUBSET),,--stringparam
# no_mpl_subset no_mpl_subset)` when MPL_SUBSET is empty. The ifeq($(OS),WNT)
# license.txt/CRLF branch is skipped outright (Linux-only port).
set(READLICENSE_OO_LICENSE_XML "${CMAKE_CURRENT_SOURCE_DIR}/license/license.xml")
set(READLICENSE_OO_LICENSE_HTML "${READLICENSE_OO_WORKDIR}/LICENSE.html")
set(READLICENSE_OO_LICENSE_TXT "${READLICENSE_OO_WORKDIR}/LICENSE")

add_custom_command(
    OUTPUT "${READLICENSE_OO_LICENSE_HTML}"
    COMMAND "${XSLTPROC_EXECUTABLE}" --nonet --novalid
            -o "${READLICENSE_OO_LICENSE_HTML}"
            --stringparam build_type ""
            --stringparam os "LINUX"
            --stringparam themes ""
            --stringparam no_mpl_subset no_mpl_subset
            "${CMAKE_CURRENT_SOURCE_DIR}/license/license_html.xsl"
            "${READLICENSE_OO_LICENSE_XML}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/license/license_html.xsl" "${READLICENSE_OO_LICENSE_XML}"
    COMMENT "readlicense_oo: LICENSE.html (CustomTarget_license.mk)"
    VERBATIM)

add_custom_command(
    OUTPUT "${READLICENSE_OO_LICENSE_TXT}"
    COMMAND "${XSLTPROC_EXECUTABLE}" --nonet --novalid
            -o "${READLICENSE_OO_LICENSE_TXT}"
            --stringparam build_type ""
            --stringparam os "LINUX"
            --stringparam themes ""
            --stringparam no_mpl_subset no_mpl_subset
            "${CMAKE_CURRENT_SOURCE_DIR}/license/license_plain_text.xsl"
            "${READLICENSE_OO_LICENSE_XML}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/license/license_plain_text.xsl" "${READLICENSE_OO_LICENSE_XML}"
    COMMENT "readlicense_oo: LICENSE (CustomTarget_license.mk)"
    VERBATIM)

add_custom_target(readlicense_oo_license_files ALL
    DEPENDS "${READLICENSE_OO_LICENSE_HTML}" "${READLICENSE_OO_LICENSE_TXT}")

# --- CustomTarget_readme: README_en-US ---------------------------------------
#
# readlicense_oo_LANGS := en-US $(filter-out en-US,$(gb_WITH_LANG)) -- with
# gb_WITH_LANG unset (this port has no --with-lang equivalent), that is just
# "en-US", and readlicense_oo_README_XRM takes the `ifeq ($(strip
# $(gb_WITH_LANG)),)` branch: the raw SRCDIR readme.xrm directly, no xrmex
# merge step needed. gb_README(en-US) = README_$(1) on non-Windows
# (solenv/gbuild/platform/com_GCC_defs.mk), i.e. the output file is literally
# named "README_en-US" with no extension.
#
# readme.xsl's own stringparams: com1/cp1 name the compiler/CPU this build was
# made with -- purely informational text in the rendered README, with no
# equivalent variable in this port yet either; COM=GCC and CPUNAME=X86_64
# mirror the toolchain/architecture this port documents itself as targeting
# (see cmake/LibreStudioPlatform.cmake's own "real upstream naming ... CPUNAME
# =X86_64" comment). lang1 is the language being rendered (en-US); type=text
# matches every upstream call (only a `text`-typed README is ever rendered
# into this file; the `html` type feeds a different consumer this build does
# not have). The ifeq($(OS),WNT) CRLF conversion and readme_SED's
# PRODUCTNAME/PRODUCTVERSION/LCPRODUCTNAME substitution are reproduced with a
# plain sed pass, mirroring readlicense_oo_README_SED exactly.
find_program(SED_EXECUTABLE sed REQUIRED)

set(READLICENSE_OO_README_XRM "${CMAKE_CURRENT_SOURCE_DIR}/docs/readme.xrm")
set(READLICENSE_OO_README_RAW "${READLICENSE_OO_WORKDIR}/README_en-US.out")
set(READLICENSE_OO_README_EN_US "${READLICENSE_OO_WORKDIR}/README_en-US")

add_custom_command(
    OUTPUT "${READLICENSE_OO_README_EN_US}"
    COMMAND "${XSLTPROC_EXECUTABLE}" --nonet --novalid
            -o "${READLICENSE_OO_README_RAW}"
            --stringparam com1 "GCC"
            --stringparam cp1 "X86_64"
            --stringparam lang1 "US"
            --stringparam os1 "LINUX"
            --stringparam type "text"
            "${CMAKE_CURRENT_SOURCE_DIR}/docs/readme.xsl"
            "${READLICENSE_OO_README_XRM}"
    COMMAND sh -c "'${SED_EXECUTABLE}' \
            -e 's,\${PRODUCTNAME},${READLICENSE_OO_PRODUCTNAME},g' \
            -e 's,\${PRODUCTVERSION},${READLICENSE_OO_PRODUCTVERSION},g' \
            -e 's,\${LCPRODUCTNAME},${READLICENSE_OO_LCPRODUCTNAME},g' \
            '${READLICENSE_OO_README_RAW}' > '${READLICENSE_OO_README_EN_US}'"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/docs/readme.xsl" "${READLICENSE_OO_README_XRM}"
    COMMENT "readlicense_oo: README_en-US (CustomTarget_readme.mk)"
    VERBATIM)

add_custom_target(readlicense_oo_readme_files ALL DEPENDS "${READLICENSE_OO_README_EN_US}")

# --- Package_files: NOTICE + CREDITS.fodt (top-level of the install tree) ---
#
# gb_Package_Package(readlicense_oo_files,$(SRCDIR)/readlicense_oo/license)
# with gb_Package_add_file(...,NOTICE,NOTICE) -- a *bare* destination (no
# directory component), which gb_Package's default OUTDIR ($(INSTROOT))
# places at the very top of the install tree, sibling to program/ and share/,
# not under either. On MACOSX the destination gains a Resources/ prefix; that
# branch is skipped (non-Darwin port). Both source files are static, verified
# present in readlicense_oo/license/.
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/license/NOTICE"
    "${CMAKE_CURRENT_SOURCE_DIR}/license/CREDITS.fodt"
    DESTINATION .)

# --- Package_license: LICENSE + LICENSE.html (top-level, non-WNT/non-MACOSX) -
#
# Same bare, top-of-tree destination convention as Package_files above.
install(FILES "${READLICENSE_OO_LICENSE_TXT}" "${READLICENSE_OO_LICENSE_HTML}" DESTINATION .)

# --- Package_readlicense_oo_readmes: README_en-US (top-level, LIBO_SHARE_
#     READMES_FOLDER) -------------------------------------------------------
#
# LIBO_SHARE_READMES_FOLDER=readmes on this platform (configure.ac's Unix/
# WINNT branches both set it, unprefixed by LIBO_SHARE_FOLDER) -- another
# top-level, not share/-nested, directory.
install(FILES "${READLICENSE_OO_README_EN_US}" DESTINATION readmes)

