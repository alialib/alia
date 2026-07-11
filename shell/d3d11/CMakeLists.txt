add_library(alia_d3d11_shell STATIC
    src/shell.cpp
    src/app.cpp
    src/fonts.cpp)
target_include_directories(alia_d3d11_shell PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/core/src)
target_link_libraries(alia_d3d11_shell PUBLIC
    alia_core
    alia_d3d11_renderer
    alia_host)
