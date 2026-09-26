# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# o3tl -- header-only utility templates (sorted_vector, cow_wrapper,
# typed_flags_set, safeint, ...). Module_o3tl.mk's gb_Module_add_targets is
# empty: there is nothing to compile or link, the headers under include/o3tl
# are already reachable from every other module via the global include/
# directory (cmake/LibreStudioPlatform.cmake's include_directories() call).
#
# This target exists purely so an IDE that reads the CMake project (CLion,
# Qt Creator, Visual Studio, Xcode, ...) lists these headers under an "o3tl"
# node instead of not showing them at all -- add_custom_target(... SOURCES
# ...) with no OUTPUT/COMMAND is the standard CMake idiom for that: it adds
# no build step, produces nothing, and is never a real dependency of
# anything (no other target needs to, or should, link against it).

set(O3TL_HEADERS
    include/o3tl/any.hxx
    include/o3tl/BigEndianTypes.hxx
    include/o3tl/char16_t2wchar_t.hxx
    include/o3tl/compare.hxx
    include/o3tl/concepts.hxx
    include/o3tl/cow_wrapper.hxx
    include/o3tl/cppunittraitshelper.hxx
    include/o3tl/deleter.hxx
    include/o3tl/enumarray.hxx
    include/o3tl/enumrange.hxx
    include/o3tl/environment.hxx
    include/o3tl/float_int_conversion.hxx
    include/o3tl/functional.hxx
    include/o3tl/hash_combine.hxx
    include/o3tl/intcmp.hxx
    include/o3tl/lru_map.hxx
    include/o3tl/make_shared.hxx
    include/o3tl/nonstaticstring.hxx
    include/o3tl/numeric.hxx
    include/o3tl/runtimetooustring.hxx
    include/o3tl/safeCoInitUninit.hxx
    include/o3tl/safeint.hxx
    include/o3tl/sorted_vector.hxx
    include/o3tl/source_location.hxx
    include/o3tl/sprintf.hxx
    include/o3tl/string_view.hxx
    include/o3tl/strong_int.hxx
    include/o3tl/temporary.hxx
    include/o3tl/test_info.hxx
    include/o3tl/typed_flags_set.hxx
    include/o3tl/underlyingenumvalue.hxx
    include/o3tl/unit_conversion.hxx
    include/o3tl/unreachable.hxx
    include/o3tl/unsafe_downcast.hxx
    include/o3tl/untaint.hxx
    include/o3tl/vector_pool.hxx
    include/o3tl/vector_utils.hxx
)

list(TRANSFORM O3TL_HEADERS PREPEND "${CMAKE_SOURCE_DIR}/")

add_custom_target(o3tl SOURCES ${O3TL_HEADERS})

# --- o3tl_tests --------------------------------------------------------------
#
# CppunitTest_o3tl_tests.mk: use_libraries sal, plus gb_CppunitTest_use_sdk_api
# ("Needed for include/test/cppunitasserthelper.hxx").
#
# gb_CppunitTest_use_sdk_api == gb_CppunitTest_use_api($1, udkapi offapi),
# which -- via gb_LinkTarget__use_api -> gb_UnoApiHeadersTarget_get_target/
# get_dir (solenv/gbuild/UnoApiTarget.mk) -- resolves to the *plain* variant
# (cppumaker's -L flag), not gb_*_use_internal_comprehensive_api's -C variant.
# gb_UnoApiHeadersTarget_select_variant only redirects udkapi to comprehensive
# when DISABLE_DYNLOADING=TRUE (iOS/static-linking builds), which this desktop
# Linux port is not, so the plain variant is the correct one here. That is
# also exactly the mapping offapi/CMakeLists.txt documents next to its own
# "normal" target ("gb_Library_use_sdk_api (comphelper) resolves to the
# normal variant") and the one comphelper/CMakeLists.txt already consumes via
# udkapi_normal_HEADERS_INCLUDE_DIR / offapi_normal_HEADERS_INCLUDE_DIR -- so
# this reuses those same udkapi/offapi "normal" targets rather than the
# "comprehensive" ones cppuhelper needs.
librestudio_add_cppunit_test(o3tl_tests
    SOURCES
        o3tl/qa/cow_wrapper_clients
        o3tl/qa/test-cow_wrapper
        o3tl/qa/test-enumarray
        o3tl/qa/test-lru_map
        o3tl/qa/test-numeric
        o3tl/qa/test-safeint
        o3tl/qa/test-sorted_vector
        o3tl/qa/test-string_view
        o3tl/qa/test-temporary
        o3tl/qa/test-typed_flags
        o3tl/qa/test-unit_conversion
        o3tl/qa/test-vector_pool
        o3tl/qa/test-hash_combine
        o3tl/qa/BigEndianTypesTest
    LIBRARIES sal)

target_include_directories(o3tl_tests PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")

add_dependencies(o3tl_tests udkapi_normal_headers offapi_normal_headers)
