cmake_minimum_required(VERSION 3.16)
project(alia)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_subdirectory(core)

add_subdirectory(tests/core)
add_test(NAME test_core COMMAND test_core)

add_subdirectory(drivers/renderers/gl)

add_subdirectory(drivers/platforms/glfw)

add_subdirectory(drivers/text_engines/msdf)

add_subdirectory(apis/cpp)

add_executable(alia_app ${PROJECT_SOURCE_DIR}/examples/app.cpp)
target_link_libraries(alia_app PRIVATE
    alia_cpp alia_core alia_gl_renderer alia_glfw_platform
    alia_msdf_text_engine)

# --- Linking & Flags ---
if(EMSCRIPTEN)
    # 1. Compiler/Linker Flags for Web
    # target_compile_options(alia_app PRIVATE -std=c++20)

    # 2. Emscripten Specific Linker Flags
    # -s USE_GLFW=3    -> Use internal GLFW
    # -s USE_WEBGL2=1  -> Enable WebGL 2
    # --preload-file   -> Map source assets to virtual filesystem
    target_compile_options(alia_app PRIVATE
        "-fwasm-exceptions")
    target_link_options(alia_app PRIVATE
        "-sUSE_GLFW=3"
        "-sUSE_WEBGL2=1"
        "-sFULL_ES3=1"
        "-sWASM=1"
        "-fwasm-exceptions"
        "-sALLOW_MEMORY_GROWTH=1"
        "--preload-file" "${CMAKE_SOURCE_DIR}/assets@/assets" # <--- The Magic Map
        "--shell-file" "${CMAKE_SOURCE_DIR}/shell_minimal.html" # Optional: Custom HTML template
    )

    # Tell CMake to generate .html instead of .js or .wasm default
    set_target_properties(alia_app PROPERTIES SUFFIX ".html")
endif()

add_executable(alia_benchmarks ${PROJECT_SOURCE_DIR}/benchmarks/layout.cpp)
target_link_libraries(alia_benchmarks PRIVATE
    alia_core)
if (MSVC)
    target_link_options(alia_benchmarks PRIVATE
        $<$<CONFIG:RelWithDebInfo>:/PROFILE>)
endif()
