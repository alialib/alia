find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_library(alia_gl_renderer STATIC
    ${PROJECT_SOURCE_DIR}/backends/renderers/gl/src/renderer.cpp)
target_include_directories(alia_gl_renderer PUBLIC
    ${PROJECT_SOURCE_DIR}/backends/renderers/gl/include)
target_link_libraries(alia_gl_renderer PUBLIC
    glfw glad::glad alia_ui)
