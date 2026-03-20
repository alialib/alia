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

# Define our options...
# As we define the options, we also add the corresponding vcpkg options.

option(
    ALIA_ENABLE_TESTING
    "Generate targets for testing alia"
    ${ALIA_IS_MAIN_PROJECT})
if(ALIA_ENABLE_TESTING)
  list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

option(
    ALIA_ENABLE_EXAMPLES
    "Build examples"
    ${ALIA_IS_MAIN_PROJECT})

option(
    ALIA_ENABLE_BENCHMARKS
    "Build benchmarks"
    ${ALIA_IS_MAIN_PROJECT})

option(
    ALIA_ENABLE_OPENGL
    "Enable the OpenGL renderer"
    ON)
if(ALIA_ENABLE_OPENGL)
  list(APPEND VCPKG_MANIFEST_FEATURES "opengl")
endif()

include(CMakeDependentOption)

option(
    ALIA_ENABLE_GLFW
    "Enable the GLFW backend for the UI library"
    ON)
if(ALIA_ENABLE_GLFW)
  list(APPEND VCPKG_MANIFEST_FEATURES "glfw")
endif()

option(
    ALIA_ENABLE_ASSET_PIPELINE
    "Enable the asset pipeline"
    ${ALIA_IS_NOT_EMSCRIPTEN})
if(ALIA_ENABLE_ASSET_PIPELINE)
  list(APPEND VCPKG_MANIFEST_FEATURES "asset-pipeline")
endif()

# We can only declare the project after we've set the vcpkg manifest features.
project(alia)

alia_detect_compiler()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(core)

if(ALIA_ENABLE_OPENGL)
    add_subdirectory(drivers/renderers/gl)
endif()

if(ALIA_ENABLE_GLFW)
    add_subdirectory(drivers/platforms/glfw)
endif()

add_subdirectory(drivers/text_engines/msdf)

add_subdirectory(apis/cpp)

set(ALIA_GEN_DIR "${CMAKE_BINARY_DIR}/gen"
    CACHE PATH "directory containing generated alia font sources")

set(ALIA_FONTS_H "${ALIA_GEN_DIR}/alia_fonts.h")
set(ALIA_FONTS_CPP "${ALIA_GEN_DIR}/alia_fonts.cpp")

if(ALIA_ENABLE_ASSET_PIPELINE)
    set(ALIA_FONT_MANIFEST "${CMAKE_SOURCE_DIR}/assets/fonts.yaml")

    add_subdirectory(tools)
    file(MAKE_DIRECTORY "${ALIA_GEN_DIR}")
    add_custom_command(
        OUTPUT "${ALIA_FONTS_H}" "${ALIA_FONTS_CPP}"
        COMMAND $<TARGET_FILE:alia_asset_builder>
            "${ALIA_FONT_MANIFEST}" "${ALIA_FONTS_H}" "${ALIA_FONTS_CPP}"
            --cache-dir "${CMAKE_BINARY_DIR}/font_cache"
        DEPENDS alia_asset_builder "${ALIA_FONT_MANIFEST}"
        COMMENT "Generating Alia font assets"
    )
    set_source_files_properties("${ALIA_FONTS_CPP}" PROPERTIES GENERATED TRUE)
endif()

add_library(alia_font_assets STATIC "${ALIA_FONTS_CPP}")
target_include_directories(alia_font_assets PUBLIC
    "${ALIA_GEN_DIR}"
    ${CMAKE_SOURCE_DIR}/drivers/text_engines/msdf/include
    ${CMAKE_SOURCE_DIR}/core/include)
target_link_libraries(alia_font_assets PUBLIC alia_core)

if(ALIA_ENABLE_EXAMPLES)
    add_executable(alia_app ${PROJECT_SOURCE_DIR}/examples/app.cpp)
    target_link_libraries(alia_app PRIVATE
        alia_cpp alia_core alia_gl_renderer alia_glfw_platform
        alia_msdf_text_engine alia_font_assets)
    target_include_directories(alia_app PRIVATE "${ALIA_GEN_DIR}")

    if(EMSCRIPTEN)
        target_compile_options(alia_app PRIVATE
            "-fwasm-exceptions")
        # -s USE_GLFW=3 enables internal GLFW
        # --preload-file maps source assets to virtual filesystem

        target_link_options(alia_app PRIVATE
            "-sUSE_GLFW=3"
            "-sUSE_WEBGL2=1"
            "-sFULL_ES3=1"
            "-sWASM=1"
            "-fwasm-exceptions"
            "-sALLOW_MEMORY_GROWTH=1"
            "--preload-file" "${CMAKE_SOURCE_DIR}/assets@/assets"
            "--shell-file" "${CMAKE_SOURCE_DIR}/shell_minimal.html"
        )
        set_target_properties(alia_app PROPERTIES SUFFIX ".html")
    endif()
endif()

if(ALIA_ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests/core)
    add_test(NAME test_core_cpp COMMAND test_core_cpp)
    add_test(NAME test_core_c COMMAND test_core_c)
endif()

if(ALIA_ENABLE_BENCHMARKS)
    add_executable(alia_benchmarks ${PROJECT_SOURCE_DIR}/benchmarks/layout.cpp)
    target_link_libraries(alia_benchmarks PRIVATE
        alia_core)
    if (MSVC)
        target_link_options(alia_benchmarks PRIVATE
            $<$<CONFIG:RelWithDebInfo>:/PROFILE>)
    endif()
endif()
