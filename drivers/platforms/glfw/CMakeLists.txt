# GLFW integration helpers for embedding Alia in an app-owned GLFWwindow.
# This is not a product host (no window/present loop).

find_package(glfw3 CONFIG REQUIRED)

add_library(alia_glfw STATIC
    src/input_glue.cpp)
target_include_directories(alia_glfw PUBLIC
    include)
target_link_libraries(alia_glfw PUBLIC
    glfw alia_core)
