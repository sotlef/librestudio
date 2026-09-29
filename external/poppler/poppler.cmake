# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for poppler, invoked by librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake, called from sdext/CMakeLists.txt -- the only
# real consumer) only when no sufficiently new system poppler + poppler-cpp
# pair was found. See sdext/CMakeLists.txt for the system-or-vendor call
# itself and the rest of this dependency's own rationale (why poppler-cpp
# is needed only for 3 version macros, the 26.06.0-vs-26.08.0 honest
# discrepancy in the upstream patch, why no poppler-cpp source is actually
# compiled).
#
# Sets LIBRESTUDIO_POPPLER_INCLUDE_DIRS/LIBRESTUDIO_POPPLER_DEFS itself, in
# addition to the primary LIBRESTUDIO_POPPLER_TARGET -- sdext/CMakeLists.txt's
# own found-branch sets the same two variables for the system case
# (different values: DEFS gets SYSTEM_POPPLER, INCLUDE_DIRS stays empty
# since PkgConfig::POPPLER already carries its own).
#
# _librestudio_driver_dir: see external/zlib/zlib.cmake's own header
# comment for why this is captured at file top level, outside the function.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_poppler)
    message(STATUS "poppler: no system package found -- building poppler 26.08.0 from source")

    find_package(EXPAT REQUIRED)
    find_package(Freetype REQUIRED)
    # No find_package(PNG...)/LIBRESTUDIO_PNG_TARGET here: StaticLibrary_
    # poppler.mk's own gb_StaticLibrary_use_externals,poppler is just
    # "libjpeg zlib" (re-checked directly) -- goo/PNGWriter.cc (one of the
    # 117 compiled files below) exists but does not pull in libpng, matching
    # that real .mk.

    # Own dependency resolved lazily, right here, instead of relying on any
    # guaranteed external order (there is none anymore) -- same pattern as
    # external/libpng/libpng.cmake's own recursive ZLIB resolution.
    librestudio_find_package(NAME ZLIB TARGET_VAR _poppler_zlib_target)

    # download.lst's own POPPLER_TARBALL/POPPLER_SHA256SUM, verified
    # directly (grep -n poppler download.lst) and cross-checked against the
    # real downloaded file's own sha256sum.
    librestudio_fetch_tarball(poppler poppler-26.08.0.tar.xz
        dc906e68cea698109706ac6aa3d2c9d4512fcfcac42d90b8afcda486d1b9abd0)

    # UnpackedTarball_poppler.mk's own patch list (patchlevel 1 for every
    # entry it names; sanitizer.patch only under -fsanitize=, real but
    # inert here).
    set(_poppler_patches
        disable-freetype.patch.1
        poppler-config.patch.1)
    if (CMAKE_CXX_FLAGS MATCHES "-fsanitize=" OR CMAKE_C_FLAGS MATCHES "-fsanitize=")
        list(APPEND _poppler_patches sanitizer.patch)
    endif ()
    librestudio_apply_patches(poppler "${poppler_SOURCE_DIR}" "${_librestudio_driver_dir}" 0 ${_poppler_patches})

    # StaticLibrary_poppler.mk's own two source lists, transcribed verbatim
    # (re-verified against the real 26.08.0 tarball: all 117 files exist, 0
    # missing). gb_StaticLibrary_add_generated_cobjects compiles the 14
    # pregenerated font-width tables as .c; gb_StaticLibrary_add_generated_
    # exception_objects compiles the rest as .cc.
    set(_poppler_cobj_sources
        poppler/CourierWidths.pregenerated
        poppler/CourierBoldWidths.pregenerated
        poppler/CourierBoldObliqueWidths.pregenerated
        poppler/CourierObliqueWidths.pregenerated
        poppler/HelveticaWidths.pregenerated
        poppler/HelveticaBoldWidths.pregenerated
        poppler/HelveticaBoldObliqueWidths.pregenerated
        poppler/HelveticaObliqueWidths.pregenerated
        poppler/SymbolWidths.pregenerated
        poppler/TimesBoldWidths.pregenerated
        poppler/TimesBoldItalicWidths.pregenerated
        poppler/TimesItalicWidths.pregenerated
        poppler/TimesRomanWidths.pregenerated
        poppler/ZapfDingbatsWidths.pregenerated)
    set(_poppler_cc_sources
        goo/gbase64
        goo/gbasename
        goo/gfile
        goo/glibc
        goo/glibc_strtok_r
        goo/grandom
        goo/gstrtod
        goo/GooString
        goo/GooTimer
        goo/ImgWriter
        goo/JpegWriter
        goo/NetPBMWriter
        goo/PNGWriter
        goo/TiffWriter
        fofi/FoFiBase
        fofi/FoFiEncodings
        fofi/FoFiIdentifier
        fofi/FoFiTrueType
        fofi/FoFiType1
        fofi/FoFiType1C
        poppler/Annot
        poppler/AnnotStampImageHelper
        poppler/Array
        poppler/BBoxOutputDev
        poppler/CachedFile
        poppler/Catalog
        poppler/CertificateInfo
        poppler/CharCodeToUnicode
        poppler/CMap
        poppler/CryptoSignBackend
        poppler/DCTStream
        poppler/DateInfo
        poppler/Decrypt
        poppler/Dict
        poppler/Error
        poppler/FDPDFDocBuilder
        poppler/FILECacheLoader
        poppler/FileSpec
        poppler/FlateEncoder
        poppler/FontEncodingTables
        poppler/FontInfo
        poppler/Form
        poppler/Function
        poppler/Gfx
        poppler/GfxFont
        poppler/GfxState
        poppler/GlobalParams
        poppler/Hints
        poppler/ImageEmbeddingUtils
        poppler/JArithmeticDecoder
        poppler/JBIG2Stream
        poppler/JSInfo
        poppler/Lexer
        poppler/Linearization
        poppler/Link
        poppler/LocalPDFDocBuilder
        poppler/MarkedContentOutputDev
        poppler/Movie
        poppler/NameToCharCode
        poppler/Object
        poppler/OptionalContent
        poppler/Outline
        poppler/OutputDev
        poppler/Page
        poppler/PageLabelInfo
        poppler/PageTransition
        poppler/Parser
        poppler/PDFDoc
        poppler/PDFDocBuilder
        poppler/PDFDocEncoding
        poppler/PDFDocFactory
        poppler/PreScanOutputDev
        poppler/ProfileData
        poppler/PSOutputDev
        poppler/PSTokenizer
        poppler/Rendition
        poppler/SecurityHandler
        poppler/SignatureInfo
        poppler/Sound
        poppler/SplashOutputDev
        poppler/Stream
        poppler/StructElement
        poppler/StructTreeRoot
        poppler/TextOutputDev
        poppler/UTF
        poppler/UnicodeMap
        poppler/UnicodeMapFuncs
        poppler/UnicodeTypeTable
        poppler/ViewerPreferences
        poppler/XRef
        splash/Splash
        splash/SplashBitmap
        splash/SplashClip
        splash/SplashFont
        splash/SplashFontEngine
        splash/SplashFontFile
        splash/SplashFontFileID
        splash/SplashPath
        splash/SplashPattern
        splash/SplashScreen
        splash/SplashState
        splash/SplashXPath
        splash/SplashXPathScanner)

    set(_poppler_sources "")
    foreach (_f IN LISTS _poppler_cobj_sources)
        list(APPEND _poppler_sources "${poppler_SOURCE_DIR}/${_f}.c")
    endforeach ()
    foreach (_f IN LISTS _poppler_cc_sources)
        list(APPEND _poppler_sources "${poppler_SOURCE_DIR}/${_f}.cc")
    endforeach ()
    unset(_f)

    add_library(poppler STATIC ${_poppler_sources})
    # gb_StaticLibrary_set_include,poppler: fofi is needed here (poppler/
    # splash sources include fofi/*.h directly) even though a consumer of
    # the finished library never has to (see LIBRESTUDIO_POPPLER_INCLUDE_
    # DIRS below, which deliberately leaves fofi out, matching
    # RepositoryExternal.mk's own bundled gb_LinkTarget__use_poppler).
    target_include_directories(poppler PRIVATE
        "${poppler_SOURCE_DIR}"
        "${poppler_SOURCE_DIR}/fofi"
        "${poppler_SOURCE_DIR}/goo"
        "${poppler_SOURCE_DIR}/poppler")
    # gb_StaticLibrary_set_warnings_disabled,poppler
    target_compile_options(poppler PRIVATE -w)
    # 2026-09-29: JPEG resolved via librestudio_find_package() now (own
    # dependency resolved lazily, same pattern as ZLIB above) -- see
    # external/libjpeg-turbo/libjpeg-turbo.cmake's own header comment.
    # LIBRESTUDIO_JPEG_TARGET is a real CACHE INTERNAL export now, visible
    # everywhere regardless of add_subdirectory() scope (unlike the old
    # plain find_package(JPEG REQUIRED) this used to call directly, which
    # is why sdext/CMakeLists.txt and vcl/CMakeLists.txt used to each call
    # their own copy too -- no longer necessary, but kept anyway since
    # TARGET_VAR here is cheaper than relying on load order).
    librestudio_find_package(NAME JPEG DRIVER libjpeg-turbo
        FORCE_VENDOR_VAR LIBRESTUDIO_FORCE_VENDOR_LIBJPEG_TURBO
        TARGET_VAR _poppler_jpeg_target)
    target_link_libraries(poppler PRIVATE ${_poppler_jpeg_target} ${_poppler_zlib_target})

    # poppler_data: rendering support for non-Latin encodings
    # (ExternalPackage_poppler_data.mk, read in full -- a hand-enumerated
    # list of several hundred files that is, structurally, just "copy these
    # 4 whole directories", so installed that way here rather than
    # transcribed name by name). Only needed (and only installed) on this
    # bundled branch -- a system poppler's own poppler-data package already
    # provides the same files.
    librestudio_fetch_tarball(poppler_data poppler-data-0.4.12.tar.gz
        c835b640a40ce357e1b83666aabd95edffa24ddddd49b8daff63adb851cdab74)
    foreach (_dir unicodeMap cMap nameToUnicode cidToUnicode)
        install(DIRECTORY "${poppler_data_SOURCE_DIR}/${_dir}"
            DESTINATION "share/xpdfimport/poppler_data")
    endforeach ()
    unset(_dir)

    # gb_LinkTarget__use_poppler's bundled branch (RepositoryExternal.
    # mk:3010-3015): root + poppler/ + goo/ only, not fofi/.
    set(LIBRESTUDIO_POPPLER_INCLUDE_DIRS
        "${poppler_SOURCE_DIR}"
        "${poppler_SOURCE_DIR}/poppler"
        "${poppler_SOURCE_DIR}/goo"
        CACHE INTERNAL "" FORCE)
    # sdext/CMakeLists.txt and vcl/CMakeLists.txt each also call
    # librestudio_find_package(NAME JPEG ...) themselves and link
    # LIBRESTUDIO_JPEG_TARGET directly, the same way they already
    # unconditionally link LIBRESTUDIO_ZLIB_TARGET regardless of which
    # poppler branch is active -- poppler.a needs both libjpeg and zlib
    # internally, and a static archive does not pull its own PRIVATE link
    # dependencies into a consumer's link line automatically.
    set(LIBRESTUDIO_POPPLER_DEFS BOOST_ALL_NO_LIB CACHE INTERNAL "" FORCE)

    librestudio_vendor_set_target(POPPLER "poppler;Threads::Threads")
endfunction()
