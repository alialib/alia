add_library(alia_win32_platform STATIC
    src/host.cpp
    src/input_glue.cpp)
target_include_directories(alia_win32_platform PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/drivers/platforms/api/include
    ${CMAKE_SOURCE_DIR}/core/include)
target_include_directories(alia_win32_platform PRIVATE
    ${CMAKE_SOURCE_DIR}/core/src)
target_link_libraries(alia_win32_platform PUBLIC
    alia_core
    d3d11
    dxgi
    dcomp
    user32
    gdi32
    winmm)
