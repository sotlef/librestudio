# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sax — the SAX parser layer: the expat and fast-parser wrappers, the XML
# writer, and the encoding converter they share.
#
# It is reached for two different reasons.  Documents above it parse XML
# through these services; and i18npool's localedata generator uses the expat
# wrapper to read the locale descriptions, which is why it is ported here.
#
# Source list from Library_sax.mk (the precompiled header is not ported).

set(SAX_SOURCES
    sax/source/expatwrap/sax_expat
    sax/source/expatwrap/saxwriter
    sax/source/expatwrap/xml2utf
    sax/source/fastparser/fastparser
    sax/source/fastparser/legacyfastparser
    sax/source/tools/converter
    sax/source/tools/fastattribs
    sax/source/tools/fastserializer
    sax/source/tools/fshelper)

librestudio_expand_cxx(SAX_SOURCES)

add_library(sax SHARED ${SAX_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(sax REUSE_FROM precompiled_system)
endif ()

target_include_directories(sax PRIVATE
    "${CMAKE_SOURCE_DIR}/sax/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}"
    "${LIBXML2_INCLUDE_DIRS}")
add_dependencies(sax udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(sax PRIVATE SAX_DLLIMPLEMENTATION)
target_link_libraries(sax PRIVATE
    basegfx comphelper cppu cppuhelper sal salhelper tl
    EXPAT::EXPAT "${LIBXML2_LIBRARIES}")

librestudio_add_component(sax
    COMPONENT "${CMAKE_SOURCE_DIR}/sax/source/expatwrap/expwrap.component"
    LIBRARY   sax
    LAYER     OOO)

librestudio_set_library_output_name(sax sax OOO)

install(TARGETS sax LIBRARY DESTINATION program)

# --- CppunitTest targets -----------------------------------------------------

# sax's 4 CppunitTest_*.mk files (Module_sax.mk's gb_Module_add_check_
# targets list), re-checked directly against their live .mk files.

# CppunitTest_sax.mk: gb_CppunitTest_CppunitTest,sax names its internal
# target "sax" -- the SAME name as the sax library target above. gbuild's
# CppunitTest and Library namespaces are separate, but CMake's is not, so
# this collides exactly like store/CMakeLists.txt's CppunitTest_store.mk,
# basegfx/CMakeLists.txt's CppunitTest_basegfx.mk, and i18nutil/CMakeLists.
# txt's CppunitTest_i18nutil.mk before it -- same fix, appending "_test"
# to the CMake target name. 1 source (sax/qa/cppunit/test_converter),
# use_sdk_api, use_libraries sax sal comphelper cppu. (use_common_
# precompiled_header is not modelled -- no CppunitTest in this port uses a
# PCH, matching every other CppunitTest ported so far.)
librestudio_add_cppunit_test(sax_test
    SOURCES
        sax/qa/cppunit/test_converter
    LIBRARIES sax sal comphelper cppu
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers)

# CppunitTest_sax_attributes.mk: 1 source (sax/qa/cppunit/attributes),
# use_libraries cppu cppuhelper sal sax, use_sdk_api, use_ure -- the
# protector plug-ins (unobootstrapprotector/unoexceptionprotector,
# unotest/CMakeLists.txt) are wired in, same as comphelper_test's precedent
# in comphelper/CMakeLists.txt.
librestudio_add_cppunit_test(sax_attributes
    SOURCES
        sax/qa/cppunit/attributes
    LIBRARIES cppu cppuhelper sal sax
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector)

# CppunitTest_sax_parser.mk and CppunitTest_sax_xmlimport.mk: registered in
# sax/CppunitTest_deferred.cmake, NOT here, and include()'d from the root
# CMakeLists.txt well after configmgr/framework/i18npool/sfx2/ucb/uui have
# all had their own add_subdirectory() -- exactly the same ordering reason
# comphelper_test's own registration was moved to comphelper/CppunitTest_
# deferred.cmake (see that file, and its include() call site in the root
# CMakeLists.txt, for the full mechanism); both sax_parser and sax_xmlimport
# need use_components dependencies -- gb_CppunitTest_use_components, now
# wired via librestudio_add_cppunit_test()'s COMPONENTS parameter (cmake/
# LibreStudioTest.cmake) -- on modules that sit textually AFTER sax's own
# add_subdirectory() in the root CMakeLists.txt, and COMPONENTS resolves each
# name through a GLOBAL property that only exists once the owning module's
# own librestudio_add_component() call has run.
#
# What this wave found and fixed vs. what is left, re-verified directly
# against each live .mk file rather than trusted from an earlier summary:
#
#   - The use_components mechanism itself (the gap this file used to note as
#     wholly unaddressed) is fixed generically: librestudio_add_component()
#     now records each component's built file under a GLOBAL property
#     (cmake/LibreStudioComponent.cmake), and librestudio_add_cppunit_test()
#     gained a COMPONENTS parameter that resolves it into a single
#     "-env:UNO_SERVICES=..." argument (cmake/LibreStudioTest.cmake). Every
#     component name both tests need was verified to exist under the exact
#     name each module's own librestudio_add_component() call uses (NOT
#     always gbuild's own use_components spelling): sax_parser needs
#     configmgr (configmgr/CMakeLists.txt), fwk (framework/CMakeLists.txt,
#     not "framework"), i18npool (i18npool/CMakeLists.txt), sfx
#     (sfx2/CMakeLists.txt, not "sfx2"), ucb1 and ucpfile1 (both
#     ucb/CMakeLists.txt); sax_xmlimport's actual use_components list (read
#     directly from sax/CppunitTest_sax_xmlimport.mk, which differs from
#     sax_parser's -- no framework/i18npool/sfx2, but includes sax's own
#     expwrap component) needs configmgr, sax (this file's own
#     librestudio_add_component(sax ...) call above, for sax/source/
#     expatwrap/expwrap.component), ucb1, ucpfile1, and uui
#     (uui/CMakeLists.txt). All confirmed present.
#   - vclbootstrapprotector (Library_vclbootstrapprotector.mk) is also no
#     longer a blocker: test/CMakeLists.txt now ports it as a MODULE target
#     named exactly "vclbootstrapprotector" -- referenced by name only below,
#     same as unobootstrapprotector/unoexceptionprotector already are, since
#     CMake resolves target names across the whole project at generate time
#     regardless of add_subdirectory() order.
#   - use_configuration (gb_CppunitTest_use_configuration, solenv/gbuild/
#     CppunitTest.mk) was a THIRD, SEPARATE gap noted here as unaddressed for
#     a long time -- CLOSED this wave: librestudio_add_cppunit_test() gained
#     a CONFIGURATION option (cmake/LibreStudioTest.cmake, read its header
#     comment for the full mechanism) backed by officecfg/CMakeLists.txt's
#     already-existing officecfg_registry target (the instdir layer) and a
#     new staged copy of test/user-template (test/CMakeLists.txt's
#     LIBRESTUDIO_TEST_UNITTEST_DIR, the common/user-ui layers). Both
#     sax_parser and sax_xmlimport now pass CONFIGURATION (sax/CppunitTest_
#     deferred.cmake) -- see that file's own header comment for the up-to-
#     date detail; this note stays only as the history of why the two tests
#     were split into a deferred file in the first place.
