# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# opencl -- the OpenCL device-selection/config/wrapper layer sc's calc
# formula engine (and other consumers) build their GPU-accelerated code
# paths on: picks a usable OpenCL platform/device, reads and writes the
# officecfg allow/deny-list configuration that governs which devices are
# used, and wraps clew's raw function pointers in a slightly higher-level
# API. Two targets, from Module_opencl.mk: Library_opencl and
# Executable_opencltest.

# --- opencl ------------------------------------------------------------------

set(OPENCL_SOURCES
    opencl/source/openclconfig
    opencl/source/openclwrapper
    opencl/source/opencl_device
    opencl/source/platforminfo
    opencl/source/OpenCLZone
)

librestudio_expand_cxx(OPENCL_SOURCES)
add_library(opencl SHARED ${OPENCL_SOURCES})

# gb_Library_use_custom_headers,opencl,officecfg/registry: OpenCLZone.cxx,
# openclconfig.cxx and openclwrapper.cxx all call
# officecfg::Office::Common::Misc::UseOpenCL::get()/set() and
# OpenCLDenyList::get()/set() / OpenCLAllowList::get()/set() directly, so
# the generated officecfg:: headers are a real, load-bearing dependency
# here, not a leftover from the .mk -- same shape as comphelper's use of
# officecfg (comphelper/CMakeLists.txt).
target_include_directories(opencl PRIVATE
    "${CMAKE_SOURCE_DIR}/opencl/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(opencl officecfg_headers
    udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(opencl PRIVATE OPENCL_DLLIMPLEMENTATION)

target_link_libraries(opencl PRIVATE
    comphelper cppu sal salhelper tl
    clew
    ICU::uc ICU::i18n
    rt)

# gb_Library_add_libs,opencl,-lrt lives in an `ifeq ($(OS),LINUX)` block in
# Library_opencl.mk; this is a Linux-only port, so it applies
# unconditionally (linked above alongside the rest, matching how
# sc/CMakeLists.txt links its own Linux-only "rt" the same way).

# No gb_Library_set_componentfile in Library_opencl.mk -- opencl is a plain
# helper library, not itself a UNO service, so no .component file/
# librestudio_add_component() call here (confirmed: no *.component file
# anywhere under opencl/).

# Repository.mk registers opencl under
# gb_Helper_register_libraries_for_install,OOOLIBS,ooo (line 460:
# `$(call gb_Helper_optional,OPENCL,opencl)`), the very same block clew is
# registered in above -- so LAYER OOO here too, i.e. libopencllo.so.
librestudio_set_library_output_name(opencl opencl OOO)

install(TARGETS opencl LIBRARY DESTINATION program)

# --- opencltest ----------------------------------------------------------------

# A standalone diagnostic tool (probes and reports on available OpenCL
# platforms/devices via clew); no officecfg/UNO API use at all
# (Executable_opencltest.mk has no gb_Executable_use_sdk_api call, and
# main.cxx only includes clew/clew.h, clew_setup.hxx and sal/log.hxx),
# unlike the opencl library above.
set(OPENCLTEST_SOURCES
    opencl/opencltest/main
)
librestudio_expand_cxx(OPENCLTEST_SOURCES)
add_executable(opencltest ${OPENCLTEST_SOURCES})

target_include_directories(opencltest PRIVATE
    "${CMAKE_SOURCE_DIR}/opencl/inc")

target_link_libraries(opencltest PRIVATE clew sal)

# gb_Executable_add_default_nativeres,opencltest embeds a Windows
# icon/version resource; not applicable on this Linux-only port, so
# omitted (no other Executable target in this port ports it either).

install(TARGETS opencltest RUNTIME DESTINATION program)
