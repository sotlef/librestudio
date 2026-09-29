# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# instsetoo_native -- upstream's native (non-Java) installer-set builder: it
# generates the runtime bootstrap .ini/rc files every soffice.bin needs to
# start, AND drives the actual installable-package construction (.deb/.rpm/
# .msi/.dmg/...) from them plus scp2's file lists.
#
# Module_instsetoo_native.mk, read in full, has two gb_Module_add_targets
# calls:
#
#   1. ifneq (,$(PKGFORMAT)$(filter TRUE,$(LIBO_TEST_INSTALL) $(ENABLE_WIX)))
#      -- gates CustomTarget_install (or, under OS=EMSCRIPTEN,
#      CustomTarget_emscripten-install). PKGFORMAT (deb/rpm/msi/dmg/...),
#      LIBO_TEST_INSTALL and ENABLE_WIX are all upstream ./configure options;
#      grepped this whole port for all three: zero hits anywhere in any
#      CMakeLists.txt/*.cmake file, so, matching upstream's own configure-less
#      default, the gate is false and this target group is genuinely inactive
#      -- and even set, it drives per-format installer packaging (WiX/rpmbuild/
#      dpkg-deb/pkgbuild invocations over the whole tree) that has no CMake
#      equivalent to fall back on: this port has no packaging/release
#      pipeline at all yet, an honest gap, not a "superseded" claim like
#      solenv/soltools's build tooling.
#
#   2. gb_Module_add_targets(instsetoo_native, CustomTarget_setup Package_setup
#      Package_setup_ure $(if $(ENABLE_OOENV),Package_instsetoo_native_ooenv))
#      -- UNCONDITIONAL (no PKGFORMAT/OS gate at all): every normal build,
#      including this one, is expected to produce these. This is the
#      contradiction worth flagging explicitly: this is real, load-bearing
#      work, not installer packaging.
#
#      CustomTarget_setup (instsetoo_native/CustomTarget_setup.mk) generates
#      the actual runtime bootstrap files soffice.bin's configmgr/cppuhelper
#      read at startup: bootstrap.ini (UserInstallation/InstallMode),
#      fundamentalrc (BRAND_BASE_DIR, CONFIGURATION_LAYERS -- without which
#      configmgr has nowhere to look and InitVCL() fails outright), unorc
#      (URE_INTERNAL_LIB_DIR/UNO_TYPES/UNO_SERVICES), sofficerc, versionrc,
#      redirectrc (DESKTOP builds, non-macOS) and crashreport.ini.
#      Package_setup/Package_setup_ure (instsetoo_native/Package_setup*.mk)
#      then copy those generated files into $(LIBO_ETC_FOLDER)/
#      $(LIBO_URE_ETC_FOLDER) of the install tree -- plain file-copy Package
#      targets, no different in kind from the install(TARGETS ... DESTINATION
#      program) calls desktop/CMakeLists.txt already uses against this port's
#      own CMAKE_INSTALL_PREFIX-as-instdir tree (root CMakeLists.txt:234-235
#      defaults CMAKE_INSTALL_PREFIX to ${CMAKE_BINARY_DIR}/instdir when the
#      user does not override it, the same staged-tree role gbuild's own
#      INSTDIR plays for these exact Package_* targets).
#
#      This is NOT untouched, unexpected territory: this port has ALREADY
#      independently reimplemented part of CustomTarget_setup's job, directly
#      in cmake/LibreStudioUnorc.cmake, whose own header comment says so in
#      as many words: "Upstream writes this file from
#      instsetoo_native/CustomTarget_setup.mk, together with fundamentalrc,
#      sofficerc and friends, which carry the full installation
#      configuration. This build generates only unorc, which is what
#      defaultBootstrap_InitialComponentContext() needs" -- i.e. exactly the
#      unorc slice of instsetoo_native's real, unconditional output, done as
#      its own small piece of CMake infrastructure rather than a full
#      instsetoo_native port, with the rest (bootstrap.ini/fundamentalrc/
#      sofficerc/versionrc/redirectrc/crashreport.ini, and the Package_setup/
#      Package_setup_ure copy step for any of it) called out by that same
#      file's own comment as not yet done ("This build generates only
#      unorc"). Whoever continues cppuhelper/desktop's runtime-bootstrap work
#      should treat that TODO, not a from-scratch instsetoo_native/
#      CMakeLists.txt port, as the next step for the rest of this target
#      group -- duplicating it here without touching cmake/LibreStudioUnorc.cmake
#      or desktop/CMakeLists.txt (both out of this file's scope) would only
#      create a second, competing generator for the same files.
#
#      Package_instsetoo_native_ooenv is gated on ENABLE_OOENV, another
#      upstream configure option this port never introduces (grepped: zero
#      hits) -- genuinely inactive, matching upstream's own default-off
#      behaviour.
#
# Net: instsetoo_native is NOT simply "installer packaging, out of scope" --
# its unconditional bootstrap-file-generation half is real, necessary work
# this port has already started (in cmake/LibreStudioUnorc.cmake) and left
# admittedly incomplete; only its PKGFORMAT/WiX-gated packaging half is the
# genuine, honest "no packaging pipeline yet" gap. Nothing is wired directly
# in this file: the active part of the work belongs with the runtime-
# bootstrap files it already owns (cmake/LibreStudioUnorc.cmake, desktop/),
# and the inactive part has no CMake equivalent to write yet.
