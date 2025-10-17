find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_library(alia_gl_renderer STATIC
    src/renderer.cpp)
target_include_directories(alia_gl_renderer PUBLIC
    include)
target_link_libraries(alia_gl_renderer PUBLIC
    glfw glad::glad alia_core)
