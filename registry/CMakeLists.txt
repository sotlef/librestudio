# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# registry — reading and writing of the binary .rdb type and service registries,
# on top of store.  Source list from Library_reg.mk; the library is called "reg"
# because that is the name the rest of the tree links against.

set(REG_SOURCES
    registry/source/keyimpl
    registry/source/reflread
    registry/source/reflwrit
    registry/source/regimpl
    registry/source/registry
    registry/source/regkey)

librestudio_expand_cxx(REG_SOURCES)

add_library(reg SHARED ${REG_SOURCES})

target_include_directories(reg PRIVATE "${CMAKE_SOURCE_DIR}/registry/inc")
target_compile_definitions(reg PRIVATE REG_DLLIMPLEMENTATION)
target_link_libraries(reg PRIVATE sal store)

librestudio_set_library_output_name(reg reg OOO)

install(TARGETS reg LIBRARY DESTINATION program)
