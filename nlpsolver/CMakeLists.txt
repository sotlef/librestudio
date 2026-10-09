# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# nlpsolver -- the Calc "Solver" nonlinear-optimization add-in: a vendored
# evolutionary-algorithm engine (ThirdParty/EvolutionarySolver, its own
# net.adaptivebox.* package, no relation to any UNO API) plus a UNO-facing
# Java service (com.sun.star.comp.Calc.NLPSolver) that wraps it and is
# shipped to end users as an installable .oxt extension.
#
# Module_nlpsolver.mk, read in full, has a single conditional:
#
#   ifeq ($(ENABLE_JAVA),TRUE)
#   $(eval $(call gb_Module_add_targets,nlpsolver,
#       Jar_EvolutionarySolver Jar_nlpsolver Extension_nlpsolver))
#   endif
#
# As with qadevOOo (see that module's own CMakeLists.txt), "no Java bridge"
# is not the reason this stays mostly unwired: LIBRESTUDIO_ENABLE_JAVA is a
# real, wired option with real consumers (jvmfwk/ridljar/jurt/bean/...) and
# this port's own librestudio_add_uno_jar() (cmake/LibreStudioJavaJar.cmake)
# already compiles real UNO jars (ridljar/CMakeLists.txt). Each of the three
# targets was checked individually against that:
#
#   - Jar_EvolutionarySolver (nlpsolver/Jar_EvolutionarySolver.mk): 22
#     self-contained net.adaptivebox.* sourcefiles under
#     nlpsolver/ThirdParty/EvolutionarySolver/src, gb_Jar_set_packageroot
#     net/adaptivebox, its own Manifest.mf, and NO gb_Jar_use_jars/
#     gb_Jar_use_externals call at all -- zero dependency on anything else in
#     this tree (not even java_uno or libreoffice.jar). Genuinely,
#     immediately buildable with existing infrastructure: wired below.
#
#   - Jar_nlpsolver (nlpsolver/Jar_nlpsolver.mk): gb_Jar_use_jars,nlpsolver,
#     "libreoffice java_uno EvolutionarySolver". 2026-09-28: ported for real,
#     in nlpsolver/Jar_nlpsolver_deferred.cmake -- deferred (not called
#     directly here) because it needs ${java_uno_JAR}, which is itself only
#     set once bridges/Jar_java_uno_deferred.cmake's own deferred call has
#     already fired; scheduling this module's own DEFER call here, textually
#     after bridges/CMakeLists.txt's own add_subdirectory(), guarantees that
#     one runs first (deferred calls against the same DIRECTORY fire in
#     scheduling order). No gb_Jar_set_componentfile in this .mk -- the
#     actual com.sun.star.comp.Calc.NLPSolver UNO component is a prebuilt
#     components.rdb (nlpsolver/src/com/sun/star/comp/Calc/NLPSolver/
#     components.rdb) that Extension_nlpsolver.mk packages directly, not
#     something any Jar_* macro generates -- so, unlike wizards' own four
#     Jars, there is no RDB-routing decision to make here at all.
#
#   - Extension_nlpsolver (nlpsolver/Extension_nlpsolver.mk): packages
#     nlpsolver.jar + EvolutionarySolver.jar + components.rdb + localized
#     help/locale files into an installable .oxt via gb_Extension_Extension/
#     gb_Extension_add_file/gb_Extension_localize_properties/
#     gb_Extension_add_helpfiles. Grepped this whole CMake tree for any
#     Extension-building mechanism (the way librestudio_add_uno_jar exists
#     for jars): none exists -- no module in this port builds a gb_Extension_*
#     target yet. A real, currently-missing piece of build infrastructure,
#     not something specific to nlpsolver; still blocked on that gap alone
#     now that both Jars it would package are real. Matches Repository.mk's
#     own gb_Helper_register_jars,OXT (not _for_install) grouping for both
#     nlpsolver/EvolutionarySolver -- neither jar is meant to be installed
#     directly outside an .oxt, so neither gets an install() call here.
#
# Jar_EvolutionarySolver is wired directly below (no deferral needed --
# zero jar dependencies of its own); Jar_nlpsolver is wired via the deferred
# file described above.

if (LIBRESTUDIO_ENABLE_JAVA)
    librestudio_add_uno_jar(EvolutionarySolver
        PACKAGEROOT "net/adaptivebox"
        MANIFEST    "${CMAKE_SOURCE_DIR}/nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/Manifest.mf"
        SOURCES
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/goodness/ACRComparator
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/goodness/BCHComparator
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/goodness/IGoodnessCompareEngine
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/global/BasicBound
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/global/RandomGenerator
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/global/IUpdateCycleEngine
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/encode/EvalStruct
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/encode/EvalElement
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/encode/IEncodeEngine
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/space/ILocationEngine
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/space/BasicPoint
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/space/DesignSpace
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/space/DesignDim
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/problem/ProblemEncoder
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/knowledge/Library
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/knowledge/ILibEngine
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/knowledge/SearchPoint
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/deps/DEPSAgent
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/deps/behavior/PSGTBehavior
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/deps/behavior/AbsGTBehavior
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/deps/behavior/DEGTBehavior
            nlpsolver/ThirdParty/EvolutionarySolver/src/net/adaptivebox/sco/SCAgent)

    librestudio_include_deferred("${CMAKE_SOURCE_DIR}/nlpsolver/Jar_nlpsolver_deferred.cmake")
endif ()
