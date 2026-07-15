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

# Product shells: web+GL on Emscripten, Win32+D3D11 on Windows desktop.
if(EMSCRIPTEN AND ALIA_ENABLE_OPENGL)
    alia_add_host_library()
    add_subdirectory(shell/gl)
endif()

if(WIN32 AND NOT EMSCRIPTEN AND TARGET alia_win32_platform)
    alia_add_host_library()
    add_subdirectory(shell/d3d11)
endif()

add_subdirectory(apis/cpp)

set(ALIA_GEN_DIR "${CMAKE_BINARY_DIR}/gen"
    CACHE PATH "directory containing generated alia font sources")

set(ALIA_FONTS_H "${ALIA_GEN_DIR}/alia_fonts.h")
set(ALIA_FONTS_CPP "${ALIA_GEN_DIR}/alia_fonts.cpp")
set(ALIA_FONT_MANIFEST "${CMAKE_SOURCE_DIR}/assets/fonts.yaml")

if(ALIA_ENABLE_ASSET_PIPELINE)
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
elseif(EMSCRIPTEN AND ALIA_ENABLE_EXAMPLES)
    # Cross-compile: run the native asset builder from a desktop build tree.
    set(ALIA_HOST_BUILD_DIR "${CMAKE_SOURCE_DIR}/build/Release"
        CACHE PATH "Native build directory containing tools/alia_asset_builder")
    set(ALIA_HOST_ASSET_BUILDER
        "${ALIA_HOST_BUILD_DIR}/tools/alia_asset_builder.exe")
    if(NOT EXISTS "${ALIA_HOST_ASSET_BUILDER}")
        message(FATAL_ERROR
            "Native alia_asset_builder not found at:\n"
            "  ${ALIA_HOST_ASSET_BUILDER}\n"
            "Build the desktop Release tree first (scripts/build.bat alia_app).")
    endif()
    file(MAKE_DIRECTORY "${ALIA_GEN_DIR}")
    add_custom_command(
        OUTPUT "${ALIA_FONTS_H}" "${ALIA_FONTS_CPP}"
        COMMAND "${ALIA_HOST_ASSET_BUILDER}"
            "${ALIA_FONT_MANIFEST}" "${ALIA_FONTS_H}" "${ALIA_FONTS_CPP}"
            --cache-dir "${CMAKE_BINARY_DIR}/font_cache"
        DEPENDS "${ALIA_HOST_ASSET_BUILDER}" "${ALIA_FONT_MANIFEST}"
        COMMENT "Generating Alia font assets (host tool)"
    )
    set_source_files_properties("${ALIA_FONTS_CPP}" PROPERTIES GENERATED TRUE)
endif()

# TODO: Use a different option test for this.
if(ALIA_ENABLE_EXAMPLES)
    add_library(alia_font_assets STATIC "${ALIA_FONTS_CPP}")
    target_include_directories(alia_font_assets PUBLIC
        "${ALIA_GEN_DIR}"
        ${CMAKE_SOURCE_DIR}/core/include)
    target_link_libraries(alia_font_assets PUBLIC alia_core)

    # Stock widget icons (e.g. checkbox checkmark) compile against generated
    # glyph IDs so remapped private-use codepoints stay in sync per build tree.
    add_custom_target(alia_fonts_gen DEPENDS "${ALIA_FONTS_H}" "${ALIA_FONTS_CPP}")
    add_dependencies(alia_core alia_fonts_gen)
    target_include_directories(alia_core PRIVATE "${ALIA_GEN_DIR}")
    target_compile_definitions(alia_core PRIVATE ALIA_HAS_STOCK_FONTS=1)
    set_source_files_properties(
        "${CMAKE_SOURCE_DIR}/core/src/alia/ui/library/checkbox.cpp"
        PROPERTIES OBJECT_DEPENDS "${ALIA_FONTS_H}")
    if(TARGET alia_gl_shell)
        target_link_libraries(alia_gl_shell PUBLIC alia_font_assets)
    endif()
    if(TARGET alia_d3d11_shell)
        target_link_libraries(alia_d3d11_shell PUBLIC alia_font_assets)
    endif()
endif()

if(ALIA_ENABLE_EXAMPLES)
    add_executable(alia_app
        ${PROJECT_SOURCE_DIR}/examples/app.cpp)
    if(WIN32 AND TARGET alia_d3d11_shell)
        target_link_libraries(alia_app PRIVATE alia_cpp alia_d3d11_shell)
        target_compile_definitions(alia_app PRIVATE ALIA_APP_USE_D3D11=1)
    else()
        target_link_libraries(alia_app PRIVATE alia_cpp alia_gl_shell)
    endif()

    add_executable(alia_shader_gallery
        ${PROJECT_SOURCE_DIR}/examples/shader_gallery_app.cpp)
    if(WIN32 AND TARGET alia_d3d11_shell)
        target_link_libraries(alia_shader_gallery PRIVATE alia_cpp alia_d3d11_shell)
        target_compile_definitions(alia_shader_gallery PRIVATE ALIA_GALLERY_USE_D3D11=1)
    else()
        target_link_libraries(alia_shader_gallery PRIVATE alia_cpp alia_gl_shell)
    endif()

    if(ALIA_ENABLE_GLFW AND ALIA_ENABLE_OPENGL AND NOT EMSCRIPTEN
            AND TARGET alia_glfw)
        add_executable(alia_glfw_gl_embed
            ${PROJECT_SOURCE_DIR}/examples/glfw_gl_embed_app.cpp)
        target_link_libraries(alia_glfw_gl_embed PRIVATE
            alia_glfw
            alia_gl_renderer
            alia_font_assets)
        target_include_directories(alia_glfw_gl_embed PRIVATE
            ${PROJECT_SOURCE_DIR}/core/src)
    endif()

    if(EMSCRIPTEN)
        target_compile_options(alia_app PRIVATE
            "-fwasm-exceptions")
        target_compile_options(alia_shader_gallery PRIVATE
            "-fwasm-exceptions")

        target_link_options(alia_app PRIVATE
            "-sUSE_WEBGL2=1"
            "-sFULL_ES3=1"
            "-sWASM=1"
            "-fwasm-exceptions"
            "-sALLOW_MEMORY_GROWTH=1"
            "--preload-file" "${CMAKE_SOURCE_DIR}/assets@/assets"
            "--shell-file" "${ALIA_EMSCRIPTEN_SHELL_FILE}"
        )
        target_link_options(alia_shader_gallery PRIVATE
            "-sUSE_WEBGL2=1"
            "-sFULL_ES3=1"
            "-sWASM=1"
            "-fwasm-exceptions"
            "-sALLOW_MEMORY_GROWTH=1"
            "--preload-file" "${CMAKE_SOURCE_DIR}/assets@/assets"
            "--shell-file" "${ALIA_EMSCRIPTEN_SHELL_FILE}"
        )
        set_target_properties(alia_app PROPERTIES SUFFIX ".html")
        set_target_properties(alia_shader_gallery PROPERTIES SUFFIX ".html")
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
