# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# The smoke test for the UNO core this build produces.  See
# librestudio_smoke.cxx for what it exercises and why building the libraries
# alone does not establish the same thing.

add_executable(librestudio_smoke librestudio_smoke.cxx)

target_include_directories(librestudio_smoke PRIVATE
    "${CMAKE_SOURCE_DIR}/cppuhelper/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(librestudio_smoke udkapi_normal_headers offapi_normal_headers)

target_link_libraries(librestudio_smoke PRIVATE cppuhelper cppu sal)
target_compile_definitions(librestudio_smoke PRIVATE LIBO_INTERNAL_ONLY)

install(TARGETS librestudio_smoke RUNTIME DESTINATION program)

if (BUILD_TESTING)
    # The test needs the registries and unorc in place, not just the
    # executable, so it depends on the whole core.
    add_dependencies(librestudio_smoke services_rdb program_services_rdb program_types_rdb)

    add_test(NAME librestudio_smoke COMMAND librestudio_smoke)
    set_tests_properties(librestudio_smoke PROPERTIES
        WORKING_DIRECTORY "${LIBRESTUDIO_PROGRAM_DIR}")
endif ()
