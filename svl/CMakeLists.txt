# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# svl — the shared "Sfx" value layer: item pools and items, number formats,
# collation-adjacent configuration, the broadcast/listener plumbing, undo, and
# the lock file and URI helpers.  Source lists from Library_svl.mk and its two
# siblings Library_fsstorage.mk and Library_passwordcontainer.mk.

# --- zxcvbn-c -----------------------------------------------------------------
#
# svl/source/misc/PasswordHelper.cxx measures password strength with zxcvbn, and
# the library is not in this tree.  Unlike the other tarball dependencies it is
# built, not just included, and building it takes two steps: dict-generate.cpp
# compiles into a host tool, the tool turns six word lists into dict-src.h, and
# zxcvbn.c compiles against that.
#
# Upstream splits these the same way — gb_ExternalProject_zxcvbn-c runs
# `make dict-src.h` as a target of its own, and Library_svl's StaticLibrary
# compiles only zxcvbn.c.  dict-crc.h, which the upstream tarball also knows how
# to generate, belongs to the variant that reads its dictionary from a file at
# run time and is not what this build wants.

set(ZXCVBN_C_DIR "${CMAKE_CURRENT_BINARY_DIR}/zxcvbn-c")

add_executable(zxcvbn_dictgen "${zxcvbn_c_SOURCE_DIR}/dict-generate.cpp")
target_compile_features(zxcvbn_dictgen PRIVATE cxx_std_11)

set(ZXCVBN_WORDS
    words-eng_wiki.txt
    words-female.txt
    words-male.txt
    words-passwd.txt
    words-surname.txt
    words-tv_film.txt)
set(_zxcvbnWordPaths "")
foreach (_word IN LISTS ZXCVBN_WORDS)
    list(APPEND _zxcvbnWordPaths "${zxcvbn_c_SOURCE_DIR}/${_word}")
endforeach ()

add_custom_command(
    OUTPUT "${ZXCVBN_C_DIR}/dict-src.h"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ZXCVBN_C_DIR}"
    COMMAND zxcvbn_dictgen -o "${ZXCVBN_C_DIR}/dict-src.h" ${_zxcvbnWordPaths}
    DEPENDS zxcvbn_dictgen ${_zxcvbnWordPaths}
    COMMENT "Generating dict-src.h"
    VERBATIM)

# dict-src.h is named as a source as well, even though nothing compiles it: that
# is what makes the generation below run before zxcvbn.c is compiled.
add_library(zxcvbn-c STATIC
    "${zxcvbn_c_SOURCE_DIR}/zxcvbn.c"
    "${ZXCVBN_C_DIR}/dict-src.h")
# dict-src.h is quoted as "dict-src.h" by zxcvbn.c, and zxcvbn.h likewise comes
# from the unpacked tarball.
target_include_directories(zxcvbn-c PRIVATE
    "${ZXCVBN_C_DIR}" "${zxcvbn_c_SOURCE_DIR}")
target_link_libraries(zxcvbn-c PRIVATE m)
# gb_StaticLibrary_set_warnings_disabled: it is not our source and it is not
# warning-clean under -Wall -Wextra.
target_compile_options(zxcvbn-c PRIVATE -w)
set_target_properties(zxcvbn-c PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- svl ---------------------------------------------------------------------

# The crypto backend is chosen by LIBRESTUDIO_TLS_BACKEND, the same switch
# comphelper's crypto is built with; svl/source/crypto adds document signing on
# top of it and links whatever that backend is.
if (LIBRESTUDIO_TLS_BACKEND STREQUAL "NSS")
    set(SVL_HAS_NSS TRUE)
elseif (LIBRESTUDIO_TLS_BACKEND STREQUAL "OPENSSL")
    set(SVL_HAS_NSS FALSE)
else ()
    message(FATAL_ERROR "Unknown LIBRESTUDIO_TLS_BACKEND: ${LIBRESTUDIO_TLS_BACKEND}")
endif ()

set(SVL_SOURCES
    svl/source/config/asiancfg
    svl/source/config/cjkoptions
    svl/source/config/ctloptions
    svl/source/config/itemholder2
    svl/source/config/languageoptions
    svl/source/crypto/cryptosign
    svl/source/filepicker/pickerhistory
    svl/source/items/cenumitm
    svl/source/items/cintitem
    svl/source/items/flagitem
    svl/source/items/globalnameitem
    svl/source/items/globalpool
    svl/source/items/grabbagitem
    svl/source/items/ilstitem
    svl/source/items/imageitm
    svl/source/items/intitem
    svl/source/items/int64item
    svl/source/items/itemiter
    svl/source/items/itempool
    svl/source/items/itemprop
    svl/source/items/IndexedStyleSheets
    svl/source/items/itemset
    svl/source/items/lckbitem
    svl/source/items/macitem
    svl/source/items/poolitem
    svl/source/items/ptitem
    svl/source/items/rectitem
    svl/source/items/rngitem
    svl/source/items/sitem
    svl/source/items/slstitm
    svl/source/items/srchitem
    svl/source/items/stritem
    svl/source/items/style
    svl/source/items/stylepool
    svl/source/items/visitem
    svl/source/items/voiditem
    svl/source/items/whiter
    svl/source/misc/PasswordHelper
    svl/source/misc/adrparse
    # Library_svl.mk guards these two with $(if $(filter DESKTOP,$(BUILD_TYPE)),
    # ...); this build is a desktop one, which is what BUILD_TYPE means by
    # default upstream.
    svl/source/misc/documentlockfile
    svl/source/misc/msodocumentlockfile
    svl/source/misc/filenotation
    svl/source/misc/fstathelper
    svl/source/misc/getstringresource
    svl/source/misc/gridprinter
    svl/source/misc/inethist
    svl/source/misc/inettype
    svl/source/misc/lngmisc
    svl/source/misc/lockfilecommon
    svl/source/misc/ownlist
    svl/source/misc/sharecontrolfile
    svl/source/misc/sharedstring
    svl/source/misc/sharedstringpool
    svl/source/misc/strmadpt
    svl/source/misc/urihelper
    svl/source/notify/SfxBroadcaster
    svl/source/notify/broadcast
    svl/source/notify/listener
    svl/source/notify/lstner
    svl/source/numbers/currencytable
    svl/source/numbers/numfmuno
    svl/source/numbers/numuno
    svl/source/numbers/supservs
    svl/source/numbers/zforfind
    svl/source/numbers/zforlist
    svl/source/numbers/zformat
    svl/source/numbers/zforscan
    svl/source/svsql/converter
    svl/source/undo/undo
    svl/source/uno/pathservice
    # The DDE client is Windows-only; elsewhere svl ships a stub, which is the
    # branch of the ifeq in Library_svl.mk that applies here.
    svl/unx/source/svdde/ddedummy
)

librestudio_expand_cxx(SVL_SOURCES)
add_library(svl SHARED ${SVL_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(svl PRIVATE
        "${CMAKE_SOURCE_DIR}/svl/inc/pch/precompiled_svl.hxx")
    target_compile_definitions(svl PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(svl PRIVATE
    "${CMAKE_SOURCE_DIR}/svl/source/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    # MDDS_INCLUDE_DIR for the grid printer, FAST_FLOAT_INCLUDE_DIR for
    # zforfind's number parsing, and the tarball root for zxcvbn.h.
    "${MDDS_INCLUDE_DIR}"
    "${FAST_FLOAT_INCLUDE_DIR}"
    "${zxcvbn_c_SOURCE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(svl officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(svl PRIVATE SVL_DLLIMPLEMENTATION)
target_link_libraries(svl PRIVATE
    zxcvbn-c
    basegfx comphelper cppu cppuhelper i18nlangtag i18nutil sal salhelper sot tl
    ucbhelper utl
    Boost::boost
    CURL::libcurl
    ICU::uc ICU::i18n
    LibXml2::LibXml2)

# Imported targets from pkg_check_modules are directory-scoped, so the backend
# comphelper picked has to be asked for again here rather than reused.
if (SVL_HAS_NSS)
    pkg_check_modules(NSS REQUIRED IMPORTED_TARGET nss)
    target_link_libraries(svl PRIVATE PkgConfig::NSS)
else ()
    pkg_check_modules(OPENSSL REQUIRED IMPORTED_TARGET openssl)
    target_link_libraries(svl PRIVATE PkgConfig::OPENSSL)
endif ()

# Library_svl.mk: gb_Library_use_system_win32_libs(svl, advapi32 crypt32
# gdi32 gdiplus imm32 mpr ole32 shell32 usp10 uuid version winspool setupapi
# shlwapi). Kept structurally present, behind if (WIN32), per this port's
# policy of not silently dropping Windows-only gbuild targets -- see
# sal/CMakeLists.txt's sal_comtools for the same pattern. Inert on this
# Linux build.
if (WIN32)
    target_link_libraries(svl PRIVATE
        advapi32 crypt32 gdi32 gdiplus imm32 mpr ole32 shell32 usp10 uuid
        version winspool setupapi shlwapi)
endif ()

librestudio_add_component(svl
    COMPONENT "${CMAKE_SOURCE_DIR}/svl/util/svl.component"
    LIBRARY   svl
    LAYER     OOO)

librestudio_set_library_output_name(svl svl OOO)
install(TARGETS svl LIBRARY DESTINATION program)

# --- fsstorage and passwordcontainer ------------------------------------------
#
# Two small plug-ins of the same module, each with its own component file.

set(FSSTORAGE_SOURCES
    svl/source/fsstor/fsfactory
    svl/source/fsstor/fsstorage
    svl/source/fsstor/oinputstreamcontainer
    svl/source/fsstor/ostreamcontainer)
librestudio_expand_cxx(FSSTORAGE_SOURCES)
add_library(fsstorage SHARED ${FSSTORAGE_SOURCES})

target_include_directories(fsstorage PRIVATE
    "${CMAKE_SOURCE_DIR}/svl/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(fsstorage udkapi_normal_headers offapi_normal_headers)
target_link_libraries(fsstorage PRIVATE comphelper cppu cppuhelper sal tl ucbhelper utl)

librestudio_add_component(fsstorage
    COMPONENT "${CMAKE_SOURCE_DIR}/svl/source/fsstor/fsstorage.component"
    LIBRARY   fsstorage
    LAYER     OOO)

librestudio_set_library_output_name(fsstorage fsstorage OOO)
install(TARGETS fsstorage LIBRARY DESTINATION program)

set(PASSWORDCONTAINER_SOURCES
    svl/source/passwordcontainer/passwordcontainer
    svl/source/passwordcontainer/syscreds)
librestudio_expand_cxx(PASSWORDCONTAINER_SOURCES)
add_library(passwordcontainer SHARED ${PASSWORDCONTAINER_SOURCES})

target_include_directories(passwordcontainer PRIVATE
    "${CMAKE_SOURCE_DIR}/svl/source/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(passwordcontainer udkapi_normal_headers offapi_normal_headers)
target_link_libraries(passwordcontainer PRIVATE comphelper cppu cppuhelper sal ucbhelper utl)

librestudio_add_component(passwordcontainer
    COMPONENT "${CMAKE_SOURCE_DIR}/svl/source/passwordcontainer/passwordcontainer.component"
    LIBRARY   passwordcontainer
    LAYER     OOO)

librestudio_set_library_output_name(passwordcontainer passwordcontainer OOO)
install(TARGETS passwordcontainer LIBRARY DESTINATION program)

# --- CppunitTest ---------------------------------------------------------------
#
# 8 CppunitTest_*.mk files exist for svl. 5 are wired directly below (no
# protectors/components/configuration, so nothing here needs to wait on a
# module add_subdirectory()'d after svl); svl_urihelper and svl_qa_cppunit
# are deferred to svl/CppunitTest_deferred.cmake (see the pointer comment
# below for why); svl_lockfiles is deferred outright (see the comment at the
# very end of this file).

# CppunitTest_svl_adrparse.mk: 1 source, use_libraries sal svl. No
# protectors/components/configuration/set_include.
librestudio_add_cppunit_test(svl_adrparse
    SOURCES
        svl/qa/unit/test_SvAddressParser
    LIBRARIES sal svl)

# CppunitTest_svl_lngmisc.mk: 1 source, use_libraries cppu cppuhelper sal
# salhelper svl, set_include -I$(SRCDIR)/svl/source/inc. No protectors/
# components/configuration.
librestudio_add_cppunit_test(svl_lngmisc
    SOURCES
        svl/qa/unit/test_lngmisc
    LIBRARIES cppu cppuhelper sal salhelper svl
    INCLUDE "${CMAKE_SOURCE_DIR}/svl/source/inc")

# CppunitTest_svl_lngmisc.mk's own `ifeq ($(OS),WNT)` block:
# gb_CppunitTest_use_system_win32_libs adds oleaut32 only under WNT. Gated
# the same way sal_comtools gates its own WNT-only system libs
# (sal/CMakeLists.txt) -- inert on this Linux build.
if (WIN32)
    target_link_libraries(svl_lngmisc PRIVATE oleaut32)
endif ()

# CppunitTest_svl_notify.mk: 1 source (notify/test_SfxBroadcaster),
# use_external(boost_headers), use_sdk_api (udkapi+offapi "normal" headers),
# use_libraries svl comphelper sal cppu cppuhelper, set_include
# -I$(SRCDIR)/svl/source/inc. No protectors/components/configuration.
librestudio_add_cppunit_test(svl_notify
    SOURCES
        svl/qa/unit/notify/test_SfxBroadcaster
    LIBRARIES svl comphelper sal cppu cppuhelper Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/svl/source/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers)

# CppunitTest_svl_inetcontenttype.mk: 1 source, use_api(udkapi) -- verified
# it really is the plain gb_CppunitTest_use_api, not use_sdk_api, and it
# names only udkapi, no offapi -- mapped to the same "normal" udkapi headers
# use_sdk_api uses (cppu_typelib, cppu/CMakeLists.txt, is the identical
# use_api(udkapi)-alone precedent), just without offapi.
# use_externals(boost_headers), use_libraries sal svl tl. No protectors/
# components/configuration/set_include.
librestudio_add_cppunit_test(svl_inetcontenttype
    SOURCES
        svl/qa/unit/test_INetContentType
    LIBRARIES sal svl tl Boost::boost
    INCLUDE "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers)

# CppunitTest_svl_items.mk: 1 source (items/test_IndexedStyleSheets),
# use_external(boost_headers), use_sdk_api, use_libraries svl comphelper sal
# salhelper cppu cppuhelper. No protectors/components/configuration/
# set_include.
librestudio_add_cppunit_test(svl_items
    SOURCES
        svl/qa/unit/items/test_IndexedStyleSheets
    LIBRARIES svl comphelper sal salhelper cppu cppuhelper Boost::boost
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers)

# CppunitTest_svl_urihelper.mk and CppunitTest_svl_qa_cppunit.mk: registered
# in svl/CppunitTest_deferred.cmake, NOT here, include()'d from the root
# CMakeLists.txt only after ucb/unotest/test (svl_urihelper's own
# dependencies) and configmgr/framework/lingucomponent (svl_qa_cppunit's, the
# later-binding constraint of the two, so both are include()'d together at
# that later point rather than splitting into two include() sites for a
# one-test-earlier difference) are all add_subdirectory()'d. Both call
# gb_CppunitTest_use_ure (2 protector pairs, from unotest/CMakeLists.txt,
# add_subdirectory()'d well after svl here) and gb_CppunitTest_use_components
# naming modules add_subdirectory()'d after svl too (ucb's ucb1/ucpfile1 for
# svl_urihelper; configmgr, framework's fwk, and lingucomponent's numbertext
# for svl_qa_cppunit, on top of i18npool, which IS already available at
# svl's own position, add_subdirectory()'d before svl). Defining either here
# would hit librestudio_add_cppunit_test()'s COMPONENTS lookup (a GLOBAL
# property read eagerly via get_property(GLOBAL ...)) or its
# cppunittester/protector TARGET checks before those modules have run, and
# abort configure with its own FATAL_ERROR -- exactly the shape of problem
# comphelper_test/sax_parser/sax_xmlimport/configmgr_unit already hit (see
# comphelper/CppunitTest_deferred.cmake's header comment, the closest
# precedent). See svl/CppunitTest_deferred.cmake for the full per-test detail
# and its include() call site in the root CMakeLists.txt for why it sits
# where it does.

# CppunitTest_svl_lockfiles.mk is now ported for real -- see svl/
# CppunitTest_deferred.cmake's own "svl_lockfiles" section, alongside
# svl_urihelper/svl_qa_cppunit there. (Historical note: an earlier wave left
# this one deferred on the belief that "it needs svt, sw, and vcl, none of
# which are part of this port's currently-completed dependency chain" -- that
# premise went stale once svt/vcl/sw were all actually ported; the only real
# remaining issue was ordering, already solved by the same cmake_language(
# DEFER) mechanism the other two tests in that file already use.)

# svl/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/svl/CppunitTest_deferred.cmake")
