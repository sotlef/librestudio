# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# avmedia — Library_avmedia.mk's `ifneq (,$(filter AVMEDIA,$(BUILD_TYPE)))`
# block (lines 41-61) is now wired to the real ENABLE_AVMEDIA switch
# (cmake/LibreStudioConfigHeaders.cmake, default ON: configure.ac:1648-1651
# defaults enable_avmedia to yes outside --enable-wasm-strip, and
# configure.ac:3513-3515 appends AVMEDIA to BUILD_TYPE precisely then), so on
# this port's bare default it resolves the same way it does upstream: the
# framework/viewer sources below are built, avmedia.component is registered,
# and avmediagst/avmediagtk/avmediaqt6/avmediaMacAVF/avmediawin -- each its own
# separate Library_*.mk, gated by its own flag -- become real targets further
# down in this file. mediaitem.cxx (the SfxItem that carries a media URL/
# mime-type through the document model) stays unconditional either way,
# exactly as Library_avmedia.mk itself keeps it outside the AVMEDIA guard;
# svxcore links avmedia unconditionally for that reason alone even on a build
# with ENABLE_AVMEDIA off.

set(AVMEDIA_SOURCES
    avmedia/source/framework/mediaitem
)

# Library_avmedia.mk:41-61, the AVMEDIA-gated media-player framework/viewer
# sources: mediacontrol/MediaControlBase (the toolbar-embedded transport
# control), mediaplayer (the top-level player window), mediatoolbox (the
# toolbar controller that inserts a mediacontrol), soundhandler (the
# com.sun.star.frame.ContentHandler that plays a sound file dropped/opened
# directly), and viewer/mediaevent_impl + viewer/mediawindow[_impl] (the
# generic MediaWindow widget embedded wherever a document shows a video/audio
# object).
if (ENABLE_AVMEDIA)
    list(APPEND AVMEDIA_SOURCES
        avmedia/source/framework/mediacontrol
        avmedia/source/framework/MediaControlBase
        avmedia/source/framework/mediaplayer
        avmedia/source/framework/mediatoolbox
        avmedia/source/framework/soundhandler
        avmedia/source/viewer/mediaevent_impl
        avmedia/source/viewer/mediawindow
        avmedia/source/viewer/mediawindow_impl)
endif (ENABLE_AVMEDIA)

librestudio_expand_cxx(AVMEDIA_SOURCES)
add_library(avmedia SHARED ${AVMEDIA_SOURCES})

target_include_directories(avmedia PRIVATE
    "${CMAKE_SOURCE_DIR}/avmedia/inc"
    "${CMAKE_SOURCE_DIR}/avmedia/source/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(avmedia officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(avmedia PRIVATE AVMEDIA_DLLIMPLEMENTATION)

target_link_libraries(avmedia PRIVATE
    comphelper cppu cppuhelper sal svl ucbhelper vcl
    Boost::boost)

# Library_avmedia.mk:47-52's extra use_libraries, needed only by the
# AVMEDIA-gated sources above (i18nlangtag/svt/tl/utl are not pulled in by
# mediaitem.cxx alone).
if (ENABLE_AVMEDIA)
    target_link_libraries(avmedia PRIVATE
        i18nlangtag svt tl utl)
endif (ENABLE_AVMEDIA)

# gb_Library_set_componentfile(avmedia,avmedia/util/avmedia,services) --
# avmedia/util/avmedia.component registers the SoundHandler service
# (com.sun.star.comp.framework.SoundHandler), declared by soundhandler.cxx
# above, so this can only be registered under the same ENABLE_AVMEDIA guard
# that builds that source. avmedia is an OOO-layer library (Repository.mk:388,
# registered under OOOLIBS/ooo, not a URE one), matching the OOO layer already
# used for its output name below.
if (ENABLE_AVMEDIA)
    librestudio_add_component(avmedia
        COMPONENT "${CMAKE_SOURCE_DIR}/avmedia/util/avmedia.component"
        LIBRARY   avmedia
        LAYER     OOO)
endif (ENABLE_AVMEDIA)

librestudio_set_library_output_name(avmedia avmedia OOO)

install(TARGETS avmedia LIBRARY DESTINATION program)

# --- avmediagst: GStreamer 1.0 backend --------------------------------------
#
# Library_avmediagst.mk. One of five per-backend avmedia libraries (Repository.
# mk:671-677, all PLAINLIBS_OOO/ooo, all gb_Helper_optional(AVMEDIA, ...) on
# top of their own individual flag) -- this is the Linux/GStreamer one.
#
# ENABLE_GSTREAMER_1_0 has no reader elsewhere in this port yet, so it stays
# local here rather than moving to cmake/LibreStudioConfigHeaders.cmake --
# the same locality choice vcl/CMakeLists.txt made for ENABLE_QT6 (nothing to
# centralise until a second reader exists). vcl/inc/unx/gstsink.hxx reads a
# same-spelled ENABLE_GSTREAMER_1_0 macro too, but that one comes from
# cmake/config/config_vclplug.h.in (a hardcoded 0, generated separately) and
# gates a vcl-internal GStreamer test-pattern sink, not this library -- the
# two are unrelated and this option does not touch that header.
#
# Real configure.ac default: test_gstreamer_1_0 follows using_x11
# (configure.ac:1321), which this port's cmake/LibreStudioConfigHeaders.cmake
# hardcodes TRUE for a Linux/X11 desktop build, and enable_gstreamer_1_0
# itself defaults to yes (configure.ac:2027-2030,
# libo_FUZZ_ARG_ENABLE(gstreamer-1-0, ...)). So a bare ./configure on this
# platform reaches configure.ac:12998-13011 whenever avmedia is on too --
# and PKG_CHECK_MODULES there (line 13004) has no action-if-not-found, so
# upstream actually *requires* the four GStreamer pkg-config modules once
# reached, rather than silently falling back. This port instead follows the
# softer, already-established ENABLE_GTK3/ENABLE_AVAHI shape in
# cmake/LibreStudioConfigHeaders.cmake -- autodetect the default from the
# packages being present -- so a machine without the four modules still
# configures, just without this backend, rather than failing configure the
# way upstream's bare PKG_CHECK_MODULES would. Forcing this ON by hand on
# such a machine still fails, via the REQUIRED pkg_check_modules below --
# same fallback-with-a-clear-error shape as vclplug_gtk3's PkgConfig::GTK3.
pkg_check_modules(GSTREAMER_1_0_DETECT
    gstreamer-1.0 gstreamer-plugins-base-1.0 gstreamer-pbutils-1.0 gstreamer-video-1.0)
if (ENABLE_AVMEDIA AND USING_X11 AND GSTREAMER_1_0_DETECT_FOUND)
    set(_enable_gstreamer_1_0_default TRUE)
else ()
    set(_enable_gstreamer_1_0_default FALSE)
endif ()
set(ENABLE_GSTREAMER_1_0 ${_enable_gstreamer_1_0_default} CACHE BOOL
    "configure.ac's ENABLE_GSTREAMER_1_0 (real default follows using_x11 + the four gstreamer-*.pc modules being present, both true on this machine's Linux/X11 build once found): build avmediagst, the GStreamer 1.0 avmedia backend")
unset(_enable_gstreamer_1_0_default)

if (ENABLE_GSTREAMER_1_0)
    set(AVMEDIAGST_SOURCES
        avmedia/source/gstreamer/gstmanager
        avmedia/source/gstreamer/gstplayer
        avmedia/source/gstreamer/gstwindow
        avmedia/source/gstreamer/gstframegrabber
    )
    librestudio_expand_cxx(AVMEDIAGST_SOURCES)
    add_library(avmediagst SHARED ${AVMEDIAGST_SOURCES})

    target_include_directories(avmediagst PRIVATE
        "${CMAKE_SOURCE_DIR}/avmedia/source/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(avmediagst officecfg_headers udkapi_normal_headers offapi_normal_headers)

    pkg_check_modules(GSTREAMER_1_0 REQUIRED IMPORTED_TARGET
        gstreamer-1.0 gstreamer-plugins-base-1.0 gstreamer-pbutils-1.0 gstreamer-video-1.0)

    target_link_libraries(avmediagst PRIVATE
        comphelper cppu cppuhelper sal salhelper tl vcl avmedia
        Boost::boost
        PkgConfig::GSTREAMER_1_0)

    librestudio_add_component(avmediagst
        COMPONENT "${CMAKE_SOURCE_DIR}/avmedia/source/gstreamer/avmediagstreamer.component"
        LIBRARY   avmediagst
        LAYER     OOO)

    librestudio_set_library_output_name(avmediagst avmediagst OOO)
    install(TARGETS avmediagst LIBRARY DESTINATION program)
endif ()

# --- avmediagtk: GTK4 backend ------------------------------------------------
#
# Library_avmediagtk.mk. Reuses avmedia/source/gstreamer/gstwindow.cxx via
# #include (confirmed by reading avmedia/source/gtk/gstwindow.cxx: it is a
# one-line `#include "../gstreamer/gstwindow.cxx"`), hence the extra include
# path below.
#
# ENABLE_GTK4 is unlike ENABLE_GSTREAMER_1_0/ENABLE_GTK3 above: it is never
# autodetected from the package being present. configure.ac:1915-1917
# (AC_ARG_ENABLE(gtk4, ...)) gives enable_gtk4 no default of its own when the
# flag is absent -- no `action-if-not-given` clause at all, unlike gtk3/
# gstreamer's `test "${enable_x+set}" = set || enable_x=yes` -- and
# configure.ac:12750 then requires the literal string "yes"
# (`test "$test_gtk4" = yes -a "x$enable_gtk4" = "xyes"`), so a bare
# ./configure leaves GTK4 off even on a machine with libgtk4 installed; only
# an explicit --enable-gtk4 turns it on. OFF here matches that real default
# exactly, rather than mirroring the GStreamer/GTK3 autodetection shape.
option(ENABLE_GTK4
    "configure.ac's --enable-gtk4 (real default OFF -- unlike GStreamer/GTK3, upstream never turns this on from package presence alone, only from an explicit --enable-gtk4): build avmediagtk, the GTK4 avmedia backend" OFF)

if (ENABLE_GTK4)
    set(AVMEDIAGTK_SOURCES
        avmedia/source/gtk/gstwindow
        avmedia/source/gtk/gtkmanager
        avmedia/source/gtk/gtkplayer
    )
    librestudio_expand_cxx(AVMEDIAGTK_SOURCES)
    add_library(avmediagtk SHARED ${AVMEDIAGTK_SOURCES})

    target_include_directories(avmediagtk PRIVATE
        "${CMAKE_SOURCE_DIR}/avmedia/source/inc"
        "${CMAKE_SOURCE_DIR}/avmedia/source/gstreamer"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(avmediagtk officecfg_headers udkapi_normal_headers offapi_normal_headers)

    # configure.ac:12755, the same modules the (not yet ported) vclplug_gtk4
    # would ask for.
    pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET
        gtk4>=4.10 gmodule-no-export-2.0 glib-2.0>=2.38 cairo)

    target_link_libraries(avmediagtk PRIVATE
        comphelper cppu cppuhelper sal salhelper tl vcl
        Boost::boost
        PkgConfig::GTK4)

    librestudio_add_component(avmediagtk
        COMPONENT "${CMAKE_SOURCE_DIR}/avmedia/source/gtk/avmediagtk.component"
        LIBRARY   avmediagtk
        LAYER     OOO)

    librestudio_set_library_output_name(avmediagtk avmediagtk OOO)
    install(TARGETS avmediagtk LIBRARY DESTINATION program)
endif ()

# --- avmediaqt6: Qt6 QtMultimedia backend -----------------------------------
#
# Library_avmediaqt6.mk + CustomTarget_avmediaqt6_moc.mk. Distinct from
# vcl/CMakeLists.txt's ENABLE_QT6 (that one gates vclplug_qt6, the VCL widget
# back end); this is Qt6's separate multimedia module, gated by its own
# ENABLE_QT6_MULTIMEDIA, though it still needs ENABLE_QT6 (or KF6, unsupported
# here) to be on first -- see the requirement check below.
#
# Real configure.ac default: enable_qt6_multimedia (configure.ac:1978-1980,
# bare AC_ARG_ENABLE with no action-if-not-given) is only ever consulted
# inside the "qt6 or kf6 integration" block (configure.ac:14192-14345), and
# entering that block itself requires --enable-qt6 or --enable-kf6 given
# explicitly (configure.ac:1972-1976, itself no-default). This port's
# ENABLE_QT6 (vcl/CMakeLists.txt) defaults OFF, so a bare ./configure here
# never reaches the qt6-multimedia check at all -- ENABLE_QT6_MULTIMEDIA
# defaults OFF for the same reason ENABLE_QT6 does, not as an independent
# choice. configure.ac:14552-14554 hard-errors
# ("--enable-qt6-multimedia requires --enable-qt6") if it is requested
# without QT6; reproduced below as a configure-time FATAL_ERROR rather than
# silently ignoring the request.
option(ENABLE_QT6_MULTIMEDIA
    "configure.ac's --enable-qt6-multimedia (real default follows ENABLE_QT6, which defaults OFF -- see vcl/CMakeLists.txt): build avmediaqt6, the Qt6 QtMultimedia avmedia backend" OFF)

if (ENABLE_QT6_MULTIMEDIA AND NOT ENABLE_QT6)
    message(FATAL_ERROR
        "ENABLE_QT6_MULTIMEDIA requires ENABLE_QT6 (configure.ac:14552-14554: "
        "--enable-qt6-multimedia requires --enable-qt6 or --enable-kf6)")
endif ()

if (ENABLE_QT6_MULTIMEDIA)
    # vcl/CMakeLists.txt's own ENABLE_QT6 block (add_subdirectory(vcl) at
    # CMakeLists.txt:496, textually before add_subdirectory(avmedia) at
    # CMakeLists.txt:549) already ran pkg_check_modules(QT6 REQUIRED
    # IMPORTED_TARGET Qt6Core Qt6Gui Qt6Widgets) and found QT6_MOC by the time
    # this block runs -- but its `IMPORTED_TARGET` target (PkgConfig::QT6) is
    # scoped to vcl/CMakeLists.txt's own directory, not visible here, so the
    # pkg-config modules are re-checked below under this library's own name.
    # QT6_MOC itself is a plain find_program() CACHE variable, which *is*
    # visible from any directory once set, so it is reused as-is rather than
    # re-found.
    #
    # configure.ac's real QT6_LIBS composition, once qt6-multimedia is
    # enabled, is Qt6Core+Qt6Gui+Qt6Widgets (unconditional QT6 block) plus
    # Qt6Multimedia+Qt6MultimediaWidgets (configure.ac:14342, the Linux/X11
    # branch) -- reproduced here as one pkg_check_modules call. configure.ac
    # additionally links QT6_XCB under USING_X11 (configure.ac:14347-14353)
    # for X11 key-modifier handling, but vcl/CMakeLists.txt's own vclplug_qt6
    # does not wire that in yet either (grepped: no XCB reference there), so
    # it is left as the same honest, pre-existing gap here rather than fixed
    # only for this one consumer.
    pkg_check_modules(QT6_MULTIMEDIA REQUIRED IMPORTED_TARGET
        Qt6Core Qt6Gui Qt6Widgets Qt6Multimedia Qt6MultimediaWidgets)

    if (NOT QT6_MOC)
        message(FATAL_ERROR
            "ENABLE_QT6_MULTIMEDIA: QT6_MOC was not found -- expected it to "
            "already be set by vcl/CMakeLists.txt's ENABLE_QT6 block")
    endif ()

    # CustomTarget_avmediaqt6_moc.mk: moc runs directly on the two qt6
    # headers that declare Q_OBJECT (grepped: QtFrameGrabber.hxx and
    # QtPlayer.hxx only, matching the .mk's own two-file output list) --
    # unlike vclplug_qt6's own moc step, avmedia/source/qt6/*.hxx are real
    # headers here, not qt5-forwarding shims, so moc reads them directly
    # rather than a qt5 sibling.
    set(AVMEDIAQT6_MOC_HEADERS QtFrameGrabber QtPlayer)
    set(AVMEDIAQT6_MOC_DIR "${CMAKE_CURRENT_BINARY_DIR}/avmediaqt6-moc")
    file(MAKE_DIRECTORY "${AVMEDIAQT6_MOC_DIR}")

    set(AVMEDIAQT6_MOC_OUTPUTS)
    foreach (_avmediaqt6_moc_name IN LISTS AVMEDIAQT6_MOC_HEADERS)
        set(_avmediaqt6_moc_header
            "${CMAKE_SOURCE_DIR}/avmedia/source/qt6/${_avmediaqt6_moc_name}.hxx")
        set(_avmediaqt6_moc_output
            "${AVMEDIAQT6_MOC_DIR}/${_avmediaqt6_moc_name}.moc")
        add_custom_command(
            OUTPUT "${_avmediaqt6_moc_output}"
            COMMAND "${QT6_MOC}" "${_avmediaqt6_moc_header}" -o "${_avmediaqt6_moc_output}"
            DEPENDS "${_avmediaqt6_moc_header}"
            COMMENT "MOC ${_avmediaqt6_moc_name}"
            VERBATIM)
        list(APPEND AVMEDIAQT6_MOC_OUTPUTS "${_avmediaqt6_moc_output}")
    endforeach ()
    unset(_avmediaqt6_moc_name)
    unset(_avmediaqt6_moc_header)
    unset(_avmediaqt6_moc_output)

    add_custom_target(avmediaqt6_moc DEPENDS ${AVMEDIAQT6_MOC_OUTPUTS})

    # Library_avmediaqt6.mk's add_exception_objects; gstwindow.cxx here is
    # again a one-line #include of the gstreamer one (confirmed the same way
    # as avmediagtk's), hence the extra include path.
    set(AVMEDIAQT6_SOURCES
        avmedia/source/qt6/gstwindow
        avmedia/source/qt6/QtFrameGrabber
        avmedia/source/qt6/QtManager
        avmedia/source/qt6/QtPlayer
    )
    librestudio_expand_cxx(AVMEDIAQT6_SOURCES)
    add_library(avmediaqt6 SHARED ${AVMEDIAQT6_SOURCES})

    # gb_Library_use_custom_headers(avmediaqt6,avmedia/source/qt6): forwards
    # the moc CustomTarget's own output dir so #include "X.moc" (Qt's own
    # convention, confirmed present in QtFrameGrabber.cxx/QtPlayer.cxx) finds
    # its generated file.
    add_dependencies(avmediaqt6
        avmediaqt6_moc officecfg_headers udkapi_normal_headers offapi_normal_headers)
    target_include_directories(avmediaqt6 PRIVATE
        "${CMAKE_SOURCE_DIR}/avmedia/source/inc"
        "${CMAKE_SOURCE_DIR}/avmedia/source/gstreamer"
        "${AVMEDIAQT6_MOC_DIR}"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}")

    target_link_libraries(avmediaqt6 PRIVATE
        comphelper cppu cppuhelper i18nlangtag sal salhelper tl vcl
        Boost::boost
        PkgConfig::QT6_MULTIMEDIA)

    librestudio_add_component(avmediaqt6
        COMPONENT "${CMAKE_SOURCE_DIR}/avmedia/source/qt6/avmediaqt.component"
        LIBRARY   avmediaqt6
        LAYER     OOO)

    librestudio_set_library_output_name(avmediaqt6 avmediaqt6 OOO)
    install(TARGETS avmediaqt6 LIBRARY DESTINATION program)
endif ()

# --- avmediaMacAVF: macOS AVFoundation backend ------------------------------
#
# Library_avmediaMacAVF.mk. Structurally present but unverified/best-effort,
# same status as sal/CMakeLists.txt's own APPLE .mm branch and vcl's recent
# macOS platform-tail work: no macOS SDK on this machine to confirm the
# Objective-C++ (.mm) sources actually compile/link against
# AVFoundation/Cocoa/CoreMedia. The four sources on disk are real .mm files
# (avmedia/source/macavf/framegrabber.mm etc., confirmed by listing the
# directory), which CMake recognises as Objective-C++ automatically on Apple
# platforms -- the same reasoning sal/CMakeLists.txt's uunxapi.mm/system.mm
# branch already documents -- so they are named directly rather than through
# librestudio_expand_cxx (which would append the wrong, .cxx extension).
if (APPLE)
    add_library(avmediaMacAVF SHARED
        "${CMAKE_SOURCE_DIR}/avmedia/source/macavf/framegrabber.mm"
        "${CMAKE_SOURCE_DIR}/avmedia/source/macavf/manager.mm"
        "${CMAKE_SOURCE_DIR}/avmedia/source/macavf/player.mm"
        "${CMAKE_SOURCE_DIR}/avmedia/source/macavf/window.mm")

    target_include_directories(avmediaMacAVF PRIVATE
        "${CMAKE_SOURCE_DIR}/avmedia/source/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(avmediaMacAVF officecfg_headers udkapi_normal_headers offapi_normal_headers)

    target_link_libraries(avmediaMacAVF PRIVATE
        comphelper cppu cppuhelper sal tl vcl
        Boost::boost
        "-framework AVFoundation" "-framework Cocoa" "-framework CoreMedia")

    librestudio_add_component(avmediaMacAVF
        COMPONENT "${CMAKE_SOURCE_DIR}/avmedia/source/macavf/avmediaMacAVF.component"
        LIBRARY   avmediaMacAVF
        LAYER     OOO)

    librestudio_set_library_output_name(avmediaMacAVF avmediaMacAVF OOO)
    install(TARGETS avmediaMacAVF LIBRARY DESTINATION program)
endif ()

# --- avmediawin: Windows Media Foundation backend ---------------------------
#
# Library_avmediawin.mk. Structurally present but unverified/best-effort,
# same status as this port's other Windows/MSVC branches (e.g. sal/
# CMakeLists.txt's SAL_W32_SOURCES): no cl.exe/ml64.exe/SDK on this machine
# to confirm this actually compiles/links.
if (WIN32)
    set(AVMEDIAWIN_SOURCES
        avmedia/source/win/framegrabber
        avmedia/source/win/manager
        avmedia/source/win/player
        avmedia/source/win/window
    )
    librestudio_expand_cxx(AVMEDIAWIN_SOURCES)
    add_library(avmediawin SHARED ${AVMEDIAWIN_SOURCES})

    target_include_directories(avmediawin PRIVATE
        "${CMAKE_SOURCE_DIR}/avmedia/source/inc"
        "${OFFICECFG_INCLUDE_DIR}"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(avmediawin officecfg_headers udkapi_normal_headers offapi_normal_headers)

    target_link_libraries(avmediawin PRIVATE
        comphelper cppu cppuhelper sal tl utl vcl avmedia
        Boost::boost
        gdi32 ole32 oleaut32 strmiids propsys shlwapi
        mf mfuuid mfplat mfplay mfreadwrite)

    librestudio_add_component(avmediawin
        COMPONENT "${CMAKE_SOURCE_DIR}/avmedia/source/win/avmediawin.component"
        LIBRARY   avmediawin
        LAYER     OOO)

    librestudio_set_library_output_name(avmediawin avmediawin OOO)
    install(TARGETS avmediawin LIBRARY DESTINATION program)
endif ()
