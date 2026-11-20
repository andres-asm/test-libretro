cmake_minimum_required(VERSION 3.25)

project(test_libretro LANGUAGES C CXX)

# Works standalone and as a subdirectory of Multitap (hosts/host-libretro/test-libretro).
# Inside Multitap the options default to Multitap's own settings.
if(DEFINED MULTITAP_WARNINGS_AS_ERRORS)
    set(_warnings_default ${MULTITAP_WARNINGS_AS_ERRORS})
else()
    set(_warnings_default OFF)
endif()
if(DEFINED MULTITAP_SANITIZE)
    set(_sanitize_default ${MULTITAP_SANITIZE})
else()
    set(_sanitize_default OFF)
endif()

option(TEST_LIBRETRO_WARNINGS_AS_ERRORS "Treat warnings in the core's own code as errors" ${_warnings_default})
option(TEST_LIBRETRO_SANITIZE           "Build the core with ASan + UBSan"                ${_sanitize_default})

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

foreach(_dep imgui/imgui.h imgui_sw/src/imgui_sw.cpp libretro-common/include/libretro.h)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/${_dep}")
        message(FATAL_ERROR "deps/${_dep} is missing. Run: git submodule update --init --recursive")
    endif()
endforeach()

# Third-party: Dear ImGui and its software renderer (imgui_sw), compiled into
# the core with hidden symbols, so they never meet a frontend's own ImGui.
set(_third_party_sources
    deps/imgui/imgui.cpp
    deps/imgui/imgui_draw.cpp
    deps/imgui/imgui_tables.cpp
    deps/imgui/imgui_widgets.cpp
    deps/imgui_sw/src/imgui_sw.cpp
)

# The core: a MODULE library is what frontends dlopen / LoadLibrary.
add_library(test_libretro MODULE
    src/core.c
    src/pages.cpp
    src/ui.cpp
    src/pad_drawing.cpp
    src/pixel_font.cpp
    src/pixel_font_imgui.cpp
    src/video.c
    src/input.c
    src/controllers.c
    src/keyboard.c
    src/audio.c
    src/options.c
    src/sram.c
    src/text_input.c
    ${_third_party_sources}
)

set_target_properties(test_libretro PROPERTIES
    PREFIX ""                       # test_libretro.so, not libtest_libretro.so
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    C_VISIBILITY_PRESET hidden      # only the retro_* functions (RETRO_API) are exported
    CXX_STANDARD 17                 # imgui_sw uses std::clamp
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden    # ImGui too: never exported, never interposed
    VISIBILITY_INLINES_HIDDEN ON
)
if(NOT MSVC)
    # C++ written like C (Multitap's STYLE.md): no exceptions, no RTTI.
    target_compile_options(test_libretro PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti>)
endif()

target_include_directories(test_libretro SYSTEM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/libretro-common/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui_sw/src"
)
target_include_directories(test_libretro PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Multitap's libretro extensions (libretro_extensions.h, kept in host-libretro).
# Found where piccolo and the test frontend keep host-libretro, or given with
# TEST_LIBRETRO_EXTENSIONS_DIR; without it the core is built without them.
set(TEST_LIBRETRO_EXTENSIONS_DIR "" CACHE PATH "Folder with libretro_extensions.h (host-libretro's src/)")
set(_extensions_dir "${TEST_LIBRETRO_EXTENSIONS_DIR}")
if(NOT _extensions_dir)
    foreach(_candidate "${CMAKE_SOURCE_DIR}/deps/libretro/host-libretro/src"
                       "${CMAKE_SOURCE_DIR}/deps/host-libretro/src")
        if(EXISTS "${_candidate}/libretro_extensions.h")
            set(_extensions_dir "${_candidate}")
            break()
        endif()
    endforeach()
endif()
if(_extensions_dir AND EXISTS "${_extensions_dir}/libretro_extensions.h")
    target_include_directories(test_libretro PRIVATE "${_extensions_dir}")
    target_compile_definitions(test_libretro PRIVATE TEST_LIBRETRO_EXTENSIONS=1)
    message(STATUS "Test libretro: Multitap's extensions from ${_extensions_dir}")
else()
    message(STATUS "Test libretro: built without Multitap's extensions (libretro_extensions.h not found)")
endif()

# The core's version, reported as library_version: the base on main (or
# master), base-<branch> on any other branch, base-<commit> when detached, the
# base alone without git. CMake configures again when the branch changes.
set(TEST_LIBRETRO_VERSION_BASE 0.5)
set(TEST_LIBRETRO_VERSION "${TEST_LIBRETRO_VERSION_BASE}")
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${CMAKE_CURRENT_SOURCE_DIR}" rev-parse --abbrev-ref HEAD
                    OUTPUT_VARIABLE _branch OUTPUT_STRIP_TRAILING_WHITESPACE
                    RESULT_VARIABLE _result ERROR_QUIET)
    if(_result EQUAL 0 AND _branch)
        if(_branch STREQUAL "HEAD")
            execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${CMAKE_CURRENT_SOURCE_DIR}" rev-parse --short HEAD
                            OUTPUT_VARIABLE _branch OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
        endif()
        if(NOT _branch STREQUAL "main" AND NOT _branch STREQUAL "master")
            set(TEST_LIBRETRO_VERSION "${TEST_LIBRETRO_VERSION_BASE}-${_branch}")
        endif()
        execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${CMAKE_CURRENT_SOURCE_DIR}" rev-parse --path-format=absolute --git-path HEAD
                        OUTPUT_VARIABLE _head_file OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
        if(_head_file AND EXISTS "${_head_file}")
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_head_file}")
        endif()
    endif()
endif()
target_compile_definitions(test_libretro PRIVATE TEST_LIBRETRO_VERSION="${TEST_LIBRETRO_VERSION}")
message(STATUS "Test libretro ${TEST_LIBRETRO_VERSION}")

find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
    target_link_libraries(test_libretro PRIVATE ${MATH_LIBRARY})
endif()

# Our code gets strict warnings; ImGui and imgui_sw are third-party and get none.
set(_own_c_sources src/core.c src/video.c src/input.c src/controllers.c src/keyboard.c src/audio.c src/options.c src/sram.c
                    src/text_input.c)
set(_own_cxx_sources src/pages.cpp src/ui.cpp src/pad_drawing.cpp src/pixel_font.cpp src/pixel_font_imgui.cpp)
if(MSVC)
    set_source_files_properties(${_own_c_sources} ${_own_cxx_sources} PROPERTIES COMPILE_OPTIONS "/W4")
    set_source_files_properties(${_third_party_sources} PROPERTIES COMPILE_OPTIONS "/W0")
    if(TEST_LIBRETRO_WARNINGS_AS_ERRORS)
        set_property(SOURCE ${_own_c_sources} ${_own_cxx_sources} APPEND PROPERTY COMPILE_OPTIONS "/WX")
    endif()
else()
    set(_common_warnings -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wcast-qual
                         -Wno-missing-field-initializers)
    set_source_files_properties(${_own_c_sources} PROPERTIES COMPILE_OPTIONS
        "${_common_warnings};-Wstrict-prototypes;-Wmissing-prototypes")
    set_source_files_properties(${_own_cxx_sources} PROPERTIES COMPILE_OPTIONS
        "${_common_warnings};-Wmissing-declarations")
    set_source_files_properties(${_third_party_sources} PROPERTIES COMPILE_OPTIONS "-w")
    if(TEST_LIBRETRO_WARNINGS_AS_ERRORS)
        set_property(SOURCE ${_own_c_sources} ${_own_cxx_sources} APPEND PROPERTY COMPILE_OPTIONS "-Werror")
    endif()
    if(TEST_LIBRETRO_SANITIZE)
        target_compile_options(test_libretro PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(test_libretro PRIVATE -fsanitize=address,undefined)
    endif()
endif()
