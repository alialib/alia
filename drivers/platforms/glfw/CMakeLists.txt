if(EMSCRIPTEN)
    add_compile_definitions(ALIA_PLATFORM_WEB)
else()
    find_package(glad CONFIG REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
endif()

add_library(alia_glfw_platform STATIC
    src/window.cpp)
target_include_directories(alia_glfw_platform PUBLIC
    include)
if (EMSCRIPTEN)
    target_link_libraries(alia_glfw_platform PUBLIC
        glfw alia_core)
else()
    target_link_libraries(alia_glfw_platform PUBLIC
        glfw glad::glad alia_core)
endif()
