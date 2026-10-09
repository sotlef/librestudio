# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# sw (LibreOffice Writer) -- Module_sw.mk lists these build targets:
#   CustomTarget_generated       -- sw/generated/tokens.cxx (gperf token
#                                    tables for AutoText). Feeds Library_sw.
#   CustomTarget_source          -- writerfilter's OOXML factory/parser
#                                    generator (model.xml -> C++). Feeds
#                                    Library_sw_writerfilter, see below.
#   Library_sw       (sw)        -- the Writer core + UI + UNO API
#                                    implementation, 720 exception objects
#                                    (691 unconditional + 26 accessibility,
#                                    ENABLE_WASM_STRIP_ACCESSIBILITY is unset
#                                    in this build so its `ifneq` guard is
#                                    true + 2 gated behind ENABLE_DBCONNECTIVITY,
#                                    which defaults ON).
#   Library_swd       (swd)      -- the tiny format-detection service. No
#                                    dependency on Library_sw. Ported already,
#                                    untouched below.
#   Library_swui      (swui)     -- dialog implementations
#                                    (SwAbstractDialogFactory), 96 exception
#                                    objects (83 unconditional + 13 gated
#                                    behind ENABLE_DBCONNECTIVITY, which
#                                    defaults ON). Depends on sw.
#   Library_sw_writerfilter (writerfilter) -- RTF/OOXML import, 2026-09-28:
#                                    now built, see the writerfilter note below.
#   Library_msword    (msword)   -- legacy binary .doc/.docx/.rtf filter, 39
#                                    exception objects. Depends on sw.
#   Library_vbaswobj  (vbaswobj) -- Writer VBA object model, 82 exception
#                                    objects. The whole target is gated behind
#                                    ENABLE_SCRIPTING (default ON), matching
#                                    Module_sw.mk's own
#                                    `ifneq ($(filter SCRIPTING,$(BUILD_TYPE)),)`
#                                    gate around Library_vbaswobj. Depends on
#                                    sw and vbahelper.
#
# sw_writerfilter (Library_sw_writerfilter + CustomTarget_source, the OOXML
# import filter) -- ported 2026-09-28, the last remaining gap of the sc/sw/sd
# CppunitTest wave (393/399 at the time this was picked up; see
# PORTING_STATUS.md). CustomTarget_source.mk's generator (below, right after
# the "sw" library) reproduces gbuild's own multi-stage xmllint+Python
# pipeline over sw/source/writerfilter/ooxml/model.xml exactly:
# oox_generated's own namespaces.txt output feeds a preprocessing step, which
# in turn feeds 4 single-output generators (OOXMLFactory_generated.{cxx,hxx},
# qnametostr.cxx, resourceids.hxx) plus 28 per-namespace OOXMLFactory_<Ns>.
# {cxx,hxx} pairs (writerfilter_OOXMLNAMESPACES in the real .mk lists exactly
# 28 names, not "~30" as an earlier draft of this comment estimated).
# `OOXMLFactory_values.hxx`, listed as a CustomTarget_source.mk goal
# dependency with no rule of its own, is NOT reproduced here: tracing
# factory_ns.py's own argv-parsing (`ns = filePath.split('OOXMLFactory_')
# [1].split('.hxx')[0]`) confirms real gbuild only produces it by accident,
# because its filename also matches the `OOXMLFactory%.hxx` pattern rule
# meant for the 28 real namespaces -- factory_ns.py then runs against the
# nonexistent "values" namespace and emits a harmless, unused, action-less
# header (confirmed via `grep -rn "OOXMLFactory_values"
# sw/source/writerfilter/`: zero #includes anywhere, and it is not listed in
# Library_sw_writerfilter.mk's own gb_Library_add_generated_exception_objects
# either). Skipping it is an honest simplification of dead output, not a
# fabrication.
#
# This does NOT block msword: grepping all 39 of Library_msword.mk's source
# files for "writerfilter" finds only comments and
# sw/source/filter/docx/swdocxreader.cxx's two
# xMultiServiceFactory->createInstance(u"com.sun.star.comp.Writer.WriterFilter")
# calls -- a UNO service-manager lookup by name, resolved at runtime through
# the component registry, not a compile-time/link-time dependency. No real
# header or symbol dependency on writerfilter exists in msword's sources.
#
# md4c: sw/source/filter/md/*.cxx (Markdown import/export) need the external
# md4c library, resolved right below via librestudio_pkgconfig() (cmake/
# LibreStudioTarballs.cmake) into LIBRESTUDIO_MD4C_TARGET -- sw is one of
# the two real consumers (alongside sc, which also needs md4c-html).
#
# ENABLE_YRS / ENABLE_YRS_WEAK: sw/source/core/doc/DocumentStateManager.cxx
# reaches the "yrs" (y-crdt Rust collaboration) external only under
# #if ENABLE_YRS / #if ENABLE_YRS_WEAK, which cmake/config/config_collab.h.in
# stubs to 0 in this build (same precedent as editeng) -- "yrs" from
# Library_sw.mk's use_externals is therefore not linked.
#
# textconversiondlgs (svx/source/unodialogs/textconversiondlgs/*) is linked
# directly by sw (annotsh.cxx/drwtxtsh.cxx/viewling.cxx instantiate
# textconversiondlgs::ChineseTranslation_UnoDialog via rtl::Reference<...>,
# never through the UNO service manager) -- svx/CMakeLists.txt already has a
# `textconversiondlgs` target for this.
#
# -DSWUI_DLL_NAME="..." (Library_sw.mk's add_defs) is a build-time computed
# runtime filename of the swui shared library, used by sw to look up swui's
# dialog factory at runtime. Reproduced here as the fixed string
# "libswuilo.so", matching librestudio_set_library_output_name(swui swui OOO)
# below.
#
# pkg-config modules used in this file: md4c (msword, via MD4C).

librestudio_pkgconfig(MODULES md4c)

find_program(GPERF_EXECUTABLE gperf REQUIRED)
find_program(PYTHON3_EXECUTABLE NAMES python3 python REQUIRED)
find_program(XMLLINT_EXECUTABLE xmllint REQUIRED)

# ---------------------------------------------------------------------------
# sw_generated -- CustomTarget_generated.mk (sw/generated/tokens.cxx, two
# gperf token tables -- TextBlockTokens, BlockListTokens -- for AutoText).
# sw/source/core/swg/SwXMLBlockImport.cxx #includes tokens.cxx directly (bare
# #include, not linked as an object -- same idiom as editeng's tokens.cxx).
# ---------------------------------------------------------------------------

# CACHE INTERNAL (not a plain set()): sw/CppunitTest_deferred.cmake's own
# sw_uwriter recompiles sw/source/core/swg/SwXMLBlockImport.cxx (part of
# SW_SOURCES) and needs this same directory on its own INCLUDE list, but that
# file include()s back into the ROOT directory's scope, not this one -- a
# plain set() here would not be visible there (same cross-scope visibility
# gap already found and fixed for librestudio_add_sdi_target()'s own
# ${target}_INCLUDE_DIR, cmake/LibreStudioSdiTarget.cmake). Confirmed via the
# real remote build: sw_uwriter's own -I list had no reference to this
# directory at all, "fatal error: tokens.cxx: No such file or directory".
set(SW_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated" CACHE INTERNAL "")
set(SW_TOKENS_SRC "${CMAKE_SOURCE_DIR}/sw/source/core/swg")

foreach(_tbl TextBlockTokens BlockListTokens)
    add_custom_command(
        OUTPUT "${SW_GENERATED_DIR}/${_tbl}.gperf"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SW_GENERATED_DIR}"
        COMMAND "${PYTHON3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/solenv/bin/gentoken.py"
                "${SW_TOKENS_SRC}/${_tbl}.txt" "${SW_GENERATED_DIR}/${_tbl}.gperf"
        DEPENDS "${SW_TOKENS_SRC}/${_tbl}.txt" "${CMAKE_SOURCE_DIR}/solenv/bin/gentoken.py"
        COMMENT "gentoken.py: sw/generated/${_tbl}.gperf" VERBATIM)
endforeach()

add_custom_command(
    OUTPUT "${SW_GENERATED_DIR}/TextBlockTokens.processed"
    COMMAND sh -c "'${GPERF_EXECUTABLE}' --compare-strncmp --readonly-tables --class-name=TextBlockTokens --pic --string-pool-name=textBlockPool --word-array-name=textBlockList --enum '${SW_GENERATED_DIR}/TextBlockTokens.gperf' > '${SW_GENERATED_DIR}/TextBlockTokens.processed'"
    DEPENDS "${SW_GENERATED_DIR}/TextBlockTokens.gperf" VERBATIM)
add_custom_command(
    OUTPUT "${SW_GENERATED_DIR}/BlockListTokens.processed"
    COMMAND sh -c "'${GPERF_EXECUTABLE}' --compare-strncmp --readonly-tables -T '${SW_GENERATED_DIR}/BlockListTokens.gperf' --pic --string-pool-name=blockListPool --class-name=BlockListTokens --word-array-name=blockListList > '${SW_GENERATED_DIR}/BlockListTokens.processed'"
    DEPENDS "${SW_GENERATED_DIR}/BlockListTokens.gperf" VERBATIM)

add_custom_command(
    OUTPUT "${SW_GENERATED_DIR}/tokens.cxx"
    COMMAND sh -c "sed -e '/^#line/d' -e 's/(char\\*)0/(char\\*)0, XML_TOKEN_INVALID/g' '${SW_GENERATED_DIR}/TextBlockTokens.processed' '${SW_GENERATED_DIR}/BlockListTokens.processed' > '${SW_GENERATED_DIR}/tokens.cxx'"
    DEPENDS "${SW_GENERATED_DIR}/TextBlockTokens.processed" "${SW_GENERATED_DIR}/BlockListTokens.processed" VERBATIM)
add_custom_target(sw_generated DEPENDS "${SW_GENERATED_DIR}/tokens.cxx")

# ---------------------------------------------------------------------------
# sw_writerfilter_generated -- CustomTarget_source.mk. A 6-stage pipeline,
# reproducing the real gbuild recipes almost line for line (see the
# writerfilter note in this file's own header comment for the design
# rationale). Every add_custom_command here repeats
# `${CMAKE_COMMAND} -E make_directory` for its output directory, matching
# how oox/CMakeLists.txt's own oox_generated block stays parallel-build-safe
# without relying on ordering alone.
#
# model_preprocessed.xml (stage 2) depends on oox's own already-ported
# oox_generated target/output (OOX_GENERATED_DIR/misc/namespaces.txt) -- a
# real cross-module generated-file dependency, resolved simply because
# add_subdirectory(oox) already runs before add_subdirectory(sw) in the root
# CMakeLists.txt.
#
# The `> "${output}"` redirection below is native CMake add_custom_command
# syntax (no shell needed), the same already-working idiom
# oox/CMakeLists.txt's own oox_generated block uses for its `-E cat ...`
# steps. Only the xmllint validation stage needs an actual shell (`2>&1` plus
# an `||` fallback are not native add_custom_command redirection features),
# matching oox/CMakeLists.txt's own `sh -c "... | sed ... > ..."` precedent
# for its gperf/sed post-processing stage.
# ---------------------------------------------------------------------------

set(SW_WRITERFILTER_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/writerfilter_generated")
set(SW_WRITERFILTER_OOXML_SRC "${CMAKE_SOURCE_DIR}/sw/source/writerfilter/ooxml")
set(SW_WRITERFILTER_OOXML_GEN "${SW_WRITERFILTER_GENERATED_DIR}/ooxml")

# writerfilter_OOXMLNAMESPACES, transcribed literally from CustomTarget_source.mk.
set(SW_WRITERFILTER_OOXMLNAMESPACES
    dml-baseStylesheet dml-baseTypes dml-chartDrawing dml-documentProperties
    dml-graphicalObject dml-shape3DCamera dml-shape3DLighting dml-shape3DScene
    dml-shape3DStyles dml-shapeEffects dml-shapeGeometry dml-shapeLineProperties
    dml-shapeProperties dml-styleDefaults dml-stylesheet dml-textCharacter
    dml-wordprocessingDrawing shared-math shared-relationshipReference
    sml-customXmlMappings vml-main vml-officeDrawing vml-wordprocessingDrawing
    wp14 w14 w15 a14 wml)

# Stage 1: model.validated -- xmllint --relaxng, pure validation stamp.
set(SW_WRITERFILTER_MODEL_VALIDATED "${SW_WRITERFILTER_OOXML_GEN}/model.validated")
add_custom_command(
    OUTPUT "${SW_WRITERFILTER_MODEL_VALIDATED}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SW_WRITERFILTER_OOXML_GEN}"
    COMMAND sh -c "'${XMLLINT_EXECUTABLE}' --noout --relaxng '${CMAKE_SOURCE_DIR}/sw/documentation/writerfilter/ooxml/model.rng' '${SW_WRITERFILTER_OOXML_SRC}/model.xml' > '${SW_WRITERFILTER_MODEL_VALIDATED}' 2>&1 || (cat '${SW_WRITERFILTER_MODEL_VALIDATED}'; false)"
    DEPENDS "${CMAKE_SOURCE_DIR}/sw/documentation/writerfilter/ooxml/model.rng"
            "${SW_WRITERFILTER_OOXML_SRC}/model.xml"
    COMMENT "xmllint: validating writerfilter model.xml" VERBATIM)

# Stage 2: model_preprocessed.xml -- resolves namespace aliases against oox's
# own generated namespaces.txt.
set(SW_WRITERFILTER_MODEL_PROCESSED "${SW_WRITERFILTER_OOXML_GEN}/model_preprocessed.xml")
add_custom_command(
    OUTPUT "${SW_WRITERFILTER_MODEL_PROCESSED}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SW_WRITERFILTER_OOXML_GEN}"
    COMMAND "${PYTHON3_EXECUTABLE}" "${SW_WRITERFILTER_OOXML_SRC}/modelpreprocess.py"
            "${OOX_GENERATED_DIR}/misc/namespaces.txt" "${SW_WRITERFILTER_OOXML_SRC}/model.xml"
            > "${SW_WRITERFILTER_MODEL_PROCESSED}"
    DEPENDS "${SW_WRITERFILTER_OOXML_SRC}/modelpreprocess.py"
            "${OOX_GENERATED_DIR}/misc/namespaces.txt"
            "${SW_WRITERFILTER_MODEL_VALIDATED}"
            oox_generated
    COMMENT "modelpreprocess.py: writerfilter model_preprocessed.xml" VERBATIM)

# Stage 3: the 4 single-output generators.
set(SW_WRITERFILTER_FACTORY_GENERATED_CXX "${SW_WRITERFILTER_OOXML_GEN}/OOXMLFactory_generated.cxx")
add_custom_command(
    OUTPUT "${SW_WRITERFILTER_FACTORY_GENERATED_CXX}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SW_WRITERFILTER_OOXML_GEN}"
    COMMAND "${PYTHON3_EXECUTABLE}" "${SW_WRITERFILTER_OOXML_SRC}/factoryimpl.py"
            "${OOX_GENERATED_DIR}/misc/namespaces.txt" "${SW_WRITERFILTER_MODEL_PROCESSED}"
            > "${SW_WRITERFILTER_FACTORY_GENERATED_CXX}"
    DEPENDS "${SW_WRITERFILTER_OOXML_SRC}/factoryimpl.py"
            "${OOX_GENERATED_DIR}/misc/namespaces.txt"
            "${SW_WRITERFILTER_MODEL_PROCESSED}"
    COMMENT "factoryimpl.py: OOXMLFactory_generated.cxx" VERBATIM)

set(SW_WRITERFILTER_FACTORY_GENERATED_HXX "${SW_WRITERFILTER_OOXML_GEN}/OOXMLFactory_generated.hxx")
add_custom_command(
    OUTPUT "${SW_WRITERFILTER_FACTORY_GENERATED_HXX}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SW_WRITERFILTER_OOXML_GEN}"
    COMMAND "${PYTHON3_EXECUTABLE}" "${SW_WRITERFILTER_OOXML_SRC}/factoryinc.py"
            "${SW_WRITERFILTER_MODEL_PROCESSED}" > "${SW_WRITERFILTER_FACTORY_GENERATED_HXX}"
    DEPENDS "${SW_WRITERFILTER_OOXML_SRC}/factoryinc.py" "${SW_WRITERFILTER_MODEL_PROCESSED}"
    COMMENT "factoryinc.py: OOXMLFactory_generated.hxx" VERBATIM)

set(SW_WRITERFILTER_QNAMETOSTR_CXX "${SW_WRITERFILTER_OOXML_GEN}/qnametostr.cxx")
add_custom_command(
    OUTPUT "${SW_WRITERFILTER_QNAMETOSTR_CXX}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SW_WRITERFILTER_OOXML_GEN}"
    COMMAND "${PYTHON3_EXECUTABLE}" "${SW_WRITERFILTER_OOXML_SRC}/qnametostr.py"
            "${SW_WRITERFILTER_MODEL_PROCESSED}" > "${SW_WRITERFILTER_QNAMETOSTR_CXX}"
    DEPENDS "${SW_WRITERFILTER_OOXML_SRC}/qnametostr.py" "${SW_WRITERFILTER_MODEL_PROCESSED}"
    COMMENT "qnametostr.py: qnametostr.cxx" VERBATIM)

set(SW_WRITERFILTER_RESOURCEIDS_HXX "${SW_WRITERFILTER_OOXML_GEN}/resourceids.hxx")
add_custom_command(
    OUTPUT "${SW_WRITERFILTER_RESOURCEIDS_HXX}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SW_WRITERFILTER_OOXML_GEN}"
    COMMAND "${PYTHON3_EXECUTABLE}" "${SW_WRITERFILTER_OOXML_SRC}/resourceids.py"
            "${SW_WRITERFILTER_MODEL_PROCESSED}" > "${SW_WRITERFILTER_RESOURCEIDS_HXX}"
    DEPENDS "${SW_WRITERFILTER_OOXML_SRC}/resourceids.py" "${SW_WRITERFILTER_MODEL_PROCESSED}"
    COMMENT "resourceids.py: resourceids.hxx" VERBATIM)

# Stage 4: 28 x 2 per-namespace outputs. gbuild expressed this as one
# %-pattern rule over patsubst'd names; CMake has no wildcard pattern rule
# for add_custom_command, so it is expanded into 56 explicit commands via
# foreach -- the same idiom already used above for sw_generated's own
# `foreach(_tbl TextBlockTokens BlockListTokens)`. Each script parses its own
# namespace name back out of the output path it is handed as its 2nd
# argument (factoryimpl_ns.py/factory_ns.py's own
# `filePath.split('OOXMLFactory_')[1].split('.cxx'/'.hxx')[0]`), exactly
# matching real gbuild's `$@`-as-argument recipe.
set(SW_WRITERFILTER_NS_SOURCES)
set(SW_WRITERFILTER_NS_OUTPUTS)
foreach(_ns IN LISTS SW_WRITERFILTER_OOXMLNAMESPACES)
    set(_ns_cxx "${SW_WRITERFILTER_OOXML_GEN}/OOXMLFactory_${_ns}.cxx")
    set(_ns_hxx "${SW_WRITERFILTER_OOXML_GEN}/OOXMLFactory_${_ns}.hxx")
    add_custom_command(
        OUTPUT "${_ns_cxx}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SW_WRITERFILTER_OOXML_GEN}"
        COMMAND "${PYTHON3_EXECUTABLE}" "${SW_WRITERFILTER_OOXML_SRC}/factoryimpl_ns.py"
                "${SW_WRITERFILTER_MODEL_PROCESSED}" "${_ns_cxx}" > "${_ns_cxx}"
        DEPENDS "${SW_WRITERFILTER_OOXML_SRC}/factoryimpl_ns.py" "${SW_WRITERFILTER_MODEL_PROCESSED}"
        COMMENT "factoryimpl_ns.py: OOXMLFactory_${_ns}.cxx" VERBATIM)
    add_custom_command(
        OUTPUT "${_ns_hxx}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SW_WRITERFILTER_OOXML_GEN}"
        COMMAND "${PYTHON3_EXECUTABLE}" "${SW_WRITERFILTER_OOXML_SRC}/factory_ns.py"
                "${SW_WRITERFILTER_MODEL_PROCESSED}" "${_ns_hxx}" > "${_ns_hxx}"
        DEPENDS "${SW_WRITERFILTER_OOXML_SRC}/factory_ns.py" "${SW_WRITERFILTER_MODEL_PROCESSED}"
        COMMENT "factory_ns.py: OOXMLFactory_${_ns}.hxx" VERBATIM)
    list(APPEND SW_WRITERFILTER_NS_SOURCES "${_ns_cxx}")
    list(APPEND SW_WRITERFILTER_NS_OUTPUTS "${_ns_cxx}" "${_ns_hxx}")
endforeach()

# OOXMLFactory_values.hxx is deliberately NOT generated here -- see this
# file's own header comment for why (a real gbuild %-pattern-rule accident,
# never #included, not part of the library's own generated exception
# objects).
add_custom_target(sw_writerfilter_generated DEPENDS
    "${SW_WRITERFILTER_MODEL_VALIDATED}" "${SW_WRITERFILTER_MODEL_PROCESSED}"
    "${SW_WRITERFILTER_FACTORY_GENERATED_CXX}" "${SW_WRITERFILTER_FACTORY_GENERATED_HXX}"
    "${SW_WRITERFILTER_QNAMETOSTR_CXX}" "${SW_WRITERFILTER_RESOURCEIDS_HXX}"
    ${SW_WRITERFILTER_NS_OUTPUTS})

# ---------------------------------------------------------------------------
# sw -- Library_sw.mk
# ---------------------------------------------------------------------------

librestudio_add_sdi_target(sw_sdi
    SDI        "${CMAKE_SOURCE_DIR}/sw/sdi/swslots.sdi"
    EXPORTS    "${CMAKE_SOURCE_DIR}/sw/sdi/swriter.sdi"
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/SdiTarget/sw/sdi"
    INCLUDES   "${CMAKE_SOURCE_DIR}/sw/inc" "${CMAKE_SOURCE_DIR}/svx/sdi" "${CMAKE_SOURCE_DIR}/sfx2/sdi")

set(SW_SOURCES
    # ifneq ($(ENABLE_WASM_STRIP_ACCESSIBILITY),TRUE) -- applies, the variable
    # is unset (0) in this build.
    sw/source/core/access/acccell
    sw/source/core/access/acccontext
    sw/source/core/access/accdoc
    sw/source/core/access/accembedded
    sw/source/core/access/accfootnote
    sw/source/core/access/accframe
    sw/source/core/access/accframebase
    sw/source/core/access/accfrmobj
    sw/source/core/access/accfrmobjmap
    sw/source/core/access/accfrmobjslist
    sw/source/core/access/accgraphic
    sw/source/core/access/accheaderfooter
    sw/source/core/access/acchyperlink
    sw/source/core/access/acchypertextdata
    sw/source/core/access/accmap
    sw/source/core/access/accnotextframe
    sw/source/core/access/accnotexthyperlink
    sw/source/core/access/accpage
    sw/source/core/access/accpara
    sw/source/core/access/accportions
    sw/source/core/access/accpreview
    sw/source/core/access/accselectionhelper
    sw/source/core/access/acctable
    sw/source/core/access/acctextframe
    sw/source/core/access/parachangetrackinginfo
    sw/source/core/access/textmarkuphelper
    sw/source/uibase/docvw/SidebarWinAcc

    # main source list (unconditional)
    sw/source/core/SwNumberTree/SwNodeNum
    sw/source/core/SwNumberTree/SwNumberTree
    sw/source/core/access/AccessibilityCheck
    sw/source/core/access/AccessibilityIssue
    sw/source/core/attr/BorderCacheOwner
    sw/source/core/attr/calbck
    sw/source/core/attr/cellatr
    sw/source/core/attr/fmtfollowtextflow
    sw/source/core/attr/fmtwrapinfluenceonobjpos
    sw/source/core/attr/format
    sw/source/core/attr/formatflysplit
    sw/source/core/attr/formatwraptextatflystart
    sw/source/core/attr/hints
    sw/source/core/attr/swatrset
    sw/source/core/bastyp/SwSmartTagMgr
    sw/source/core/bastyp/bparr
    sw/source/core/bastyp/breakit
    sw/source/core/bastyp/calc
    sw/source/core/bastyp/checkit
    sw/source/core/bastyp/index
    sw/source/core/bastyp/init
    sw/source/core/bastyp/proofreadingiterator
    sw/source/core/bastyp/swcache
    sw/source/core/bastyp/swrect
    sw/source/core/bastyp/swregion
    sw/source/core/bastyp/swtypes
    sw/source/core/bastyp/tabcol
    sw/source/core/crsr/annotationmark
    sw/source/core/crsr/BlockCursor
    sw/source/core/crsr/bookmark
    sw/source/core/crsr/callnk
    sw/source/core/crsr/contentcontrolbutton
    sw/source/core/crsr/crbm
    sw/source/core/crsr/crossrefbookmark
    sw/source/core/crsr/crsrsh
    sw/source/core/crsr/crstrvl
    sw/source/core/crsr/crstrvl1
    sw/source/core/crsr/DateFormFieldButton
    sw/source/core/crsr/DropDownFormFieldButton
    sw/source/core/crsr/datecontentcontrolbutton
    sw/source/core/crsr/dropdowncontentcontrolbutton
    sw/source/core/crsr/findattr
    sw/source/core/crsr/findcoll
    sw/source/core/crsr/findfmt
    sw/source/core/crsr/findtxt
    sw/source/core/crsr/FormFieldButton
    sw/source/core/crsr/pam
    sw/source/core/crsr/paminit
    sw/source/core/crsr/swcrsr
    sw/source/core/crsr/trvlcol
    sw/source/core/crsr/trvlfnfl
    sw/source/core/crsr/trvlreg
    sw/source/core/crsr/trvltbl
    sw/source/core/crsr/viscrs
    sw/source/core/crsr/overlayrangesoutline
    sw/source/core/doc/SwDocIdle
    sw/source/core/doc/SwStyleNameMapper
    sw/source/core/doc/acmplwrd
    sw/source/core/doc/CntntIdxStore
    sw/source/core/doc/dbgoutsw
    sw/source/core/doc/doc
    sw/source/core/doc/docbasic
    sw/source/core/doc/docbm
    sw/source/core/doc/docchart
    sw/source/core/doc/doccomp
    sw/source/core/doc/doccorr
    sw/source/core/doc/docdesc
    sw/source/core/doc/docdraw
    sw/source/core/doc/docedt
    sw/source/core/doc/docfld
    sw/source/core/doc/docfly
    sw/source/core/doc/docfmt
    sw/source/core/doc/docftn
    sw/source/core/doc/docglbl
    sw/source/core/doc/docglos
    sw/source/core/doc/doclay
    sw/source/core/doc/docnew
    sw/source/core/doc/docnum
    sw/source/core/doc/docredln
    sw/source/core/doc/docruby
    sw/source/core/doc/docsort
    sw/source/core/doc/docstat
    sw/source/core/doc/doctxm
    sw/source/core/doc/DocumentDeviceManager
    sw/source/core/doc/docxforms
    sw/source/core/doc/DocumentSettingManager
    sw/source/core/doc/DocumentDrawModelManager
    sw/source/core/doc/DocumentChartDataProviderManager
    sw/source/core/doc/DocumentTimerManager
    sw/source/core/doc/DocumentLinksAdministrationManager
    sw/source/core/doc/FillBitmapLinkTracker
    sw/source/core/doc/DocumentListItemsManager
    sw/source/core/doc/DocumentListsManager
    sw/source/core/doc/DocumentOutlineNodesManager
    sw/source/core/doc/DocumentContentOperationsManager
    sw/source/core/doc/DocumentRedlineManager
    sw/source/core/doc/DocumentFieldsManager
    sw/source/core/doc/DocumentStatisticsManager
    sw/source/core/doc/DocumentStateManager
    sw/source/core/doc/DocumentLayoutManager
    sw/source/core/doc/DocumentStylePoolManager
    sw/source/core/doc/DocumentExternalDataManager
    sw/source/core/doc/extinput
    sw/source/core/doc/fmtcol
    sw/source/core/doc/ftnidx
    sw/source/core/doc/gctable
    sw/source/core/doc/htmltbl
    sw/source/core/doc/lineinfo
    sw/source/core/doc/list
    sw/source/core/doc/notxtfrm
    sw/source/core/doc/number
    sw/source/core/doc/poolfmt
    sw/source/core/doc/rdfhelper
    sw/source/core/doc/sortopt
    sw/source/core/doc/swserv
    sw/source/core/doc/swstylemanager
    sw/source/core/doc/tblafmt
    sw/source/core/doc/tblcpy
    sw/source/core/doc/tblrwcl
    sw/source/core/doc/textboxhelper
    sw/source/core/doc/visiturl
    sw/source/core/docnode/cancellablejob
    sw/source/core/docnode/finalthreadmanager
    sw/source/core/docnode/ndcopy
    sw/source/core/docnode/ndnotxt
    sw/source/core/docnode/ndnum
    sw/source/core/docnode/ndsect
    sw/source/core/docnode/ndtbl
    sw/source/core/docnode/ndtbl1
    sw/source/core/docnode/node
    sw/source/core/docnode/node2lay
    sw/source/core/docnode/nodes
    sw/source/core/docnode/observablethread
    sw/source/core/docnode/pausethreadstarting
    sw/source/core/docnode/retrievedinputstreamdata
    sw/source/core/docnode/retrieveinputstream
    sw/source/core/docnode/retrieveinputstreamconsumer
    sw/source/core/docnode/section
    sw/source/core/docnode/swbaslnk
    sw/source/core/docnode/swthreadjoiner
    sw/source/core/docnode/swthreadmanager
    sw/source/core/docnode/threadlistener
    sw/source/core/docnode/threadmanager
    sw/source/core/draw/dcontact
    sw/source/core/draw/dflyobj
    sw/source/core/draw/dobjfac
    sw/source/core/draw/dpage
    sw/source/core/draw/drawdoc
    sw/source/core/draw/dview
    sw/source/core/edit/acorrect
    sw/source/core/edit/autofmt
    sw/source/core/edit/edatmisc
    sw/source/core/edit/edattr
    sw/source/core/edit/eddel
    sw/source/core/edit/edfcol
    sw/source/core/edit/edfld
    sw/source/core/edit/edfldexp
    sw/source/core/edit/edfmt
    sw/source/core/edit/edglbldc
    sw/source/core/edit/edglss
    sw/source/core/edit/editsh
    sw/source/core/edit/edlingu
    sw/source/core/edit/ednumber
    sw/source/core/edit/edredln
    sw/source/core/edit/edsect
    sw/source/core/edit/edtab
    sw/source/core/edit/edtox
    sw/source/core/edit/edundo
    sw/source/core/edit/edws
    sw/source/core/fields/authfld
    sw/source/core/fields/cellfml
    sw/source/core/fields/chpfld
    sw/source/core/fields/dbfld
    sw/source/core/fields/ddefld
    sw/source/core/fields/ddetbl
    sw/source/core/fields/docufld
    sw/source/core/fields/expfld
    sw/source/core/fields/fldbas
    sw/source/core/fields/flddat
    sw/source/core/fields/flddropdown
    sw/source/core/fields/fldlst
    sw/source/core/fields/macrofld
    sw/source/core/fields/postithelper
    sw/source/core/fields/reffld
    sw/source/core/fields/scrptfld
    sw/source/core/fields/tblcalc
    sw/source/core/fields/textapi
    sw/source/core/fields/usrfld
    sw/source/core/frmedt/fecopy
    sw/source/core/frmedt/fedesc
    sw/source/core/frmedt/fefly1
    sw/source/core/frmedt/feflyole
    sw/source/core/frmedt/feshview
    sw/source/core/frmedt/fetab
    sw/source/core/frmedt/fews
    sw/source/core/frmedt/tblsel
    sw/source/core/graphic/grfatr
    sw/source/core/graphic/ndgrf
    sw/source/core/graphic/GraphicSizeCheck
    sw/source/core/layout/anchoreddrawobject
    sw/source/core/layout/anchoredobject
    sw/source/core/layout/atrfrm
    sw/source/core/layout/calcmove
    sw/source/core/layout/colfrm
    sw/source/core/layout/dbg_lay
    sw/source/core/layout/dumpfilter
    sw/source/core/layout/findfrm
    sw/source/core/layout/flowfrm
    sw/source/core/layout/fly
    sw/source/core/layout/flycnt
    sw/source/core/layout/flyincnt
    sw/source/core/layout/flylay
    sw/source/core/layout/flypos
    sw/source/core/layout/frmtool
    sw/source/core/layout/ftnfrm
    sw/source/core/layout/hffrm
    sw/source/core/layout/layact
    sw/source/core/layout/laycache
    sw/source/core/layout/layouter
    sw/source/core/layout/movedfwdfrmsbyobjpos
    sw/source/core/layout/newfrm
    sw/source/core/layout/objectformatter
    sw/source/core/layout/objectformatterlayfrm
    sw/source/core/layout/objectformattertxtfrm
    sw/source/core/layout/objstmpconsiderwrapinfl
    sw/source/core/layout/pagechg
    sw/source/core/layout/pagedesc
    sw/source/core/layout/paintfrm
    sw/source/core/layout/sectfrm
    sw/source/core/layout/softpagebreak
    sw/source/core/layout/sortedobjs
    sw/source/core/layout/ssfrm
    sw/source/core/layout/swselectionlist
    sw/source/core/layout/tabfrm
    sw/source/core/layout/trvlfrm
    sw/source/core/layout/unusedf
    sw/source/core/layout/virtoutp
    sw/source/core/layout/wsfrm
    sw/source/core/model/ModelTraverser
    sw/source/core/model/SearchResultLocator
    sw/source/core/model/ThemeColorChanger
    sw/source/core/objectpositioning/anchoredobjectposition
    sw/source/core/objectpositioning/ascharanchoredobjectposition
    sw/source/core/objectpositioning/environmentofanchoredobject
    sw/source/core/objectpositioning/tocntntanchoredobjectposition
    sw/source/core/objectpositioning/tolayoutanchoredobjectposition
    sw/source/core/ole/ndole
    sw/source/core/para/paratr
    sw/source/core/sw3io/swacorr
    sw/source/core/swg/SwXMLBlockExport
    sw/source/core/swg/SwXMLBlockImport
    sw/source/core/swg/SwXMLSectionList
    sw/source/core/swg/SwXMLTextBlocks
    sw/source/core/swg/SwXMLTextBlocks1
    sw/source/core/swg/swblocks
    sw/source/core/table/swnewtable
    sw/source/core/table/swtable
    sw/source/core/text/EnhancedPDFExportHelper
    sw/source/core/text/SwGrammarMarkUp
    sw/source/core/text/atrstck
    sw/source/core/text/frmcrsr
    sw/source/core/text/frmform
    sw/source/core/text/frminf
    sw/source/core/text/frmpaint
    sw/source/core/text/guess
    sw/source/core/text/inftxt
    sw/source/core/text/itradj
    sw/source/core/text/itratr
    sw/source/core/text/itrcrsr
    sw/source/core/text/itrform2
    sw/source/core/text/itrpaint
    sw/source/core/text/itrtxt
    sw/source/core/text/noteurl
    sw/source/core/text/porexp
    sw/source/core/text/porfld
    sw/source/core/text/porfly
    sw/source/core/text/porglue
    sw/source/core/text/porlay
    sw/source/core/text/porlin
    sw/source/core/text/pormulti
    sw/source/core/text/porref
    sw/source/core/text/porrst
    sw/source/core/text/portox
    sw/source/core/text/portxt
    sw/source/core/text/redlnitr
    sw/source/core/text/txtcache
    sw/source/core/text/txtdrop
    sw/source/core/text/txtfld
    sw/source/core/text/txtfly
    sw/source/core/text/txtfrm
    sw/source/core/text/txtftn
    sw/source/core/text/txthyph
    sw/source/core/text/txtinit
    sw/source/core/text/txtpaint
    sw/source/core/text/txttab
    sw/source/core/text/widorp
    sw/source/core/text/wrong
    sw/source/core/text/xmldump
    sw/source/core/tox/tox
    sw/source/core/tox/toxhlp
    sw/source/core/tox/txmsrt
    sw/source/core/tox/ToxLinkProcessor
    sw/source/core/tox/ToxTabStopTokenHandler
    sw/source/core/tox/ToxTextGenerator
    sw/source/core/tox/ToxWhitespaceStripper
    sw/source/core/txtnode/GrammarContact
    sw/source/core/txtnode/OnlineAccessibilityCheck
    sw/source/core/txtnode/attrcontentcontrol
    sw/source/core/txtnode/atrfld
    sw/source/core/txtnode/atrflyin
    sw/source/core/txtnode/atrftn
    sw/source/core/txtnode/atrref
    sw/source/core/txtnode/atrtox
    sw/source/core/txtnode/attrlinebreak
    sw/source/core/txtnode/chrfmt
    sw/source/core/txtnode/justify
    sw/source/core/txtnode/fmtatr2
    sw/source/core/txtnode/fntcache
    sw/source/core/txtnode/fntcap
    sw/source/core/txtnode/modeltoviewhelper
    sw/source/core/txtnode/ndhints
    sw/source/core/txtnode/ndtxt
    sw/source/core/txtnode/swfntcch
    sw/source/core/txtnode/swfont
    sw/source/core/txtnode/thints
    sw/source/core/txtnode/txatbase
    sw/source/core/txtnode/txatritr
    sw/source/core/txtnode/txtatr2
    sw/source/core/txtnode/txtedt
    sw/source/core/undo/SwRewriter
    sw/source/core/undo/SwUndoField
    sw/source/core/undo/SwUndoFmt
    sw/source/core/undo/SwUndoPageDesc
    sw/source/core/undo/SwUndoTOXChange
    sw/source/core/undo/docundo
    sw/source/core/undo/rolbck
    sw/source/core/undo/unattr
    sw/source/core/undo/unbkmk
    sw/source/core/undo/undel
    sw/source/core/undo/undobj
    sw/source/core/undo/undobj1
    sw/source/core/undo/undoflystrattr
    sw/source/core/undo/undraw
    sw/source/core/undo/unfmco
    sw/source/core/undo/unins
    sw/source/core/undo/unmove
    sw/source/core/undo/unnum
    sw/source/core/undo/unoutl
    sw/source/core/undo/unovwr
    sw/source/core/undo/unredln
    sw/source/core/undo/unsect
    sw/source/core/undo/unsort
    sw/source/core/undo/unspnd
    sw/source/core/undo/untbl
    sw/source/core/undo/untblk
    sw/source/core/undo/UndoThemeChange
    sw/source/core/unocore/SwXTextDefaults
    sw/source/core/unocore/TextCursorHelper
    sw/source/core/unocore/XMLRangeHelper
    sw/source/core/unocore/swunohelper
    sw/source/core/unocore/unobkm
    sw/source/core/unocore/unochart
    sw/source/core/unocore/unocoll
    sw/source/core/unocore/unocontentcontrol
    sw/source/core/unocore/unocrsr
    sw/source/core/unocore/unocrsrhelper
    sw/source/core/unocore/unodraw
    sw/source/core/unocore/unoevent
    sw/source/core/unocore/unofield
    sw/source/core/unocore/unoflatpara
    sw/source/core/unocore/unoframe
    sw/source/core/unocore/unoftn
    sw/source/core/unocore/unoidx
    sw/source/core/unocore/unolinebreak
    sw/source/core/unocore/unomap
    sw/source/core/unocore/unomap1
    sw/source/core/unocore/unoobj
    sw/source/core/unocore/unoobj2
    sw/source/core/unocore/unoparagraph
    sw/source/core/unocore/unoport
    sw/source/core/unocore/unoportenum
    sw/source/core/unocore/unoredline
    sw/source/core/unocore/unoredlines
    sw/source/core/unocore/unorefmk
    sw/source/core/unocore/unosect
    sw/source/core/unocore/unosett
    sw/source/core/unocore/unosrch
    sw/source/core/unocore/unostyle
    sw/source/core/unocore/unotbl
    sw/source/core/unocore/unotext
    sw/source/core/unocore/unotextmarkup
    sw/source/core/view/dialoghelp
    sw/source/core/view/pagepreviewlayout
    sw/source/core/view/printdata
    sw/source/core/view/vdraw
    sw/source/core/view/viewimp
    sw/source/core/view/viewpg
    sw/source/core/view/viewsh
    sw/source/core/view/vnew
    sw/source/core/view/vprint
    sw/source/filter/ascii/ascatr
    sw/source/filter/ascii/parasc
    sw/source/filter/ascii/wrtasc
    sw/source/filter/basflt/docfact
    sw/source/filter/basflt/fltini
    sw/source/filter/basflt/fltshell
    sw/source/filter/basflt/iodetect
    sw/source/filter/basflt/shellio
    sw/source/filter/html/SwAppletImpl
    sw/source/filter/html/css1atr
    sw/source/filter/html/htmlatr
    sw/source/filter/html/htmlbas
    sw/source/filter/html/htmlcss1
    sw/source/filter/html/htmlctxt
    sw/source/filter/html/htmldrawreader
    sw/source/filter/html/htmldrawwriter
    sw/source/filter/html/htmlfld
    sw/source/filter/html/htmlfldw
    sw/source/filter/html/htmlfly
    sw/source/filter/html/htmlflywriter
    sw/source/filter/html/htmlflyt
    sw/source/filter/html/htmlform
    sw/source/filter/html/htmlforw
    sw/source/filter/html/htmlftn
    sw/source/filter/html/htmlgrin
    sw/source/filter/html/htmlnum
    sw/source/filter/html/htmlnumreader
    sw/source/filter/html/htmlnumwriter
    sw/source/filter/html/htmlplug
    sw/source/filter/html/htmlreqifreader
    sw/source/filter/html/htmlsect
    sw/source/filter/html/htmltab
    sw/source/filter/html/htmltabw
    sw/source/filter/html/parcss1
    sw/source/filter/html/svxcss1
    sw/source/filter/html/swhtml
    sw/source/filter/html/wrthtml
    sw/source/filter/md/mdcallbcks
    sw/source/filter/md/mdnum
    sw/source/filter/md/mdtab
    sw/source/filter/md/swmd
    sw/source/filter/md/wrtmd
    sw/source/filter/writer/writer
    sw/source/filter/writer/wrt_fn
    sw/source/filter/writer/wrtswtbl
    sw/source/filter/xml/XMLRedlineImportHelper
    sw/source/filter/xml/swxml
    sw/source/filter/xml/wrtxml
    sw/source/filter/xml/xmlbrsh
    sw/source/filter/xml/xmlexp
    sw/source/filter/xml/xmlexpit
    sw/source/filter/xml/xmlfmt
    sw/source/filter/xml/xmlfmte
    sw/source/filter/xml/xmlfonte
    sw/source/filter/xml/xmlimp
    sw/source/filter/xml/xmlimpit
    sw/source/filter/xml/xmlitem
    sw/source/filter/xml/xmliteme
    sw/source/filter/xml/xmlitemi
    sw/source/filter/xml/xmlitemm
    sw/source/filter/xml/xmlithlp
    sw/source/filter/xml/xmlitmpr
    sw/source/filter/xml/xmlmeta
    sw/source/filter/xml/xmlscript
    sw/source/filter/xml/xmltble
    sw/source/filter/xml/xmltbli
    sw/source/filter/xml/xmltext
    sw/source/filter/xml/xmltexte
    sw/source/filter/xml/xmltexti
    sw/source/filter/indexing/IndexingExport
    sw/source/filter/indexing/IndexingExportFilter
    sw/source/uibase/app/appenv
    sw/source/uibase/app/apphdl
    sw/source/uibase/app/applab
    sw/source/uibase/app/appopt
    sw/source/uibase/app/docsh
    sw/source/uibase/app/docsh2
    sw/source/uibase/app/docshdrw
    sw/source/uibase/app/docshini
    sw/source/uibase/app/docst
    sw/source/uibase/app/docstyle
    sw/source/uibase/app/mainwn
    sw/source/uibase/app/swdll
    sw/source/uibase/app/swmodul1
    sw/source/uibase/app/swmodule
    sw/source/uibase/app/swwait
    sw/source/uibase/dbui/dbmgr
    sw/source/uibase/dbui/dbui
    sw/source/uibase/dbui/maildispatcher
    sw/source/uibase/dbui/mailmergehelper
    sw/source/uibase/dbui/mmconfigitem
    sw/source/uibase/chrdlg/ccoll
    sw/source/uibase/config/StoredChapterNumbering
    sw/source/uibase/config/barcfg
    sw/source/uibase/config/caption
    sw/source/uibase/config/cfgitems
    sw/source/uibase/config/dbconfig
    sw/source/uibase/config/fontcfg
    sw/source/uibase/config/modcfg
    sw/source/uibase/config/prtopt
    sw/source/uibase/config/SwBaselineGridConfig
    sw/source/uibase/config/uinums
    sw/source/uibase/config/usrpref
    sw/source/uibase/config/viewopt
    sw/source/uibase/dialog/SwSpellDialogChildWindow
    sw/source/uibase/dialog/regionsw
    sw/source/uibase/dialog/swabstdlg
    sw/source/uibase/dialog/swwrtshitem
    sw/source/uibase/dialog/watermarkdialog
    sw/source/uibase/dochdl/gloshdl
    sw/source/uibase/dochdl/swdtflvr
    sw/source/uibase/docvw/AnchorOverlayObject
    sw/source/uibase/docvw/AnnotationMenuButton
    sw/source/uibase/docvw/AnnotationWin
    sw/source/uibase/docvw/AnnotationWin2
    sw/source/uibase/docvw/DashedLine
    sw/source/uibase/docvw/FrameControlsManager
    sw/source/uibase/docvw/UnfloatTableButton
    sw/source/uibase/docvw/PageBreakWin
    sw/source/uibase/docvw/OverlayRanges
    sw/source/uibase/docvw/PostItMgr
    sw/source/uibase/docvw/ShadowOverlayObject
    sw/source/uibase/docvw/SidebarTxtControl
    sw/source/uibase/docvw/HeaderFooterWin
    sw/source/uibase/docvw/OutlineContentVisibilityWin
    sw/source/uibase/docvw/contentcontrolaliasbutton
    sw/source/uibase/docvw/edtdd
    sw/source/uibase/docvw/edtwin
    sw/source/uibase/docvw/edtwin2
    sw/source/uibase/docvw/edtwin3
    sw/source/uibase/docvw/frmsidebarwincontainer
    sw/source/uibase/docvw/romenu
    sw/source/uibase/docvw/srcedtw
    sw/source/uibase/envelp/envimg
    sw/source/uibase/envelp/labelcfg
    sw/source/uibase/envelp/labimg
    sw/source/uibase/envelp/syncbtn
    sw/source/uibase/fldui/fldmgr
    sw/source/uibase/fldui/fldwrap
    sw/source/uibase/fldui/xfldui
    sw/source/uibase/frmdlg/colex
    sw/source/uibase/frmdlg/colmgr
    sw/source/uibase/frmdlg/frmmgr
    sw/source/uibase/globdoc/globdoc
    sw/source/uibase/index/idxmrk
    sw/source/uibase/dialog/wordcountwrapper
    sw/source/uibase/index/toxmgr
    sw/source/uibase/lingu/hhcwrp
    sw/source/uibase/lingu/hyp
    sw/source/uibase/lingu/olmenu
    sw/source/uibase/lingu/sdrhhcwrap
    sw/source/uibase/misc/glosdoc
    sw/source/uibase/misc/glshell
    sw/source/uibase/misc/numberingtypelistbox
    sw/source/uibase/misc/redlndlg
    sw/source/uibase/misc/swruler
    sw/source/uibase/ribbar/conarc
    sw/source/uibase/ribbar/concustomshape
    sw/source/uibase/ribbar/conform
    sw/source/uibase/ribbar/conpoly
    sw/source/uibase/ribbar/conrect
    sw/source/uibase/ribbar/drawbase
    sw/source/uibase/ribbar/dselect
    sw/source/uibase/ribbar/inputwin
    sw/source/uibase/ribbar/workctrl
    sw/source/uibase/shells/annotsh
    sw/source/uibase/shells/basesh
    sw/source/uibase/shells/beziersh
    sw/source/uibase/shells/drawdlg
    sw/source/uibase/shells/drawsh
    sw/source/uibase/shells/drformsh
    sw/source/uibase/shells/drwbassh
    sw/source/uibase/shells/drwtxtex
    sw/source/uibase/shells/drwtxtsh
    sw/source/uibase/shells/frmsh
    sw/source/uibase/shells/grfsh
    sw/source/uibase/shells/grfshex
    sw/source/uibase/shells/langhelper
    sw/source/uibase/shells/pam2html
    sw/source/uibase/shells/listsh
    sw/source/uibase/shells/mediash
    sw/source/uibase/shells/navsh
    sw/source/uibase/shells/olesh
    sw/source/uibase/shells/slotadd
    sw/source/uibase/shells/tabsh
    sw/source/uibase/shells/textdrw
    sw/source/uibase/shells/textfld
    sw/source/uibase/shells/textglos
    sw/source/uibase/shells/textidx
    sw/source/uibase/shells/textsh
    sw/source/uibase/shells/textsh1
    sw/source/uibase/shells/textsh2
    sw/source/uibase/shells/txtattr
    sw/source/uibase/shells/txtcrsr
    sw/source/uibase/shells/txtnum
    sw/source/uibase/sidebar/StylePresetsPanel
    sw/source/uibase/sidebar/PageOrientationControl
    sw/source/uibase/sidebar/PageOrientationPopup
    sw/source/uibase/sidebar/PageMarginControl
    sw/source/uibase/sidebar/PageMarginPopup
    sw/source/uibase/sidebar/PageSizeControl
    sw/source/uibase/sidebar/PageSizePopup
    sw/source/uibase/sidebar/PageColumnControl
    sw/source/uibase/sidebar/PageColumnPopup
    sw/source/uibase/sidebar/PageFormatPanel
    sw/source/uibase/sidebar/PageHeaderPanel
    sw/source/uibase/sidebar/PageFooterPanel
    sw/source/uibase/sidebar/PageStylesPanel
    sw/source/uibase/sidebar/WrapPropertyPanel
    sw/source/uibase/sidebar/TableEditPanel
    sw/source/uibase/sidebar/ThemePanel
    sw/source/uibase/sidebar/SwPanelFactory
    sw/source/uibase/sidebar/WriterInspectorTextPanel
    sw/source/uibase/sidebar/CommentsPanel
    sw/source/uibase/sidebar/A11yCheckIssuesPanel
    sw/source/uibase/sidebar/QuickFindPanel
    sw/source/uibase/table/chartins
    sw/source/uibase/table/swtablerep
    sw/source/uibase/table/tablemgr
    sw/source/uibase/uitest/uiobject
    sw/source/uibase/uiview/formatclipboard
    sw/source/uibase/uiview/pview
    sw/source/uibase/uiview/scroll
    sw/source/uibase/uiview/srcview
    sw/source/uibase/uiview/swcli
    sw/source/uibase/uiview/uivwimp
    sw/source/uibase/uiview/view
    sw/source/uibase/uiview/view0
    sw/source/uibase/uiview/view1
    sw/source/uibase/uiview/view2
    sw/source/uibase/uiview/viewcoll
    sw/source/uibase/uiview/viewdlg
    sw/source/uibase/uiview/viewdlg2
    sw/source/uibase/uiview/viewdraw
    sw/source/uibase/uiview/viewling
    sw/source/uibase/uiview/viewmdi
    sw/source/uibase/uiview/viewport
    sw/source/uibase/uiview/viewprt
    sw/source/uibase/uiview/viewsrch
    sw/source/uibase/uiview/viewstat
    sw/source/uibase/uiview/viewtab
    sw/source/uibase/uno/SwXDocumentSettings
    sw/source/uibase/uno/SwXFilterOptions
    sw/source/uibase/uno/dlelstnr
    sw/source/uibase/uno/unoatxt
    sw/source/uibase/uno/unodefaults
    sw/source/uibase/uno/unodispatch
    sw/source/uibase/uno/unodoc
    sw/source/uibase/uno/unomailmerge
    sw/source/uibase/uno/unomod
    sw/source/uibase/uno/unomodule
    sw/source/uibase/uno/unotxdoc
    sw/source/uibase/uno/loktxdoc
    sw/source/uibase/uno/unotxvw
    sw/source/uibase/utlui/AccessibilityStatusBarControl
    sw/source/uibase/utlui/attrdesc
    sw/source/uibase/utlui/bookctrl
    sw/source/uibase/utlui/content
    sw/source/uibase/utlui/glbltree
    sw/source/uibase/utlui/gloslst
    sw/source/uibase/utlui/initui
    sw/source/uibase/utlui/navicfg
    sw/source/uibase/utlui/navipi
    sw/source/uibase/utlui/numfmtlb
    sw/source/uibase/utlui/prcntfld
    sw/source/uibase/utlui/shdwcrsr
    sw/source/uibase/utlui/tmplctrl
    sw/source/uibase/utlui/uiitems
    sw/source/uibase/utlui/uitool
    sw/source/uibase/utlui/unotools
    sw/source/uibase/utlui/viewlayoutctrl
    sw/source/uibase/utlui/wordcountctrl
    sw/source/uibase/utlui/zoomctrl
    sw/source/uibase/web/wdocsh
    sw/source/uibase/web/wformsh
    sw/source/uibase/web/wfrmsh
    sw/source/uibase/web/wgrfsh
    sw/source/uibase/web/wlistsh
    sw/source/uibase/web/wolesh
    sw/source/uibase/web/wtabsh
    sw/source/uibase/web/wtextsh
    sw/source/uibase/web/wview
    sw/source/uibase/wrtsh/delete
    sw/source/uibase/wrtsh/move
    sw/source/uibase/wrtsh/navmgr
    sw/source/uibase/wrtsh/select
    sw/source/uibase/wrtsh/wrtsh1
    sw/source/uibase/wrtsh/wrtsh2
    sw/source/uibase/wrtsh/wrtsh3
    sw/source/uibase/wrtsh/wrtsh4
    sw/source/uibase/wrtsh/wrtundo
)

# ifneq (,$(filter DBCONNECTIVITY,$(BUILD_TYPE)))
if (ENABLE_DBCONNECTIVITY)
    list(APPEND SW_SOURCES
        sw/source/uibase/dbui/dbtree
        sw/source/uibase/dbui/mailmergetoolbarcontrols)
endif ()

librestudio_expand_cxx(SW_SOURCES)

# Cached for reuse by sw_uwriter (sw/CppunitTest_deferred.cmake), which
# needs to recompile these same sources directly into the test binary via
# gb_CppunitTest_use_library_objects(sw_uwriter,sw) -- same CACHE INTERNAL
# reuse pattern already established by editeng/CMakeLists.txt's own
# EDITENG_SOURCES for editeng_core and sc/CMakeLists.txt's own SC_SOURCES
# for sc_ucalc_test.
set(SW_SOURCES "${SW_SOURCES}" CACHE INTERNAL
    "sw library's own expanded source list, for sw_uwriter's use_library_objects")

add_library(sw SHARED ${SW_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(sw PRIVATE
        "${CMAKE_SOURCE_DIR}/sw/inc/pch/precompiled_sw.hxx")
    target_compile_definitions(sw PRIVATE PCH_LEVEL=2)
endif ()
add_dependencies(sw sw_generated sw_sdi)

target_include_directories(sw PRIVATE
    "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
    "${CMAKE_SOURCE_DIR}/sw/source/filter/inc"
    "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
    "${CMAKE_SOURCE_DIR}/sw/inc"
    "${SW_GENERATED_DIR}"
    "${sw_sdi_INCLUDE_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}")
add_dependencies(sw
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
    officecfg_headers)

target_compile_definitions(sw PRIVATE
    -DSW_DLLIMPLEMENTATION
    -DSWUI_DLL_NAME=\"libswuilo.so\")

target_link_libraries(sw PRIVATE
    avmedia basegfx comphelper cppu cppuhelper docmodel
    drawinglayercore drawinglayer editeng fwk i18nlangtag i18npool i18nutil
    lng msfilter sal salhelper sax sb sfx sot svl svt svx svxcore
    textconversiondlgs tk tl ucbhelper utl vbahelper vcl xmlreader xo
    Boost::boost
    ICU::uc ICU::i18n
    LibXml2::LibXml2
    ${LIBRESTUDIO_MD4C_TARGET})

# ifneq (,$(filter DBCONNECTIVITY,$(BUILD_TYPE)))
if (ENABLE_DBCONNECTIVITY)
    target_link_libraries(sw PRIVATE dbtools)
endif ()

# gb_Library_add_componentimpl(sw,mailmerge) is gated by the same
# DBCONNECTIVITY ifneq as the two exception objects above: the mailmerge
# component implementations (lo.writer.MMCurrentEntryController,
# lo.writer.MMExcludeEntryController -- both marked <optional/> in
# sw.component itself) only exist to be *not* filtered out of the registry
# when this platform provides them. librestudio_add_component()'s
# IMPLEMENTATION is a multi-value keyword arg (cmake_parse_arguments); passed
# an empty list it leaves _comp_IMPLEMENTATION unset, taking the same
# "no IMPLEMENTATION given" branch as msword/swui/vbaswobj/swd below, all of
# which never had one -- so an empty SW_COMPONENT_IMPLS is equivalent to
# omitting the keyword entirely, and this is the ON/OFF-safe way to make the
# argument conditional without duplicating the whole call in an if/else.
set(SW_COMPONENT_IMPLS "")
if (ENABLE_DBCONNECTIVITY)
    list(APPEND SW_COMPONENT_IMPLS "${CMAKE_SOURCE_DIR}/sw/util/sw.component.mailmerge")
endif ()

librestudio_add_component(sw
    COMPONENT      "${CMAKE_SOURCE_DIR}/sw/util/sw.component"
    IMPLEMENTATION ${SW_COMPONENT_IMPLS}
    LIBRARY        sw
    LAYER          OOO)

librestudio_set_library_output_name(sw sw OOO)

install(TARGETS sw LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# swui -- Library_swui.mk
# ---------------------------------------------------------------------------

set(SWUI_SOURCES
    # main source list (unconditional)
    sw/source/ui/chrdlg/break
    sw/source/ui/chrdlg/chardlg
    sw/source/ui/chrdlg/drpcps
    sw/source/ui/chrdlg/numpara
    sw/source/ui/chrdlg/pardlg
    sw/source/ui/chrdlg/swuiccoll
    sw/source/ui/chrdlg/tblnumfm
    sw/source/ui/config/mailconfigpage
    sw/source/ui/config/optcomp
    sw/source/ui/config/optload
    sw/source/ui/config/optpage
    sw/source/ui/config/SwGridTabPage
    sw/source/ui/dialog/abstract
    sw/source/ui/dialog/addrdlg
    sw/source/ui/dialog/ascfldlg
    sw/source/ui/dialog/docstdlg
    sw/source/ui/dialog/macassgn
    sw/source/ui/dialog/swdlgfact
    sw/source/ui/dialog/swmessdialog
    sw/source/ui/dialog/swuiexp
    sw/source/ui/dialog/uiregionsw
    sw/source/ui/dialog/wordcountdialog
    sw/source/ui/dochdl/selglos
    sw/source/ui/envelp/envfmt
    sw/source/ui/envelp/envlop1
    sw/source/ui/envelp/envprt
    sw/source/ui/envelp/label1
    sw/source/ui/envelp/labelexp
    sw/source/ui/envelp/labfmt
    sw/source/ui/envelp/labprt
    sw/source/ui/envelp/mailmrge
    sw/source/ui/fldui/DateFormFieldDialog
    sw/source/ui/fldui/DropDownFieldDialog
    sw/source/ui/fldui/DropDownFormFieldDialog
    sw/source/ui/fldui/changedb
    sw/source/ui/fldui/flddb
    sw/source/ui/fldui/flddinf
    sw/source/ui/fldui/flddok
    sw/source/ui/fldui/fldedt
    sw/source/ui/fldui/fldfunc
    sw/source/ui/fldui/fldpage
    sw/source/ui/fldui/fldref
    sw/source/ui/fldui/fldtdlg
    sw/source/ui/fldui/fldvar
    sw/source/ui/fldui/inpdlg
    sw/source/ui/fldui/javaedit
    sw/source/ui/fmtui/tmpdlg
    sw/source/ui/frmdlg/column
    sw/source/ui/frmdlg/cption
    sw/source/ui/frmdlg/frmdlg
    sw/source/ui/frmdlg/frmpage
    sw/source/ui/frmdlg/pattern
    sw/source/ui/frmdlg/uiborder
    sw/source/ui/frmdlg/wrap
    sw/source/ui/index/cntex
    sw/source/ui/index/cnttab
    sw/source/ui/index/multmrk
    sw/source/ui/index/swuiidxmrk
    sw/source/ui/misc/bookmark
    sw/source/ui/misc/contentcontroldlg
    sw/source/ui/misc/contentcontrollistitemdlg
    sw/source/ui/misc/pagenumberdlg
    sw/source/ui/misc/docfnote
    sw/source/ui/misc/glosbib
    sw/source/ui/misc/glossary
    sw/source/ui/misc/insfnote
    sw/source/ui/misc/linenum
    sw/source/ui/misc/num
    sw/source/ui/misc/outline
    sw/source/ui/misc/pgfnote
    sw/source/ui/misc/pggrid
    sw/source/ui/misc/srtdlg
    sw/source/ui/misc/swmodalredlineacceptdlg
    sw/source/ui/misc/titlepage
    sw/source/ui/table/colwd
    sw/source/ui/table/convert
    sw/source/ui/table/instable
    sw/source/ui/table/mergetbl
    sw/source/ui/table/rowht
    sw/source/ui/table/splittbl
    sw/source/ui/table/tabledlg
    sw/source/ui/utlui/swrenamexnameddlg
    sw/source/ui/utlui/copyfielddlg
)

# ifneq (,$(filter DBCONNECTIVITY,$(BUILD_TYPE)))
if (ENABLE_DBCONNECTIVITY)
    list(APPEND SWUI_SOURCES
        sw/source/ui/dbui/addresslistdialog
        sw/source/ui/dbui/createaddresslistdialog
        sw/source/ui/dbui/customizeaddresslistdialog
        sw/source/ui/dbui/dbinsdlg
        sw/source/ui/dbui/dbtablepreviewdialog
        sw/source/ui/dbui/mailmergewizard
        sw/source/ui/dbui/mmaddressblockpage
        sw/source/ui/dbui/mmdocselectpage
        sw/source/ui/dbui/mmgreetingspage
        sw/source/ui/dbui/mmlayoutpage
        sw/source/ui/dbui/mmoutputtypepage
        sw/source/ui/dbui/mmresultdialogs
        sw/source/ui/dbui/selectdbtabledialog)
endif ()

librestudio_expand_cxx(SWUI_SOURCES)
add_library(swui SHARED ${SWUI_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(swui PRIVATE
        "${CMAKE_SOURCE_DIR}/sw/inc/pch/precompiled_swui.hxx")
    target_compile_definitions(swui PRIVATE PCH_LEVEL=2)
endif ()
add_dependencies(swui sw_sdi)

target_include_directories(swui PRIVATE
    "${CMAKE_SOURCE_DIR}/sw/inc"
    "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
    "${CMAKE_SOURCE_DIR}/sw/source/ui/inc"
    "${sw_sdi_INCLUDE_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(swui
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
    officecfg_headers)

target_link_libraries(swui PRIVATE
    comphelper cppu cppuhelper editeng i18nlangtag i18nutil msfilter
    sal salhelper sfx sot svl svt svx svxcore sw tk tl ucbhelper utl vcl
    drawinglayercore drawinglayer
    Boost::boost
    ICU::uc ICU::i18n
    LibXml2::LibXml2)

# ifneq (,$(filter DBCONNECTIVITY,$(BUILD_TYPE)))
if (ENABLE_DBCONNECTIVITY)
    target_link_libraries(swui PRIVATE dbtools)
endif ()

librestudio_add_component(swui
    COMPONENT "${CMAKE_SOURCE_DIR}/sw/util/swui.component"
    LIBRARY   swui
    LAYER     OOO)

librestudio_set_library_output_name(swui swui OOO)

install(TARGETS swui LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# sw_writerfilter -- Library_sw_writerfilter.mk (RTF/OOXML import). See this
# file's own header comment for the writerfilter design note.
# ---------------------------------------------------------------------------

set(SW_WRITERFILTER_SOURCES
    sw/source/writerfilter/rtftok/rtfcharsets
    sw/source/writerfilter/rtftok/rtfdispatchdestination
    sw/source/writerfilter/rtftok/rtfdispatchflag
    sw/source/writerfilter/rtftok/rtfdispatchsymbol
    sw/source/writerfilter/rtftok/rtfdispatchvalue
    sw/source/writerfilter/rtftok/rtfdocumentfactory
    sw/source/writerfilter/rtftok/rtfdocumentimpl
    sw/source/writerfilter/rtftok/rtflookahead
    sw/source/writerfilter/rtftok/rtfreferenceproperties
    sw/source/writerfilter/rtftok/rtfreferencetable
    sw/source/writerfilter/rtftok/rtfsdrimport
    sw/source/writerfilter/rtftok/rtfskipdestination
    sw/source/writerfilter/rtftok/rtfsprm
    sw/source/writerfilter/rtftok/rtftokenizer
    sw/source/writerfilter/rtftok/rtfvalue
    sw/source/writerfilter/dmapper/BorderHandler
    sw/source/writerfilter/dmapper/CellColorHandler
    sw/source/writerfilter/dmapper/CellMarginHandler
    sw/source/writerfilter/dmapper/ConversionHelper
    sw/source/writerfilter/dmapper/DocumentProtection
    sw/source/writerfilter/dmapper/DomainMapper
    sw/source/writerfilter/dmapper/DomainMapperTableHandler
    sw/source/writerfilter/dmapper/DomainMapperTableManager
    sw/source/writerfilter/dmapper/DomainMapper_Impl
    sw/source/writerfilter/dmapper/domainmapperfactory
    sw/source/writerfilter/dmapper/FFDataHandler
    sw/source/writerfilter/dmapper/FontTable
    sw/source/writerfilter/dmapper/FormControlHelper
    sw/source/writerfilter/dmapper/GraphicHelpers
    sw/source/writerfilter/dmapper/GraphicImport
    sw/source/writerfilter/dmapper/LatentStyleHandler
    sw/source/writerfilter/dmapper/LoggedResources
    sw/source/writerfilter/dmapper/MeasureHandler
    sw/source/writerfilter/dmapper/TrackChangesHandler
    sw/source/writerfilter/dmapper/ModelEventListener
    sw/source/writerfilter/dmapper/NumberingManager
    sw/source/writerfilter/dmapper/OLEHandler
    sw/source/writerfilter/dmapper/PageBordersHandler
    sw/source/writerfilter/dmapper/PropertyIds
    sw/source/writerfilter/dmapper/PropertyMap
    sw/source/writerfilter/dmapper/PropertyMapHelper
    sw/source/writerfilter/dmapper/SdtHelper
    sw/source/writerfilter/dmapper/SectionColumnHandler
    sw/source/writerfilter/dmapper/SettingsTable
    sw/source/writerfilter/dmapper/SmartTagHandler
    sw/source/writerfilter/dmapper/StyleSheetTable
    sw/source/writerfilter/dmapper/TDefTableHandler
    sw/source/writerfilter/dmapper/TableManager
    sw/source/writerfilter/dmapper/TablePositionHandler
    sw/source/writerfilter/dmapper/TablePropertiesHandler
    sw/source/writerfilter/dmapper/TagLogger
    sw/source/writerfilter/dmapper/TextEffectsHandler
    sw/source/writerfilter/dmapper/TblStylePrHandler
    sw/source/writerfilter/dmapper/ThemeHandler
    sw/source/writerfilter/dmapper/WrapPolygonHandler
    sw/source/writerfilter/dmapper/WriteProtection
    sw/source/writerfilter/dmapper/util
    sw/source/writerfilter/filter/RtfFilter
    sw/source/writerfilter/filter/WriterFilter
    sw/source/writerfilter/ooxml/Handler
    sw/source/writerfilter/ooxml/OOXMLBinaryObjectReference
    sw/source/writerfilter/ooxml/OOXMLDocumentImpl
    sw/source/writerfilter/ooxml/OOXMLFactory
    sw/source/writerfilter/ooxml/OOXMLFastContextHandler
    sw/source/writerfilter/ooxml/OOXMLFastContextHandlerTheme
    sw/source/writerfilter/ooxml/OOXMLFastDocumentHandler
    sw/source/writerfilter/ooxml/OOXMLParserState
    sw/source/writerfilter/ooxml/OOXMLPropertySet
    sw/source/writerfilter/ooxml/OOXMLStreamImpl
    sw/source/writerfilter/ooxml/ShadowContext
)
librestudio_expand_cxx(SW_WRITERFILTER_SOURCES)

# The 30 generated compilation units (28 per-namespace + OOXMLFactory_
# generated.cxx + qnametostr.cxx) are absolute, already-.cxx-suffixed paths
# from the generator block above -- appended directly, not through
# librestudio_expand_cxx (same technique already established for
# i18npool/CMakeLists.txt's own generated localedata_*.cxx sources).
add_library(sw_writerfilter SHARED
    ${SW_WRITERFILTER_SOURCES}
    "${SW_WRITERFILTER_FACTORY_GENERATED_CXX}"
    "${SW_WRITERFILTER_QNAMETOSTR_CXX}"
    ${SW_WRITERFILTER_NS_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(sw_writerfilter PRIVATE
        "${CMAKE_SOURCE_DIR}/sw/inc/pch/precompiled_sw_writerfilter.hxx")
    target_compile_definitions(sw_writerfilter PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(sw_writerfilter PRIVATE
    "${CMAKE_SOURCE_DIR}/sw/inc"
    "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
    "${CMAKE_SOURCE_DIR}/sw/source/uibase/uno"
    "${CMAKE_SOURCE_DIR}/sw/source/writerfilter/inc"
    "${CMAKE_SOURCE_DIR}/sw/source/writerfilter"
    "${SW_WRITERFILTER_GENERATED_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${OOX_GENERATED_DIR}"
    "${FROZEN_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")

add_dependencies(sw_writerfilter
    sw_writerfilter_generated oox_generated officecfg_headers
    udkapi_normal_headers offapi_normal_headers)

target_link_libraries(sw_writerfilter PRIVATE
    basegfx comphelper cppu cppuhelper docmodel editeng i18nlangtag i18nutil
    msfilter oox sal salhelper sax sfx sot svl svt svxcore sw tl utl vcl
    Boost::boost
    ICU::uc ICU::i18n
    LibXml2::LibXml2)

librestudio_add_component(sw_writerfilter
    COMPONENT "${CMAKE_SOURCE_DIR}/sw/util/sw_writerfilter.component"
    LIBRARY   sw_writerfilter
    LAYER     OOO)

librestudio_set_library_output_name(sw_writerfilter sw_writerfilter OOO)

install(TARGETS sw_writerfilter LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# msword -- Library_msword.mk
# ---------------------------------------------------------------------------
#
# sw_writerfilter is built now (see the header comment at the top of this
# file), but msword never needed it at compile/link time either way --
# grepping all 39 source files below for "writerfilter" turns up only
# comments and two UNO createInstance("com.sun.star.comp.Writer.
# WriterFilter") service-manager lookups in swdocxreader.cxx, resolved at
# runtime through the component registry, not a header/link dependency.

set(MSWORD_SOURCES
    sw/source/filter/docx/swdocxreader
    sw/source/filter/rtf/swparrtf
    sw/source/filter/ww8/docxattributeoutput
    sw/source/filter/ww8/docxexport
    sw/source/filter/ww8/docxexportfilter
    sw/source/filter/ww8/docxsdrexport
    sw/source/filter/ww8/docxtableexport
    sw/source/filter/ww8/docxtablestyleexport
    sw/source/filter/ww8/rtfattributeoutput
    sw/source/filter/ww8/rtfexport
    sw/source/filter/ww8/rtfexportfilter
    sw/source/filter/ww8/rtfsdrexport
    sw/source/filter/ww8/rtfstringbuffer
    sw/source/filter/ww8/WW8FFData
    sw/source/filter/ww8/WW8FibData
    sw/source/filter/ww8/WW8Sttbf
    sw/source/filter/ww8/WW8TableInfo
    sw/source/filter/ww8/fields
    sw/source/filter/ww8/styles
    sw/source/filter/ww8/writerhelper
    sw/source/filter/ww8/writerwordglue
    sw/source/filter/ww8/wrtw8esh
    sw/source/filter/ww8/wrtw8nds
    sw/source/filter/ww8/wrtw8num
    sw/source/filter/ww8/wrtw8sty
    sw/source/filter/ww8/wrtww8
    sw/source/filter/ww8/wrtww8gr
    sw/source/filter/ww8/ww8atr
    sw/source/filter/ww8/ww8glsy
    sw/source/filter/ww8/ww8graf
    sw/source/filter/ww8/ww8graf2
    sw/source/filter/ww8/ww8par
    sw/source/filter/ww8/ww8par2
    sw/source/filter/ww8/ww8par3
    sw/source/filter/ww8/ww8par4
    sw/source/filter/ww8/ww8par5
    sw/source/filter/ww8/ww8par6
    sw/source/filter/ww8/ww8scan
    sw/source/filter/ww8/ww8toolbar
)

librestudio_expand_cxx(MSWORD_SOURCES)
add_library(msword SHARED ${MSWORD_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(msword PRIVATE
        "${CMAKE_SOURCE_DIR}/sw/inc/pch/precompiled_msword.hxx")
    target_compile_definitions(msword PRIVATE PCH_LEVEL=2)
endif ()
add_dependencies(msword oox_generated)

target_include_directories(msword PRIVATE
    "${CMAKE_SOURCE_DIR}/sw/source/core/inc"
    "${CMAKE_SOURCE_DIR}/sw/source/filter/inc"
    "${CMAKE_SOURCE_DIR}/sw/inc"
    "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
    "${OOX_GENERATED_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}"
    "${FROZEN_INCLUDE_DIR}")
add_dependencies(msword
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
    officecfg_headers)

target_link_libraries(msword PRIVATE
    basegfx comphelper cppu cppuhelper docmodel editeng i18nlangtag i18nutil
    msfilter sal sax sb oox sfx sot svl svt svx svxcore sw tk tl ucbhelper
    utl vcl
    Boost::boost
    ICU::uc ICU::i18n
    LibXml2::LibXml2)

librestudio_add_component(msword
    COMPONENT "${CMAKE_SOURCE_DIR}/sw/util/msword.component"
    LIBRARY   msword
    LAYER     OOO)

librestudio_set_library_output_name(msword msword OOO)

install(TARGETS msword LIBRARY DESTINATION program)

# ---------------------------------------------------------------------------
# vbaswobj -- Library_vbaswobj.mk
# ---------------------------------------------------------------------------
#
# Module_sw.mk only adds Library_vbaswobj to the module's target list at all
# under `ifneq ($(filter SCRIPTING,$(BUILD_TYPE)),)` -- unlike the
# DBCONNECTIVITY gates above, which each cover a source-file/library-link
# subset of an always-built target, this gate is on the whole target. Mirror
# that here by wrapping the entire vbaswobj section in ENABLE_SCRIPTING
# (default ON) rather than gating pieces of it.

if (ENABLE_SCRIPTING)

set(VBASWOBJ_SOURCES
    sw/source/ui/vba/vbarange
    sw/source/ui/vba/vbacell
    sw/source/ui/vba/vbacells
    sw/source/ui/vba/vbacolumn
    sw/source/ui/vba/vbacolumns
    sw/source/ui/vba/vbacontentcontrol
    sw/source/ui/vba/vbacontentcontrols
    sw/source/ui/vba/vbacontentcontrollistentry
    sw/source/ui/vba/vbacontentcontrollistentries
    sw/source/ui/vba/vbaformfield
    sw/source/ui/vba/vbaformfields
    sw/source/ui/vba/vbaformfieldcheckbox
    sw/source/ui/vba/vbaformfielddropdown
    sw/source/ui/vba/vbaformfielddropdownlistentries
    sw/source/ui/vba/vbaformfielddropdownlistentry
    sw/source/ui/vba/vbaformfieldtextinput
    sw/source/ui/vba/vbaframe
    sw/source/ui/vba/vbaframes
    sw/source/ui/vba/vbalistformat
    sw/source/ui/vba/vbalistgallery
    sw/source/ui/vba/vbalistgalleries
    sw/source/ui/vba/vbalisthelper
    sw/source/ui/vba/vbalistlevel
    sw/source/ui/vba/vbalistlevels
    sw/source/ui/vba/vbalisttemplate
    sw/source/ui/vba/vbalisttemplates
    sw/source/ui/vba/vbamailmerge
    sw/source/ui/vba/vbarevision
    sw/source/ui/vba/vbarevisions
    sw/source/ui/vba/vbarow
    sw/source/ui/vba/vbarows
    sw/source/ui/vba/vbadocument
    sw/source/ui/vba/vbasections
    sw/source/ui/vba/vbadialog
    sw/source/ui/vba/vbawrapformat
    sw/source/ui/vba/vbafont
    sw/source/ui/vba/vbarangehelper
    sw/source/ui/vba/vbaaddin
    sw/source/ui/vba/vbaautotextentry
    sw/source/ui/vba/vbadocumentproperties
    sw/source/ui/vba/vbaeventshelper
    sw/source/ui/vba/vbastyle
    sw/source/ui/vba/vbapane
    sw/source/ui/vba/vbaglobals
    sw/source/ui/vba/vbatemplate
    sw/source/ui/vba/vbaaddins
    sw/source/ui/vba/vbaview
    sw/source/ui/vba/vbaheaderfooter
    sw/source/ui/vba/vbaheadersfooters
    sw/source/ui/vba/vbaheaderfooterhelper
    sw/source/ui/vba/vbabookmarks
    sw/source/ui/vba/vbaoptions
    sw/source/ui/vba/vbadialogs
    sw/source/ui/vba/vbapalette
    sw/source/ui/vba/vbaparagraph
    sw/source/ui/vba/vbafind
    sw/source/ui/vba/vbasection
    sw/source/ui/vba/vbabookmark
    sw/source/ui/vba/vbaapplication
    sw/source/ui/vba/vbawindow
    sw/source/ui/vba/vbareplacement
    sw/source/ui/vba/vbatable
    sw/source/ui/vba/vbatablehelper
    sw/source/ui/vba/vbatabstop
    sw/source/ui/vba/vbatabstops
    sw/source/ui/vba/vbaselection
    sw/source/ui/vba/vbasystem
    sw/source/ui/vba/vbainformationhelper
    sw/source/ui/vba/vbapagesetup
    sw/source/ui/vba/vbafield
    sw/source/ui/vba/vbatables
    sw/source/ui/vba/vbatableofcontents
    sw/source/ui/vba/vbatablesofcontents
    sw/source/ui/vba/vbavariable
    sw/source/ui/vba/vbawordbasic
    sw/source/ui/vba/vbadocuments
    sw/source/ui/vba/vbaparagraphformat
    sw/source/ui/vba/vbaborders
    sw/source/ui/vba/vbavariables
    sw/source/ui/vba/vbastyles
    sw/source/ui/vba/vbapanes
    sw/source/ui/vba/wordvbahelper
)

librestudio_expand_cxx(VBASWOBJ_SOURCES)
add_library(vbaswobj SHARED ${VBASWOBJ_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(vbaswobj PRIVATE
        "${CMAKE_SOURCE_DIR}/sw/inc/pch/precompiled_vbaswobj.hxx")
    target_compile_definitions(vbaswobj PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(vbaswobj PRIVATE
    "${CMAKE_SOURCE_DIR}/sw/source/uibase/inc"
    "${CMAKE_SOURCE_DIR}/sw/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}"
    "${oovbaapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(vbaswobj
    udkapi_normal_headers offapi_normal_headers oovbaapi_normal_headers
    officecfg_headers)

target_link_libraries(vbaswobj PRIVATE
    comphelper cppu cppuhelper editeng i18nlangtag sal sb sfx svl svt svx
    svxcore sw tk tl utl vbahelper vcl
    Boost::boost
    LibXml2::LibXml2)

librestudio_add_component(vbaswobj
    COMPONENT "${CMAKE_SOURCE_DIR}/sw/util/vbaswobj.component"
    LIBRARY   vbaswobj
    LAYER     OOO)

librestudio_set_library_output_name(vbaswobj vbaswobj OOO)

install(TARGETS vbaswobj LIBRARY DESTINATION program)

endif () # ENABLE_SCRIPTING

# ---------------------------------------------------------------------------
# swd -- Library_swd.mk
# ---------------------------------------------------------------------------

set(SWD_SOURCES
    sw/source/ui/uno/swdetect
)

librestudio_expand_cxx(SWD_SOURCES)
add_library(swd SHARED ${SWD_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(swd REUSE_FROM precompiled_system)
endif ()

target_include_directories(swd PRIVATE
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(swd udkapi_normal_headers offapi_normal_headers)

target_link_libraries(swd PRIVATE
    comphelper cppu cppuhelper sal sfx sot tl utl
    Boost::boost)

librestudio_add_component(swd
    COMPONENT "${CMAKE_SOURCE_DIR}/sw/util/swd.component"
    LIBRARY   swd
    LAYER     OOO)

librestudio_set_library_output_name(swd swd OOO)

install(TARGETS swd LIBRARY DESTINATION program)

# --- UIConfig_swriter (registers internally as "modules/swriter") ---------
#
# sw/UIConfig_swriter.mk: gb_UIConfig_UIConfig(modules/swriter) -- 309 files
# across 5 categories (2 menubarfiles, 17 popupmenufiles, 1 statusbarfiles, 57
# toolbarfiles, 232 uifiles), unconditional (no ifeq/ifneq gates in the live
# .mk -- verified by reading it in full), packaged into share/config/
# soffice.cfg/modules/swriter/{menubar,popupmenu,statusbar,toolbar,ui} --
# gb_UIConfig_DIR_FOR's own destination convention, same multi-category shape
# UIConfig_basicide/UIConfig_svt/UIConfig_vcl already established (menubar/
# popupmenu/statusbar/toolbar use .xml, ui uses .ui). All 309 files verified
# present on disk. CppunitTest_desktop_lib.mk's own use_uiconfigs(desktop_lib,
# cui "modules/swriter") call needs a real build-time ALL target for this second
# module too -- introduced here as "swriter_uiconfig" (this port's own bare-name
# convention for these targets, not the gbuild "modules/swriter" path).
set(SW_UICONFIG_MENUBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/menubar/menubar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/menubar/mscompatibleformsmenu.xml")

set(SW_UICONFIG_POPUPMENU_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/anchor.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/annotation.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/draw.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/drawtext.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/form.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/formrichtext.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/frame.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/graphic.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/insertfield.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/media.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/notebookbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/oleobject.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/preview.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/table.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/text.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/showtrackedchanges.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/recordtrackedchanges.xml")

set(SW_UICONFIG_STATUSBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/statusbar/statusbar.xml")

set(SW_UICONFIG_TOOLBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/alignmentbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/connectorsbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/arrowsbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/arrowshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/basicshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/bezierobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/calloutshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/changes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/classificationbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/colorbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/drawbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/drawingobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/drawtextobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/extrusionobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/findbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/flowchartshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/fontworkobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/fontworkshapetype.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/formcontrols.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/formdesign.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/formsfilterbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/formsnavigationbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/formtextobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/frameobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/fullscreenbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/graffilterbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/graphicobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/insertbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/linesbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/mailmerge.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/mediaobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/navigationobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/notebookbarshortcuts.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/numobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/oleobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/optimizetablebar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/previewobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/singlemode.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/singlemode-annotation.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/singlemode-draw.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/singlemode-drawtext.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/singlemode-form.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/singlemode-frame.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/singlemode-graphic.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/singlemode-media.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/singlemode-ole.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/singlemode-printpreview.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/singlemode-table.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/singlemode-text.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/standardbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/starshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/symbolshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/tableobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/textobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/textstylebar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/toolbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/toolbar/viewerbar.xml")

set(SW_UICONFIG_UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/abstractdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/addentrydialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/addressblockdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/alreadyexistsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/addressfragment.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/annotation.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/asciifilterdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/asksearchdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/assignfieldsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/assignfragment.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/assignstylesdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/attachnamedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/authenticationsettingsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/autoformattable.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/autotext.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/bibliographyentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/bibliofragment.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/bookmarkmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/bulletsandnumbering.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/businessdatapage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/calendar.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/cannotsavelabeldialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/captiondialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/captionoptions.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/cardmediumpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/ccdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/characterproperties.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/checkbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/columndialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/columnpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/columnwidth.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/combobox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/comboboxfragment.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/conditionpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/contentcontrolaliasbutton.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/contentcontrolcalendar.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/contentcontroldlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/contentcontroldropdown.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/contentcontrollistitemdlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/converttexttable.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/copyfielddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/createaddresslist.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/createauthorentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/createautomarkdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/customizeaddrlistdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/datasourcesunavailabledialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/dateformfielddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/dropcapspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/dropdownfielddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/dropdownformfielddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/editbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/editcategories.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/editfielddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/editsectiondialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/endnotepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/envaddresspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/envdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/envformatpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/envprinterpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/exchangedatabases.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/fielddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/findentrydialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/flddbpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/flddocinfopage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/flddocumentpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/fldfuncpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/fldrefpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/fldvarpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/floatingsync.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/formatsectiondialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/formattablepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/formdropdown.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/footendnotedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/footnotepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/footnoteareapage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/footnotesendnotestabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/framedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/frmaddpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/frmtypepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/frmurlpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/hfmenubutton.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/indexentry.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/inputfielddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/inputwinmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/indentpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/infonotfounddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/inforeadonlydialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/inputeditbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/insertautotextdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/insertbookmark.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/insertbreak.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/insertcaption.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/insertdbcolumnsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/insertfootnote.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/insertsectiondialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/insertscript.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/inserttable.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/jumpposbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/labeldialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/labelformatpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/labeloptionspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/linenumbering.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/mailconfigpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/mailmergedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/mergeconnectdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/mergetabledialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/newuserindexdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/numparapage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/numberingnamedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/numberinput.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/objectdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/optcomparison.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/optcompatpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/optcaptionpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/optfonttabpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/optformataidspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/optgeneralpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/optredlinepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/opttablepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/opttestpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/outlinebutton.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/outlinenumbering.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/outlinenumberingpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/outlinepositionpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/pbmenubutton.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/querycontinuebegindialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/querycontinueenddialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/querydefaultcompatdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/querysavelabeldialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/mailmerge.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/managechangessidebar.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/mastercontextmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/mmaddressblockpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/mmcreatingdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/mmlayoutpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/mmmailbody.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/mmoutputtypepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/mmresultemaildialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/mmresultprintdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/mmresultsavedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/mmselectpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/mmsendmails.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/mmsalutationpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/navigatorcontextmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/navigatordraginsertmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/navigatorpanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/notebookbar.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/notebookbar_compact.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/notebookbar_single.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/notebookbar_groups.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/notebookbar_groupedbar_full.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/notebookbar_groupedbar_compact.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/pagecolumncontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/pagemargincontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/pagenumberdlg.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/pageorientationcontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/pagesizecontrol.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/pagestylemenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/paradialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/picturedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/picturepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/previewmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/previewzoomdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/printeroptions.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/printmergedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/printmonitordialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/printoptionspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/privateuserpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/readonlymenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/renameautotextdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/renameentrydialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/renameobjectdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/rowheight.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/saveashtmldialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/savelabeldialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/savemonitordialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/sectionpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/selectaddressdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/selectautotextdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/selectblockdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/selectindexdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/selecttabledialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/pageformatpanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/pagestylespanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/pageheaderpanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/pagefooterpanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/commentspanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/commentsthread.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/commentwidget.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/commentcontextmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/a11ycheckissuespanel.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/poseditbox.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/sidebarwrap.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/sidebarstylepresets.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/sidebartableedit.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/sidebartheme.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/sidebarquickfind.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/sortdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/spellmenu.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/splittable.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/statisticsinfopage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/stringinput.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/subjectdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/testmailsettings.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/tocdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/tocentriespage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/tocindexpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/tocstylespage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/tablecolumnpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/tablepreviewdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/tableproperties.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/tabletextflowpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/templatedialog1.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/templatedialog2.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/templatedialog4.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/templatedialog8.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/templatedialog16.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/textgridpage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/titlepage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/tokenwidget.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/toxentrywidget.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/toxbuttonwidget.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/unfloatbutton.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/viewoptionspage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/warndatasourcedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/warnemaildialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/warnhiddensectiondialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/watermarkdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/wordcount.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/wrapdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/wrappage.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/ui/zoombox.ui")

install(FILES ${SW_UICONFIG_MENUBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/swriter/menubar)
install(FILES ${SW_UICONFIG_POPUPMENU_FILES}
    DESTINATION share/config/soffice.cfg/modules/swriter/popupmenu)
install(FILES ${SW_UICONFIG_STATUSBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/swriter/statusbar)
install(FILES ${SW_UICONFIG_TOOLBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/swriter/toolbar)
install(FILES ${SW_UICONFIG_UI_FILES}
    DESTINATION share/config/soffice.cfg/modules/swriter/ui)

set(_sw_uiconfig_outputs "")
foreach (_sw_uiconfig_pair
        "SW_UICONFIG_MENUBAR_FILES:menubar"
        "SW_UICONFIG_POPUPMENU_FILES:popupmenu"
        "SW_UICONFIG_STATUSBAR_FILES:statusbar"
        "SW_UICONFIG_TOOLBAR_FILES:toolbar"
        "SW_UICONFIG_UI_FILES:ui")
    string(REPLACE ":" ";" _sw_uiconfig_pair "${_sw_uiconfig_pair}")
    list(GET _sw_uiconfig_pair 0 _sw_uiconfig_var)
    list(GET _sw_uiconfig_pair 1 _sw_uiconfig_category)
    foreach (_sw_uiconfig_src IN LISTS ${_sw_uiconfig_var})
        get_filename_component(_sw_uiconfig_name "${_sw_uiconfig_src}" NAME)
        set(_sw_uiconfig_dest
            "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/swriter/${_sw_uiconfig_category}/${_sw_uiconfig_name}")
        add_custom_command(
            OUTPUT "${_sw_uiconfig_dest}"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/swriter/${_sw_uiconfig_category}"
            COMMAND ${CMAKE_COMMAND} -E copy "${_sw_uiconfig_src}" "${_sw_uiconfig_dest}"
            DEPENDS "${_sw_uiconfig_src}"
            COMMENT "Copying ${_sw_uiconfig_name} into build-tree soffice.cfg/modules/swriter/${_sw_uiconfig_category}")
        list(APPEND _sw_uiconfig_outputs "${_sw_uiconfig_dest}")
    endforeach ()
endforeach ()
unset(_sw_uiconfig_pair)
unset(_sw_uiconfig_var)
unset(_sw_uiconfig_category)
unset(_sw_uiconfig_src)
unset(_sw_uiconfig_name)
unset(_sw_uiconfig_dest)

add_custom_target(swriter_uiconfig ALL DEPENDS ${_sw_uiconfig_outputs})
unset(_sw_uiconfig_outputs)

# --- UIConfig_sglobal (gb_UIConfig_UIConfig(modules/sglobal)) -----------------------
#
# UIConfig_sglobal.mk, re-checked directly, every file verified present on
# disk before writing this block. Same shape/pattern as simpress_uiconfig/
# sdraw_uiconfig/scalc_uiconfig above (different module).
# NOTE: sglobal's own popupmenu list names two DIFFERENT source paths that
# both resolve to the SAME destination filename (both named
# "showtrackedchanges.xml" once install()ed) -- confirmed the two source
# files are byte-identical (diff, exit 0), so which one "wins" is moot;
# deduped here, keeping the later entry (matches sequential-copy semantics).
set(SW_SGLOBAL_UICONFIG_MENUBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/menubar/menubar.xml"
)

set(SW_SGLOBAL_UICONFIG_POPUPMENU_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/popupmenu/anchor.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/popupmenu/annotation.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/popupmenu/draw.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/popupmenu/drawtext.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/popupmenu/form.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/popupmenu/formrichtext.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/popupmenu/frame.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/popupmenu/graphic.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/popupmenu/insertfield.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/showtrackedchanges.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/popupmenu/media.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/popupmenu/oleobject.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/popupmenu/preview.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/popupmenu/table.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/popupmenu/text.xml"
)

set(SW_SGLOBAL_UICONFIG_STATUSBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/statusbar/statusbar.xml"
)

set(SW_SGLOBAL_UICONFIG_TOOLBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/alignmentbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/arrowshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/basicshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/bezierobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/calloutshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/colorbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/drawbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/drawingobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/drawtextobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/extrusionobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/findbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/flowchartshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/fontworkobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/fontworkshapetype.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/formcontrols.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/formdesign.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/formsfilterbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/formsnavigationbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/formtextobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/frameobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/fullscreenbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/graffilterbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/graphicobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/insertbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/insertobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/mediaobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/numobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/oleobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/optimizetablebar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/previewobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/standardbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/starshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/symbolshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/tableobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/textobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/textstylebar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/toolbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sglobal/toolbar/viewerbar.xml"
)

install(FILES ${SW_SGLOBAL_UICONFIG_MENUBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/sglobal/menubar)
install(FILES ${SW_SGLOBAL_UICONFIG_POPUPMENU_FILES}
    DESTINATION share/config/soffice.cfg/modules/sglobal/popupmenu)
install(FILES ${SW_SGLOBAL_UICONFIG_STATUSBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/sglobal/statusbar)
install(FILES ${SW_SGLOBAL_UICONFIG_TOOLBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/sglobal/toolbar)

set(_sglobal_uiconfig_outputs "")
foreach (_sglobal_uiconfig_pair
        "SW_SGLOBAL_UICONFIG_MENUBAR_FILES:menubar"
        "SW_SGLOBAL_UICONFIG_POPUPMENU_FILES:popupmenu"
        "SW_SGLOBAL_UICONFIG_STATUSBAR_FILES:statusbar"
        "SW_SGLOBAL_UICONFIG_TOOLBAR_FILES:toolbar")
    string(REPLACE ":" ";" _sglobal_uiconfig_pair "${_sglobal_uiconfig_pair}")
    list(GET _sglobal_uiconfig_pair 0 _sglobal_uiconfig_var)
    list(GET _sglobal_uiconfig_pair 1 _sglobal_uiconfig_category)
    foreach (_sglobal_uiconfig_src IN LISTS ${_sglobal_uiconfig_var})
        get_filename_component(_sglobal_uiconfig_name "${_sglobal_uiconfig_src}" NAME)
        set(_sglobal_uiconfig_dest
            "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/sglobal/${_sglobal_uiconfig_category}/${_sglobal_uiconfig_name}")
        add_custom_command(
            OUTPUT "${_sglobal_uiconfig_dest}"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/sglobal/${_sglobal_uiconfig_category}"
            COMMAND ${CMAKE_COMMAND} -E copy "${_sglobal_uiconfig_src}" "${_sglobal_uiconfig_dest}"
            DEPENDS "${_sglobal_uiconfig_src}"
            COMMENT "Copying ${_sglobal_uiconfig_name} into build-tree soffice.cfg/modules/sglobal/${_sglobal_uiconfig_category}")
        list(APPEND _sglobal_uiconfig_outputs "${_sglobal_uiconfig_dest}")
    endforeach ()
endforeach ()
unset(_sglobal_uiconfig_pair)
unset(_sglobal_uiconfig_var)
unset(_sglobal_uiconfig_category)
unset(_sglobal_uiconfig_src)
unset(_sglobal_uiconfig_name)
unset(_sglobal_uiconfig_dest)

add_custom_target(sglobal_uiconfig ALL DEPENDS ${_sglobal_uiconfig_outputs})
unset(_sglobal_uiconfig_outputs)

# --- UIConfig_sweb (gb_UIConfig_UIConfig(modules/sweb)) -----------------------
#
# UIConfig_sweb.mk, re-checked directly, every file verified present on
# disk before writing this block. Same shape/pattern as simpress_uiconfig/
# sdraw_uiconfig/scalc_uiconfig above (different module).
set(SW_SWEB_UICONFIG_MENUBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/menubar/menubar.xml"
)

set(SW_SWEB_UICONFIG_POPUPMENU_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/popupmenu/anchor.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/popupmenu/annotation.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/popupmenu/form.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/popupmenu/formrichtext.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/popupmenu/frame.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/popupmenu/graphic.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/popupmenu/insertfield.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/popupmenu/oleobject.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/popupmenu/preview.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/popupmenu/source.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/popupmenu/table.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/popupmenu/text.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/showtrackedchanges.xml"
)

set(SW_SWEB_UICONFIG_STATUSBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/statusbar/statusbar.xml"
)

set(SW_SWEB_UICONFIG_TOOLBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/arrowshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/basicshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/bezierobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/calloutshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/colorbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/drawingobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/drawtextobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/extrusionobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/findbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/flowchartshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/fontworkobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/fontworkshapetype.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/formcontrols.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/formdesign.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/formsfilterbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/formsnavigationbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/formtextobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/frameobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/fullscreenbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/graffilterbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/graphicobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/insertbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/mediaobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/numobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/oleobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/optimizetablebar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/previewobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/standardbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/starshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/symbolshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/tableobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/textobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/toolbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/sweb/toolbar/viewerbar.xml"
)

install(FILES ${SW_SWEB_UICONFIG_MENUBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/sweb/menubar)
install(FILES ${SW_SWEB_UICONFIG_POPUPMENU_FILES}
    DESTINATION share/config/soffice.cfg/modules/sweb/popupmenu)
install(FILES ${SW_SWEB_UICONFIG_STATUSBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/sweb/statusbar)
install(FILES ${SW_SWEB_UICONFIG_TOOLBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/sweb/toolbar)

set(_sweb_uiconfig_outputs "")
foreach (_sweb_uiconfig_pair
        "SW_SWEB_UICONFIG_MENUBAR_FILES:menubar"
        "SW_SWEB_UICONFIG_POPUPMENU_FILES:popupmenu"
        "SW_SWEB_UICONFIG_STATUSBAR_FILES:statusbar"
        "SW_SWEB_UICONFIG_TOOLBAR_FILES:toolbar")
    string(REPLACE ":" ";" _sweb_uiconfig_pair "${_sweb_uiconfig_pair}")
    list(GET _sweb_uiconfig_pair 0 _sweb_uiconfig_var)
    list(GET _sweb_uiconfig_pair 1 _sweb_uiconfig_category)
    foreach (_sweb_uiconfig_src IN LISTS ${_sweb_uiconfig_var})
        get_filename_component(_sweb_uiconfig_name "${_sweb_uiconfig_src}" NAME)
        set(_sweb_uiconfig_dest
            "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/sweb/${_sweb_uiconfig_category}/${_sweb_uiconfig_name}")
        add_custom_command(
            OUTPUT "${_sweb_uiconfig_dest}"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/sweb/${_sweb_uiconfig_category}"
            COMMAND ${CMAKE_COMMAND} -E copy "${_sweb_uiconfig_src}" "${_sweb_uiconfig_dest}"
            DEPENDS "${_sweb_uiconfig_src}"
            COMMENT "Copying ${_sweb_uiconfig_name} into build-tree soffice.cfg/modules/sweb/${_sweb_uiconfig_category}")
        list(APPEND _sweb_uiconfig_outputs "${_sweb_uiconfig_dest}")
    endforeach ()
endforeach ()
unset(_sweb_uiconfig_pair)
unset(_sweb_uiconfig_var)
unset(_sweb_uiconfig_category)
unset(_sweb_uiconfig_src)
unset(_sweb_uiconfig_name)
unset(_sweb_uiconfig_dest)

add_custom_target(sweb_uiconfig ALL DEPENDS ${_sweb_uiconfig_outputs})
unset(_sweb_uiconfig_outputs)

# --- UIConfig_swform (gb_UIConfig_UIConfig(modules/swform)) -----------------------
#
# UIConfig_swform.mk, re-checked directly, every file verified present on
# disk before writing this block. Same shape/pattern as simpress_uiconfig/
# sdraw_uiconfig/scalc_uiconfig above (different module).
set(SW_SWFORM_UICONFIG_MENUBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/menubar/menubar.xml"
)

set(SW_SWFORM_UICONFIG_POPUPMENU_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/popupmenu/anchor.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/popupmenu/annotation.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/popupmenu/draw.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/popupmenu/drawtext.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/popupmenu/form.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/popupmenu/formrichtext.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/popupmenu/frame.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/popupmenu/graphic.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/popupmenu/insertfield.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/popupmenu/media.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/popupmenu/oleobject.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/popupmenu/preview.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/popupmenu/table.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/popupmenu/text.xml"
)

set(SW_SWFORM_UICONFIG_STATUSBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/statusbar/statusbar.xml"
)

set(SW_SWFORM_UICONFIG_TOOLBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/alignmentbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/arrowshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/basicshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/bezierobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/calloutshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/colorbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/drawbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/drawingobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/drawtextobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/extrusionobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/flowchartshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/fontworkobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/fontworkshapetype.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/formcontrols.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/formdesign.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/formsfilterbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/formsnavigationbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/formtextobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/frameobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/fullscreenbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/graffilterbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/graphicobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/insertbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/insertobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/mailmerge.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/mediaobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/numobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/oleobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/optimizetablebar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/previewobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/standardbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/starshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/symbolshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/tableobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/textobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/toolbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swform/toolbar/viewerbar.xml"
)

install(FILES ${SW_SWFORM_UICONFIG_MENUBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/swform/menubar)
install(FILES ${SW_SWFORM_UICONFIG_POPUPMENU_FILES}
    DESTINATION share/config/soffice.cfg/modules/swform/popupmenu)
install(FILES ${SW_SWFORM_UICONFIG_STATUSBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/swform/statusbar)
install(FILES ${SW_SWFORM_UICONFIG_TOOLBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/swform/toolbar)

set(_swform_uiconfig_outputs "")
foreach (_swform_uiconfig_pair
        "SW_SWFORM_UICONFIG_MENUBAR_FILES:menubar"
        "SW_SWFORM_UICONFIG_POPUPMENU_FILES:popupmenu"
        "SW_SWFORM_UICONFIG_STATUSBAR_FILES:statusbar"
        "SW_SWFORM_UICONFIG_TOOLBAR_FILES:toolbar")
    string(REPLACE ":" ";" _swform_uiconfig_pair "${_swform_uiconfig_pair}")
    list(GET _swform_uiconfig_pair 0 _swform_uiconfig_var)
    list(GET _swform_uiconfig_pair 1 _swform_uiconfig_category)
    foreach (_swform_uiconfig_src IN LISTS ${_swform_uiconfig_var})
        get_filename_component(_swform_uiconfig_name "${_swform_uiconfig_src}" NAME)
        set(_swform_uiconfig_dest
            "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/swform/${_swform_uiconfig_category}/${_swform_uiconfig_name}")
        add_custom_command(
            OUTPUT "${_swform_uiconfig_dest}"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/swform/${_swform_uiconfig_category}"
            COMMAND ${CMAKE_COMMAND} -E copy "${_swform_uiconfig_src}" "${_swform_uiconfig_dest}"
            DEPENDS "${_swform_uiconfig_src}"
            COMMENT "Copying ${_swform_uiconfig_name} into build-tree soffice.cfg/modules/swform/${_swform_uiconfig_category}")
        list(APPEND _swform_uiconfig_outputs "${_swform_uiconfig_dest}")
    endforeach ()
endforeach ()
unset(_swform_uiconfig_pair)
unset(_swform_uiconfig_var)
unset(_swform_uiconfig_category)
unset(_swform_uiconfig_src)
unset(_swform_uiconfig_name)
unset(_swform_uiconfig_dest)

add_custom_target(swform_uiconfig ALL DEPENDS ${_swform_uiconfig_outputs})
unset(_swform_uiconfig_outputs)

# --- UIConfig_swreport (gb_UIConfig_UIConfig(modules/swreport)) -----------------------
#
# UIConfig_swreport.mk, re-checked directly, every file verified present on
# disk before writing this block. Same shape/pattern as simpress_uiconfig/
# sdraw_uiconfig/scalc_uiconfig above (different module).
set(SW_SWREPORT_UICONFIG_MENUBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/menubar/menubar.xml"
)

set(SW_SWREPORT_UICONFIG_POPUPMENU_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/popupmenu/anchor.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/popupmenu/annotation.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/popupmenu/draw.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/popupmenu/drawtext.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/popupmenu/form.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/popupmenu/formrichtext.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/popupmenu/frame.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/popupmenu/graphic.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/popupmenu/insertfield.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/popupmenu/media.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/popupmenu/oleobject.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/popupmenu/preview.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/popupmenu/table.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/popupmenu/text.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/showtrackedchanges.xml"
)

set(SW_SWREPORT_UICONFIG_STATUSBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/statusbar/statusbar.xml"
)

set(SW_SWREPORT_UICONFIG_TOOLBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/alignmentbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/arrowshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/basicshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/bezierobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/calloutshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/colorbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/drawbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/drawingobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/drawtextobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/extrusionobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/flowchartshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/fontworkobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/fontworkshapetype.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/formcontrols.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/formdesign.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/formsfilterbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/formsnavigationbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/formtextobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/frameobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/fullscreenbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/graffilterbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/graphicobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/insertbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/insertobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/mailmerge.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/mediaobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/numobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/oleobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/optimizetablebar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/previewobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/standardbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/starshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/symbolshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/tableobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/textobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/toolbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swreport/toolbar/viewerbar.xml"
)

install(FILES ${SW_SWREPORT_UICONFIG_MENUBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/swreport/menubar)
install(FILES ${SW_SWREPORT_UICONFIG_POPUPMENU_FILES}
    DESTINATION share/config/soffice.cfg/modules/swreport/popupmenu)
install(FILES ${SW_SWREPORT_UICONFIG_STATUSBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/swreport/statusbar)
install(FILES ${SW_SWREPORT_UICONFIG_TOOLBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/swreport/toolbar)

set(_swreport_uiconfig_outputs "")
foreach (_swreport_uiconfig_pair
        "SW_SWREPORT_UICONFIG_MENUBAR_FILES:menubar"
        "SW_SWREPORT_UICONFIG_POPUPMENU_FILES:popupmenu"
        "SW_SWREPORT_UICONFIG_STATUSBAR_FILES:statusbar"
        "SW_SWREPORT_UICONFIG_TOOLBAR_FILES:toolbar")
    string(REPLACE ":" ";" _swreport_uiconfig_pair "${_swreport_uiconfig_pair}")
    list(GET _swreport_uiconfig_pair 0 _swreport_uiconfig_var)
    list(GET _swreport_uiconfig_pair 1 _swreport_uiconfig_category)
    foreach (_swreport_uiconfig_src IN LISTS ${_swreport_uiconfig_var})
        get_filename_component(_swreport_uiconfig_name "${_swreport_uiconfig_src}" NAME)
        set(_swreport_uiconfig_dest
            "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/swreport/${_swreport_uiconfig_category}/${_swreport_uiconfig_name}")
        add_custom_command(
            OUTPUT "${_swreport_uiconfig_dest}"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/swreport/${_swreport_uiconfig_category}"
            COMMAND ${CMAKE_COMMAND} -E copy "${_swreport_uiconfig_src}" "${_swreport_uiconfig_dest}"
            DEPENDS "${_swreport_uiconfig_src}"
            COMMENT "Copying ${_swreport_uiconfig_name} into build-tree soffice.cfg/modules/swreport/${_swreport_uiconfig_category}")
        list(APPEND _swreport_uiconfig_outputs "${_swreport_uiconfig_dest}")
    endforeach ()
endforeach ()
unset(_swreport_uiconfig_pair)
unset(_swreport_uiconfig_var)
unset(_swreport_uiconfig_category)
unset(_swreport_uiconfig_src)
unset(_swreport_uiconfig_name)
unset(_swreport_uiconfig_dest)

add_custom_target(swreport_uiconfig ALL DEPENDS ${_swreport_uiconfig_outputs})
unset(_swreport_uiconfig_outputs)

# --- UIConfig_swxform (gb_UIConfig_UIConfig(modules/swxform)) -----------------------
#
# UIConfig_swxform.mk, re-checked directly, every file verified present on
# disk before writing this block. Same shape/pattern as simpress_uiconfig/
# sdraw_uiconfig/scalc_uiconfig above (different module).
set(SW_SWXFORM_UICONFIG_MENUBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/menubar/menubar.xml"
)

set(SW_SWXFORM_UICONFIG_POPUPMENU_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/popupmenu/anchor.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/popupmenu/annotation.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/popupmenu/draw.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/popupmenu/drawtext.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/popupmenu/form.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/popupmenu/formrichtext.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/popupmenu/frame.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/popupmenu/graphic.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/popupmenu/insertfield.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/popupmenu/media.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/popupmenu/oleobject.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/popupmenu/preview.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/popupmenu/table.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/popupmenu/text.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swriter/popupmenu/showtrackedchanges.xml"
)

set(SW_SWXFORM_UICONFIG_STATUSBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/statusbar/statusbar.xml"
)

set(SW_SWXFORM_UICONFIG_TOOLBAR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/alignmentbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/arrowshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/basicshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/bezierobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/calloutshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/colorbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/drawbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/drawingobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/drawtextobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/extrusionobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/findbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/flowchartshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/fontworkobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/fontworkshapetype.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/formcontrols.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/formdesign.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/formsfilterbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/formsnavigationbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/formtextobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/frameobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/fullscreenbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/graffilterbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/graphicobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/insertbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/insertobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/mailmerge.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/mediaobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/numobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/oleobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/optimizetablebar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/previewobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/standardbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/starshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/symbolshapes.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/tableobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/textobjectbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/toolbar.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/uiconfig/swxform/toolbar/viewerbar.xml"
)

install(FILES ${SW_SWXFORM_UICONFIG_MENUBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/swxform/menubar)
install(FILES ${SW_SWXFORM_UICONFIG_POPUPMENU_FILES}
    DESTINATION share/config/soffice.cfg/modules/swxform/popupmenu)
install(FILES ${SW_SWXFORM_UICONFIG_STATUSBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/swxform/statusbar)
install(FILES ${SW_SWXFORM_UICONFIG_TOOLBAR_FILES}
    DESTINATION share/config/soffice.cfg/modules/swxform/toolbar)

set(_swxform_uiconfig_outputs "")
foreach (_swxform_uiconfig_pair
        "SW_SWXFORM_UICONFIG_MENUBAR_FILES:menubar"
        "SW_SWXFORM_UICONFIG_POPUPMENU_FILES:popupmenu"
        "SW_SWXFORM_UICONFIG_STATUSBAR_FILES:statusbar"
        "SW_SWXFORM_UICONFIG_TOOLBAR_FILES:toolbar")
    string(REPLACE ":" ";" _swxform_uiconfig_pair "${_swxform_uiconfig_pair}")
    list(GET _swxform_uiconfig_pair 0 _swxform_uiconfig_var)
    list(GET _swxform_uiconfig_pair 1 _swxform_uiconfig_category)
    foreach (_swxform_uiconfig_src IN LISTS ${_swxform_uiconfig_var})
        get_filename_component(_swxform_uiconfig_name "${_swxform_uiconfig_src}" NAME)
        set(_swxform_uiconfig_dest
            "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/swxform/${_swxform_uiconfig_category}/${_swxform_uiconfig_name}")
        add_custom_command(
            OUTPUT "${_swxform_uiconfig_dest}"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${LIBRESTUDIO_SHARE_DIR}/config/soffice.cfg/modules/swxform/${_swxform_uiconfig_category}"
            COMMAND ${CMAKE_COMMAND} -E copy "${_swxform_uiconfig_src}" "${_swxform_uiconfig_dest}"
            DEPENDS "${_swxform_uiconfig_src}"
            COMMENT "Copying ${_swxform_uiconfig_name} into build-tree soffice.cfg/modules/swxform/${_swxform_uiconfig_category}")
        list(APPEND _swxform_uiconfig_outputs "${_swxform_uiconfig_dest}")
    endforeach ()
endforeach ()
unset(_swxform_uiconfig_pair)
unset(_swxform_uiconfig_var)
unset(_swxform_uiconfig_category)
unset(_swxform_uiconfig_src)
unset(_swxform_uiconfig_name)
unset(_swxform_uiconfig_dest)

add_custom_target(swxform_uiconfig ALL DEPENDS ${_swxform_uiconfig_outputs})
unset(_swxform_uiconfig_outputs)

# --- CppunitTests ------------------------------------------------------------
#
# 175 CppunitTest_*.mk files -- split out to sw/CppunitTest_deferred.cmake
# (deferred: needs program_services_rdb, only set much later in the root
# CMakeLists.txt, same ordering constraint as every other module's own
# CppunitTest_deferred.cmake). See that file's own header comment for the
# real structure (6 shared gbuild macros covering most of the 175, ported
# incrementally wave by wave rather than 175 independent hand
# transcriptions) and current progress.
librestudio_include_deferred("${CMAKE_SOURCE_DIR}/sw/CppunitTest_deferred.cmake")
