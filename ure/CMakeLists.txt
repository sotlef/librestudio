# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# ure -- Module_ure.mk, read in full, lists exactly two targets:
#   Package_ure_install
#   Rdb_ure
# Both are documentation-only here: one has zero real content on this
# platform, the other is redundant with a mechanism this port already runs.

# --- Package_ure_install -----------------------------------------------------
#
# ure/Package_ure_install.mk, read in full:
#
#   $(eval $(call gb_Package_Package,ure_install,$(SRCDIR)/ure/source))
#   ifeq (MACOSX,$(OS))
#   ifneq ($(ENABLE_MACOSX_SANDBOX),TRUE)
#   $(eval $(call gb_Package_add_symbolic_link,ure_install,MacOS/urelibs,../Frameworks))
#   endif
#   endif
#
# The Package base directory (ure/source) is declared but gb_Package_add_file/
# gb_Package_add_files is never called on it -- the only content-adding call
# in the whole file is gb_Package_add_symbolic_link, and it is nested inside
# `ifeq (MACOSX,$(OS))`. On this Linux-only port that condition never holds,
# so, exactly like static/Module_static.mk and android/Module_android.mk
# (see static/CMakeLists.txt), this target contributes zero real files on
# every platform this port currently builds for. There is nothing to
# transcribe into install() here.

# --- Rdb_ure ------------------------------------------------------------------
#
# ure/Rdb_ure.mk, read in full:
#
#   $(eval $(call gb_Rdb_Rdb_install,ure/services,$(LIBO_URE_MISC_FOLDER)/services.rdb))
#
# This declares an Rdb named "ure/services" and installs the packed registry
# to $(LIBO_URE_MISC_FOLDER)/services.rdb. Two facts, both checked directly
# rather than assumed, show this is already done by this port's existing
# service-registry mechanism:
#
# 1. LIBO_URE_MISC_FOLDER resolves to "program" on every non-Darwin, non-WNT
#    target (configure.ac's OS-dispatch around line 4168/4191 -- WINNT and the
#    generic-Unix `else` branch both set LIBO_URE_MISC_FOLDER=program), so the
#    file Rdb_ure.mk installs is program/services.rdb.
#
# 2. "ure/services" is not a private, URE-only registry: it is the exact Rdb
#    name every low-level bootstrap library in this tree registers its
#    .component file into -- grep confirms 13 gb_Library_set_componentfile
#    calls naming "ure/services" literally (io/Library_io.mk,
#    remotebridges/Library_uuresolver.mk, binaryurp/Library_binaryurp.mk,
#    and stoc's bootstrap/proxyfac/reflection/invocation/javavm/stocservices/
#    javaloader/invocadapt/namingservice/introspection libraries). Every one
#    of those libraries, as ported in this tree, registers into this port's
#    own single combined "services" property via librestudio_add_component()
#    (see e.g. io/CMakeLists.txt's own librestudio_add_component(io ...) call)
#    -- there is no separate "ure/services"-only registry anywhere in this
#    port's CMake graph; this port folds it into the same one upstream calls
#    plain "services" (see postprocess/CMakeLists.txt for the other half of
#    this same finding, and why that merge is also the right call there).
#
# The root CMakeLists.txt packs that combined property and installs it at
# exactly program/services.rdb:
#
#   librestudio_finalize_components(services)
#   add_custom_command(
#       OUTPUT "${LIBRESTUDIO_PROGRAM_DIR}/services.rdb"
#       COMMAND ... copy_if_different "${services_RDB}" "${LIBRESTUDIO_PROGRAM_DIR}/services.rdb"
#       ...)
#   add_custom_target(program_services_rdb ALL DEPENDS "${LIBRESTUDIO_PROGRAM_DIR}/services.rdb")
#
# (root CMakeLists.txt, right after the last add_subdirectory() call, comment
# "The service manager finds libraries through the uri attribute ..."), where
# LIBRESTUDIO_PROGRAM_DIR is CMAKE_BINARY_DIR/instdir/program (cmake/
# LibreStudioPlatform.cmake) -- i.e. the CMAKE_INSTALL_PREFIX-relative path
# "program", matching LIBO_URE_MISC_FOLDER exactly. This is also the path
# unorc's own built-in default for UNO_SERVICES already expects (root
# CMakeLists.txt's own comment, near program_services_rdb's DEPENDS wiring:
# "unorc's own ini default for UNO_SERVICES" falls back to this exact file),
# so nothing downstream needs a second, differently-named services.rdb.
#
# Net effect: this port already produces, at the exact path Rdb_ure.mk names,
# a services.rdb containing a strict superset of what "ure/services" alone
# would hold (the 13 ure-scoped components plus everything the 188 other
# gb_Library_set_componentfile(...,services) callers upstream register into
# the *other* Rdb -- see postprocess/CMakeLists.txt). Re-declaring Rdb_ure as
# its own separate CMake target here would either duplicate that work or
# race it; there is nothing left for this file to add.

