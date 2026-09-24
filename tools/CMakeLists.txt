# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# tl — the "toolkit-lite" layer: URLs, dates, streams, colours, XML walking,
# fraction and string helpers.  The module is called tools but the library it
# produces is tl (tools/Library_tl.mk); everything above comphelper builds on
# it, and utl and sot both link it.

# bestreversemap is a build-time helper: it turns the word list compiled into it
# into a reverse-map header.  It is defined first because tl's generated source
# is produced by running it.
add_executable(bestreversemap
    "${CMAKE_SOURCE_DIR}/tools/source/reversemap/bestreversemap.cxx")
target_link_libraries(bestreversemap PRIVATE sal)

# Source list from Library_tl.mk, with its conditions resolved for a Linux
# build:
#   * stream/strmunx, not stream/strmwnt (that one is Win32 API only);
#   * misc/fix16, which the .mk includes unless the system provides libfixmath.
# The precompiled header is not ported.

set(TL_SOURCES
    tools/source/datetime/datetime
    tools/source/datetime/datetimeutils
    tools/source/datetime/duration
    tools/source/datetime/systemdatetime
    tools/source/datetime/tdate
    tools/source/datetime/ttime
    tools/source/debug/debug
    tools/source/fsys/fileutil
    tools/source/fsys/urlobj
    tools/source/fsys/wldcrd
    tools/source/generic/b3dtrans
    tools/source/generic/bigint
    tools/source/generic/color
    tools/source/generic/config
    tools/source/generic/fract
    tools/source/generic/gen
    tools/source/generic/line
    tools/source/generic/point
    tools/source/generic/poly
    tools/source/generic/poly2
    tools/source/generic/svborder
    tools/source/inet/hostfilter
    tools/source/inet/inetmime
    tools/source/inet/inetmsg
    tools/source/inet/inetstrm
    tools/source/memtools/multisel
    tools/source/misc/cpuid
    tools/source/misc/extendapplicationenvironment
    tools/source/misc/fix16
    tools/source/misc/json_writer
    tools/source/misc/lazydelete
    tools/source/misc/UniqueID
    tools/source/ref/globname
    tools/source/ref/ref
    tools/source/stream/GenericTypeSerializer
    tools/source/stream/stream
    tools/source/stream/strmunx
    tools/source/stream/vcompat
    tools/source/string/tenccvt
    tools/source/xml/XmlWalker
    tools/source/xml/XmlWriter
    tools/source/xml/XPath
    tools/source/zcodec/zcodec
)

librestudio_expand_cxx(TL_SOURCES)

# gb_Library_add_generated_exception_objects(tl, CustomTarget/tools/string/reversemap):
# bestreversemap has the word list compiled in and writes the source to stdout.
set(TL_REVERSEMAP_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/reversemap.cxx")
add_custom_command(
    OUTPUT "${TL_REVERSEMAP_SOURCE}"
    COMMAND sh -c "\"$<TARGET_FILE:bestreversemap>\" > \"${TL_REVERSEMAP_SOURCE}\""
    DEPENDS bestreversemap
    COMMENT "bestreversemap: reversemap.cxx"
    VERBATIM)

add_library(tl SHARED ${TL_SOURCES} "${TL_REVERSEMAP_SOURCE}")

target_include_directories(tl PRIVATE
    "${CMAKE_SOURCE_DIR}/tools/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}"
    "${LIBXML2_INCLUDE_DIRS}")
add_dependencies(tl udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(tl PRIVATE TOOLS_DLLIMPLEMENTATION SYSTEM_ZLIB ZLIB_CONST)

target_link_libraries(tl PRIVATE
    basegfx comphelper i18nlangtag cppu cppuhelper sal
    ICU::uc ICU::i18n ZLIB::ZLIB
    "${LIBXML2_LIBRARIES}"
    rt)

librestudio_set_library_output_name(tl tl OOO)
install(TARGETS tl bestreversemap
    LIBRARY DESTINATION program
    RUNTIME DESTINATION program)
