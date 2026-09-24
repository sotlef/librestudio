# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# offapi — the office API: the types the UNO core services are described in
# terms of.  It builds no library either; its product is offapi.rdb and the C++
# bindings generated from it.
#
# The UNO core needs more of this than one might expect: cppuhelper throws
# com.sun.star.ucb exceptions (cppuhelper/source/exc_thrower.cxx), and
# comphelper is built against the SDK API, so neither can be compiled with
# udkapi alone.
#
# Types live under both com/ and org/ (the latter holding org.libreoffice.* and
# org.freedesktop.*); the directories under type_reference/ are dumps for
# unoidl-check and are excluded from the scan automatically.

librestudio_uno_api_rdb(offapi
    IDL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT   "${CMAKE_BINARY_DIR}/UnoApiTarget/offapi.rdb"
    DEP_RDBS "${udkapi_RDB}")

# gb_Library_use_sdk_api (comphelper) resolves to the normal variant.
librestudio_uno_api_headers(offapi_normal
    RDB         "${offapi_RDB}"
    OUTPUT_DIR  "${CMAKE_BINARY_DIR}/UnoApiHeadersTarget/offapi/normal"
    VARIANT     normal
    EXTRA_RDBS  "${udkapi_RDB}")

# gb_Library_use_internal_comprehensive_api (cppuhelper, bridges) needs the
# self-contained variant, which inlines the type information of its dependencies.
librestudio_uno_api_headers(offapi_comprehensive
    RDB         "${offapi_RDB}"
    OUTPUT_DIR  "${CMAKE_BINARY_DIR}/UnoApiHeadersTarget/offapi/comprehensive"
    VARIANT     comprehensive
    EXTRA_RDBS  "${udkapi_RDB}")

install(FILES "${offapi_RDB}" DESTINATION program/types)
