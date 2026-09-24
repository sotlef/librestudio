# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# utl (module unotools) — the utility layer above comphelper: configuration
# access, paths, streams, locales, the config item machinery.  Everything
# further up the tree sits on it, which is why it belongs in the first tier
# above the UNO core.
#
# Source list from Library_utl.mk.  Not ported from there: the precompiled
# header, and the link against tl — that module has no headers left in this
# tree, so nothing can come of it.

set(UTL_SOURCES
    unotools/source/accessibility/accessiblerelationsethelper
    unotools/source/config/bootstrap
    unotools/source/config/cmdoptions
    unotools/source/config/compatibility
    unotools/source/config/configitem
    unotools/source/config/configmgr
    unotools/source/config/confignode
    unotools/source/config/configpaths
    unotools/source/config/configvaluecontainer
    unotools/source/config/defaultoptions
    unotools/source/config/docinfohelper
    unotools/source/config/dynamicmenuoptions
    unotools/source/config/eventcfg
    unotools/source/config/fontcfg
    unotools/source/config/historyoptions
    unotools/source/config/itemholder1
    unotools/source/config/lingucfg
    unotools/source/config/moduleoptions
    unotools/source/config/options
    unotools/source/config/optionsdlg
    unotools/source/config/pathoptions
    unotools/source/config/saveopt
    unotools/source/config/searchopt
    unotools/source/config/securityoptions
    unotools/source/config/syslocaleoptions
    unotools/source/config/useroptions
    unotools/source/i18n/calendarwrapper
    unotools/source/i18n/caserotate
    unotools/source/i18n/charclass
    unotools/source/i18n/collatorwrapper
    unotools/source/i18n/intlwrapper
    unotools/source/i18n/localedatawrapper
    unotools/source/i18n/nativenumberwrapper
    unotools/source/i18n/resmgr
    unotools/source/i18n/textsearch
    unotools/source/i18n/transliterationwrapper
    unotools/source/misc/closeveto
    unotools/source/misc/datetime
    unotools/source/misc/defaultencoding
    unotools/source/misc/desktopterminationobserver
    unotools/source/misc/eventlisteneradapter
    unotools/source/misc/fontcvt
    unotools/source/misc/fontdefs
    unotools/source/misc/mediadescriptor
    unotools/source/misc/ServiceDocumenter
    unotools/source/misc/sharedunocomponent
    unotools/source/misc/syslocale
    unotools/source/misc/VersionConfig
    unotools/source/misc/wincodepage
    unotools/source/misc/ZipPackageHelper
    unotools/source/streaming/streamwrap
    unotools/source/ucbhelper/localfilehelper
    unotools/source/ucbhelper/progresshandlerwrap
    unotools/source/ucbhelper/tempfile
    unotools/source/ucbhelper/ucbhelper
    unotools/source/ucbhelper/ucblockbytes
    unotools/source/ucbhelper/ucbstreamhelper
    unotools/source/ucbhelper/xtempfile
)

librestudio_expand_cxx(UTL_SOURCES)

add_library(utl SHARED ${UTL_SOURCES})

# gb_Library_use_sdk_api plus gb_Library_use_custom_headers(... officecfg/registry)
target_include_directories(utl PRIVATE
    "${CMAKE_SOURCE_DIR}/unotools/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(utl officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(utl PRIVATE UNOTOOLS_DLLIMPLEMENTATION)

target_link_libraries(utl PRIVATE
    comphelper cppu cppuhelper i18nlangtag sal salhelper tl ucbhelper
    Boost::boost Boost::locale)

librestudio_add_component(utl
    COMPONENT "${CMAKE_SOURCE_DIR}/unotools/util/utl.component"
    LIBRARY   utl
    LAYER     OOO)

librestudio_set_library_output_name(utl utl OOO)

install(TARGETS utl LIBRARY DESTINATION program)
