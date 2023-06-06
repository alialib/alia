cmake_minimum_required(VERSION 3.16)

# ALIA_IS_MAIN_PROJECT defines whether we are building alia as the main
# project or as a dependency of another project.
if(NOT DEFINED ALIA_IS_MAIN_PROJECT)
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(ALIA_IS_MAIN_PROJECT ON)
    else()
        set(ALIA_IS_MAIN_PROJECT OFF)
    endif()
endif()

include(core/cmake/utilities.cmake)

alia_detect_compiler()

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

cmake_dependent_option(
    ALIA_ENABLE_HTML
    "Enable integration with HTML5"
    ${ALIA_IS_MAIN_PROJECT}
    "IS_EMSCRIPTEN"
    OFF)
if(ALIA_ENABLE_HTML)
  list(APPEND VCPKG_MANIFEST_FEATURES "html")
endif()

cmake_dependent_option(
    ALIA_ENABLE_UI
    "Enable the UI library"
    ON
    "NOT IS_EMSCRIPTEN"
    OFF)
if(ALIA_ENABLE_UI)
  list(APPEND VCPKG_MANIFEST_FEATURES "ui")
endif()

cmake_dependent_option(
    ALIA_BUILD_DOCS
    "Build the docs"
    ${ALIA_IS_MAIN_PROJECT}
    "IS_EMSCRIPTEN"
    OFF)

# We can only declare the project after we've set the vcpkg manifest features.
project(alia)

# If alia is the main project, set some global development flags.
if (ALIA_IS_MAIN_PROJECT)
    alia_set_development_warning_flags()

    # Use absolute paths for error messages.
    if (IS_MSVC)
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
add_subdirectory(core)

# Add alia/HTML.
if (ALIA_ENABLE_HTML)
    add_subdirectory("modules/html")
endif()

# Add the docs.
if (ALIA_BUILD_DOCS)
    add_subdirectory(docs)
endif()
