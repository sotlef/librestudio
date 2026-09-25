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
