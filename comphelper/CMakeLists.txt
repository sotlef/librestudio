# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# comphelper — the grab bag of helpers the UNO services are built from:
# property sets, containers, streams, accessibility plumbing, configuration
# access and the crypto backend used for document passwords.
#
# Source list from Library_comphelper.mk.  What is *not* ported from there:
# the GPGME support (disabled by default in gbuild too, via ENABLE_GPGMEPP),
# and the precompiled header.

# The crypto backend is chosen by LIBRESTUDIO_TLS_BACKEND, mirroring the
# TLS switch in the .mk: exactly one of these three files is compiled.
if (LIBRESTUDIO_TLS_BACKEND STREQUAL "NSS")
    set(COMPHELPER_CRYPTO_SOURCE comphelper/source/crypto/Crypto_NSS)
elseif (LIBRESTUDIO_TLS_BACKEND STREQUAL "OPENSSL")
    set(COMPHELPER_CRYPTO_SOURCE comphelper/source/crypto/Crypto_OpenSSL)
else ()
    message(FATAL_ERROR "Unknown LIBRESTUDIO_TLS_BACKEND: ${LIBRESTUDIO_TLS_BACKEND}")
endif ()

set(COMPHELPER_SOURCES
    comphelper/source/crypto/Crypto
    ${COMPHELPER_CRYPTO_SOURCE}
    comphelper/source/compare/AnyCompareFactory
    comphelper/source/container/container
    comphelper/source/container/containermultiplexer
    comphelper/source/container/embeddedobjectcontainer
    comphelper/source/container/enumerablemap
    comphelper/source/container/enumhelper
    comphelper/source/container/IndexedPropertyValuesContainer
    comphelper/source/container/interfacecontainer2
    comphelper/source/container/namecontainer
    comphelper/source/container/NamedPropertyValuesContainer
    comphelper/source/eventattachermgr/eventattachermgr
    comphelper/source/misc/accessibleeventnotifier
    comphelper/source/misc/AccessibleImplementationHelper
    comphelper/source/misc/accessiblekeybindinghelper
    comphelper/source/misc/accessibleselectionhelper
    comphelper/source/misc/accessibletexthelper
    comphelper/source/misc/anycompare
    comphelper/source/misc/anytohash
    comphelper/source/misc/anytostring
    comphelper/source/misc/asyncnotification
    comphelper/source/misc/asyncquithandler
    comphelper/source/misc/automationinvokedzone
    comphelper/source/misc/backupfilehelper
    comphelper/source/misc/base64
    comphelper/source/misc/compbase
    comphelper/source/misc/componentbase
    comphelper/source/misc/configuration
    comphelper/source/misc/configurationhelper
    comphelper/source/misc/date
    comphelper/source/misc/debuggerinfo
    comphelper/source/misc/diagnose_ex
    comphelper/source/misc/DirectoryHelper
    comphelper/source/misc/dispatchcommand
    comphelper/source/misc/docpasswordhelper
    comphelper/source/misc/docpasswordrequest
    comphelper/source/misc/documentinfo
    comphelper/source/misc/emscriptenthreading
    comphelper/source/misc/errcode
    comphelper/source/misc/evtlistenerhlp
    comphelper/source/misc/evtmethodhelper
    comphelper/source/misc/fileurl
    comphelper/source/misc/getexpandeduri
    comphelper/source/misc/graphicmimetype
    comphelper/source/misc/hash
    comphelper/source/misc/instancelocker
    comphelper/source/misc/interaction
    comphelper/source/misc/logging
    comphelper/source/misc/lok
    comphelper/source/misc/mimeconfighelper
    comphelper/source/misc/namedvaluecollection
    comphelper/source/misc/numberedcollection
    comphelper/source/misc/numbers
    comphelper/source/misc/OAccessible
    comphelper/source/misc/officerestartmanager
    comphelper/source/misc/random
    comphelper/source/misc/scriptbrowse
    comphelper/source/misc/SelectionMultiplex
    comphelper/source/misc/sequenceashashmap
    comphelper/source/misc/sharedmutex
    comphelper/source/misc/simplefileaccessinteraction
    comphelper/source/misc/solarmutex
    comphelper/source/misc/stillreadwriteinteraction
    comphelper/source/misc/storagehelper
    comphelper/source/misc/string
    comphelper/source/misc/synchronousdispatch
    comphelper/source/misc/syntaxhighlight
    comphelper/source/misc/threadpool
    comphelper/source/misc/traceevent
    comphelper/source/misc/types
    comphelper/source/misc/weakeventlistener
    comphelper/source/misc/xmlsechelper
    comphelper/source/officeinstdir/officeinstallationdirectories
    comphelper/source/processfactory/processfactory
    comphelper/source/property/ChainablePropertySet
    comphelper/source/property/ChainablePropertySetInfo
    comphelper/source/property/genericpropertyset
    comphelper/source/property/MasterPropertySet
    comphelper/source/property/MasterPropertySetInfo
    comphelper/source/property/opropertybag
    comphelper/source/property/propagg
    comphelper/source/property/property
    comphelper/source/property/propertybag
    comphelper/source/property/propertycontainer
    comphelper/source/property/propertycontainerhelper
    comphelper/source/property/propertysethelper
    comphelper/source/property/propertysetinfo
    comphelper/source/property/propertystatecontainer
    comphelper/source/property/propmultiplex
    comphelper/source/property/propmultiplex2
    comphelper/source/property/propshlp
    comphelper/source/property/propstate
    comphelper/source/streaming/basicio
    comphelper/source/streaming/memorystream
    comphelper/source/streaming/oslfile2streamwrap
    comphelper/source/streaming/seekableinput
    comphelper/source/streaming/seqinputstreamserv
    comphelper/source/streaming/seqoutputstreamserv
    comphelper/source/streaming/seqstream
    comphelper/source/streaming/streamsection
    comphelper/source/xml/attributelist
    comphelper/source/xml/ofopxmlhelper
    comphelper/source/xml/xmltools
)

librestudio_expand_cxx(COMPHELPER_SOURCES)

add_library(comphelper SHARED ${COMPHELPER_SOURCES})

# gb_Library_use_custom_headers(comphelper, officecfg/registry) — comphelper
# reads configuration through the generated officecfg:: types.
target_include_directories(comphelper PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}")
add_dependencies(comphelper officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(comphelper PRIVATE COMPHELPER_DLLIMPLEMENTATION)

target_link_libraries(comphelper PRIVATE
    cppu cppuhelper sal salhelper ucbhelper i18nlangtag
    ICU::uc
    ZLIB::ZLIB
    Boost::boost)

if (LIBRESTUDIO_TLS_BACKEND STREQUAL "NSS")
    pkg_check_modules(NSS REQUIRED IMPORTED_TARGET nss)
    target_link_libraries(comphelper PRIVATE PkgConfig::NSS)
else ()
    pkg_check_modules(OPENSSL REQUIRED IMPORTED_TARGET openssl)
    target_link_libraries(comphelper PRIVATE PkgConfig::OPENSSL)
endif ()

# gb_Library_set_componentfile(comphelper, comphelper/util/comphelp, services) —
# comphelper is an office-layer library, so its component uses the OOO prefix.
librestudio_add_component(comphelper
    COMPONENT "${CMAKE_SOURCE_DIR}/comphelper/util/comphelp.component"
    LIBRARY   comphelper
    LAYER     OOO)

librestudio_set_library_output_name(comphelper comphelper OOO)

install(TARGETS comphelper LIBRARY DESTINATION program)
