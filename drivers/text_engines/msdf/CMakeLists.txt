find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_library(alia_msdf_text_engine STATIC
    src/msdf.cpp)
target_include_directories(alia_msdf_text_engine PUBLIC
    include)
target_link_libraries(alia_msdf_text_engine PUBLIC
    glfw glad::glad alia_core alia_gl_renderer)
