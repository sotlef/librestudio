# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# eventattacher (module evtatt) — the generic UNO XEventAttacher2
# implementation that wires script-event listeners onto arbitrary UNO
# objects via introspection. Source list from Library_evtatt.mk: a single
# translation unit. No *_DLLIMPLEMENTATION define: the .mk never calls
# gb_Library_add_defs for evtatt.
#
# gb_Library_use_api(evtatt,udkapi) only requests udkapi (not the usual
# gb_Library_use_sdk_api udkapi+offapi combo) -- confirmed by grepping its
# includes, which are all lang/beans/script/reflection (UDK-namespace)
# types, so only udkapi_normal_HEADERS_INCLUDE_DIR is wired in here.

set(EVTATT_SOURCES
    eventattacher/source/eventattacher
)

librestudio_expand_cxx(EVTATT_SOURCES)
add_library(evtatt SHARED ${EVTATT_SOURCES})

target_include_directories(evtatt PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(evtatt officecfg_headers udkapi_normal_headers)

target_link_libraries(evtatt PRIVATE
    comphelper cppuhelper cppu sal)

librestudio_add_component(evtatt
    COMPONENT "${CMAKE_SOURCE_DIR}/eventattacher/source/evtatt.component"
    LIBRARY   evtatt
    LAYER     OOO)

librestudio_set_library_output_name(evtatt evtatt OOO)

install(TARGETS evtatt LIBRARY DESTINATION program)
