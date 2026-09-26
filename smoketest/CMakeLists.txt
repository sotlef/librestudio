# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# smoketest — two independent things live in this directory, deliberately
# kept side by side rather than merged:
#
#   - librestudio_smoke (below, unchanged): this port's own hand-written,
#     cheap, in-process UNO-bridge sanity check. Not a translation of any
#     upstream smoketest/*.mk target -- see librestudio_smoke.cxx for what
#     it actually exercises and why building the libraries alone does not
#     establish the same thing. Kept as-is: it is a real, useful, much
#     cheaper check than the full end-to-end test below (no separate
#     soffice.bin process, no ODF document, no Basic macro run).
#
#   - A real port of upstream's own smoketest module (Module_smoketest.mk),
#     which launches a whole separate soffice.bin, loads an ODF document
#     through it, and runs a Basic macro inside that live office --
#     genuinely testing the built office application, not just the UNO
#     core in-process. Six real gbuild targets, ported/deferred below one
#     by one; each subsection says which and why.
#
# Read directly against the live files for this porting pass:
# Module_smoketest.mk, Library_smoketest.mk, CppunitTest_smoketest.mk,
# Zip_smoketestdoc.mk, solenv/gbuild/Zip.mk, smoketest/smoketest.cxx,
# smoketest/smoketest_too.cxx.

add_executable(librestudio_smoke librestudio_smoke.cxx)

target_include_directories(librestudio_smoke PRIVATE
    "${CMAKE_SOURCE_DIR}/cppuhelper/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(librestudio_smoke udkapi_normal_headers offapi_normal_headers)

target_link_libraries(librestudio_smoke PRIVATE cppuhelper cppu sal)
target_compile_definitions(librestudio_smoke PRIVATE LIBO_INTERNAL_ONLY)

install(TARGETS librestudio_smoke RUNTIME DESTINATION program)

if (BUILD_TESTING)
    # The test needs the registries and unorc in place, not just the
    # executable, so it depends on the whole core.
    add_dependencies(librestudio_smoke
        services_rdb program_services_rdb program_types_rdb gcc3_uno officecfg_registry)

    add_test(NAME librestudio_smoke COMMAND librestudio_smoke)
    set_tests_properties(librestudio_smoke PROPERTIES
        WORKING_DIRECTORY "${LIBRESTUDIO_PROGRAM_DIR}")
endif ()

# ==============================================================================
# Library_smoketest.mk
# ==============================================================================
#
# Upstream's own comment on this target: "this is a Library that contains a
# unit test, so it can be packaged" -- i.e. a plain shared library (built
# through gb_Library_Library, never through gb_CppunitTest_CppunitTest),
# whose only oddity is that it links cppunit/unotest and carries the same
# -DCPPUNIT_PLUGIN_EXPORT define every real CppunitTest gets, so the object
# code inside it is suitable for a Java-side test runner to dlopen as a
# cppunit plug-in later (see the Jar_TestExtension/Jar_smoketest deferral
# note at the bottom of this file). It is NOT wired through
# librestudio_add_cppunit_test() here: that helper builds a MODULE with a
# "test_<name>" filename convention matching gb_CppunitTest_get_filename,
# which is the wrong shape and the wrong name for a gb_Library_Library
# target.
#
# use_sdk_api -> normal udkapi/offapi headers, same as every other library
# in this port. use_libraries: cppu cppuhelper sal unotest. use_externals:
# boost_headers cppunit (cppunit resolved via the same pkg_check_modules
# ... IMPORTED_TARGET cppunit unotest/CMakeLists.txt and test/CMakeLists.txt
# already use in this directory scope -- re-run here since an
# IMPORTED_TARGET is only visible in the directory that created it and
# below).
#
# CMake target name: "smoketest_lib", not "smoketest". Upstream can have a
# Library named smoketest and a CppunitTest named smoketest at the same
# time because gbuild keeps them in separate namespaces
# (gb_Library_get_target vs. gb_CppunitTest_get_target); CMake has one flat
# target namespace, and CppunitTest_smoketest below is ported through
# librestudio_add_cppunit_test(smoketest ...), which needs the literal name
# "smoketest" for its own target (so its OUTPUT_NAME "test_smoketest"
# matches gb_CppunitTest_get_filename exactly, the same fidelity
# Module_DLL's hardcoded "libtest_Module_DLL.so" expectation relies on
# elsewhere in this port). Nothing in this tree links against
# Library_smoketest (Jar_TestExtension/Jar_smoketest, its only upstream
# consumers, are deferred below), so renaming *this* side of the collision
# is the one with zero fallout -- the exact same choice test/CMakeLists.txt
# made for its own "test" vs. CTest's built-in "test" target, there calling
# the CMake target test_fixtures while keeping OUTPUT_NAME "test".
pkg_check_modules(CPPUNIT REQUIRED IMPORTED_TARGET cppunit)

set(SMOKETEST_LIB_SOURCES
    smoketest/smoketest
)
librestudio_expand_cxx(SMOKETEST_LIB_SOURCES)

add_library(smoketest_lib SHARED ${SMOKETEST_LIB_SOURCES})

target_include_directories(smoketest_lib PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(smoketest_lib udkapi_normal_headers offapi_normal_headers)

# -DCPPUNIT_PLUGIN_EXPORT='extern "C" SAL_DLLPUBLIC_EXPORT' (Library_smoketest.mk
# line 15) -- necessary, per its own comment there, "because this is no
# CppUnitTest but a Library": librestudio_add_cppunit_test() applies the
# identical define automatically for every MODULE it builds (cmake/
# LibreStudioTest.cmake), but this is a plain add_library(), so it is
# applied by hand here to match.
target_compile_definitions(smoketest_lib PRIVATE
    "CPPUNIT_PLUGIN_EXPORT=extern \"C\" SAL_DLLPUBLIC_EXPORT")

target_link_libraries(smoketest_lib PRIVATE
    cppu cppuhelper sal unotest
    PkgConfig::CPPUNIT
    Boost::boost)

# Repository.mk registers smoketest under PLAINLIBS_NONE (the same block as
# unobootstrapprotector/unoexceptionprotector/unotest/test-setupvcl/test,
# verified again here via grep -n "smoketest" Repository.mk: line 586, the
# gb_Helper_register_libraries,PLAINLIBS_NONE call -- the *other* "smoketest"
# match at line 807 is the unrelated Jar_smoketest registration, not this
# library). unxgcc.mk's gb_Library_FILENAMES maps every PLAINLIBS_NONE
# member to plain "$(gb_Library_SYSPRE)$(lib)$(gb_Library_PLAINEXT)" --
# libsmoketest.so, no "lo" suffix and no version -- exactly this port's
# PLAIN layer (cmake/LibreStudioPlatform.cmake), the same layer already
# used for unotest/unobootstrapprotector/unoexceptionprotector/test-setupvcl/
# test_fixtures. And, like every one of those, PLAINLIBS_NONE is also in
# gb_Library_VALIDGROUPS_NOTINSTALLED (solenv/gbuild/Helper.mk), so upstream
# never installs this library either -- deliberately no install(TARGETS
# smoketest_lib ...) call here, matching that same precedent exactly.
librestudio_set_library_output_name(smoketest_lib smoketest PLAIN)

# ==============================================================================
# Zip_smoketestdoc.mk (via cmake/LibreStudioZip.cmake)
# ==============================================================================
#
# 16 files under smoketest/data/, re-counted directly from the live
# Zip_smoketestdoc.mk (16 gb_Zip_add_file calls) and cross-checked with
# `find smoketest/data -type f` (16 files, same set) -- not the 15 files a
# prior paraphrase of this task suggested.
#
# gb_Zip_add_commandoptions(smoketestdoc,-D) (Zip_smoketestdoc.mk line 15):
# "-D", no directory entries stored in the archive -- reproduced by
# librestudio_add_zip() unconditionally for every caller (see cmake/
# LibreStudioZip.cmake's header comment for the full flag-by-flag mapping
# against solenv/gbuild/Zip.mk).
#
# Output path: gb_Zip_get_target(smoketestdoc) is upstream
# $(WORKDIR)/Zip/smoketestdoc.zip, and CppunitTest_smoketest.mk then adds
# its own rule to copy that to $(WORKDIR)/Zip/smoketestdoc.sxw (an .sxw file
# *is* just a zip archive under a different, StarWriter-specific extension)
# -- two steps only because gb_Zip_get_target hardcodes the ".zip"
# extension. librestudio_add_zip()'s OUTPUT has no such hardcoding, so this
# port skips the intermediate ".zip" name and the copy-rename step
# entirely, writing the ".sxw" file directly.
set(SMOKETESTDOC_FILES
    mimetype
    content.xml
    meta.xml
    settings.xml
    styles.xml
    META-INF/manifest.xml
    Basic/script-lc.xml
    Basic/Standard/script-lb.xml
    Basic/Standard/Events.xml
    Basic/Standard/Global.xml
    Basic/Standard/Test_10er.xml
    Basic/Standard/Test_DB.xml
    Basic/Standard/Test_Ext.xml
    Dialogs/dialog-lc.xml
    Dialogs/Standard/dialog-lb.xml
    Dialogs/Standard/OptionsDlg.xml
)

set(SMOKETESTDOC_OUTPUT "${CMAKE_BINARY_DIR}/Zip/smoketestdoc.sxw")

librestudio_add_zip(smoketestdoc
    SOURCEDIR "${CMAKE_SOURCE_DIR}/smoketest/data"
    OUTPUT "${SMOKETESTDOC_OUTPUT}"
    FILES ${SMOKETESTDOC_FILES})

# ==============================================================================
# CppunitTest_smoketest.mk
# ==============================================================================
#
# The real end-to-end test: SOURCES points at smoketest_too.cxx, which is
# confirmed (read in full) to be the one-line "#include \"smoketest.cxx\""
# wrapper -- so this reuses the exact same Test fixture the Library above
# also compiles, just built into a dlopened CppunitTest MODULE this time
# instead of a plain shared library.
#
# use_libraries: cppu cppuhelper sal unotest (verified against the live
# .mk). use_external boost_headers -> Boost::boost (resolved globally by
# find_package(Boost ...) in the root CMakeLists.txt, before
# add_subdirectory(smoketest) is reached). use_sdk_api -> normal
# udkapi/offapi headers, same convention as every other CppunitTest ported
# so far.
#
# use_ure (verified: this .mk calls use_ure and nothing else -- no
# use_vcl/use_vcl_non_headless* call anywhere in the file) wires exactly
# unobootstrapprotector + unoexceptionprotector (solenv/gbuild/CppunitTest.mk,
# gb_CppunitTest_use_ure, lines ~250-257) -- NOT a third, vcl-flavoured
# protector; that one is only added by the separate use_vcl family, which
# this test does not call. So PROTECTORS gets exactly 2 pairs, in the same
# unobootstrapprotector-then-unoexceptionprotector order sax_attributes and
# cppu_typelib already use (sax/CMakeLists.txt, cppu/CMakeLists.txt) for the
# identical pair -- gbuild's own --protector emission order
# (gb_CppunitTest__make_args) is actually exceptionprotector-then-
# bootstrapprotector, but nothing depends on inter-protector ordering (each
# is registered with cppunittester independently), so this port's own
# established convention is kept rather than flipped to match upstream
# byte-for-byte.
#
# soffice.bin, scratch profile, and the ODF test document: CppunitTest_
# smoketest.mk needs three things no other CppunitTest ported so far has
# needed, none of which librestudio_add_cppunit_test() has a parameter for
# today: a "-env:arg-soffice=" pointing at a real soffice(.bin), a
# "-env:arg-user=" scratch profile directory seeded with
# qadevOOo/qa/registrymodifications.xcu, and a
# "-env:arg-testarg.smoketest.doc=" pointing at the .sxw built above.  These
# are read, at test run time, by unotest/source/cpp/officeconnection.cxx's
# OfficeConnection::setUp() (via getArgument("soffice"/"user"/"env")) and by
# unotest/source/cpp/gettestargument.cxx (via getTestArgument("smoketest.doc"),
# called from smoketest.cxx's own Test::test()) -- not by anything this port
# already models.
#
# Mechanism chosen for the extra "-env:" arguments: librestudio_add_cppunit_
# test() gained a new EXTRA_ARGS parameter (cmake/LibreStudioTest.cmake) that
# appends raw extra arguments, verbatim and last, to the cppunittester
# COMMAND -- the CMake-side equivalent of gb_CppunitTest_add_arguments's
# $(ARGS), which upstream's own gb_CppunitTest__make_args also appends last.
# Chosen over hand-rolling this test's add_test() call the way sot's
# sot_storage_check does (sot/CMakeLists.txt) because sot_storage_check's
# situation is different in kind, not just degree: that test cannot go
# through librestudio_add_cppunit_test() at all (it needs unotest fixtures
# that themselves need vcl, per that file's own comment), so hand-rolling a
# plain add_executable()+add_test() is the only option there. This test, by
# contrast, genuinely *is* a CppunitTest -- a MODULE dlopened by
# cppunittester, needing the same protector wiring, the same generic
# UserInstallation fixture, and the same UNO_SERVICES machinery every other
# CppunitTest in this port already gets from librestudio_add_cppunit_test().
# Hand-rolling it would mean re-implementing all of that a second time for
# one call site. EXTRA_ARGS is purely additive (every existing call site
# omits it and is unaffected) and low-risk: it does not interact with
# anything else the function already builds, so this was judged the safer,
# more maintainable choice.
#
# -env:arg-soffice=: this port's own known, pre-existing gap --
# desktop/CMakeLists.txt's header comment states outright that no
# soffice/scalc/.../swriter shell-wrapper scripts are built (only Package_*
# file-copy targets upstream produce them, and this port does not
# reproduce pure packaging targets). gb_JunitTest_SOFFICEARG's own upstream
# default is "path:$(INSTROOT)/$(LIBO_BIN_FOLDER)/soffice" -- that wrapper
# script, not soffice.bin directly. Since this port has no such script,
# -env:arg-soffice= below points directly at the real application binary
# instead: $<TARGET_FILE:soffice_bin> (desktop/CMakeLists.txt --
# add_executable(soffice_bin ...), OUTPUT_NAME "soffice.bin"; confirmed by
# reading that file). The "path:" prefix is required by
# OfficeConnection::setUp() itself (argSoffice.match("path:") gates the
# whole spawn-a-process branch; the other branch, "connect:", assumes an
# already-running office instead).
#
# CAVEAT (deliberately not silently glossed over): the soffice.sh wrapper
# script this bypasses normally also sets up the runtime environment (in
# upstream's case, mainly LD_LIBRARY_PATH) before exec-ing soffice.bin.
# Going straight to $<TARGET_FILE:soffice_bin> skips whatever that script
# would otherwise have done. This may not matter in practice -- every
# shared library this port builds already gets its rpath baked in by
# CMake's own default rpath handling, which is the main reason a wrapper
# script needs to set a library-search-path variable in the first place --
# but that equivalence is assumed here, not verified against a real run
# (this port has never actually executed soffice.bin end-to-end; see the
# porting memory notes on that open item). If soffice.bin ever fails to
# find a library at runtime under this test, this bypassed wrapper is the
# first place to look.
#
# -env:arg-env=: OfficeConnection::setUp() treats this argument as entirely
# optional (envs stays nullptr, and osl_executeProcess() then leaves
# soffice.bin's environment untouched/inherited, if it is absent) and,
# when present, as a single literal "NAME=VALUE" environment-variable
# override string (envs = &argEnv.pData, count 1) -- not a PATH-style list.
# Upstream's own value
# ($(gb_Helper_LIBRARY_PATH_VAR)"$$$${...+=$$$$...}") is a make/shell
# macro that expands, at build time, to "LD_LIBRARY_PATH" alone if
# LD_LIBRARY_PATH is unset in the invoking shell, or to
# "LD_LIBRARY_PATH=<value>" if it is set -- there is no CMake equivalent of
# that shell parameter-expansion trick available inside a plain add_test()
# COMMAND list (no shell is involved unless one is invoked explicitly).
# Reproduced here as a CMake-configure-time snapshot instead: if
# LD_LIBRARY_PATH is set and non-empty in *this* CMake configure run's
# environment, it is baked into a literal "-env:arg-env=LD_LIBRARY_PATH=..."
# argument; otherwise the flag is omitted entirely (equivalent to
# soffice.bin simply inheriting ctest's own ambient environment unmodified
# -- the sane default, and per the rpath caveat above, likely sufficient).
# Known limitation of this simplification: a LD_LIBRARY_PATH exported only
# at `ctest` run time (not at the earlier `cmake` configure time) will not
# be forwarded to the spawned soffice.bin process. Flagged here rather than
# silently assumed equivalent to upstream's dynamic behaviour.
set(SMOKETEST_SCRATCH_DIR "${CMAKE_BINARY_DIR}/CustomTarget/smoketest")

set(SMOKETEST_EXTRA_ARGS
    "-env:arg-soffice=path:$<TARGET_FILE:soffice_bin>"
    "-env:arg-user=${SMOKETEST_SCRATCH_DIR}"
    "-env:arg-testarg.smoketest.doc=${SMOKETESTDOC_OUTPUT}")
if (DEFINED ENV{LD_LIBRARY_PATH} AND NOT "$ENV{LD_LIBRARY_PATH}" STREQUAL "")
    list(APPEND SMOKETEST_EXTRA_ARGS
        "-env:arg-env=LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}")
endif ()

librestudio_add_cppunit_test(smoketest
    SOURCES
        smoketest/smoketest_too
    LIBRARIES cppu cppuhelper sal unotest Boost::boost
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        soffice_bin smoketestdoc
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
    EXTRA_ARGS ${SMOKETEST_EXTRA_ARGS})

# clean_CustomTarget_smoketest (CppunitTest_smoketest.mk lines 47-50):
# "rm -rf $(WORKDIR)/CustomTarget/smoketest && mkdir -p .../user && cp
# qadevOOo/qa/registrymodifications.xcu .../user" -- a fresh scratch
# UserInstallation profile, reset on every build. qadevOOo itself has no
# CMakeLists.txt and stays out of this port's scope; referencing this one
# static file directly (rather than porting the module) needs nothing else
# from it. Upstream runs this as a build-time make prerequisite of the
# CppunitTest target; reproduced here as a CTest FIXTURES_SETUP test
# instead, which actually goes one better -- it re-seeds the scratch
# directory on every `ctest` run, not only on the next `cmake --build`
# after one.  Wired onto the "smoketest" test's own FIXTURES_REQUIRED via
# set_property(... APPEND ...), so as not to clobber the
# "smoketest_user_installation" fixture librestudio_add_cppunit_test()
# already attaches there for its own generic -env:UserInstallation=
# profile (a different directory, unrelated to this one).
add_test(NAME smoketest_scratch_user
    COMMAND sh -c "rm -rf '${SMOKETEST_SCRATCH_DIR}' && mkdir -p '${SMOKETEST_SCRATCH_DIR}/user' && cp '${CMAKE_SOURCE_DIR}/qadevOOo/qa/registrymodifications.xcu' '${SMOKETEST_SCRATCH_DIR}/user/'")
set_tests_properties(smoketest_scratch_user PROPERTIES
    FIXTURES_SETUP smoketest_scratch_user)
set_property(TEST smoketest APPEND PROPERTY
    FIXTURES_REQUIRED smoketest_scratch_user)

# ==============================================================================
# Deferred: Extension_TestExtension.mk / Jar_TestExtension.mk / Jar_smoketest.mk
# ==============================================================================
#
# All three are ENABLE_JAVA-gated in Module_smoketest.mk and all three are
# explicitly deferred here, not silently dropped:
#
#   - Jar_TestExtension.mk / Extension_TestExtension.mk together build and
#     package a .oxt extension (a TestExtension Java class plus its
#     manifest/description.xml, zipped into an .oxt via the same kind of
#     packaging machinery Zip.mk's siblings in solenv/gbuild provide for
#     extensions specifically -- a different, more involved mechanism than
#     the plain Zip class ported above).
#   - Jar_smoketest.mk builds a services jar consumed by that Java-side
#     test extension.
#
# This is NOT a "Java packaging doesn't exist in this port" gap: this port
# already has working Java jar infrastructure (cmake/LibreStudioJavaJar.cmake,
# with real production call sites in ridljar/CMakeLists.txt and
# bean/CMakeLists.txt). What is genuinely missing is specific to this
# module: the TestExtension Java sources themselves have not been ported,
# and neither has the .oxt extension-packaging step (manifest generation +
# zip-as-oxt) that Extension_TestExtension.mk drives -- LibreStudioJavaJar.cmake
# has no equivalent of that yet, and building one was judged out of scope
# for this pass given the added complexity (a whole second packaging format
# on top of Zip/Jar) relative to how much of the module's real testing value
# they add beyond CppunitTest_smoketest above. Left for a later, dedicated
# wave, same as this port defers other whole categories elsewhere.
