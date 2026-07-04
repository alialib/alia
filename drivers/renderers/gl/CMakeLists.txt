if(EMSCRIPTEN)
    add_compile_definitions(ALIA_PLATFORM_WEB)
else()
    find_package(glad CONFIG REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
endif()

add_library(alia_gl_renderer STATIC
    src/renderer.cpp
    src/shaders.cpp)
target_include_directories(alia_gl_renderer PUBLIC
    include)
target_link_libraries(alia_gl_renderer PUBLIC alia_core)
if(NOT EMSCRIPTEN)
    target_link_libraries(alia_gl_renderer PUBLIC glad::glad glfw)
endif()
