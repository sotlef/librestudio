# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# solenv -- gbuild's OWN build-system internals: dmake-era compatibility
# tools, gbuild's dependency-merger, its own IDE/tooling JSON exporter, its
# build-directory lockfile, and (Windows-only) a gcc/g++-as-cl.exe wrapper.
# None of it is "the office's source code" -- every target here exists only
# to make gbuild (a GNU Make-based build system) work at all, or to test
# gbuild's own internal tooling.
#
# Module_solenv.mk, read in full, has five gb_Module_add_*_targets calls:
#
#   1. gb_Module_add_targets_for_build(solenv, Executable_concat-deps
#      Executable_gbuildtojson) -- unconditional. concat-deps
#      (solenv/Executable_concat-deps.mk -> solenv/bin/concat-deps.cxx) merges
#      the .d dependency files GCC/Clang emit per translation unit into the
#      single combined dependency file gbuild's own Makefile re-include logic
#      wants. gbuildtojson (solenv/Executable_gbuildtojson.mk ->
#      solenv/gbuildtojson/gbuildtojson.cxx) walks gbuild's own target graph
#      and emits it as JSON for external tooling (IDE project generators,
#      etc.) to consume. Both are gbuild-internal build-graph tooling with a
#      direct, already-native CMake equivalent: CMake/Ninja track
#      per-translation-unit dependencies itself (via compiler-emitted depfiles
#      or MSVC /showIncludes, wired into the generated build.ninja/Makefile
#      without any user-visible merge step), and CMake already emits its own
#      build-graph description for external tooling
#      (compile_commands.json via CMAKE_EXPORT_COMPILE_COMMANDS, --graphviz=,
#      the file-based API in CMakeFiles/api/ that every IDE/language-server
#      integration already consumes). Nothing needs porting here: this is the
#      "CMake already has its own way to do this" case, not a missing
#      transcription.
#
#   2. ifeq (,$(LOCKFILE)) gb_Module_add_targets_for_build(solenv,
#      $(call gb_CondExeLockfile,Executable_lockfile)) -- builds
#      solenv/Executable_lockfile.mk (solenv/lockfile/{dotlockfile,lockfile}.c)
#      only when no system `lockfile` binary was found at configure time
#      (upstream's LOCKFILE variable, from configure.ac's AC_PATH_PROG). This
#      is gbuild's own build-directory concurrency guard (used by gbuild's
#      dmake-compatibility layer and by solenv/bin scripts that serialize
#      access to shared build state) -- Ninja/CMake's own build systems
#      already serialize/parallelize the build graph correctly without a
#      separate advisory-lock helper; this port has no dmake layer for it to
#      guard in the first place.
#
#   3. ifeq ($(COM),MSC) gb_Module_add_targets(solenv, StaticLibrary_wrapper
#      Executable_gcc-wrapper Executable_g++-wrapper) -- COM is gbuild's
#      compiler-family switch (MSC = MSVC's cl.exe); these three targets
#      (solenv/gcc-wrappers/{wrapper,gcc,gcc}.cxx) let an MSVC-hosted build
#      invoke gcc/g++ as a drop-in cl.exe replacement for the handful of
#      places upstream still shells out to a GNU-style compiler under
#      Windows. This port's own COM/compiler-family switch lives in
#      cmake/LibreStudioPlatform.cmake (its own RTL_OS/LIBRESTUDIO_CPPU_ENV
#      13-platform table); it is a real Linux-x86_64 GCC/Clang build, COM is
#      never MSC here, and this whole target group is inert even on the
#      Windows path this port doesn't build yet (it is Visual-Studio-only
#      glue, orthogonal to whether Windows support exists).
#
#   4. ifneq ($(DISABLE_PYTHON),TRUE) ifneq ($(OS),WNT) ifneq ($(OS),MACOSX)
#      gb_Module_add_subsequentcheck_targets(solenv, CustomTarget_gbuildtesttools
#      PythonTest_solenv_python) -- unlike the three groups above, this gate
#      genuinely HOLDS on a normal Linux build (Python is not disabled here,
#      and OS is neither WNT nor MACOSX): the real upstream module builds
#      this pair. But both are gbuild-testing-itself, not office code:
#      CustomTarget_gbuildtesttools (solenv/CustomTarget_gbuildtesttools.mk)
#      emits a JSON blob naming gbuild's own $(MAKE)/$(shell command -v bash)/
#      the gbuildtojson executable from group 1 above, and
#      PythonTest_solenv_python (solenv/PythonTest_solenv_python.mk) runs
#      solenv/qa/python/gbuildtojson.py against that JSON to test gbuild's
#      own gbuildtojson tool end-to-end. There is nothing to port: gbuildtojson
#      itself is the already-superseded tool from group 1 (CMake has its own
#      build-graph export), so a test that only exercises it has no object
#      left to test in this build. Separately, and even if it did: this port
#      has no PythonTest runner anywhere -- the same already-established gap
#      as e.g. comphelper's JunitTest_comphelper_complex/
#      PythonTest_comphelper_python (see comphelper/CMakeLists.txt) or
#      xmloff/xmlsecurity's equivalents -- no JUnit-style or Python-test
#      harness exists in this CMake port, so PythonTest_solenv_python could
#      not run even if it had something left to test.
#
#   5. ifneq ($(COMPILER_PLUGINS),) ifeq ($(COMPILER_EXTERNAL_TOOL)$(COMPILER_PLUGIN_TOOL),)
#      gb_Module_add_check_targets(solenv, CompilerTest_compilerplugins_clang
#      CompilerTest_compilerplugins_clang-c++03) -- COMPILER_PLUGINS gates
#      upstream's own loplugin Clang static-analysis plugin suite
#      (compilerplugins/), an opt-in developer tool upstream itself does not
#      enable by default. Grepped this whole port for COMPILER_PLUGINS/
#      loplugin/compilerplugins: zero hits anywhere -- this port has never
#      introduced the option, so the gate is false here exactly as it is on
#      a default upstream build, not a platform restriction.
#
# Every target this module could contribute on this Linux/CMake configuration
# is therefore either an already-superseded piece of gbuild's own tooling, an
# inert non-Linux/non-MSVC gate, or blocked on the same PythonTest-runner gap
# already tracked elsewhere in this port. Nothing to wire.
