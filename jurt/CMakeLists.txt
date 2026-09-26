# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# jurt -- Library_jpipe.mk only (Module_jurt.mk's other two targets are out of
# scope: Jar_jurt is the empty legacy Class-Path shim jar this port never
# builds, same as ridl.jar/unoil.jar; Library_jpipx is the Windows-only real
# implementation that jpipe on Windows merely wraps -- this port is
# Linux-only, so Library_jpipe.mk's own OS,WNT branch, which builds nothing
# but that wrapper, is skipped too).
#
# jpipe is not a UNO component -- Library_jpipe.mk has no
# gb_Library_set_componentfile call, and PipeConnection.java (ridljar) reaches
# it via a plain JNI System.loadLibrary, not the UNO service manager -- so,
# unlike stoc_add_library's targets, this needs no .component file and no
# librestudio_add_component call.
#
# The Clang -fsanitize=address staticsalhack branch (Library_jpipe__staticsalhack)
# is skipped: this port has no sanitizer-flag detection anywhere else either,
# same reasoning as every other build-environment-specific knob skipped so far.
add_library(jpipe SHARED
    "${CMAKE_SOURCE_DIR}/jurt/source/pipe/com_sun_star_lib_connections_pipe_PipeConnection.c")

target_include_directories(jpipe PRIVATE "${JNI_INCLUDE_DIRS}")

target_link_libraries(jpipe PRIVATE sal)

# Repository.mk registers jpipe under PLAINLIBS_URE, not a *LIBS_URE
# uno_/gcc3 group -- it is dlopen'd by Java's System.loadLibrary("jpipe"),
# which expects the bare "libjpipe.so" name, not libuno_jpipe*.so.
librestudio_set_library_output_name(jpipe jpipe PLAIN)

install(TARGETS jpipe LIBRARY DESTINATION program)
