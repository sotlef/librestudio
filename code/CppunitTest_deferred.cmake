# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# The tests of Code's services.  Original to LibreStudio (no CppunitTest_*.mk
# upstream).  Deferred (see code/CMakeLists.txt) because RDBS "services" is
# only defined once every component-registering module has run.
#
# code_semantictokencontroller runs the controller of codeservices, found by
# UNO through services.rdb, with a provider of its own.  code_clangsemantictokens
# includes the provider's source and calls it directly, so it needs libclang
# and nothing of UNO but the types.

set(_code_test_include
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${codeapi_normal_HEADERS_INCLUDE_DIR}")
set(_code_test_depends
    udkapi_normal_headers offapi_normal_headers codeapi_normal_headers program_services_rdb)
set(_code_test_protectors
    unobootstrapprotector unobootstrapprotector
    unoexceptionprotector unoexceptionprotector)

librestudio_add_cppunit_test(code_semantictokencontroller
    SOURCES code/qa/semantic/semantictokencontroller
    LIBRARIES comphelper cppu cppuhelper sal
    INCLUDE ${_code_test_include}
    DEPENDS ${_code_test_depends}
    PROTECTORS ${_code_test_protectors}
    RDBS services)

if (TARGET codeclang)
    librestudio_add_cppunit_test(code_clangsemantictokens
        SOURCES code/qa/clang/clangsemantictokens
        LIBRARIES "${LIBCLANG_LIBRARY}" comphelper cppu cppuhelper sal
        INCLUDE "${LIBCLANG_INCLUDE_DIR}" ${_code_test_include}
        DEPENDS ${_code_test_depends}
        PROTECTORS ${_code_test_protectors}
        RDBS services)
endif ()
