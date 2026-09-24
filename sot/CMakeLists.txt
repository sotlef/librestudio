# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sot — the storage layer: the compound-document (OLE2) and ODF package
# containers, and the UNO services that expose them.  This is the first module
# that is about documents rather than about UNO itself.
#
# Source list from Library_sot.mk (the precompiled header is not ported).
# Note that the .mk links utl without declaring it in any way that makes the
# dependency obvious; libsot genuinely needs unotools, so it is listed below.

set(SOT_SOURCES
    sot/source/unoolestorage/xolesimplestorage
    sot/source/base/formats
    sot/source/base/object
    sot/source/base/exchange
    sot/source/base/filelist
    sot/source/sdstor/stg
    sot/source/sdstor/stgavl
    sot/source/sdstor/stgcache
    sot/source/sdstor/stgdir
    sot/source/sdstor/stgelem
    sot/source/sdstor/stgio
    sot/source/sdstor/stgole
    sot/source/sdstor/stgstrms
    sot/source/sdstor/storage
    sot/source/sdstor/storinfo
    sot/source/sdstor/ucbstorage)

librestudio_expand_cxx(SOT_SOURCES)

add_library(sot SHARED ${SOT_SOURCES})

target_include_directories(sot PRIVATE
    "${CMAKE_SOURCE_DIR}/sot/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(sot udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(sot PRIVATE SOT_DLLIMPLEMENTATION)

target_link_libraries(sot PRIVATE
    comphelper cppu cppuhelper i18nlangtag sal salhelper tl ucbhelper utl)

librestudio_add_component(sot
    COMPONENT "${CMAKE_SOURCE_DIR}/sot/util/sot.component"
    LIBRARY   sot
    LAYER     OOO)

librestudio_set_library_output_name(sot sot OOO)

install(TARGETS sot LIBRARY DESTINATION program)
