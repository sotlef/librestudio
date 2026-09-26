# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# unoil -- CustomTarget_javamaker.mk only: runs javamaker over offapi.rdb
# (extended with udkapi.rdb) to produce the offapi Java stub sources under
# com/sun/star/**. ridljar's libreoffice.jar packages this output directly
# (Jar_libreoffice.mk lists "unoil/javamaker" as a customtarget dependency
# and adds $(gb_CustomTarget_workdir)/unoil/javamaker/com as a package root),
# so this is genuinely load-bearing, not vestigial.
#
# Jar_unoil.mk (unoil.jar) is NOT ported: it has no gb_Jar_add_sourcefile
# calls at all, only gb_Jar_add_manifest_classpath,unoil,libreoffice.jar --
# i.e. it is an empty legacy shim jar whose only content is a manifest
# Class-Path entry pointing at libreoffice.jar, the same kind of
# no-real-content compatibility artifact as ridl.jar/jurt.jar/juh.jar, which
# this port also does not build. Nothing in this port's CMake files
# references unoil.jar.
#
# CliUnoApi_oootypes (the .NET/Mono half of Module_unoil.mk, gated on
# ENABLE_CLI/MSC) is out of scope like the rest of cli_ure.
#
# This whole module is only ever add_subdirectory'd under
# LIBRESTUDIO_ENABLE_JAVA (see the root CMakeLists.txt), matching upstream's
# own Module_unoil.mk "ifneq ($(ENABLE_JAVA),)" gate around
# CustomTarget_javamaker, so no further gating is needed in this file.
librestudio_uno_api_java(unoil
    RDB        "${offapi_RDB}"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/javamaker"
    EXTRA_RDBS "${udkapi_RDB}")
