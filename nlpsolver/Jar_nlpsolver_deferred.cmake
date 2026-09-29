# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Jar_nlpsolver.mk -- deferred out of nlpsolver/CMakeLists.txt because it
# needs ${java_uno_JAR}, only set once bridges/Jar_java_uno_deferred.cmake's
# own deferred call has fired; see the DEFER call site in nlpsolver/
# CMakeLists.txt for the full ordering rationale (same pattern as bridges'
# own java_uno.jar and wizards' six jars). Runs in the root CMakeLists.txt's
# own directory scope, hence every path below is CMAKE_SOURCE_DIR-relative.
#
# gb_Jar_use_jars(nlpsolver,libreoffice java_uno EvolutionarySolver) --
# EvolutionarySolver is built directly (un-deferred) in nlpsolver/
# CMakeLists.txt itself, since it has zero jar dependencies of its own; its
# cache variable is already set by the time this file runs regardless.
# gb_Jar_set_manifest points at a single-line Manifest.mf (RegistrationClass
# Name: com.sun.star.comp.Calc.NLPSolver.Registration) -- no gb_Jar_add_
# manifest_classpath call in this .mk, so no MANIFEST_CLASSPATH here. No
# gb_Jar_set_componentfile either (see nlpsolver/CMakeLists.txt's own
# comment: the real UNO component is a prebuilt components.rdb, packaged
# directly by the out-of-scope Extension_nlpsolver, not generated here).
librestudio_add_uno_jar(nlpsolver
    PACKAGEROOT com
    MANIFEST "${CMAKE_SOURCE_DIR}/nlpsolver/src/com/sun/star/comp/Calc/NLPSolver/Manifest.mf"
    CLASSPATH
        "${libreoffice_JAR}"
        "${java_uno_JAR}"
        "${EvolutionarySolver_JAR}"
    DEPENDS
        libreoffice_jar
        java_uno_jar
        EvolutionarySolver_jar
    SOURCES
        nlpsolver/src/com/sun/star/comp/Calc/NLPSolver/Registration
        nlpsolver/src/com/sun/star/comp/Calc/NLPSolver/PropertyInfo
        nlpsolver/src/com/sun/star/comp/Calc/NLPSolver/dialogs/controls/BaseControl
        nlpsolver/src/com/sun/star/comp/Calc/NLPSolver/dialogs/controls/ProgressBar
        nlpsolver/src/com/sun/star/comp/Calc/NLPSolver/dialogs/controls/Button
        nlpsolver/src/com/sun/star/comp/Calc/NLPSolver/dialogs/controls/LabeledControl
        nlpsolver/src/com/sun/star/comp/Calc/NLPSolver/dialogs/controls/Label
        nlpsolver/src/com/sun/star/comp/Calc/NLPSolver/dialogs/DummyEvolutionarySolverStatusDialog
        nlpsolver/src/com/sun/star/comp/Calc/NLPSolver/dialogs/IEvolutionarySolverStatusDialog
        nlpsolver/src/com/sun/star/comp/Calc/NLPSolver/dialogs/EvolutionarySolverStatusUno
        nlpsolver/src/com/sun/star/comp/Calc/NLPSolver/dialogs/BaseDialog
        nlpsolver/src/com/sun/star/comp/Calc/NLPSolver/DEPSSolverImpl
        nlpsolver/src/com/sun/star/comp/Calc/NLPSolver/SCOSolverImpl
        nlpsolver/src/com/sun/star/comp/Calc/NLPSolver/BaseEvolutionarySolver
        nlpsolver/src/com/sun/star/comp/Calc/NLPSolver/ResourceManager
        nlpsolver/src/com/sun/star/comp/Calc/NLPSolver/BaseNLPSolver)

# No install() -- Repository.mk registers both nlpsolver and EvolutionarySolver
# under gb_Helper_register_jars,OXT (not _for_install): neither jar installs
# on its own, only packaged into the (out-of-scope) nlpsolver.oxt. Matches
# the already-established precedent of EvolutionarySolver's own wiring
# (nlpsolver/CMakeLists.txt), which likewise has no install() call.
