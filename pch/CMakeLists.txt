# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# pch -- the "system" precompiled header, shared by every LinkTarget that
# does not generate its own (Library_*.mk files that call
# gb_LinkTarget_use_common_precompiled_header instead of
# gb_*_set_precompiled_header, e.g. sax, the format-detector libraries
# scd/sdd/swd, scaddins, writerperfect's wpftdraw/wpftwriter -- see each
# module's own CMakeLists.txt for the exact target_precompile_headers(...
# REUSE_FROM precompiled_system) call).
#
# precompiled_system itself is transcribed directly from
# pch/StaticLibrary_precompiled_system.mk: a throwaway static library whose
# only job is to compile pch/system_empty.cxx (an empty translation unit)
# against pch/inc/pch/precompiled_system.hxx, producing one .pch file other
# targets can reuse. Nothing links precompiled_system itself -- same as in
# gbuild, where "the libraries themselves are not used" (pch/README.md).
#
# ENABLE_PCH (cmake/LibreStudioConfigHeaders.cmake) defaults OFF, matching
# configure.ac's own Linux default (enable_pch=no unless $_os=WINNT) -- so
# with no override this target compiles system_empty.cxx normally, without
# ever invoking target_precompile_headers, and nothing here changes build
# output. PCH_LEVEL=2 mirrors configure.ac's own GCC default for a bare
# --enable-pch ("system and base headers") -- every precompiled_*.hxx in
# this tree gates its #include blocks behind #if PCH_LEVEL >= N (N=1..4),
# so the level has to be defined wherever a PCH is actually being compiled
# or none of those blocks are seen at all.

add_library(precompiled_system STATIC pch/system_empty.cxx)

if (ENABLE_PCH)
    target_precompile_headers(precompiled_system PRIVATE
        "${CMAKE_SOURCE_DIR}/pch/inc/pch/precompiled_system.hxx")
    target_compile_definitions(precompiled_system PRIVATE
        PCH_LEVEL=2
        BOOST_ALL_NO_LIB)
endif ()
