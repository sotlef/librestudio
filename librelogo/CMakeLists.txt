# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# librelogo -- the LibreLogo turtle-graphics macro (a pure-Python Basic/Writer
# extension, no compiled code at all).
#
# Module_librelogo.mk, read in full: its one gb_Module_add_targets call
# (CustomTarget_librelogo, Package_librelogo, Package_librelogo_properties) is
# gated `ifneq ($(filter SCRIPTING,$(BUILD_TYPE)),)` + `ifneq ($(DISABLE_PYTHON),TRUE)`
# -- i.e. it needs both the Basic/VBA object-model slice (ENABLE_SCRIPTING,
# cmake/LibreStudioConfigHeaders.cmake, default ON and already the live
# default here) and a working Python (pyuno/CMakeLists.txt already builds and
# links against the system Python on this port, so the DISABLE_PYTHON=TRUE
# branch never applies). Both gates hold on this port's actual configuration,
# so -- unlike android/ios/static -- this module's targets are genuinely live
# here and are wired for real below, not just documented.
#
# Package_librelogo.mk: packages exactly two files verbatim, no build step --
#   source/LibreLogo/LibreLogo.py             -> Scripts/python/LibreLogo/LibreLogo.py
#   source/pythonpath/LibreLogo_en_US.properties -> Scripts/python/LibreLogo/LibreLogo_en_US.properties
# under $(LIBO_SHARE_FOLDER)/Scripts/python/LibreLogo, i.e. share/Scripts/
# python/LibreLogo on this platform -- the same share/... + plain
# install(FILES ...) convention already established for package/CMakeLists.txt's
# Package_dtd.mk port and officecfg/CMakeLists.txt's registry install (see
# either for the LIBO_SHARE_FOLDER derivation; not repeated here).
if (ENABLE_SCRIPTING)
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/LibreLogo/LibreLogo.py"
        DESTINATION share/Scripts/python/LibreLogo)
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/pythonpath/LibreLogo_en_US.properties"
        DESTINATION share/Scripts/python/LibreLogo)
endif ()

# CustomTarget_librelogo.mk (the librelogo/locproperties CustomTarget) and
# Package_librelogo_properties.mk both exist only to translate
# LibreLogo_en_US.properties into every OTHER UI language: the CustomTarget's
# own recipe is wrapped in `ifneq ($(strip $(gb_WITH_LANG)),)` and derives its
# per-language file list from `$(filter-out en-US,$(gb_WITH_LANG))` --
# librelogo_LANGS. This port never sets gb_WITH_LANG anywhere (grepped: zero
# hits in the whole CMake tree) -- it is a single-locale (en-US only) build --
# so librelogo_LANGS is always empty here and both the CustomTarget and
# Package_librelogo_properties genuinely produce zero files on this port's
# actual configuration, the same "verified-empty-on-this-config" shape as
# android/ios/static, not a skipped translation step.
