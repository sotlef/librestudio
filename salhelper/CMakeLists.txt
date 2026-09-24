# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# salhelper — small helpers on top of sal: reference counting, threads, timers,
# dynamic loading.  Source list from Library_salhelper.mk.

set(SALHELPER_SOURCES
    salhelper/source/condition
    salhelper/source/dynload
    salhelper/source/simplereferenceobject
    salhelper/source/thread
    salhelper/source/timer)

librestudio_expand_cxx(SALHELPER_SOURCES)

add_library(salhelper SHARED ${SALHELPER_SOURCES})

target_compile_definitions(salhelper PRIVATE SALHELPER_DLLIMPLEMENTATION)
target_link_libraries(salhelper PRIVATE sal)

target_link_options(salhelper PRIVATE
    "-Wl,--version-script=${CMAKE_SOURCE_DIR}/salhelper/source/gcc3.map")

librestudio_set_library_output_name(salhelper salhelper RT)

install(TARGETS salhelper LIBRARY DESTINATION program)
