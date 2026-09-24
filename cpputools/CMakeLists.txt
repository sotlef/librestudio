# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# cpputools — the command-line UNO tools.
#
#   uno    loads a UNO component or runs a macro from the shell; it is also the
#          most convenient way to poke at a UNO installation by hand
#   sp2bv  renders StarBasic source, used by the Basic IDE
#
# Source lists from Executable_uno.mk and Executable_sp2bv.mk.

# gb_Executable_use_internal_comprehensive_api(uno, udkapi): the tool has to
# work before a typelib can be consulted, so it takes the self-contained variant
# of the bindings.
add_executable(uno "${CMAKE_SOURCE_DIR}/cpputools/source/unoexe/unoexe.cxx")
target_include_directories(uno PRIVATE "${udkapi_comprehensive_HEADERS_INCLUDE_DIR}")
add_dependencies(uno udkapi_comprehensive_headers)
target_link_libraries(uno PRIVATE cppu cppuhelper sal salhelper)

# gb_Executable_use_api(sp2bv, udkapi): the normal variant is enough here.
add_executable(sp2bv "${CMAKE_SOURCE_DIR}/cpputools/source/sp2bv/sp2bv.cxx")
target_include_directories(sp2bv PRIVATE "${udkapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(sp2bv udkapi_normal_headers)
target_link_libraries(sp2bv PRIVATE cppu cppuhelper sal)

install(TARGETS uno sp2bv RUNTIME DESTINATION program)
