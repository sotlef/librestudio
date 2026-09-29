# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# odk -- the Software Development Kit: the public API headers/IDL, generated
# docs, examples, settings makefiles and a couple of small native helpers
# shipped so third parties can build UNO components against this office.
#
# Module_odk.mk, read in full, is the LEAST "probably out of scope" of the
# eight modules assigned here -- most of its content is unconditional on
# Linux, not gated behind ENABLE_JAVA/OS=WNT/DOXYGEN at all. Each target
# group was checked individually, not assumed:
#
#   - Executable_unoapploader (odk/Executable_unoapploader.mk): unconditional,
#     no OS gate on the non-Windows branch. A small, fully standalone native
#     loader (odk/source/unoapploader/unx/unoapploader.c on this platform)
#     that only needs cppuhelper's findsofficepath static helper -- already
#     ported and building today (cppuhelper/CMakeLists.txt's own
#     `add_library(findsofficepath STATIC ...)`) -- and libdl (the .mk's own
#     `$(if $(filter-out FREEBSD NETBSD DRAGONFLY,$(OS)),-ldl)`, unconditionally
#     true for this Linux port). No missing dependency, no staged-install
#     tree needed: genuinely buildable right now, so it is the one target in
#     this file that is actually wired below, not just documented.
#
#   - Package_odk_headers / Package_odk_headers_generated / Package_config /
#     Package_docs / Package_examples / Package_settings /
#     Package_settings_generated / Package_share_readme /
#     Package_share_readme_generated / Package_scripts, and the
#     CustomTarget_html / CustomTarget_settings that feed two of them: all
#     unconditional (Package_share_readme_generated's one file is WNT-only,
#     the rest apply on Linux too). These stage the actual SDK tree --
#     public headers verbatim from $(SRCDIR)/include, generated HTML docs,
#     ~650 example files, settings/config/readme scripts -- under
#     $(INSTDIR)/$(SDKDIRNAME)/. This is real, mechanically portable work,
#     NOT blocked by any missing staged-install concept: this port already
#     has one, the same CMAKE_INSTALL_PREFIX-as-instdir tree root
#     CMakeLists.txt:234-235 defaults to ${CMAKE_BINARY_DIR}/instdir and that
#     desktop/CMakeLists.txt already installs into (`install(TARGETS
#     soffice_bin RUNTIME DESTINATION program)` and its siblings) -- a plain
#     `install(FILES ... DESTINATION ${SDKDIRNAME}/...)` per Package_*.mk
#     would work the same way. What IS genuinely missing, verified by
#     grepping every CMakeLists.txt/*.cmake in this tree: SDKDIRNAME itself
#     (the SDK subdirectory name every one of these targets installs under)
#     is defined nowhere, and neither are PRODUCTNAME/PRODUCTVERSION/
#     PLATFORMID (CustomTarget_html's sed substitutions and
#     CustomTarget_settings' dk.mk substitutions both need them). Small,
#     concrete, and not yet chosen -- unlike instsetoo_native/scp2's genuine
#     "no packaging pipeline" gap, this is a "nobody has picked SDKDIRNAME/
#     PRODUCTNAME yet" gap. Given the size of this group (Package_examples.mk
#     alone lists ~607 files, and odk/examples/ on disk actually has 652 --
#     the extra ~45 are not part of the curated upstream list, so a faithful
#     port needs the exact enumerated set, not a directory copy) transcribing
#     the full file lists was left out of this pass rather than done
#     partially or approximated with install(DIRECTORY ...), which would
#     silently over-copy relative to the real Package_examples.mk. Flagged
#     here as real, ready-to-do follow-up work, not modeled as "out of
#     scope."
#
#   - CustomTarget_allheaders / CppunitTest_odk_checkapi / CustomTarget_check /
#     CustomTarget_build-examples[_java] (gb_Module_add_check_targets /
#     gb_Module_add_subsequentcheck_targets, the latter under
#     `ifneq ($(filter $(OS),LINUX MACOSX),)` -- true here): every one reads
#     back the staged SDK tree above (allheaders.hxx from odk_headers/
#     odk_headers_generated's Package targets; checkapi.cxx's
#     `-I$(INSTDIR)/$(SDKDIRNAME)/include`; check.pl driven against
#     $(INSTDIR)/$(SDKDIRNAME)). This is a real CppunitTest -- this port DOES
#     have librestudio_add_cppunit_test() (cmake/LibreStudioTest.cmake) and
#     could in principle run it -- but it is blocked transitively on the
#     still-unstaged SDK tree above, not on any missing test-runner category
#     (unlike qadevOOo's JunitTest gap or solenv's PythonTest gap).
#     CustomTarget_build-examples/_java additionally invoke the SDK's own
#     configure.pl/setsdkenv_unix + make against the packaged examples, a
#     shell-out to a shipped build system this port does not drive from
#     CTest.
#
#   - $(if $(DOXYGEN), CustomTarget_doxygen GeneratedPackage_odk_doxygen):
#     DOXYGEN is upstream's own ./configure-detected doxygen binary; this
#     port has no find_package(Doxygen) anywhere (grepped: zero hits) -- an
#     honest "not looked for yet" gap, not a platform restriction (Doxygen
#     itself is available on Linux).
#
#   - `ifeq ($(OS),WNT)` block (Package_cli/CustomTarget_config_win/
#     Package_config_win) and `ifneq ($(ENABLE_JAVA),)` block
#     (CustomTarget_classes/CustomTarget_javadoc/
#     GeneratedPackage_odk_javadoc/GeneratedPackage_uno_loader_classes):
#     the WNT block is inapplicable to this Linux x86_64 port outright. The
#     ENABLE_JAVA block is NOT simply "no Java bridge" (LIBRESTUDIO_ENABLE_JAVA
#     is real and wired, see nlpsolver/qadevOOo's own CMakeLists.txt) -- it
#     is additionally blocked on the same unstaged $(INSTDIR)/$(SDKDIRNAME)
#     tree as the check-targets group above (CustomTarget_classes packages
#     odk/source/com/sun/star/lib/loader/* into it), so both gaps would need
#     closing together.
#
# Net: this module contradicts a blanket "probably out of scope" reading.
# Only unoapploader is wired here; the SDK-staging group is real,
# unblocked-in-principle work intentionally left as follow-up (see above),
# and the check/Java/Doxygen groups are genuinely blocked on that same
# staging gap plus SDKDIRNAME/PRODUCTNAME/DOXYGEN not being chosen/detected
# yet -- none of it is inapplicable by architecture the way, say,
# solenv/soltools's gbuild-tooling is.

add_executable(unoapploader
    "${CMAKE_SOURCE_DIR}/odk/source/unoapploader/unx/unoapploader.c")

target_link_libraries(unoapploader PRIVATE findsofficepath dl)
