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
if(ALIA_ENABLE_OPENGL AND NOT ALIA_IS_EMSCRIPTEN)
  list(APPEND VCPKG_MANIFEST_FEATURES "opengl")
endif()

# GLFW integration helpers (input/surface glue) for embedding — not a host.
option(
    ALIA_ENABLE_GLFW
    "Enable GLFW integration helpers and the GL embed example"
    ${ALIA_IS_MAIN_PROJECT})
if(ALIA_ENABLE_GLFW AND NOT ALIA_IS_EMSCRIPTEN)
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

add_subdirectory(drivers/platforms/api)

if(ALIA_ENABLE_GLFW AND NOT EMSCRIPTEN)
    add_subdirectory(drivers/platforms/glfw)
endif()

if(WIN32 AND NOT EMSCRIPTEN)
    add_subdirectory(drivers/platforms/win32)
    add_subdirectory(drivers/renderers/d3d11)
endif()

if(EMSCRIPTEN)
    add_subdirectory(drivers/platforms/web)
endif()

# Product shell: shared UI shell + app bootstrap (D3D11 on Win32, GL on web).
if((EMSCRIPTEN AND ALIA_ENABLE_OPENGL)
        OR (WIN32 AND NOT EMSCRIPTEN AND TARGET alia_win32_platform))
    alia_add_host_library()
    add_subdirectory(shell)
    set(ALIA_EMSCRIPTEN_SHELL_FILE
        "${CMAKE_SOURCE_DIR}/shell/shell_minimal.html")
    add_subdirectory(shell/app)
endif()

add_subdirectory(apis/cpp)

set(ALIA_GEN_DIR "${CMAKE_BINARY_DIR}/gen"
    CACHE PATH "directory containing generated alia font sources")

set(ALIA_FONTS_H "${ALIA_GEN_DIR}/alia_fonts.h")
set(ALIA_FONTS_CPP "${ALIA_GEN_DIR}/alia_fonts.cpp")
# The font manifest is split into typography (where the library is more
# tolerant of app-provided overrides) and UI icons (where overrides are more
# likely to break the appearance of library-provided UI components).
set(ALIA_FONT_MANIFEST_UI_ICONS
    "${CMAKE_SOURCE_DIR}/shell/assets/ui_icons.yaml")
set(ALIA_FONT_MANIFEST_TYPOGRAPHY
    "${CMAKE_SOURCE_DIR}/shell/assets/fonts.yaml")

macro(alia_generate_font_assets out_h out_cpp)
    set(_alia_font_manifests ${ARGN})
    if(ALIA_ENABLE_ASSET_PIPELINE)
        add_custom_command(
            OUTPUT "${out_h}" "${out_cpp}"
            COMMAND $<TARGET_FILE:alia_asset_builder>
                ${_alia_font_manifests}
                "${out_h}" "${out_cpp}"
                --cache-dir "${CMAKE_BINARY_DIR}/font_cache"
            DEPENDS alia_asset_builder ${_alia_font_manifests}
            COMMENT "Generating font assets (${out_h})"
        )
    elseif(EMSCRIPTEN AND ALIA_ENABLE_EXAMPLES)
        set(ALIA_HOST_BUILD_DIR "${CMAKE_SOURCE_DIR}/build/Release"
            CACHE PATH
            "Native build directory containing tools/alia_asset_builder")
        set(ALIA_HOST_ASSET_BUILDER
            "${ALIA_HOST_BUILD_DIR}/tools/alia_asset_builder.exe")
        if(NOT EXISTS "${ALIA_HOST_ASSET_BUILDER}")
            message(FATAL_ERROR
                "Native alia_asset_builder not found at:\n"
                "  ${ALIA_HOST_ASSET_BUILDER}\n"
                "Build the desktop Release tree first "
                "(scripts/build.bat alia_widget_gallery).")
        endif()
        add_custom_command(
            OUTPUT "${out_h}" "${out_cpp}"
            COMMAND "${ALIA_HOST_ASSET_BUILDER}"
                ${_alia_font_manifests}
                "${out_h}" "${out_cpp}"
                --cache-dir "${CMAKE_BINARY_DIR}/font_cache"
            DEPENDS "${ALIA_HOST_ASSET_BUILDER}" ${_alia_font_manifests}
            COMMENT "Generating font assets (host tool) (${out_h})"
        )
    endif()
    set_source_files_properties("${out_cpp}" PROPERTIES GENERATED TRUE)
endmacro()

if(ALIA_ENABLE_ASSET_PIPELINE)
    add_subdirectory(tools)
endif()

if(ALIA_ENABLE_ASSET_PIPELINE OR (EMSCRIPTEN AND ALIA_ENABLE_EXAMPLES))
    file(MAKE_DIRECTORY "${ALIA_GEN_DIR}")
    alia_generate_font_assets(
        "${ALIA_FONTS_H}" "${ALIA_FONTS_CPP}"
        "${ALIA_FONT_MANIFEST_UI_ICONS}"
        "${ALIA_FONT_MANIFEST_TYPOGRAPHY}")
endif()

# This defines the stock atlas for the shell and most examples. Apps may link a
# richer atlas with the same exported symbols instead (see
# alia_widget_gallery_fonts).
# TODO: Use a different option test for this.
if(ALIA_ENABLE_EXAMPLES)
    add_library(alia_font_assets STATIC "${ALIA_FONTS_CPP}")
    target_include_directories(alia_font_assets PUBLIC
        "${ALIA_GEN_DIR}"
        ${CMAKE_SOURCE_DIR}/core/include)
    target_link_libraries(alia_font_assets PUBLIC alia_core)

    add_custom_target(alia_fonts_gen DEPENDS "${ALIA_FONTS_H}" "${ALIA_FONTS_CPP}")
    # Compile shell stock-text setup against the generated declarations.
    if(TARGET alia_shell)
        target_include_directories(alia_shell PRIVATE "${ALIA_GEN_DIR}")
        add_dependencies(alia_shell alia_fonts_gen)
    endif()
endif()

if(ALIA_ENABLE_EXAMPLES)
    set(ALIA_DEMO_COMMON_SOURCES
        ${PROJECT_SOURCE_DIR}/examples/common/demo_text.cpp)
    set(ALIA_DEMO_COMMON_INCLUDE
        ${PROJECT_SOURCE_DIR}/examples)

    set(ALIA_WG_FONTS_H "${ALIA_GEN_DIR}/widget_gallery/alia_fonts.h")
    set(ALIA_WG_FONTS_CPP "${ALIA_GEN_DIR}/widget_gallery/alia_fonts.cpp")
    set(ALIA_WG_FONT_MANIFEST
        "${PROJECT_SOURCE_DIR}/examples/widget_gallery/fonts.yaml")
    file(MAKE_DIRECTORY "${ALIA_GEN_DIR}/widget_gallery")
    alia_generate_font_assets(
        "${ALIA_WG_FONTS_H}" "${ALIA_WG_FONTS_CPP}"
        "${ALIA_FONT_MANIFEST_UI_ICONS}"
        "${ALIA_FONT_MANIFEST_TYPOGRAPHY}"
        "${ALIA_WG_FONT_MANIFEST}")
    add_library(alia_widget_gallery_fonts STATIC "${ALIA_WG_FONTS_CPP}")
    target_include_directories(alia_widget_gallery_fonts PUBLIC
        "${ALIA_GEN_DIR}/widget_gallery"
        ${CMAKE_SOURCE_DIR}/core/include)
    target_link_libraries(alia_widget_gallery_fonts PUBLIC alia_core)

    add_executable(alia_widget_gallery
        ${PROJECT_SOURCE_DIR}/examples/widget_gallery/main.cpp
        ${ALIA_DEMO_COMMON_SOURCES})
    target_link_libraries(alia_widget_gallery PRIVATE
        alia_cpp alia_shell_app alia_widget_gallery_fonts)
    target_include_directories(alia_widget_gallery PRIVATE
        ${ALIA_DEMO_COMMON_INCLUDE})

    add_executable(alia_layout_lab
        ${PROJECT_SOURCE_DIR}/examples/layout_lab/main.cpp
        ${ALIA_DEMO_COMMON_SOURCES})
    target_link_libraries(alia_layout_lab PRIVATE
        alia_cpp alia_shell_app alia_font_assets)
    target_include_directories(alia_layout_lab PRIVATE
        ${ALIA_DEMO_COMMON_INCLUDE})

    set(ALIA_GALLERY_SHADER_DIR
        "${PROJECT_SOURCE_DIR}/examples/shader_gallery")
    set(ALIA_NOTARGS_SLANG "${ALIA_GALLERY_SHADER_DIR}/notargs.slang")
    set(ALIA_NOTARGS_GEN_H "${ALIA_GEN_DIR}/alia_notargs_shader.h")
    set(ALIA_NOTARGS_GEN_CPP "${ALIA_GEN_DIR}/alia_notargs_shader.cpp")

    if(WIN32 AND NOT EMSCRIPTEN)
        set(ALIA_NOTARGS_FORMAT dxbc)
    else()
        set(ALIA_NOTARGS_FORMAT glsl-es)
    endif()

    if(ALIA_ENABLE_ASSET_PIPELINE AND TARGET alia_shader_builder)
        set(ALIA_SHADER_BUILDER_CMD $<TARGET_FILE:alia_shader_builder>)
        set(ALIA_SHADER_BUILDER_DEP alia_shader_builder)
        set(ALIA_SLANGC_FOR_BUILD "${ALIA_SLANGC}")
    elseif(EMSCRIPTEN)
        set(ALIA_HOST_BUILD_DIR "${CMAKE_SOURCE_DIR}/build/Release"
            CACHE PATH "Native build directory containing host tools")
        set(ALIA_SHADER_BUILDER_CMD
            "${ALIA_HOST_BUILD_DIR}/tools/alia_shader_builder.exe")
        set(ALIA_SHADER_BUILDER_DEP "${ALIA_SHADER_BUILDER_CMD}")
        set(ALIA_SLANGC_FOR_BUILD
            "${ALIA_HOST_BUILD_DIR}/slang-sdk/bin/slangc.exe")
        if(NOT EXISTS "${ALIA_SHADER_BUILDER_CMD}")
            message(FATAL_ERROR
                "Native alia_shader_builder not found at:\n"
                "  ${ALIA_SHADER_BUILDER_CMD}\n"
                "Build the desktop Release tree first "
                "(scripts/build.bat alia_shader_gallery).")
        endif()
    else()
        message(FATAL_ERROR
            "alia_shader_gallery requires alia_shader_builder "
            "(enable ALIA_ENABLE_ASSET_PIPELINE on desktop).")
    endif()

    add_custom_command(
        OUTPUT "${ALIA_NOTARGS_GEN_H}" "${ALIA_NOTARGS_GEN_CPP}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${ALIA_GEN_DIR}"
        COMMAND ${ALIA_SHADER_BUILDER_CMD}
            "${ALIA_NOTARGS_SLANG}"
            "${ALIA_NOTARGS_GEN_H}"
            "${ALIA_NOTARGS_GEN_CPP}"
            --format "${ALIA_NOTARGS_FORMAT}"
            --entry ps_main
            --symbol alia_notargs_shader
            --slangc "${ALIA_SLANGC_FOR_BUILD}"
            --include-dir "${ALIA_GALLERY_SHADER_DIR}"
        DEPENDS
            ${ALIA_SHADER_BUILDER_DEP}
            "${ALIA_NOTARGS_SLANG}"
            "${ALIA_GALLERY_SHADER_DIR}/alia_shader_frame.slang"
            "${ALIA_GALLERY_SHADER_DIR}/notargs_params.h"
        COMMENT "Baking notargs shader (${ALIA_NOTARGS_FORMAT})"
    )
    set_source_files_properties("${ALIA_NOTARGS_GEN_CPP}" PROPERTIES
        GENERATED TRUE)

    add_executable(alia_shader_gallery
        ${PROJECT_SOURCE_DIR}/examples/shader_gallery/main.cpp
        ${ALIA_DEMO_COMMON_SOURCES}
        "${ALIA_NOTARGS_GEN_CPP}")
    target_link_libraries(alia_shader_gallery PRIVATE
        alia_cpp alia_shell_app alia_font_assets)
    target_include_directories(alia_shader_gallery PRIVATE
        ${ALIA_DEMO_COMMON_INCLUDE}
        "${ALIA_GEN_DIR}"
        "${ALIA_GALLERY_SHADER_DIR}")

    if(ALIA_ENABLE_GLFW AND ALIA_ENABLE_OPENGL AND NOT EMSCRIPTEN
            AND TARGET alia_glfw)
        add_executable(alia_glfw_gl_embed
            ${PROJECT_SOURCE_DIR}/examples/glfw_gl_embed/main.cpp)
        target_link_libraries(alia_glfw_gl_embed PRIVATE
            alia_glfw
            alia_gl_renderer
            alia_font_assets)
        target_include_directories(alia_glfw_gl_embed PRIVATE
            ${PROJECT_SOURCE_DIR}/core/src)
    endif()

    if(EMSCRIPTEN)
        foreach(_alia_web_target
                alia_widget_gallery alia_layout_lab alia_shader_gallery)
            target_compile_options(${_alia_web_target} PRIVATE
                "-fwasm-exceptions")
            target_link_options(${_alia_web_target} PRIVATE
                "-sUSE_WEBGL2=1"
                "-sFULL_ES3=1"
                "-sWASM=1"
                "-fwasm-exceptions"
                "-sALLOW_MEMORY_GROWTH=1"
                "--shell-file" "${ALIA_EMSCRIPTEN_SHELL_FILE}"
            )
            set_target_properties(${_alia_web_target} PROPERTIES SUFFIX ".html")
        endforeach()
    endif()
endif()

if(ALIA_ENABLE_TESTING OR ALIA_ENABLE_BENCHMARKS)
    add_subdirectory(tests/support)
endif()

if(ALIA_ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests/core)
    add_subdirectory(tests/apis)
    add_test(NAME test_core_impl COMMAND test_core_impl)
    add_test(NAME test_core_c COMMAND test_core_c)
    add_test(NAME test_apis_cpp COMMAND test_apis_cpp)
endif()

if(ALIA_ENABLE_BENCHMARKS)
    add_executable(alia_benchmarks
        ${PROJECT_SOURCE_DIR}/benchmarks/layout.cpp)
    target_link_libraries(alia_benchmarks PRIVATE alia_layout_fixture)
    target_include_directories(alia_benchmarks PRIVATE
        ${PROJECT_SOURCE_DIR}/benchmarks)
    if (MSVC)
        target_link_options(alia_benchmarks PRIVATE
            $<$<CONFIG:RelWithDebInfo>:/PROFILE>)
    endif()
    if(ALIA_ENABLE_TESTING)
        add_test(
            NAME alia_benchmarks_smoke
            COMMAND alia_benchmarks)
        set_tests_properties(alia_benchmarks_smoke PROPERTIES
            ENVIRONMENT ALIA_BENCHMARK_SMOKE=1)
    endif()
endif()
