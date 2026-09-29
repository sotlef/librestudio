# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# static -- statically-linked-components support for Emscripten/WASM and
# similar single-binary targets (a Library_components combining every UNO
# service's own component-registration code directly into one archive,
# bypassing the usual dlopen()-a-.so-per-component model this port's own
# shared-library targets all use).
#
# Module_static.mk, read in full: every gb_Module_add_targets call in it is
# gated, and none of the gates hold for this port's own default Linux/
# dynamic-linking configuration:
#   - CustomTarget_components + Library_components need
#     ENABLE_CUSTOMTARGET_COMPONENTS=TRUE -- grepped this whole port for the
#     variable: zero hits anywhere, it has never been introduced, so it is
#     unset/false here, matching gbuild's own default (an opt-in switch,
#     not something a normal build enables).
#   - CustomTarget_emscripten_fs_image / CustomTarget_unoembind /
#     Package_favicon / Package_unoembind / StaticLibrary_unoembind
#     additionally need OS=EMSCRIPTEN, nested inside the already-false
#     ENABLE_CUSTOMTARGET_COMPONENTS block above.
#   - Executable_embindmaker / Executable_wasmbridgegen need
#     BUILD_TYPE_FOR_HOST to contain EMSCRIPTEN.
# This port targets real Linux/Windows/macOS/etc. dynamic linking (see
# cmake/LibreStudioPlatform.cmake's own 13-platform RTL_OS/LIBRESTUDIO_CPPU_ENV
# switch) and has no Emscripten/WASM build path at all yet -- so, exactly
# like android/Module_android.mk and ios/Module_ios.mk, this module
# contributes zero targets on every platform this port currently supports,
# verified directly from the live .mk rather than assumed out of scope.
#
# A future Emscripten/WASM target for this port (CMAKE_SYSTEM_NAME STREQUAL
# "Emscripten", Emscripten's own CMake toolchain file being the idiomatic
# CMake mechanism here, not a bespoke one) would need this module for real;
# left with no active targets until then.

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    message(STATUS
        "static: CMAKE_SYSTEM_NAME is Emscripten, but this port has no "
        "Emscripten/WASM build path yet (no unoembind/wasmbridgegen/"
        "single-static-binary components support) -- this module's real "
        "targets are not wired.")
endif ()
