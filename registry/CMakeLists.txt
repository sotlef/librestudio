# This file is part of the LibreStudio project.
#
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

# registry_helper — StaticLibrary_registry_helper.mk.  Just the file: URL
# parsing regview's command line needs; kept as its own tiny static library
# upstream, so this port follows the same shape.
add_library(registry_helper STATIC
    "${CMAKE_SOURCE_DIR}/registry/tools/fileurl.cxx")

target_include_directories(registry_helper PRIVATE
    "${CMAKE_SOURCE_DIR}/registry/inc")

# regview — Executable_regview.mk, the command-line .rdb dump tool.
#
# Executable_regview.mk's use_libraries is:
#   reg $(if $(filter TRUE,$(DISABLE_DYNLOADING)),store)
#       $(if $(filter TRUE,$(DISABLE_DYNLOADING)),salhelper) sal
# DISABLE_DYNLOADING is not defined anywhere in this CMake port (confirmed by
# grep), so, as codemaker/CMakeLists.txt's javamaker does for the same
# construct, the DISABLE_DYNLOADING-only extras (store, salhelper) are
# omitted: they come in transitively through reg's own link interface in the
# builds where they matter, and reg already links them here (see above).
add_executable(regview
    "${CMAKE_SOURCE_DIR}/registry/tools/regview.cxx")

target_include_directories(regview PRIVATE
    "${CMAKE_SOURCE_DIR}/registry/inc")

target_link_libraries(regview PRIVATE
    registry_helper
    reg
    sal)

install(TARGETS regview RUNTIME DESTINATION program)
