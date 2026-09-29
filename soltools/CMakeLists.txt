# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# soltools -- two more of gbuild's own build-time host tools, alongside
# solenv's (see solenv/CMakeLists.txt for the sibling case).
#
# Module_soltools.mk, read in full, has a single unconditional
# gb_Module_add_targets_for_build(soltools, Executable_cpp Executable_makedepend)
# call -- nothing gated, nothing platform-specific to verify away.
#
#   - Executable_cpp (soltools/Executable_cpp.mk, soltools/cpp/_*.c): a
#     bespoke, portable, standalone re-implementation of a C preprocessor,
#     historically needed because upstream once had to preprocess files
#     (.sdi/.src resource scripts, IDL) on platforms/toolchains whose native
#     cc -E either did not exist, behaved incompatibly across compilers, or
#     could not be invoked standalone from a build tool. CMake/Ninja do not
#     need a bundled preprocessor: every compiler this port's own
#     cmake/LibreStudioPlatform.cmake 13-platform table targets (GCC/Clang,
#     and MSVC once wired) ships a real `-E` preprocessor CMake already knows
#     how to invoke, and the specific upstream consumers of this bespoke cpp
#     (dmake's build recipes, old .sdi/.src pipelines) are gbuild/dmake-era
#     infrastructure this port does not replicate in the first place.
#
#   - Executable_makedepend (soltools/Executable_makedepend.mk,
#     soltools/mkdepend/*.c, -DNO_X11 -DXP_PC -DHW_THREADS): a vendored,
#     patched copy of X.Org's classic `makedepend` tool, used by dmake-era
#     recipes to generate header dependency lists before compilers grew their
#     own -M/-MD flags. This is exactly the class of problem CMake/Ninja
#     solve natively and better: every compiler this port targets is invoked
#     with -MD/-MMD (or MSVC's /showIncludes) by CMake's own generated build
#     rules, producing precise, automatically re-read dependency information
#     per translation unit with no separate dependency-scanning pass at all.
#
# Both targets are gbuild/dmake-era build-tooling this CMake port supersedes
# with mechanisms CMake already has natively (compiler-native preprocessing
# and compiler-native dependency generation) -- not office source code, and
# not something a CMake build needs an equivalent of. Nothing to wire.
