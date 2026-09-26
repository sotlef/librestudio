# This file is part of the LibreStudio project.
#
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

# --- store (CppunitTest) -------------------------------------------------------
#
# CppunitTest_store.mk: 1 source, use_libraries sal salhelper store.
# No use_sdk_api/use_ure/use_vcl. Note the CppunitTest shares the plain name
# "store" with the library target above -- gbuild's CppunitTest and Library
# namespaces are separate, but CMake's is not, so this would collide if the
# CppunitTest target were also literally named "store".
librestudio_add_cppunit_test(store_test
    SOURCES
        store/qa/store
    LIBRARIES sal salhelper store)
