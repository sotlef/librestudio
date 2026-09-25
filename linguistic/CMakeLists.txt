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
