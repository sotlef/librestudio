# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# ios -- the iOS app shell (Objective-C/Swift UI, Xcode project generation).
#
# Module_ios.mk, read in full: its own gb_Module_add_targets call
# (CustomTarget_iOS_setup) is itself wrapped in "ifeq ($(OS),iOS)" -- on any
# non-iOS OS (this Linux port included), the real upstream module
# contributes exactly zero targets, the same verified-empty shape as
# android/Module_android.mk (see that module's own CMakeLists.txt comment
# for the identical reasoning).
#
# CustomTarget_iOS_setup generates/updates an Xcode project
# (ios/experimental/... .xcodeproj) around the native libraries this port's
# CMake tree produces -- a real iOS build would cross-compile this same
# CMake tree with CMake's own iOS toolchain support (CMAKE_SYSTEM_NAME
# STREQUAL "iOS", a real cross-compilation target CMake understands
# natively) and then use Xcode/xcodebuild for the app shell and code
# signing, mirroring upstream's own two-build-systems split, not a gap
# specific to this port.
#
# Left with no active targets, matching upstream's own real behaviour on
# this platform exactly, until a real iOS toolchain cross-build is
# attempted.

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    message(STATUS
        "ios: CMAKE_SYSTEM_NAME is iOS, but this port does not yet drive "
        "CustomTarget_iOS_setup's Xcode project generation -- only the "
        "native library cross-build (the rest of this CMake tree, built "
        "against CMake's own iOS toolchain support) is in scope so far.")
endif ()
