# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Building a real UNO Java .jar (ridljar's unoloader/libreoffice, later bean's
# officebean) -- solenv/gbuild/Jar.mk + solenv/gbuild/JavaClassSet.mk, minus
# everything gbuild-environment-specific (icecream, SOURCE_DATE_EPOCH-based
# reproducible timestamps) and minus the whole MODULAR_JAVA / module-info.class
# path, which every upstream Jar_*.mk that uses it calls "ugly" -- niche,
# JDK9+-only, and not needed for the jar to function. Only ever used under
# LIBRESTUDIO_ENABLE_JAVA.

# librestudio_expand_java(<list-var>)
#
# Appends .java to every relative entry, leaving absolute paths alone -- same
# idea as librestudio_expand_cxx (cmake/LibreStudioPlatform.cmake) but for the
# java sourcefile lists in Jar_*.mk, which likewise name each file without its
# extension.
macro(librestudio_expand_java _list)
    set(_java_expanded "")
    foreach (_src IN LISTS ${_list})
        if (IS_ABSOLUTE "${_src}")
            list(APPEND _java_expanded "${_src}")
        else ()
            list(APPEND _java_expanded "${CMAKE_SOURCE_DIR}/${_src}.java")
        endif ()
    endforeach ()
    set(${_list} "${_java_expanded}")
    unset(_java_expanded)
endmacro()

# librestudio_add_uno_jar(<name>
#     PACKAGEROOT <dir>
#     [MANIFEST    <file>]
#     [MANIFEST_CLASSPATH <entry>...]
#     [CLASSPATH <path>...]
#     [PACKAGEDIRS <dir>...]
#     [DEPENDS <target-or-file>...]
#     SOURCES <file...>)
#
# Mirrors gb_Jar_Jar + the javac step in gb_JavaClassSet__command + the jar
# assembly in gb_Jar__command: compile SOURCES into a private classdir with
# javac, assemble MANIFEST.MF the way gb_Jar__command does it (Manifest-
# Version, then -- when MANIFEST_CLASSPATH is given -- a Class-Path line built
# the same way solenv/bin/write_classpath.sh does, one entry per continuation
# line, then -- only when MANIFEST is given, matching gb_Jar__command's own
# `$(if $(MANIFEST),cat $(MANIFEST) >> ...)` -- the raw MANIFEST fragment
# appended verbatim; a Jar_*.mk that never calls gb_Jar_set_manifest leaves
# MANIFEST empty upstream too, so the assembled manifest is just the
# Manifest-Version line plus any Class-Path), `jar -c` the
# classdir's PACKAGEROOT into <name>.jar, then `jar -u` each PACKAGEDIRS entry
# on top. PACKAGEDIRS holds directories built elsewhere that are already
# ready to pack -- e.g. javamaker's output (see librestudio_uno_api_java in
# cmake/LibreStudioUnoApi.cmake: javamaker writes JVM .class bytecode
# directly, not .java text), which is why they are merged in as-is rather
# than recompiled, exactly like gb_Jar__command's own PACKAGEDIRS handling.
#
# Skipped relative to upstream, deliberately: -source/-target javac flags
# (JAVA_SOURCE_VER/JAVA_TARGET_VER have no equivalent in this configure-less
# CMake port -- javac defaults to the running JDK's own version, same kind of
# simplification used elsewhere in this port); the Solar-Version manifest
# line (purely informational, and this port has no LIBO_VERSION_* to put
# there); and MODULAR_JAVA/module-info.class, per the module docstring above.
function(librestudio_add_uno_jar name)
    cmake_parse_arguments(_jar "" "PACKAGEROOT;MANIFEST"
        "MANIFEST_CLASSPATH;CLASSPATH;PACKAGEDIRS;DEPENDS;SOURCES" ${ARGN})

    if (NOT _jar_PACKAGEROOT OR NOT _jar_SOURCES)
        message(FATAL_ERROR
            "librestudio_add_uno_jar(${name}): PACKAGEROOT and SOURCES are required")
    endif ()

    librestudio_expand_java(_jar_SOURCES)

    set(_classdir "${CMAKE_CURRENT_BINARY_DIR}/${name}-classes")
    set(_jarfile "${CMAKE_CURRENT_BINARY_DIR}/${name}.jar")
    set(_manifest "${_classdir}/META-INF/MANIFEST.MF")

    set(_classpath "")
    if (_jar_CLASSPATH)
        string(REPLACE ";" ":" _classpath "${_jar_CLASSPATH}")
    endif ()

    set(_sources_sh "")
    foreach (_src IN LISTS _jar_SOURCES)
        string(APPEND _sources_sh " '${_src}'")
    endforeach ()

    set(_script_content "#!/bin/sh
set -e
mkdir -p '${_classdir}/META-INF'
'${Java_JAVAC_EXECUTABLE}' -encoding utf8 -d '${_classdir}' -classpath '${_classpath}'${_sources_sh}
echo 'Manifest-Version: 1.0' > '${_manifest}'
")

    if (_jar_MANIFEST_CLASSPATH)
        list(GET _jar_MANIFEST_CLASSPATH 0 _first_cp)
        string(APPEND _script_content "echo 'Class-Path:  ${_first_cp}' >> '${_manifest}'\n")
        list(REMOVE_AT _jar_MANIFEST_CLASSPATH 0)
        foreach (_cp IN LISTS _jar_MANIFEST_CLASSPATH)
            string(APPEND _script_content "echo '  ${_cp}' >> '${_manifest}'\n")
        endforeach ()
    endif ()

    if (_jar_MANIFEST)
        string(APPEND _script_content "cat '${_jar_MANIFEST}' >> '${_manifest}'\n")
    endif ()

    string(APPEND _script_content "cd '${_classdir}'
'${Java_JAR_EXECUTABLE}' -c -f '${_jarfile}' -m '${_manifest}' META-INF ${_jar_PACKAGEROOT}
")

    foreach (_dir IN LISTS _jar_PACKAGEDIRS)
        get_filename_component(_dir_parent "${_dir}" DIRECTORY)
        get_filename_component(_dir_name "${_dir}" NAME)
        string(APPEND _script_content "cd '${_dir_parent}'\n'${Java_JAR_EXECUTABLE}' -u -f '${_jarfile}' '${_dir_name}'\n")
    endforeach ()

    set(_build_script "${CMAKE_CURRENT_BINARY_DIR}/${name}-build-jar.sh")
    file(GENERATE OUTPUT "${_build_script}" CONTENT "${_script_content}")

    add_custom_command(
        OUTPUT "${_jarfile}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_classdir}"
        COMMAND sh "${_build_script}"
        DEPENDS ${_jar_SOURCES} ${_jar_MANIFEST} "${_build_script}" ${_jar_DEPENDS}
        COMMENT "jar: ${name}"
        VERBATIM)

    add_custom_target(${name}_jar ALL DEPENDS "${_jarfile}")

    set(${name}_JAR "${_jarfile}" CACHE INTERNAL "${name} jar")
endfunction ()
