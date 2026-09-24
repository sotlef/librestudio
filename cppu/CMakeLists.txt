# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# cppu — the C++ UNO runtime: Any, Type, Reference, the type library loader, the
# thread pool and the environment stack.  This is the first module that compiles
# against generated com/sun/star bindings, so it is the first consumer of
# udkapi's headers.

set(CPPU_SOURCES
    cppu/source/cppu/compat
    cppu/source/cppu/cppu_opt
    cppu/source/threadpool/current
    cppu/source/threadpool/jobqueue
    cppu/source/threadpool/thread
    cppu/source/threadpool/threadident
    cppu/source/threadpool/threadpool
    cppu/source/typelib/static_types
    cppu/source/typelib/typelib
    cppu/source/uno/any
    cppu/source/uno/cascade_mapping
    cppu/source/uno/check
    cppu/source/uno/data
    cppu/source/uno/EnvDcp
    cppu/source/uno/EnvStack
    cppu/source/uno/IdentityMapping
    cppu/source/uno/lbenv
    cppu/source/uno/lbmap
    cppu/source/uno/loadmodule
    cppu/source/uno/sequence)

librestudio_expand_cxx(CPPU_SOURCES)

add_library(cppu SHARED ${CPPU_SOURCES})

# gb_Library_use_udk_api maps to the "normal" header variant - see
# gb_UnoApiHeadersTarget_select_variant in solenv/gbuild/UnoApiTarget.mk.
target_include_directories(cppu PRIVATE "${udkapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(cppu udkapi_normal_headers)

target_compile_definitions(cppu PRIVATE CPPU_DLLIMPLEMENTATION)
target_link_libraries(cppu PRIVATE sal salhelper)

target_link_options(cppu PRIVATE
    "-Wl,--version-script=${CMAKE_SOURCE_DIR}/cppu/util/cppu.map")

librestudio_set_library_output_name(cppu cppu UNO)

install(TARGETS cppu LIBRARY DESTINATION program)
