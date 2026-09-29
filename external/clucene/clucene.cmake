# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Vendoring driver for clucene, invoked by librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake, called from CMakeLists.txt's own root-level
# LIBCLUCENE_CORE resolution -- xmlhelp/CMakeLists.txt and helpcompiler/
# CMakeLists.txt are the two real consumers) only when no system
# libclucene-core was found.
#
# Module_clucene.mk registers exactly ONE target -- Library_clucene -- a
# real gbuild StaticLibrary-style direct compile (like libpng/graphite/
# md4c/libjxl), NOT an autotools ExternalProject, despite this port's own
# earlier "21 real patches" note in cmake/LibreStudioTarballs.cmake
# originally filing it under "harder". It bundles core + shared + contribs-
# lib (analysis/snowball stemmers) into ONE library, unlike a real system
# install which splits libclucene-core.so from libclucene-contribs-lib.so
# -- see helpcompiler/CMakeLists.txt's own header comment on that split,
# and the FOUND-branch handling added there for it below.
#
# _librestudio_driver_dir: see external/zlib/zlib.cmake's own header
# comment for why this is captured at file top level, outside the function.
set(_librestudio_driver_dir "${CMAKE_CURRENT_LIST_DIR}")

function(librestudio_vendor_clucene)
    message(STATUS "clucene: no system package found -- building clucene 2.3.3.4 from source")

    librestudio_find_package(NAME ZLIB TARGET_VAR _clucene_zlib_target)

    # download.lst's own CLUCENE_TARBALL/CLUCENE_SHA256SUM, verified
    # directly (grep -n clucene download.lst) -- the tarball's own
    # filename carries an upstream download-cache hash prefix, not part of
    # the unpacked top-level directory name (librestudio_fetch_tarball()
    # auto-detects that regardless).
    librestudio_fetch_tarball(clucene 48d647fbd8ef8889e5a7f422c1bfda94-clucene-core-2.3.3.4.tar.gz
        ddfdc433dd8ad31b5c5819cc4404a8d2127472a3b720d3e744e8c51d79732eab)

    # UnpackedTarball_clucene.mk's own gb_UnpackedTarball_add_file calls,
    # applied in gbuild's own fixed order (solenv/gbuild/UnpackedTarball.mk:
    # FILES, then PATCHES, then POST_ACTION -- confirmed by reading that
    # file directly, NOT the .mk's own statement order, which is not
    # execution order) -- BEFORE patches. Two pre-generated config headers,
    # picked the same way upstream's own ifeq chain does for a non-Windows,
    # Linux build (this port is Linux-only -- see cmake/
    # LibreStudioPlatform.cmake's own header comment): clucene-config.h
    # depends on HAVE_GCC_BUILTIN_ATOMIC (a real CACHE variable this port
    # already computes in cmake/LibreStudioConfigHeaders.cmake via
    # check_cxx_source_compiles, virtually always TRUE on a modern GCC/
    # Clang); _clucene-config.h is unconditionally the LINUX variant.
    if (HAVE_GCC_BUILTIN_ATOMIC)
        set(_clucene_config_h "clucene-config-GCC-atomic.h")
    else ()
        set(_clucene_config_h "clucene-config-generic.h")
    endif ()
    configure_file(
        "${_librestudio_driver_dir}/configs/${_clucene_config_h}"
        "${clucene_SOURCE_DIR}/src/shared/CLucene/clucene-config.h"
        COPYONLY)
    configure_file(
        "${_librestudio_driver_dir}/configs/_clucene-config-LINUX.h"
        "${clucene_SOURCE_DIR}/src/shared/CLucene/_clucene-config.h"
        COPYONLY)
    unset(_clucene_config_h)

    # UnpackedTarball_clucene.mk's own 21-entry patch list, plus
    # clucene-libcpp.patch (its own `ifneq ($(OS),WNT)` arm, always taken
    # here) = 22 total. Default patchlevel 0 (gb_UnpackedTarball_
    # set_patchlevel,clucene,0); clucene-reprobuild.patch.1 and clucene-
    # const.patch.1 each carry their own ".1" suffix, overriding to level 1.
    # contribs-lib-makefile.patch (present in external/clucene/patches/ but
    # never referenced by the .mk) is deliberately NOT applied -- it is an
    # unused leftover, not a missed patch (grepped: 0 references anywhere).
    librestudio_apply_patches(clucene "${clucene_SOURCE_DIR}" "${_librestudio_driver_dir}/patches" 0
        clucene-debug.patch
        clucene-multimap-put.patch
        clucene-narrowing-conversions.patch
        clucene-nullptr.patch
        clucene-warnings.patch
        clucene-git1-win64.patch
        clucene-ub.patch
        clucene-mutex.patch
        clucene-asan.patch
        clucene-mixes-uptemplate-parameter-msvc-14.patch
        ostream-wchar_t.patch
        heap-buffer-overflow.patch
        c++20.patch
        write-strings.patch
        nullstring.patch
        binary_function.patch
        clucene-pure-virtual.patch
        enumarith.patch
        clucene-reprobuild.patch.1
        clucene-const.patch.1
        wcsncpy.patch
        clucene-libcpp.patch)

    # UnpackedTarball_clucene.mk's own non-WNT gb_UnpackedTarball_
    # set_post_action, applied AFTER patches (same fixed gbuild ordering as
    # above): dirent.h is patched in place at its original src/shared/
    # location, then moved to inc/internal/ -- reproduced as a real
    # file-move, guarded so a second reconfigure of an already-vendored
    # tree (dirent.h already moved) does not fail trying to move a file
    # that is no longer at the source path.
    set(_clucene_dirent_src "${clucene_SOURCE_DIR}/src/shared/CLucene/util/dirent.h")
    set(_clucene_dirent_dest_dir "${clucene_SOURCE_DIR}/inc/internal/CLucene/util")
    if (EXISTS "${_clucene_dirent_src}")
        file(MAKE_DIRECTORY "${_clucene_dirent_dest_dir}")
        file(RENAME "${_clucene_dirent_src}" "${_clucene_dirent_dest_dir}/dirent.h")
    endif ()
    unset(_clucene_dirent_src)
    unset(_clucene_dirent_dest_dir)

    # Library_clucene.mk's own source lists, transcribed verbatim
    # (re-verified against the real 2.3.3.4 tarball: all 178 files exist, 0
    # missing) -- gb_Library_set_generated_cxx_suffix,clucene,cpp for the
    # first (149 .cpp files across shared/core/contribs-lib), gb_Library_
    # add_generated_cobjects for the second (29 .c files: libstemmer +
    # its per-language snowball stemmers + runtime).
    set(_clucene_cpp_sources
        src/shared/CLucene/SharedHeader
        src/shared/CLucene/config/gunichartables
        src/shared/CLucene/config/repl_tcslwr
        src/shared/CLucene/config/repl_tcstoll
        src/shared/CLucene/config/repl_tcscasecmp
        src/shared/CLucene/config/repl_tprintf
        src/shared/CLucene/config/repl_lltot
        src/shared/CLucene/config/repl_tcstod
        src/shared/CLucene/config/utf8
        src/shared/CLucene/config/threads
        src/shared/CLucene/debug/condition
        src/shared/CLucene/util/StringBuffer
        src/shared/CLucene/util/Misc
        src/shared/CLucene/util/dirent
        src/core/CLucene/StdHeader
        src/core/CLucene/debug/error
        src/core/CLucene/util/ThreadLocal
        src/core/CLucene/util/Reader
        src/core/CLucene/util/Equators
        src/core/CLucene/util/FastCharStream
        src/core/CLucene/util/MD5Digester
        src/core/CLucene/util/StringIntern
        src/core/CLucene/util/BitSet
        src/core/CLucene/queryParser/FastCharStream
        src/core/CLucene/queryParser/MultiFieldQueryParser
        src/core/CLucene/queryParser/QueryParser
        src/core/CLucene/queryParser/QueryParserTokenManager
        src/core/CLucene/queryParser/QueryToken
        src/core/CLucene/queryParser/legacy/Lexer
        src/core/CLucene/queryParser/legacy/MultiFieldQueryParser
        src/core/CLucene/queryParser/legacy/QueryParser
        src/core/CLucene/queryParser/legacy/QueryParserBase
        src/core/CLucene/queryParser/legacy/QueryToken
        src/core/CLucene/queryParser/legacy/TokenList
        src/core/CLucene/analysis/standard/StandardAnalyzer
        src/core/CLucene/analysis/standard/StandardFilter
        src/core/CLucene/analysis/standard/StandardTokenizer
        src/core/CLucene/analysis/Analyzers
        src/core/CLucene/analysis/AnalysisHeader
        src/core/CLucene/store/MMapInput
        src/core/CLucene/store/IndexInput
        src/core/CLucene/store/Lock
        src/core/CLucene/store/LockFactory
        src/core/CLucene/store/IndexOutput
        src/core/CLucene/store/Directory
        src/core/CLucene/store/FSDirectory
        src/core/CLucene/store/RAMDirectory
        src/core/CLucene/document/Document
        src/core/CLucene/document/DateField
        src/core/CLucene/document/DateTools
        src/core/CLucene/document/Field
        src/core/CLucene/document/FieldSelector
        src/core/CLucene/document/NumberTools
        src/core/CLucene/index/IndexFileNames
        src/core/CLucene/index/IndexFileNameFilter
        src/core/CLucene/index/IndexDeletionPolicy
        src/core/CLucene/index/SegmentMergeInfo
        src/core/CLucene/index/SegmentInfos
        src/core/CLucene/index/MergeScheduler
        src/core/CLucene/index/SegmentTermDocs
        src/core/CLucene/index/FieldsWriter
        src/core/CLucene/index/TermInfosWriter
        src/core/CLucene/index/Term
        src/core/CLucene/index/Terms
        src/core/CLucene/index/MergePolicy
        src/core/CLucene/index/DocumentsWriter
        src/core/CLucene/index/DocumentsWriterThreadState
        src/core/CLucene/index/SegmentTermVector
        src/core/CLucene/index/TermVectorReader
        src/core/CLucene/index/FieldInfos
        src/core/CLucene/index/CompoundFile
        src/core/CLucene/index/SkipListReader
        src/core/CLucene/index/SkipListWriter
        src/core/CLucene/index/IndexFileDeleter
        src/core/CLucene/index/SegmentReader
        src/core/CLucene/index/DirectoryIndexReader
        src/core/CLucene/index/TermVectorWriter
        src/core/CLucene/index/IndexReader
        src/core/CLucene/index/SegmentTermPositions
        src/core/CLucene/index/SegmentMerger
        src/core/CLucene/index/IndexWriter
        src/core/CLucene/index/MultiReader
        src/core/CLucene/index/MultiSegmentReader
        src/core/CLucene/index/Payload
        src/core/CLucene/index/SegmentTermEnum
        src/core/CLucene/index/TermInfo
        src/core/CLucene/index/IndexModifier
        src/core/CLucene/index/SegmentMergeQueue
        src/core/CLucene/index/FieldsReader
        src/core/CLucene/index/TermInfosReader
        src/core/CLucene/index/MultipleTermPositions
        src/core/CLucene/search/Compare
        src/core/CLucene/search/Scorer
        src/core/CLucene/search/ScorerDocQueue
        src/core/CLucene/search/PhraseScorer
        src/core/CLucene/search/SloppyPhraseScorer
        src/core/CLucene/search/DisjunctionSumScorer
        src/core/CLucene/search/ConjunctionScorer
        src/core/CLucene/search/PhraseQuery
        src/core/CLucene/search/PrefixQuery
        src/core/CLucene/search/ExactPhraseScorer
        src/core/CLucene/search/TermScorer
        src/core/CLucene/search/Similarity
        src/core/CLucene/search/BooleanScorer
        src/core/CLucene/search/BooleanScorer2
        src/core/CLucene/search/HitQueue
        src/core/CLucene/search/FieldCacheImpl
        src/core/CLucene/search/ChainedFilter
        src/core/CLucene/search/RangeFilter
        src/core/CLucene/search/CachingWrapperFilter
        src/core/CLucene/search/QueryFilter
        src/core/CLucene/search/TermQuery
        src/core/CLucene/search/FuzzyQuery
        src/core/CLucene/search/SearchHeader
        src/core/CLucene/search/RangeQuery
        src/core/CLucene/search/IndexSearcher
        src/core/CLucene/search/Sort
        src/core/CLucene/search/PhrasePositions
        src/core/CLucene/search/FieldDocSortedHitQueue
        src/core/CLucene/search/WildcardTermEnum
        src/core/CLucene/search/MultiSearcher
        src/core/CLucene/search/Hits
        src/core/CLucene/search/MultiTermQuery
        src/core/CLucene/search/FilteredTermEnum
        src/core/CLucene/search/FieldSortedHitQueue
        src/core/CLucene/search/WildcardQuery
        src/core/CLucene/search/Explanation
        src/core/CLucene/search/BooleanQuery
        src/core/CLucene/search/FieldCache
        src/core/CLucene/search/DateFilter
        src/core/CLucene/search/MatchAllDocsQuery
        src/core/CLucene/search/MultiPhraseQuery
        src/core/CLucene/search/ConstantScoreQuery
        src/core/CLucene/search/CachingSpanFilter
        src/core/CLucene/search/SpanQueryFilter
        src/core/CLucene/search/spans/NearSpansOrdered
        src/core/CLucene/search/spans/NearSpansUnordered
        src/core/CLucene/search/spans/SpanFirstQuery
        src/core/CLucene/search/spans/SpanNearQuery
        src/core/CLucene/search/spans/SpanNotQuery
        src/core/CLucene/search/spans/SpanOrQuery
        src/core/CLucene/search/spans/SpanScorer
        src/core/CLucene/search/spans/SpanTermQuery
        src/core/CLucene/search/spans/SpanWeight
        src/core/CLucene/search/spans/TermSpans
        src/contribs-lib/CLucene/analysis/cjk/CJKAnalyzer
        src/contribs-lib/CLucene/analysis/LanguageBasedAnalyzer
        src/contribs-lib/CLucene/analysis/PorterStemmer
        src/contribs-lib/CLucene/snowball/Snowball)
    set(_clucene_c_sources
        src/contribs-lib/CLucene/snowball/libstemmer/libstemmer
        src/contribs-lib/CLucene/snowball/src_c/stem_ISO_8859_1_danish
        src/contribs-lib/CLucene/snowball/src_c/stem_ISO_8859_1_dutch
        src/contribs-lib/CLucene/snowball/src_c/stem_ISO_8859_1_english
        src/contribs-lib/CLucene/snowball/src_c/stem_ISO_8859_1_finnish
        src/contribs-lib/CLucene/snowball/src_c/stem_ISO_8859_1_french
        src/contribs-lib/CLucene/snowball/src_c/stem_ISO_8859_1_german
        src/contribs-lib/CLucene/snowball/src_c/stem_ISO_8859_1_italian
        src/contribs-lib/CLucene/snowball/src_c/stem_ISO_8859_1_norwegian
        src/contribs-lib/CLucene/snowball/src_c/stem_ISO_8859_1_porter
        src/contribs-lib/CLucene/snowball/src_c/stem_ISO_8859_1_portuguese
        src/contribs-lib/CLucene/snowball/src_c/stem_ISO_8859_1_spanish
        src/contribs-lib/CLucene/snowball/src_c/stem_ISO_8859_1_swedish
        src/contribs-lib/CLucene/snowball/src_c/stem_KOI8_R_russian
        src/contribs-lib/CLucene/snowball/src_c/stem_UTF_8_danish
        src/contribs-lib/CLucene/snowball/src_c/stem_UTF_8_dutch
        src/contribs-lib/CLucene/snowball/src_c/stem_UTF_8_english
        src/contribs-lib/CLucene/snowball/src_c/stem_UTF_8_finnish
        src/contribs-lib/CLucene/snowball/src_c/stem_UTF_8_french
        src/contribs-lib/CLucene/snowball/src_c/stem_UTF_8_german
        src/contribs-lib/CLucene/snowball/src_c/stem_UTF_8_italian
        src/contribs-lib/CLucene/snowball/src_c/stem_UTF_8_norwegian
        src/contribs-lib/CLucene/snowball/src_c/stem_UTF_8_porter
        src/contribs-lib/CLucene/snowball/src_c/stem_UTF_8_portuguese
        src/contribs-lib/CLucene/snowball/src_c/stem_UTF_8_russian
        src/contribs-lib/CLucene/snowball/src_c/stem_UTF_8_spanish
        src/contribs-lib/CLucene/snowball/src_c/stem_UTF_8_swedish
        src/contribs-lib/CLucene/snowball/runtime/utilities
        src/contribs-lib/CLucene/snowball/runtime/api)

    set(_clucene_sources "")
    foreach (_f IN LISTS _clucene_cpp_sources)
        list(APPEND _clucene_sources "${clucene_SOURCE_DIR}/${_f}.cpp")
    endforeach ()
    foreach (_f IN LISTS _clucene_c_sources)
        list(APPEND _clucene_sources "${clucene_SOURCE_DIR}/${_f}.c")
    endforeach ()
    unset(_f)

    add_library(clucene STATIC ${_clucene_sources})
    # Library_clucene.mk's own gb_Library_set_include: internal (inc/
    # internal, where dirent.h was just moved to) + the three source roots.
    target_include_directories(clucene PRIVATE
        "${clucene_SOURCE_DIR}/inc/internal"
        "${clucene_SOURCE_DIR}/src/core"
        "${clucene_SOURCE_DIR}/src/contribs-lib"
        "${clucene_SOURCE_DIR}/src/shared")
    # gb_Library_set_warnings_disabled,clucene
    target_compile_options(clucene PRIVATE -w)
    set_target_properties(clucene PROPERTIES POSITION_INDEPENDENT_CODE ON)

    target_compile_definitions(clucene PRIVATE
        clucene_shared_EXPORTS
        clucene_core_EXPORTS
        clucene_contribs_lib_EXPORTS)
    # HAVE_LIBCPP is a real CACHE variable this port already hardcodes to 0
    # (cmake/LibreStudioConfigHeaders.cmake: "this build uses libstdc++"),
    # so this branch never actually taken here -- reproduced anyway for
    # fidelity with Library_clucene.mk's own ifeq chain (the $(COM),MSC
    # else-arm is skipped outright, same as every other Windows-only branch
    # in this port).
    if (HAVE_LIBCPP)
        target_compile_definitions(clucene PRIVATE
            _LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION)
    endif ()

    # Library_clucene.mk's own `ifeq ($(OS),LINUX)` arm -- always taken,
    # this port is Linux-only.
    target_link_libraries(clucene PRIVATE ${_clucene_zlib_target} m dl)
    target_link_options(clucene PRIVATE -pthread)

    librestudio_vendor_set_target(CLUCENE clucene)
endfunction()
