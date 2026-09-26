# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# ucbhelper — the helpers for implementing Universal Content Broker providers,
# and the result-set machinery built on top of them.  comphelper links it, which
# is how it enters the UNO core's closure.
# Source list from Library_ucbhelper.mk.

set(UCBHELPER_SOURCES
    ucbhelper/source/client/activedatasink
    ucbhelper/source/client/activedatastreamer
    ucbhelper/source/client/commandenvironment
    ucbhelper/source/client/content
    ucbhelper/source/client/interceptedinteraction
    ucbhelper/source/client/proxydecider
    ucbhelper/source/provider/authenticationfallback
    ucbhelper/source/provider/cancelcommandexecution
    ucbhelper/source/provider/contenthelper
    ucbhelper/source/provider/contentidentifier
    ucbhelper/source/provider/contentinfo
    ucbhelper/source/provider/interactionrequest
    ucbhelper/source/provider/propertyvalueset
    ucbhelper/source/provider/providerhelper
    ucbhelper/source/provider/registerucb
    ucbhelper/source/provider/resultset
    ucbhelper/source/provider/resultsethelper
    ucbhelper/source/provider/resultsetmetadata
    ucbhelper/source/provider/simpleauthenticationrequest
    ucbhelper/source/provider/simpleinteractionrequest
    ucbhelper/source/provider/simpleioerrorrequest
    ucbhelper/source/provider/simplenameclashresolverequest)

librestudio_expand_cxx(UCBHELPER_SOURCES)

add_library(ucbhelper SHARED ${UCBHELPER_SOURCES})

# gb_Library_use_sdk_api: normal bindings of udkapi and offapi.
target_include_directories(ucbhelper PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(ucbhelper udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(ucbhelper PRIVATE UCBHELPER_DLLIMPLEMENTATION)
target_link_libraries(ucbhelper PRIVATE cppu cppuhelper sal salhelper)

# Library_ucbhelper.mk: gb_Library_use_system_win32_libs,ucbhelper,Winhttp --
# there is no explicit ifeq ($(OS),WNT) around this call in the .mk itself,
# but gb_Library_use_system_win32_libs is only defined in the MSVC-class
# platform file (solenv/gbuild/platform/com_MSC_class.mk); on every other
# platform file it does not exist at all, i.e. it is a no-op by omission
# rather than by an explicit guard. The effect is Windows-only, so it is
# ported here behind an explicit if (WIN32).
if (WIN32)
    target_link_libraries(ucbhelper PRIVATE Winhttp)
endif ()

librestudio_set_library_output_name(ucbhelper ucbhelper OOO)

install(TARGETS ucbhelper LIBRARY DESTINATION program)
