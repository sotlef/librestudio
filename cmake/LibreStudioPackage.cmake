# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# librestudio_add_package_mirror(<name> PAIRS "<abs-src>:<abs-destdir>"...)
#
# gb_Package_Package/gb_Package_add_files's equivalent for CMake: several
# existing modules already install() plain, checked-in data files (not
# produced by any compiler) into the build tree via a hand-written
# per-category foreach + add_custom_command loop -- e.g. the UIConfig_*
# blocks in vcl/svtools/svx/basctl/cui/desktop/filter/sw's own CMakeLists.txt
# (each: set(<VAR> <absolute files>...), install(FILES ${<VAR>} DESTINATION
# ...), then a foreach that add_custom_command()-copies every file into the
# build tree, finally add_custom_target(<name>_uiconfig ALL DEPENDS
# ${outputs})). This function is that same shape, pulled out once as a
# reusable helper, for the analogous but more general gbuild mechanism:
# gb_Package_Package(name, basedir) + one or more gb_Package_add_files(name,
# destdir, files...) calls per package -- an arbitrary multi-destination
# file copy, not always a single category/directory the way every UIConfig_*
# call site happens to be.
#
# Introduced for wizards' 13 gb_Package_Package(wizards_basicsrv*, ...)
# calls (see wizards/CMakeLists.txt's own install(FILES...) blocks, which
# this function's PAIRS argument mirrors 1:1) plus wizards_basicshare and
# scripting_scriptbindinglib -- the exact package names CppunitTest_desktop_
# lib.mk's own gb_CppunitTest_use_packages call needs (see desktop/
# CppunitTest_deferred.cmake's own "desktop_lib" section), each of which
# librestudio_add_cppunit_test()'s PACKAGES parameter (cmake/LibreStudioTest.
# cmake) resolves via a plain if(NOT TARGET ...) check -- exactly like
# COMPONENTS/UICONFIG already do -- so every gbuild Package_*.mk this port
# wants to make reachable that way needs a real, named, build-time ALL
# target, not just an install(FILES ...) rule (real for `cmake --install`,
# but producing nothing a `cmake --build` -- or a CppunitTest's own
# add_dependencies() -- ever touches). This function produces exactly that,
# for however many source-file/destination-directory pairs a given
# Package_*.mk's own gb_Package_add_files calls need (wizards_basicsrvtools:
# one destination directory; wizards_basicsrvscriptforge: four, across
# share/basic/ScriptForge, share/Scripts/python, share/basic/ScriptForge/po,
# and program/ -- see wizards/CMakeLists.txt's own call site for exactly
# which).
#
# PAIRS: "<abs-src>:<abs-destdir>" strings, one per source file --
#   deliberately not "<abs-src>:<abs-dest-file>", since gb_Package_add_files
#   itself only ever renames a file's directory, never its basename (see
#   gb_Package_add_files's own $(2)/$(notdir $(file)) destination pattern,
#   solenv/gbuild/Package.mk) -- this keeps every call site's PAIRS list a
#   plain "file goes into this directory" statement, matching the shape
#   gb_Package_add_files(name,destdir,files...) already has. Colon-separated
#   (not e.g. space-separated) because CMake list elements cannot themselves
#   contain semicolons, and every path here is a plain Linux path that can
#   never contain a literal colon -- the same delimiter choice already made
#   for the "<VAR>:<category>" pairs in e.g. basctl/CMakeLists.txt's
#   UIConfig_basicide block and sw/CMakeLists.txt's UIConfig_swriter block.
function(librestudio_add_package_mirror name)
    cmake_parse_arguments(_pkg "" "" "PAIRS" ${ARGN})

    if (NOT _pkg_PAIRS)
        message(FATAL_ERROR "librestudio_add_package_mirror(${name}): PAIRS is required")
    endif ()

    set(_pkg_outputs "")
    foreach (_pkg_pair IN LISTS _pkg_PAIRS)
        # Split at the colon between the two absolute paths -- not at a drive
        # letter's ("D:/..." on Windows), which is why this is not a plain
        # string(REPLACE ":" ";").
        if (_pkg_pair MATCHES "^(([A-Za-z]:)?[^:]*):(.*)$")
            set(_pkg_src "${CMAKE_MATCH_1}")
            set(_pkg_destdir "${CMAKE_MATCH_3}")
        else ()
            message(FATAL_ERROR "librestudio_add_package_mirror(${name}): bad PAIRS entry '${_pkg_pair}'")
        endif ()
        get_filename_component(_pkg_name "${_pkg_src}" NAME)
        set(_pkg_dest "${_pkg_destdir}/${_pkg_name}")
        add_custom_command(
            OUTPUT "${_pkg_dest}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_pkg_destdir}"
            COMMAND ${CMAKE_COMMAND} -E copy "${_pkg_src}" "${_pkg_dest}"
            DEPENDS "${_pkg_src}"
            COMMENT "Copying ${_pkg_name} into build-tree ${_pkg_destdir}")
        list(APPEND _pkg_outputs "${_pkg_dest}")
    endforeach ()
    unset(_pkg_pair)
    unset(_pkg_src)
    unset(_pkg_destdir)
    unset(_pkg_name)
    unset(_pkg_dest)

    add_custom_target(${name} ALL DEPENDS ${_pkg_outputs})
endfunction()
