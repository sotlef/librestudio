# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# oovbaapi — the VBA-compatibility API (ooo.vba.*): the types StarBasic's UNO
# automation objects are described in terms of. Builds no library; its product
# is oovbaapi.rdb and the C++ bindings generated from it.
#
# UnoApi_oovbaapi.mk selects its ~1060 .idl files one at a time through three
# macros (gb_UnoApi_add_idlfiles[_nohdl|_noheader]) that differ only in which
# of the generated .hpp/.hdl headers gbuild tracks as build outputs for its own
# dependency graph -- every variant still feeds the file into the registry.
# cppumaker has no equivalent per-type switch: it always emits headers for
# everything in the .rdb it is pointed at. So scanning the whole tree (minus
# genconstidl/, a helper Perl script with no .idl files of its own -- not a
# types dump like offapi's type_reference/, just not part of the type tree
# either) is exactly equivalent for our purposes -- verified by counting: 1061
# .idl files on disk, 1061 filenames named across the three macro calls.

librestudio_uno_api_rdb(oovbaapi
    IDL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT   "${CMAKE_BINARY_DIR}/UnoApiTarget/oovbaapi.rdb"
    EXCLUDE  genconstidl
    DEP_RDBS "${udkapi_RDB}" "${offapi_RDB}")

# gb_Library_use_api(sb, oovbaapi) resolves to the normal variant, same as
# gb_Library_use_sdk_api.
librestudio_uno_api_headers(oovbaapi_normal
    RDB         "${oovbaapi_RDB}"
    OUTPUT_DIR  "${CMAKE_BINARY_DIR}/UnoApiHeadersTarget/oovbaapi/normal"
    VARIANT     normal
    EXTRA_RDBS  "${udkapi_RDB}" "${offapi_RDB}")

install(FILES "${oovbaapi_RDB}" DESTINATION program/types)
