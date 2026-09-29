# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# qadevOOo -- the Java "complex test" (QA) framework: a runner library
# (qadevOOo/runner) plus a large body of interface-conformance test helpers
# (qadevOOo/tests/java/ifc/...) packaged as one jar, historically driven by
# JunitTest_* targets elsewhere in the tree that put OOoRunner.jar on their
# classpath and launch a live soffice.bin to exercise UNO services against
# it.
#
# Module_qadevOOo.mk, read in full, has a single target:
#
#   ifeq ($(ENABLE_JAVA),TRUE)
#   $(eval $(call gb_Module_add_targets,qadevOOo, Jar_OOoRunner))
#   endif
#
# Unlike the assumption that this port simply has no Java/JVM bridge:
# LIBRESTUDIO_ENABLE_JAVA is a real, wired build option (root CMakeLists.txt,
# OFF by default) with real consumers already ported under it -- jvmfwk,
# jvmaccess, unoil, ridljar, jurt, javaunohelper, bean -- and this port
# already has its own working "compile a real UNO jar" mechanism,
# librestudio_add_uno_jar() (cmake/LibreStudioJavaJar.cmake), used by
# ridljar/CMakeLists.txt to build libreoffice.jar/unoloader.jar for real. So
# "no Java bridge" is not, by itself, why Jar_OOoRunner stays unwired here.
#
# Two concrete, verified reasons it is not wired, neither of them "no Java
# bridge":
#
#   1. qadevOOo/Jar_OOoRunner.mk's own gb_Jar_use_jars,OOoRunner lists
#      "libreoffice java_uno". 2026-09-28 update: java_uno.jar (and
#      Library_java_uno, the native bridge) is now ported --
#      bridges/CMakeLists.txt's own "--- java_uno ---" section -- so this
#      specific dependency is no longer missing.
#
#   2. This reason alone still blocks OOoRunner regardless: every place
#      upstream uses it is a JunitTest_* target (the ifc/ test helpers this
#      jar packages exist only to be driven by JUnit-launched, live-office
#      complex tests), and this port has no JUnit-style test runner
#      anywhere -- the same already-established gap cited in e.g.
#      comphelper/CMakeLists.txt ("JunitTest_comphelper_complex ... a
#      completely different test-runner technology ... that this port has no
#      infrastructure for anywhere"), and repeated in xmloff's, bean's,
#      extensions', forms', linguistic's and unoxml's CMakeLists.txt for
#      their own JunitTest_* targets. Building a QA jar with nothing in this
#      tree able to load and drive it would not be real, useful output.
#
# Jar_OOoRunner.mk's own gb_Jar_add_sourcefiles list is, separately, over
# 500 .java files (qadevOOo/runner/... plus qadevOOo/tests/java/ifc/...) --
# read via `Read` with pagination to confirm its full, unconditional extent
# (no per-file gating beyond the single ENABLE_JAVA check above) before
# reaching this conclusion, not skimmed.
#
# Net: real target, real (and now, with java_uno ported, complete) Java/UNO
# jar-building infrastructure to build it with -- but still blocked, on its
# own, independently, by the absent JUnit-runner category (reason 2 above).
# Nothing wired until that is addressed too.
