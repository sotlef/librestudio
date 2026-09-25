# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# avmedia — Library_avmedia.mk's `ifneq (,$(filter AVMEDIA,$(BUILD_TYPE)))`
# block resolves false in this build: cmake/config/config_features.h.in
# hardcodes HAVE_FEATURE_AVMEDIA 0 (no media-playback backend is wired into
# this port), so only the always-built part of the library applies -- the
# single mediaitem.cxx (the SfxItem that carries a media URL/mime-type
# through the document model), none of the framework/viewer player UI
# sources, and no avmedia.component (that's declared inside the same guarded
# block). svxcore still needs this stub for the media item itself.

set(AVMEDIA_SOURCES
    avmedia/source/framework/mediaitem
)

librestudio_expand_cxx(AVMEDIA_SOURCES)
add_library(avmedia SHARED ${AVMEDIA_SOURCES})

target_include_directories(avmedia PRIVATE
    "${CMAKE_SOURCE_DIR}/avmedia/inc"
    "${CMAKE_SOURCE_DIR}/avmedia/source/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(avmedia officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(avmedia PRIVATE AVMEDIA_DLLIMPLEMENTATION)

target_link_libraries(avmedia PRIVATE
    comphelper cppu cppuhelper sal svl ucbhelper vcl
    Boost::boost)

librestudio_set_library_output_name(avmedia avmedia OOO)

install(TARGETS avmedia LIBRARY DESTINATION program)
