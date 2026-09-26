# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# bean -- Jar_officebean.mk + Library_officebean.mk. Module_bean.mk gates both
# behind "ifneq ($(OS),MACOSX)" and "ifneq ($(OS),ANDROID)" (nested), which is
# a no-op restriction on this Linux-only port; JunitTest_bean_complex is test
# infra (and upstream itself has it commented out as "compilable but fail at
# runtime"), out of scope like every other JunitTest/CppunitTest here.

# officebean.jar: Jar_officebean.mk. gb_Jar_use_jars,officebean,[libreoffice]
# only adds libreoffice.jar to the *compile* classpath (gb_JavaClassSet_use_jar)
# -- it does NOT add a manifest Class-Path entry, because gb_Jar_use_jar only
# calls gb_Jar_add_manifest_classpath for jars outside gb_Jar_default_jars
# (:= gb_Jar_URE, the jars registered under Repository.mk's URE jar group --
# "libreoffice" is one of them, see ridljar/CMakeLists.txt), and this .mk never
# calls gb_Jar_set_manifest either -- so, matching gb_Jar__command's own
# `$(if $(MANIFEST),...)`/JARCLASSPATH-only behaviour, officebean.jar is built
# with no MANIFEST/MANIFEST_CLASSPATH argument at all: just the default
# Manifest-Version line librestudio_add_uno_jar always writes.
librestudio_add_uno_jar(officebean
    PACKAGEROOT "com"
    CLASSPATH
        "${libreoffice_JAR}"
    DEPENDS
        libreoffice_jar
    SOURCES
        bean/com/sun/star/comp/beans/ContainerFactory
        bean/com/sun/star/comp/beans/Controller
        bean/com/sun/star/comp/beans/Frame
        bean/com/sun/star/comp/beans/HasConnectionException
        bean/com/sun/star/comp/beans/InvalidArgumentException
        bean/com/sun/star/comp/beans/JavaWindowPeerFake
        bean/com/sun/star/comp/beans/LocalOfficeConnection
        bean/com/sun/star/comp/beans/LocalOfficeWindow
        bean/com/sun/star/comp/beans/NativeConnection
        bean/com/sun/star/comp/beans/NativeService
        bean/com/sun/star/comp/beans/NoConnectionException
        bean/com/sun/star/comp/beans/NoDocumentException
        bean/com/sun/star/comp/beans/OfficeConnection
        bean/com/sun/star/comp/beans/OfficeDocument
        bean/com/sun/star/comp/beans/OfficeWindow
        bean/com/sun/star/comp/beans/OOoBean
        bean/com/sun/star/comp/beans/SystemWindowException
        bean/com/sun/star/comp/beans/Wrapper
        bean/com/sun/star/comp/beans/CallWatchThread)

install(FILES "${officebean_JAR}" DESTINATION program/classes)

# libofficebean.so: Library_officebean.mk. Only external is jawt (AWT's native
# drawing-surface bridge) -- there is no gb_Library_use_libraries call at all
# in this .mk, so, unlike every other native library ported so far, officebean
# links neither sal nor any UNO runtime library: com_sun_star_comp_beans_
# LocalOfficeWindow.c only reaches sal/config.h and sal/types.h (both
# header-only) plus the JNI/JAWT/X11 APIs, so nothing here needs symbols from
# libsal. The OS,WNT cobjects branch (bean/native/win32/...) is skipped, same
# as every other Windows-only branch in this port.
add_library(officebean SHARED
    "${CMAKE_SOURCE_DIR}/bean/native/unix/com_sun_star_comp_beans_LocalOfficeWindow.c")

# JNI_INCLUDE_DIRS carries jawt_md.h's directory too (FindJNI's
# JAVA_INCLUDE_PATH2, the JDK's os-specific include subdir); JAVA_AWT_INCLUDE_PATH
# is FindJNI's own separate cache var for jawt.h's directory (not folded into
# JNI_INCLUDE_DIRS), and JAVA_AWT_LIBRARY is the libjawt.so FindJNI locates --
# both set by the find_package(JNI REQUIRED) call gating this whole module.
target_include_directories(officebean PRIVATE
    "${JNI_INCLUDE_DIRS}"
    "${JAVA_AWT_INCLUDE_PATH}")

target_link_libraries(officebean PRIVATE "${JAVA_AWT_LIBRARY}")

# Repository.mk registers officebean under PLAINLIBS_OOO, not OOOLIBS -- same
# bare-name PLAIN layer as jpipe/juh/juhx (which are PLAINLIBS_URE), not the
# "lo" suffix OOOLIBS gets. It's a JNI native library loaded by the Java
# OOoBean/NativeService class via System.loadLibrary("officebean"), which
# expects the bare "libofficebean.so" name.
librestudio_set_library_output_name(officebean officebean PLAIN)

install(TARGETS officebean LIBRARY DESTINATION program)
