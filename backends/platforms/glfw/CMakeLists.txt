find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_library(alia_glfw_platform STATIC
    ${PROJECT_SOURCE_DIR}/backends/platforms/glfw/src/window.cpp)
target_include_directories(alia_glfw_platform PUBLIC
    ${PROJECT_SOURCE_DIR}/backends/platforms/glfw/include)
target_link_libraries(alia_glfw_platform PUBLIC
    glfw glad::glad alia_ui alia_foundation)
