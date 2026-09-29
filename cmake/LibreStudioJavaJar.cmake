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
#     PACKAGEROOT <dir>...
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
# PACKAGEROOT accepts multiple values (space-joined, unquoted, into one `jar
# -c ... META-INF <root1> <root2> ...` command line -- the jar tool's own -c
# mode already takes a list of files/directories, so this needs no special
# handling beyond not quoting the joined string away) because a single
# javac invocation can legitimately emit classes under more than one
# top-level package: connectivity/Jar_sdbc_hsqldb.mk is the first call site
# that needs this -- gb_Jar_set_packageroot(sdbc_hsqldb,com) plus
# gb_Jar_add_packagedir(sdbc_hsqldb,org) is upstream's own two-step way of
# saying the same thing (PACKAGEROOT=com, and a *second* top-level root,
# org/hsqldb/lib/FileSystemRuntimeException's own package, packed via the
# PACKAGEDIRS post-processing step below) -- re-derived here as one
# PACKAGEROOT of two entries instead, since (unlike genuine PACKAGEDIRS
# uses, e.g. javamaker's pre-built output) org/ in this case is compiled by
# this same function's own javac invocation, landing in the same classdir,
# so it needs no separate merge step.
#
# Skipped relative to upstream, deliberately: -source/-target javac flags
# (JAVA_SOURCE_VER/JAVA_TARGET_VER have no equivalent in this configure-less
# CMake port -- javac defaults to the running JDK's own version, same kind of
# simplification used elsewhere in this port); the Solar-Version manifest
# line (purely informational, and this port has no LIBO_VERSION_* to put
# there); and MODULAR_JAVA/module-info.class, per the module docstring above.
# librestudio_cmake_escape(<out-var> <value>)
#
# Escapes backslashes and double quotes in <value>, WITHOUT adding
# surrounding quotes -- for splicing an arbitrary string as the *interior*
# of some other double-quoted CMake string literal being built up in a
# *generated* .cmake script (as opposed to a literal appearing directly in a
# file this project's own authors write, which CMake's own parser already
# handles). Needed because the build scripts below are themselves CMake
# code, built up as one big string and written out with file(GENERATE) --
# see this file's own 2026-09-28 header update for why a generated CMake
# script replaced a generated POSIX shell script here: `sh` is not a
# cross-platform assumption (no MSYS/Git-Bash/WSL guaranteed on Windows),
# while `${CMAKE_COMMAND} -P` already is, since a CMake-generated build has
# CMake itself as its one unconditional dependency.
function(librestudio_cmake_escape out_var value)
    string(REPLACE "\\" "\\\\" _e "${value}")
    string(REPLACE "\"" "\\\"" _e "${_e}")
    set(${out_var} "${_e}" PARENT_SCOPE)
endfunction ()

# librestudio_cmake_quote(<out-var> <value>)
#
# librestudio_cmake_escape() plus the surrounding quotes -- for a value used
# as a whole, standalone argument in generated CMake source text (e.g. one
# execute_process(COMMAND ...) argument), as opposed to text spliced into
# the interior of some other, already-quoted string literal (use
# librestudio_cmake_escape() directly for that -- mixing the two up is a
# real bug: wrapping an already-quoted value in more quotes nests a literal
# quote character where none was wanted).
function(librestudio_cmake_quote out_var value)
    librestudio_cmake_escape(_q "${value}")
    set(${out_var} "\"${_q}\"" PARENT_SCOPE)
endfunction ()

# librestudio_cmake_quote_list(<out-var> <list-var>)
#
# Same as librestudio_cmake_quote() but for a whole CMake list, joining the
# individually-quoted elements with a single space -- for splicing a list of
# paths/arguments into one execute_process(COMMAND ...) line in generated
# CMake source text.
function(librestudio_cmake_quote_list out_var list_var)
    set(_parts "")
    foreach (_item IN LISTS ${list_var})
        librestudio_cmake_quote(_qitem "${_item}")
        list(APPEND _parts "${_qitem}")
    endforeach ()
    string(REPLACE ";" " " _joined "${_parts}")
    set(${out_var} "${_joined}" PARENT_SCOPE)
endfunction ()

function(librestudio_add_uno_jar name)
    cmake_parse_arguments(_jar "" "MANIFEST"
        "PACKAGEROOT;MANIFEST_CLASSPATH;CLASSPATH;PACKAGEDIRS;DEPENDS;SOURCES" ${ARGN})

    if (NOT _jar_PACKAGEROOT OR NOT _jar_SOURCES)
        message(FATAL_ERROR
            "librestudio_add_uno_jar(${name}): PACKAGEROOT and SOURCES are required")
    endif ()

    librestudio_expand_java(_jar_SOURCES)

    set(_classdir "${CMAKE_CURRENT_BINARY_DIR}/${name}-classes")
    set(_jarfile "${CMAKE_CURRENT_BINARY_DIR}/${name}.jar")
    set(_manifest "${_classdir}/META-INF/MANIFEST.MF")

    # javac's own classpath-entry separator is platform-specific (":" on
    # POSIX, ";" on Windows -- java.io.File.pathSeparator, not something
    # CMake exposes as a variable of its own) -- this project's own CMake
    # process always runs on the same host the resulting javac invocation
    # will (no cross-compiling a Java jar), so WIN32 here reflects the
    # actual target correctly, not just the configuring host by coincidence.
    if (WIN32)
        set(_cp_sep ";")
    else ()
        set(_cp_sep ":")
    endif ()
    set(_classpath "")
    if (_jar_CLASSPATH)
        string(REPLACE ";" "${_cp_sep}" _classpath "${_jar_CLASSPATH}")
    endif ()

    # A response file for SOURCES, the same reason hsqldb-build.cmake.in/
    # beanshell-build.cmake.in below use one for their own (much larger)
    # source lists: javac's own @file syntax sidesteps a real, cross-
    # platform argv/command-line length ceiling (Windows CreateProcess's is
    # a hard ~32767 characters) that a large SOURCES list of long absolute
    # paths could plausibly hit, not just a Unix ARG_MAX concern the old
    # shell-script version never had to consider on its one supported
    # platform.
    set(_sources_rsp "${CMAKE_CURRENT_BINARY_DIR}/${name}-sources.rsp")
    set(_sources_rsp_content "")
    foreach (_src IN LISTS _jar_SOURCES)
        string(APPEND _sources_rsp_content "\"${_src}\"\n")
    endforeach ()
    file(GENERATE OUTPUT "${_sources_rsp}" CONTENT "${_sources_rsp_content}")

    librestudio_cmake_quote(_q_javac "${Java_JAVAC_EXECUTABLE}")
    librestudio_cmake_quote(_q_jar "${Java_JAR_EXECUTABLE}")
    librestudio_cmake_quote(_q_classdir "${_classdir}")
    librestudio_cmake_quote(_q_classdir_meta "${_classdir}/META-INF")
    librestudio_cmake_quote(_q_classpath "${_classpath}")
    librestudio_cmake_quote(_q_jarfile "${_jarfile}")
    librestudio_cmake_quote(_q_manifest "${_manifest}")
    librestudio_cmake_quote(_q_rsp "@${_sources_rsp}")
    librestudio_cmake_quote_list(_q_packageroot _jar_PACKAGEROOT)

    set(_script_content "# Auto-generated by librestudio_add_uno_jar(${name}) -- do not edit.
file(MAKE_DIRECTORY ${_q_classdir_meta})
execute_process(
    COMMAND ${_q_javac} -encoding utf8 -d ${_q_classdir} -classpath ${_q_classpath} ${_q_rsp}
    RESULT_VARIABLE _r)
if (NOT _r EQUAL 0)
    message(FATAL_ERROR \"librestudio_add_uno_jar(${name}): javac failed\")
endif ()
file(WRITE ${_q_manifest} \"Manifest-Version: 1.0\\n\")
")

    if (_jar_MANIFEST_CLASSPATH)
        list(GET _jar_MANIFEST_CLASSPATH 0 _first_cp)
        librestudio_cmake_escape(_e_first_cp "${_first_cp}")
        string(APPEND _script_content "file(APPEND ${_q_manifest} \"Class-Path:  ${_e_first_cp}\\n\")\n")
        list(REMOVE_AT _jar_MANIFEST_CLASSPATH 0)
        foreach (_cp IN LISTS _jar_MANIFEST_CLASSPATH)
            librestudio_cmake_escape(_e_cp "${_cp}")
            string(APPEND _script_content "file(APPEND ${_q_manifest} \"  ${_e_cp}\\n\")\n")
        endforeach ()
    endif ()

    if (_jar_MANIFEST)
        librestudio_cmake_quote(_q_jar_manifest "${_jar_MANIFEST}")
        string(APPEND _script_content "file(READ ${_q_jar_manifest} _frag)\nfile(APPEND ${_q_manifest} \"\${_frag}\")\n")
    endif ()

    string(APPEND _script_content "execute_process(
    COMMAND ${_q_jar} -c -f ${_q_jarfile} -m ${_q_manifest} META-INF ${_q_packageroot}
    WORKING_DIRECTORY ${_q_classdir}
    RESULT_VARIABLE _r)
if (NOT _r EQUAL 0)
    message(FATAL_ERROR \"librestudio_add_uno_jar(${name}): jar -c failed\")
endif ()
")

    foreach (_dir IN LISTS _jar_PACKAGEDIRS)
        get_filename_component(_dir_parent "${_dir}" DIRECTORY)
        get_filename_component(_dir_name "${_dir}" NAME)
        librestudio_cmake_quote(_q_dir_parent "${_dir_parent}")
        librestudio_cmake_quote(_q_dir_name "${_dir_name}")
        string(APPEND _script_content "execute_process(
    COMMAND ${_q_jar} -u -f ${_q_jarfile} ${_q_dir_name}
    WORKING_DIRECTORY ${_q_dir_parent}
    RESULT_VARIABLE _r)
if (NOT _r EQUAL 0)
    message(FATAL_ERROR \"librestudio_add_uno_jar(${name}): jar -u failed\")
endif ()
")
    endforeach ()

    set(_build_script "${CMAKE_CURRENT_BINARY_DIR}/${name}-build-jar.cmake")
    file(GENERATE OUTPUT "${_build_script}" CONTENT "${_script_content}")

    add_custom_command(
        OUTPUT "${_jarfile}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_classdir}"
        COMMAND ${CMAKE_COMMAND} -P "${_build_script}"
        DEPENDS ${_jar_SOURCES} ${_jar_MANIFEST} "${_build_script}" "${_sources_rsp}" ${_jar_DEPENDS}
        COMMENT "jar: ${name}"
        VERBATIM)

    add_custom_target(${name}_jar ALL DEPENDS "${_jarfile}")

    set(${name}_JAR "${_jarfile}" CACHE INTERNAL "${name} jar")
endfunction ()
