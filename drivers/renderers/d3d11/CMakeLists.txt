add_library(alia_d3d11_renderer STATIC
    src/renderer.cpp)
target_include_directories(alia_d3d11_renderer PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/core/src)
target_link_libraries(alia_d3d11_renderer PUBLIC
    alia_core
    d3d11
    d3dcompiler
    dxgi)
