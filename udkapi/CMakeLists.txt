# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# udkapi — the core UNO type definitions (417 .idl under com/), and the source
# of `types.rdb`, which an installation carries next to unorc and every UNO
# runtime reads.
#
# It builds no library: the only thing here is the type registry and the C++
# bindings generated from it.

librestudio_uno_api_rdb(udkapi
    IDL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT   "${CMAKE_BINARY_DIR}/UnoApiTarget/udkapi.rdb")

# Most of the tree compiles against the "normal" bindings (-L): lightweight,
# with the rest of the type information resolved through the typelib at run
# time.  cppuhelper, the C++ bridge and stoc/bootstrap need their own variants
# and request them where they are built.
librestudio_uno_api_headers(udkapi_normal
    RDB        "${udkapi_RDB}"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/UnoApiHeadersTarget/udkapi/normal"
    VARIANT    normal)

librestudio_uno_api_headers(udkapi_comprehensive
    RDB        "${udkapi_RDB}"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/UnoApiHeadersTarget/udkapi/comprehensive"
    VARIANT    comprehensive)

# stoc/bootstrap has to link without a typelib existing yet, which is what the
# bootstrap variant is for.
librestudio_uno_api_headers(udkapi_bootstrap
    RDB        "${udkapi_RDB}"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/UnoApiHeadersTarget/udkapi/bootstrap"
    VARIANT    bootstrap)

# types.rdb is what an installation calls it; the UNO_TYPES entry in unorc
# points at it.
install(FILES "${udkapi_RDB}" DESTINATION program RENAME types.rdb)

# The reference dump used to detect incompatible changes to the published API.
# Not built into the registry above, but worth keeping next to it.
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/type_reference/udkapi.idl"
        DESTINATION program/types)
