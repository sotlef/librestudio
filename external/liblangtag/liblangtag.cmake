# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for liblangtag, invoked by librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake, from the root CMakeLists.txt) only when no system
# liblangtag was found -- which is always the case on Windows.
#
# ExternalProject_liblangtag.mk drives autotools; this compiles the sources
# directly instead, the way libpng/md4c are done here.  Like upstream's MSC
# build it is a static library with both ldml extensions (-t and -u) linked in
# (--disable-modules), so there is nothing to dlopen.
#
# Besides the library this stages the registry data at share/liblangtag
# (ExternalPackage_liblangtag_data.mk), which i18nlangtag looks up through
# lt_db_set_datadir().  language-subtag-registry.xml is regenerated from the
# newer registry tarball (UnpackedTarball_liblangtag.mk's pre_action) with
# data/reg2xml.c, built here for the purpose.

set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_liblangtag)
    message(STATUS "liblangtag: no system package found -- building liblangtag 0.6.8 from source")

    find_package(LibXml2 REQUIRED)

    librestudio_fetch_tarball(liblangtag liblangtag-0.6.8.tar.bz2
        a96975b79ddd8fef6d9295c083fe3f1afa1a8898a57235d4069255ade44e5cf2)
    librestudio_fetch_tarball(langtagreg language-subtag-registry-2026-9-17.tar.bz2
        10223cc8c219427c86a4c66ce98282a00c7f4d9b0609355146282344783007f2)

    set(_lt "${liblangtag_SOURCE_DIR}")

    # config.h, as configure would have written it for a Windows/MSVC build:
    # no dirent/dlfcn/libgen/unistd/execinfo, no getuid family.
    file(WRITE "${CMAKE_BINARY_DIR}/liblangtag-config/config.h" [[
#define HAVE_STDINT_H 1
#define HAVE_STRING_H 1
#define HAVE_STDLIB_H 1
#define HAVE_VSNPRINTF 1
#define LT_HAVE_C99_VSNPRINTF 1
#define LT_HAVE_ISO_VARARGS 1
#define LT_HAVE_INLINE 1
#define __VA_COPY_USE_C99(d, s) va_copy((d), (s))
#define HAVE_VA_COPY 1
#define ALIGNOF_VOID_P 8
]])

    set(_lt_sources
        lt-database lt-error lt-ext-module lt-ext-module-data lt-extension
        lt-extlang lt-extlang-db lt-grandfathered lt-grandfathered-db lt-iter
        lt-lang lt-lang-db lt-list lt-mem lt-messages lt-redundant
        lt-redundant-db lt-region lt-region-db lt-relation-db lt-script
        lt-script-db lt-string lt-tag lt-trie lt-utils lt-variant
        lt-variant-db lt-xml)
    set(_lt_abs "")
    foreach (_s IN LISTS _lt_sources)
        list(APPEND _lt_abs "${_lt}/liblangtag/${_s}.c")
    endforeach ()
    unset(_s)

    add_library(liblangtag STATIC ${_lt_abs}
        "${_lt}/extensions/lt-ext-ldml-t.c"
        "${_lt}/extensions/lt-ext-ldml-u.c")
    set_source_files_properties("${_lt}/extensions/lt-ext-ldml-t.c"
        PROPERTIES COMPILE_DEFINITIONS LT_MODULE_PREFIX=lt_module_ext_t)
    set_source_files_properties("${_lt}/extensions/lt-ext-ldml-u.c"
        PROPERTIES COMPILE_DEFINITIONS LT_MODULE_PREFIX=lt_module_ext_u)
    set_target_properties(liblangtag PROPERTIES POSITION_INDEPENDENT_CODE ON)
    # Consumers include <liblangtag/langtag.h>.
    target_include_directories(liblangtag
        PUBLIC "${_lt}"
        PRIVATE "${_lt}/liblangtag" "${CMAKE_BINARY_DIR}/liblangtag-config")
    target_compile_definitions(liblangtag PRIVATE
        HAVE_CONFIG_H __LANGTAG_COMPILATION __LANGTAG_PRIVATE
        # lt-macros.h defines LT_INLINE_FUNC (static inline) only with this;
        # lt-config.h sets it for C++ alone, and without it the header's
        # function bodies are emitted as external symbols in every object.
        LT_CAN_INLINE=1
        "BUILDDIR=\"\"" "SRCDIR=\"\"" "REGDATADIR=\"\"" "LANGTAG_EXT_MODULE_PATH=\"\"")
    if (MSVC)
        target_compile_definitions(liblangtag PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_compile_options(liblangtag PRIVATE /w)
    else ()
        target_compile_options(liblangtag PRIVATE -w)
    endif ()
    target_link_libraries(liblangtag PUBLIC LibXml2::LibXml2)

    # data/reg2xml.c: registry text -> language-subtag-registry.xml
    add_executable(liblangtag_reg2xml "${_lt}/data/reg2xml.c")
    target_include_directories(liblangtag_reg2xml PRIVATE
        "${_lt}" "${_lt}/liblangtag" "${CMAKE_BINARY_DIR}/liblangtag-config")
    target_compile_definitions(liblangtag_reg2xml PRIVATE HAVE_CONFIG_H __LANGTAG_COMPILATION __LANGTAG_PRIVATE)
    if (MSVC)
        target_compile_definitions(liblangtag_reg2xml PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_compile_options(liblangtag_reg2xml PRIVATE /w)
    endif ()
    target_link_libraries(liblangtag_reg2xml PRIVATE liblangtag)
    # Not part of the instdir: it only produces build-time data.
    set_target_properties(liblangtag_reg2xml PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/liblangtag-config")

    set(_lt_data "${LIBRESTUDIO_SHARE_DIR}/liblangtag")
    set(_lt_data_files
        common/bcp47/calendar.xml common/bcp47/collation.xml
        common/bcp47/currency.xml common/bcp47/number.xml
        common/bcp47/timezone.xml common/bcp47/transform.xml
        common/bcp47/transform_ime.xml common/bcp47/transform_keyboard.xml
        common/bcp47/transform_mt.xml common/bcp47/transform_private_use.xml
        common/bcp47/variant.xml
        common/supplemental/likelySubtags.xml
        common/supplemental/supplementalMetadata.xml)
    set(_lt_outputs "${_lt_data}/language-subtag-registry.xml")
    set(_lt_commands COMMAND liblangtag_reg2xml
        "${langtagreg_SOURCE_DIR}/language-subtag-registry"
        "${_lt_data}/language-subtag-registry.xml")
    foreach (_f IN LISTS _lt_data_files)
        list(APPEND _lt_outputs "${_lt_data}/${_f}")
        list(APPEND _lt_commands
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_lt}/data/${_f}" "${_lt_data}/${_f}")
    endforeach ()
    unset(_f)
    add_custom_command(OUTPUT ${_lt_outputs}
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${_lt_data}/common/bcp47" "${_lt_data}/common/supplemental"
        ${_lt_commands}
        DEPENDS liblangtag_reg2xml
        COMMENT "liblangtag: staging share/liblangtag"
        VERBATIM)
    add_custom_target(liblangtag_data ALL DEPENDS ${_lt_outputs})
    install(DIRECTORY "${_lt_data}/" DESTINATION share/liblangtag)

    # The version the configure-time header switch (LIBLANGTAG_INLINE_FIX) reads.
    set(LIBRESTUDIO_LIBLANGTAG_VERSION "0.6.8" CACHE INTERNAL "" FORCE)

    librestudio_vendor_set_target(LIBLANGTAG liblangtag)
endfunction()
