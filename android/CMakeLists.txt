# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# android -- the Android app shell (Java/Kotlin UI, JNI glue, Gradle build).
#
# Module_android.mk, read in full: its own gb_Module_add_targets call
# (CustomTarget_lo_android) is itself wrapped in "ifeq ($(OS),ANDROID)" --
# on any non-Android OS (this Linux port included), the real upstream module
# contributes exactly zero targets. This is not a scope guess: it is the
# literal, unconditional behaviour of the live .mk file on this platform,
# verifiable without an Android toolchain.
#
# CustomTarget_lo_android itself is not a small, transcribable CMake unit
# even when OS=ANDROID: it drives a whole separate Gradle/Android-Studio
# project (android/source/...) that builds a Java/Kotlin UI shell around the
# native libraries this port's own CMake tree already produces, via JNI. A
# real Android build of this port would cross-compile the existing CMake
# tree with the Android NDK toolchain (CMAKE_SYSTEM_NAME STREQUAL "Android",
# CMake's own idiomatic cross-compilation mechanism -- an Android NDK
# toolchain file, not something bespoke this project would invent) to
# produce the native .so set, then invoke Gradle separately for the Java/
# Kotlin shell -- two different build systems cooperating, the same shape
# upstream itself uses (gbuild for native, Gradle for the app shell), not a
# gap specific to this port.
#
# Left with no active targets, matching upstream's own real behaviour on
# this platform exactly, until a real Android NDK cross-build is attempted.

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    message(STATUS
        "android: CMAKE_SYSTEM_NAME is Android, but this port does not yet "
        "drive CustomTarget_lo_android's Gradle/Android-Studio app shell -- "
        "only the native library cross-build (the rest of this CMake tree, "
        "built against an Android NDK toolchain file) is in scope so far.")
endif ()
