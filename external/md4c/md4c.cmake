# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring drivers for md4c and md4c-html, invoked by librestudio_pkgconfig()
# (cmake/LibreStudioTarballs.cmake, called from sc/sw's own CMakeLists.txt)
# only when the respective system package is not found. md4c and md4c-html
# are resolved by two INDEPENDENT calls (md4c and md4c-html are resolved
# separately -- a deliberate, small behavior change from the pre-refactor
# "no system md4c-html -> vendor BOTH" coupling, since they are genuinely
# independent system packages), both pointing at this same driver file.
#
# One shared tarball (librestudio_fetch_tarball() is idempotent via its own
# stamp file, so calling it twice -- once per driver, in whichever order
# librestudio_vendor_md4c()/librestudio_vendor_md4c_html() happen to run --
# just returns the already-unpacked md4c_SOURCE_DIR the second time).
function(librestudio_vendor_md4c)
    message(STATUS "md4c: no system package found -- building md4c 0.5.3 from source")

    # download.lst's own MD4C_TARBALL/MD4C_SHA256SUM, verified directly and
    # cross-checked against the real downloaded file's own sha256sum.
    librestudio_fetch_tarball(md4c md4c-release-0.5.3.tar.gz
        353c346f376b87c954a13f3415ede2d51264cc61dc5abcd38ff1d2aa0d059b9e)

    # StaticLibrary_md4c.mk's own source list, transcribed verbatim
    # (re-verified against the real 0.5.3 tarball: all 3 files exist, 0
    # missing). No patches (UnpackedTarball_md4c.mk names none).
    add_library(md4c STATIC
        "${md4c_SOURCE_DIR}/src/entity.c"
        "${md4c_SOURCE_DIR}/src/md4c.c")
    target_include_directories(md4c PUBLIC "${md4c_SOURCE_DIR}/src")
    target_compile_options(md4c PRIVATE -w)

    librestudio_vendor_set_target(MD4C md4c)
endfunction()

function(librestudio_vendor_md4c_html)
    message(STATUS "md4c-html: no system package found -- building md4c-html 0.5.3 from source")

    # md4c-html.c is a separate translation unit on top of md4c's own AST
    # (#include "md4c.h", calls md_parse()) -- resolved lazily here (system
    # or vendored, whichever md4c itself needs independently) rather than
    # assumed vendored, since md4c and md4c-html are now independently
    # resolved system-or-vendor decisions.
    librestudio_pkgconfig(
        MODULES md4c
        TARGET_VAR _md4c_html_md4c_target)

    librestudio_fetch_tarball(md4c md4c-release-0.5.3.tar.gz
        353c346f376b87c954a13f3415ede2d51264cc61dc5abcd38ff1d2aa0d059b9e)

    add_library(md4c_html STATIC "${md4c_SOURCE_DIR}/src/md4c-html.c")
    target_include_directories(md4c_html PUBLIC "${md4c_SOURCE_DIR}/src")
    target_compile_options(md4c_html PRIVATE -w)
    target_link_libraries(md4c_html PUBLIC ${_md4c_html_md4c_target})

    librestudio_vendor_set_target(MD4C_HTML md4c_html)
endfunction()
