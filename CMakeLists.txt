cmake_minimum_required(VERSION 3.16)
project(alia)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_subdirectory(core/kernel/cpp)

add_subdirectory(tests/kernel)
add_test(NAME test_kernel COMMAND test_kernel)

add_subdirectory(core/ui/cpp)

add_subdirectory(backends/renderers/gl)

add_subdirectory(backends/platforms/glfw)

add_subdirectory(backends/text_engines/msdf)

add_executable(alia_app ${PROJECT_SOURCE_DIR}/examples/app.cpp)
target_link_libraries(alia_app PRIVATE
    alia_ui alia_gl_renderer alia_glfw_platform
    alia_msdf_text_engine)

add_executable(alia_benchmarks ${PROJECT_SOURCE_DIR}/benchmarks/layout.cpp)
target_link_libraries(alia_benchmarks PRIVATE
    alia_ui)
if (MSVC)
    target_link_options(alia_benchmarks PRIVATE
        $<$<CONFIG:RelWithDebInfo>:/PROFILE>)
endif()
