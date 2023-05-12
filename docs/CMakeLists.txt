cmake_minimum_required (VERSION 3.14)
project(alia-docs)

# Add alia-html.
include(cmake/alia-html.cmake)

# Add the nlohmann/json library.
include(cmake/json.cmake)

# Add the demos target.
include_directories(src)
file(GLOB demo_files "src/demos/*.cpp")
add_executable(demos src/entry.cpp src/color.cpp src/demo.cpp ${demo_files})
set_property(TARGET demos PROPERTY CXX_STANDARD 17)
target_link_libraries(demos PRIVATE alia_html)

# Set various other Emscripten options...
# TODO: Improve some of this from the alia-html side.
string(APPEND CMAKE_CXX_FLAGS
    " -s EXPORTED_FUNCTIONS='[\"_init_demo\"]'")
string(APPEND CMAKE_CXX_FLAGS
    " -s EXTRA_EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\", \"UTF8ToString\"]'")
string(APPEND CMAKE_CXX_FLAGS
    " -s MODULARIZE=1")
string(APPEND CMAKE_CXX_FLAGS
    " -s 'EXPORT_NAME=\"AliaDemos\"'")
string(APPEND CMAKE_CXX_FLAGS
    " -s ENVIRONMENT=web")
string(APPEND CMAKE_CXX_FLAGS
    " -s WASM=1 --bind")
