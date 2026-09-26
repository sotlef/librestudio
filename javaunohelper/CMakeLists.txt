# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# javaunohelper -- Library_juh.mk + Library_juhx.mk only (Module_javaunohelper.mk's
# other two targets are out of scope: Jar_juh is the empty legacy Class-Path shim
# jar this port never builds, same as ridl.jar/jurt.jar/unoil.jar -- it has a
# gb_Jar_add_manifest_classpath and gb_Jar_set_manifest call but no
# gb_Jar_add_sourcefiles call at all; JunitTest_juh is test infra, out of scope
# like every other JunitTest/CppunitTest in this port).
#
# This port does not set DISABLE_DYNLOADING (no other module implements that
# flag either), so Library_juh.mk takes its "normal" branch: juh is a thin
# library holding only source/preload, and the real bootstrap/vm implementation
# that the DISABLE_DYNLOADING branch would otherwise fold into juh instead
# lives in juhx, exactly as it does upstream on every non-DISABLE_DYNLOADING
# platform.

# juh: preload.cxx only, dlopens juhx (via osl::Module, not a link dependency)
# and reflects Bootstrap/SharedLibraryLoader's native methods to it.
add_library(juh SHARED
    "${CMAKE_SOURCE_DIR}/javaunohelper/source/preload.cxx")

target_include_directories(juh PRIVATE
    "${JNI_INCLUDE_DIRS}"
    "${udkapi_comprehensive_HEADERS_INCLUDE_DIR}")
add_dependencies(juh udkapi_comprehensive_headers)

target_link_libraries(juh PRIVATE sal)

# juhx: the real Bootstrap/SharedLibraryLoader JNI entry points (bootstrap,
# javaunohelper) plus the UnoVirtualMachine-holding ComponentContext callback
# object (vm) that ties a bootstrapped UNO instance to the attaching JVM
# thread via jvmaccess.
add_library(juhx SHARED
    "${CMAKE_SOURCE_DIR}/javaunohelper/source/bootstrap.cxx"
    "${CMAKE_SOURCE_DIR}/javaunohelper/source/javaunohelper.cxx"
    "${CMAKE_SOURCE_DIR}/javaunohelper/source/vm.cxx")

target_include_directories(juhx PRIVATE
    "${JNI_INCLUDE_DIRS}"
    "${udkapi_comprehensive_HEADERS_INCLUDE_DIR}")
add_dependencies(juhx udkapi_comprehensive_headers)

target_link_libraries(juhx PRIVATE
    cppu cppuhelper jvmaccess sal salhelper)

# Repository.mk registers both juh and juhx under PLAINLIBS_URE, not a *LIBS_URE
# uno_/gcc3 group -- juh is dlopen'd by the JVM-side UnoLoader
# (NativeLibraryLoader.loadLibrary), and juhx is dlopen'd by juh itself
# (source/preload.cxx), so both need the bare "libjuh.so"/"libjuhx.so" names,
# not libuno_juh*.so.
librestudio_set_library_output_name(juh juh PLAIN)
librestudio_set_library_output_name(juhx juhx PLAIN)

install(TARGETS juh juhx LIBRARY DESTINATION program)
