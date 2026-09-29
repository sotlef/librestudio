# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# linguistic (module lng) — the linguistic-services dispatch layer: spelling,
# hyphenation, thesaurus and conversion-dictionary managers that the app-level
# editing engines (editeng, and everything above it) call into. Source list
# from Library_lng.mk. $(if $(ENABLE_CURL),curl) resolves to the true branch
# in this build (CMakeLists.txt:133 does find_package(CURL REQUIRED), and
# gciterator.cxx/lngsvcmgr.cxx do use it), same as svl's CURL::libcurl.

set(LNG_SOURCES
    linguistic/source/convdic
    linguistic/source/convdiclist
    linguistic/source/convdicxml
    linguistic/source/dicimp
    linguistic/source/dlistimp
    linguistic/source/gciterator
    linguistic/source/hhconvdic
    linguistic/source/hyphdsp
    linguistic/source/hyphdta
    linguistic/source/iprcache
    linguistic/source/lngopt
    linguistic/source/lngprophelp
    linguistic/source/lngsvcmgr
    linguistic/source/misc
    linguistic/source/misc2
    linguistic/source/spelldsp
    linguistic/source/spelldta
    linguistic/source/thesdsp
)

librestudio_expand_cxx(LNG_SOURCES)
add_library(lng SHARED ${LNG_SOURCES})

target_include_directories(lng PRIVATE
    "${CMAKE_SOURCE_DIR}/linguistic/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(lng officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(lng PRIVATE LNG_DLLIMPLEMENTATION)

target_link_libraries(lng PRIVATE
    comphelper cppu cppuhelper i18nlangtag sal sax svl tl ucbhelper utl vcl xo
    Boost::boost
    ICU::uc
    CURL::libcurl)

librestudio_add_component(lng
    COMPONENT "${CMAKE_SOURCE_DIR}/linguistic/source/lng.component"
    LIBRARY   lng
    LAYER     OOO)

librestudio_set_library_output_name(lng lng OOO)

install(TARGETS lng LIBRARY DESTINATION program)

# CppunitTest_linguistic_restprotocol.mk: registered in linguistic/
# CppunitTest_deferred.cmake, NOT here, include()'d from the root
# CMakeLists.txt only after program_services_rdb exists (use_rdb(services))
# and officecfg/test/unotest are add_subdirectory()'d (use_custom_headers/
# use_configuration/use_ure), all well after linguistic's own position. See
# that file for the full per-test detail and its include() call site in the
# root CMakeLists.txt for why it sits where it does.
#
# JunitTest_linguistic_unoapi and JunitTest_linguistic_complex (Module_
# linguistic.mk's subsequentcheck targets) are Java tests, out of scope like
# every other JunitTest in this port.

# linguistic/CppunitTest_deferred.cmake needs data (a COMPONENTS name,
# program_services_rdb, CONFIGURATION's officecfg_registry/
# LIBRESTUDIO_TEST_UNITTEST_DIR, ...) from a module add_subdirectory()'d
# later than this one -- scheduled here rather than included directly, so it
# runs only once every add_subdirectory() in the whole tree has completed,
# regardless of which one is actually the latest-binding dependency. See
# that file's own header comment for the specific constraint, and
# cmake/LibreStudioTest.cmake's header comment for why CMAKE_SOURCE_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) is required in the path argument below.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/linguistic/CppunitTest_deferred.cmake")
