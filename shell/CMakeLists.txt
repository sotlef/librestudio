# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# shell — small desktop-integration helpers, each gated in Module_shell.mk by
# its own platform/feature condition. Source lists from Library_desktopbe.mk,
# Library_localebe.mk, Library_losessioninstall.mk, StaticLibrary_xmlparser.mk,
# Library_syssh.mk, Executable_uri_encode.mk and Library_cmdmail.mk.
#
# Module_shell.mk's targets, and how each is decided for this Linux-only port:
#  - Library_desktopbe (desktopbe1): `$(if $(filter-out MACOSX WNT,$(OS)),...)`
#    -- true on Linux, ported unconditionally below.
#  - Library_localebe (localebe1): unconditional in the .mk, ported below
#    (its MACOSX-only CoreFoundation/CoreServices frameworks branch is
#    omitted, same as every other Windows/macOS-only branch in this port).
#  - Library_losessioninstall: gated `ifneq ($(filter ... LINUX ...,$(OS)),)`
#    (true) and `ifeq ($(ENABLE_GIO),TRUE)`. ENABLE_GIO is a real tracked
#    switch in this port (cmake/LibreStudioConfigHeaders.cmake, derived from
#    `pkg_check_modules(GIO_FEATURE gio-2.0>=2.26)`); on this machine gio-2.0
#    is 2.84.4, so ENABLE_GIO is 1 and the target is ported below, wrapped in
#    `if (ENABLE_GIO)` so a build without it simply drops the target (same
#    style as vcl/CMakeLists.txt's ENABLE_GEN/ENABLE_CUPS/... guards).
#  - Library_kf5be (kf5be1): gated `ifeq ($(ENABLE_KF5),TRUE)`. Unlike
#    ENABLE_GIO, this port has no ENABLE_KF5 switch at all -- ENABLE_GIO is
#    the only optional-backend switch cmake/LibreStudioConfigHeaders.cmake
#    derives from a pkg-config probe. Qt5/KF5 are also not present on this
#    machine (`pkg-config --exists Qt5Core` fails, and no KF5*.pc exists
#    anywhere under the system pkg-config search path). Excluded entirely --
#    not even referenced behind a guard, since there is nothing in the port's
#    config surface to guard it with.
#  - Windows-only targets (Executable_regsvrex/senddoc/spsupp_helper,
#    Library_smplmail/wininetbe/jumplist, the COM=MSC shlxthdl/propertyhdl/
#    ooofilt/spsupp_x86_x64 cluster) and Library_macbe (MACOSX-only): skipped,
#    same as every other Windows/macOS-only branch in this port.
#  - `ifeq ($(filter DESKTOP,$(BUILD_TYPE)),DESKTOP)`: Library_syssh,
#    StaticLibrary_xmlparser, Executable_uri_encode and Library_cmdmail (the
#    last three further gated `ifneq ($(OS),WNT)`, true here). This port's
#    baseline build type is DESKTOP -- vcl/CMakeLists.txt already establishes
#    `LIBRESTUDIO_BUILD_TYPE "DESKTOP" CACHE STRING ...` as the default, and
#    jvmfwk/CMakeLists.txt's javaldx and svl/CMakeLists.txt's documentlockfile/
#    msodocumentlockfile already treat a bare `filter DESKTOP,$(BUILD_TYPE)`
#    guard as unconditionally true for exactly this reason. All four targets
#    below follow that same, already-established precedent and are ported
#    unconditionally (their sources are unconditional C++/C and every
#    dependency they name is already ported in this tree).
#  - AllLangMoTarget_shell: localization (.mo) install, not a build target.
#  - CppunitTest_shell_zip: COM=MSC-only test, not applicable.

# --- desktopbe1 --------------------------------------------------------------

set(DESKTOPBE1_SOURCES
    shell/source/backends/desktopbe/desktopbackend
)

librestudio_expand_cxx(DESKTOPBE1_SOURCES)
add_library(desktopbe1 SHARED ${DESKTOPBE1_SOURCES})

target_include_directories(desktopbe1 PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(desktopbe1 udkapi_normal_headers offapi_normal_headers)

target_link_libraries(desktopbe1 PRIVATE
    comphelper cppu cppuhelper sal vcl)

librestudio_add_component(desktopbe1
    COMPONENT "${CMAKE_SOURCE_DIR}/shell/source/backends/desktopbe/desktopbe1.component"
    LIBRARY   desktopbe1
    LAYER     OOO)

librestudio_set_library_output_name(desktopbe1 desktopbe1 OOO)

install(TARGETS desktopbe1 LIBRARY DESTINATION program)

# --- localebe1 -----------------------------------------------------------------

set(LOCALEBE1_SOURCES
    shell/source/backends/localebe/localebackend
)

librestudio_expand_cxx(LOCALEBE1_SOURCES)
add_library(localebe1 SHARED ${LOCALEBE1_SOURCES})

target_include_directories(localebe1 PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(localebe1 udkapi_normal_headers offapi_normal_headers)

target_link_libraries(localebe1 PRIVATE
    cppu cppuhelper sal i18nlangtag)

librestudio_add_component(localebe1
    COMPONENT "${CMAKE_SOURCE_DIR}/shell/source/backends/localebe/localebe1.component"
    LIBRARY   localebe1
    LAYER     OOO)

librestudio_set_library_output_name(localebe1 localebe1 OOO)

install(TARGETS localebe1 LIBRARY DESTINATION program)

# --- losessioninstall --------------------------------------------------------

# ENABLE_GIO is 1 on this machine (see the module-level comment above); guard
# the whole target so a build without gio-2.0 simply omits it, same pattern
# vcl/CMakeLists.txt uses for its own optional-backend libraries.
if (ENABLE_GIO)

# IMPORTED_TARGET targets from pkg_check_modules are only visible in the
# directory scope where created and below (same note as ucb/CMakeLists.txt's
# ucpgio1 and vcl/CMakeLists.txt's own gio use), so both externals are
# re-declared here rather than reused from elsewhere.
pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)

set(LOSESSIONINSTALL_SOURCES
    shell/source/sessioninstall/SyncDbusSessionHelper
)

librestudio_expand_cxx(LOSESSIONINSTALL_SOURCES)
add_library(losessioninstall SHARED ${LOSESSIONINSTALL_SOURCES})

target_include_directories(losessioninstall PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(losessioninstall udkapi_normal_headers offapi_normal_headers)

target_link_libraries(losessioninstall PRIVATE
    comphelper cppu cppuhelper sal
    PkgConfig::DBUS PkgConfig::GIO)

librestudio_add_component(losessioninstall
    COMPONENT "${CMAKE_SOURCE_DIR}/shell/source/sessioninstall/losessioninstall.component"
    LIBRARY   losessioninstall
    LAYER     OOO)

librestudio_set_library_output_name(losessioninstall losessioninstall OOO)

install(TARGETS losessioninstall LIBRARY DESTINATION program)

endif () # ENABLE_GIO

# --- kf5be1 --------------------------------------------------------------------
#
# Excluded: see the module-level comment above. No Qt5/KF5 development
# packages are present on this machine, and this port tracks no ENABLE_KF5
# switch to gate the target behind in the first place.

# --- shell_xmlparser (StaticLibrary_xmlparser.mk) -----------------------------
#
# A tiny STATIC helper (one .cxx) that wraps expat; linked into syssh below,
# not installed on its own (no gb_*_set_componentfile / install in the .mk).

set(SHELL_XMLPARSER_SOURCES
    shell/source/all/xml_parser
)

librestudio_expand_cxx(SHELL_XMLPARSER_SOURCES)
add_library(shell_xmlparser STATIC ${SHELL_XMLPARSER_SOURCES})

target_include_directories(shell_xmlparser PRIVATE
    "${CMAKE_SOURCE_DIR}/shell/inc")

target_link_libraries(shell_xmlparser PRIVATE EXPAT::EXPAT)

# Linked into the shared library syssh below; needs PIC objects just like
# package/CMakeLists.txt's argon2 STATIC library, ported for the same reason.
set_target_properties(shell_xmlparser PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- syssh (Library_syssh.mk, OS != WNT branch) -------------------------------

set(SYSSH_SOURCES
    shell/source/unix/exec/shellexec
)

librestudio_expand_cxx(SYSSH_SOURCES)
add_library(syssh SHARED ${SYSSH_SOURCES})

target_include_directories(syssh PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(syssh udkapi_normal_headers offapi_normal_headers)

target_link_libraries(syssh PRIVATE
    cppu cppuhelper comphelper sal
    shell_xmlparser
    EXPAT::EXPAT)

librestudio_add_component(syssh
    COMPONENT "${CMAKE_SOURCE_DIR}/shell/source/unix/exec/syssh.component"
    LIBRARY   syssh
    LAYER     OOO)

librestudio_set_library_output_name(syssh syssh OOO)

install(TARGETS syssh LIBRARY DESTINATION program)

# --- uri-encode (Executable_uri_encode.mk) ------------------------------------
#
# A tiny standalone C filter (RFC 2396 percent-encoder read from stdin/written
# to stdout); no LO library dependencies at all -- same "plain add_executable"
# shape as jvmfwk/CMakeLists.txt's javaldx, the precedent for add_executable()
# already established in this port.
add_executable(uri-encode
    "${CMAKE_SOURCE_DIR}/shell/source/unix/misc/uri-encode.c")

install(TARGETS uri-encode RUNTIME DESTINATION program)

# --- cmdmail (Library_cmdmail.mk) ---------------------------------------------

set(CMDMAIL_SOURCES
    shell/source/cmdmail/cmdmailmsg
    shell/source/cmdmail/cmdmailsuppl
)

librestudio_expand_cxx(CMDMAIL_SOURCES)
add_library(cmdmail SHARED ${CMDMAIL_SOURCES})

target_include_directories(cmdmail PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(cmdmail udkapi_normal_headers offapi_normal_headers)

target_link_libraries(cmdmail PRIVATE
    comphelper cppu cppuhelper sal)

librestudio_add_component(cmdmail
    COMPONENT "${CMAKE_SOURCE_DIR}/shell/source/cmdmail/cmdmail.component"
    LIBRARY   cmdmail
    LAYER     OOO)

librestudio_set_library_output_name(cmdmail cmdmail OOO)

install(TARGETS cmdmail LIBRARY DESTINATION program)
