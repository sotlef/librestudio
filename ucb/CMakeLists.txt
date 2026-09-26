# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# ucb (module ucb) — the Universal Content Broker: the core content-provider
# registry/dispatch library (ucb1) plus its content-provider (UCP) plugins.
# The module has 13 Library_*.mk files; 12 are ported here (all their
# gb_Library_use_libraries/use_externals dependencies are already available in
# this tree). Only one is skipped -- see the comment by ucpcmis1 below. The
# other library the task guidance flagged as a likely skip, ucpgio1, turned
# out to be portable -- see its comment further down for why:
#
#  * ucpcmis1 (CMIS/SharePoint content provider) needs the libcmis external.
#    Only the runtime shared library is installed on this system
#    (libcmis-0.6.2-alt1: /usr/lib64/libcmis-0.6.so.6, libcmis-c-0.6.so.6) --
#    there is no libcmis .pc file anywhere on the system (pkg-config
#    --exists libcmis-0.5 fails, and nothing under /usr/lib*/pkgconfig or
#    /usr/share/pkgconfig mentions cmis) and no libcmis headers under
#    /usr/include (find turns up nothing). Without a devel package there is
#    nothing to add_library/target_link against or #include, so ucpcmis1 is
#    skipped.
#
#  * ucpgio1 (gio/GVfs content provider) is ported, not skipped: gio-2.0 is
#    confirmed present via `pkg-config --exists gio-2.0` (system has 2.84.4),
#    and vcl/CMakeLists.txt already links libvcl against it unconditionally
#    (`pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)`). vcl's
#    PkgConfig::GIO target is directory-scoped to vcl/, so it is not visible
#    here; ucpgio1 below repeats the same pkg_check_modules call to get its
#    own PkgConfig::GIO, mirroring vcl's pattern rather than depending on it.
#
# Every other library (ucb1, ucphier1, ucpfile1, ucppkg1, ucpdav1, ucptdoc1,
# srtrs1, ucpext, ucpexpand1, ucpimage, cached1) depends only on targets
# already ported in this tree (comphelper, cppu, cppuhelper, sal, salhelper,
# tl, svl, ucbhelper, utl, vcl) plus boost_headers (header-only, no separate
# target needed) and, for ucpdav1, curl (CURL::libcurl, already wired at the
# top level).
#
# Source lists, include paths and library sets below are transcribed
# one-for-one from each Library_*.mk.

# ---------------------------------------------------------------------------
# ucb1 -- core UCB: provider registry/dispatch, identifier factory, provider
# proxy, UCB-level commands/properties, the persistent UCB store, FileAccess
# service, and the regexp helper used by content-provider matching.
# ---------------------------------------------------------------------------

set(UCB1_SOURCES
    ucb/source/core/cmdenv
    ucb/source/core/identify
    ucb/source/core/provprox
    ucb/source/core/ucb
    ucb/source/core/ucbcmds
    ucb/source/core/ucbprops
    ucb/source/core/ucbstore
    ucb/source/core/FileAccess
    ucb/source/regexp/regexp
)

librestudio_expand_cxx(UCB1_SOURCES)
add_library(ucb1 SHARED ${UCB1_SOURCES})

target_include_directories(ucb1 PRIVATE
    "${CMAKE_SOURCE_DIR}/ucb/source/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(ucb1 officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(ucb1 PRIVATE
    comphelper cppu cppuhelper sal tl ucbhelper utl
    Boost::boost)

librestudio_add_component(ucb1
    COMPONENT "${CMAKE_SOURCE_DIR}/ucb/source/core/ucb1.component"
    LIBRARY   ucb1
    LAYER     OOO)

librestudio_set_library_output_name(ucb1 ucb1 OOO)

install(TARGETS ucb1 LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# ucphier1 -- the "vnd.sun.star.hier" hierarchy content provider (used for
# in-memory tree structures such as the UCB's own root view).
# ---------------------------------------------------------------------------

set(UCPHIER1_SOURCES
    ucb/source/ucp/hierarchy/dynamicresultset
    ucb/source/ucp/hierarchy/hierarchycontentcaps
    ucb/source/ucp/hierarchy/hierarchycontent
    ucb/source/ucp/hierarchy/hierarchydata
    ucb/source/ucp/hierarchy/hierarchydatasource
    ucb/source/ucp/hierarchy/hierarchydatasupplier
    ucb/source/ucp/hierarchy/hierarchyprovider
    ucb/source/ucp/hierarchy/hierarchyuri
)

librestudio_expand_cxx(UCPHIER1_SOURCES)
add_library(ucphier1 SHARED ${UCPHIER1_SOURCES})

target_include_directories(ucphier1 PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(ucphier1 officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(ucphier1 PRIVATE
    comphelper cppu cppuhelper sal salhelper tl ucbhelper
    Boost::boost)

librestudio_add_component(ucphier1
    COMPONENT "${CMAKE_SOURCE_DIR}/ucb/source/ucp/hierarchy/ucphier1.component"
    LIBRARY   ucphier1
    LAYER     OOO)

librestudio_set_library_output_name(ucphier1 ucphier1 OOO)

install(TARGETS ucphier1 LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# ucpfile1 -- the "file" content provider (local filesystem access).
# ---------------------------------------------------------------------------

set(UCPFILE1_SOURCES
    ucb/source/ucp/file/bc
    ucb/source/ucp/file/filcmd
    ucb/source/ucp/file/filglob
    ucb/source/ucp/file/filid
    ucb/source/ucp/file/filinpstr
    ucb/source/ucp/file/filinsreq
    ucb/source/ucp/file/filnot
    ucb/source/ucp/file/filprp
    ucb/source/ucp/file/filrec
    ucb/source/ucp/file/filrow
    ucb/source/ucp/file/filrset
    ucb/source/ucp/file/filstr
    ucb/source/ucp/file/filtask
    ucb/source/ucp/file/prov
)

librestudio_expand_cxx(UCPFILE1_SOURCES)
add_library(ucpfile1 SHARED ${UCPFILE1_SOURCES})

target_include_directories(ucpfile1 PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(ucpfile1 officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(ucpfile1 PRIVATE
    comphelper cppu cppuhelper sal tl ucbhelper utl
    Boost::boost)

librestudio_add_component(ucpfile1
    COMPONENT "${CMAKE_SOURCE_DIR}/ucb/source/ucp/file/ucpfile1.component"
    LIBRARY   ucpfile1
    LAYER     OOO)

librestudio_set_library_output_name(ucpfile1 ucpfile1 OOO)

install(TARGETS ucpfile1 LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# ucppkg1 -- the "vnd.sun.star.pkg" content provider (browsing into package
# storages -- zip/ODF containers -- as a hierarchical content tree).
# ---------------------------------------------------------------------------

set(UCPPKG1_SOURCES
    ucb/source/ucp/package/pkgcontentcaps
    ucb/source/ucp/package/pkgcontent
    ucb/source/ucp/package/pkgdatasupplier
    ucb/source/ucp/package/pkgprovider
    ucb/source/ucp/package/pkgresultset
    ucb/source/ucp/package/pkguri
)

librestudio_expand_cxx(UCPPKG1_SOURCES)
add_library(ucppkg1 SHARED ${UCPPKG1_SOURCES})

target_include_directories(ucppkg1 PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(ucppkg1 officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(ucppkg1 PRIVATE
    comphelper cppu cppuhelper sal salhelper ucbhelper
    Boost::boost)

librestudio_add_component(ucppkg1
    COMPONENT "${CMAKE_SOURCE_DIR}/ucb/source/ucp/package/ucppkg1.component"
    LIBRARY   ucppkg1
    LAYER     OOO)

librestudio_set_library_output_name(ucppkg1 ucppkg1 OOO)

install(TARGETS ucppkg1 LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# ucpdav1 -- the WebDAV content provider, curl-based (ucb/source/ucp/webdav-
# curl). curl is already wired at the top level (CURL::libcurl / find_package
# (CURL REQUIRED)), so this one is portable.
# ---------------------------------------------------------------------------

set(UCPDAV1_SOURCES
    ucb/source/ucp/webdav-curl/ContentProperties
    ucb/source/ucp/webdav-curl/CurlSession
    ucb/source/ucp/webdav-curl/CurlUri
    ucb/source/ucp/webdav-curl/DAVProperties
    ucb/source/ucp/webdav-curl/DAVResourceAccess
    ucb/source/ucp/webdav-curl/DAVSessionFactory
    ucb/source/ucp/webdav-curl/DAVTypes
    ucb/source/ucp/webdav-curl/DateTimeHelper
    ucb/source/ucp/webdav-curl/PropfindCache
    ucb/source/ucp/webdav-curl/SerfLockStore
    ucb/source/ucp/webdav-curl/UCBDeadPropertyValue
    ucb/source/ucp/webdav-curl/webdavcontent
    ucb/source/ucp/webdav-curl/webdavcontentcaps
    ucb/source/ucp/webdav-curl/webdavdatasupplier
    ucb/source/ucp/webdav-curl/webdavprovider
    ucb/source/ucp/webdav-curl/webdavresponseparser
    ucb/source/ucp/webdav-curl/webdavresultset
)

librestudio_expand_cxx(UCPDAV1_SOURCES)
add_library(ucpdav1 SHARED ${UCPDAV1_SOURCES})

target_include_directories(ucpdav1 PRIVATE
    "${CMAKE_SOURCE_DIR}/ucb/source/ucp/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(ucpdav1 officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(ucpdav1 PRIVATE
    comphelper cppu cppuhelper sal salhelper svl tl ucbhelper
    CURL::libcurl)

librestudio_add_component(ucpdav1
    COMPONENT "${CMAKE_SOURCE_DIR}/ucb/source/ucp/webdav-curl/ucpdav1.component"
    LIBRARY   ucpdav1
    LAYER     OOO)

librestudio_set_library_output_name(ucpdav1 ucpdav1 OOO)

install(TARGETS ucpdav1 LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# ucpcmis1 -- CMIS/SharePoint content provider. SKIPPED: needs the libcmis
# external. Confirmed via `pkg-config --exists libcmis-0.5` (fails) and a
# system-wide search for pkgconfig/header files (`find / -iname "libcmis*"`,
# `find /usr/include -iname "*cmis*"`) -- only the runtime shared libraries
# from the distro's libcmis-0.6.2 package are present
# (/usr/lib64/libcmis-0.6.so.6, libcmis-c-0.6.so.6), with no .pc file and no
# headers anywhere on the system, so there is nothing to link a CMake target
# against or #include. It also needs boost_date_time and libxml2 on top of
# that, but libcmis alone already rules it out.
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# ucpgio1 -- gio/GVfs content provider. gio-2.0 is confirmed present
# (`pkg-config --exists gio-2.0`, system has 2.84.4) and vcl/CMakeLists.txt
# already links against it unconditionally via
# `pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)`; that target is
# scoped to vcl/'s directory, so it is re-created here rather than reused.
# ---------------------------------------------------------------------------

pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)

set(UCPGIO1_SOURCES
    ucb/source/ucp/gio/gio_content
    ucb/source/ucp/gio/gio_datasupplier
    ucb/source/ucp/gio/gio_inputstream
    ucb/source/ucp/gio/gio_mount
    ucb/source/ucp/gio/gio_outputstream
    ucb/source/ucp/gio/gio_provider
    ucb/source/ucp/gio/gio_resultset
    ucb/source/ucp/gio/gio_seekable
)

librestudio_expand_cxx(UCPGIO1_SOURCES)
add_library(ucpgio1 SHARED ${UCPGIO1_SOURCES})

target_include_directories(ucpgio1 PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(ucpgio1 officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(ucpgio1 PRIVATE
    comphelper cppu cppuhelper sal salhelper ucbhelper vcl
    Boost::boost
    PkgConfig::GIO)

librestudio_add_component(ucpgio1
    COMPONENT "${CMAKE_SOURCE_DIR}/ucb/source/ucp/gio/ucpgio.component"
    LIBRARY   ucpgio1
    LAYER     OOO)

librestudio_set_library_output_name(ucpgio1 ucpgio1 OOO)

install(TARGETS ucpgio1 LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# ucptdoc1 -- the "vnd.sun.star.tdoc" transient-document content provider
# (exposes an open document's internal storage as a UCB content tree).
# ---------------------------------------------------------------------------

set(UCPTDOC1_SOURCES
    ucb/source/ucp/tdoc/tdoc_contentcaps
    ucb/source/ucp/tdoc/tdoc_content
    ucb/source/ucp/tdoc/tdoc_datasupplier
    ucb/source/ucp/tdoc/tdoc_docmgr
    ucb/source/ucp/tdoc/tdoc_documentcontentfactory
    ucb/source/ucp/tdoc/tdoc_passwordrequest
    ucb/source/ucp/tdoc/tdoc_provider
    ucb/source/ucp/tdoc/tdoc_resultset
    ucb/source/ucp/tdoc/tdoc_stgelems
    ucb/source/ucp/tdoc/tdoc_storage
    ucb/source/ucp/tdoc/tdoc_uri
)

librestudio_expand_cxx(UCPTDOC1_SOURCES)
add_library(ucptdoc1 SHARED ${UCPTDOC1_SOURCES})

target_include_directories(ucptdoc1 PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(ucptdoc1 officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(ucptdoc1 PRIVATE
    comphelper cppu cppuhelper sal salhelper tl ucbhelper)

librestudio_add_component(ucptdoc1
    COMPONENT "${CMAKE_SOURCE_DIR}/ucb/source/ucp/tdoc/ucptdoc1.component"
    LIBRARY   ucptdoc1
    LAYER     OOO)

librestudio_set_library_output_name(ucptdoc1 ucptdoc1 OOO)

install(TARGETS ucptdoc1 LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# srtrs1 -- the sorted-result-set wrapper (wraps a dynamic result set,
# applying a sort order).
# ---------------------------------------------------------------------------

set(SRTRS1_SOURCES
    ucb/source/sorter/sortdynres
    ucb/source/sorter/sortresult
)

librestudio_expand_cxx(SRTRS1_SOURCES)
add_library(srtrs1 SHARED ${SRTRS1_SOURCES})

target_include_directories(srtrs1 PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(srtrs1 officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(srtrs1 PRIVATE
    comphelper cppu cppuhelper sal tl ucbhelper
    Boost::boost)

librestudio_add_component(srtrs1
    COMPONENT "${CMAKE_SOURCE_DIR}/ucb/source/sorter/srtrs1.component"
    LIBRARY   srtrs1
    LAYER     OOO)

librestudio_set_library_output_name(srtrs1 srtrs1 OOO)

install(TARGETS srtrs1 LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# ucpext -- the "vnd.sun.star.extension" content provider (browsing into
# installed extension packages).
# ---------------------------------------------------------------------------

set(UCPEXT_SOURCES
    ucb/source/ucp/ext/ucpext_content
    ucb/source/ucp/ext/ucpext_datasupplier
    ucb/source/ucp/ext/ucpext_provider
    ucb/source/ucp/ext/ucpext_resultset
)

librestudio_expand_cxx(UCPEXT_SOURCES)
add_library(ucpext SHARED ${UCPEXT_SOURCES})

target_include_directories(ucpext PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(ucpext officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(ucpext PRIVATE
    comphelper cppu cppuhelper sal salhelper tl ucbhelper
    Boost::boost)

librestudio_add_component(ucpext
    COMPONENT "${CMAKE_SOURCE_DIR}/ucb/source/ucp/ext/ucpext.component"
    LIBRARY   ucpext
    LAYER     OOO)

librestudio_set_library_output_name(ucpext ucpext OOO)

install(TARGETS ucpext LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# ucpexpand1 -- the "vnd.sun.star.expand" content provider (macro-expansion
# URLs, e.g. $(prog) / $(brand) style placeholders).
# ---------------------------------------------------------------------------

set(UCPEXPAND1_SOURCES
    ucb/source/ucp/expand/ucpexpand
)

librestudio_expand_cxx(UCPEXPAND1_SOURCES)
add_library(ucpexpand1 SHARED ${UCPEXPAND1_SOURCES})

target_include_directories(ucpexpand1 PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(ucpexpand1 officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(ucpexpand1 PRIVATE
    comphelper cppu cppuhelper sal tl ucbhelper)

librestudio_add_component(ucpexpand1
    COMPONENT "${CMAKE_SOURCE_DIR}/ucb/source/ucp/expand/ucpexpand1.component"
    LIBRARY   ucpexpand1
    LAYER     OOO)

librestudio_set_library_output_name(ucpexpand1 ucpexpand1 OOO)

install(TARGETS ucpexpand1 LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# ucpimage -- the "vnd.sun.star.image" content provider (deals-in image
# storage streams, e.g. for embedded-graphics access).
# ---------------------------------------------------------------------------

set(UCPIMAGE_SOURCES
    ucb/source/ucp/image/ucpimage
)

librestudio_expand_cxx(UCPIMAGE_SOURCES)
add_library(ucpimage SHARED ${UCPIMAGE_SOURCES})

target_include_directories(ucpimage PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(ucpimage officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(ucpimage PRIVATE
    comphelper cppu cppuhelper sal ucbhelper vcl
    Boost::boost)

librestudio_add_component(ucpimage
    COMPONENT "${CMAKE_SOURCE_DIR}/ucb/source/ucp/image/ucpimage.component"
    LIBRARY   ucpimage
    LAYER     OOO)

librestudio_set_library_output_name(ucpimage ucpimage OOO)

install(TARGETS ucpimage LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# cached1 -- the cached-content result-set wrappers used by content providers
# that need to cache remote directory listings locally.
# ---------------------------------------------------------------------------

set(CACHED1_SOURCES
    ucb/source/cacher/cachedcontentresultset
    ucb/source/cacher/cachedcontentresultsetstub
    ucb/source/cacher/cacheddynamicresultset
    ucb/source/cacher/cacheddynamicresultsetstub
    ucb/source/cacher/contentresultsetwrapper
    ucb/source/cacher/dynamicresultsetwrapper
)

librestudio_expand_cxx(CACHED1_SOURCES)
add_library(cached1 SHARED ${CACHED1_SOURCES})

target_include_directories(cached1 PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(cached1 officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(cached1 PRIVATE
    comphelper cppu cppuhelper sal ucbhelper
    Boost::boost)

librestudio_add_component(cached1
    COMPONENT "${CMAKE_SOURCE_DIR}/ucb/source/cacher/cached1.component"
    LIBRARY   cached1
    LAYER     OOO)

librestudio_set_library_output_name(cached1 cached1 OOO)

install(TARGETS cached1 LIBRARY DESTINATION program)
