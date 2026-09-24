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
