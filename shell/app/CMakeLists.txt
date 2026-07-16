add_library(alia_shell_app STATIC
    src/app.cpp)
target_include_directories(alia_shell_app PUBLIC
    include)
target_link_libraries(alia_shell_app PUBLIC
    alia_shell
    alia_host)

# Product GPU backend: D3D11 on Windows desktop, GL elsewhere (web).
if(WIN32 AND NOT EMSCRIPTEN AND TARGET alia_d3d11_renderer)
    target_link_libraries(alia_shell_app PUBLIC alia_d3d11_renderer)
    target_compile_definitions(alia_shell_app PUBLIC ALIA_SHELL_BACKEND_D3D11=1)
elseif(TARGET alia_gl_renderer)
    target_link_libraries(alia_shell_app PUBLIC alia_gl_renderer)
    target_compile_definitions(alia_shell_app PUBLIC ALIA_SHELL_BACKEND_GL=1)
else()
    message(FATAL_ERROR
        "alia_shell_app requires alia_d3d11_renderer (Windows) or alia_gl_renderer")
endif()
