if(EMSCRIPTEN)
    add_compile_definitions(ALIA_PLATFORM_WEB)
else()
    find_package(glad CONFIG REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
endif()

add_library(alia_msdf_text_engine STATIC
    src/msdf.cpp)
target_include_directories(alia_msdf_text_engine PUBLIC
    include)
if (EMSCRIPTEN)
    target_link_libraries(alia_msdf_text_engine PUBLIC
        glfw alia_core alia_gl_renderer)
else()
    target_link_libraries(alia_msdf_text_engine PUBLIC
        glfw glad::glad alia_core alia_gl_renderer)
endif()
