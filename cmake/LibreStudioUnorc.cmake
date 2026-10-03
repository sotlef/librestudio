# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Generation of unorc, the bootstrap configuration the UNO runtime reads.
#
# cppuhelper/source/paths.cxx looks for a file called unorc in the directory of
# the library it is linked into — not next to the executable, and not in an etc
# directory — so this has to land in the office's program/ directory alongside
# libcppuhelper${LIBRESTUDIO_CPPU_ENV}.so (libcppuhelpergcc3.so on Linux and
# macOS today; on Windows this would be cppuhelper${LIBRESTUDIO_CPPU_ENV}.dll,
# i.e. cppuhelpermscx.dll, once the mscx_uno bridge referenced in
# LibreStudioPlatform.cmake exists to link against).
#
# The filename itself is NOT "unorc" on every platform: include/sal/config.h's
# SAL_CONFIGFILE(name) macro (which cppu::getUnoIniUri() in paths.cxx calls as
# SAL_CONFIGFILE("uno")) expands to "unorc" under SAL_UNX (Linux, macOS, ...)
# but to "uno.ini" under _WIN32 — so the real Windows filename would be
# uno.ini, not unorc. getUnoIniUri() otherwise resolves the directory the same
# way on Windows as on Linux (next to the library, via get_this_libpath()); the
# extra MACOSX-only redirect into Resources/ure/etc does not apply there either.
# TODO(windows): only the Linux content/filename below is actually generated;
# nobody has verified whether the [Bootstrap] key names, the ${ORIGIN} token,
# or CONFIGURATION_LAYERS' value syntax need to change for uno.ini, only that
# the filename does. That is an open question for whoever picks up the
# Windows bridge work, not resolved here.
#
# Upstream writes this file from instsetoo_native/CustomTarget_setup.mk, together
# with fundamentalrc, sofficerc and friends, which carry the full installation
# configuration.  This build generates only unorc, which is what
# defaultBootstrap_InitialComponentContext() needs:
#
#   * URE_INTERNAL_LIB_DIR is what the uri attributes in services.rdb expand
#     $URE_INTERNAL_LIB_DIR against;
#   * LO_LIB_DIR does the same for office-layer components such as comphelper;
#   * LO_JAVA_DIR/URE_INTERNAL_JAVA_DIR do the same for Java-loaded (loader=
#     "com.sun.star.loader.Java2") components' uri attributes -- e.g.
#     scripting's own ScriptFramework/ScriptProviderForBeanShell.component
#     (librestudio_add_java_component(), cmake/LibreStudioComponent.cmake) --
#     both point at the same physical directory as URE_INTERNAL_JAVA_DIR on
#     this single-layer port (matching how LO_LIB_DIR/URE_INTERNAL_LIB_DIR
#     already collapse to the same ${ORIGIN} for native components above);
#   * UNO_TYPES and UNO_SERVICES name the registries.
#
# UNO_TYPES is a list, and every entry is needed: the UNO core's own types are
# in types.rdb, every service in this tree that speaks an interface from the
# office API — com.sun.star.i18n among them — needs offapi's registry, and
# basic's VBA-compatibility objects (ooo.vba.* interfaces, instantiated at
# macro runtime) need oovbaapi's, and Code's backend services
# (com.sun.star.code.*) need codeapi's.  Without any one of them, the service
# manager fails to build the corresponding component at all.  A single merged
# types.rdb would do as well; see the comment on program_types_rdb in the
# top-level CMakeLists.txt for why this build keeps them apart.
#
# ${ORIGIN} is expanded by the bootstrap code to the directory the ini file was
# found in, so the file stays correct wherever the tree is moved.
#
# CONFIGURATION_LAYERS is what configmgr reads to find the registry, and without
# it there is none: InitVCL() builds the default style settings, asks for
# /org.openoffice.VCL/DefaultFonts, and gets a RuntimeException from a
# configuration that has nowhere to look.  Upstream writes the same value into
# fundamentalrc, in the longer form with the extension, user and res layers —
# see instsetoo_native/CustomTarget_setup.mk — of which this build produces the
# first.  The value is not a path but a list of <type>:<url>, and xcsxcu is the
# type that reads .xcd files from the url and, failing those, the .xcs and .xcu
# trees beside them.
#
# BRAND_BASE_DIR/BRAND_SHARE_SUBDIR: added after a real run of a plain VCL
# app (code/, see project memory project_code_ide) crashed constructing its
# first InterimItemWindow-based control (TabBar's own internal TabButtons) --
# AllSettings::GetUIRootDir() (vcl/source/app/settings.cxx) expands
# "$BRAND_BASE_DIR/$BRAND_SHARE_SUBDIR/config/soffice.cfg/" via
# rtl::Bootstrap::expandMacros(), and an unresolved bootstrap macro expands to
# nothing rather than erroring, so with neither variable defined the whole
# thing silently collapsed to "//config/soffice.cfg/" -- passed straight to
# osl_openFile() by xmlreader::XmlReader, which of course cannot open it.
# Every generated rc (this function is the only source of all of them,
# sofficerc included) was missing both, so this was never Code-specific: it
# is only that no earlier CppunitTest or verification pass had ever actually
# constructed a real InterimItemWindow/weld-.ui-backed widget at runtime to
# notice. BRAND_BASE_DIR is the installation root (one level up from
# program/, where ${ORIGIN} always points); BRAND_SHARE_SUBDIR is "share" --
# the same fixed folder name OFFICECFG_SHARE_FOLDER already uses elsewhere in
# this port (officecfg/CMakeLists.txt) for the identical "share" value.
#
# UserInstallation: added after a real run of "code" (see project memory
# project_code_ide) got past everything else and then hard-failed with
# framework's own STR_CORRUPT_UICFG_USER message ("An error occurred while
# loading the user interface configuration data"). framework's PresetHandler
# (framework/source/accelerators/presethandler.cxx,
# getOrCreateRootStorageUser()) opens a real writable "soffice.cfg" directory
# under XPathSettings' UserConfig path, which resolves through the $(user)
# path-substitution variable -- and that variable
# (framework/source/services/substitutepathvars.cxx,
# SetPredefinedPathVariables()) comes from utl::Bootstrap::locateUserData(),
# which reads exactly this "UserInstallation" bootstrap key
# (unotools/source/config/bootstrap.cxx). Without it $(user) is empty, so
# UserConfig resolves to a bogus path with nothing there to open -- not
# specific to Code, every generated rc this function writes was missing it
# (this file's own history already has one prior case of exactly this shape:
# BRAND_BASE_DIR/BRAND_SHARE_SUBDIR above). Points inside the build tree
# itself (a real "user" directory is created there on first run) rather than
# a real system config directory, since this is a dev build, not an install.

function(librestudio_generate_unorc output_file)
    set(_content
"# Generated by LibreStudio's CMake build -- do not edit.
[Bootstrap]
BRAND_BASE_DIR=\${ORIGIN}/..
BRAND_SHARE_SUBDIR=share
URE_INTERNAL_LIB_DIR=\${ORIGIN}
URE_INTERNAL_JAVA_DIR=\${ORIGIN}/classes
LO_LIB_DIR=\${ORIGIN}
LO_JAVA_DIR=\${ORIGIN}/classes
UNO_TYPES=\${ORIGIN}/types.rdb \${ORIGIN}/types/offapi.rdb \${ORIGIN}/types/oovbaapi.rdb \${ORIGIN}/types/codeapi.rdb
UNO_SERVICES=\${ORIGIN}/services.rdb
CONFIGURATION_LAYERS=xcsxcu:\${ORIGIN}/../share/registry
UserInstallation=\${ORIGIN}/../user
")
    file(WRITE "${output_file}" "${_content}")
endfunction ()
