# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# pyuno — the Python-UNO bridge: three libraries, in dependency order:
#  - pyuno:         the actual bridge (pyuno_runtime and friends) that
#                    embeds/talks to Python and marshals UNO values back and
#                    forth. Built and installed as a bare "pyuno" module (see
#                    the PLAIN layer note below), not "pyunolo".
#  - pythonloader:  the UNO component (org.openoffice.comp.pyuno.Loader,
#                    service com.sun.star.loader.Python) that lets the UNO
#                    service manager load Python-implemented services; links
#                    against pyuno above.
#  - pyuno_wrapper: a tiny C-only dlopen trampoline. Python's "import pyuno"
#                    loads it (as RTLD_LOCAL); it in turn dlopens libpyuno.so
#                    itself with RTLD_GLOBAL. See the comment above its
#                    add_library() below for why it must stay pure C.
# Source lists from Library_pyuno.mk, Library_pythonloader.mk and
# Library_pyuno_wrapper.mk.
#
# Module gate: Module_pyuno.mk wraps everything in
# `ifneq ($(DISABLE_PYTHON),TRUE)`. This port has no DISABLE_PYTHON switch
# (nothing sets it), and the machine has Python 3 development headers and a
# linkable libpython (`pkg-config --exists python3` and `python3-embed` both
# succeed, python3.12), so the module is ported unconditionally.
#
# Library_pyuno_wrapper.mk is itself further gated `ifneq ($(OS),WNT)` in
# Module_pyuno.mk — true unconditionally on this Linux-only port, so
# pyuno_wrapper is ported like the other two, not treated as optional.
#
# Out of scope, and why (this file only ports the three Library_* targets the
# porting task named; every other Module_pyuno.mk target needs
# infrastructure this port does not have yet):
#   - CustomTarget_pyuno_pythonloader_ini / Package_pyuno_pythonloader_ini:
#     generates and installs pythonloader.ini (Python module search path for
#     the loader). No CustomTarget-install/Package precedent exists in this
#     port for a plain text config file install; deferred along with it.
#   - Package_python_scripts: installs the Scripting Framework's bundled
#     .py helper scripts (pyuno/zipcore, pyuno/demo) — a resource install,
#     not a build target.
#   - Rdb_pyuno: assembles ure/share/misc/pyuno.rdb from pythonloader's
#     .component file via xsltproc/packcomponents.xslt. No module in this
#     port yet builds its own Rdb this way (the port's librestudio_add_
#     component() records a component for a *pre-existing* services.rdb-style
#     install, it does not drive the xsltproc packcomponents step itself),
#     so this is deferred rather than improvised.
#   - Executable_python (WNT-only), CustomTarget_python_shell /
#     Package_python_shell (non-SYSTEM_PYTHON only — this build uses the
#     system Python, so SYSTEM_PYTHON is effectively TRUE and these are
#     skipped upstream too), and every PythonTest_*/CppunitTest_* check
#     target: test/tooling infrastructure, not production build output.

# --- external: python ---------------------------------------------------------
#
# gbuild's "python" external (RepositoryExternal.mk) is PYTHON_CFLAGS (include
# path) + PYTHON_LIBS (link against libpython) for a system Python build --
# python3-embed is the pkg-config module that carries both on this
# distribution (plain "python3" ships headers only, with an empty Libs: it is
# meant for extension modules that get dlopen'd into an already-running
# interpreter, which is exactly why it is used for pyuno_wrapper below instead
# of this one). Confirmed present: `pkg-config --exists python3-embed`
# succeeds, version 3.12, matching `pkg-config --cflags python3`
# (-I/usr/include/python3.12).
pkg_check_modules(PYTHON3 REQUIRED IMPORTED_TARGET python3-embed)

# --- pyuno -------------------------------------------------------------------

set(PYUNO_SOURCES
    pyuno/source/module/pyuno_runtime
    pyuno/source/module/pyuno
    pyuno/source/module/pyuno_struct
    pyuno/source/module/pyuno_callable
    pyuno/source/module/pyuno_module
    pyuno/source/module/pyuno_type
    pyuno/source/module/pyuno_util
    pyuno/source/module/pyuno_except
    pyuno/source/module/pyuno_adapter
    pyuno/source/module/pyuno_gc
    pyuno/source/module/pyuno_iterator
    pyuno/source/module/pyuno_service_constructor
)

librestudio_expand_cxx(PYUNO_SOURCES)
add_library(pyuno SHARED ${PYUNO_SOURCES})

target_include_directories(pyuno PRIVATE
    "${CMAKE_SOURCE_DIR}/pyuno/source/loader"
    "${CMAKE_SOURCE_DIR}/pyuno/inc"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(pyuno udkapi_normal_headers offapi_normal_headers)

target_compile_definitions(pyuno PRIVATE LO_DLLIMPLEMENTATION_PYUNO)

target_link_libraries(pyuno PRIVATE
    cppu cppuhelper sal salhelper
    PkgConfig::PYTHON3)

# No gb_Library_set_componentfile in Library_pyuno.mk -- pyuno is a native
# Python module (imported as "import pyuno"/"import uno"), not itself
# registered as a UNO service.

# Repository.mk registers pyuno under PLAINLIBS_OOO (Repository.mk line 559),
# not OOOLIBS: bare "libpyuno.so", no "lo" suffix, same PLAIN naming scheme as
# officebean/jpipe/juh already in this port.
librestudio_set_library_output_name(pyuno pyuno PLAIN)

install(TARGETS pyuno LIBRARY DESTINATION program)

# --- pythonloader --------------------------------------------------------------

set(PYTHONLOADER_SOURCES
    pyuno/source/loader/pyuno_loader
)

librestudio_expand_cxx(PYTHONLOADER_SOURCES)
add_library(pythonloader SHARED ${PYTHONLOADER_SOURCES})

# gb_Library_use_custom_headers,pythonloader,officecfg/registry.
target_include_directories(pythonloader PRIVATE
    "${CMAKE_SOURCE_DIR}/pyuno/source/loader"
    "${CMAKE_SOURCE_DIR}/pyuno/inc"
    "${OFFICECFG_INCLUDE_DIR}"
    "${udkapi_normal_HEADERS_INCLUDE_DIR}"
    "${offapi_normal_HEADERS_INCLUDE_DIR}")
add_dependencies(pythonloader
    officecfg_headers udkapi_normal_headers offapi_normal_headers)

target_link_libraries(pythonloader PRIVATE
    comphelper cppu cppuhelper pyuno sal
    PkgConfig::PYTHON3)

# gb_Library_set_componentfile,pythonloader,pyuno/source/loader/pythonloader,
# pyuno: the third argument is the Rdb name ("pyuno", not "services"), but
# Rdb_pyuno.mk's `gb_Rdb_Rdb_install,pyuno` (no second/install-path argument)
# installs it at the same $(INSTROOT)/$(LIBO_ETC_FOLDER)/services/ location
# family as the plain "services" rdb every other OOO-layer component in this
# port registers against -- so LAYER OOO here, same as e.g. ucb's ucpgio1 and
# shell's desktopbe1/localebe1 below, not LAYER URELIB.
librestudio_add_component(pythonloader
    COMPONENT "${CMAKE_SOURCE_DIR}/pyuno/source/loader/pythonloader.component"
    LIBRARY   pythonloader
    LAYER     OOO)

librestudio_set_library_output_name(pythonloader pythonloader OOO)

install(TARGETS pythonloader LIBRARY DESTINATION program)

# --- pyuno_wrapper ---------------------------------------------------------

# Library_pyuno_wrapper.mk's own comment (reproduced from the .mk): Python's
# "import pyuno" dlopens pyuno.so as RTLD_LOCAL, so g++ exception handling
# used to not work there; pyuno.so is therefore just a thin wrapper that
# dlopens libpyuno.so as RTLD_GLOBAL. Linking pyuno.so against libstdc++
# (which has not yet been loaded into the Python process) risks resolving a
# symbol libpyuno.so also defines to the wrong DSO once RTLD_GLOBAL pulls
# libpyuno.so in, so the wrapper must link with the plain C compiler/runtime,
# never C++ -- hence a single .c translation unit and no gb_Library_
# add_exception_objects here, only gb_Library_add_cobjects.
add_library(pyuno_wrapper SHARED
    "${CMAKE_SOURCE_DIR}/pyuno/source/module/pyuno_dlopenwrapper.c")

target_include_directories(pyuno_wrapper PRIVATE
    "${CMAKE_SOURCE_DIR}/pyuno/source/module"
    "${CMAKE_SOURCE_DIR}/pyuno/inc")

# "not using external python because we do not want to link against python" --
# gb_Library_use_externals,pyuno_wrapper,python_headers only, i.e. cflags/
# include path but no libpython link. plain "python3" (as opposed to
# python3-embed above) is exactly that on this distribution: its Libs: field
# is empty (`pkg-config --libs python3` prints nothing), only Cflags:
# -I/usr/include/python3.12, so linking PkgConfig::PYTHON3_HEADERS adds an
# include path and no library, matching the .mk's intent.
pkg_check_modules(PYTHON3_HEADERS REQUIRED IMPORTED_TARGET python3)
target_link_libraries(pyuno_wrapper PRIVATE PkgConfig::PYTHON3_HEADERS)

# `ifeq ($(filter DRAGONFLY FREEBSD NETBSD OPENBSD MACOSX,$(OS)),)` --
# unconditionally true on this Linux port, so -ldl is always added.
target_link_libraries(pyuno_wrapper PRIVATE ${CMAKE_DL_LIBS})

# RepositoryFixes.mk (lines 71-72) renames the Linux runtime file:
#   libpyuno_wrapper.so => pyuno.so
# This is the file Python's "import pyuno" actually finds (libpyuno.so above
# is a different file, dlopen'd from inside this one, never imported
# directly), so neither the PLAIN nor OOO naming helper applies here -- both
# would keep a "lib" prefix this target must not have.
set_target_properties(pyuno_wrapper PROPERTIES OUTPUT_NAME "pyuno" PREFIX "")

install(TARGETS pyuno_wrapper LIBRARY DESTINATION program)

# pyuno/CppunitTest_deferred.cmake needs data (program_services_rdb) from a
# module add_subdirectory()'d later than this one -- scheduled here rather
# than included directly, so it runs only once every add_subdirectory() in
# the whole tree has completed. See that file's own header comment for the
# specific constraint, and cmake/LibreStudioTest.cmake's header comment for
# why CMAKE_SOURCE_DIR (not CMAKE_CURRENT_SOURCE_DIR) is required in the
# path argument below.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include
    "${CMAKE_SOURCE_DIR}/pyuno/CppunitTest_deferred.cmake")
