# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# jvmaccess -- the thin JNI wrapper (VirtualMachine/UnoVirtualMachine/
# ClassPath) that the rest of the real Java bridge (stoc's javaloader/
# javavm, jurt, javaunohelper, bean) uses to attach the current thread to a
# JVM started through jvmfwk and to build a classpath for it. Source list,
# defs and libraries transcribed from Library_jvmaccess.mk; gated behind
# LIBRESTUDIO_ENABLE_JAVA like jvmfwk above it. No workbench/* example
# makefiles or CppunitTest here are ported, per this port's usual scoping of
# test/example infra as out of scope.
set(JVMACCESS_SOURCES
    jvmaccess/source/classpath
    jvmaccess/source/unovirtualmachine
    jvmaccess/source/virtualmachine
)

librestudio_expand_cxx(JVMACCESS_SOURCES)
add_library(jvmaccess SHARED ${JVMACCESS_SOURCES})

target_include_directories(jvmaccess PRIVATE
    "${JNI_INCLUDE_DIRS}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(jvmaccess udkapi_normal_headers)

target_compile_definitions(jvmaccess PRIVATE JVMACCESS_DLLIMPLEMENTATION)

target_link_libraries(jvmaccess PRIVATE
    cppu cppuhelper sal salhelper)

librestudio_set_library_output_name(jvmaccess jvmaccess OOO)

install(TARGETS jvmaccess LIBRARY DESTINATION program)
