# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# reportbuilder -- the Base "Report Builder" wizard: a Java UNO service
# (org.libreoffice.report.*) built on top of the vendored Pentaho/JFreeReport
# engine (flow-engine/flute/libbase/libfonts/libformula/liblayout/libloader/
# librepository/libserializer/libxml/sac), plus a single default report
# template.
#
# Module_reportbuilder.mk, read in full, gates its entire content on one
# flag:
#
#   ifeq ($(ENABLE_REPORTBUILDER),TRUE)
#   $(eval $(call gb_Module_add_targets,reportbuilder, Jar_reportbuilder Package_templates))
#   endif
#
# ENABLE_REPORTBUILDER is a real upstream ./configure switch (default ON
# there, alongside ENABLE_JAVA); this CMake port never introduces the option
# at all (grepped every CMakeLists.txt/*.cmake: zero hits) -- so, matching a
# configure-less build with no way to turn it on yet, the whole module is
# currently inactive, the same "genuinely empty on this configuration" shape
# as e.g. static/CMakeLists.txt's ENABLE_CUSTOMTARGET_COMPONENTS gate. This is
# NOT the same "no Java bridge" story as qadevOOo/nlpsolver: even a future
# LIBRESTUDIO_ENABLE_REPORTBUILDER option would immediately hit a second,
# harder blocker, verified below.
#
# Jar_reportbuilder.mk itself (read in full): ~150 org.libreoffice.report.*
# sourcefiles, plus gb_Jar_use_externals,reportbuilder, "flow-engine flute
# libbase libfonts libformula liblayout libloader librepository libserializer
# libxml sac" -- every one of those is a jar built from
# external/jfreereport/*.mk (ExternalProject_jfreereport_*.mk driving an
# ant/ivy build of the vendored JFreeReport/Pentaho engine sources,
# UnpackedTarball_jfreereport_*.mk unpacking+patching them first). Checked
# this CMake port for any trace of external/jfreereport: none -- no
# CMakeLists.txt anywhere under external/jfreereport, and no reference to
# "jfreereport" in any CMakeLists.txt/*.cmake in this tree. Per this port's
# own policy (externals are wired individually by whichever module needs
# them, not enumerated speculatively), jfreereport simply has not been
# reached yet.
#
# Package_templates.mk (the module's other target) is, in isolation, a
# trivial single-file copy (default.otr into
# $(LIBO_SHARE_FOLDER)/template/common/wizard/report/) -- but it sits behind
# the exact same ENABLE_REPORTBUILDER gate as Jar_reportbuilder in
# Module_reportbuilder.mk, so it is equally inactive on this configuration,
# not a smaller independently-portable piece.
#
# Net: real work, but doubly blocked -- (1) no ENABLE_REPORTBUILDER-equivalent
# option exists in this port yet, matching upstream's own gate faithfully,
# and (2) even with one, the whole Jar depends on external/jfreereport, which
# has not been vendored/ported to CMake at all. An honest two-layer gap, not
# a "no Java bridge" or "superseded tooling" case. Nothing wired.
