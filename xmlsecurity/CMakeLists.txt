# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# xmlsecurity — document digital signatures: the UI (certificate chooser,
# viewer, macro security, digital signatures dialog), the signature framework
# that drives SAX-level signing/verification, and the xmlsec1-based crypto
# glue that talks to the actual crypto backend.
#
# Two libraries, from Library_xmlsecurity.mk and Library_xsec_xmlsec.mk.
# xsec_xmlsec is defined first because xmlsecurity links it.
#
# Crypto backend: this port already has a single project-wide switch for
# this, LIBRESTUDIO_TLS_BACKEND (see cmake/config/config_crypto.h.in and
# comphelper/CMakeLists.txt, svl/CMakeLists.txt, oox/CMakeLists.txt for the
# established branching style) — "this build links one backend, not both".
# A separate ENABLE_NSS option is deliberately NOT introduced here: that
# would let LIBRESTUDIO_TLS_BACKEND=OPENSSL coexist with a stray
# ENABLE_NSS=ON, which is exactly the "two backends at once" state the
# existing switch exists to rule out.
#
# xmlsecurity needs one more condition beyond "which backend", though: its
# NSS-specific sources (source/xmlsec/nss/*, plus three files in
# Library_xmlsecurity.mk) call into xmlsec1's own NSS crypto-engine wrapper
# (xmlSecNssInit, xmlSecNssAppDefaultKeysMngrInit, ...), whose headers
# (xmlsec/nss/*.h) and implementation (libxmlsec1-nss.so) ship in a pkg-config
# module of their own, "xmlsec1-nss", separate from the base "xmlsec1".
# XMLSECURITY_HAS_NSS_BACKEND below is TRUE only when both the project has
# chosen the NSS backend *and* that module is actually present, and gates
# every NSS-specific source/define/external in both targets.
#
# LIBRESTUDIO_TLS_BACKEND defaults to "NSS" and xmlsec1-nss.pc is installed
# on this machine, so XMLSECURITY_HAS_NSS_BACKEND currently evaluates TRUE.
#
# Library_xmlsecurity.mk's own NSS gate is further nested inside
# `ifneq (,$(filter DESKTOP,$(BUILD_TYPE))...)`. This port's baseline build
# type is DESKTOP -- vcl/CMakeLists.txt establishes LIBRESTUDIO_BUILD_TYPE
# "DESKTOP" as the default, and shell/CMakeLists.txt, svl/CMakeLists.txt and
# jvmfwk/CMakeLists.txt already treat a bare `filter DESKTOP,$(BUILD_TYPE)`
# guard as unconditionally true for that reason; the same precedent applies
# here, so that half of the gate is dropped and only the NSS/xmlsec1-nss
# condition is tracked in XMLSECURITY_HAS_NSS_BACKEND.
#
# The precompiled headers each .mk sets (xmlsecurity/inc/pch/precompiled_xmlsecurity,
# precompiled_xsec_xmlsec) are not ported, same as elsewhere in this port.
#
# OpenSSL: Library_xsec_xmlsec.mk's ENABLE_OPENSSL branch (the non-NSS,
# non-WNT `else` arm) links the openssl external and nothing else — no
# exception objects, no -DXMLSEC_CRYPTO_* define. xmlsecurity's own
# SecurityEnvironment/XMLSignatureContext glue has no OpenSSL implementation
# anywhere in this tree (only source/xmlsec/nss and the WNT-only
# source/xmlsec/mscrypt directories exist); selecting OpenSSL would build a
# library that initializes no crypto engine at all. So
# XMLSECURITY_HAS_NSS_BACKEND simply stays FALSE under
# LIBRESTUDIO_TLS_BACKEND=OPENSSL — there is nothing OpenSSL-specific to gate
# in, unlike svl/comphelper/oox which have real OpenSSL implementations.
#
# What is not ported:
# - The GPGME-backed crypto backend (source/gpg/*, xsec_xmlsec.component.gpg):
#   GPGME is not wired up anywhere in this port — comphelper/CMakeLists.txt's
#   LIBRESTUDIO_ENABLE_GPGME option defaults OFF and nothing consumes it, so
#   staying consistent means omitting the gpg sources outright rather than
#   introducing a second unconsumed option here.
# - WNT/MSCrypto sources (source/xmlsec/mscrypt/*): this is a Linux-only port.
# - AllLangMoTarget_xsc (l10n .po -> .mo): out of scope, same precedent as
#   uui/CMakeLists.txt and fpicker/CMakeLists.txt excluding their own
#   equivalents. UIConfig_xmlsec (.ui packaging) is NO LONGER in this list --
#   see the "--- UIConfig_xmlsec ---" block below, added once xmlsecurity_
#   dialogs_test's own CppunitTest_deferred.cmake needed a real "xmlsec_
#   uiconfig" build-time target for its UICONFIG option to depend on.
# - Executable_pdfverify: a workben/ workbench-only CLI tool, not shipped;
#   this port has not been porting workben/-style tools elsewhere. Deferred,
#   not a hard blocker (add_executable() has plenty of precedent here, e.g.
#   idl/CMakeLists.txt, sal/CMakeLists.txt).

set(XMLSECURITY_HAS_NSS_BACKEND FALSE)
if (LIBRESTUDIO_TLS_BACKEND STREQUAL "NSS")
    pkg_check_modules(NSS REQUIRED IMPORTED_TARGET nss)
    pkg_check_modules(XMLSEC1_NSS QUIET IMPORTED_TARGET xmlsec1-nss)
    if (XMLSEC1_NSS_FOUND)
        set(XMLSECURITY_HAS_NSS_BACKEND TRUE)
    endif ()
elseif (LIBRESTUDIO_TLS_BACKEND STREQUAL "OPENSSL")
    # No OpenSSL implementation exists under xmlsecurity/source/xmlsec — see
    # the comment above. Nothing to detect or gate in on this branch.
else ()
    message(FATAL_ERROR "Unknown LIBRESTUDIO_TLS_BACKEND: ${LIBRESTUDIO_TLS_BACKEND}")
endif ()

# --- xsec_xmlsec ---------------------------------------------------------
# The xmlsec1-based crypto glue (Library_xsec_xmlsec.mk). Source list is the
# 8 common files unconditionally, plus the 10 source/xmlsec/nss/* files under
# XMLSECURITY_HAS_NSS_BACKEND. GPGME (source/gpg/*, 8 files) and WNT/MSCrypto
# (source/xmlsec/mscrypt/*, 7 files) are not ported — see above.

set(XSEC_XMLSEC_SOURCES
    xmlsecurity/source/xmlsec/biginteger
    xmlsecurity/source/xmlsec/certificateextension_certextn
    xmlsecurity/source/xmlsec/errorcallback
    xmlsecurity/source/xmlsec/saxhelper
    xmlsecurity/source/xmlsec/xmldocumentwrapper_xmlsecimpl
    xmlsecurity/source/xmlsec/xmlelementwrapper_xmlsecimpl
    xmlsecurity/source/xmlsec/xmlsec_init
    xmlsecurity/source/xmlsec/xmlstreamio
)
if (XMLSECURITY_HAS_NSS_BACKEND)
    list(APPEND XSEC_XMLSEC_SOURCES
        xmlsecurity/source/xmlsec/nss/ciphercontext
        xmlsecurity/source/xmlsec/nss/digestcontext
        xmlsecurity/source/xmlsec/nss/nssinitializer
        xmlsecurity/source/xmlsec/nss/sanextension_nssimpl
        xmlsecurity/source/xmlsec/nss/secerror
        xmlsecurity/source/xmlsec/nss/securityenvironment_nssimpl
        xmlsecurity/source/xmlsec/nss/seinitializer_nssimpl
        xmlsecurity/source/xmlsec/nss/x509certificate_nssimpl
        xmlsecurity/source/xmlsec/nss/xmlsecuritycontext_nssimpl
        xmlsecurity/source/xmlsec/nss/xmlsignature_nssimpl
    )
endif ()

librestudio_expand_cxx(XSEC_XMLSEC_SOURCES)

add_library(xsec_xmlsec SHARED ${XSEC_XMLSEC_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(xsec_xmlsec PRIVATE
        "${CMAKE_SOURCE_DIR}/xmlsecurity/inc/pch/precompiled_xsec_xmlsec.hxx")
    target_compile_definitions(xsec_xmlsec PRIVATE PCH_LEVEL=2)
endif ()

# gb_Library_set_include,xsec_xmlsec's own -I.../xmlsecurity/source/xmlsec is
# not carried over: every xsec_xmlsec source (xmlsec/*.cxx and
# xmlsec/nss/*.cxx alike) only quote-includes headers that live next to it in
# the same directory, which the compiler already finds without an explicit
# -I (verified against every #include "..." in both directories). The
# -I.../xmlsec/include for the bundled xmlsec external is likewise dropped:
# PkgConfig::XMLSEC1's IMPORTED_TARGET carries its own
# INTERFACE_INCLUDE_DIRECTORIES (-I/usr/include/xmlsec1) for the system
# package used here.
target_include_directories(xsec_xmlsec PRIVATE
    "${CMAKE_SOURCE_DIR}/xmlsecurity/inc"
    "${CMAKE_SOURCE_DIR}/xmlsecurity/source/xmlsec"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}")
add_dependencies(xsec_xmlsec officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(xsec_xmlsec PRIVATE
    XMLSEC_NO_XSLT
    XSECXMLSEC_DLLIMPLEMENTATION)
if (XMLSECURITY_HAS_NSS_BACKEND)
    target_compile_definitions(xsec_xmlsec PRIVATE XMLSEC_CRYPTO_NSS)
endif ()

target_link_libraries(xsec_xmlsec PRIVATE
    comphelper cppu cppuhelper sal svl tl xo utl
    Boost::boost
    LibXml2::LibXml2)

# Library_xsec_xmlsec.mk's own `ifeq ($(SYSTEM_XMLSEC),)` branches (bundling
# xmlsec1 from source, gb_Library_use_packages/add_libs against a
# WORKDIR-built static lib) do not apply: like every other external in this
# port, xmlsec1 is only ever consumed as the system package via pkg-config,
# never bundled/built from a tarball.
pkg_check_modules(XMLSEC1 REQUIRED IMPORTED_TARGET xmlsec1)
target_link_libraries(xsec_xmlsec PRIVATE PkgConfig::XMLSEC1)

if (XMLSECURITY_HAS_NSS_BACKEND)
    # Not PkgConfig::NSS here: the symbols xsec_xmlsec's own NSS sources call
    # (xmlSecNssInit, xmlSecNssAppDefaultKeysMngrInit,
    # xmlSecNssKeyDataX509GetKlass, xmlSecNssPKIAdoptKey, ...) are xmlsec1's
    # NSS crypto-engine wrapper functions, implemented in libxmlsec1-nss.so,
    # not in Mozilla's own libnss3.so. Linking plain PkgConfig::NSS would
    # leave those undefined at link time; PkgConfig::XMLSEC1_NSS is the
    # module that actually ships libxmlsec1-nss, and its .pc "Requires:"
    # (libxml-2.0, libxslt, nspr, nss) already pulls in raw NSS transitively,
    # same as upstream's own system-xmlsec detection
    # (configure.ac: libo_CHECK_SYSTEM_MODULE([xmlsec], [XMLSEC],
    # [xmlsec1-nss >= 1.2.35]) — the "xmlsec" external's cflags/libs come
    # from the xmlsec1-nss module even though the library is named xmlsec1).
    target_link_libraries(xsec_xmlsec PRIVATE PkgConfig::XMLSEC1_NSS)
endif ()

# gb_Library_add_componentimpls,xsec_xmlsec: nss_mscrypt (WNT-or-NSS) and nss
# (NSS only); gpg omitted (GPGME not ported).
set(XSEC_XMLSEC_COMPONENT_IMPLS "")
if (XMLSECURITY_HAS_NSS_BACKEND)
    list(APPEND XSEC_XMLSEC_COMPONENT_IMPLS
        "${CMAKE_SOURCE_DIR}/xmlsecurity/util/xsec_xmlsec.component.nss_mscrypt"
        "${CMAKE_SOURCE_DIR}/xmlsecurity/util/xsec_xmlsec.component.nss")
endif ()

librestudio_add_component(xsec_xmlsec
    COMPONENT      "${CMAKE_SOURCE_DIR}/xmlsecurity/util/xsec_xmlsec.component"
    IMPLEMENTATION ${XSEC_XMLSEC_COMPONENT_IMPLS}
    LIBRARY        xsec_xmlsec
    LAYER          OOO)

librestudio_set_library_output_name(xsec_xmlsec xsec_xmlsec OOO)

install(TARGETS xsec_xmlsec LIBRARY DESTINATION program)

# --- xmlsecurity -----------------------------------------------------------
# The signing/verification UI and framework (Library_xmlsecurity.mk). 23
# sources unconditionally, plus 3 more (the DocumentDigitalSignatures service
# and its helpers) under XMLSECURITY_HAS_NSS_BACKEND, mirroring the .mk's own
# `$(or $(ENABLE_NSS),$(filter WNT,$(OS)))` gate (WNT dropped, Linux-only
# port).

set(XMLSECURITY_SOURCES
    xmlsecurity/source/component/certificatecontainer
    xmlsecurity/source/dialogs/certificatechooser
    xmlsecurity/source/dialogs/certificateviewer
    xmlsecurity/source/dialogs/digitalsignaturesdialog
    xmlsecurity/source/dialogs/macrosecurity
    xmlsecurity/source/framework/buffernode
    xmlsecurity/source/framework/elementcollector
    xmlsecurity/source/framework/elementmark
    xmlsecurity/source/framework/saxeventkeeperimpl
    xmlsecurity/source/framework/securityengine
    xmlsecurity/source/framework/signaturecreatorimpl
    xmlsecurity/source/framework/signatureengine
    xmlsecurity/source/framework/signatureverifierimpl
    xmlsecurity/source/framework/xmlsignaturetemplateimpl
    xmlsecurity/source/helper/documentsignaturehelper
    xmlsecurity/source/helper/documentsignaturemanager
    xmlsecurity/source/helper/ooxmlsecparser
    xmlsecurity/source/helper/ooxmlsecexporter
    xmlsecurity/source/helper/pdfsignaturehelper
    xmlsecurity/source/helper/UriBindingHelper
    xmlsecurity/source/helper/xsecctl
    xmlsecurity/source/helper/xsecparser
    xmlsecurity/source/helper/xsecsign
)
if (XMLSECURITY_HAS_NSS_BACKEND)
    list(APPEND XMLSECURITY_SOURCES
        xmlsecurity/source/component/documentdigitalsignatures
        xmlsecurity/source/helper/xmlsignaturehelper
        xmlsecurity/source/helper/xsecverify
    )
endif ()

librestudio_expand_cxx(XMLSECURITY_SOURCES)

add_library(xmlsecurity SHARED ${XMLSECURITY_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(xmlsecurity PRIVATE
        "${CMAKE_SOURCE_DIR}/xmlsecurity/inc/pch/precompiled_xmlsecurity.hxx")
    target_compile_definitions(xmlsecurity PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(xmlsecurity PRIVATE
    "${CMAKE_SOURCE_DIR}/xmlsecurity/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}")
# officecfg_headers: digitalsignaturesdialog.cxx (unconditional) reads
# officecfg::Office::Common::Security::Scripting::CertMgrPath.
add_dependencies(xmlsecurity officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(xmlsecurity PRIVATE XMLSECURITY_DLLIMPLEMENTATION)
if (XMLSECURITY_HAS_NSS_BACKEND)
    target_compile_definitions(xmlsecurity PRIVATE XMLSEC_CRYPTO_NSS)
endif ()

target_link_libraries(xmlsecurity PRIVATE
    comphelper cppu cppuhelper sal sax svl sfx svt svxcore tl ucbhelper utl
    vcl xo i18nlangtag
    xsec_xmlsec
    Boost::boost
    LibXml2::LibXml2)

if (XMLSECURITY_HAS_NSS_BACKEND)
    # gb_Library_use_externals,xmlsecurity,nss3 plc4 -- the raw Mozilla NSS
    # pair collapses to the single "nss" pkg-config module, the same
    # simplification svl/comphelper/oox already use for their own NSS need.
    # (No source file here calls NSS APIs directly -- documentdigitalsignatures.cxx
    # and friends only reach NSS indirectly through xsec_xmlsec -- but the .mk
    # links it unconditionally on this gate, so it is mirrored here too.)
    target_link_libraries(xmlsecurity PRIVATE PkgConfig::NSS)
endif ()

# gb_Library_add_componentimpl,xmlsecurity,dds: only the "dds"
# (DocumentDigitalSignatures) implementation exists for this library; it is
# marked <optional/> in xmlsecurity.component and only actually provided
# under the same NSS-or-WNT gate as the sources above.
set(XMLSECURITY_COMPONENT_IMPLS "")
if (XMLSECURITY_HAS_NSS_BACKEND)
    list(APPEND XMLSECURITY_COMPONENT_IMPLS
        "${CMAKE_SOURCE_DIR}/xmlsecurity/util/xmlsecurity.component.dds")
endif ()

librestudio_add_component(xmlsecurity
    COMPONENT      "${CMAKE_SOURCE_DIR}/xmlsecurity/util/xmlsecurity.component"
    IMPLEMENTATION ${XMLSECURITY_COMPONENT_IMPLS}
    LIBRARY        xmlsecurity
    LAYER          OOO)

librestudio_set_library_output_name(xmlsecurity xmlsecurity OOO)

install(TARGETS xmlsecurity LIBRARY DESTINATION program)

# --- UIConfig_xmlsec ---------------------------------------------------------
#
# UIConfig_xmlsec.mk: 9 .ui files under xmlsecurity/uiconfig/ui packaged into
# share/config/soffice.cfg/xmlsec/ui -- same gb_UIConfig_UIConfig destination
# convention already established by UIConfig_vcl/UIConfig_svt/UIConfig_svx
# (vcl/svtools/svx CMakeLists.txt; those files' own comments have the
# gb_UIConfig_DIR_FOR derivation, not repeated here). The destination
# directory name is the UIConfig's own registered name, "xmlsec" (from the
# live UIConfig_xmlsec.mk's `$(eval $(call gb_UIConfig_UIConfig,xmlsec))`),
# NOT the module name "xmlsecurity" -- matching gb_CppunitTest_use_uiconfigs
# ,xmlsecurity_dialogs_test,xmlsec's own spelling in CppunitTest_xmlsecurity_
# dialogs_test.mk.
#
# Module_xmlsecurity.mk itself only adds UIConfig_xmlsec conditionally,
# `$(if $(ENABLE_NSS)$(ENABLE_OPENSSL),...)` -- this port's crypto backend
# switch, LIBRESTUDIO_TLS_BACKEND, always resolves to exactly one of NSS or
# OPENSSL (validated by the FATAL_ERROR near the top of this file), so this
# condition is unconditionally true here, the same "always-true filter"
# precedent this file's own header comment already applies to Module_
# xmlsecurity.mk's `$(if $(filter DESKTOP,$(BUILD_TYPE))...)` gate -- no
# variable guard is introduced for it.
#
# Previously listed as "out of scope" in this file's header comment -- that
# verdict predates xmlsecurity_dialogs_test's own CppunitTest_deferred.cmake
# (xmlsecurity/CppunitTest_deferred.cmake) needing use_uiconfigs(xmlsec)'s
# CMake equivalent, UICONFIG xmlsec (cmake/LibreStudioTest.cmake), which
# FATAL_ERRORs unless a real "xmlsec_uiconfig" build-time target already
# exists -- the same reason UIConfig_svx gained its own build-time mirror
# (svx/CMakeLists.txt's own "--- UIConfig_svx ---" comment). Added now with
# real install(FILES ...) packaging too, not only the minimal mirror, to
# match every other UIConfig_* block in this port.
#
# Verified against the live UIConfig_xmlsec.mk: exactly 9 add_uifiles
# entries, and `find xmlsecurity/uiconfig/ui -name "*.ui" | wc -l` also
# returns 9 -- every .ui file in the directory is listed, nothing extra
# needs excluding (the same verification vcl/svx/svtools's own UIConfig_*
# blocks already apply).
set(XMLSECURITY_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/certpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/certdetails.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/certgeneral.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/digitalsignaturesdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/securitylevelpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/securitytrustpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/selectcertificatedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/macrosecuritydialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/ui/viewcertdialog.ui"
)

install(FILES ${XMLSECURITY_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/xmlsec/ui)

# Real build-time mirror of the install() rule above, alongside it (not
# instead of it) -- see vcl/CMakeLists.txt's "--- UIConfig_vcl ---" block for
# the full mechanism explanation (one add_custom_command(OUTPUT ...) per
# file, each with its own make_directory before the copy, collected into one
# add_custom_target(... ALL DEPENDS ...)). xmlsecurity_dialogs_test's own
# UICONFIG xmlsec option (xmlsecurity/CppunitTest_deferred.cmake) depends on
# the "xmlsec_uiconfig" target defined here.
set(_xmlsec_uiconfig_outputs "")
foreach (_xmlsec_uiconfig_src IN LISTS XMLSECURITY_UICONFIG_UI_FILES)
    get_filename_component(_xmlsec_uiconfig_name "${_xmlsec_uiconfig_src}" NAME)
    set(_xmlsec_uiconfig_dest "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/xmlsec/ui/${_xmlsec_uiconfig_name}")
    add_custom_command(
        OUTPUT "${_xmlsec_uiconfig_dest}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/xmlsec/ui"
        COMMAND ${CMAKE_COMMAND} -E copy "${_xmlsec_uiconfig_src}" "${_xmlsec_uiconfig_dest}"
        DEPENDS "${_xmlsec_uiconfig_src}"
        COMMENT "Copying ${_xmlsec_uiconfig_name} into build-tree soffice.cfg/xmlsec/ui"
        VERBATIM)
    list(APPEND _xmlsec_uiconfig_outputs "${_xmlsec_uiconfig_dest}")
endforeach ()
unset(_xmlsec_uiconfig_src)
unset(_xmlsec_uiconfig_name)
unset(_xmlsec_uiconfig_dest)

add_custom_target(xmlsec_uiconfig ALL DEPENDS ${_xmlsec_uiconfig_outputs})
unset(_xmlsec_uiconfig_outputs)

# --- CppunitTest targets ------------------------------------------------------
#
# All 5 active CppunitTest_xmlsecurity_*.mk targets (pdfsigning, xmlsec,
# signing, signing2, dialogs_test -- CppunitTest_qa_certext is excluded, see
# below) call gb_CppunitTest_use_rdb(name, services), which needs
# program_services_rdb -- a target only defined near the very end of the
# root CMakeLists.txt, well after this module's own add_subdirectory(). So
# none of them are registered here; they are all in xmlsecurity/CppunitTest_
# deferred.cmake instead, include()'d from the root CMakeLists.txt once
# program_services_rdb exists (same pattern as scaddins_analysis, svx_unit/
# svx_gallery_test/svx_core, sfx2_misc and others -- see that file's own
# header comment for the full per-test detail).
#
# CppunitTest_qa_certext (Module_xmlsecurity.mk) is not ported: the live .mk
# marks it "# failing" and comments out its entire gb_Module_add_check_
# targets call (the leading "#" on a backslash-continued multi-line $(eval
# ...) comments out the whole logical line under GNU Make's line-splicing
# rules, not just that one physical line) -- upstream itself never builds or
# runs it either, on this tree or otherwise. Matching upstream's own choice,
# not a gap introduced by this port.

# xmlsecurity/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/xmlsecurity/CppunitTest_deferred.cmake")
