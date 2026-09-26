# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# CMake-script transcription of unoidl/CustomTarget_unoidl-check_test.mk, run
# as a CTest test body (see the add_test() call in unoidl/CMakeLists.txt,
# gated the same ENABLE_ODK way the .mk's own recipe was gated on the ODK
# BUILD_TYPE filter).
#
# What it does: builds an .rdb registry from each of the 8 fixture .idl files
# in unoidl/qa/unoidl-check/ (via unoidl-write), then runs unoidl-check
# (--ignore-unpublished or not) with each of {psa,usa}.rdb as the reference
# registry against every one of the 8 .rdb files in turn, asserting the exact
# pass/fail outcome the .mk hardcodes for each combination. It is a real
# regression test for unoidl-check's publish-compatibility logic (published
# vs. unpublished types, added/removed/changed members), not a build step
# that produces an artifact anything else consumes -- hence CTest add_test()
# rather than a plain custom target that always runs.
#
# Invoked as:
#   cmake -DUNOIDL_WRITE=<path> -DUNOIDL_CHECK=<path> -DQA_DIR=<path>
#         -DOUT_DIR=<path> -P unoidl-check_test.cmake

foreach (_var UNOIDL_WRITE UNOIDL_CHECK QA_DIR OUT_DIR)
    if (NOT DEFINED ${_var})
        message(FATAL_ERROR "unoidl-check_test.cmake: -D${_var}=... is required")
    endif ()
endforeach ()

file(MAKE_DIRECTORY "${OUT_DIR}")

# unoidl-write turns each fixture .idl into an .rdb (CustomTarget_unoidl-
# check_test.mk's %.rdb pattern rule).
set(_fixtures other1 other2 pe psa psb ue usa usb)
foreach (_name IN LISTS _fixtures)
    execute_process(
        COMMAND "${UNOIDL_WRITE}" "${QA_DIR}/${_name}.idl" "${OUT_DIR}/${_name}.rdb"
        RESULT_VARIABLE _rc
        OUTPUT_VARIABLE _out
        ERROR_VARIABLE _err)
    if (NOT _rc EQUAL 0)
        message(FATAL_ERROR
            "unoidl-write failed to build ${_name}.rdb (exit ${_rc}):\n${_out}${_err}")
    endif ()
endforeach ()

# _check(<TRUE|FALSE expect-success> <unoidl-check args...>): runs
# unoidl-check and fails the whole test (message(FATAL_ERROR) aborts the
# script, matching the .mk's own "|| (cat $@.log && false)" abort-on-mismatch
# behaviour) if the exit code does not match what was expected.
function(_check expect_pass)
    execute_process(COMMAND "${UNOIDL_CHECK}" ${ARGN}
        RESULT_VARIABLE _rc
        OUTPUT_VARIABLE _out
        ERROR_VARIABLE _err)
    if (expect_pass AND NOT _rc EQUAL 0)
        message(FATAL_ERROR
            "expected PASS but got exit ${_rc}: unoidl-check ${ARGN}\n${_out}${_err}")
    elseif (NOT expect_pass AND _rc EQUAL 0)
        message(FATAL_ERROR
            "expected FAIL but got exit 0: unoidl-check ${ARGN}\n${_out}${_err}")
    endif ()
endfunction()

set(_r "${OUT_DIR}")

# Group 1 (.mk lines 37-76): reference psa.rdb, --ignore-unpublished --
# only an exact self-match passes.
_check(TRUE  --ignore-unpublished "${_r}/psa.rdb" -- "${_r}/psa.rdb")
_check(FALSE --ignore-unpublished "${_r}/psa.rdb" -- "${_r}/psb.rdb")
_check(FALSE --ignore-unpublished "${_r}/psa.rdb" -- "${_r}/usa.rdb")
_check(FALSE --ignore-unpublished "${_r}/psa.rdb" -- "${_r}/usb.rdb")
_check(FALSE --ignore-unpublished "${_r}/psa.rdb" -- "${_r}/pe.rdb")
_check(FALSE --ignore-unpublished "${_r}/psa.rdb" -- "${_r}/ue.rdb")
_check(FALSE --ignore-unpublished "${_r}/psa.rdb" -- "${_r}/other1.rdb")
_check(FALSE --ignore-unpublished "${_r}/psa.rdb" -- "${_r}/other2.rdb")

# Group 2 (.mk lines 77-116): reference psa.rdb, strict -- same, only the
# exact self-match passes.
_check(TRUE  "${_r}/psa.rdb" -- "${_r}/psa.rdb")
_check(FALSE "${_r}/psa.rdb" -- "${_r}/psb.rdb")
_check(FALSE "${_r}/psa.rdb" -- "${_r}/usa.rdb")
_check(FALSE "${_r}/psa.rdb" -- "${_r}/usb.rdb")
_check(FALSE "${_r}/psa.rdb" -- "${_r}/pe.rdb")
_check(FALSE "${_r}/psa.rdb" -- "${_r}/ue.rdb")
_check(FALSE "${_r}/psa.rdb" -- "${_r}/other1.rdb")
_check(FALSE "${_r}/psa.rdb" -- "${_r}/other2.rdb")

# Group 3 (.mk lines 117-156): reference usa.rdb, --ignore-unpublished --
# every object registry passes against the unpublished-tolerant reference.
_check(TRUE --ignore-unpublished "${_r}/usa.rdb" -- "${_r}/psa.rdb")
_check(TRUE --ignore-unpublished "${_r}/usa.rdb" -- "${_r}/psb.rdb")
_check(TRUE --ignore-unpublished "${_r}/usa.rdb" -- "${_r}/usa.rdb")
_check(TRUE --ignore-unpublished "${_r}/usa.rdb" -- "${_r}/usb.rdb")
_check(TRUE --ignore-unpublished "${_r}/usa.rdb" -- "${_r}/pe.rdb")
_check(TRUE --ignore-unpublished "${_r}/usa.rdb" -- "${_r}/ue.rdb")
_check(TRUE --ignore-unpublished "${_r}/usa.rdb" -- "${_r}/other1.rdb")
_check(TRUE --ignore-unpublished "${_r}/usa.rdb" -- "${_r}/other2.rdb")

# Group 4 (.mk lines 157-196): reference usa.rdb, strict -- only psa.rdb
# (published subset of usa) and usa.rdb itself pass.
_check(TRUE  "${_r}/usa.rdb" -- "${_r}/psa.rdb")
_check(FALSE "${_r}/usa.rdb" -- "${_r}/psb.rdb")
_check(TRUE  "${_r}/usa.rdb" -- "${_r}/usa.rdb")
_check(FALSE "${_r}/usa.rdb" -- "${_r}/usb.rdb")
_check(FALSE "${_r}/usa.rdb" -- "${_r}/pe.rdb")
_check(FALSE "${_r}/usa.rdb" -- "${_r}/ue.rdb")
_check(FALSE "${_r}/usa.rdb" -- "${_r}/other1.rdb")
_check(FALSE "${_r}/usa.rdb" -- "${_r}/other2.rdb")

message(STATUS "unoidl-check_test: all 32 unoidl-check assertions matched expectations")
