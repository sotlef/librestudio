# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# codeapi -- the UNO API of "Code" (com.sun.star.code.*): the contracts
# between Code's backend services (project, document, output, task), the
# plugins that extend them (project-format readers, output filters, task
# handlers) and the application shell that implements the front end. Original
# LibreStudio API, not a port of an upstream module, so it has no Module_*.mk
# and no row in PORTING_STATUS.md. Modelled on the type-registry-only layout
# of oovbaapi: it builds no library, its products are codeapi.rdb and the C++
# bindings generated from it.
#
# The tree under com/ mirrors the type names (com/sun/star/code/<category>/
# <Type>.idl is com.sun.star.code.<category>.<Type>), one entity per file as
# unoidl-write requires.

librestudio_uno_api_rdb(codeapi
    IDL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT   "${CMAKE_BINARY_DIR}/UnoApiTarget/codeapi.rdb"
    DEP_RDBS "${udkapi_RDB}" "${offapi_RDB}")

librestudio_uno_api_headers(codeapi_normal
    RDB         "${codeapi_RDB}"
    OUTPUT_DIR  "${CMAKE_BINARY_DIR}/UnoApiHeadersTarget/codeapi/normal"
    VARIANT     normal
    EXTRA_RDBS  "${udkapi_RDB}" "${offapi_RDB}")

install(FILES "${codeapi_RDB}" DESTINATION program/types)
