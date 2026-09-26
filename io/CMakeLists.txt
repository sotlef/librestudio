# This file is part of the LibreStudio project.
#
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

# --- CppunitTest targets ------------------------------------------------------
#
# CppunitTest_io_textinputstream.mk: 1 source, use_libraries cppu cppuhelper
# sal unotest, use_udk_api -> plain udkapi, use_ure -> PROTECTORS. Re-checked
# directly against the live .mk (not just the prior-research summary). Note:
# upstream wires this in via gb_Module_add_subsequentcheck_targets rather than
# the normal gb_Module_add_check_targets (Module_io.mk) -- a "subsequentcheck"
# runs against an already-installed instance rather than a fresh build product,
# but functionally it is still just "a test that runs"; there is no
# subsequentcheck-vs-check distinction in this port's CMake/CTest model, so it
# is registered exactly like every other librestudio_add_cppunit_test() call.
librestudio_add_cppunit_test(io_textinputstream
    SOURCES
        io/qa/textinputstream
    LIBRARIES cppu cppuhelper sal unotest
    INCLUDE "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector)
