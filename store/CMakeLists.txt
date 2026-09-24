# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# store — the structured storage implementation (the "OStore" file format) that
# registry and everything above it persist into.  Source list from
# Library_store.mk.

set(STORE_SOURCES
    store/source/object
    store/source/lockbyte
    store/source/storbase
    store/source/storbios
    store/source/storcach
    store/source/stordata
    store/source/stordir
    store/source/storlckb
    store/source/stortree
    store/source/storpage
    store/source/store)

librestudio_expand_cxx(STORE_SOURCES)

add_library(store SHARED ${STORE_SOURCES})

target_compile_definitions(store PRIVATE STORE_DLLIMPLEMENTATION)
target_link_libraries(store PRIVATE sal salhelper)

librestudio_set_library_output_name(store store OOO)

install(TARGETS store LIBRARY DESTINATION program)
