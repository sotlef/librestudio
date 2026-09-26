# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# UNO components and the service registry they end up in.
#
# A .component file describes which services a shared library implements.  It is
# not usable as-is: the service manager needs to know where the library is, so
# each file is rewritten with a uri attribute before being packed together with
# all the others into a single XML registry.
#
# Upstream does the same through createcomponent.xslt and packcomponents.xslt
# from solenv/bin (solenv/gbuild/ComponentTarget.mk and Rdb.mk); the stylesheets
# are reused as they are, they are plain data and carry no gbuild dependency.
#
# Modules declare their components where the library is defined; the registries
# are packed once every module has had its say, which is why the collection is
# deferred to librestudio_finalize_components().

set(LIBRESTUDIO_CREATE_COMPONENT_XSLT "${CMAKE_SOURCE_DIR}/solenv/bin/createcomponent.xslt")
set(LIBRESTUDIO_PACK_COMPONENTS_XSLT "${CMAKE_SOURCE_DIR}/solenv/bin/packcomponents.xslt")
set(LIBRESTUDIO_OPTIONALS_XSLT "${CMAKE_SOURCE_DIR}/solenv/bin/optionalimplementations.xslt")

# The prefixes come from gb_Library__COMPONENTPREFIXES in
# solenv/gbuild/Library.mk, keyed by the layer the library belongs to: UNO
# runtime libraries live in the URE's library directory, everything else in the
# office's.  Both resolve through unorc at run time; in this build they name the
# same directory.
set(LIBRESTUDIO_COMPONENT_PREFIX_URELIB "vnd.sun.star.expand:\$URE_INTERNAL_LIB_DIR/")
set(LIBRESTUDIO_COMPONENT_PREFIX_OOO    "vnd.sun.star.expand:\$LO_LIB_DIR/")

# librestudio_add_component(<name>
#     COMPONENT <file.component>
#     LIBRARY   <cmake-target>
#     [LAYER    URELIB|OOO]        # default: URELIB
#     [RDB      <rdb-name>]        # default: the main app registry (see below)
#     [IMPLEMENTATION <file>...])  # one or more <file.component>.<implid> files
#                                  # naming the implementations this platform
#                                  # provides; gbuild accumulates these too, one
#                                  # per gb_Library_add_componentimpl(s) call, so
#                                  # several are just concatenated before the
#                                  # optionals comparison (cat $(1).optionals
#                                  # $(COMPONENTIMPL) in ComponentTarget.mk).
#
# RDB mirrors gb_Library_set_componentfile's third argument: gbuild lets a
# component be packed into any named Rdb, not just the main app's "services"
# one -- e.g. testtools/Library_cppobj.mk routes into "uno_services", a
# private registry consumed only by its own generated test-launcher scripts,
# never installed. Omitting RDB keeps today's behaviour (the single global
# LIBRESTUDIO_COMPONENTS property, packed by the root CMakeLists.txt's one
# librestudio_finalize_components(services) call); passing RDB appends
# instead to a separate global property per name, so any number of
# independent registries can coexist, each fed only by the components
# explicitly routed to it -- matching gb_Rdb_add_component's per-Rdb
# COMPONENTS list.
#
# Rewrites <file.component> into the build tree with the uri of the library
# filled in, and records it for the registry packed by
# librestudio_finalize_components().
function(librestudio_add_component name)
    cmake_parse_arguments(_comp "" "COMPONENT;LIBRARY;LAYER;RDB" "IMPLEMENTATION" ${ARGN})

    if (NOT _comp_COMPONENT OR NOT _comp_LIBRARY)
        message(FATAL_ERROR
            "librestudio_add_component(${name}): COMPONENT and LIBRARY are required")
    endif ()
    if (NOT _comp_LAYER)
        set(_comp_LAYER URELIB)
    endif ()

    if (_comp_LAYER STREQUAL "URELIB")
        set(_prefix "${LIBRESTUDIO_COMPONENT_PREFIX_URELIB}")
    elseif (_comp_LAYER STREQUAL "OOO")
        set(_prefix "${LIBRESTUDIO_COMPONENT_PREFIX_OOO}")
    else ()
        message(FATAL_ERROR "librestudio_add_component(${name}): unknown LAYER '${_comp_LAYER}'")
    endif ()

    # A component file may mark an implementation <optional/>: one that some
    # platforms provide and others do not.  createcomponent.xslt drops every
    # implementation whose name appears anywhere in the `filtered` parameter,
    # so that string has to name the optionals this build does *not* provide.
    #
    # Upstream works it out as `cat <optionals> <impl> | sort | uniq -u`, where
    # optionals comes from optionalimplementations.xslt — through xsl:message,
    # which is why it arrives on stderr — and impl is the file given as
    # IMPLEMENTATION.  Both are files of the source tree, so the same value is
    # computed here at configure time instead; createcomponent.xslt then has
    # nothing left to interpolate at build time.
    set(_filtered "")
    if (_comp_IMPLEMENTATION)
        execute_process(
            COMMAND "${XSLTPROC_EXECUTABLE}" --nonet
                    "${LIBRESTUDIO_OPTIONALS_XSLT}" "${_comp_COMPONENT}"
            OUTPUT_QUIET ERROR_VARIABLE _optionals
            RESULT_VARIABLE _optionals_status)
        if (NOT _optionals_status EQUAL 0)
            message(FATAL_ERROR
                "librestudio_add_component(${name}): optionalimplementations.xslt "
                "failed on ${_comp_COMPONENT}")
        endif ()
        string(REPLACE "\n" ";" _optionals "${_optionals}")

        set(_built "")
        foreach (_impl IN LISTS _comp_IMPLEMENTATION)
            file(STRINGS "${_impl}" _impl_lines)
            list(APPEND _built ${_impl_lines})
        endforeach ()
        list(FILTER _built EXCLUDE REGEX "^#")

        # Exactly the names that appear on one side only — sort | uniq -u.
        set(_seen "")
        set(_twice "")
        foreach (_line IN LISTS _optionals _built)
            if (_line IN_LIST _seen)
                list(APPEND _twice "${_line}")
            else ()
                list(APPEND _seen "${_line}")
            endif ()
        endforeach ()
        foreach (_line IN LISTS _seen)
            if (NOT _line IN_LIST _twice)
                list(APPEND _filtered "${_line}")
            endif ()
        endforeach ()
        string(REPLACE ";" " " _filtered "${_filtered}")

        # Both files are read while configuring, so CMake has to run again when
        # either changes; nothing else would notice.
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
            "${_comp_COMPONENT}" "${_comp_IMPLEMENTATION}")
    endif ()

    set(_out "${CMAKE_CURRENT_BINARY_DIR}/components/${name}.component")

    # Recorded so a CppunitTest anywhere else in the tree can find this
    # component's built .component file by name -- librestudio_add_cppunit_
    # test()'s COMPONENTS parameter (cmake/LibreStudioTest.cmake) looks it up
    # this way, the same role gb_CppunitTest_use_components plays upstream by
    # resolving a component name against whichever module's gb_Library_set_
    # componentfile declared it. Named like the per-RDB LIBRESTUDIO_
    # COMPONENTS_<rdb> property above, but keyed by component name instead of
    # RDB name and holding a single path rather than a list: a CppunitTest
    # looks a component up by the name given as librestudio_add_component()'s
    # own first argument, never by which RDB (if any) it is also routed to,
    # and a given name is only ever declared once, so there is nothing to
    # APPEND here.
    #
    # This is purely additive metadata -- it does not change what
    # librestudio_add_component() builds or how "services"/other RDBs are
    # packed, only makes the already-computed ${_out} path discoverable by
    # name from outside this directory scope.
    #
    # Real ordering constraint this creates: get_property(GLOBAL ...) reads
    # whatever has been set by the exact point it is called, so a consumer
    # can only look up "LIBRESTUDIO_COMPONENT_FILE_${name}" successfully if
    # this librestudio_add_component() call has already run -- i.e. the
    # owning module's add_subdirectory() must textually precede the
    # consuming librestudio_add_cppunit_test() call. See cmake/
    # LibreStudioTest.cmake's COMPONENTS handling for how a consumer that
    # cannot satisfy this at its own natural position works around it.
    set_property(GLOBAL PROPERTY "LIBRESTUDIO_COMPONENT_FILE_${name}" "${_out}")

    # The uri is $<TARGET_FILE_NAME:...> rather than a hardcoded name so that it
    # follows the naming rules in LibreStudioPlatform.cmake — a component that
    # points at the wrong filename is a run-time failure with no build-time
    # symptom, which is exactly the kind of thing to avoid guessing at.
    add_custom_command(
        OUTPUT "${_out}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/components"
        COMMAND "${XSLTPROC_EXECUTABLE}" --nonet
                --stringparam uri "${_prefix}$<TARGET_FILE_NAME:${_comp_LIBRARY}>"
                --stringparam cppu_env "${LIBRESTUDIO_CPPU_ENV}"
                --stringparam filtered "${_filtered}"
                -o "${_out}"
                "${LIBRESTUDIO_CREATE_COMPONENT_XSLT}"
                "${_comp_COMPONENT}"
        DEPENDS "${_comp_COMPONENT}" ${_comp_LIBRARY} ${_comp_IMPLEMENTATION}
        COMMENT "createcomponent.xslt: ${name}.component"
        VERBATIM)

    add_custom_target(${name}_component DEPENDS "${_out}")
    add_dependencies(${name}_component ${_comp_LIBRARY})

    if (_comp_RDB)
        set_property(GLOBAL APPEND PROPERTY "LIBRESTUDIO_COMPONENTS_${_comp_RDB}" "${_out}")
    else ()
        set_property(GLOBAL APPEND PROPERTY LIBRESTUDIO_COMPONENTS "${_out}")
    endif ()
endfunction ()

# librestudio_finalize_components(<rdb-name> [OUTPUT <file.rdb>])
#
# Packs every component declared with librestudio_add_component() -- via a
# matching RDB <rdb-name>, or via no RDB at all when rdb-name is "services"
# -- into a single XML registry. Must be called after every component that
# belongs in it has been declared: for "services" that means after every
# module (from the root CMakeLists.txt); a private registry like
# "uno_services" can instead be finalized right at the end of the one
# module that owns it, since all of its components are declared there.
#
# OUTPUT defaults to the same build-tree path this function has always used
# (${CMAKE_BINARY_DIR}/registry/<rdb-name>.rdb); pass it explicitly to match
# gbuild's own per-Rdb path convention ($(WORKDIR)/Rdb/<name>.rdb) for a
# registry that isn't the main app's. Unlike the main "services" registry,
# a private one gets no separate install step here -- that copy into
# LIBRESTUDIO_PROGRAM_DIR is the root CMakeLists.txt's own doing for
# "services" specifically, not something this function does for any name.
function(librestudio_finalize_components rdb_name)
    cmake_parse_arguments(_fin "" "OUTPUT" "" ${ARGN})

    if (rdb_name STREQUAL "services")
        get_property(_components GLOBAL PROPERTY LIBRESTUDIO_COMPONENTS)
    else ()
        get_property(_components GLOBAL PROPERTY "LIBRESTUDIO_COMPONENTS_${rdb_name}")
    endif ()

    if (NOT _components)
        message(FATAL_ERROR
            "librestudio_finalize_components(${rdb_name}): no components were declared")
    endif ()

    # packcomponents.xslt reads an XML list of filenames and concatenates the
    # single <component> element of each, so the list is materialised as a file
    # rather than passed as arguments.
    set(_list "${CMAKE_BINARY_DIR}/components-${rdb_name}.list.xml")
    set(_content "<list>\n")
    foreach (_component IN LISTS _components)
        string(APPEND _content "  <filename>${_component}</filename>\n")
    endforeach ()
    string(APPEND _content "</list>\n")
    file(WRITE "${_list}" "${_content}")

    if (_fin_OUTPUT)
        set(_rdb "${_fin_OUTPUT}")
    else ()
        set(_rdb "${CMAKE_BINARY_DIR}/registry/${rdb_name}.rdb")
    endif ()
    get_filename_component(_rdb_dir "${_rdb}" DIRECTORY)

    add_custom_command(
        OUTPUT "${_rdb}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_rdb_dir}"
        COMMAND "${XSLTPROC_EXECUTABLE}" --nonet -o "${_rdb}"
                "${LIBRESTUDIO_PACK_COMPONENTS_XSLT}" "${_list}"
        DEPENDS ${_components} "${_list}" "${LIBRESTUDIO_PACK_COMPONENTS_XSLT}"
        COMMENT "packcomponents.xslt: ${rdb_name}.rdb"
        VERBATIM)

    add_custom_target(${rdb_name}_rdb ALL DEPENDS "${_rdb}")
    set(${rdb_name}_RDB "${_rdb}" CACHE INTERNAL "${rdb_name} service registry")
endfunction ()
