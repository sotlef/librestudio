# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# unotest — C++ CppunitTest fixture helpers (bootstrap a UNO component context,
# resolve test data directories, drive filter-import/export round trips, launch
# and talk to a separate soffice instance).  Source list from
# Library_unotest.mk.  Every remaining production module that names "unotest"
# in its dependencies (sc, sd, sdext, sw, hwpfilter, pyuno, postprocess,
# slideshow, writerperfect, xmlsecurity, uui, fpicker, desktop) only needs it
# for that module's *own* CppunitTest targets, so this library is what those
# eventually build against — not something the production sc/sw/... libraries
# themselves link.
#
# Out of scope for this tier, and why:
#   - Jar_test.mk / Jar_test-tools.mk: the Java side (org.openoffice.test.*)
#     used by subsequentcheck.  Needs OOO_JUNIT_JAR and an OOoRunner jar this
#     port does not build; nothing here depends on it.
#   - Library_embindtest.mk: an Emscripten/embind smoke-test component, not
#     applicable to a native Linux build.
#
# Library_unobootstrapprotector.mk / Library_unoexceptionprotector.mk -- NOW
# PORTED below (unobootstrapprotector / unoexceptionprotector targets).  They
# used to be listed here as out of scope ("this port has no CppunitTest
# framework yet") back when cmake/LibreStudioTest.cmake's
# librestudio_add_cppunit_test() PROTECTORS parameter was still unused
# plumbing (see that file's header comment).  comphelper -- the one library
# both protectors link against beyond cppu/cppuhelper/sal -- is now ported at
# this test-infrastructure layer (comphelper/CMakeLists.txt), which is what
# unblocks them here.  CppunitTest.mk still wires them in itself upstream
# (nothing in the .mk tree names them through use_libraries); this port's
# equivalent wiring is the PROTECTORS argument of librestudio_add_cppunit_test(),
# used by comphelper_test (comphelper/CMakeLists.txt) and cppu_typelib
# (cppu/CMakeLists.txt).
#
# Runtime note carried forward from the old comment: Library_unobootstrap
# protector.mk's gb_Library_use_package(unobootstrapprotector,
# instsetoo_native_setup_ure) packages instsetoo_native's ure tree (unorc/
# uno.ini) next to the protector at install time -- instsetoo_native itself is
# still out of this port's scope (category 4 in the porting survey), so that
# packaging step is not reproduced here.  It may be less load-bearing than it
# looks, though: cmake/LibreStudioUnorc.cmake's librestudio_generate_unorc(),
# built in an earlier platform-abstraction wave, already writes a real unorc
# straight into the program directory at configure time (root CMakeLists.txt:
# librestudio_generate_unorc("${LIBRESTUDIO_PROGRAM_DIR}/unorc")) -- which is
# exactly the file unobootstrapprotector's Prot::Prot() needs
# (defaultBootstrap_InitialComponentContext() -> cppu::getUnoIniUri(), reading
# unorc from next to libcppuhelpergcc3.so). Whether that generated unorc is
# actually sufficient to satisfy this runtime need is not verified here (no
# CppunitTest that uses this protector has been run yet); this is flagged as
# a plausible reason the old blocking reasoning may now be partly stale, not
# resolved.

set(UNOTEST_SOURCES
    unotest/source/cpp/bootstrapfixturebase
    unotest/source/cpp/directories
    unotest/source/cpp/filters-test
    unotest/source/cpp/getargument
    unotest/source/cpp/gettestargument
    unotest/source/cpp/macros_test
    unotest/source/cpp/officeconnection
    unotest/source/cpp/toabsolutefileurl
)

librestudio_expand_cxx(UNOTEST_SOURCES)
add_library(unotest SHARED ${UNOTEST_SOURCES})

# cppunit's headers (TestAssert.h etc.) are used inline by several of the
# fixture .cxx files above; this is the first module in the port that needs
# them.  On this machine cppunit is a system package with a pkg-config file
# and no special include path (headers already sit under /usr/include).
pkg_check_modules(CPPUNIT REQUIRED IMPORTED_TARGET cppunit)

target_include_directories(unotest PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(unotest udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(unotest PRIVATE OOO_DLLIMPLEMENTATION_UNOTEST)
target_link_libraries(unotest PRIVATE
    comphelper cppu cppuhelper sal sb tl utl vcl
    PkgConfig::CPPUNIT
    Boost::boost)

# PLAINLIBS_NONE (see the comment on the two protector targets below for the
# full evidence): bare "libunotest.so", not "unotestlo.so" -- was OOO, fixed.
# PLAINLIBS_NONE is also one of gb_Library_VALIDGROUPS_NOTINSTALLED
# (solenv/gbuild/Helper.mk), so upstream never installs this library either --
# the install() call below was likewise wrong and is removed, not just the
# naming.
librestudio_set_library_output_name(unotest unotest PLAIN)

# unobootstrapprotector / unoexceptionprotector -- cppunit "Protector"
# plug-ins (Library_unobootstrapprotector.mk / Library_unoexceptionprotector.mk,
# 1 source file each).  Like every CppunitTest target built by
# librestudio_add_cppunit_test() (cmake/LibreStudioTest.cmake), these are
# never linked against by anything in the tree -- CppunitTest.mk's own
# gb_CppunitTest_use_ure passes them to cppunittester as
# "--protector <path> <symbol>" pairs, which dlopens them via
# osl::Module::getFunctionSymbol (CppUnit::PlugInManager), exactly the same
# loading model librestudio_add_cppunit_test()'s own PROTECTORS argument
# already implements. So these are CMake MODULE libraries, not SHARED, and
# get no install() rule -- same as librestudio_add_cppunit_test()'s own
# targets, and consistent with their Repository.mk registration:
# PLAINLIBS_NONE is one of gb_Library_VALIDGROUPS_NOTINSTALLED
# (solenv/gbuild/Helper.mk), i.e. this category is deliberately not part of
# the normal install tree upstream either.
#
# Output naming: both are registered in Repository.mk under
# gb_Helper_register_libraries(PLAINLIBS_NONE, ... unobootstrapprotector
# unoexceptionprotector unotest ...) -- the SAME category as unotest above.
# Checked against solenv/gbuild/platform/unxgcc.mk's gb_Library_FILENAMES
# (not assumed from unotest's own OOO choice above): PLAINLIBS_NONE resolves
# to "$(gb_Library_SYSPRE)$(lib)$(gb_Library_PLAINEXT)" -- a bare
# "lib<name>.so", no gb_Library_DLLPOSTFIX ("lo") suffix and no version.
# That is librestudio_set_library_output_name()'s PLAIN case (bare
# OUTPUT_NAME, CMake's own lib/.so defaults do the rest), NOT its OOO case
# (which appends "lo" -- gb_Library_OOOEXT := $(gb_Library_DLLPOSTFIX)$
# (gb_Library_PLAINEXT), used only by gb_Library_OOOLIBS / PLAINLIBS_OOO).
# So: PLAIN here. This same evidence means unotest's own
# "librestudio_set_library_output_name(unotest unotest OOO)" call two lines
# above looks like a second, separate pre-existing bug -- unotest is
# registered in the identical PLAINLIBS_NONE group, so upstream builds it as
# bare "libunotest.so", not "libunotestlo.so" -- but per this task's scope
# that is flagged here only, not fixed.
if (NOT TARGET PkgConfig::CPPUNIT)
    pkg_check_modules(CPPUNIT REQUIRED IMPORTED_TARGET cppunit)
endif ()

set(UNOBOOTSTRAPPROTECTOR_SOURCES
    unotest/source/cpp/unobootstrapprotector/unobootstrapprotector
)
librestudio_expand_cxx(UNOBOOTSTRAPPROTECTOR_SOURCES)
add_library(unobootstrapprotector MODULE ${UNOBOOTSTRAPPROTECTOR_SOURCES})
target_include_directories(unobootstrapprotector PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(unobootstrapprotector udkapi_normal_headers offapi_normal_headers)
target_link_libraries(unobootstrapprotector PRIVATE
    comphelper cppu cppuhelper sal
    PkgConfig::CPPUNIT
    Boost::boost)
librestudio_set_library_output_name(unobootstrapprotector unobootstrapprotector PLAIN)

set(UNOEXCEPTIONPROTECTOR_SOURCES
    unotest/source/cpp/unoexceptionprotector/unoexceptionprotector
)
librestudio_expand_cxx(UNOEXCEPTIONPROTECTOR_SOURCES)
add_library(unoexceptionprotector MODULE ${UNOEXCEPTIONPROTECTOR_SOURCES})
target_include_directories(unoexceptionprotector PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(unoexceptionprotector udkapi_normal_headers offapi_normal_headers)
target_link_libraries(unoexceptionprotector PRIVATE
    comphelper cppu cppuhelper sal
    PkgConfig::CPPUNIT
    Boost::boost)
librestudio_set_library_output_name(unoexceptionprotector unoexceptionprotector PLAIN)
