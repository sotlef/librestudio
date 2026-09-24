# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# io — the pipe and socket UNO services: acceptors, connectors and the stream
# wrappers built on them.
# Source list from Library_io.mk.

set(IO_SOURCES
    io/source/acceptor/acc_pipe
    io/source/acceptor/acc_socket
    io/source/acceptor/acceptor
    io/source/connector/connector
    io/source/connector/ctr_pipe
    io/source/connector/ctr_socket
    io/source/stm/odata
    io/source/stm/omark
    io/source/stm/opipe
    io/source/stm/opump
    io/source/stm/streamhelper
    io/source/TextInputStream/TextInputStream
    io/source/TextOutputStream/TextOutputStream)

librestudio_expand_cxx(IO_SOURCES)

add_library(io SHARED ${IO_SOURCES})

target_include_directories(io PRIVATE
    "${CMAKE_SOURCE_DIR}/io/source"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(io udkapi_normal_headers)

target_link_libraries(io PRIVATE cppu cppuhelper sal)

# io/source/io.component describes where the service manager finds this library.
librestudio_add_component(io
    COMPONENT "${CMAKE_SOURCE_DIR}/io/source/io.component"
    LIBRARY   io
    LAYER     URELIB)

librestudio_set_library_output_name(io io OOO)

install(TARGETS io LIBRARY DESTINATION program)
