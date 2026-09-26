# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# clew -- the OpenCL Extension Wrangler (CLEW): a GLEW-style runtime loader
# for OpenCL. clew/clew.h declares every cl* entry point as a function
# pointer that clewInit() populates by dlopen()-ing the system's real OpenCL
# ICD loader and dlsym()-ing each symbol out of it, so this library never
# links against an actual OpenCL library, only against libdl/librt to do the
# dynamic loading itself (clew.c: on non-Windows it #includes <dlfcn.h> and
# defines CLCC_DYNLIB_OPEN/CLOSE/IMPORT as dlopen(path, RTLD_NOW|
# RTLD_GLOBAL)/dlclose/dlsym).
#
# external/clew/README: "This project doesn't appear to have an obvious
# upstream, so the source files are in our git repo and there is no option
# to use a system library" -- the source is already vendored directly in
# this tree (no UnpackedTarball/ExternalProject, no network fetch needed),
# so this is a plain add_library, same shape as any other in-tree module.
#
# Source list from Library_clew.mk: a single C translation unit
# (gb_Library_add_cobjects, not add_exception_objects), so it is added with
# its explicit .c extension directly rather than through
# librestudio_expand_cxx (which only handles .cxx) -- same precedent as
# pyuno/CMakeLists.txt's pyuno_dlopenwrapper.c and shell/CMakeLists.txt's
# uri-encode.c.

add_library(clew SHARED
    "${CMAKE_SOURCE_DIR}/external/clew/source/clew.c")

# clew/clew.h #errors out if <CL/cl.h> or <CL/cl_platform.h> was already
# included -- it exists to *replace* the real OpenCL headers -- so this
# include path is PUBLIC: consumers (opencl/CMakeLists.txt) must see
# <clew/clew.h> too, without redeclaring the path themselves.
target_include_directories(clew PUBLIC
    "${CMAKE_SOURCE_DIR}/external/clew/source/include")

# gb_Library_add_defs,clew,-DCLEW_BUILD: selects the dllexport branch of
# clew.h's CLEWAPI macro ladder (CLEW_STATIC / CLEW_BUILD, around line 1012
# of clew.h).
target_compile_definitions(clew PRIVATE CLEW_BUILD)

# gb_Library_use_libraries,clew, is empty -- clew has no dependency on any
# other library in this tree. (This comment mirrors the .mk's own comment:
# "This is required for module-deps.pl to produce correct dependencies.")

# gb_Library_add_libs,clew,-ldl -lrt lives in an `ifeq ($(OS),LINUX)` block
# in Library_clew.mk (a separate MACOSX block adds "-framework OpenCL"
# instead); this is a Linux-only port, so the LINUX branch applies
# unconditionally and the MACOSX branch is dropped.
target_link_libraries(clew PRIVATE ${CMAKE_DL_LIBS} rt)

# Repository.mk registers clew under
# gb_Helper_register_libraries_for_install,OOOLIBS,ooo (line 403:
# `$(call gb_Helper_optional,OPENCL,clew)`, sitting in the very same
# OOOLIBS,ooo block as basegfx/sax/svx/editeng/dbtools) -- NOT a
# PLAINLIBS_* block. Per cmake/LibreStudioPlatform.cmake's
# librestudio_set_library_output_name() (OOOLIBS -> "OOO" layer ->
# lib<name>lo.so, mirroring solenv/gbuild/platform/unxgcc.mk's naming
# rule), that makes this LAYER OOO, i.e. libclewlo.so -- not the bare-name
# PLAIN layer pyuno/pyuno_wrapper use (those are registered under
# PLAINLIBS_OOO instead, a different Repository.mk block entirely).
librestudio_set_library_output_name(clew clew OOO)

install(TARGETS clew LIBRARY DESTINATION program)
