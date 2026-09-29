# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# uitest -- end-to-end UI test recordings (Python scripts driving the office
# through its own accessibility/UNO automation API).
#
# Module_uitest.mk, read in full: it contains nothing but
# gb_Module_add_uicheck_targets(UITest_impress_demo, UITest_demo_ui,
# UITest_math_demo) -- three UITest_*.mk targets, no Library_*/Executable_*/
# Package_* of its own. UITest is its own distinct gbuild test-runner
# category (like JunitTest/PythonTest, and unlike CppunitTest, which this
# port's own librestudio_add_cppunit_test() mechanism already covers) --
# this port has never built a UITest runner anywhere (grepped: no
# UITest_*.mk is referenced from any CMakeLists.txt in this tree), the same
# honest, already-established gap as JunitTest/PythonTest for other modules
# (e.g. comphelper_complex, xmloff_unoapi).
#
# Nothing to wire until a UITest runner (Python + this port's own
# accessibility/UNO automation surface) exists as real CMake infrastructure.
