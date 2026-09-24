# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# i18nlangtag — language tag parsing and canonicalisation, on top of ICU and
# liblangtag.  comphelper depends on it, which is how it ends up in the UNO
# core's dependency closure.
# Source list from Library_i18nlangtag.mk.

set(I18NLANGTAG_SOURCES
    i18nlangtag/source/isolang/insys
    i18nlangtag/source/isolang/isolang
    i18nlangtag/source/isolang/mslangid
    i18nlangtag/source/languagetag/languagetag
    i18nlangtag/source/languagetag/languagetagicu)

librestudio_expand_cxx(I18NLANGTAG_SOURCES)

add_library(i18nlangtag SHARED ${I18NLANGTAG_SOURCES})

# gb_Library_use_sdk_api: the normal bindings of both udkapi and offapi
# (languagetag.cxx includes com/sun/star/i18n/ScriptType.hpp, which is offapi).
target_include_directories(i18nlangtag PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(i18nlangtag udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(i18nlangtag PRIVATE I18NLANGTAG_DLLIMPLEMENTATION)
target_link_libraries(i18nlangtag PRIVATE sal ICU::uc PkgConfig::LIBLANGTAG)

librestudio_set_library_output_name(i18nlangtag i18nlangtag OOO)

install(TARGETS i18nlangtag LIBRARY DESTINATION program)
