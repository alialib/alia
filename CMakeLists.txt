cmake_minimum_required(VERSION 3.16)

include(cmake/utilities.cmake)

# ALIA_IS_MAIN_PROJECT defines whether we are building alia as the main
# project or as a dependency of another project.
if(NOT DEFINED ALIA_IS_MAIN_PROJECT)
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(ALIA_IS_MAIN_PROJECT ON)
    else()
        set(ALIA_IS_MAIN_PROJECT OFF)
    endif()
endif()

alia_detect_emscripten()

# Set VCPKG target triplet to support the Skia build.
if (ALIA_IS_MAIN_PROJECT AND WIN32)
    # This assumes we're doing a 64-bit build. It seems impossible to verify
    # that at this point in the CMake execution - it's possible after the call
    # to `project()`, but we need to set these variables before that - but we
    # could add an option to support 32-bit builds.
    set(VCPKG_TARGET_TRIPLET x64-windows-mixed)
    set(VCPKG_HOST_TRIPLET x64-windows-mixed)
endif()

# Define our options...
# As we define the options, we also add the corresponding vcpkg options.

option(
    ALIA_ENABLE_TESTING
    "Generate targets for testing alia"
    ${ALIA_IS_MAIN_PROJECT})
if(ALIA_ENABLE_TESTING)
  list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

include(CMakeDependentOption)

option(
    ALIA_ENABLE_UI
    "Enable the UI library"
    "NOT IS_EMSCRIPTEN")
if(ALIA_ENABLE_UI)
  list(APPEND VCPKG_MANIFEST_FEATURES "ui")
endif()

cmake_dependent_option(
    ALIA_ENABLE_GLFW
    "Enable the GLFW backend for the UI library"
    ON
    "ALIA_ENABLE_UI"
    OFF)
if(ALIA_ENABLE_GLFW)
  list(APPEND VCPKG_MANIFEST_FEATURES "glfw")
endif()

cmake_dependent_option(
    ALIA_ENABLE_SDL
    "Enable the SDL backend for the UI library"
    ON
    "ALIA_ENABLE_UI"
    OFF)
if(ALIA_ENABLE_SDL)
  list(APPEND VCPKG_MANIFEST_FEATURES "sdl")
endif()

cmake_dependent_option(
    ALIA_BUILD_DOCS
    "Build the docs"
    ${ALIA_IS_MAIN_PROJECT}
    "IS_EMSCRIPTEN"
    OFF)

option(
    ALIA_BUILD_EXAMPLES
    "Build examples"
    ${ALIA_IS_MAIN_PROJECT})

cmake_dependent_option(
    ALIA_ENABLE_HTML
    "Enable integration with HTML5"
    ${ALIA_IS_MAIN_PROJECT}
    "IS_EMSCRIPTEN"
    OFF)
if(ALIA_ENABLE_HTML)
  list(APPEND VCPKG_MANIFEST_FEATURES "html")
endif()

# We can only declare the project after we've set the vcpkg manifest features.
project(alia)

alia_detect_compiler()

# If alia is the main project, set some global development flags.
if (ALIA_IS_MAIN_PROJECT)
    alia_set_development_warning_flags()

    if (IS_MSVC)
        # Use EditAndContinue-style debug info for MSVC.
        # TODO: Use CMAKE_MSVC_DEBUG_INFORMATION_FORMAT once it's more widely
        # supported:
        # set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
        #    "$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>")
        string(REGEX REPLACE "/Zi" "/ZI"
            CMAKE_CXX_FLAGS_RELWITHDEBINFO
            "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
        string(REGEX REPLACE "/Zi" "/ZI"
            CMAKE_CXX_FLAGS_DEBUG
            "${CMAKE_CXX_FLAGS_DEBUG}")
        # Use absolute paths for error messages.
        add_compile_options(/FC)
    endif()
endif()

# If we're testing, set the build options for instrumenting test coverage.
if(ALIA_ENABLE_TESTING)
    enable_testing()
    if(IS_CLANG AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Enabling gcov support")
        add_compile_options(-DLLVM_USE_LINKER=gold -fprofile-instr-generate
                            -fcoverage-mapping)
        string(APPEND CMAKE_EXE_LINKER_FLAGS
            " -fprofile-instr-generate -fcoverage-mapping")
    endif()
endif()

# Add the core library.
include_directories(src)
if (ALIA_IS_MAIN_PROJECT)
    set(ALIA_GLOB_OPTIONS "CONFIGURE_DEPENDS")
else()
    set(ALIA_GLOB_OPTIONS "")
endif()
file(GLOB_RECURSE ALIA_CORE_SRC_FILES ${ALIA_GLOB_OPTIONS}
    "src/alia/core/*.cpp")
add_library(alia_core STATIC ${ALIA_CORE_SRC_FILES})
target_include_directories(alia_core PUBLIC src)
target_compile_features(alia_core PUBLIC cxx_std_17)

# Add the UI library.
if (ALIA_ENABLE_UI)
    find_package(unofficial-skia CONFIG REQUIRED)

    include(FetchContent)
    FetchContent_Declare(dmilos_color
    GIT_REPOSITORY https://github.com/dmilos/color.git
    GIT_TAG 84dc0512cb5fcf6536d79f0bee2530e678c01b03)
    FetchContent_Populate(dmilos_color)

    file(GLOB_RECURSE ALIA_UI_SOURCES ${ALIA_GLOB_OPTIONS}
        "src/alia/ui/*.cpp")
    add_library(alia_ui STATIC ${ALIA_UI_SOURCES})
    target_include_directories(alia_ui PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(alia_ui PUBLIC
        alia_core
        unofficial::skia::skia
        unofficial::skia::modules::skshaper)
        # unofficial::skia::modules::svg)
    if (IS_EMSCRIPTEN)
        target_link_options(alia_ui PUBLIC
            "-sFULL_ES3"
            "-sTOTAL_MEMORY=64MB")
        set(CMAKE_EXECUTABLE_SUFFIX ".html")
    endif()
    target_include_directories(alia_ui PUBLIC
        "${dmilos_color_SOURCE_DIR}/src")
    target_precompile_headers(alia_ui PUBLIC
        src/alia/ui.hpp)
endif()

# Add the GLFW3 backend.
if (ALIA_ENABLE_GLFW)
    find_package(glfw3 3.3 CONFIG REQUIRED)
    add_library(alia_glfw STATIC "src/alia/backends/glfw.cpp")
    target_include_directories(alia_glfw PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(alia_glfw PUBLIC alia_ui glfw)
endif()

# Add the SDL backend.
if (ALIA_ENABLE_SDL)
    find_package(SDL2 CONFIG REQUIRED)
    add_library(alia_sdl STATIC "src/alia/backends/sdl.cpp")
    target_include_directories(alia_sdl PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(alia_sdl PUBLIC alia_ui
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
endif()

# Add the tests.
if (ALIA_ENABLE_TESTING)
    add_subdirectory("tests")
endif()

# Add the examples.
if (ALIA_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Add alia/HTML.
if (ALIA_ENABLE_HTML)
    add_subdirectory("integrations/html")
endif()

# Add the docs.
if (ALIA_BUILD_DOCS)
    add_subdirectory(docs)
endif()
