# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# scp2 -- upstream's own installer-manifest scripting layer: a bespoke
# ".scp" file format (compiled by the scp2 tool into per-feature file lists)
# that tells instsetoo_native's packaging step which already-built
# libraries/executables/jars/data files belong in which installer feature
# ("module", in installer-set terms -- Base, Calc, Writer, the URE, ...) and
# under what conditions (platform, enabled features, language). It produces
# no libraries, executables, or anything soffice.bin loads at runtime; its
# only consumer is instsetoo_native's own packaging pipeline.
#
# Module_scp2.mk, read in full: a single gb_Module_add_targets(scp2, ...)
# call listing AutoInstall, CustomTarget_langmacros, 14 unconditional
# InstallModule_* targets, InstallScript_setup_osl, and four conditional
# groups --
#   - $(if $(filter ODK,$(BUILD_TYPE)), InstallModule_sdkoo InstallScript_sdkoo)
#   - $(if $(filter WNT,$(OS)), InstallModule_activex InstallModule_quickstart
#     InstallModule_windows InstallModule_winexplorerext)
#   - $(if $(filter TRUE,$(ENABLE_EVOAB2) $(ENABLE_GIO) $(ENABLE_GTK3)), InstallModule_gnome)
#   - $(if $(filter TRUE,$(ENABLE_QT5) $(ENABLE_QT6) $(ENABLE_KF5) $(ENABLE_KF6) $(ENABLE_GTK3_KDE5)), InstallModule_kde)
# -- all four gated on values this port either never sets (BUILD_TYPE=ODK,
# ENABLE_EVOAB2/ENABLE_GIO/ENABLE_GTK3/ENABLE_QT5/ENABLE_KF5/
# ENABLE_GTK3_KDE5: grepped, zero hits anywhere) or is not the target platform
# for (OS=WNT); ENABLE_QT6 IS used elsewhere in this port's own
# cmake/LibreStudioPlatform.cmake (per the "Qt6 vclplug" work in this
# branch's own recent commits) but scp2 itself has no CMakeLists.txt wiring
# it up, so that one conditional group is inert here too pending an
# InstallModule_kde port, not because the flag is unreachable.
#
# The 14 unconditional targets themselves are the real substance:
# AutoInstall.mk (gb_AutoInstall_add_module per feature, e.g. "ooo"/"calc"/
# "writer") and each InstallModule_*.mk (e.g. InstallModule_base.mk, read as
# the representative sample: gb_InstallModule_use_auto_install_libs +
# gb_InstallModule_add_scpfiles/add_localized_scpfiles pulling in
# scp2/source/base/*.scp) are exactly the installer-feature-manifest
# authoring this module exists for -- there is no lesser, always-useful
# subset of it independent of that purpose.
#
# Unlike solenv/soltools (gbuild's own tooling, superseded natively by
# CMake/Ninja) this is real content with a real, still-missing consumer:
# instsetoo_native's own CustomTarget_install is itself gated on PKGFORMAT/
# LIBO_TEST_INSTALL/ENABLE_WIX, none of which this port ever sets (see
# instsetoo_native/CMakeLists.txt) -- this port has no .deb/.rpm/.msi/.dmg
# packaging or release pipeline at all yet. scp2's own output (compiled .scp
# file lists) is useless without that pipeline to feed, so there is nothing
# for a CMake target here to produce that anything else in this build tree
# would consume. An honest packaging-pipeline gap, not a superseded
# mechanism and not an inapplicable platform gate: nothing to wire until
# instsetoo_native's own packaging step exists.
