# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# unoidl — the UNO IDL type registry: a reader/writer for .rdb type registries
# and the provider that turns a tree of .idl files into one.
#
# This is the first half of the toolchain the rest of the build depends on: the
# sources here deliberately do not include any com/sun/star headers, so the
# library and the tools below it can be built before any C++ bindings exist.

find_package(BISON 3.0 REQUIRED)
find_package(FLEX 2.6 REQUIRED)

set(UNOIDL_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

# The parser and scanner are C++ and are wired together by name: the grammar
# emits sourceprovider-parser.hxx, which the checked-in
# sourceprovider-scanner.hxx and the scanner itself include.  Upstream generates
# them into workdir/YaccTarget and workdir/LexTarget (solenv/gbuild/LinkTarget.mk);
# the file names are what matters, not the directory.
bison_target(SourceProviderParser
    "${CMAKE_SOURCE_DIR}/unoidl/source/sourceprovider-parser.y"
    "${UNOIDL_GENERATED_DIR}/sourceprovider-parser.cxx"
    DEFINES_FILE "${UNOIDL_GENERATED_DIR}/sourceprovider-parser.hxx")

flex_target(SourceProviderScanner
    "${CMAKE_SOURCE_DIR}/unoidl/source/sourceprovider-scanner.l"
    "${UNOIDL_GENERATED_DIR}/sourceprovider-scanner.cxx")

set(UNOIDL_SOURCES
    unoidl/source/sourcefileprovider
    unoidl/source/sourcetreeprovider
    unoidl/source/unoidl
    unoidl/source/unoidlprovider
    unoidl/source/legacyprovider)

librestudio_expand_cxx(UNOIDL_SOURCES)

add_library(unoidl SHARED
    ${UNOIDL_SOURCES}
    ${BISON_SourceProviderParser_OUTPUTS}
    ${FLEX_SourceProviderScanner_OUTPUTS})

target_include_directories(unoidl PRIVATE
    "${CMAKE_SOURCE_DIR}/unoidl/source"
    "${UNOIDL_GENERATED_DIR}")

target_compile_definitions(unoidl PRIVATE LO_DLLIMPLEMENTATION_UNOIDL)
target_link_libraries(unoidl PRIVATE reg sal salhelper)

librestudio_set_library_output_name(unoidl unoidl OOO)

# --- tools -------------------------------------------------------------------

# unoidl-write turns a tree of .idl into an .rdb; every generated C++ binding in
# the build descends from its output.  unoidl-check validates an .rdb against a
# reference, unoidl-read dumps one — both are useful for checking the port.
add_executable(unoidl-write "${CMAKE_SOURCE_DIR}/unoidl/source/unoidl-write.cxx")
target_link_libraries(unoidl-write PRIVATE unoidl salhelper sal)

add_executable(unoidl-check "${CMAKE_SOURCE_DIR}/unoidl/source/unoidl-check.cxx")
target_link_libraries(unoidl-check PRIVATE unoidl salhelper sal)

add_executable(unoidl-read "${CMAKE_SOURCE_DIR}/unoidl/source/unoidl-read.cxx")
target_link_libraries(unoidl-read PRIVATE unoidl salhelper sal)

install(TARGETS unoidl unoidl-write unoidl-check unoidl-read
    LIBRARY DESTINATION program
    RUNTIME DESTINATION program)
