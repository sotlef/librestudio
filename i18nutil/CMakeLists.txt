# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# i18nutil — case folding and width/transliteration helpers shared by the
# i18n services.  Source list from Library_i18nutil.mk.

set(I18NUTIL_SOURCES
    i18nutil/source/utility/casefolding
    i18nutil/source/utility/guessparadirection
    i18nutil/source/utility/kashida
    i18nutil/source/utility/oneToOneMapping
    i18nutil/source/utility/paper
    i18nutil/source/utility/scriptchangescanner
    i18nutil/source/utility/scriptclass
    i18nutil/source/utility/scripttypedetector
    i18nutil/source/utility/unicode
    i18nutil/source/utility/unicodeescape
    i18nutil/source/utility/widthfolding
)

librestudio_expand_cxx(I18NUTIL_SOURCES)
add_library(i18nutil SHARED ${I18NUTIL_SOURCES})

target_include_directories(i18nutil PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(i18nutil officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(i18nutil PRIVATE I18NUTIL_DLLIMPLEMENTATION)
target_link_libraries(i18nutil PRIVATE utl i18nlangtag comphelper cppu sal ICU::uc ICU::i18n)

librestudio_set_library_output_name(i18nutil i18nutil OOO)
install(TARGETS i18nutil LIBRARY DESTINATION program)

# --- CppunitTest targets -----------------------------------------------------

# i18nutil's 2 CppunitTest_*.mk files, ported via librestudio_add_cppunit_test()
# (cmake/LibreStudioTest.cmake -- read its header comment first). Both
# re-checked directly against their live .mk files. Neither calls use_ure or
# any use_vcl* variant, so no PROTECTORS for either.

# CppunitTest_i18nutil.mk: 3 sources (i18nutil/qa/cppunit/
# test_guessparadirection, test_kashida, test_scriptchangescanner --
# confirmed these exact relative paths against the live .mk, not the
# "guessparadirection"/"kashida"/"scriptchangescanner" short names from the
# prior-research summary, which were wrong), use_libraries i18nutil sal test,
# use_sdk_api (udkapi + offapi -- gb_CppunitTest_use_sdk_api ==
# gb_CppunitTest_use_api($1, udkapi offapi), same "normal" variant mapping
# documented in o3tl/CMakeLists.txt's own CppunitTest_o3tl_tests section and
# already consumed by add_library(i18nutil ...) above).
#
# use_libraries test: the gbuild library name is "test" (Library_test.mk),
# but this port's CMake target for it is test_fixtures, not test --
# test/CMakeLists.txt renames it deliberately ("enable_testing() in the root
# CMakeLists.txt reserves the target name \"test\" for CTest's own built-in
# target", see the comment right above its own add_library(test_fixtures ...)
# call), and outputs it as bare libtest.so (PLAINLIBS_NONE, not installed --
# see that same comment block). So the target genuinely exists -- just not
# spelled "test" -- and this links test_fixtures rather than inventing a stub
# or flagging it as missing.
# test/CMakeLists.txt's add_subdirectory(test) call runs much later than
# add_subdirectory(i18nutil) in the root CMakeLists.txt (test depends on sfx/
# svt/vcl/drawinglayer, all higher up the stack), but that is not a problem:
# comphelper/CMakeLists.txt already links against unotest the same way, even
# though add_subdirectory(unotest) also runs after add_subdirectory
# (comphelper) -- target_link_libraries resolves target names at generate
# time, once every CMakeLists.txt in the tree has been processed, not at the
# time of the call, so this forward reference is exactly as safe as that
# already-established precedent.
#
# Also note the CppunitTest shares the plain name "i18nutil" with the library
# target above -- gbuild's CppunitTest and Library namespaces are separate,
# but CMake's is not, so this would collide if the CppunitTest target were
# also literally named "i18nutil" (same issue, same fix, as store/CMakeLists.
# txt's "store (CppunitTest)" section documents for CppunitTest_store.mk).
librestudio_add_cppunit_test(i18nutil_test
    SOURCES
        i18nutil/qa/cppunit/test_guessparadirection
        i18nutil/qa/cppunit/test_kashida
        i18nutil/qa/cppunit/test_scriptchangescanner
    LIBRARIES i18nutil sal test_fixtures
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS udkapi_normal_headers offapi_normal_headers)

# CppunitTest_i18nutil_unicodeescape.mk: 1 source (i18nutil/qa/cppunit/
# test_unicodeescape), use_external boost_headers, use_libraries cppu
# cppuhelper i18nutil sal. No use_api/use_sdk_api/use_ure/use_vcl* in the
# .mk, confirmed by reading it in full.
librestudio_add_cppunit_test(i18nutil_unicodeescape
    SOURCES
        i18nutil/qa/cppunit/test_unicodeescape
    LIBRARIES cppu cppuhelper i18nutil sal Boost::boost)
