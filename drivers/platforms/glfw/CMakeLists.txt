if(EMSCRIPTEN)
    add_compile_definitions(ALIA_PLATFORM_WEB)
else()
    find_package(glad CONFIG REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
endif()

add_library(alia_glfw_platform STATIC
    src/input_glue.cpp
    src/host.cpp)
target_include_directories(alia_glfw_platform PUBLIC
    include)
if (EMSCRIPTEN)
    target_link_libraries(alia_glfw_platform PUBLIC
        glfw alia_core alia_platform_api)
else()
    target_link_libraries(alia_glfw_platform PUBLIC
        glfw glad::glad alia_core alia_platform_api)
    if(WIN32)
        find_package(Vulkan REQUIRED)
        target_sources(alia_glfw_platform PRIVATE
            src/vk_present.cpp
            src/vk_gl_ext_win32.cpp)
        target_link_libraries(alia_glfw_platform PUBLIC Vulkan::Vulkan)
        target_link_libraries(alia_glfw_platform PUBLIC winmm dwmapi opengl32)
    endif()
endif()
