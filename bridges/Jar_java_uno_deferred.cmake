# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# java_uno.jar (Jar_java_uno.mk) -- split out of bridges/CMakeLists.txt and
# deferred, since it needs ${libreoffice_JAR} (set by ridljar/CMakeLists.txt,
# add_subdirectory()'d after bridges/CMakeLists.txt); see the DEFER call site
# in bridges/CMakeLists.txt for the full ordering rationale. Runs in the root
# CMakeLists.txt's own directory scope (include() does not create a new
# directory scope the way add_subdirectory() does), hence every path below is
# CMAKE_SOURCE_DIR-relative rather than relying on this file's own location.
#
# gb_Jar_use_jars(java_uno,libreoffice): compile classpath is libreoffice.jar
# only. gb_Jar_set_manifest points at bridges/source/jni_uno/java/.../manifest
# (just "Sealed: true", read in full). gb_Jar_add_manifest_classpath adds
# libreoffice.jar plus $(if $(filter MACOSX,$(OS)),../../Frameworks/,../) --
# resolved to the plain "../" branch for this Linux-only port, the same
# resolution ridljar/CMakeLists.txt's own libreoffice.jar call already uses.
# gb_Jar_set_packageroot(com) + gb_Jar_add_sourcefiles: 2 sourcefiles,
# JNI_info_holder and JNI_proxy.
#
# No DEPENDS on the java_uno library target (bridges/CMakeLists.txt, right
# above the DEFER call that reaches this file): javac compiling these 2 .java
# files never touches the native library -- the only real connection between
# the two is a Java-side System.loadLibrary() at run time, not a build-time
# one, so no such dependency is fabricated here.
librestudio_add_uno_jar(java_uno
    PACKAGEROOT com
    MANIFEST "${CMAKE_SOURCE_DIR}/bridges/source/jni_uno/java/com/sun/star/bridges/jni_uno/manifest"
    MANIFEST_CLASSPATH
        libreoffice.jar
        ../
    CLASSPATH
        "${libreoffice_JAR}"
    DEPENDS
        libreoffice_jar
    SOURCES
        bridges/source/jni_uno/java/com/sun/star/bridges/jni_uno/JNI_info_holder
        bridges/source/jni_uno/java/com/sun/star/bridges/jni_uno/JNI_proxy)

# gb_Helper_register_jars_for_install(URE,ure,[... java_uno ...]) in
# Repository.mk -- a real jar (unlike juh.jar/jurt.jar/ridl.jar, the empty
# legacy Class-Path shims this port never builds), installed alongside
# libreoffice.jar/unoloader.jar, same as ridljar/CMakeLists.txt's own
# install() call for those two.
install(FILES "${java_uno_JAR}" DESTINATION program/classes)
