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

add_executable(alia_app ${PROJECT_SOURCE_DIR}/examples/app.cpp)
target_link_libraries(alia_app PRIVATE
    alia_core alia_gl_renderer alia_glfw_platform
    alia_msdf_text_engine)

add_executable(alia_benchmarks ${PROJECT_SOURCE_DIR}/benchmarks/layout.cpp)
target_link_libraries(alia_benchmarks PRIVATE
    alia_core)
if (MSVC)
    target_link_options(alia_benchmarks PRIVATE
        $<$<CONFIG:RelWithDebInfo>:/PROFILE>)
endif()
