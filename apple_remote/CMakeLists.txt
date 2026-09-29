# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# apple_remote -- a single library, AppleRemote, wrapping Martin Kahr's
# open-source "Apple Remote" framework (IR receiver support for the old
# aluminium/white Apple Remote and Keyspan FrontRow-era remotes) so
# soffice.bin can react to the remote's buttons on macOS (see sd's
# PresenterScreen / RemoteControl consumer -- not touched here, out of this
# module's scope).
#
# Module_apple_remote.mk, read in full:
#
#   $(eval $(call gb_Module_Module,apple_remote))
#   ifeq ($(OS),MACOSX)
#   ifneq ($(ENABLE_MACOSX_SANDBOX),TRUE)
#   $(eval $(call gb_Module_add_targets,apple_remote,\
#       Library_AppleRemote \
#   ))
#   endif
#   endif
#
# i.e. exactly one target, gated on two conditions this Linux port cannot
# satisfy: $(OS) is never MACOSX here (real gate: CMAKE_SYSTEM_NAME STREQUAL
# "Darwin" below), and even on macOS the library is dropped again once the
# App Sandbox is on (ENABLE_MACOSX_SANDBOX, the same real tracked CACHE BOOL
# sal/CMakeLists.txt already declares at line ~445, default OFF -- Apple's
# sandbox forbids the raw HID/IOKit device access this library depends on,
# which is exactly why upstream drops it there instead of just doing nothing
# useful inside the sandbox).
#
# Library_AppleRemote.mk, read in full: 8 Objective-C (.m) sources via
# gb_Library_add_objcobjects (no .mm/.cxx anywhere in this library -- `find
# apple_remote/source -name "*.cxx" -o -name "*.mm"` comes back empty, only
# .m/.h), 3 system Darwin frameworks via gb_Library_use_system_darwin_
# frameworks (Cocoa, Carbon, IOKit), and nothing else: no use_sdk_api, no
# use_libraries, no componentfile -- this is a plain Objective-C library, not
# a UNO service, consumed directly by whatever links it (sd's remote-control
# code) rather than instantiated through the UNO service manager.
#
# Kept structurally present, behind if (APPLE), per this port's policy of
# not silently dropping platform-only gbuild targets (see cpputools/
# CMakeLists.txt's sp2bv for the canonical precedent of this exact pattern,
# there for WIN32). Inert on this Linux build: the whole block below never
# executes here, and even a real macOS build of this port would still need
# ENABLE_MACOSX_SANDBOX left at its OFF default for this target to appear at
# all -- exactly mirroring gbuild's own double gate above.
if (APPLE AND NOT ENABLE_MACOSX_SANDBOX)
    enable_language(OBJC)

    add_library(AppleRemote SHARED
        "${CMAKE_SOURCE_DIR}/apple_remote/source/KeyspanFrontRowControl.m"
        "${CMAKE_SOURCE_DIR}/apple_remote/source/AppleRemote.m"
        "${CMAKE_SOURCE_DIR}/apple_remote/source/RemoteControl.m"
        "${CMAKE_SOURCE_DIR}/apple_remote/source/RemoteControlContainer.m"
        "${CMAKE_SOURCE_DIR}/apple_remote/source/GlobalKeyboardDevice.m"
        "${CMAKE_SOURCE_DIR}/apple_remote/source/HIDRemoteControlDevice.m"
        "${CMAKE_SOURCE_DIR}/apple_remote/source/MultiClickRemoteBehavior.m"
        "${CMAKE_SOURCE_DIR}/apple_remote/source/RemoteMainController.m")

    # gb_Library_use_system_darwin_frameworks,AppleRemote,Cocoa Carbon IOKit --
    # find_library() is CMake's own equivalent of gbuild's darwin-frameworks
    # helper (both ultimately just pass "-framework <Name>" to the linker).
    find_library(APPLE_REMOTE_COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(APPLE_REMOTE_CARBON_FRAMEWORK Carbon REQUIRED)
    find_library(APPLE_REMOTE_IOKIT_FRAMEWORK IOKit REQUIRED)

    target_link_libraries(AppleRemote PRIVATE
        "${APPLE_REMOTE_COCOA_FRAMEWORK}"
        "${APPLE_REMOTE_CARBON_FRAMEWORK}"
        "${APPLE_REMOTE_IOKIT_FRAMEWORK}")

    # No gb_Library_set_componentfile call in Library_AppleRemote.mk (verified:
    # no AppleRemote.component anywhere under apple_remote/) and no
    # gb_Library_use_sdk_api/use_libraries call either -- nothing to install
    # into program/ as a UNO component and no udkapi/offapi include path
    # needed. Left uninstalled here for the same reason cli_ure's native
    # assemblies below are left uninstalled: nothing in this port's own
    # add_subdirectory graph links AppleRemote today (sd's remote-control
    # consumer is a separate, not-yet-audited question), so there is nothing
    # yet that would pull it into an `install()` step; whoever wires a real
    # macOS consumer in should add the corresponding `install(TARGETS
    # AppleRemote LIBRARY DESTINATION program)` alongside that consumer.
endif ()
