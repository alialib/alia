add_library(alia_d3d11_renderer STATIC
    src/renderer.cpp)
target_include_directories(alia_d3d11_renderer PUBLIC
    include
    ${ALIA_SOURCE_DIR}/core/include
    ${ALIA_SOURCE_DIR}/core/src)
target_link_libraries(alia_d3d11_renderer PUBLIC
    alia_core
    d3d11
    d3dcompiler
    dxgi)
