set(ALIA_EMSCRIPTEN_SHELL_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/shell_minimal.html"
    PARENT_SCOPE)

add_library(alia_gl_shell STATIC
    src/shell.cpp
    src/app.cpp
    src/fonts.cpp)
target_include_directories(alia_gl_shell PUBLIC
    include)
target_link_libraries(alia_gl_shell PUBLIC
    alia_core alia_gl_renderer alia_host)
