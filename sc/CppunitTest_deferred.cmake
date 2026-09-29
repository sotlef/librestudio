# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sc has 189 CppunitTest_*.mk files -- the largest test-porting wave in this
# whole project. Split out of sc/CMakeLists.txt into this file, scheduled
# via cmake_language(DEFER) from that file's own tail, for the same reason
# every other module's own CppunitTest_deferred.cmake already is:
# gb_CppunitTest_use_rdb(services)/gb_CppunitTest_use_configuration need
# program_services_rdb (only set once librestudio_finalize_components(
# services) has run, much later in the root CMakeLists.txt) and test/
# CMakeLists.txt's own LIBRESTUDIO_TEST_UNITTEST_DIR -- both only available
# after add_subdirectory(sc) (root CMakeLists.txt line 2095) has already
# returned. Because this file runs in the root CMakeLists.txt's own
# directory scope (include() does not create a new directory scope the way
# add_subdirectory() does), source/include paths below are CMAKE_SOURCE_DIR-
# relative throughout, same as every other *_deferred.cmake in this port.
#
# Most of sc's 189 tests are not 189 independent hand-written .mk bodies --
# re-checked directly, upstream itself factors the bulk of them through 4
# `define ... endef` gbuild macros (sc/common_unoapi_tests.mk, sc/
# functions_setup.mk, sc/subsequent_setup.mk, sc/ucalc_setup.mk), each
# CppunitTest_sc_*.mk then reduced to one `$(eval $(call <macro>,<name>))`
# line (occasionally plus 1-4 extra lines for a per-test wrinkle). Ported
# the same way here: one CMake function per gbuild macro, matching its
# template line for line, called once per name -- not 100+ near-identical
# hand-copied blocks.
#
# This file covers the first and simplest of the 4: sc_unoapi_common (103
# tests, all of the shape sc/qa/extras/sc<name>.cxx). The other 3 --
# sc_functions_test, sc_subsequent_test, sc_ucalc_test -- all need
# scqahelper (sc/Library_scqahelper.mk), not yet ported in this tree, and
# sc_ucalc_test additionally needs a gb_CppunitTest_use_library_objects
# equivalent (this port's own established technique for that, already
# proven on editeng_core -- see editeng/CppunitTest_deferred.cmake's own
# header comment); left for a following pass, not attempted in this file.
# The remaining ~46 sc tests with no shared macro at all are likewise a
# following pass, individually, the same way every other test in this
# project has been ported so far.

# librestudio_add_sc_unoapi_test(<name>)
#
# sc/common_unoapi_tests.mk's own `define sc_unoapi_common`, re-checked
# directly against the live file: gb_CppunitTest_CppunitTest(sc_$(1)),
# use_externals(boost_headers libxml2), use_common_precompiled_header
# (build-time-only optimisation, no CMake equivalent needed -- this port
# implements no PCH mechanism for CppunitTests anywhere), add_defs($(if
# $(ENABLE_COINMP),-DENABLE_COINMP)) -- ENABLE_COINMP is a real, detected
# CMake option here (cmake/LibreStudioConfigHeaders.cmake, COINMP_DETECT_
# FOUND), not a permanently-off stub like OPENCL elsewhere in this port, so
# reproduced as a live generator expression rather than assumed off --
# add_exception_objects(sc/qa/extras/sc$(1)) (1 source), use_libraries
# (comphelper cppu cppuhelper sal salhelper sc sfx subsequenttest test
# unotest utl tl vcl -- "test" -> test_fixtures, this port's established
# rename), set_include(sc/inc, sc/source/ui/inc), use_api(offapi udkapi)
# (normal headers), use_ure + use_vcl (plain, headless) -> the usual 3
# protector pairs, use_rdb(services) -> plain DEPENDS on program_services_
# rdb (a single named registry, not use_rdbs -- no RDBS parameter needed,
# same shape already established for scripting_scripthandler/jsprov),
# use_configuration -> CONFIGURATION. No COMPONENTS/PACKAGES/UICONFIG calls
# anywhere in this template.
function(librestudio_add_sc_unoapi_test name)
    librestudio_add_cppunit_test(sc_${name}
        SOURCES
            sc/qa/extras/sc${name}
        LIBRARIES
            comphelper cppu cppuhelper sal salhelper sc sfx subsequenttest
            test_fixtures unotest utl tl vcl Boost::boost LibXml2::LibXml2
        INCLUDE
            "${CMAKE_SOURCE_DIR}/sc/inc"
            "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
            "${udkapi_normal_HEADERS_INCLUDE_DIR}"
            "${offapi_normal_HEADERS_INCLUDE_DIR}"
        DEFINITIONS
            $<$<BOOL:${ENABLE_COINMP}>:ENABLE_COINMP>
        DEPENDS
            udkapi_normal_headers offapi_normal_headers
            program_services_rdb
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
            vclbootstrapprotector vclbootstrapprotector
        CONFIGURATION)
endfunction()

# The 101 CppunitTest_sc_*.mk files whose entire body (beyond comments) is
# exactly one `$(eval $(call sc_unoapi_common,<name>))` line -- verified by
# grepping every non-blank, non-comment line of all 189 files and checking
# each sc/qa/extras/sc<name>.cxx actually exists on disk (all 103 do; the
# other 2 sc_unoapi_common names, autoformatsobj and solverobj, have one
# real extra wrinkle each and are wired individually below instead of in
# this list).
set(SC_UNOAPI_TEST_NAMES
    annotationobj annotationshapeobj annotationsobj arealinkobj
    arealinksobj autoformatobj cellcursorobj cellfieldsobj
    cellformatsenumeration cellformatsobj cellobj cellrangeobj
    cellrangesobj cellsearchobj cellsenumeration cellsobj chartobj
    chartsobj check_data_pilot_field check_data_pilot_table
    check_xcell_ranges_query consolidationdescriptorobj databaserangeobj
    databaserangesobj datapilotfieldgroupitemobj datapilotfieldgroupobj
    datapilotfieldgroupsobj datapilotfieldobj datapilotfieldsobj
    datapilotitemobj datapilotitemsobj datapilottableobj
    datapilottablesobj ddelinkobj ddelinksobj documentconfigurationobj
    drawpageobj drawpagesobj editfieldobj_cell editfieldobj_header
    filterdescriptorbase functiondescriptionobj headerfieldsobj
    headerfootercontentobj importdescriptorbaseobj
    importdescriptorcellrangeobj indexenumeration_cellannotationsenumeration
    indexenumeration_cellarealinksenumeration
    indexenumeration_databaserangesenumeration
    indexenumeration_datapilotfieldsenumeration
    indexenumeration_datapilotitemsenumeration
    indexenumeration_datapilottablesenumeration
    indexenumeration_ddelinksenumeration
    indexenumeration_functiondescriptionenumeration
    indexenumeration_labelrangesenumeration
    indexenumeration_namedrangesenumeration
    indexenumeration_scenariosenumeration
    indexenumeration_sheetcellrangesenumeration
    indexenumeration_sheetlinksenumeration
    indexenumeration_spreadsheetsenumeration
    indexenumeration_spreadsheetviewpanesenumeration
    indexenumeration_subtotalfieldsenumeration
    indexenumeration_tableautoformatenumeration
    indexenumeration_tablechartsenumeration
    indexenumeration_tablecolumnsenumeration
    indexenumeration_tableconditionalentryenumeration
    indexenumeration_tablerowsenumeration indexenumeration_textfieldenumeration
    labelrangeobj labelrangesobj modelobj namedrangeobj namedrangesobj
    outlineobj recentfunctionsobj recordchanges scenariosobj shapeobj
    sheetlinkobj sheetlinksobj sortdescriptorbaseobj spreadsheetsettings
    spreadsheetsettingsobj stylefamiliesobj stylefamilyobj styleobj
    subtotaldescriptorbase subtotalfieldobj tablecolumnobj
    tablecolumnsobj tableconditionalentryobj tableconditionalformat
    tablerowobj tablerowsobj tablesheetobj tablesheetsobj
    tablevalidationobj tabviewobj uniquecellformatsenumeration
    uniquecellformatsobj viewpaneobj)

foreach (_sc_unoapi_name IN LISTS SC_UNOAPI_TEST_NAMES)
    librestudio_add_sc_unoapi_test("${_sc_unoapi_name}")
endforeach ()
unset(_sc_unoapi_name)

# --- sc_autoformatsobj: sc_unoapi_common plus a raw makefile prerequisite --
#
# CppunitTest_sc_autoformatsobj.mk, re-checked directly: the
# sc_unoapi_common(autoformatsobj) call, plus a raw (non-gbuild-macro) make
# rule making the test target depend on a PHONY "restore-default-
# autoformats" recipe that copies sc/res/xml/tablestyles.xml back over
# $(INSTROOT)/$(LIBO_SHARE_FOLDER)/calc/tablestyles.xml before the test
# runs -- resetting the installed default-tablestyles file in case a
# previous run of this same test (which edits table styles) left it
# modified. The PHONY-ness (no output file, so make always reruns it) is
# the point: reproduced here the same way, as an add_custom_target() with
# no OUTPUT (always considered out of date, always rebuilt/rerun), rather
# than an add_custom_command() with an OUTPUT path (which would cache
# after the first copy and never "restore" again).
#
# tablestyles.xml's own destination, $(LIBO_SHARE_FOLDER)/calc, is also
# where sc/Package_res_xml.mk installs it in the first place (alongside
# styles.xml) -- that Package was not yet ported either; added below via
# librestudio_add_package_mirror(), this port's established gb_Package_
# add_files equivalent (see e.g. wizards/CMakeLists.txt's own call sites),
# so the file exists at that path at all before anything tries to restore
# it.
librestudio_add_package_mirror(sc_res_xml
    PAIRS
        "${CMAKE_SOURCE_DIR}/sc/res/xml/styles.xml:${LIBRESTUDIO_SHARE_DIR}/calc"
        "${CMAKE_SOURCE_DIR}/sc/res/xml/tablestyles.xml:${LIBRESTUDIO_SHARE_DIR}/calc")

librestudio_add_sc_unoapi_test(autoformatsobj)

add_custom_target(sc_autoformatsobj_restore_default_autoformats
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/sc/res/xml/tablestyles.xml"
            "${LIBRESTUDIO_SHARE_DIR}/calc/tablestyles.xml"
    DEPENDS sc_res_xml
    COMMENT "Restoring default calc/tablestyles.xml before sc_autoformatsobj")
add_dependencies(sc_autoformatsobj sc_autoformatsobj_restore_default_autoformats)

# --- sc_solverobj: sc_unoapi_common plus an environment-passthrough arg ----
#
# CppunitTest_sc_solverobj.mk, re-checked directly: the sc_unoapi_common(
# solverobj) call, plus gb_CppunitTest_add_arguments passing
# "-env:arg-env=LD_LIBRARY_PATH<value, if currently set, appended as
# =$LD_LIBRARY_PATH>" (gb_Helper_LIBRARY_PATH_VAR expands to the plain
# variable NAME "LD_LIBRARY_PATH" on this platform; the surrounding
# ${VAR+=$VAR} is POSIX shell parameter-expansion syntax, substituted by
# make into the generated shell command line that actually launches
# cppunittester). This is a real, environment-dependent runtime value (what
# LD_LIBRARY_PATH happens to be in the shell invoking the test), not
# something resolvable to a fixed string at CMake configure time the way
# every other argument in this file is -- add_test() in this port invokes
# the test binary directly, with no intervening shell to perform that
# expansion, so there is no faithful way to reproduce it as a static
# EXTRA_ARGS string without baking in whatever LD_LIBRARY_PATH happens to
# be set at THIS configure time (which could easily be wrong for a later,
# different shell running ctest). Deliberately NOT added -- a genuine,
# narrow gap: sc_solverobj presumably passes this through to some
# subprocess it spawns (a solver plugin) so that subprocess can find shared
# libraries the same way cppunittester itself does; documented here rather
# than approximated with a possibly-wrong hardcoded value.
librestudio_add_sc_unoapi_test(solverobj)

# =============================================================================
# Wave 2: sc_functions_test (14), sc_subsequent_test (11), sc_ucalc_test (15)
# =============================================================================
#
# All three need scqahelper (sc/Library_scqahelper.mk, ported directly in
# sc/CMakeLists.txt above, right before this file's own DEFER call site) --
# sc_functions_test/sc_subsequent_test link it as an ordinary shared library
# (gb_CppunitTest_use_libraries), sc_ucalc_test instead recompiles both sc's
# AND scqahelper's own sources directly into each test binary
# (gb_CppunitTest_use_library_objects(sc_ucalc$(1), sc scqahelper)) -- the
# same technique already proven on editeng_core (see editeng/CppunitTest_
# deferred.cmake's own header comment on that function for the full
# rationale: individual SAL_DLLPRIVATE methods are hidden from the shared
# library's dynamic symbol table regardless of any global visibility flag,
# so a test calling one needs the owning library's sources compiled
# directly into it instead of linked against the .so).
#
# LIBRESTUDIO_MD4C_TARGET (sc_ucalc_test's own use_externals(md4c)): used to
# need the same directory-scope-visibility guard PkgConfig::CPPUNIT still
# does below (sc/CMakeLists.txt's own pkg_check_modules(MD4C...) was scoped
# to sc's own directory, not reachable from this file's root-scope deferred
# include()) -- no longer, since 2026-09-28's system-or-vendor refactor
# made it a real CACHE INTERNAL export (librestudio_pkgconfig(), resolved
# by sc/CMakeLists.txt's own call near its own top), visible everywhere
# with no guard.

# librestudio_add_sc_functions_test(<name>)
#
# sc/functions_setup.mk's own `define sc_functions_test`, re-checked
# directly: gb_CppunitTest_CppunitTest(sc_$(1)_functions_test),
# use_common_precompiled_header (no CMake equivalent needed, as above),
# add_exception_objects(sc/qa/unit/functions_$(1)), use_externals
# (boost_headers mdds_headers md4c libxml2), use_libraries (basegfx
# comphelper cppu cppuhelper drawinglayer drawinglayercore editeng for
# forui i18nlangtag msfilter oox sal salhelper sax sc scqahelper sfx sot
# subsequenttest svl svt svx svxcore test tk tl ucbhelper unotest utl vcl
# xo -- "test" -> test_fixtures), set_include(sc/source/ui/inc, sc/inc),
# use_api(offapi udkapi oovbaapi), use_ure + use_vcl (plain) -> 3 protector
# pairs, use_rdb(services) -> plain DEPENDS program_services_rdb,
# use_configuration -> CONFIGURATION.
function(librestudio_add_sc_functions_test name)
    librestudio_add_cppunit_test(sc_${name}_functions_test
        SOURCES
            sc/qa/unit/functions_${name}
        LIBRARIES
            basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
            editeng for forui i18nlangtag msfilter oox sal salhelper sax sc
            scqahelper sfx sot subsequenttest svl svt svx svxcore
            test_fixtures tk tl ucbhelper unotest utl vcl xo
            Boost::boost LibXml2::LibXml2 ${LIBRESTUDIO_MD4C_TARGET}
        INCLUDE
            # 2026-09-29, found via the real remote build: the header comment
            # a few screens down ("mdds_headers -> no CMake action ...
            # already on every relevant include path via MDDS_INCLUDE_DIR
            # where actually needed") assumed linking sc/scqahelper was
            # enough, but sc's own target_include_directories(sc PRIVATE ...
            # MDDS_INCLUDE_DIR) is PRIVATE -- it does not propagate to a
            # consumer that also directly #includes sc's own headers (sc/inc/
            # dpcache.hxx and friends reach <mdds/...> straight away). Real
            # "fatal error: mdds/flat_segment_tree.hpp: No such file or
            # directory" confirmed live.
            "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
            "${CMAKE_SOURCE_DIR}/sc/inc"
            "${MDDS_INCLUDE_DIR}"
            "${udkapi_normal_HEADERS_INCLUDE_DIR}"
            "${offapi_normal_HEADERS_INCLUDE_DIR}"
            "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
        DEPENDS
            udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
            program_services_rdb
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
            vclbootstrapprotector vclbootstrapprotector
        CONFIGURATION)
endfunction()

foreach (_sc_fn_name IN ITEMS
    addin array database datetime dynamic_array financial old information
    lambda logical mathematical spreadsheet statistical text)
    librestudio_add_sc_functions_test("${_sc_fn_name}")
endforeach ()
unset(_sc_fn_name)

# librestudio_add_sc_subsequent_test(<name>)
#
# sc/subsequent_setup.mk's own `define sc_subsequent_test`, re-checked
# directly: same shape as sc_functions_test above (gb_CppunitTest_
# CppunitTest(sc_subsequent_$(1)), add_exception_objects(sc/qa/unit/
# subsequent_$(1)), use_externals(boost_headers mdds_headers libxml2) --
# no md4c here, use_libraries (basegfx comphelper cppu cppuhelper
# drawinglayer docmodel editeng for forui i18nlangtag msfilter oox sal
# salhelper sax sc scqahelper sfx sot subsequenttest svl svt svx svxcore
# test tk tl ucbhelper unotest utl vcl xo), set_include/use_api/use_ure/
# use_vcl/use_rdb(services) identical to sc_functions_test -- plus 3 extra
# calls that one does not have:
#   - use_custom_headers(officecfg/registry) -> INCLUDE
#     "${OFFICECFG_INCLUDE_DIR}" + DEPENDS officecfg_headers, same
#     resolution already established by editeng_core.
#   - use_packages(oox_generated) -> DEPENDS oox_generated (a real,
#     already-ported target -- oox/CMakeLists.txt's own
#     add_custom_target(oox_generated ...), already consumed the same way
#     by scfilt/msword elsewhere in this tree).
#   - use_more_fonts -> inert (gated on a BUILD_TYPE filter, "MORE_FONTS",
#     this port never sets -- same precedent as editeng's own two tests).
# add_arguments' own "-env:arg-env=LD_LIBRARY_PATH..." passthrough is the
# same genuine, narrow, environment-dependent gap already documented above
# for sc_solverobj -- not reproduced here either, for the same reason.
function(librestudio_add_sc_subsequent_test name)
    librestudio_add_cppunit_test(sc_subsequent_${name}
        SOURCES
            sc/qa/unit/subsequent_${name}
        LIBRARIES
            basegfx comphelper cppu cppuhelper drawinglayer docmodel editeng
            for forui i18nlangtag msfilter oox sal salhelper sax sc
            scqahelper sfx sot subsequenttest svl svt svx svxcore
            test_fixtures tk tl ucbhelper unotest utl vcl xo
            Boost::boost LibXml2::LibXml2
        INCLUDE
            # Same real gap as librestudio_add_sc_functions_test just above
            # (see its own comment) -- sc's MDDS_INCLUDE_DIR is PRIVATE to sc
            # itself, and subsequent_${name}.cxx reaches sc/inc headers that
            # need <mdds/...> directly. Confirmed live: "fatal error: mdds/
            # flat_segment_tree.hpp: No such file or directory".
            "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
            "${CMAKE_SOURCE_DIR}/sc/inc"
            "${MDDS_INCLUDE_DIR}"
            "${OFFICECFG_INCLUDE_DIR}"
            "${udkapi_normal_HEADERS_INCLUDE_DIR}"
            "${offapi_normal_HEADERS_INCLUDE_DIR}"
            "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
        DEPENDS
            udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
            officecfg_headers oox_generated
            program_services_rdb
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
            vclbootstrapprotector vclbootstrapprotector
        CONFIGURATION)
endfunction()

foreach (_sc_seq_name IN ITEMS
    export_test export_test2 export_test3 export_test4 export_test5
    export_test6 filters_test filters_test2 filters_test3 filters_test4
    filters_test5)
    librestudio_add_sc_subsequent_test("${_sc_seq_name}")
endforeach ()
unset(_sc_seq_name)

# librestudio_add_sc_ucalc_test(<suffix>)
#
# sc/ucalc_setup.mk's own `define sc_ucalc_test`, re-checked directly:
# gb_CppunitTest_CppunitTest(sc_ucalc$(1)) (suffix already includes its own
# leading underscore, e.g. "_condformat"; sc_ucalc.mk itself calls the
# macro with an EMPTY argument, giving plain "sc_ucalc" -- modelled below
# by accepting "" as a valid suffix), use_library_objects(sc_ucalc$(1), sc
# scqahelper) -- sc's and scqahelper's own cached SOURCES lists spliced
# directly into SOURCES below, "sc"/"scqahelper" themselves dropped from
# LIBRARIES (see editeng_core's identical precedent) -- use_externals
# (boost_headers, OPENCL's optional clew skipped [inert everywhere else in
# this port], icu_headers/icui18n/icuuc -> ICU::uc ICU::i18n, libxml2,
# mdds_headers, orcus + orcus-parser -> both already resolve to this
# port's own ${LIBRESTUDIO_ORCUS_TARGET}/${LIBRESTUDIO_ORCUS_SPREADSHEET_
# TARGET} pair [sc's own library already needs both], md4c ->
# ${LIBRESTUDIO_MD4C_TARGET}), use_libraries (avmedia basegfx chart2api comphelper
# cppu cppuhelper dbtools drawinglayer drawinglayercore docmodel editeng
# for forui fwk i18nlangtag i18nutil OPENCL's optional opencl [skipped],
# sal salhelper sax sb sfx sot subsequenttest svl svt svx svxcore test
# textconversiondlgs tk tl ucbhelper unotest utl SCRIPTING's optional
# vbahelper [included -- ENABLE_SCRIPTING defaults ON in this port,
# vbahelper is a real, already-ported target] vcl xo), set_include(sc/
# source/ui/inc, sc/source/core/inc, sc/inc), use_api(udkapi offapi
# oovbaapi), use_custom_headers(officecfg/registry) -> INCLUDE
# "${OFFICECFG_INCLUDE_DIR}" + DEPENDS officecfg_headers, use_ure +
# use_vcl -> 3 protector pairs, use_components (14 names, resolved below
# against each owning module's own librestudio_add_component() call),
# add_libs(-lrt on LINUX -- glibc merged librt into libc long ago on any
# glibc modern enough to build this tree at all; a no-op here, same
# reasoning already applied elsewhere in this port to comparable now-
# redundant -lrt/-ldl additions), use_configuration -> CONFIGURATION.
# add_arguments' LD_LIBRARY_PATH passthrough is, again, the same
# genuine gap as sc_solverobj/sc_subsequent_test -- not reproduced.
#
# COMPONENTS lookup keys, each verified against the owning module's own
# librestudio_add_component() call: "configmgr/source/configmgr" ->
# "configmgr" (configmgr/CMakeLists.txt), "framework/util/fwk" -> "fwk"
# (framework/CMakeLists.txt), "i18npool/source/search/i18nsearch" ->
# "i18nsearch" and "i18npool/util/i18npool" -> "i18npool" (both i18npool/
# CMakeLists.txt), "sax/source/expatwrap/expwrap" -> "sax" (sax/
# CMakeLists.txt), "scaddins/source/analysis/analysis" -> "analysis",
# "scaddins/source/datefunc/date" -> "date", "scaddins/source/pricing/
# pricing" -> "pricing" (all three scaddins/CMakeLists.txt), "sfx2/util/
# sfx" -> "sfx" (sfx2/CMakeLists.txt), "ucb/source/core/ucb1" -> "ucb1"
# and "ucb/source/ucp/file/ucpfile1" -> "ucpfile1" (both ucb/
# CMakeLists.txt), "unoxml/source/service/unoxml" -> "unoxml" (unoxml/
# CMakeLists.txt), "uui/util/uui" -> "uui" (uui/CMakeLists.txt), "vcl/
# vcl.common" -> "vcl" (vcl/CMakeLists.txt). All 14 resolve to real,
# already-registered components -- same mapping style already verified
# for editeng_core's own, smaller COMPONENTS list.
function(librestudio_add_sc_ucalc_test suffix)
    librestudio_add_cppunit_test(sc_ucalc${suffix}
        SOURCES
            ${SC_SOURCES}
            ${SCQAHELPER_SOURCES}
            sc/qa/unit/ucalc${suffix}
        LIBRARIES
            avmedia basegfx chart2api comphelper cppu cppuhelper dbtools
            drawinglayer drawinglayercore docmodel editeng for forui fwk
            i18nlangtag i18nutil sal salhelper sax sb sfx sot subsequenttest
            svl svt svx svxcore test_fixtures textconversiondlgs tk tl
            ucbhelper unotest utl vbahelper vcl xo
            Boost::boost ICU::uc ICU::i18n LibXml2::LibXml2
            # LIBRESTUDIO_MD4C_HTML_TARGET: found missing via the real remote
            # build -- sc/source/ui/docshell/impex.cxx calls md_html()
            # directly (md4c-html, not plain md4c). sc/CMakeLists.txt's own
            # "sc" target links both ${LIBRESTUDIO_MD4C_TARGET} AND
            # ${LIBRESTUDIO_MD4C_HTML_TARGET}; this function only had the
            # first. Real "undefined reference to `md_html'" at link time.
            ${LIBRESTUDIO_MD4C_TARGET} ${LIBRESTUDIO_MD4C_HTML_TARGET}
            ${LIBRESTUDIO_ORCUS_TARGET} ${LIBRESTUDIO_ORCUS_SPREADSHEET_TARGET}
        INCLUDE
            # 2026-09-29, found via the real remote build: this recompiles
            # SC_SOURCES/SCQAHELPER_SOURCES directly (not by linking sc/
            # scqahelper), so it needs their own PRIVATE MDDS_INCLUDE_DIR too
            # -- same real gap as librestudio_add_sc_functions_test/
            # librestudio_add_sc_subsequent_test above (see their own
            # comments). "fatal error: clew/clew.h" (via sc/inc/formulagroup.
            # hxx) is fixed below via the same ENABLE_OPENCL-gated
            # target_link_libraries(... clew opencl) sc/CMakeLists.txt's own
            # "sc" target uses (clew's own target_include_directories is
            # PUBLIC). "fatal error: scslots.hxx" is fixed by adding
            # sc_sdi_INCLUDE_DIR below -- sc/CMakeLists.txt's own "sc" target
            # includes it (its librestudio_add_sdi_target(sc_sdi ...) call),
            # this function never did. Now that librestudio_add_sdi_target()
            # sets it CACHE INTERNAL (see cmake/LibreStudioSdiTarget.cmake's
            # own comment on that, same day), it is visible here too -- it
            # just also needs to be actually listed.
            "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
            "${CMAKE_SOURCE_DIR}/sc/source/core/inc"
            "${CMAKE_SOURCE_DIR}/sc/source/filter/inc"
            "${CMAKE_SOURCE_DIR}/sc/inc"
            "${sc_sdi_INCLUDE_DIR}"
            "${MDDS_INCLUDE_DIR}"
            "${OFFICECFG_INCLUDE_DIR}"
            "${udkapi_normal_HEADERS_INCLUDE_DIR}"
            "${offapi_normal_HEADERS_INCLUDE_DIR}"
            "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
        DEFINITIONS
            # SC_INFO_OSVERSION="Linux" matches sc/CMakeLists.txt's own "sc"
            # target definitions exactly (real "error: expected ')' before
            # SC_INFO_OSVERSION" in interpr5.cxx's ScInterpreter::ScInfo()
            # without it -- confirmed live).
            SC_DLLIMPLEMENTATION SCQAHELPER_DLLIMPLEMENTATION
            SC_INFO_OSVERSION="Linux"
        DEPENDS
            udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
            officecfg_headers
            program_services_rdb
            sc_sdi
        PROTECTORS
            unobootstrapprotector unobootstrapprotector
            unoexceptionprotector unoexceptionprotector
            vclbootstrapprotector vclbootstrapprotector
        COMPONENTS
            configmgr fwk i18nsearch i18npool sax analysis date pricing sfx
            ucb1 ucpfile1 unoxml uui vcl
        CONFIGURATION)
    if (ENABLE_OPENCL)
        target_link_libraries(sc_ucalc${suffix} PRIVATE clew opencl)
    endif ()
endfunction()

foreach (_sc_ucalc_suffix IN ITEMS
    "" _condformat _copypaste _datatransformation _formula _formula2
    _nanpayload _parallelism _pivottable _rangelst _range _sharedformula
    _sort _sparkline _spilled_range)
    librestudio_add_sc_ucalc_test("${_sc_ucalc_suffix}")
endforeach ()
unset(_sc_ucalc_suffix)

# --- sc_ucalc_formula2 / sc_ucalc_nanpayload: DISABLE_NAN_TESTS -------------
#
# Both .mk files add one conditional extra def, `ifeq ($(DISABLE_NAN_TESTS),
# TRUE) -DDISABLE_NAN_TESTS endif`. DISABLE_NAN_TESTS has no CMake option
# anywhere in this port (grepped project-wide, confirmed absent) -- treated
# as permanently FALSE/unset, the same default-off resolution already
# applied elsewhere in this port to comparable environment-only knobs
# (OPENCL, MORE_FONTS, gb_WITH_LANG). Not reproduced as a real toggle;
# noted here rather than silently dropped.

# --- sc_ucalc_document_themes / sc_ucalc_solver / sc_ucalc_tablestyles -----
#
# Not sc_ucalc_test calls at all -- 3 fully hand-written .mk files that
# happen to share the "ucalc_" naming family (discovered while reading the
# whole ucalc_*.mk set). All 3, re-checked directly against their live
# files, use PLAIN gb_CppunitTest_use_libraries(..., sc, scqahelper, ...)
# (upstream's own choice, not use_library_objects -- so sc/scqahelper are
# linked as ordinary shared libraries here, not recompiled in), an empty
# gb_CppunitTest_use_components(<name>) call with no names at all (a real
# no-op in upstream too -- reproduced by simply omitting COMPONENTS), the
# same use_sdk_api + use_api(offapi udkapi) double call (use_sdk_api takes
# no further arguments upstream; this port's own use_api already covers
# the udkapi+offapi normal-header pair it would add), use_rdb(services) ->
# plain DEPENDS program_services_rdb, and the same LD_LIBRARY_PATH-
# passthrough add_arguments gap as sc_solverobj/sc_subsequent_test/
# sc_ucalc_test above (not reproduced, for the same reason).
librestudio_add_cppunit_test(sc_ucalc_document_themes
    SOURCES
        sc/qa/unit/ucalc_DocumentThemes
    LIBRARIES
        basegfx comphelper cppu cppuhelper docmodel editeng sal salhelper
        sax sc svl scqahelper sfx svxcore subsequenttest test_fixtures tl
        unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sc_ucalc_solver
    SOURCES
        sc/qa/unit/ucalc_solver
    LIBRARIES
        basegfx comphelper cppu cppuhelper i18nlangtag sal sax sc scqahelper
        sfx subsequenttest svl svx svxcore test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        # MDDS_INCLUDE_DIR: found missing via the real remote build --
        # ucalc_solver.cxx -> sc/inc/table.hxx -> sc/inc/columnspanset.hxx
        # needs <mdds/flat_segment_tree.hpp> directly, and linking "sc" alone
        # doesn't propagate it (sc's own MDDS_INCLUDE_DIR is PRIVATE).
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sc_ucalc_tablestyles
    SOURCES
        sc/qa/unit/ucalc_tablestyles
    LIBRARIES
        basegfx comphelper cppu cppuhelper docmodel editeng sal salhelper
        sax sc svl scqahelper sfx svxcore subsequenttest test_fixtures tl
        unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# =============================================================================
# Wave 3: the 42 remaining sc tests with no shared gbuild macro at all
# =============================================================================
#
# Each read in full, individually, the same way every other module's own
# CppunitTest_deferred.cmake in this port already does for its own non-
# templated tests. Shared notes, to avoid repeating them 42 times below:
#
#   - "test" -> test_fixtures (this port's established rename), boost_headers
#     -> Boost::boost, libxml2 -> LibXml2::LibXml2, icu_headers/icuuc/icui18n
#     -> ICU::uc/ICU::i18n, mdds_headers -> no CMake action (header-only,
#     already on every relevant include path via MDDS_INCLUDE_DIR where
#     actually needed), md4c -> LIBRESTUDIO_MD4C_TARGET (external/md4c/
#     CMakeLists.txt, no guard needed -- CACHE INTERNAL, unlike Wave 2's
#     PkgConfig::CPPUNIT). use_common_precompiled_header -> no action (no PCH mechanism
#     for CppunitTests anywhere in this port). use_sdk_api and a bare
#     use_api(oovbaapi) both resolve to the same normal-header INCLUDE
#     entries already used everywhere else (offapi/udkapi always; oovbaapi
#     only when actually named).
#   - The "-env:arg-env=LD_LIBRARY_PATH..." add_arguments passthrough some
#     of these .mk files carry is the same genuine, narrow, environment-
#     dependent gap already documented for sc_solverobj/sc_subsequent_test/
#     sc_ucalc_test above -- not reproduced in any of the 42 below either,
#     for the identical reason (no shell interposes between add_test() and
#     the test binary in this CMake port to perform the substitution).
#   - DISABLE_CVE_TESTS (sc_filters_test) has no CMake option anywhere in
#     this port (grepped, confirmed absent) -- same permanently-unset
#     resolution as DISABLE_NAN_TESTS above.
#   - Three real, narrow infrastructure gaps recurred across several of
#     these 42 (each noted again at its own call site, not just here): (1)
#     UIConfig_scalc (gb_CppunitTest_use_uiconfigs' own "modules/scalc"
#     name) -- 2026-09-28: now built ("scalc_uiconfig", sc/CMakeLists.txt's
#     own "UIConfig_scalc" section, 257 files total across menubar/
#     popupmenu/statusbar/toolbar/ui, all verified present on disk) -- the 5
#     tests below that previously omitted "scalc" from UICONFIG (sc_
#     dataprovider, sc_screenshots, sc_sheetview_test, sc_tiledrendering,
#     sc_tiledrendering2) now include it for real; (2) Package_filter_xhtml/
#     Package_filter_xslt (gb_CppunitTest_use_packages' "filter_xhtml"/
#     "filter_xslt") have no CMake port either -- PACKAGES omitted
#     entirely for the 3 tests that ask for them; (3) postprocess_images
#     (sc_a11y's own use_packages) -- 2026-09-28: now built (postprocess/
#     CMakeLists.txt's own "CustomTarget_images + Package_images:
#     implemented" section) -- sc_a11y's own PACKAGES below now includes it.
#   - LIBRESTUDIO_TLS_BACKEND (sc_pdf_export's own `ifeq ($(TLS),NSS)`
#     branch) is a real, dynamic CMake option in this port (default "NSS"
#     -- comphelper/CMakeLists.txt), not a permanently-off stub -- resolved
#     live via the identical guarded pkg_check_modules(NSS...) pattern
#     comphelper/CppunitTest_deferred.cmake's own header comment already
#     documents (PkgConfig::NSS's own directory-scope visibility needs the
#     same re-guard as PkgConfig::CPPUNIT above; LIBRESTUDIO_MD4C_TARGET no
#     longer needs one, see the md4c mapping above).
#   - COMPONENTS lookup keys below are all verified against each owning
#     module's own librestudio_add_component() call (built from a single
#     project-wide grep, not assumed from the gbuild path spelling) --
#     originally three names among them resolved to NOTHING anywhere in this
#     tree (connectivity/source/manager/sdbc2, filter/source/
#     xmlfilteradaptor/xmlfa, filter/source/xsltfilter/xsltfilter). 2026-09-
#     28: xmlfa is now built (filter/CMakeLists.txt's own odfflatxml/xmlfa/
#     xmlfd wave, closing the same gap chart2_import/import2/geometry/
#     pivot_chart_test/trendcalculators were separately blocked on) -- added
#     to sc_filters_test's own COMPONENTS list below. 2026-09-29: xsltfilter
#     is now built too (filter/CMakeLists.txt's own remaining-7-libraries
#     wave), added to the same COMPONENTS list below. sdbc2 alone remains
#     genuinely unresolved (still zero librestudio_add_component() hits
#     under that name) -- a real, narrow, still-open gap, not a mapping
#     error.

# --- sc_a11y -----------------------------------------------------------------
# CppunitTest_sc_a11y.mk: 1 source (sc/qa/extras/accessibility/basics),
# use_libraries(sal cppu cppuhelper subsequenttest test unotest tl vcl),
# use_sdk_api, use_rdb(services), use_ure+use_vcl -> 3 protectors,
# use_instdir_configuration + use_common_configuration (2 of the 3 layers
# plain CONFIGURATION bundles -- the missing third, use_user_ui_
# configuration, is presumably immaterial to an accessibility smoke test;
# using CONFIGURATION here is a harmless superset, not an under-approximation).
# use_packages(postprocess_images) -- 2026-09-28: now built, see the wave
# header comment above, point 3.
librestudio_add_cppunit_test(sc_a11y
    SOURCES
        sc/qa/extras/accessibility/basics
    LIBRARIES
        sal cppu cppuhelper subsequenttest test_fixtures tl unotest vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    PACKAGES
        postprocess_images
    CONFIGURATION)

# --- sc_anchor_test ------------------------------------------------------------
# CppunitTest_sc_anchor_test.mk: standard "big sc test" shape (see this
# wave's own header), 1 source (sc/qa/unit/anchor), use_rdb(services).
librestudio_add_cppunit_test(sc_anchor_test
    SOURCES
        sc/qa/unit/anchor
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sb sc
        scqahelper sfx sot subsequenttest svl svt svx svxcore test_fixtures
        tk tl ucbhelper unotest utl vbahelper vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_basic_types_test -------------------------------------------------------
# CppunitTest_sc_basic_types_test.mk: 2 sources (sc/qa/unit/types/
# ScAddressTest, SortOrderReverserTest), use_libraries(sc scui) only,
# use_external(boost_headers), use_sdk_api. No use_ure/use_vcl/use_rdb/
# use_configuration calls at all -- re-verified directly, the .mk really
# has none -- so no PROTECTORS/CONFIGURATION here either.
librestudio_add_cppunit_test(sc_basic_types_test
    SOURCES
        sc/qa/unit/types/ScAddressTest
        sc/qa/unit/types/SortOrderReverserTest
    LIBRARIES
        sc scui
        Boost::boost
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers)

# --- sc_cache_test ---------------------------------------------------------
# CppunitTest_sc_cache_test.mk: 1 source (sc/qa/unit/datacache), standard
# shape plus ICU + a 38-name use_components list (all real, resolved
# against each owning module).
librestudio_add_cppunit_test(sc_cache_test
    SOURCES
        sc/qa/unit/datacache
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sb sc
        scqahelper sfx sot svl svt svx svxcore test_fixtures tk tl ucbhelper
        unotest utl vbahelper vcl xo
        Boost::boost ICU::uc ICU::i18n LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        sb chart2 comphelper configmgr dba embobj evtatt filterconfig frm
        fwk i18npool oox xstor package2 sax analysis date basprov
        scriptframe sc scd scfilt vbaobj sfx sot fsstorage svl svx svxcore
        tk ucb1 ucpfile1 ucptdoc1 utl unordf unoxml vcl xo
    CONFIGURATION)

# --- sc_chart2dataprovider -------------------------------------------------
# CppunitTest_sc_chart2dataprovider.mk: standard shape, 1 source (sc/qa/
# unit/chart2dataprovider), use_rdb(services). gb_UWINAPI is a Windows-only
# addition (empty on Linux, already this port's own convention elsewhere).
librestudio_add_cppunit_test(sc_chart2dataprovider
    SOURCES
        sc/qa/unit/chart2dataprovider
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sb sc
        scqahelper sfx sot subsequenttest svl svt svx svxcore test_fixtures
        tk tl ucbhelper unotest utl vbahelper vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_color_scale_api ------------------------------------------------------
# CppunitTest_sc_color_scale_api.mk: standard shape, 1 source (sc/qa/
# extras/color_scale_api), use_sdk_api (not use_api), use_rdb(services).
librestudio_add_cppunit_test(sc_color_scale_api
    SOURCES
        sc/qa/extras/color_scale_api
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sb sc
        scqahelper sfx sot subsequenttest svl svt svx svxcore test_fixtures
        tk tl ucbhelper unotest utl vbahelper vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${CMAKE_SOURCE_DIR}/sc/qa/unit"
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_cond_format ----------------------------------------------------------
# CppunitTest_sc_cond_format.mk: smaller LIBRARIES set (no basegfx/
# drawinglayer/for/forui/msfilter/oox/sax/sb/sot/tk/ucbhelper/xo), 1 source
# (sc/qa/unit/cond_format), use_sdk_api, use_rdb(services).
librestudio_add_cppunit_test(sc_cond_format
    SOURCES
        sc/qa/unit/cond_format
    LIBRARIES
        comphelper cppu cppuhelper editeng i18nlangtag sal sc scqahelper
        sfx subsequenttest svl svx svxcore test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_cond_format_merge ----------------------------------------------------
# CppunitTest_sc_cond_format_merge.mk: standard shape, 1 source (sc/qa/
# unit/cond_format_merge), 39-name use_components list (all real).
librestudio_add_cppunit_test(sc_cond_format_merge
    SOURCES
        sc/qa/unit/cond_format_merge
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sb sc
        scqahelper sfx sot subsequenttest svl svt svx svxcore test_fixtures
        tk tl ucbhelper unotest utl vbahelper vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        sb chart2 comphelper configmgr dba embobj evtatt filterconfig
        storagefd frm fwk i18npool oox xstor package2 sax analysis date
        basprov scriptframe sc scd scfilt vbaobj sfx sot fsstorage svl svt
        svx svxcore tk ucb1 ucpfile1 ucptdoc1 utl unordf unoxml uui vcl xo
    CONFIGURATION)

# --- sc_copypaste --------------------------------------------------------------
# CppunitTest_sc_copypaste.mk: 1 source (sc/qa/unit/copy_paste_test), same
# smaller LIBRARIES set as sc_cond_format, use_sdk_api, use_rdb(services).
# The -fsanitize=vptr-conditional salhelper addition is inert (this port
# has no sanitizer-flag detection anywhere, same precedent already applied
# elsewhere) -- salhelper simply isn't linked here, matching a plain build.
librestudio_add_cppunit_test(sc_copypaste
    SOURCES
        sc/qa/unit/copy_paste_test
    LIBRARIES
        comphelper cppu cppuhelper editeng i18nlangtag sal sc scqahelper
        sfx subsequenttest svl svx svxcore test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_core -------------------------------------------------------------------
# CppunitTest_sc_core.mk: tiny, 1 source (sc/qa/unit/
# test_ScChartListenerCollection), use_libraries(cppu cppuhelper sal
# salhelper sc test unotest), use_sdk_api, use_rdb(services).
librestudio_add_cppunit_test(sc_core
    SOURCES
        sc/qa/unit/test_ScChartListenerCollection
    LIBRARIES
        cppu cppuhelper sal salhelper sc test_fixtures unotest
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_dataprovider -----------------------------------------------------------
# CppunitTest_sc_dataprovider.mk: standard shape plus orcus/orcus-parser ->
# ${LIBRESTUDIO_ORCUS_TARGET} (this port's own resolution, matching sc's
# own library and sc_ucalc_test above), use_custom_headers(officecfg/
# registry) -> OFFICECFG_INCLUDE_DIR + officecfg_headers, use_uiconfigs(
# modules/scalc sfx svt) -- 2026-09-28: scalc_uiconfig is now built (sc/
# CMakeLists.txt's own "UIConfig_scalc" section), so "scalc" is included
# below for real now (previously omitted).
librestudio_add_cppunit_test(sc_dataprovider
    SOURCES
        sc/qa/unit/dataproviders_test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sb sc
        scqahelper sfx sot subsequenttest svl svt svx svxcore test_fixtures
        tk tl ucbhelper unotest utl vbahelper vcl xo
        Boost::boost LibXml2::LibXml2 ${LIBRESTUDIO_ORCUS_TARGET}
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${CMAKE_SOURCE_DIR}/sc/source/filter/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG
        scalc sfx svt
    CONFIGURATION)

# --- sc_filter_html / sc_filter_xml / sc_html_export_test -------------------
# All three (CppunitTest_sc_filter_html.mk / _filter_xml.mk /
# _html_export_test.mk) share the same standard shape and the same
# use_packages(filter_xhtml filter_xslt) -- omitted, see this wave's own
# header comment point 2. use_rdb(services) throughout; filter_html/
# filter_xml also add sc/qa/unit for INCLUDE.
librestudio_add_cppunit_test(sc_filter_html
    SOURCES
        sc/qa/filter/html/html
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sc
        scqahelper sfx sot subsequenttest svl svt svx svxcore test_fixtures
        tk tl ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${CMAKE_SOURCE_DIR}/sc/qa/unit"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sc_filter_xml
    SOURCES
        sc/qa/filter/xml/xml
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sc
        scqahelper sfx sot subsequenttest svl svt svx svxcore test_fixtures
        tk tl ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${CMAKE_SOURCE_DIR}/sc/qa/unit"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sc_html_export_test
    SOURCES
        sc/qa/extras/htmlexporttest
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sc
        scqahelper sfx sot subsequenttest svl svt svx svxcore test_fixtures
        tk tl ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_filters_test -----------------------------------------------------------
# CppunitTest_sc_filters_test.mk: 1 source (sc/qa/unit/filters-test), adds
# "scfilt" to LIBRARIES (the filter library itself, real), DISABLE_CVE_
# TESTS inert (see wave header), 41-name use_components list of which 39
# resolve to real components as of 2026-09-29 (xmlfa and now xsltfilter both
# built -- see wave header) -- only sdbc2 still does not exist anywhere in
# this tree and remains omitted.
librestudio_add_cppunit_test(sc_filters_test
    SOURCES
        sc/qa/unit/filters-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sc
        scfilt scqahelper sfx sot svl svt svx svxcore test_fixtures tk tl
        ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        sb chart2 comphelper configmgr dba embobj emfio evtatt filterconfig
        frm fwk i18npool lng oox xstor package2 sax analysis date sc scfilt
        sfx sot svl svt svx svxcore tk ucb1 ucpfile1 ucptdoc1 utl unordf
        unoxml uui vcl xmlfa xsltfilter xo xsec_xmlsec
    CONFIGURATION)

# --- sc_functionlistobj --------------------------------------------------------
# CppunitTest_sc_functionlistobj.mk: 1 source (sc/qa/extras/
# scfunctionlistobj), smaller LIBRARIES (cppu cppuhelper sal subsequenttest
# test unotest utl tl), use_sdk_api, 34-name use_components (all real,
# same mapping family as sc_cache_test/sc_mark_test, plus i18nsearch).
librestudio_add_cppunit_test(sc_functionlistobj
    SOURCES
        sc/qa/extras/scfunctionlistobj
    LIBRARIES
        cppu cppuhelper sal subsequenttest test_fixtures unotest utl tl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        sb comphelper configmgr dba filterconfig storagefd frm fwk
        i18nsearch i18npool lng oox xstor package2 sax basprov scriptframe
        sc scd scfilt vbaobj sfx sot fsstorage tk ucb1 ucpfile1 ucptdoc1
        utl unordf unoxml uui vcl xo svt
    CONFIGURATION)

# --- sc_goal_seek_test -----------------------------------------------------
# CppunitTest_sc_GoalSeekTest.mk: 1 source (sc/qa/unit/GoalSeekTest),
# smaller LIBRARIES (basegfx comphelper cppu cppuhelper docmodel sal
# salhelper sax sc scqahelper sfx subsequenttest svl(no, not present --
# re-checked: no svl here) test tl unotest utl vcl), use_api(offapi
# udkapi) + use_sdk_api (redundant with each other, same normal headers
# either way), use_rdb(services), an empty use_components() call (real
# no-op, matches the 3 "ucalc_*" bonus tests above -- omitted here too).
librestudio_add_cppunit_test(sc_goal_seek_test
    SOURCES
        sc/qa/unit/GoalSeekTest
    LIBRARIES
        basegfx comphelper cppu cppuhelper docmodel sal salhelper sax sc
        scqahelper sfx subsequenttest test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_jumbosheets_test ---------------------------------------------------
# CppunitTest_sc_jumbosheets_test.mk: 1 source (sc/qa/unit/
# jumbosheets-test), smaller LIBRARIES (no editeng/for/forui/msfilter/
# oox/sot/tk/ucbhelper/vbahelper/xo), use_api(udkapi offapi only, no
# oovbaapi), use_rdb(services).
librestudio_add_cppunit_test(sc_jumbosheets_test
    SOURCES
        sc/qa/unit/jumbosheets-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper sal salhelper sax sc scqahelper
        sfx subsequenttest svxcore test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${CMAKE_SOURCE_DIR}/sc/source/filter/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_macros_test --------------------------------------------------------
# CppunitTest_sc_macros_test.mk: standard shape, 1 source (sc/qa/extras/
# macros-test), use_rdb(services).
librestudio_add_cppunit_test(sc_macros_test
    SOURCES
        sc/qa/extras/macros-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sb sc
        scqahelper sfx sot subsequenttest svl svt svx svxcore test_fixtures
        tk tl ucbhelper unotest utl vbahelper vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${CMAKE_SOURCE_DIR}/sc/qa/unit"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_mark_test ----------------------------------------------------------
# CppunitTest_sc_mark_test.mk: standard shape plus ICU, 1 source (sc/qa/
# unit/mark_test), 37-name use_components (all real, same family as
# sc_cache_test).
librestudio_add_cppunit_test(sc_mark_test
    SOURCES
        sc/qa/unit/mark_test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sb sc
        scqahelper sfx sot svl svt svx svxcore test_fixtures tk tl
        ucbhelper unotest utl vbahelper vcl xo
        Boost::boost ICU::uc ICU::i18n LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    COMPONENTS
        sb chart2 comphelper configmgr dba embobj evtatt filterconfig frm
        fwk i18npool oox xstor package2 sax analysis date basprov
        scriptframe sc scd scfilt vbaobj sfx sot fsstorage svl svx svxcore
        tk ucb1 ucpfile1 ucptdoc1 utl unordf unoxml vcl xo
    CONFIGURATION)

# --- sc_named_sheet_views_import_export_test --------------------------------
# CppunitTest_sc_NamedSheetViewsImportExportTest.mk: 1 source (sc/qa/unit/
# NamedSheetViewsImportExportTest), smaller LIBRARIES, use_sdk_api,
# use_rdb(services), an empty use_components() call (real no-op, omitted).
librestudio_add_cppunit_test(sc_named_sheet_views_import_export_test
    SOURCES
        sc/qa/unit/NamedSheetViewsImportExportTest
    LIBRARIES
        basegfx comphelper cppu cppuhelper sal salhelper sax sc scqahelper
        sfx subsequenttest test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_new_cond_format_api --------------------------------------------------
# CppunitTest_sc_new_cond_format_api.mk: standard shape minus "sc"/"ucbhelper"
# from LIBRARIES (re-checked: "sc" itself is genuinely absent from this
# one's own use_libraries call -- presumably reached transitively through
# another linked library), 1 source (sc/qa/extras/new_cond_format),
# use_sdk_api, use_rdb(services).
librestudio_add_cppunit_test(sc_new_cond_format_api
    SOURCES
        sc/qa/extras/new_cond_format
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sb
        sfx sot subsequenttest svl svt svx svxcore test_fixtures tk tl
        ucbhelper unotest utl vbahelper vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_oox_helpers_test -----------------------------------------------------
# CppunitTest_sc_oox_helpers_test.mk: 1 source (sc/qa/unit/
# RedundantParenthesesStripperTest), adds "scfilt", use_api(offapi udkapi)
# + use_sdk_api (redundant), use_rdb(services), empty use_components()
# (omitted).
librestudio_add_cppunit_test(sc_oox_helpers_test
    SOURCES
        sc/qa/unit/RedundantParenthesesStripperTest
    LIBRARIES
        basegfx comphelper cppu cppuhelper docmodel sal salhelper sax sc
        scfilt scqahelper sfx subsequenttest test_fixtures tl unotest utl
        vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/source/filter/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_opencl-1 / sc_opencl-2 -----------------------------------------------
# CppunitTest_sc_opencl-1.mk / _opencl-2.mk: standard shape (OPENCL's own
# optional clew skipped, inert everywhere else in this port), 1 source
# each (sc/qa/unit/opencl-test-1 / opencl-test-2), use_rdb(services). CMake
# target names with a literal hyphen are valid (add_test/add_custom_target
# both accept them).
librestudio_add_cppunit_test(sc_opencl-1
    SOURCES
        sc/qa/unit/opencl-test-1
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sc
        scqahelper sfx sot subsequenttest svl svt svx svxcore test_fixtures
        tk tl ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sc_opencl-2
    SOURCES
        sc/qa/unit/opencl-test-2
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sc
        scqahelper sfx sot subsequenttest svl svt svx svxcore test_fixtures
        tk tl ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/source/core/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_parallelism ----------------------------------------------------------
# CppunitTest_sc_parallelism.mk: 1 source (sc/qa/unit/parallelism),
# smaller LIBRARIES, use_custom_headers(officecfg/registry), use_sdk_api,
# use_rdb(services), empty use_components() (omitted).
librestudio_add_cppunit_test(sc_parallelism
    SOURCES
        sc/qa/unit/parallelism
    LIBRARIES
        basegfx comphelper cppu cppuhelper i18nlangtag sal sax sc
        scqahelper sfx subsequenttest svl svx svxcore test_fixtures tl
        unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)
# Found missing via the real remote build: parallelism.cxx -> sc/inc/
# formulagroup.hxx -> <opencl/platforminfo.hxx> -> <clew/clew.h>, same
# ENABLE_OPENCL-gated fix as sc_ucalc/sc/sd/sd_uimpress's own "clew opencl"
# link (clew's own target_include_directories is PUBLIC).
if (ENABLE_OPENCL)
    target_link_libraries(sc_parallelism PRIVATE clew opencl)
endif ()

# --- sc_pdf_export -----------------------------------------------------------
# CppunitTest_sc_pdf_export.mk: 1 source (sc/qa/extras/scpdfexport),
# smaller LIBRARIES, `ifeq ($(TLS),NSS)` -> PkgConfig::NSS (this port's
# LIBRESTUDIO_TLS_BACKEND defaults to "NSS" -- see wave header), use_api
# (udkapi offapi only), use_rdb(services).
if (LIBRESTUDIO_TLS_BACKEND STREQUAL "NSS")
    if (NOT TARGET PkgConfig::NSS)
        pkg_check_modules(NSS REQUIRED IMPORTED_TARGET nss)
    endif ()
    set(_sc_pdf_export_tls PkgConfig::NSS)
else ()
    set(_sc_pdf_export_tls "")
endif ()
librestudio_add_cppunit_test(sc_pdf_export
    SOURCES
        sc/qa/extras/scpdfexport
    LIBRARIES
        comphelper cppu cppuhelper editeng sal sax sc sfx subsequenttest
        svl test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2 ${_sc_pdf_export_tls}
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)
unset(_sc_pdf_export_tls)

# --- sc_perfobj ----------------------------------------------------------------
# CppunitTest_sc_perfobj.mk: 1 source (sc/qa/perf/scperfobj), standard
# shape minus scqahelper, use_api(oovbaapi) + use_sdk_api (udkapi+offapi),
# use_rdb(services).
librestudio_add_cppunit_test(sc_perfobj
    SOURCES
        sc/qa/perf/scperfobj
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sb sc
        sfx sot subsequenttest svl svt svx svxcore test_fixtures tk tl
        ucbhelper unotest utl vbahelper vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_pivottable_filters_test / sc_pivottable_formats_import_export_test --
# Both standard shape, use_custom_headers(officecfg/registry), use_rdb(
# services). filters_test has 2 sources.
librestudio_add_cppunit_test(sc_pivottable_filters_test
    SOURCES
        sc/qa/unit/pivottable_filters_test
        sc/qa/unit/PivotTable_FieldsAndItemsExport
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sc
        scqahelper sfx sot subsequenttest svl svt svx svxcore test_fixtures
        tk tl ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sc_pivottable_formats_import_export_test
    SOURCES
        sc/qa/unit/PivotTableFormatsImportExport
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sc
        scqahelper sfx sot subsequenttest svl svt svx svxcore test_fixtures
        tk tl ucbhelper unotest utl vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        officecfg_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_screenshots ----------------------------------------------------------
# CppunitTest_sc_screenshots.mk: 1 source (sc/qa/unit/screenshots/
# screenshots), adds "scui", use_vcl_non_headless_with_windows (NOT plain
# use_vcl) -> NONHEADLESS, use_rdb(services), use_uiconfigs(cui modules/
# scalc) -- 2026-09-28: scalc_uiconfig now built, "scalc" included for real.
librestudio_add_cppunit_test(sc_screenshots
    SOURCES
        sc/qa/unit/screenshots/screenshots
    LIBRARIES
        comphelper cppu cppuhelper editeng sal sfx svl svt svx svxcore sc
        scui test_fixtures unotest vcl tl utl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    NONHEADLESS
    UICONFIG
        cui scalc
    CONFIGURATION)

# --- sc_scriptforge_test -----------------------------------------------------
# CppunitTest_sc_scriptforge_test.mk: 1 source (sc/qa/extras/scriptforge),
# smaller LIBRARIES, use_rdb(services).
librestudio_add_cppunit_test(sc_scriptforge_test
    SOURCES
        sc/qa/extras/scriptforge
    LIBRARIES
        sal sc scqahelper cppu cppuhelper subsequenttest svl svt svx
        svxcore test_fixtures tk tl ucbhelper unotest utl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${CMAKE_SOURCE_DIR}/sc/qa/unit"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_shapetest --------------------------------------------------------------
# CppunitTest_sc_shapetest.mk: 1 source (sc/qa/unit/scshapetest), smaller
# LIBRARIES, use_sdk_api, use_rdb(services).
librestudio_add_cppunit_test(sc_shapetest
    SOURCES
        sc/qa/unit/scshapetest
    LIBRARIES
        cppu cppuhelper sal sc scqahelper sfx subsequenttest svl svx
        svxcore test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_sheetview_test -------------------------------------------------------
# CppunitTest_sc_sheetview_test.mk: 1 source (sc/qa/unit/tiledrendering/
# SheetViewTest), adds scfilt/scui, use_sdk_api + use_api(oovbaapi),
# use_rdb(services), use_uiconfigs(modules/scalc sfx svt) -- 2026-09-28:
# scalc_uiconfig now built, "scalc" included for real.
librestudio_add_cppunit_test(sc_sheetview_test
    SOURCES
        sc/qa/unit/tiledrendering/SheetViewTest
    LIBRARIES
        comphelper cppu cppuhelper editeng sal sfx sot svl svt svxcore sc
        scfilt scui scqahelper subsequenttest test_fixtures unotest
        vbahelper vcl tl utl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${CMAKE_SOURCE_DIR}/sc/qa/unit/helper"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG
        scalc sfx svt
    CONFIGURATION)

# --- sc_solver -----------------------------------------------------------------
# CppunitTest_sc_solver.mk (NOT sc_solverobj -- a genuinely separate, fully
# hand-written file that happens to share a similar name): tiny, 1 source
# (sc/qa/unit/solver), the same live ENABLE_COINMP generator expression as
# sc_unoapi_common above, use_libraries(cppu sal test unotest) only,
# use_sdk_api, use_rdb(services).
librestudio_add_cppunit_test(sc_solver
    SOURCES
        sc/qa/unit/solver
    LIBRARIES
        cppu sal test_fixtures unotest LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEFINITIONS
        $<$<BOOL:${ENABLE_COINMP}>:ENABLE_COINMP>
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_sparkline_test -------------------------------------------------------
# CppunitTest_sc_sparkline_test.mk: 1 source (sc/qa/unit/
# SparklineImportExportTest), smaller LIBRARIES, use_sdk_api,
# use_rdb(services), empty use_components() (omitted).
librestudio_add_cppunit_test(sc_sparkline_test
    SOURCES
        sc/qa/unit/SparklineImportExportTest
    LIBRARIES
        basegfx comphelper cppu cppuhelper docmodel sal salhelper sax sc
        scqahelper sfx subsequenttest test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_swarmsolvertest ------------------------------------------------------
# CppunitTest_sc_swarmsolvertest.mk: 1 source (sc/qa/unit/
# SwarmSolverTest), standard-ish shape (no mdds_headers/boost_headers
# externals beyond libxml2), use_sdk_api, use_rdb(services). gb_UWINAPI is
# Windows-only, empty here.
librestudio_add_cppunit_test(sc_swarmsolvertest
    SOURCES
        sc/qa/unit/SwarmSolverTest
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer editeng for forui
        i18nlangtag msfilter oox sal salhelper sax sb sc scqahelper sfx sot
        subsequenttest svl svt svx svxcore test_fixtures tk tl ucbhelper
        unotest utl vbahelper vcl xo
        LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${CMAKE_SOURCE_DIR}/sc/source/solver"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_theme_import_export_test ---------------------------------------------
# CppunitTest_sc_theme_import_export_test.mk: 1 source (sc/qa/unit/
# ThemeImportExportTest), smaller LIBRARIES, use_sdk_api, use_rdb(services),
# empty use_components() (omitted).
librestudio_add_cppunit_test(sc_theme_import_export_test
    SOURCES
        sc/qa/unit/ThemeImportExportTest
    LIBRARIES
        basegfx docmodel comphelper cppu cppuhelper editeng sal salhelper
        sax sc scqahelper sfx svl subsequenttest test_fixtures tl unotest
        utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_tiledrendering / sc_tiledrendering2 ----------------------------------
# Both (CppunitTest_sc_tiledrendering.mk / _tiledrendering2.mk): adds
# scfilt/scui, use_sdk_api + use_api(oovbaapi), use_rdb(services),
# use_uiconfigs(modules/scalc sfx svt) -- 2026-09-28: scalc_uiconfig now
# built, "scalc" included for real in both.
# tiledrendering's own -fsanitize=vptr-conditional salhelper is inert (no
# sanitizer detection in this port).
librestudio_add_cppunit_test(sc_tiledrendering
    SOURCES
        sc/qa/unit/tiledrendering/tiledrendering
    LIBRARIES
        comphelper cppu cppuhelper editeng sal sfx sot svl svt svxcore sc
        scfilt scui scqahelper subsequenttest test_fixtures unotest
        vbahelper vcl tl utl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${CMAKE_SOURCE_DIR}/sc/qa/unit/helper"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG
        scalc sfx svt
    CONFIGURATION)

librestudio_add_cppunit_test(sc_tiledrendering2
    SOURCES
        sc/qa/unit/tiledrendering/tiledrendering2
    LIBRARIES
        comphelper cppu cppuhelper editeng sal sfx sot svl svt svxcore sc
        scfilt scui scqahelper subsequenttest test_fixtures unotest
        vbahelper vcl tl utl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${CMAKE_SOURCE_DIR}/sc/qa/unit/helper"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    UICONFIG
        scalc sfx svt
    CONFIGURATION)

# --- sc_uicalc / sc_uicalc2 --------------------------------------------------
# Both (CppunitTest_sc_uicalc.mk / _uicalc2.mk): smaller LIBRARIES (each
# upstream file lists "svl" twice -- a harmless upstream duplicate, not
# repeated here), use_sdk_api, use_rdb(services). uicalc2 adds "tk" and an
# inert -fsanitize=vptr-conditional salhelper.
librestudio_add_cppunit_test(sc_uicalc
    SOURCES
        sc/qa/unit/uicalc/uicalc
    LIBRARIES
        comphelper cppu cppuhelper editeng i18nlangtag sal sc scqahelper
        sfx subsequenttest svl svx svxcore test_fixtures tl unotest utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

librestudio_add_cppunit_test(sc_uicalc2
    SOURCES
        sc/qa/unit/uicalc/uicalc2
    LIBRARIES
        comphelper cppu cppuhelper editeng i18nlangtag sal sc scqahelper
        sfx sot subsequenttest svl svx svxcore test_fixtures tk tl unotest
        utl vcl
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)

# --- sc_vba_macro_test -------------------------------------------------------
# CppunitTest_sc_vba_macro_test.mk: standard shape, 1 source (sc/qa/
# extras/vba-macro-test), use_rdb(services).
librestudio_add_cppunit_test(sc_vba_macro_test
    SOURCES
        sc/qa/extras/vba-macro-test
    LIBRARIES
        basegfx comphelper cppu cppuhelper drawinglayer drawinglayercore
        editeng for forui i18nlangtag msfilter oox sal salhelper sax sb sc
        scqahelper sfx sot subsequenttest svl svt svx svxcore test_fixtures
        tk tl ucbhelper unotest utl vbahelper vcl xo
        Boost::boost LibXml2::LibXml2
    INCLUDE
        "${CMAKE_SOURCE_DIR}/sc/source/ui/inc"
        "${CMAKE_SOURCE_DIR}/sc/inc"
        "${MDDS_INCLUDE_DIR}"
        "${CMAKE_SOURCE_DIR}/sc/qa/unit"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
        "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
        program_services_rdb
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
        vclbootstrapprotector vclbootstrapprotector
    CONFIGURATION)
