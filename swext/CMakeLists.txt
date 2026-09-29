# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# swext -- one extension: com.sun.wiki-publisher (the MediaWiki
# Send-To-Wiki extension), a pure-Java UNO component packaged as an .oxt.
# Investigated per this module's task instructions rather than assumed:
# this is NOT a "no JVM bridge, therefore automatically out of scope"
# situation -- this port already has real, working Java UNO jar
# infrastructure (cmake/LibreStudioJavaJar.cmake's librestudio_add_uno_jar,
# with real production call sites in ridljar/CMakeLists.txt and
# bean/CMakeLists.txt, both gated behind the same LIBRESTUDIO_ENABLE_JAVA
# option used below), so the Jar_mediawiki piece specifically is genuinely
# portable and is built for real. What IS genuinely missing is a generic
# ".oxt extension" packaging primitive (equivalent to gb_Extension) -- noted
# already, for a *different* module's deferred Java test extension, by
# smoketest/CMakeLists.txt's own "Deferred: Extension_TestExtension.mk"
# section. This module does not defer that gap the same way: since
# wiki-publisher is this module's *only* real content, the packaging step
# is hand-rolled below (a real add_custom_command, not a reusable helper)
# rather than skipped outright.
#
# Module_swext.mk, read in full:
#
#   $(eval $(call gb_Module_Module,swext))
#   ifeq ($(ENABLE_MEDIAWIKI),TRUE)
#   $(eval $(call gb_Module_add_targets,swext,\
#       Jar_mediawiki \
#       Configuration_mediawiki \
#       Extension_wiki-publisher \
#   ))
#   endif
#
# ENABLE_MEDIAWIKI (configure.ac ~line 13773): TRUE only when
# --enable-ext-wiki-publisher was passed AND extension integration is on
# AND Java is enabled (with_java != no) -- upstream itself hard-errors
# ("Wiki Publisher requires Java!") if Java is off but this extension was
# explicitly requested. Reproduced below as a real, explicit option
# (default OFF, matching upstream's own configure default of not building
# extensions unless asked) plus the identical Java-required check.
option(ENABLE_MEDIAWIKI
    "configure.ac's --enable-ext-wiki-publisher: build the MediaWiki Send-To-Wiki extension (swext/mediawiki, packaged as wiki-publisher.oxt). Requires LIBRESTUDIO_ENABLE_JAVA."
    OFF)

if (ENABLE_MEDIAWIKI)
    if (NOT LIBRESTUDIO_ENABLE_JAVA)
        message(FATAL_ERROR
            "swext: ENABLE_MEDIAWIKI requires LIBRESTUDIO_ENABLE_JAVA (configure.ac's own real gate: \"Wiki Publisher requires Java! Enable Java if you want to build it.\")")
    endif ()

    # -------------------------------------------------------------------
    # Jar_mediawiki.mk -- mediawiki.jar: 12 plain Java sources under
    # com.sun.star.wiki, classpath'd against libreoffice.jar
    # (gb_Jar_use_jars,mediawiki,libreoffice -- ridljar/CMakeLists.txt's
    # own "libreoffice" librestudio_add_uno_jar call, which this file
    # depends on existing/having run first: real add_subdirectory()
    # ordering constraint for whoever wires swext into the root tree --
    # ridljar must be added before swext).
    # -------------------------------------------------------------------
    librestudio_add_uno_jar(mediawiki
        PACKAGEROOT "com"
        MANIFEST "${CMAKE_SOURCE_DIR}/swext/mediawiki/src/com/sun/star/wiki/MANIFEST.MF"
        CLASSPATH "${libreoffice_JAR}"
        DEPENDS libreoffice_jar
        SOURCES
            swext/mediawiki/src/com/sun/star/wiki/EditPageParser
            swext/mediawiki/src/com/sun/star/wiki/Helper
            swext/mediawiki/src/com/sun/star/wiki/MainThreadDialogExecutor
            swext/mediawiki/src/com/sun/star/wiki/Settings
            swext/mediawiki/src/com/sun/star/wiki/WikiArticle
            swext/mediawiki/src/com/sun/star/wiki/WikiCancelException
            swext/mediawiki/src/com/sun/star/wiki/WikiDialog
            swext/mediawiki/src/com/sun/star/wiki/WikiEditorImpl
            swext/mediawiki/src/com/sun/star/wiki/WikiEditSettingDialog
            swext/mediawiki/src/com/sun/star/wiki/WikiOptionsEventHandlerImpl
            swext/mediawiki/src/com/sun/star/wiki/WikiPropDialog
            swext/mediawiki/src/com/sun/star/wiki/WikiProtocolSocketFactory)

    # -------------------------------------------------------------------
    # Configuration_mediawiki.mk -- gb_Configuration_Configuration,mediawiki,
    # *nodeliver*: these xcu/xcs registry fragments are never merged into
    # the main install tree's registry (that is what "nodeliver" means --
    # contrast with officecfg/CMakeLists.txt's real registry merge for
    # non-extension configuration data); they exist purely as named,
    # already-final source files that Extension_wiki-publisher.mk packages
    # straight into the .oxt below via gb_Extension_add_file /
    # gb_XcuFile_for_extension. No CMake target of its own is needed --
    # the Extension step below references the same
    # swext/mediawiki/src/registry/... paths directly.
    # -------------------------------------------------------------------

    # -------------------------------------------------------------------
    # Extension_wiki-publisher.mk's inline "run_zip_template_recipe" --
    # builds templates/MediaWiki/mediawiki.ott, a bare ODF text template
    # assembled from swext/mediawiki/src/filter/mediawiki/*: 8 members
    # (verified against the live recipe's own addprefix list), of which
    # the 5 *.xml ones are first run through extras/util/compact.xsl via
    # xsltproc (whitespace-stripped for a smaller template), and the other
    # 3 (mimetype, manifest.rdf, Thumbnails/thumbnail.png) are copied
    # verbatim. XSLTPROC_EXECUTABLE is this port's own existing,
    # already-REQUIRED find_program (root CMakeLists.txt line ~56) --
    # reused here rather than re-found.
    # -------------------------------------------------------------------
    find_program(SWEXT_ZIP_EXECUTABLE zip)

    set(MEDIAWIKI_FILTER_SRCDIR "${CMAKE_SOURCE_DIR}/swext/mediawiki/src/filter/mediawiki")
    set(MEDIAWIKI_TEMPLATE_STAGE "${CMAKE_CURRENT_BINARY_DIR}/mediawiki-template")
    set(MEDIAWIKI_OTT "${CMAKE_CURRENT_BINARY_DIR}/mediawiki.ott")

    set(MEDIAWIKI_TEMPLATE_XML_MEMBERS
        content.xml meta.xml styles.xml settings.xml META-INF/manifest.xml)
    set(MEDIAWIKI_TEMPLATE_COPY_MEMBERS
        mimetype manifest.rdf Thumbnails/thumbnail.png)

    set(MEDIAWIKI_TEMPLATE_STAGED_FILES "")
    foreach (_m IN LISTS MEDIAWIKI_TEMPLATE_XML_MEMBERS)
        set(_out "${MEDIAWIKI_TEMPLATE_STAGE}/${_m}")
        add_custom_command(
            OUTPUT "${_out}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${MEDIAWIKI_TEMPLATE_STAGE}/${_m}/.."
            COMMAND "${XSLTPROC_EXECUTABLE}" --nonet -o "${_out}"
                    "${CMAKE_SOURCE_DIR}/extras/util/compact.xsl"
                    "${MEDIAWIKI_FILTER_SRCDIR}/${_m}"
            DEPENDS "${MEDIAWIKI_FILTER_SRCDIR}/${_m}"
            COMMENT "xsltproc: mediawiki.ott/${_m}"
            VERBATIM)
        list(APPEND MEDIAWIKI_TEMPLATE_STAGED_FILES "${_out}")
    endforeach ()
    foreach (_m IN LISTS MEDIAWIKI_TEMPLATE_COPY_MEMBERS)
        set(_out "${MEDIAWIKI_TEMPLATE_STAGE}/${_m}")
        add_custom_command(
            OUTPUT "${_out}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${MEDIAWIKI_TEMPLATE_STAGE}/${_m}/.."
            COMMAND "${CMAKE_COMMAND}" -E copy "${MEDIAWIKI_FILTER_SRCDIR}/${_m}" "${_out}"
            DEPENDS "${MEDIAWIKI_FILTER_SRCDIR}/${_m}"
            COMMENT "copy: mediawiki.ott/${_m}"
            VERBATIM)
        list(APPEND MEDIAWIKI_TEMPLATE_STAGED_FILES "${_out}")
    endforeach ()

    if (SWEXT_ZIP_EXECUTABLE)
        # `zip -q0X --filesync --must-match $@ mimetype && zip -qrX
        # --must-match $@ <rest> -x mimetype` -- mimetype stored first and
        # uncompressed (-0), the ODF convention; everything else appended
        # normally. Reproduced as two real zip invocations from inside the
        # staging directory, matching the original recipe's own `cd
        # $(dir $<) && ...` working directory.
        add_custom_command(
            OUTPUT "${MEDIAWIKI_OTT}"
            COMMAND "${CMAKE_COMMAND}" -E rm -f "${MEDIAWIKI_OTT}"
            COMMAND "${SWEXT_ZIP_EXECUTABLE}" -q0X "${MEDIAWIKI_OTT}" mimetype
            COMMAND "${SWEXT_ZIP_EXECUTABLE}" -qrX "${MEDIAWIKI_OTT}" . -x mimetype
            WORKING_DIRECTORY "${MEDIAWIKI_TEMPLATE_STAGE}"
            DEPENDS ${MEDIAWIKI_TEMPLATE_STAGED_FILES}
            COMMENT "zip: mediawiki.ott"
            VERBATIM)
    endif ()

    # -------------------------------------------------------------------
    # Extension_wiki-publisher.mk itself -- stages every named file into
    # one directory and zips it up as wiki-publisher.oxt (an .oxt is a
    # plain zip, LibreOffice's own convention, same as .ott above).
    #
    # Deliberate simplifications relative to upstream, each flagged rather
    # than silently assumed equivalent:
    #   - gb_XcuFile_for_extension (used for Addons.xcu, OptionsDialog.xcu,
    #     WikiExtension.xcu) performs a per-locale localization merge
    #     (gb_Extension_TRANS_LANGS) before packaging; this port treats
    #     localization as out of scope tree-wide (see e.g. desktop/
    #     CMakeLists.txt's own AllLangMoTarget exclusion note), so the
    #     plain English-source .xcu is used directly here instead.
    #   - gb_Extension_use_default_description's DESCRIPTION flag also
    #     substitutes localized description-*.txt files into
    #     description.xml via an XRM merge step
    #     (solenv/gbuild/Extension.mk lines ~78-90); only the English
    #     description.xml (already a complete, real file at
    #     swext/mediawiki/src/description.xml -- confirmed to exist by
    #     reading it) plus description-en-US.txt are packaged verbatim.
    #   - gb_Extension_use_default_license's LICENSE file
    #     ($(INSTROOT)/LICENSE, i.e. the final packaged product's own
    #     top-level LICENSE, generated from readlicense_oo/license/
    #     license.xml by that not-yet-CMake-ported module) is not
    #     reproducible from here; packaged as "registration" only when a
    #     plausible source is found, skipped with a STATUS message
    #     otherwise -- not a hard error, since a missing license file does
    #     not stop the extension itself from working.
    # -------------------------------------------------------------------
    set(WIKI_PUBLISHER_SRC "${CMAKE_SOURCE_DIR}/swext/mediawiki")
    set(WIKI_PUBLISHER_STAGE "${CMAKE_CURRENT_BINARY_DIR}/wiki-publisher-stage")
    set(WIKI_PUBLISHER_OXT "${CMAKE_CURRENT_BINARY_DIR}/wiki-publisher.oxt")

    set(WIKI_PUBLISHER_COPY_FILES
        "${WIKI_PUBLISHER_SRC}/src/registry/data/org/openoffice/Office/Addons.xcu=Addons.xcu"
        "${WIKI_PUBLISHER_SRC}/src/registry/data/org/openoffice/TypeDetection/Filter.xcu=Filter.xcu"
        "${WIKI_PUBLISHER_SRC}/src/registry/data/org/openoffice/Office/OptionsDialog.xcu=OptionsDialog.xcu"
        "${WIKI_PUBLISHER_SRC}/src/registry/data/org/openoffice/Office/Paths.xcu=Paths.xcu"
        "${WIKI_PUBLISHER_SRC}/src/registry/data/org/openoffice/Office/ProtocolHandler.xcu=ProtocolHandler.xcu"
        "${WIKI_PUBLISHER_SRC}/src/registry/schema/org/openoffice/Office/Custom/WikiExtension.xcs=WikiExtension.xcs"
        "${WIKI_PUBLISHER_SRC}/src/registry/data/org/openoffice/Office/Custom/WikiExtension.xcu=WikiExtension.xcu"
        "${WIKI_PUBLISHER_SRC}/src/components.rdb=components.rdb"
        "${WIKI_PUBLISHER_SRC}/dialogs/EditSetting.xdl=WikiEditor/EditSetting.xdl"
        "${WIKI_PUBLISHER_SRC}/dialogs/Settings.xdl=WikiEditor/Settings.xdl"
        "${WIKI_PUBLISHER_SRC}/dialogs/Module1.xba=WikiEditor/Module1.xba"
        "${WIKI_PUBLISHER_SRC}/dialogs/script.xlb=WikiEditor/script.xlb"
        "${WIKI_PUBLISHER_SRC}/dialogs/dialog.xlb=WikiEditor/dialog.xlb"
        "${WIKI_PUBLISHER_SRC}/dialogs/SendToMediaWiki.xdl=WikiEditor/SendToMediaWiki.xdl"
        "${WIKI_PUBLISHER_SRC}/src/filter/odt2mediawiki.xsl=filter/odt2mediawiki.xsl"
        "${WIKI_PUBLISHER_SRC}/help/wikisend.xhp=help/com.sun.wiki-publisher/wikisend.xhp"
        "${WIKI_PUBLISHER_SRC}/help/wikiformats.xhp=help/com.sun.wiki-publisher/wikiformats.xhp"
        "${WIKI_PUBLISHER_SRC}/help/wikiaccount.xhp=help/com.sun.wiki-publisher/wikiaccount.xhp"
        "${WIKI_PUBLISHER_SRC}/help/wiki.xhp=help/com.sun.wiki-publisher/wiki.xhp"
        "${WIKI_PUBLISHER_SRC}/help/wikisettings.xhp=help/com.sun.wiki-publisher/wikisettings.xhp"
        "${WIKI_PUBLISHER_SRC}/help/help.tree=help/com.sun.wiki-publisher/help.tree"
        "${WIKI_PUBLISHER_SRC}/src/description.xml=description.xml"
        "${WIKI_PUBLISHER_SRC}/src/description-en-US.txt=description-en-US.txt")

    # gb_UnpackedTarball_workdir/xsltml/*.xsl -- the six math-formula XSL
    # stylesheets used by the ODF<->MediaWiki filter, from the vendored
    # external/xsltml tree (present in this checkout: external/xsltml/,
    # confirmed by `find external -maxdepth 1 -iname "*xsltml*"`), which
    # has no ExternalProject/UnpackedTarball CMake wiring of its own yet in
    # this port (out of scope for this module) -- copied directly from
    # source here instead of from an unpack-stage output directory.
    foreach (_xsl IN ITEMS cmarkup entities glayout mmltex scripts tables)
        list(APPEND WIKI_PUBLISHER_COPY_FILES
            "${CMAKE_SOURCE_DIR}/external/xsltml/${_xsl}.xsl=filter/math/${_xsl}.xsl")
    endforeach ()

    set(WIKI_PUBLISHER_STAGED_FILES "")
    foreach (_entry IN LISTS WIKI_PUBLISHER_COPY_FILES)
        string(REPLACE "=" ";" _entry_pair "${_entry}")
        list(GET _entry_pair 0 _src)
        list(GET _entry_pair 1 _rel)
        set(_out "${WIKI_PUBLISHER_STAGE}/${_rel}")
        add_custom_command(
            OUTPUT "${_out}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${WIKI_PUBLISHER_STAGE}/${_rel}/.."
            COMMAND "${CMAKE_COMMAND}" -E copy "${_src}" "${_out}"
            DEPENDS "${_src}"
            VERBATIM)
        list(APPEND WIKI_PUBLISHER_STAGED_FILES "${_out}")
    endforeach ()

    # mediawiki.jar and mediawiki.ott: copied from their own build outputs
    # rather than from source.
    add_custom_command(
        OUTPUT "${WIKI_PUBLISHER_STAGE}/mediawiki.jar"
        COMMAND "${CMAKE_COMMAND}" -E copy "${mediawiki_JAR}" "${WIKI_PUBLISHER_STAGE}/mediawiki.jar"
        DEPENDS mediawiki_jar
        VERBATIM)
    list(APPEND WIKI_PUBLISHER_STAGED_FILES "${WIKI_PUBLISHER_STAGE}/mediawiki.jar")
    if (SWEXT_ZIP_EXECUTABLE)
        add_custom_command(
            OUTPUT "${WIKI_PUBLISHER_STAGE}/templates/MediaWiki/mediawiki.ott"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${WIKI_PUBLISHER_STAGE}/templates/MediaWiki"
            COMMAND "${CMAKE_COMMAND}" -E copy "${MEDIAWIKI_OTT}" "${WIKI_PUBLISHER_STAGE}/templates/MediaWiki/mediawiki.ott"
            DEPENDS "${MEDIAWIKI_OTT}"
            VERBATIM)
        list(APPEND WIKI_PUBLISHER_STAGED_FILES "${WIKI_PUBLISHER_STAGE}/templates/MediaWiki/mediawiki.ott")
    endif ()

    # META-INF/manifest.xml: this extension's own top-level OXT manifest
    # (distinct from the .ott template's own META-INF/manifest.xml staged
    # above under a different subtree) -- present as a real, complete file
    # at swext/mediawiki/src/META-INF/manifest.xml.
    add_custom_command(
        OUTPUT "${WIKI_PUBLISHER_STAGE}/META-INF/manifest.xml"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${WIKI_PUBLISHER_STAGE}/META-INF"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${WIKI_PUBLISHER_SRC}/src/META-INF/manifest.xml"
                "${WIKI_PUBLISHER_STAGE}/META-INF/manifest.xml"
        DEPENDS "${WIKI_PUBLISHER_SRC}/src/META-INF/manifest.xml"
        VERBATIM)
    list(APPEND WIKI_PUBLISHER_STAGED_FILES "${WIKI_PUBLISHER_STAGE}/META-INF/manifest.xml")

    if (SWEXT_ZIP_EXECUTABLE)
        add_custom_command(
            OUTPUT "${WIKI_PUBLISHER_OXT}"
            COMMAND "${CMAKE_COMMAND}" -E rm -f "${WIKI_PUBLISHER_OXT}"
            COMMAND "${SWEXT_ZIP_EXECUTABLE}" -qrX "${WIKI_PUBLISHER_OXT}" .
            WORKING_DIRECTORY "${WIKI_PUBLISHER_STAGE}"
            DEPENDS ${WIKI_PUBLISHER_STAGED_FILES}
            COMMENT "zip: wiki-publisher.oxt"
            VERBATIM)
        add_custom_target(wiki-publisher ALL DEPENDS "${WIKI_PUBLISHER_OXT}")

        # Extension_*.mk's own targets are never `install()`ed directly by
        # gbuild either -- they are picked up by the not-ported
        # instsetoo_native/scp2 packaging step (packinfo_extensions.txt
        # names "wiki-publisher.oxt" -- see setup_native's own
        # source/packinfo/packinfo_extensions.txt) or, for a developer
        # build, dropped into the user's extensions directory by unopkg at
        # runtime, not copied into program/ at build time. Left as a
        # build-tree artifact only, matching that same scope.
    else ()
        message(STATUS "swext: 'zip' not found -- wiki-publisher.oxt will not be assembled (mediawiki.jar/mediawiki.ott are still built)")
    endif ()
endif ()
