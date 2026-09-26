# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# remotebridges (module remotebridges) -- one library: uuresolver, the
# UnoUrlResolver UNO service that turns a "uno:..." connection string into a
# live bridge (used by office::connect() and the command-line
# --accept/--unaccept machinery). Source list, include and dependencies are
# transcribed from Library_uuresolver.mk. Only gb_Library_use_udk_api is
# called (no offapi), so only the udkapi headers are needed here.

set(UURESOLVER_SOURCES
    remotebridges/source/unourl_resolver/unourl_resolver
)

librestudio_expand_cxx(UURESOLVER_SOURCES)
add_library(uuresolver SHARED ${UURESOLVER_SOURCES})

target_include_directories(uuresolver PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(uuresolver udkapi_normal_headers)

target_link_libraries(uuresolver PRIVATE
    cppu cppuhelper sal
    Boost::boost)

# remotebridges/source/unourl_resolver/uuresolver.component's third
# gb_Library_set_componentfile argument is "ure/services", i.e. the URE
# layer -- same as io and binaryurp's own componentfiles.
librestudio_add_component(uuresolver
    COMPONENT "${CMAKE_SOURCE_DIR}/remotebridges/source/unourl_resolver/uuresolver.component"
    LIBRARY   uuresolver
    LAYER     URELIB)

librestudio_set_library_output_name(uuresolver uuresolver OOO)

install(TARGETS uuresolver LIBRARY DESTINATION program)
