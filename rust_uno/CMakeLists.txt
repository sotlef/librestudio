# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# rust_uno -- experimental Rust language bindings for UNO.
#
# Module_rust_uno.mk, read in full: its one gb_Module_add_targets call
# (CustomTarget_cargo, CustomTarget_rustmaker, Extension_rust_uno-example,
# Library_rust_uno-cpp, Library_rust_uno-example) is gated
# `ifeq ($(ENABLE_RUST_UNO),TRUE)`. configure.ac:3108-3110 defines
# --enable-rust-uno via a bare AC_ARG_ENABLE with no action-if-not-given, so
# an unset $enable_rust_uno never takes the "yes" branch at configure.ac:6027
# (`if test "$enable_rust_uno" = "yes"; then ENABLE_RUST_UNO=TRUE ... else
# ENABLE_RUST_UNO= fi`) -- this is a bare-configure-default-OFF, explicitly
# opt-in "experimental" feature (per its own --help text), the same shape as
# ENABLE_ONLINE_UPDATE in cmake/LibreStudioConfigHeaders.cmake, not the
# on-by-default ENABLE_SCRIPTING/ENABLE_AVMEDIA group. Defined locally here
# (rather than in cmake/LibreStudioConfigHeaders.cmake) since nothing else in
# the tree references it.
option(ENABLE_RUST_UNO
    "configure.ac's --enable-rust-uno (bare-configure default OFF, explicitly opt-in/experimental): build the Rust UNO language-binding generator, its C++ glue library, and the example component"
    OFF)

if (ENABLE_RUST_UNO)
    # The real pipeline, verified from every .mk this module's targets touch:
    #
    #   1. CustomTarget_rustmaker.mk runs codemaker's `rustmaker` executable
    #      against udkapi/offapi's .rdb, emitting generated C++ (-Ocpp, into
    #      this CustomTarget's own workdir) AND generated Rust source
    #      (-Orust, directly into rust_uno/src/generated/ in the source tree)
    #      -- a real codegen step, output consumed by both steps 2 and 3.
    #   2. CustomTarget_cargo.mk then runs `cargo build --release` in
    #      rust_uno/ itself (Cargo.toml/Cargo.lock/build.rs/src/ already
    #      present in this tree), producing target/release/librust_uno.so.
    #   3. Library_rust_uno-cpp.mk compiles the CustomTarget_rustmaker-
    #      generated C++ bindings plus this module's own uno_bootstrap.cxx,
    #      linking cppu/cppuhelper/sal plus udkapi/offapi headers
    #      (gb_Library_use_sdk_api) -- no dependency on cargo's .so itself.
    #   4. Library_rust_uno-example.mk/Extension_rust_uno-example.mk are a
    #      demo UNO component that links against cargo's librust_uno.so
    #      directly (-L.../target/release -lrust_uno, or the Windows
    #      rust_uno.dll.lib import library) plus cppu/cppuhelper/sal.
    #
    # Step 1's prerequisite -- codemaker's own `rustmaker` executable -- was
    # ported 2026-09-29 (codemaker/CMakeLists.txt's own "--- rustmaker ---"
    # block); an earlier version of this comment said its own external,
    # `frozen`, was not vendored anywhere in this port -- that had gone
    # stale, root CMakeLists.txt already resolves FROZEN_INCLUDE_DIR.
    #
    # 2026-09-29, same day: all 4 remaining pipeline steps are ALSO now
    # written below. None of this pass's add_custom_command() COMMAND lines
    # run at CMake configure time -- only when ninja is actually asked to
    # build one of their OUTPUT files, which has deliberately not happened
    # in this pass (the codegen step's own real recipe does a destructive
    # `rm -fr`/mkdir of rust_uno/src/generated IN THE SOURCE TREE, matching
    # CustomTarget_rustmaker.mk's own real recipe exactly, not this port's
    # invention -- Cargo needs the generated .rs files to be real files
    # under its own [lib] path, not a build-dir output; and CustomTarget_
    # cargo's own step is a real external build-system invocation, `cargo
    # build --release`, this port's first). Writing the CMake code and
    # reconfiguring is inert either way -- nothing here has actually run.
    find_program(CARGO_EXECUTABLE cargo REQUIRED)

    # --- CustomTarget_rustmaker.mk (pipeline step 1: codegen) ---------------
    #
    # rustmaker's own -Ocpp/-Orust invocation against offapi/udkapi's real
    # .rdb files: ${offapi_RDB}/${udkapi_RDB} are real CACHE INTERNAL
    # variables (cmake/LibreStudioUnoApi.cmake's own gb_UnoApi-equivalent
    # helper), already used the exact same way by this port's own root
    # CMakeLists.txt (program/types.rdb's own install step). Single combined
    # output file "rust_uno_bindings.cxx", matching Library_rust_uno-cpp.mk's
    # own rust_uno_generated_cxx list (one combined file, not one per IDL
    # type -- that .mk's own comment explains why: "instead of thousands of
    # individual files").
    set(RUST_UNO_RUSTMAKER_CPP_DIR "${CMAKE_CURRENT_BINARY_DIR}/rustmaker/cpp")
    set(RUST_UNO_GENERATED_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/generated")
    set(RUST_UNO_BINDINGS_CXX "${RUST_UNO_RUSTMAKER_CPP_DIR}/rust_uno_bindings.cxx")

    add_custom_command(
        OUTPUT "${RUST_UNO_BINDINGS_CXX}"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${RUST_UNO_GENERATED_SRC_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RUST_UNO_GENERATED_SRC_DIR}"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${RUST_UNO_RUSTMAKER_CPP_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RUST_UNO_RUSTMAKER_CPP_DIR}"
        COMMAND $<TARGET_FILE:rustmaker>
                -Ocpp "${RUST_UNO_RUSTMAKER_CPP_DIR}"
                -Orust "${RUST_UNO_GENERATED_SRC_DIR}"
                "${offapi_RDB}" "${udkapi_RDB}"
        DEPENDS rustmaker offapi_rdb udkapi_rdb
        COMMENT "rustmaker: generating rust_uno C++/Rust UNO bindings"
        VERBATIM)
    add_custom_target(rust_uno_rustmaker DEPENDS "${RUST_UNO_BINDINGS_CXX}")

    # --- Library_rust_uno-cpp.mk (pipeline step 3 in the header comment's
    # own numbering, written here because CustomTarget_cargo's own real
    # recipe depends on it being built first) ------------------------------
    #
    # No gb_Library_set_componentfile in the real .mk -- rust_uno-cpp ships
    # no .component of its own (a link-time bootstrap helper for the
    # Cargo-built Rust side via FFI, not itself a discoverable UNO service),
    # so no librestudio_add_component() call here, matching e.g.
    # connectivity's own "file" library (also link-only, no component).
    # Repository.mk does register it in OOOLIBS (line 464, `$(if $(ENABLE_
    # RUST_UNO),rust_uno-cpp)`), so it still gets a real install() + OOO
    # output name.
    add_library(rust_uno-cpp SHARED
        "${CMAKE_SOURCE_DIR}/rust_uno/uno_bootstrap.cxx"
        "${RUST_UNO_BINDINGS_CXX}")
    add_dependencies(rust_uno-cpp rust_uno_rustmaker)

    target_include_directories(rust_uno-cpp PRIVATE
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(rust_uno-cpp udkapi_normal_headers offapi_normal_headers)

    target_link_libraries(rust_uno-cpp PRIVATE cppu cppuhelper sal)

    librestudio_set_library_output_name(rust_uno-cpp rust_uno-cpp OOO)

    install(TARGETS rust_uno-cpp LIBRARY DESTINATION program)

    # --- CustomTarget_cargo.mk (pipeline step 2) ----------------------------
    #
    # This port's first real external-build-system invocation via a plain
    # add_custom_command (no CMake-native Cargo integration exists/is used
    # here). Output path matches Cargo.toml's own `[package] name =
    # "rust_uno"` (-> librust_uno.so on Linux, re-verified directly against
    # Cargo.toml, not assumed) -- a real SRCDIR-based path (cargo's own
    # target/ directory lives inside rust_uno/ itself), matching Extension_
    # rust_uno-example.mk's own literal reference to the same file, not a
    # build-dir path.
    set(RUST_UNO_CARGO_SO "${CMAKE_CURRENT_SOURCE_DIR}/target/release/librust_uno.so")
    add_custom_command(
        OUTPUT "${RUST_UNO_CARGO_SO}"
        COMMAND "${CARGO_EXECUTABLE}" build --release
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        DEPENDS rust_uno-cpp rust_uno_rustmaker
        COMMENT "cargo: building rust_uno (cargo build --release)"
        VERBATIM)
    add_custom_target(rust_uno_cargo DEPENDS "${RUST_UNO_CARGO_SO}")

    # --- Library_rust_uno-example.mk (pipeline step 4) ----------------------
    #
    # Links cargo's own librust_uno.so directly. Real absolute rpath instead
    # of upstream's own $ORIGIN (same substitution reasoning as external/
    # librevenge/librevenge.cmake's own header comment: this port's own
    # target is never co-installed into the .oxt Extension bundle upstream
    # assumes $ORIGIN would find it alongside). gb_Library_set_external_code
    # is a warnings-relaxation convenience, reproduced as a plain -w.
    add_library(rust_uno-example SHARED
        "${CMAKE_SOURCE_DIR}/rust_uno/example/example.cxx")
    add_dependencies(rust_uno-example rust_uno_cargo)
    target_compile_options(rust_uno-example PRIVATE -w)

    target_include_directories(rust_uno-example PRIVATE
        "${CMAKE_SOURCE_DIR}/rust_uno/example"
        "${udkapi_normal_HEADERS_INCLUDE_DIR}"
        "${offapi_normal_HEADERS_INCLUDE_DIR}")
    add_dependencies(rust_uno-example udkapi_normal_headers offapi_normal_headers)

    target_link_libraries(rust_uno-example PRIVATE
        cppu cppuhelper sal
        Boost::boost
        "${RUST_UNO_CARGO_SO}")
    set_target_properties(rust_uno-example PROPERTIES
        BUILD_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/target/release"
        INSTALL_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/target/release")

    librestudio_set_library_output_name(rust_uno-example rust_uno-example OOO)

    # Extension_rust_uno-example.mk's own .oxt bundling (platform.components
    # carrying example.component's own two implementations, registered into
    # an EXTENSION-LOCAL rdb via gb_Rdb_get_target -- NOT the port's global
    # services.rdb -- plus Addons.xcu/ProtocolHandler.xcu and the cargo .so
    # itself, all packaged as an installable .oxt) is NOT reproduced here --
    # this port has no Extension_*.mk CMake equivalent at all yet (a
    # genuinely new packaging mechanism, not exercised by any other module
    # so far, confirmed: Repository.mk registers rust_uno-example under
    # EXTENSIONLIBS, a different list from the OOOLIBS rust_uno-cpp is in).
    # Calling librestudio_add_component()/install() here the normal OOO way
    # would misrepresent this as a directly-installed service, which
    # upstream never does for it -- left honestly unwired, buildable but not
    # installed or component-registered, rather than invented.
endif ()
