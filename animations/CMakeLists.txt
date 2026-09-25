# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# animations (module animcore) — the UNO animation-node core (com.sun.star.
# animations.AnimateSet/AnimationNodeFactory and friends) used by Impress'
# custom-animation and transition machinery. Source list from
# Library_animcore.mk: a single translation unit. No *_DLLIMPLEMENTATION
# define: the .mk never calls gb_Library_add_defs for animcore.

set(ANIMCORE_SOURCES
    animations/source/animcore/animcore
)

librestudio_expand_cxx(ANIMCORE_SOURCES)
add_library(animcore SHARED ${ANIMCORE_SOURCES})

target_include_directories(animcore PRIVATE
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(animcore officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(animcore PRIVATE
    comphelper cppu cppuhelper sal)

librestudio_add_component(animcore
    COMPONENT "${CMAKE_SOURCE_DIR}/animations/source/animcore/animcore.component"
    LIBRARY   animcore
    LAYER     OOO)

librestudio_set_library_output_name(animcore animcore OOO)

install(TARGETS animcore LIBRARY DESTINATION program)
