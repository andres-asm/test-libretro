cmake_minimum_required(VERSION 3.25)

project(test_wizard LANGUAGES C)

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

option(TEST_WIZARD_WARNINGS_AS_ERRORS "Treat warnings in the core's own code as errors" ${_warnings_default})
option(TEST_WIZARD_SANITIZE           "Build the core with ASan + UBSan"                ${_sanitize_default})

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

foreach(_dep nuklear/nuklear.h libretro-common/include/libretro.h)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/${_dep}")
        message(FATAL_ERROR "deps/${_dep} is missing. Run: git submodule update --init --recursive")
    endif()
endforeach()

# The core: a MODULE library is what frontends dlopen / LoadLibrary.
add_library(test_wizard_libretro MODULE
    src/core.c
    src/wizard.c
    src/video.c
    src/input.c
    src/audio.c
    src/options.c
    src/nuklear_impl.c
)

set_target_properties(test_wizard_libretro PROPERTIES
    PREFIX ""                       # test_wizard_libretro.so, not libtest_wizard_libretro.so
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    C_VISIBILITY_PRESET hidden      # only the retro_* functions (RETRO_API) are exported
)

target_include_directories(test_wizard_libretro SYSTEM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/libretro-common/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/nuklear"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/nuklear/demo/rawfb"
)
target_include_directories(test_wizard_libretro PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
    target_link_libraries(test_wizard_libretro PRIVATE ${MATH_LIBRARY})
endif()

# Our code gets strict warnings; nuklear_impl.c is Nuklear itself and gets none.
set(_own_sources src/core.c src/wizard.c src/video.c src/input.c src/audio.c)
if(MSVC)
    set_source_files_properties(${_own_sources} PROPERTIES COMPILE_OPTIONS "/W4")
    set_source_files_properties(src/nuklear_impl.c PROPERTIES COMPILE_OPTIONS "/W0")
    if(TEST_WIZARD_WARNINGS_AS_ERRORS)
        set_property(SOURCE ${_own_sources} APPEND PROPERTY COMPILE_OPTIONS "/WX")
    endif()
else()
    set_source_files_properties(${_own_sources} PROPERTIES COMPILE_OPTIONS
        "-Wall;-Wextra;-Wpedantic;-Wconversion;-Wsign-conversion;-Wshadow;-Wcast-qual;-Wstrict-prototypes;-Wmissing-prototypes;-Wno-missing-field-initializers")
    set_source_files_properties(src/nuklear_impl.c PROPERTIES COMPILE_OPTIONS "-w")
    if(TEST_WIZARD_WARNINGS_AS_ERRORS)
        set_property(SOURCE ${_own_sources} APPEND PROPERTY COMPILE_OPTIONS "-Werror")
    endif()
    if(TEST_WIZARD_SANITIZE)
        target_compile_options(test_wizard_libretro PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(test_wizard_libretro PRIVATE -fsanitize=address,undefined)
    endif()
endif()
