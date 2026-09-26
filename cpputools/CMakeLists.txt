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
# gcc3_uno is dlopen()ed when the UNO environment is first asked for rather than
# linked — see the note in bridges/CMakeLists.txt — so running this tool needs
# the bridge built even though nothing on its link line says so.
add_dependencies(uno udkapi_comprehensive_headers gcc3_uno)
target_link_libraries(uno PRIVATE cppu cppuhelper sal salhelper)

# Executable_sp2bv.mk: gb_CondExeSp2bv (solenv/gbuild/Conditions.mk) requires
# $(filter WNT,$(OS)) -- sp2bv is a Windows-only tool (used by the Basic IDE)
# and resolves to nothing on this Linux port. Kept structurally present,
# behind if (WIN32), per this port's policy of not silently dropping
# Windows-only gbuild targets -- see this project's own Windows/MSVC research
# on why it cannot actually be built or exercised on this machine (no
# cl.exe/ml64.exe/SDK), and sal/CMakeLists.txt's sal_comtools/
# sal_retry_if_failed for the same pattern. Inert on this Linux build: the
# if (WIN32) block below never executes here.
if (WIN32)
    # gb_Executable_use_api(sp2bv, udkapi): the normal variant is enough here.
    add_executable(sp2bv "${CMAKE_SOURCE_DIR}/cpputools/source/sp2bv/sp2bv.cxx")
    target_include_directories(sp2bv PRIVATE "${udkapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(sp2bv udkapi_normal_headers)
    target_link_libraries(sp2bv PRIVATE cppu cppuhelper sal)
endif ()

# Package_uno_sh.mk: gb_CondExeUno (solenv/gbuild/Conditions.mk) requires
# BUILD_TYPE to contain DESKTOP (true here, HAVE_FEATURE_DESKTOP) and
# DISABLE_DYNLOADING to be unset (true here) -- both hold unconditionally on
# this port, so uno.sh is installed unconditionally too, matching upstream's
# own $(filter-out MACOSX WNT,$(OS)) gate (Linux included).
install(PROGRAMS "${CMAKE_SOURCE_DIR}/cpputools/scripts/uno.sh" DESTINATION program RENAME uno)

install(TARGETS uno RUNTIME DESTINATION program)
if (WIN32)
    install(TARGETS sp2bv RUNTIME DESTINATION program)
endif ()
