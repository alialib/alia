find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_library(alia_msdf_text_engine STATIC
    ${PROJECT_SOURCE_DIR}/backends/text_engines/msdf/src/msdf.cpp)
target_include_directories(alia_msdf_text_engine PUBLIC
    ${PROJECT_SOURCE_DIR}/backends/text_engines/msdf/include)
target_link_libraries(alia_msdf_text_engine PUBLIC
    glfw glad::glad alia_ui alia_foundation alia_gl_renderer)
