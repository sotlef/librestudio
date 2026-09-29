# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# unodevtools -- Executable_uno-skeletonmaker, a pure portable C++ command-line
# tool that generates C++/Java UNO-service implementation skeletons from a
# .idl/.rdb type description.
#
# Module_unodevtools.mk, read in full: its one gb_Module_add_targets call
# (Executable_uno-skeletonmaker) is wrapped in
# `ifneq (,$(filter ODK,$(BUILD_TYPE)))` -- an ODK-only tool, the same gate
# already established for unoidl/CMakeLists.txt's unoidl-read (see there for
# why ENABLE_ODK defaults OFF on this port: a bare ./configure without
# --enable-odk never appends ODK to BUILD_TYPE). Genuinely inert by default
# here, but real, portable, dependency-light C++ with nothing platform- or
# toolchain-specific about it -- unlike rust_uno/net_ure it needs no external
# compiler toolchain beyond the one already building this whole port, so it is
# wired for real below rather than left a documentation-only stub.
#
# Executable_uno-skeletonmaker.mk: links the plain libraries sal/salhelper/
# unoidl, plus the static codemaker/codemaker_cpp/codemaker_java libraries
# already built by codemaker/CMakeLists.txt (StaticLibrary_codemaker*.mk) --
# no new dependency to vendor, just wiring to an already-ported sibling.
if (ENABLE_ODK)
    add_executable(uno-skeletonmaker
        "${CMAKE_CURRENT_SOURCE_DIR}/source/unodevtools/options.cxx"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/skeletonmaker/skeletonmaker.cxx"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/skeletonmaker/skeletoncommon.cxx"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/skeletonmaker/javatypemaker.cxx"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/skeletonmaker/cpptypemaker.cxx"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/skeletonmaker/javacompskeleton.cxx"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/skeletonmaker/cppcompskeleton.cxx"
    )
    target_include_directories(uno-skeletonmaker PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/inc"
    )
    target_link_libraries(uno-skeletonmaker PRIVATE
        sal
        salhelper
        unoidl
        codemaker
        codemaker_cpp
        codemaker_java
        Boost::boost
    )
    install(TARGETS uno-skeletonmaker RUNTIME DESTINATION program)
endif ()
