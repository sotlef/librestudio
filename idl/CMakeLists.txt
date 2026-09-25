# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# idl — the SDI slot-definition compiler (svidl only). It turns a module's
# *.sdi files (sfx2/sdi/sfx.sdi and friends) into the C++ header + binary slot
# table that gb_SdiTarget_SdiTarget consumes; sfx2 is the first (and, in this
# port so far, only) consumer.

set(SVIDL_SOURCES
    idl/source/cmptools/hash
    idl/source/cmptools/lex
    idl/source/objects/basobj
    idl/source/objects/bastype
    idl/source/objects/module
    idl/source/objects/object
    idl/source/objects/slot
    idl/source/objects/types
    idl/source/prj/command
    idl/source/prj/database
    idl/source/prj/globals
    idl/source/prj/svidl
    idl/source/prj/parser
)

librestudio_expand_cxx(SVIDL_SOURCES)
add_executable(svidl ${SVIDL_SOURCES})

target_include_directories(svidl PRIVATE
    "${CMAKE_SOURCE_DIR}/idl/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(svidl udkapi_normal_headers offapi_normal_headers)

target_link_libraries(svidl PRIVATE comphelper tl sal Boost::boost)

install(TARGETS svidl RUNTIME DESTINATION program)
