# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# jvmfwk -- JRE discovery and selection: the "sunmajor" plugin below walks the
# system for installed JREs, matches them against javavendors.xml, and lets a
# caller (jvmaccess, and ultimately jurt/javaunohelper/bean higher up the
# stack) start one according to the jvmfwk3rc/javavendors settings installed
# below. Source list, defs and externals transcribed from Library_jvmfwk.mk;
# gated behind LIBRESTUDIO_ENABLE_JAVA like the rest of the real Java bridge.
#
# JVM_ONE_PATH_CHECK (Library_jvmfwk.mk's conditional
# -DJVM_ONE_PATH_CHECK=...) is set only by a hand-edited developer override,
# never by configure.ac itself (confirmed by grep -- no --with/--enable flag
# assigns it), so it is skipped here the same way other developer-only,
# never-set build overrides are skipped elsewhere in this port.
#
# The ANDROID (lo-bootstrap), WNT (advapi32) and MACOSX (objcxx sources,
# -framework Foundation) branches are all platform-specific to targets this
# Linux-only port does not build, so only the always-on cross-platform part
# of Library_jvmfwk.mk is reproduced below.
#
# valgrind is headers-only, exactly like vcl's own use of it (see the comment
# on VALGRIND_INCLUDE_DIR in vcl/CMakeLists.txt): jvmfwk's only reference is
# the RUNNING_ON_VALGRIND check in
# plugins/sunmajor/pluginlib/sunjavaplugin.cxx, itself guarded behind
# #elif defined HAVE_VALGRIND_HEADERS. find_path() results are cached, so
# this reuses vcl's result when vcl (gated on USING_X11) already ran it
# first; it is looked up again here, unconditionally, because jvmfwk does not
# otherwise depend on vcl and needs the header regardless of USING_X11.
find_path(VALGRIND_INCLUDE_DIR valgrind/valgrind.h)
if (NOT VALGRIND_INCLUDE_DIR)
    message(FATAL_ERROR
        "valgrind headers are required by jvmfwk/plugins/sunmajor/pluginlib/sunjavaplugin.cxx")
endif ()

set(JVMFWK_SOURCES
    jvmfwk/plugins/sunmajor/pluginlib/otherjre
    jvmfwk/plugins/sunmajor/pluginlib/sunjavaplugin
    jvmfwk/plugins/sunmajor/pluginlib/sunjre
    jvmfwk/plugins/sunmajor/pluginlib/sunversion
    jvmfwk/plugins/sunmajor/pluginlib/util
    jvmfwk/plugins/sunmajor/pluginlib/vendorbase
    jvmfwk/plugins/sunmajor/pluginlib/vendorlist
    jvmfwk/source/elements
    jvmfwk/source/framework
    jvmfwk/source/fwkbase
    jvmfwk/source/fwkutil
    jvmfwk/source/libxmlutil
)

librestudio_expand_cxx(JVMFWK_SOURCES)
add_library(jvmfwk SHARED ${JVMFWK_SOURCES})

target_include_directories(jvmfwk PRIVATE
    "${CMAKE_SOURCE_DIR}/jvmfwk/inc"
    "${JNI_INCLUDE_DIRS}"
    "${VALGRIND_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(jvmfwk udkapi_normal_headers)

target_compile_definitions(jvmfwk PRIVATE JVMFWK_DLLIMPLEMENTATION)

target_link_libraries(jvmfwk PRIVATE
    cppu cppuhelper sal salhelper
    LibXml2::LibXml2)

librestudio_set_library_output_name(jvmfwk jvmfwk OOO)

install(TARGETS jvmfwk LIBRARY DESTINATION program)

# --- javaldx ---------------------------------------------------------------
# Executable_javaldx.mk. Upstream only adds this Module_jvmfwk.mk target on
# non-MACOSX/non-WNT platforms with BUILD_TYPE containing DESKTOP -- both
# conditions hold unconditionally for this Linux desktop-only port, so no
# extra gate is needed beyond the module's own LIBRESTUDIO_ENABLE_JAVA one.
add_executable(javaldx
    "${CMAKE_SOURCE_DIR}/jvmfwk/plugins/sunmajor/javaenvsetup/javaldx.cxx")

target_link_libraries(javaldx PRIVATE jvmfwk sal)

install(TARGETS javaldx RUNTIME DESTINATION program)

# --- CustomTarget_jreproperties + Package_jreproperties --------------------
# JREProperties.java queries a candidate JRE's own system properties; jvmfwk
# runs it (via a forked "java -cp ... JREProperties") against every JRE it
# finds, so it ships as a loose .class file next to the office's shared
# libraries (LIBO_URE_LIB_FOLDER), not packed into a jar -- Package_
# jreproperties.mk installs exactly one file, JREProperties.class. On this
# non-MACOSX Linux port LIBO_URE_LIB_FOLDER is "program" (configure.ac's
# non-Darwin/non-WNT branch), matching every other DESTINATION program
# install in this port.
set(JVMFWK_JREPROPERTIES_DIR "${CMAKE_CURRENT_BINARY_DIR}/jreproperties")
set(JVMFWK_JREPROPERTIES_CLASS "${JVMFWK_JREPROPERTIES_DIR}/JREProperties.class")

add_custom_command(
    OUTPUT "${JVMFWK_JREPROPERTIES_CLASS}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${JVMFWK_JREPROPERTIES_DIR}"
    COMMAND "${Java_JAVAC_EXECUTABLE}" -d "${JVMFWK_JREPROPERTIES_DIR}"
            "${CMAKE_SOURCE_DIR}/jvmfwk/plugins/sunmajor/pluginlib/JREProperties.java"
    DEPENDS "${CMAKE_SOURCE_DIR}/jvmfwk/plugins/sunmajor/pluginlib/JREProperties.java"
    COMMENT "javac: jvmfwk/jreproperties"
    VERBATIM)

# ALL, like this file's own program_*_rdb targets in the root CMakeLists.txt:
# nothing else in the build depends on JREProperties.class, so without ALL it
# would never be built for `install` to find.
add_custom_target(jvmfwk_jreproperties ALL DEPENDS "${JVMFWK_JREPROPERTIES_CLASS}")

install(FILES "${JVMFWK_JREPROPERTIES_CLASS}" DESTINATION program)

# --- CustomTarget_jvmfwk_jvmfwk3_ini + Package_jvmfwk_jvmfwk3_ini -----------
# jvmfwk3rc: the [Bootstrap] ini jvmfwk itself reads for the vendor-settings
# path and the user/shared jvmfwk3 data overrides. Static content (no build
# inputs beyond the CustomTarget file itself), so this uses the same
# configure-time file(WRITE) technique as librestudio_generate_unorc() in
# cmake/LibreStudioUnorc.cmake rather than a build-time custom command.
# gb_Helper_get_rcfile,jvmfwk3 = "jvmfwk3rc" on this (non-Windows) platform
# (solenv/gbuild/platform/com_GCC_defs.mk); UNO_JAVA_JFW_VENDOR_SETTINGS's
# path prefix is MACOSX-only in the upstream CustomTarget, so on this Linux
# port it collapses to plain ${ORIGIN}/javavendors.xml.
set(JVMFWK3RC "${CMAKE_CURRENT_BINARY_DIR}/jvmfwk3rc")
file(WRITE "${JVMFWK3RC}"
"[Bootstrap]
UNO_JAVA_JFW_VENDOR_SETTINGS=\${ORIGIN}/javavendors.xml
UNO_JAVA_JFW_SHARED_DATA=\${URE_OVERRIDE_JAVA_JFW_SHARED_DATA}
UNO_JAVA_JFW_USER_DATA=\${URE_OVERRIDE_JAVA_JFW_USER_DATA}
UNO_JAVA_JFW_CLASSPATH_URLS=\${URE_MORE_JAVA_CLASSPATH_URLS}
")

install(FILES "${JVMFWK3RC}" DESTINATION program)

# --- Package_rcfiles ---------------------------------------------------------
# javavendors.xml: which JRE vendors/versions jvmfwk will accept. Upstream
# picks the OS-specific variant (WNT/FREEBSD/MACOSX[-aarch64]/LINUX/other);
# this is a Linux-only port, so only the LINUX branch applies.
install(FILES
    "${CMAKE_SOURCE_DIR}/jvmfwk/distributions/OpenOfficeorg/javavendors_linux.xml"
    DESTINATION program
    RENAME javavendors.xml)

# --- CppunitTest targets -----------------------------------------------------

# jvmfwk's only CppunitTest, CppunitTest_jvmfwk_sunversion.mk, ported via
# librestudio_add_cppunit_test() (cmake/LibreStudioTest.cmake -- read its
# header comment first). Re-checked directly against the live .mk file, not
# just the prior-research summary.
#
# gb_CppunitTest_use_library_objects,jvmfwk_sunversion,jvmfwk
# (solenv/gbuild/CppunitTest.mk -> gb_LinkTarget_use_library_objects,
# LinkTarget.mk:1839-1849) pulls jvmfwk's *already-compiled* object files
# straight into the CppunitTest link line (EXTRAOBJECTLISTS), instead of
# linking against the jvmfwk shared library the way "use_libraries jvmfwk"
# would -- because jvmfwk/qa/unit/sunversion.cxx uses jfw_plugin::SunVersion,
# declared in jvmfwk/plugins/sunmajor/pluginlib/sunversion.hxx, which is an
# internal header (not installed/exported), not part of jvmfwk's public ABI.
# This is the exact same situation sal/CMakeLists.txt already documents for
# CppunitTest_sal_osl_file_details.mk (see the long comment right above its
# librestudio_add_cppunit_test call): CMake has no direct equivalent of
# "relink another target's already-built .o files", so that precedent
# recompiles the *same source list* the production target compiles, directly
# into the CppunitTest MODULE, rather than restructuring the working
# production `jvmfwk` target into an OBJECT library. The same technique is
# reused here: JVMFWK_SOURCES (already expanded to absolute .cxx paths by
# librestudio_expand_cxx above -- librestudio_add_cppunit_test's own
# expansion pass leaves already-absolute entries untouched, per
# LibreStudioTest.cmake's SOURCES doc) is recompiled straight into
# jvmfwk_sunversion, alongside the qa source itself.
#
# Because this recompiles from source rather than reusing prebuilt objects,
# it needs the exact same include dirs and -DJVMFWK_DLLIMPLEMENTATION define
# add_library(jvmfwk ...) above uses -- CppunitTest_jvmfwk_sunversion.mk
# itself declares neither (gbuild didn't need to: it reuses jvmfwk's already
# -- correctly -- compiled .o files), but recompiling the same sources here
# without them would not reproduce the production compile. "-I$(SRCDIR)"
# from the .mk's own gb_CppunitTest_set_include call becomes CMAKE_SOURCE_DIR
# below (needed for the qa file's own
# <jvmfwk/plugins/sunmajor/pluginlib/sunversion.hxx> include).
#
# use_libraries sal salhelper / use_externals libxml2 are exactly what the
# .mk declares -- cppu/cppuhelper are deliberately NOT added even though
# Library_jvmfwk.mk's own use_libraries lists them, because
# CppunitTest_jvmfwk_sunversion.mk itself does not call use_libraries cppu/
# cppuhelper either; this is a faithful transcription of the .mk as written,
# not a guess (this port's global -Wl,-z,defs, see
# cmake/LibreStudioPlatform.cmake, would have caught a real missing symbol
# had one of the recompiled TUs actually needed it and the .mk were wrong
# about that -- but this task does not build/link to confirm it, per this
# port's standing rule never to invoke cmake/cmake --build).
#
# No use_ure/use_vcl*/use_api/use_sdk_api in the .mk, so no PROTECTORS and no
# udkapi wiring beyond what jvmfwk's own recompiled sources need.
librestudio_add_cppunit_test(jvmfwk_sunversion
    SOURCES
        ${JVMFWK_SOURCES}
        jvmfwk/qa/unit/sunversion
    LIBRARIES sal salhelper LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/jvmfwk/inc"
        "${JNI_INCLUDE_DIRS}"
        "${VALGRIND_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    DEFINITIONS JVMFWK_DLLIMPLEMENTATION
    DEPENDS udkapi_normal_headers)
