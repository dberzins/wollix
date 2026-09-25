cmake_minimum_required(VERSION 3.16)

# The version comes from the header, the single source of truth.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/wollix.h" _wollix_version_line
     REGEX "^#define WOLLIX_VERSION \"[0-9]+\\.[0-9]+\\.[0-9]+\"")
string(REGEX REPLACE "^#define WOLLIX_VERSION \"([0-9]+\\.[0-9]+\\.[0-9]+)\".*$" "\\1"
       WOLLIX_VERSION "${_wollix_version_line}")

project(wollix VERSION ${WOLLIX_VERSION} LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Tests and demos are on only when Wollix is the top-level project; an
# add_subdirectory or FetchContent consumer gets the target alone.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(_wollix_top ON)
else()
  set(_wollix_top OFF)
endif()
option(WOLLIX_BUILD_TESTS "Build the test binaries and register them with CTest" ${_wollix_top})
option(WOLLIX_BUILD_DEMOS "Build the Raylib and SDL3 demos when those libraries are found" ${_wollix_top})
# Last-resort backend for the Raylib demos: build Raylib from source under
# build/_deps instead of requiring an install. Off by default - it turns a
# configure into a source download and a dependency build - and consulted only
# after find_package and the plain-install probe have both come up empty.
option(WOLLIX_FETCH_RAYLIB "Fetch and build Raylib from source when no installed Raylib is found" OFF)

# ----------------------------------------------------------------------------
# The consumer target: header-only, C11, plus the compile options the option
# macros need on every translation unit that includes wollix.h.
#   - gcc:   -Wno-override-init (an override under -Wextra) and
#            -Wno-override-init-side-effects (overriding a default that is
#            itself a compound literal, on by default) are C-only flags that
#            g++ rejects, so both are gated to C compiles.
#   - clang: -Winitializer-overrides is on by default in C and C++.
#   - MSVC:  /Zc:preprocessor, the conforming preprocessor, is required for
#            wlx_layout_begin_s (it forwards __VA_ARGS__ into a second macro).
# ----------------------------------------------------------------------------
add_library(wollix INTERFACE)
add_library(wollix::wollix ALIAS wollix)
target_include_directories(wollix INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(wollix INTERFACE c_std_11)
target_compile_options(wollix INTERFACE
  $<$<COMPILE_LANG_AND_ID:C,GNU>:-Wno-override-init;-Wno-override-init-side-effects>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wno-initializer-overrides>
  $<$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang>:-Wno-initializer-overrides>
  $<$<COMPILE_LANG_AND_ID:C,MSVC>:/Zc:preprocessor>
  $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/Zc:preprocessor>)
# CMake maps the c_std_11 feature to /std:c11 for MSVC from 3.20; older
# CMake records the feature without a flag, so the switch is added by hand
# there (the installed config does the same for find_package consumers).
if(MSVC AND CMAKE_VERSION VERSION_LESS 3.20)
  target_compile_options(wollix INTERFACE $<$<COMPILE_LANG_AND_ID:C,MSVC>:/std:c11>)
endif()

# ----------------------------------------------------------------------------
# Install and export: find_package(wollix CONFIG) yields wollix::wollix.
# SameMinorVersion while the major version is 0 (0.x minors may still break);
# switch to SameMajorVersion at 1.0.
# ----------------------------------------------------------------------------
set(_wollix_cmake_dir ${CMAKE_INSTALL_LIBDIR}/cmake/wollix)
install(FILES wollix.h wollix_editor.h wollix_raylib.h wollix_sdl3.h wollix_wasm.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS wollix EXPORT wollixTargets)
install(EXPORT wollixTargets NAMESPACE wollix:: DESTINATION ${_wollix_cmake_dir})
configure_package_config_file(cmake/wollixConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/wollixConfig.cmake
  INSTALL_DESTINATION ${_wollix_cmake_dir})
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/wollixConfigVersion.cmake
  COMPATIBILITY SameMinorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/wollixConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/wollixConfigVersion.cmake
        DESTINATION ${_wollix_cmake_dir})
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/wollix)

# The Makefile's warning set, per compiler, for the tests and demos.
# /W4 is stricter than -Wall -Wextra, so the MSVC leg turns off the classes
# that have no GNU/Clang counterpart and no bug behind them. They are held to
# the same substance, not the same flag spelling:
#   C4127  constant conditional - while(0) in the assertion macros.
#   C4189  local initialized but not referenced - the $S temporaries MSVC
#          materializes for the compound literals the option-struct macros
#          build. Raised at the call site, so it lands on test and demo code
#          (and on any /W4 consumer) rather than inside the header.
#   C4310  cast truncates constant value - the deliberate (char)0x80 byte
#          literals in the UTF-8, clipboard and text-layout tests.
#   C4996  "unsafe" CRT nags on strcat and friends.
set(_wollix_warn_msvc /W4 /wd4127 /wd4189 /wd4310 /D_CRT_SECURE_NO_WARNINGS)
set(_wollix_warn_c
  $<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-Wall;-Wextra;-Werror=switch>
  $<$<COMPILE_LANG_AND_ID:C,MSVC>:${_wollix_warn_msvc}>)
set(_wollix_warn_cxx
  $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wall;-Wextra;-Werror;-Wno-missing-field-initializers>
  $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:${_wollix_warn_msvc}>)

# ----------------------------------------------------------------------------
# Tests. One list, mirroring the Makefile's `test` target: a binary added
# there is added here in the same commit, and vice versa. Each test runs from
# the source tree like `make test` does.
# ----------------------------------------------------------------------------
if(WOLLIX_BUILD_TESTS)
  enable_language(CXX)
  enable_testing()

  function(wollix_add_test name)
    cmake_parse_arguments(T "" "" "SOURCES;DEFINES;OPTIONS" ${ARGN})
    add_executable(${name} ${T_SOURCES})
    target_link_libraries(${name} PRIVATE wollix::wollix)
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    target_compile_definitions(${name} PRIVATE ${T_DEFINES})
    target_compile_options(${name} PRIVATE ${_wollix_warn_c} ${T_OPTIONS})
    if(NOT MSVC)
      target_link_libraries(${name} PRIVATE m)
    endif()
    add_test(NAME ${name} COMMAND ${name} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endfunction()

  wollix_add_test(test_runner SOURCES tests/test_main.c)
  wollix_add_test(test_single_pass SOURCES tests/test_single_pass.c
                  DEFINES WLX_SLOT_SINGLE_PASS_CLAMP)
  # Death checks fork a child per guard; POSIX only.
  if(NOT WIN32)
    wollix_add_test(test_hard_assert SOURCES tests/test_hard_assert.c DEFINES NDEBUG)
  endif()
  wollix_add_test(test_config_override SOURCES tests/test_config_override.c)
  wollix_add_test(test_undo_disabled SOURCES tests/test_undo_disabled.c)
  # The declarations TU is built with the override warning promoted to an
  # error (every default macro sets each designator once); the implementation
  # TU keeps the normal flags because the library's own macro calls override
  # defaults on purpose. MSVC has no such warning.
  wollix_add_test(test_defaults_once SOURCES tests/test_defaults_once.c
                                             tests/test_defaults_once_impl.c)
  set_source_files_properties(tests/test_defaults_once.c PROPERTIES COMPILE_OPTIONS
    "$<$<C_COMPILER_ID:GNU>:-Werror=override-init>;$<$<C_COMPILER_ID:Clang,AppleClang>:-Werror=initializer-overrides>")
  # The documented C++ path: a C++11 caller over the implementation compiled
  # as C11 in its own translation unit.
  wollix_add_test(test_cpp_path SOURCES tests/test_cpp_path.cpp
                                        tests/test_defaults_once_impl.c
                  OPTIONS ${_wollix_warn_cxx})
  target_compile_features(test_cpp_path PRIVATE cxx_std_11)
endif()

# ----------------------------------------------------------------------------
# Demos, best effort: built when the backend library is found, skipped with a
# message otherwise. Raylib comes as a CMake package or as a plain install
# (raylib.h + libraylib, e.g. ~/opt/raylib; point WOLLIX_RAYLIB_DIR at it);
# SDL3 and SDL3_ttf as CMake packages (CMAKE_PREFIX_PATH). Demo binaries load
# their assets relative to the source tree, so run them from there.
# ----------------------------------------------------------------------------
if(WOLLIX_BUILD_DEMOS)
  function(wollix_add_demo name source dep)
    cmake_parse_arguments(D "" "" "DEFINES" ${ARGN})
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE wollix::wollix ${dep})
    target_compile_definitions(${name} PRIVATE ${D_DEFINES})
    # C4244 on top of the shared set, for the demos alone. They widen ints into
    # the float fields of the option structs - a raylib getter's int screen
    # size, a demo's int row height - and MSVC charges each one to the line the
    # defaults macro expansion ends on rather than to the designator, so the
    # warning cannot be silenced where it is actually written. It is the safe
    # direction and has no counterpart under -Wall -Wextra. The tests keep
    # C4244 (see the cast in test_fuzz.c) and so does the library: the eight
    # real conversions at the raylib boundary in wollix_raylib.h are cast.
    target_compile_options(${name} PRIVATE ${_wollix_warn_c}
      $<$<COMPILE_LANG_AND_ID:C,MSVC>:/wd4244>)
    if(NOT MSVC)
      target_link_libraries(${name} PRIVATE m)
    endif()
  endfunction()

  find_package(raylib CONFIG QUIET)
  add_library(wollix_raylib_dep INTERFACE)
  if(raylib_FOUND)
    target_link_libraries(wollix_raylib_dep INTERFACE raylib)
  else()
    find_path(WOLLIX_RAYLIB_INCLUDE_DIR raylib.h
              HINTS ${WOLLIX_RAYLIB_DIR} ENV WOLLIX_RAYLIB_DIR $ENV{HOME}/opt/raylib
              PATH_SUFFIXES include)
    find_library(WOLLIX_RAYLIB_LIBRARY raylib
                 HINTS ${WOLLIX_RAYLIB_DIR} ENV WOLLIX_RAYLIB_DIR $ENV{HOME}/opt/raylib
                 PATH_SUFFIXES lib)
    if(WOLLIX_RAYLIB_INCLUDE_DIR AND WOLLIX_RAYLIB_LIBRARY)
      target_include_directories(wollix_raylib_dep INTERFACE ${WOLLIX_RAYLIB_INCLUDE_DIR})
      target_link_libraries(wollix_raylib_dep INTERFACE ${WOLLIX_RAYLIB_LIBRARY})
      if(APPLE)
        target_link_libraries(wollix_raylib_dep INTERFACE
          "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
      elseif(WIN32)
        target_link_libraries(wollix_raylib_dep INTERFACE opengl32 gdi32 winmm user32 shell32)
      elseif(UNIX)
        target_link_libraries(wollix_raylib_dep INTERFACE GL dl pthread rt X11)
      endif()
      set(raylib_FOUND TRUE)
    endif()
  endif()

  # Neither an installed package nor a plain install: build Raylib from source
  # when asked. FetchContent yields the same imported raylib target, so its
  # own interface libraries (the Windows system libs among them) come along and
  # the per-platform lists above are not repeated here.
  if(NOT raylib_FOUND AND WOLLIX_FETCH_RAYLIB)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib
      URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
      URL_HASH SHA256=aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e)
    FetchContent_MakeAvailable(raylib)
    target_link_libraries(wollix_raylib_dep INTERFACE raylib)
    set(raylib_FOUND TRUE)
  endif()

  if(raylib_FOUND)
    set(_wollix_raylib_demos layout button button_image label_image text checkbox
        checkbox_tex image input scroll_panel slider demo widget_size variable_slots
        nest2_panel nested_panel grid grid_auto flex_demo minmax_demo theme_demo
        font_demo opacity_demo border_demo disabled_demo gallery auth editor popup)
    foreach(_demo IN LISTS _wollix_raylib_demos)
      wollix_add_demo(${_demo} demos/${_demo}.c wollix_raylib_dep DEFINES WLX_DEBUG)
    endforeach()
    wollix_add_demo(dashboard demos/dashboard/dashboard.c wollix_raylib_dep
                    DEFINES WLX_DEBUG WLX_PERF)
  else()
    message(STATUS "wollix: Raylib not found; Raylib demos skipped "
                   "(set WOLLIX_RAYLIB_DIR or CMAKE_PREFIX_PATH, or -DWOLLIX_FETCH_RAYLIB=ON to build it from source)")
  endif()

  find_package(SDL3 CONFIG QUIET)
  find_package(SDL3_ttf CONFIG QUIET)
  if(SDL3_FOUND AND SDL3_ttf_FOUND)
    add_library(wollix_sdl3_dep INTERFACE)
    target_link_libraries(wollix_sdl3_dep INTERFACE SDL3::SDL3 SDL3_ttf::SDL3_ttf)
    wollix_add_demo(sdl3_demo demos/sdl3_demo.c wollix_sdl3_dep)
    wollix_add_demo(gallery_sdl3 demos/gallery.c wollix_sdl3_dep DEFINES WLX_GALLERY_SDL3)
    wollix_add_demo(dashboard_sdl3 demos/dashboard/dashboard.c wollix_sdl3_dep
                    DEFINES WLX_PERF WLX_DASHBOARD_SDL3)
  else()
    message(STATUS "wollix: SDL3 or SDL3_ttf not found; SDL3 demos skipped (set CMAKE_PREFIX_PATH)")
  endif()
endif()
