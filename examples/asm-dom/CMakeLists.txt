cmake_minimum_required (VERSION 3.11)
project(asm-dom-fun)

# Apply some emscripten optimizations flags if we're in release mode.
include(cmake/release-mode.cmake)

# Add the asm-dom library.
include(cmake/asm-dom.cmake)

# Allow the use of Emscripten's Fetch API.
string(APPEND CMAKE_CXX_FLAGS " -s FETCH=1")

# Create a library of common demo functionality.
add_library(demolib STATIC color.cpp dom.cpp)
set_property(TARGET demolib PROPERTY CXX_STANDARD 17)
target_link_libraries(demolib PRIVATE asm-dom)

# Add the demos target.
include_directories(.)
string(APPEND CMAKE_CXX_FLAGS " -s EXPORTED_FUNCTIONS='[\"_init_demo\"]'")
string(APPEND CMAKE_CXX_FLAGS
    " -s EXTRA_EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\", \"UTF8ToString\"]'")
string(APPEND CMAKE_CXX_FLAGS
    " -s MODULARIZE=1 -s 'EXPORT_NAME=\"AliaDemos\"'")
file(GLOB demo_files "demos/*.cpp")
add_executable(demos demos.cpp ${demo_files})
set_property(TARGET demos PROPERTY CXX_STANDARD 17)
target_link_libraries(demos PRIVATE asm-dom demolib)
