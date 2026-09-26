# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# lotuswordpro (module lotuswordpro) -- one library: lwpft, the Lotus
# WordPro (.lwp) import filter. Source list, include, externals and
# dependencies are transcribed from Library_lwpft.mk.
#
# Module_lotuswordpro.mk itself gates its whole gb_Module_add_targets block
# behind `ifeq ($(ENABLE_LWP),TRUE)`, a configure.ac feature switch upstream
# defaults off on some platforms. Every other upstream configure.ac feature
# gate (ENABLE_*, HAVE_FEATURE_*) has already been treated as not applicable
# to this port's own module-inclusion decisions elsewhere in the tree -- only
# genuine OS/COM (Windows/macOS) conditionals are excluded -- so ENABLE_LWP
# is likewise not reproduced here: lwpft is ported unconditionally, like any
# other cross-platform library.
#
# Not reproduced:
#  - gb_Library_set_precompiled_header,lwpft,... -- precompiled headers are
#    not ported anywhere in this build.
#  - CppunitTest_lotuswordpro_test_lotuswordpro / CppunitTest_import_test
#    (gb_Module_add_check_targets): test targets outside this port's scope.

set(LWPFT_SOURCES
    lotuswordpro/source/filter/bencont
    lotuswordpro/source/filter/benlist
    lotuswordpro/source/filter/benname
    lotuswordpro/source/filter/benobj
    lotuswordpro/source/filter/benval
    lotuswordpro/source/filter/explode
    lotuswordpro/source/filter/localtime
    lotuswordpro/source/filter/LotusWordProImportFilter
    lotuswordpro/source/filter/lwp9reader
    lotuswordpro/source/filter/lwpatomholder
    lotuswordpro/source/filter/lwpbackgroundoverride
    lotuswordpro/source/filter/lwpbackgroundstuff
    lotuswordpro/source/filter/lwpbasetype
    lotuswordpro/source/filter/lwpbookmarkmgr
    lotuswordpro/source/filter/lwpborderstuff
    lotuswordpro/source/filter/lwpbreaksoverride
    lotuswordpro/source/filter/lwpbulletstylemgr
    lotuswordpro/source/filter/lwpcelllayout
    lotuswordpro/source/filter/lwpchangemgr
    lotuswordpro/source/filter/lwpcharacterstyle
    lotuswordpro/source/filter/lwpcharborderoverride
    lotuswordpro/source/filter/lwpcharsetmgr
    lotuswordpro/source/filter/lwpcolor
    lotuswordpro/source/filter/lwpcontent
    lotuswordpro/source/filter/lwpdivinfo
    lotuswordpro/source/filter/lwpdivopts
    lotuswordpro/source/filter/lwpdlvlist
    lotuswordpro/source/filter/lwpdoc
    lotuswordpro/source/filter/lwpdocdata
    lotuswordpro/source/filter/lwpdrawobj
    lotuswordpro/source/filter/lwpdropcapmgr
    lotuswordpro/source/filter/lwpfilehdr
    lotuswordpro/source/filter/lwpfilter
    lotuswordpro/source/filter/lwpfnlayout
    lotuswordpro/source/filter/lwpfont
    lotuswordpro/source/filter/lwpfootnote
    lotuswordpro/source/filter/lwpfoundry
    lotuswordpro/source/filter/lwpframelayout
    lotuswordpro/source/filter/lwpfribbreaks
    lotuswordpro/source/filter/lwpfrib
    lotuswordpro/source/filter/lwpfribframe
    lotuswordpro/source/filter/lwpfribmark
    lotuswordpro/source/filter/lwpfribptr
    lotuswordpro/source/filter/lwpfribsection
    lotuswordpro/source/filter/lwpfribtable
    lotuswordpro/source/filter/lwpfribtext
    lotuswordpro/source/filter/lwpglobalmgr
    lotuswordpro/source/filter/lwpgrfobj
    lotuswordpro/source/filter/lwpholder
    lotuswordpro/source/filter/lwphyperlinkmgr
    lotuswordpro/source/filter/lwpidxmgr
    lotuswordpro/source/filter/lwplayout
    lotuswordpro/source/filter/lwplaypiece
    lotuswordpro/source/filter/lwplnopts
    lotuswordpro/source/filter/lwpmarker
    lotuswordpro/source/filter/lwpnotes
    lotuswordpro/source/filter/lwpnumberingoverride
    lotuswordpro/source/filter/lwpnumericfmt
    lotuswordpro/source/filter/lwpobj
    lotuswordpro/source/filter/lwpobjfactory
    lotuswordpro/source/filter/lwpobjhdr
    lotuswordpro/source/filter/lwpobjid
    lotuswordpro/source/filter/lwpobjstrm
    lotuswordpro/source/filter/lwpoleobject
    lotuswordpro/source/filter/lwpoverride
    lotuswordpro/source/filter/lwppagehint
    lotuswordpro/source/filter/lwppagelayout
    lotuswordpro/source/filter/lwppara1
    lotuswordpro/source/filter/lwpparaborderoverride
    lotuswordpro/source/filter/lwppara
    lotuswordpro/source/filter/lwpparaproperty
    lotuswordpro/source/filter/lwpparastyle
    lotuswordpro/source/filter/lwpproplist
    lotuswordpro/source/filter/lwpprtinfo
    lotuswordpro/source/filter/lwprowlayout
    lotuswordpro/source/filter/lwpsdwfileloader
    lotuswordpro/source/filter/lwpsdwgrouploaderv0102
    lotuswordpro/source/filter/lwpsdwrect
    lotuswordpro/source/filter/lwpsection
    lotuswordpro/source/filter/lwpsilverbullet
    lotuswordpro/source/filter/lwpsortopt
    lotuswordpro/source/filter/lwpstory
    lotuswordpro/source/filter/lwpsvstream
    lotuswordpro/source/filter/lwptable
    lotuswordpro/source/filter/lwptablelayout
    lotuswordpro/source/filter/lwptaboverride
    lotuswordpro/source/filter/lwptabrack
    lotuswordpro/source/filter/lwptblcell
    lotuswordpro/source/filter/lwptblformula
    lotuswordpro/source/filter/lwptoc
    lotuswordpro/source/filter/lwptools
    lotuswordpro/source/filter/lwpuidoc
    lotuswordpro/source/filter/lwpusrdicts
    lotuswordpro/source/filter/lwpverdocument
    lotuswordpro/source/filter/lwpvpointer
    lotuswordpro/source/filter/tocread
    lotuswordpro/source/filter/utbenvs
    lotuswordpro/source/filter/utlist
    lotuswordpro/source/filter/xfilter/xfbase64
    lotuswordpro/source/filter/xfilter/xfbgimage
    lotuswordpro/source/filter/xfilter/xfborders
    lotuswordpro/source/filter/xfilter/xfcell
    lotuswordpro/source/filter/xfilter/xfcellstyle
    lotuswordpro/source/filter/xfilter/xfchange
    lotuswordpro/source/filter/xfilter/xfcolor
    lotuswordpro/source/filter/xfilter/xfcolstyle
    lotuswordpro/source/filter/xfilter/xfcolumns
    lotuswordpro/source/filter/xfilter/xfconfigmanager
    lotuswordpro/source/filter/xfilter/xfcontentcontainer
    lotuswordpro/source/filter/xfilter/xfcrossref
    lotuswordpro/source/filter/xfilter/xfdate
    lotuswordpro/source/filter/xfilter/xfdatestyle
    lotuswordpro/source/filter/xfilter/xfdocfield
    lotuswordpro/source/filter/xfilter/xfdrawareastyle
    lotuswordpro/source/filter/xfilter/xfdrawline
    lotuswordpro/source/filter/xfilter/xfdrawlinestyle
    lotuswordpro/source/filter/xfilter/xfdrawobj
    lotuswordpro/source/filter/xfilter/xfdrawpath
    lotuswordpro/source/filter/xfilter/xfdrawpolygon
    lotuswordpro/source/filter/xfilter/xfdrawpolyline
    lotuswordpro/source/filter/xfilter/xfdrawrect
    lotuswordpro/source/filter/xfilter/xfdrawstyle
    lotuswordpro/source/filter/xfilter/xfentry
    lotuswordpro/source/filter/xfilter/xffloatframe
    lotuswordpro/source/filter/xfilter/xffont
    lotuswordpro/source/filter/xfilter/xffontdecl
    lotuswordpro/source/filter/xfilter/xffontfactory
    lotuswordpro/source/filter/xfilter/xfframe
    lotuswordpro/source/filter/xfilter/xfframestyle
    lotuswordpro/source/filter/xfilter/xfglobal
    lotuswordpro/source/filter/xfilter/xfheaderstyle
    lotuswordpro/source/filter/xfilter/xfimage
    lotuswordpro/source/filter/xfilter/xfimagestyle
    lotuswordpro/source/filter/xfilter/xfindex
    lotuswordpro/source/filter/xfilter/xflist
    lotuswordpro/source/filter/xfilter/xfliststyle
    lotuswordpro/source/filter/xfilter/xfmargins
    lotuswordpro/source/filter/xfilter/xfmasterpage
    lotuswordpro/source/filter/xfilter/xfnumberstyle
    lotuswordpro/source/filter/xfilter/xfofficemeta
    lotuswordpro/source/filter/xfilter/xfpagemaster
    lotuswordpro/source/filter/xfilter/xfparagraph
    lotuswordpro/source/filter/xfilter/xfparastyle
    lotuswordpro/source/filter/xfilter/xfrow
    lotuswordpro/source/filter/xfilter/xfrowstyle
    lotuswordpro/source/filter/xfilter/xfsaxattrlist
    lotuswordpro/source/filter/xfilter/xfsaxstream
    lotuswordpro/source/filter/xfilter/xfsection
    lotuswordpro/source/filter/xfilter/xfsectionstyle
    lotuswordpro/source/filter/xfilter/xfshadow
    lotuswordpro/source/filter/xfilter/xfstylecont
    lotuswordpro/source/filter/xfilter/xfstyle
    lotuswordpro/source/filter/xfilter/xfstylemanager
    lotuswordpro/source/filter/xfilter/xftable
    lotuswordpro/source/filter/xfilter/xftablestyle
    lotuswordpro/source/filter/xfilter/xftabstyle
    lotuswordpro/source/filter/xfilter/xftextboxstyle
    lotuswordpro/source/filter/xfilter/xftextcontent
    lotuswordpro/source/filter/xfilter/xftextspan
    lotuswordpro/source/filter/xfilter/xftextstyle
    lotuswordpro/source/filter/xfilter/xftimestyle
    lotuswordpro/source/filter/xfilter/xfutil
)

librestudio_expand_cxx(LWPFT_SOURCES)
add_library(lwpft SHARED ${LWPFT_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(lwpft PRIVATE
        "${CMAKE_SOURCE_DIR}/lotuswordpro/inc/pch/precompiled_lwpft.hxx")
    target_compile_definitions(lwpft PRIVATE PCH_LEVEL=2)
endif ()

target_include_directories(lwpft PRIVATE
    "${CMAKE_SOURCE_DIR}/lotuswordpro/inc"
    "${MDDS_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(lwpft udkapi_normal_headers offapi_normal_headers)

target_link_libraries(lwpft PRIVATE
    comphelper cppu cppuhelper sal salhelper i18nlangtag
    sfx sot svl svt svxcore tl ucbhelper utl vcl xo
    ICU::uc ICU::i18n
    Boost::boost)

# lotuswordpro/util/lwpfilter.component's third gb_Library_set_componentfile
# argument is "services", i.e. the office layer.
librestudio_add_component(lwpft
    COMPONENT "${CMAKE_SOURCE_DIR}/lotuswordpro/util/lwpfilter.component"
    LIBRARY   lwpft
    LAYER     OOO)

librestudio_set_library_output_name(lwpft lwpft OOO)

install(TARGETS lwpft LIBRARY DESTINATION program)
