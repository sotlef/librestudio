# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# i18npool — the internationalisation services: character classification,
# collation, break iteration, calendars, transliteration, and the search
# engine in i18nsearch.
#
# This module is the first where the build has to *generate* real code rather
# than just compile it: libi18npool is built in part from tables that tools in
# this same module print out while the build runs.  There are five of them, and
# they are exactly the CustomTargets that Module_i18npool.mk declares:
#
#   localedata      saxparser reads one XML description per locale under
#                   source/localedata/data and writes a .cxx for each.
#   collator        gencoll_rule compiles each source/collator/data/*.txt into
#                   a collator_*.cxx, and writes lrl_include.hxx beside them
#                   for collator_unicode.cxx to include.
#   indexentry      genindex_data turns the index data tables into .cxx.
#   textconversion  genconv_dict turns the *.dic dictionaries into .cxx.
#   breakiterator   no tool of this module: ICU's genbrk compiles
#                   source/breakiterator/data/*.txt into resource bundles,
#                   genccode wraps each in a C array, and gencmn writes the
#                   table of contents that breakiterator_unicode.cxx hands to
#                   udata_setAppData as the "OpenOffice" data package.
#
# They all have the same shape — a generator run during the build, a glob over
# a data directory, one custom command per input — and the generated files are
# ordinary translation units of the library.

# each locale under source/localedata/data is a description
# in XML, and saxparser turns it into a .cxx that the localedata_* libraries
# compile in.  Upstream does the same through CustomTarget_localedata.mk.
#
# The locale data is what makes com.sun.star.i18n services answer for a
# locale at all; without it the storage reader cannot even upper-case the
# name of an OLE2 entry.

find_package(BISON REQUIRED)  # already found at the top level, kept local for clarity

# --- saxparser: a build tool, not part of the installation -------------------

add_executable(saxparser
    "${CMAKE_SOURCE_DIR}/i18npool/source/localedata/LocaleNode.cxx"
    "${CMAKE_SOURCE_DIR}/i18npool/source/localedata/filewriter.cxx"
    "${CMAKE_SOURCE_DIR}/i18npool/source/localedata/saxparser.cxx")

target_include_directories(saxparser PRIVATE
    "${CMAKE_SOURCE_DIR}/i18npool/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
# gcc3_uno is not linked, it is dlopen()ed when the UNO environment is first
# asked for — see the note in bridges/CMakeLists.txt.  It has to be built before
# this program runs, and this program runs in the middle of the build.
add_dependencies(saxparser udkapi_normal_headers offapi_normal_headers gcc3_uno)
target_link_libraries(saxparser PRIVATE cppu cppuhelper sal)

# --- locale data -------------------------------------------------------------

# One .cxx per locale description.  saxparser parses the XML through the expat
# wrapper in libsaxlo, which it finds through unorc and services.rdb like any
# other component — upstream points URE_MORE_SERVICES at a private registry
# holding just expwrap, but the services we already install serve the same
# purpose.
set(I18NPOOL_LOCALEDATA_DIR "${CMAKE_CURRENT_BINARY_DIR}/localedata")
file(MAKE_DIRECTORY "${I18NPOOL_LOCALEDATA_DIR}")

set(I18NPOOL_LOCALEDATA_SOURCES)
file(GLOB I18NPOOL_LOCALEDATA_XML RELATIVE
    "${CMAKE_SOURCE_DIR}/i18npool/source/localedata/data"
    "${CMAKE_SOURCE_DIR}/i18npool/source/localedata/data/*.xml")
foreach (_xml IN LISTS I18NPOOL_LOCALEDATA_XML)
    get_filename_component(_name "${_xml}" NAME_WE)
    set(_out "${I18NPOOL_LOCALEDATA_DIR}/localedata_${_name}.cxx")
    add_custom_command(
        OUTPUT "${_out}"
        COMMAND saxparser "${_name}"
                "${CMAKE_SOURCE_DIR}/i18npool/source/localedata/data/${_xml}"
                "${_out}"
        # program_services_rdb/program_types_rdb/program_offapi_rdb/
        # program_oovbaapi_rdb/sax used to be wired on ONLY via the
        # add_dependencies(i18npool_localedata ...) call below, which does
        # NOT work: add_dependencies() on the aggregate add_custom_target()
        # only orders that phony node itself against these targets -- it
        # does not reach back into the per-file add_custom_command() edges
        # that are the phony target's own DEPENDS, since those are separate,
        # sibling ninja build edges with no edge between them. Confirmed live
        # on the remote test build: ninja ran `saxparser en_US ...` before
        # program_oovbaapi_rdb had installed program/types/oovbaapi.rdb,
        # saxparser's own cppuhelper::bootstrap() failed with "file:...
        # oovbaapi.rdb: no such file" for the large majority of locales (a
        # real race, not deterministic per-locale -- whichever ninja happened
        # to schedule first). Listing the real runtime prerequisites directly
        # on THIS custom command is what actually orders them.
        DEPENDS saxparser "${CMAKE_SOURCE_DIR}/i18npool/source/localedata/data/${_xml}"
                program_services_rdb program_types_rdb program_offapi_rdb
                program_oovbaapi_rdb sax
        COMMENT "saxparser: localedata_${_name}.cxx"
        VERBATIM)
    list(APPEND I18NPOOL_LOCALEDATA_SOURCES "${_out}")
endforeach ()
add_custom_target(i18npool_localedata DEPENDS ${I18NPOOL_LOCALEDATA_SOURCES})


# --- the four locale-data libraries ------------------------------------------
#
# Upstream splits the locale data across four libraries so that a language
# package can ship only what it needs (Library_localedata_{en,es,euro,others}.mk).
# They are plug-ins loaded by i18npool, hence no component file of their own.
#
# They are PLAIN, not OOO, and that is not a detail: Repository.mk registers
# them with gb_Helper_register_plugins_for_install(PLAINLIBS_OOO, ...), so the
# file is liblocaledata_en.so and not liblocaledata_enlo.so — and the name they
# are opened by is written out in the source, not derived.  i18npool's
# loadFunctionSymbolByName ends in
#
#     module->loadRelative(&thisModule, getLibraryName(i.pLib))
#
# with lcl_DATA_EN spelled SAL_DLLPREFIX "localedata_en" SAL_DLLEXTENSION.  No
# SVLIBRARY there, no "lo"; osl_loadModuleRelativeAscii joins the module's
# directory with that string and hands it to dlopen, so a library named the
# usual way is simply not found — and the failure surfaces far away, as
# i18npool::LocaleDataImpl::getFunctionSymbol throwing RuntimeException because
# it could not even fall back to en_US.
#
# Each of the other three links localedata_en: a locale whose own description
# leaves something out falls back to the en_US getter for it, so those symbols
# have to resolve at link time.

set(LOCALEDATA_EN_SOURCES
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_AU.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_BW.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_BZ.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_CA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_DK.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_GB.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_GH.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_GM.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_GY.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_IE.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_JM.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_KE.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_LK.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_MU.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_MW.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_NA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_NG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_NZ.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_PH.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_TT.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_US.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_ZA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_ZM.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_ZW.cxx"
)
add_library(localedata_en SHARED ${LOCALEDATA_EN_SOURCES})
target_include_directories(localedata_en PRIVATE "${CMAKE_SOURCE_DIR}/i18npool/inc")
add_dependencies(localedata_en i18npool_localedata)
target_link_libraries(localedata_en PRIVATE sal)
librestudio_set_library_output_name(localedata_en localedata_en PLAIN)
install(TARGETS localedata_en LIBRARY DESTINATION program)

set(LOCALEDATA_ES_SOURCES
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_AR.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_BO.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_CL.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_CO.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_CR.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_DO.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_EC.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_ES.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_GT.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_HN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_MX.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_NI.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_PA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_PE.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_PR.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_PY.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_SV.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_UY.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_es_VE.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ext_ES.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_gl_ES.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_oc_ES_aranes.cxx"
)
add_library(localedata_es SHARED ${LOCALEDATA_ES_SOURCES})
target_include_directories(localedata_es PRIVATE "${CMAKE_SOURCE_DIR}/i18npool/inc")
add_dependencies(localedata_es i18npool_localedata)
target_link_libraries(localedata_es PRIVATE sal localedata_en)
librestudio_set_library_output_name(localedata_es localedata_es PLAIN)
install(TARGETS localedata_es LIBRARY DESTINATION program)

set(LOCALEDATA_EURO_SOURCES
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_an_ES.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ast_ES.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_be_BY.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_bg_BG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_br_FR.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_bs_BA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ca_ES.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ca_ES_valencia.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_cs_CZ.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_cu_RU.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_cv_RU.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_cy_GB.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_da_DK.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_de_AT.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_de_CH.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_de_DE.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_de_LI.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_de_LU.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_dsb_DE.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_el_GR.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_et_EE.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_eu_ES.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fi_FI.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fo_FO.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fr_BE.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fr_BF.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fr_BJ.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fr_CA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fr_CH.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fr_CI.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fr_FR.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fr_GN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fr_LU.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fr_MC.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fr_ML.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fr_NE.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fr_SN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fr_TG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fur_IT.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fy_NL.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ga_IE.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_gd_GB.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_gos_NL.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_gsc_FR.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_hr_HR.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_hsb_DE.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_is_IS.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_it_CH.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_it_IT.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ka_GE.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_kl_GL.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_la_VA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_lb_LU.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_lij_IT.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_lld_IT.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_lt_LT.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ltg_LV.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_lv_LV.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_mk_MK.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_mt_MT.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_myv_RU.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_nb_NO.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_nds_DE.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_nl_BE.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_nl_NL.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_nn_NO.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_no_NO.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_oc_FR_lengadoc.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_pl_PL.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_pt_BR.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_pt_MZ.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_pt_PT.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ro_MD.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ro_RO.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ru_RU.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_rue_SK.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sc_IT.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sk_SK.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sl_SI.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sq_AL.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sr_CS.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sr_Latn_CS.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sr_Latn_ME.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sr_Latn_RS.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sr_ME.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sr_RS.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sv_FI.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sv_SE.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_szl_PL.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_tr_TR.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_tt_RU.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_uk_UA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_vec_IT.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_vro_EE.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_wa_BE.cxx"
)
add_library(localedata_euro SHARED ${LOCALEDATA_EURO_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(localedata_euro REUSE_FROM precompiled_system)
endif ()
target_include_directories(localedata_euro PRIVATE "${CMAKE_SOURCE_DIR}/i18npool/inc")
add_dependencies(localedata_euro i18npool_localedata)
target_link_libraries(localedata_euro PRIVATE sal localedata_en)
librestudio_set_library_output_name(localedata_euro localedata_euro PLAIN)
install(TARGETS localedata_euro LIBRARY DESTINATION program)

set(LOCALEDATA_OTHERS_SOURCES
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_af_NA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_af_ZA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ak_GH.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_am_ET.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ar_AE.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ar_BH.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ar_DZ.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ar_EG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ar_IQ.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ar_JO.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ar_KW.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ar_LB.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ar_LY.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ar_MA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ar_OM.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ar_QA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ar_SA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ar_SY.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ar_TN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ar_YE.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_axk_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_az_AZ.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_beq_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_bkw_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_bm_ML.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_bn_BD.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_bn_IN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_bo_CN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_bo_IN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_bvx_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_dde_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_dv_MV.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_dz_BT.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ebo_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ee_GH.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_HK.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_IL.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_IN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_en_MY.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_eo.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_fa_IR.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_gu_IN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_gug_PY.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ha_Latn_GH.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ha_Latn_NG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_haw_US.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_he_IL.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_hi_IN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_hil_PH.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ht_HT.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_hu_HU.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_hy_AM.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ia.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_id_ID.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ilo_PH.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_iyx_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ja_JP.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_jbo.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_kab_DZ.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_kk_KZ.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_kkw_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_km_KH.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_kmr_Latn_TR.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_kn_IN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_kng_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ko_KR.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ky_KG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ldi_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_lg_UG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_lgr_SB.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_lif_NP.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ln_CD.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_lo_LA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_mai_IN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_mdw_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_mfe_MU.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_min_ID.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_mkw_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ml_IN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_mn_Cyrl_MN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_mn_Mong_MN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_mos_BF.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_mr_IN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ms_MY.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_my_MM.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ne_NP.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ngz_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_njx_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_njy_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_nr_ZA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_nso_ZA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ny_MW.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_om_ET.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_or_IN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_pa_IN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_pap_BQ.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_pap_CW.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_pdc_US.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_pjt_AU.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_plt_MG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_pt_AO.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_pui_CO.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_puu_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_rw_RW.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sat_IN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sat_Olck_IN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sdj_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sg_CF.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_shs_CA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sid_ET.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_so_SO.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ss_ZA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_st_ZA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sun_ID.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_sw_TZ.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ta_IN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_te_IN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_tek_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_tg_TJ.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_th_TH.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ti_ER.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ti_ET.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_tk_TM.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_tn_ZA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_tpi_PG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ts_ZA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_tsa_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_tyx_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ug_CN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ur_PK.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_uz_UZ.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_ve_ZA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_vi_VN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_vif_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_xh_ZA.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_xku_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_yi_US.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_yom_CG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_zh_CN.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_zh_HK.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_zh_MO.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_zh_SG.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_zh_TW.cxx"
    "${I18NPOOL_LOCALEDATA_DIR}/localedata_zu_ZA.cxx"
)
add_library(localedata_others SHARED ${LOCALEDATA_OTHERS_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(localedata_others REUSE_FROM precompiled_system)
endif ()
target_include_directories(localedata_others PRIVATE "${CMAKE_SOURCE_DIR}/i18npool/inc")
add_dependencies(localedata_others i18npool_localedata)
target_link_libraries(localedata_others PRIVATE sal localedata_en)
librestudio_set_library_output_name(localedata_others localedata_others PLAIN)
install(TARGETS localedata_others LIBRARY DESTINATION program)

# --- the collation rules -----------------------------------------------------
#
# gencoll_rule compiles a rule source with ICU and prints a .cxx defining
# get_collator_data_<name>() / ..._length(); collator_unicode.cxx calls those
# by hand for the languages it supports, so the set generated here has to match
# the set it names.  ko_charset.txt is the one exception: up to ICU 52 it had a
# rule of its own, from 53 on collator_unicode.cxx no longer asks for it, and
# Library_i18npool.mk filters it out the same way.

add_executable(gencoll_rule
    "${CMAKE_SOURCE_DIR}/i18npool/source/collator/gencoll_rule.cxx")
target_link_libraries(gencoll_rule PRIVATE sal ICU::i18n ICU::uc)

set(I18NPOOL_COLLATOR_DIR "${CMAKE_CURRENT_BINARY_DIR}/collator")
file(MAKE_DIRECTORY "${I18NPOOL_COLLATOR_DIR}")

file(GLOB I18NPOOL_COLLATOR_TXTS RELATIVE
    "${CMAKE_SOURCE_DIR}/i18npool/source/collator/data"
    "${CMAKE_SOURCE_DIR}/i18npool/source/collator/data/*.txt")
list(REMOVE_ITEM I18NPOOL_COLLATOR_TXTS "ko_charset.txt")

set(I18NPOOL_COLLATOR_SOURCES)
set(I18NPOOL_COLLATOR_LANGS)
foreach (_txt IN LISTS I18NPOOL_COLLATOR_TXTS)
    get_filename_component(_name "${_txt}" NAME_WE)
    set(_out "${I18NPOOL_COLLATOR_DIR}/collator_${_name}.cxx")
    add_custom_command(
        OUTPUT "${_out}"
        COMMAND gencoll_rule
                "${CMAKE_SOURCE_DIR}/i18npool/source/collator/data/${_txt}"
                "${_out}" "${_name}"
        DEPENDS gencoll_rule
                "${CMAKE_SOURCE_DIR}/i18npool/source/collator/data/${_txt}"
        COMMENT "gencoll_rule: collator_${_name}.cxx"
        VERBATIM)
    list(APPEND I18NPOOL_COLLATOR_SOURCES "${_out}")

    # "zh_TW_charset" and friend are the document locale; the language is the
    # leading component of the file name.
    string(REGEX REPLACE "_.*$" "" _lang "${_name}")
    list(APPEND I18NPOOL_COLLATOR_LANGS "${_lang}")
endforeach ()

# LOCAL_RULE_LANGS is the list of languages that have a rule at all; a locale
# outside it gets no custom collator.  CustomTarget_collator.mk writes the same
# line with echo, deriving it from the same file names.
list(REMOVE_DUPLICATES I18NPOOL_COLLATOR_LANGS)
list(SORT I18NPOOL_COLLATOR_LANGS)
string(REPLACE ";" " " I18NPOOL_COLLATOR_LANGS "${I18NPOOL_COLLATOR_LANGS}")

# This one line is written here rather than by a command of its own: it follows
# from the file list CMake has just read, so it cannot go stale on its own — and
# the generated header is still a tracked input of the object file, so an edit
# to this line does rebuild collator_unicode.cxx.
set(I18NPOOL_LRL_INCLUDE "${I18NPOOL_COLLATOR_DIR}/lrl_include.hxx")
file(WRITE "${I18NPOOL_LRL_INCLUDE}"
    "constexpr OUStringLiteral LOCAL_RULE_LANGS = u\"${I18NPOOL_COLLATOR_LANGS}\";\n")

add_custom_target(i18npool_collator DEPENDS ${I18NPOOL_COLLATOR_SOURCES})

# --- the index data and the text conversion dictionaries ---------------------

add_executable(genindex_data
    "${CMAKE_SOURCE_DIR}/i18npool/source/indexentry/genindex_data.cxx")
target_link_libraries(genindex_data PRIVATE sal)

set(I18NPOOL_INDEXENTRY_DIR "${CMAKE_CURRENT_BINARY_DIR}/indexentry")
file(MAKE_DIRECTORY "${I18NPOOL_INDEXENTRY_DIR}")

file(GLOB I18NPOOL_INDEXENTRY_TXTS RELATIVE
    "${CMAKE_SOURCE_DIR}/i18npool/source/indexentry/data"
    "${CMAKE_SOURCE_DIR}/i18npool/source/indexentry/data/*.txt")

set(I18NPOOL_INDEXENTRY_SOURCES)
foreach (_txt IN LISTS I18NPOOL_INDEXENTRY_TXTS)
    get_filename_component(_name "${_txt}" NAME_WE)
    set(_out "${I18NPOOL_INDEXENTRY_DIR}/${_name}.cxx")
    add_custom_command(
        OUTPUT "${_out}"
        COMMAND genindex_data
                "${CMAKE_SOURCE_DIR}/i18npool/source/indexentry/data/${_txt}"
                "${_out}" "${_name}"
        DEPENDS genindex_data
                "${CMAKE_SOURCE_DIR}/i18npool/source/indexentry/data/${_txt}"
        COMMENT "genindex_data: ${_name}.cxx"
        VERBATIM)
    list(APPEND I18NPOOL_INDEXENTRY_SOURCES "${_out}")
endforeach ()
# Named "i18npool_indexentry_codegen", not the bare "i18npool_indexentry" the
# variable/comment names above would suggest: CppunitTest_i18npool_
# indexentry.mk's own CppunitTest needs the literal name "i18npool_
# indexentry" for librestudio_add_cppunit_test() (matching this port's own
# strict "CppunitTest target name == gbuild test name" convention, load-
# bearing everywhere else in this whole port), and CMake's flat target
# namespace cannot have both -- same two-different-gbuild-namespaces-sharing-
# one-literal-name collision, and the same "rename the internal helper, not
# the CppunitTest" resolution, sfx2_classification's own Package_
# classification/CppunitTest_sfx2_classification name clash already
# established (see sfx2/CMakeLists.txt's own comment on that rename).
add_custom_target(i18npool_indexentry_codegen DEPENDS ${I18NPOOL_INDEXENTRY_SOURCES})

add_executable(genconv_dict
    "${CMAKE_SOURCE_DIR}/i18npool/source/textconversion/genconv_dict.cxx")
target_link_libraries(genconv_dict PRIVATE sal)

set(I18NPOOL_TEXTCONV_DIR "${CMAKE_CURRENT_BINARY_DIR}/textconversion")
file(MAKE_DIRECTORY "${I18NPOOL_TEXTCONV_DIR}")

file(GLOB I18NPOOL_TEXTCONV_DICS RELATIVE
    "${CMAKE_SOURCE_DIR}/i18npool/source/textconversion/data"
    "${CMAKE_SOURCE_DIR}/i18npool/source/textconversion/data/*.dic")

set(I18NPOOL_TEXTCONV_SOURCES)
foreach (_dic IN LISTS I18NPOOL_TEXTCONV_DICS)
    get_filename_component(_name "${_dic}" NAME_WE)
    set(_out "${I18NPOOL_TEXTCONV_DIR}/${_name}.cxx")
    add_custom_command(
        OUTPUT "${_out}"
        # argv[1] is the dictionary name, not a path: the tool switches on it.
        COMMAND genconv_dict "${_name}"
                "${CMAKE_SOURCE_DIR}/i18npool/source/textconversion/data/${_dic}"
                "${_out}"
        DEPENDS genconv_dict
                "${CMAKE_SOURCE_DIR}/i18npool/source/textconversion/data/${_dic}"
        COMMENT "genconv_dict: ${_name}.cxx"
        VERBATIM)
    list(APPEND I18NPOOL_TEXTCONV_SOURCES "${_out}")
endforeach ()
add_custom_target(i18npool_textconversion DEPENDS ${I18NPOOL_TEXTCONV_SOURCES})

# --- the break iterator rules ------------------------------------------------
#
# These three tools are ICU's, not ours, and no counterpart exists in this tree
# — breakiterator_unicode.cxx reaches the rules at run time through
# udata_open("OpenOffice", "brk", <rule>), which needs the table of contents
# gencmn writes, not just the bundles.  configure.ac requires the same three
# when SYSTEM_ICU is set, and looks for them in $PATH:/usr/sbin:/sbin; the
# distribution here keeps genccode and gencmn in /usr/sbin, which is normally
# not on PATH, hence the explicit search path.

find_program(GENBRK_EXECUTABLE genbrk REQUIRED)
find_program(GENCCODE_EXECUTABLE genccode REQUIRED PATHS /usr/sbin /sbin)
find_program(GENCMN_EXECUTABLE gencmn REQUIRED PATHS /usr/sbin /sbin)

set(I18NPOOL_BRK_DIR "${CMAKE_CURRENT_BINARY_DIR}/breakiterator")
file(MAKE_DIRECTORY "${I18NPOOL_BRK_DIR}")

file(GLOB I18NPOOL_BRK_TXTS RELATIVE
    "${CMAKE_SOURCE_DIR}/i18npool/source/breakiterator/data"
    "${CMAKE_SOURCE_DIR}/i18npool/source/breakiterator/data/*.txt")

set(I18NPOOL_BRK_OBJECTS)
set(I18NPOOL_BRK_NAMES)
foreach (_txt IN LISTS I18NPOOL_BRK_TXTS)
    get_filename_component(_name "${_txt}" NAME_WE)
    set(_brk "${I18NPOOL_BRK_DIR}/${_name}.brk")
    set(_c "${I18NPOOL_BRK_DIR}/${_name}_brk.c")
    add_custom_command(
        OUTPUT "${_brk}"
        COMMAND "${GENBRK_EXECUTABLE}"
                -r "${CMAKE_SOURCE_DIR}/i18npool/source/breakiterator/data/${_txt}"
                -o "${_brk}"
        DEPENDS "${CMAKE_SOURCE_DIR}/i18npool/source/breakiterator/data/${_txt}"
        COMMENT "genbrk: ${_name}.brk"
        VERBATIM)
    add_custom_command(
        OUTPUT "${_c}"
        COMMAND "${GENCCODE_EXECUTABLE}" -n OpenOffice
                -d "${I18NPOOL_BRK_DIR}" "${_brk}"
        DEPENDS "${_brk}"
        COMMENT "genccode: ${_name}_brk.c"
        VERBATIM)
    list(APPEND I18NPOOL_BRK_OBJECTS "${_c}")
    list(APPEND I18NPOOL_BRK_NAMES "${_name}.brk")
endforeach ()

# gencmn takes its input from a list file and rejects absolute paths, so the
# file holds bare names and the command runs in the bundle directory.  The list
# is fixed by the glob above, so writing it at configure time is enough.
set(I18NPOOL_BRK_LIST "${I18NPOOL_BRK_DIR}/OpenOffice.brk.list")
set(_listContents "")
foreach (_brk IN LISTS I18NPOOL_BRK_NAMES)
    string(APPEND _listContents "${_brk}\n")
endforeach ()
file(WRITE "${I18NPOOL_BRK_LIST}" "${_listContents}")

# -t dat names the output OpenOffice_dat.c and -S asks for a source file
# holding the table of contents rather than a binary package; the symbol inside
# is OpenOffice_dat, which is what breakiterator_unicode.cxx declares.
# Upstream passes -t tmp and prepends the contents to OpenOffice_dat.c by hand,
# only to add a pragma that silences a Microsoft compiler warning.
set(I18NPOOL_BRK_TOC "${I18NPOOL_BRK_DIR}/OpenOffice_dat.c")
add_custom_command(
    OUTPUT "${I18NPOOL_BRK_TOC}"
    COMMAND "${GENCMN_EXECUTABLE}" -n OpenOffice -t dat -S
            -d "${I18NPOOL_BRK_DIR}" 0 "${I18NPOOL_BRK_LIST}"
    WORKING_DIRECTORY "${I18NPOOL_BRK_DIR}"
    DEPENDS ${I18NPOOL_BRK_OBJECTS} "${I18NPOOL_BRK_LIST}"
    COMMENT "gencmn: OpenOffice_dat.c"
    VERBATIM)
add_custom_target(i18npool_breakiterator DEPENDS "${I18NPOOL_BRK_TOC}")

# --- libi18npool -------------------------------------------------------------

set(I18NPOOL_SOURCES
    i18npool/source/breakiterator/breakiterator_cjk
    i18npool/source/breakiterator/breakiteratorImpl
    i18npool/source/breakiterator/breakiterator_unicode
    i18npool/source/calendar/calendar_gregorian
    i18npool/source/calendar/calendar_hijri
    i18npool/source/calendar/calendarImpl
    i18npool/source/calendar/calendar_jewish
    i18npool/source/characterclassification/cclass_unicode
    i18npool/source/characterclassification/cclass_unicode_parser
    i18npool/source/characterclassification/characterclassificationImpl
    i18npool/source/characterclassification/unoscripttypedetector
    i18npool/source/collator/chaptercollator
    i18npool/source/collator/collatorImpl
    i18npool/source/collator/collator_unicode
    i18npool/source/defaultnumberingprovider/defaultnumberingprovider
    i18npool/source/indexentry/indexentrysupplier
    i18npool/source/indexentry/indexentrysupplier_asian
    i18npool/source/indexentry/indexentrysupplier_common
    i18npool/source/indexentry/indexentrysupplier_default
    i18npool/source/indexentry/indexentrysupplier_ja_phonetic
    i18npool/source/inputchecker/inputsequencechecker
    i18npool/source/inputchecker/inputsequencechecker_hi
    i18npool/source/inputchecker/inputsequencechecker_th
    i18npool/source/localedata/localedata
    i18npool/source/nativenumber/nativenumbersupplier
    i18npool/source/numberformatcode/numberformatcode
    i18npool/source/ordinalsuffix/ordinalsuffix
    i18npool/source/registerservices/registerservices
    i18npool/source/textconversion/textconversion
    i18npool/source/textconversion/textconversionImpl
    i18npool/source/textconversion/textconversion_ko
    i18npool/source/textconversion/textconversion_zh
    i18npool/source/transliteration/chartonum
    i18npool/source/transliteration/fullwidthToHalfwidth
    i18npool/source/transliteration/halfwidthToFullwidth
    i18npool/source/transliteration/hiraganaToKatakana
    i18npool/source/transliteration/ignoreBaFa_ja_JP
    i18npool/source/transliteration/ignoreDiacritics_CTL
    i18npool/source/transliteration/ignoreHyuByu_ja_JP
    i18npool/source/transliteration/ignoreIandEfollowedByYa_ja_JP
    i18npool/source/transliteration/ignoreIterationMark_ja_JP
    i18npool/source/transliteration/ignoreKana
    i18npool/source/transliteration/ignoreKashida_CTL
    i18npool/source/transliteration/ignoreKiKuFollowedBySa_ja_JP
    i18npool/source/transliteration/ignoreMiddleDot_ja_JP
    i18npool/source/transliteration/ignoreMinusSign_ja_JP
    i18npool/source/transliteration/ignoreProlongedSoundMark_ja_JP
    i18npool/source/transliteration/ignoreSeparator_ja_JP
    i18npool/source/transliteration/ignoreSeZe_ja_JP
    i18npool/source/transliteration/ignoreSize_ja_JP
    i18npool/source/transliteration/ignoreSpace_ja_JP
    i18npool/source/transliteration/ignoreTiJi_ja_JP
    i18npool/source/transliteration/ignoreTraditionalKana_ja_JP
    i18npool/source/transliteration/ignoreTraditionalKanji_ja_JP
    i18npool/source/transliteration/ignoreWidth
    i18npool/source/transliteration/ignoreZiZu_ja_JP
    i18npool/source/transliteration/katakanaToHiragana
    i18npool/source/transliteration/largeToSmall_ja_JP
    i18npool/source/transliteration/numtochar
    i18npool/source/transliteration/numtotext_cjk
    i18npool/source/transliteration/smallToLarge_ja_JP
    i18npool/source/transliteration/texttonum
    i18npool/source/transliteration/textToPronounce_zh
    i18npool/source/transliteration/transliteration_body
    i18npool/source/transliteration/transliteration_caseignore
    i18npool/source/transliteration/transliteration_commonclass
    i18npool/source/transliteration/transliteration_Ignore
    i18npool/source/transliteration/transliterationImpl
    i18npool/source/transliteration/transliteration_Numeric
    i18npool/source/transliteration/transliteration_OneToOne
)

# The generated tables are ordinary translation units of this library, as they
# are upstream through gb_Library_add_generated_exception_objects.
list(APPEND I18NPOOL_SOURCES
    ${I18NPOOL_COLLATOR_SOURCES}
    ${I18NPOOL_INDEXENTRY_SOURCES}
    ${I18NPOOL_TEXTCONV_SOURCES}
    ${I18NPOOL_BRK_OBJECTS}
    "${I18NPOOL_BRK_TOC}")

librestudio_expand_cxx(I18NPOOL_SOURCES)

add_library(i18npool SHARED ${I18NPOOL_SOURCES})

target_include_directories(i18npool PRIVATE
    "${CMAKE_SOURCE_DIR}/i18npool/inc"
    # lrl_include.hxx, included by collator_unicode.cxx.
    "${I18NPOOL_COLLATOR_DIR}"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
# No dependency on i18npool_localedata here: the generated locale data goes into
# the localedata_* plug-ins, not into this library.  Ordering it here as well
# would close a cycle through services.rdb — the registry needs i18npool's
# component, and the locale generator needs the registry.
#
# The other four generators are needed here: their output is compiled into this
# library, and i18npool_collator in particular has to have written
# lrl_include.hxx before collator_unicode.cxx is compiled.
add_dependencies(i18npool
    i18npool_collator i18npool_indexentry_codegen i18npool_textconversion
    i18npool_breakiterator
    officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(i18npool PRIVATE I18NPOOL_DLLIMPLEMENTATION)
target_link_libraries(i18npool PRIVATE
    comphelper cppu cppuhelper i18nlangtag i18nutil sal ICU::uc ICU::i18n)

librestudio_add_component(i18npool
    COMPONENT "${CMAKE_SOURCE_DIR}/i18npool/util/i18npool.component"
    LIBRARY   i18npool
    LAYER     OOO)

librestudio_set_library_output_name(i18npool i18npool OOO)
install(TARGETS i18npool LIBRARY DESTINATION program)

# --- libi18nsearch -----------------------------------------------------------

set(I18NSEARCH_SOURCES
    i18npool/source/search/levdis
    i18npool/source/search/textsearch
)

librestudio_expand_cxx(I18NSEARCH_SOURCES)

add_library(i18nsearch SHARED ${I18NSEARCH_SOURCES})

target_include_directories(i18nsearch PRIVATE
    "${CMAKE_SOURCE_DIR}/i18npool/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(i18nsearch udkapi_normal_headers offapi_normal_headers)

target_link_libraries(i18nsearch PRIVATE comphelper cppu cppuhelper sal ICU::uc ICU::i18n)

librestudio_add_component(i18nsearch
    COMPONENT "${CMAKE_SOURCE_DIR}/i18npool/source/search/i18nsearch.component"
    LIBRARY   i18nsearch
    LAYER     OOO)

librestudio_set_library_output_name(i18nsearch i18nsearch OOO)
install(TARGETS i18nsearch LIBRARY DESTINATION program)

# --- i18npool's 8 CppunitTest_*.mk files, re-checked directly ---------------
#
# 7 of the 8 need only this module's own "i18npool"/"i18nsearch" components
# (both already registered above, in this same file, so no ordering issue)
# plus plain sal/cppu/cppuhelper/unotest libraries -- wired directly below,
# no deferral needed. Only i18npool_defaultnumberingprovider calls use_rdb
# (services), needing program_services_rdb (defined much later in the root
# CMakeLists.txt) -- that one is deferred, see i18npool/CppunitTest_
# deferred.cmake.

# CppunitTest_i18npool_transliteration.mk: registers as "i18npool_
# transliteration" (matches its own filename, unlike breakiterator below).
# 1 source, use_components(i18npool/util/i18npool) -> "i18npool", use_
# libraries cppu cppuhelper i18npool sal, use_sdk_api, use_ure -> 2
# protector pairs (no use_vcl in this .mk).
librestudio_add_cppunit_test(i18npool_transliteration
    SOURCES
        i18npool/qa/cppunit/transliteration
    LIBRARIES
        cppu cppuhelper i18npool sal
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
    COMPONENTS
        i18npool)

# CppunitTest_i18npool_breakiterator.mk: registers as "i18npool_break_
# iterator" (NOT "i18npool_breakiterator" -- the gb_CppunitTest_CppunitTest
# call's own first argument has an underscore the .mk's filename doesn't;
# verified directly, not a typo introduced here). use_external(icu_headers)
# -> ICU::uc (icu_headers alone maps to the plain headers-only usage already
# established elsewhere in this port), use_sdk_api, use_libraries cppu
# cppuhelper sal unotest, use_ure -> 2 protector pairs, use_components(
# i18npool/util/i18npool) -> "i18npool".
librestudio_add_cppunit_test(i18npool_break_iterator
    SOURCES
        i18npool/qa/cppunit/test_breakiterator
    LIBRARIES
        cppu cppuhelper sal unotest ICU::uc
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
    COMPONENTS
        i18npool)

# CppunitTest_i18npool_textsearch.mk: use_sdk_api, use_ure -> 2 protector
# pairs, use_components(i18npool/source/search/i18nsearch,
# i18npool/util/i18npool) -> "i18nsearch"+"i18npool" (both registered above,
# in this same file), use_externals(icui18n icuuc icu_headers) ->
# ICU::i18n+ICU::uc, use_libraries cppu cppuhelper sal unotest.
librestudio_add_cppunit_test(i18npool_textsearch
    SOURCES
        i18npool/qa/cppunit/test_textsearch
    LIBRARIES
        cppu cppuhelper sal unotest ICU::i18n ICU::uc
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
    COMPONENTS
        i18nsearch i18npool)

# CppunitTest_i18npool_indexentry.mk: use_components(i18npool/util/i18npool)
# -> "i18npool", use_libraries cppu cppuhelper sal, use_sdk_api, use_ure ->
# 2 protector pairs.
librestudio_add_cppunit_test(i18npool_indexentry
    SOURCES
        i18npool/qa/cppunit/indexentry
    LIBRARIES
        cppu cppuhelper sal
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
    COMPONENTS
        i18npool)

# CppunitTest_i18npool_ordinalsuffix.mk: use_sdk_api, use_libraries cppu
# cppuhelper sal unotest, use_ure -> 2 protector pairs, use_components(
# i18npool/util/i18npool) -> "i18npool".
librestudio_add_cppunit_test(i18npool_ordinalsuffix
    SOURCES
        i18npool/qa/cppunit/test_ordinalsuffix
    LIBRARIES
        cppu cppuhelper sal unotest
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
    COMPONENTS
        i18npool)

# CppunitTest_i18npool_characterclassification.mk: use_sdk_api, use_libraries
# cppu cppuhelper sal unotest, use_ure -> 2 protector pairs, use_components(
# i18npool/util/i18npool) -> "i18npool".
librestudio_add_cppunit_test(i18npool_characterclassification
    SOURCES
        i18npool/qa/cppunit/test_characterclassification
    LIBRARIES
        cppu cppuhelper sal unotest
    INCLUDE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
    COMPONENTS
        i18npool)

# CppunitTest_i18npool_calendar.mk: set_include(-Ii18npool/inc), use_sdk_api,
# use_libraries cppu cppuhelper sal unotest i18npool, use_externals(icui18n
# icuuc icu_headers) -> ICU::i18n+ICU::uc, use_ure -> 2 protector pairs,
# use_components(i18npool/util/i18npool) -> "i18npool".
librestudio_add_cppunit_test(i18npool_calendar
    SOURCES
        i18npool/qa/cppunit/test_calendar
    LIBRARIES
        cppu cppuhelper sal unotest i18npool ICU::i18n ICU::uc
    INCLUDE
        "${CMAKE_SOURCE_DIR}/i18npool/inc"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}"
    DEPENDS
        udkapi_normal_headers offapi_normal_headers
    PROTECTORS
        unobootstrapprotector unobootstrapprotector
        unoexceptionprotector unoexceptionprotector
    COMPONENTS
        i18npool)

# i18npool/CppunitTest_deferred.cmake needs data (program_services_rdb) from
# a module add_subdirectory()'d later than this one -- scheduled here rather
# than included directly, so it runs only once every add_subdirectory() in
# the whole tree has completed. See that file's own header comment for the
# specific constraint, and cmake/LibreStudioTest.cmake's header comment for
# why CMAKE_SOURCE_DIR (not CMAKE_CURRENT_SOURCE_DIR) is required in the
# path argument below.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/i18npool/CppunitTest_deferred.cmake")
