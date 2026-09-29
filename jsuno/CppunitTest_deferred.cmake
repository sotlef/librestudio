# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# jsuno_testuno (CppunitTest_jsuno_testuno.mk) and jsuno_execute
# (CppunitTest_jsuno_execute.mk) -- split out of jsuno/CMakeLists.txt into
# their own file, include()'d from the root CMakeLists.txt only after every
# module either needs has already run. jsuno itself is add_subdirectory()'d
# at line ~941, well before unotest/test (~1017-1018) -- both tests call
# gb_CppunitTest_use_ure, whose protector plug-ins (unobootstrapprotector/
# unoexceptionprotector) come from unotest, so neither test could be
# registered at jsuno/CMakeLists.txt's own position either way. Neither test
# calls use_vcl or use_rdb(services), so program_services_rdb is not a
# constraint for either -- unlike most other deferred files in this port,
# both could in principle be registered as early as right after unotest/test
# (~1018) rather than waiting for program_services_rdb; they are included
# here, alongside the other Wave-adjacent deferred files, purely for
# consistency of where deferred CppunitTest files are collected in the root
# CMakeLists.txt, not because anything forces this exact position.
#
# Because this file runs in the root CMakeLists.txt's own directory scope
# (include() does not create a new directory scope the way add_subdirectory()
# does), not jsuno's, source paths below are still fine: librestudio_expand_
# cxx (cmake/LibreStudioPlatform.cmake) resolves them against CMAKE_SOURCE_DIR
# (the absolute tree root), never CMAKE_CURRENT_SOURCE_DIR. jsuno itself (the
# production library) is a real target by this point too, defined in jsuno/
# CMakeLists.txt, well before this file's own include() point.

# --- CppunitTest_jsuno_testuno -----------------------------------------------
#
# CppunitTest_jsuno_testuno.mk, re-checked directly against the live file: 1
# source (jsuno/qa/unit/testuno), use_libraries (jsuno sal), use_components
# (unotest/source/embindtest/embindtest), use_sdk_api (normal udkapi+offapi),
# use_ure -- NO use_vcl call in this .mk, so only 2 protector pairs
# (unobootstrapprotector, unoexceptionprotector -- solenv/gbuild/
# CppunitTest.mk:250-259's gb_CppunitTest_use_ure adds exactly these two on
# its own; vclbootstrapprotector is only added by gb_CppunitTest__use_vcl,
# never called here).
#
# 2026-09-29, corrected: use_components names "unotest/source/embindtest/
# embindtest" -- Library_embindtest.mk's own .component, now actually ported
# (unotest/CMakeLists.txt, gated on ENABLE_EMBINDTEST_UNO -- see that file's
# own comment for the full story of why the earlier "Emscripten-only, out of
# scope" characterization was wrong). Separately, and just as real: real
# upstream Module_jsuno.mk itself only adds CppunitTest_jsuno_testuno to its
# check targets `$(if $(ENABLE_EMBINDTEST_UNO), CppunitTest_jsuno_testuno)`
# -- CppunitTest_jsuno_execute (below) is the one that always runs. So a bare
# configure of real upstream LibreOffice never runs jsuno_testuno either; this
# port's own prior registration (unconditional, no COMPONENTS, "expected to
# fail") was a genuine gap on TOP of the embindtest one -- confirmed by a real
# ctest run failing exactly where expected (Test.create() with no factory),
# and by reading Module_jsuno.mk directly. Fixed by gating this whole test on
# the same option, matching upstream's own real bare-configure behavior
# exactly (OFF -- jsuno_testuno simply is not part of the default test suite,
# same as real LibreOffice), and wiring the real COMPONENTS entry for anyone
# who does turn ENABLE_EMBINDTEST_UNO on.
#
# All libraries verified as real targets (grep -n "add_library(<name> "):
# jsuno (jsuno/CMakeLists.txt, this module's own production library), sal --
# both resolve. unobootstrapprotector/unoexceptionprotector (unotest/
# CMakeLists.txt) resolve too.
if (ENABLE_EMBINDTEST_UNO)
    librestudio_add_cppunit_test(jsuno_testuno
        SOURCES
            jsuno/qa/unit/testuno
        LIBRARIES
            jsuno sal
        INCLUDE
            "${udkapi_normal_HEADERS_INCLUDE_DIR}"
            "${offapi_normal_HEADERS_INCLUDE_DIR}"
        DEPENDS
            udkapi_normal_headers offapi_normal_headers
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
        COMPONENTS
            embindtest)
endif ()

# testuno.cxx's own appendFileContents() reads SRC_ROOT directly (not via
# BootstrapFixture) to resolve jsuno/qa/extras/*.js -- also now covered by
# librestudio_add_cppunit_test()'s generic SRC_ROOT (cmake/
# LibreStudioTest.cmake), which is required for this test to actually PASS
# (not just to avoid a fixture-constructor crash, unlike the
# BootstrapFixture-derived tests elsewhere in this port).

# --- CppunitTest_jsuno_execute -----------------------------------------------
#
# CppunitTest_jsuno_execute.mk, re-checked directly against the live file: 1
# source (jsuno/qa/unit/execute), use_libraries (jsuno sal), use_sdk_api
# (normal udkapi+offapi), use_ure -- same "no use_vcl, so 2 protector pairs
# only" shape as jsuno_testuno above. No use_components, no use_rdb, no
# use_configuration anywhere in this .mk. execute.cxx's own testReturnValue/
# testThrows only exercise jsuno::execute() (pure QuickJS expression
# evaluation and its ScriptExceptionRaisedException mapping) directly, with
# no UNO component/service lookup at all -- unlike jsuno_testuno above, this
# test has no known missing-infrastructure blocker; it is expected to both
# build and run cleanly.
librestudio_add_cppunit_test(jsuno_execute
    SOURCES
        jsuno/qa/unit/execute
    LIBRARIES
        jsuno sal
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector)
