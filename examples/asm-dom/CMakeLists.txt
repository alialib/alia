cmake_minimum_required (VERSION 3.11)
project(asm-dom-fun)

# Apply some emscripten optimizations flags if we're in release mode.
include(cmake/release-mode.cmake)

# Add the asm-dom library.
include(cmake/asm-dom.cmake)

# Add the nlohmann/json library.
include(cmake/json.cmake)

# Allow the use of Emscripten's Fetch API.
string(APPEND CMAKE_CXX_FLAGS " -s FETCH=1")
# Enable exceptions.
string(APPEND CMAKE_CXX_FLAGS " -s DISABLE_EXCEPTION_CATCHING=0")

# Create a library of common demo functionality.
add_library(demolib STATIC color.cpp dom.cpp)
set_property(TARGET demolib PROPERTY CXX_STANDARD 17)
target_link_libraries(demolib PRIVATE asm-dom)

# Add the sandbox.
add_executable(sandbox sandbox.cpp)
set_property(TARGET sandbox PROPERTY CXX_STANDARD 17)
target_link_libraries(sandbox PRIVATE asm-dom demolib)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/index.html
  ${CMAKE_CURRENT_BINARY_DIR}/index.html
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/sandbox.css
  ${CMAKE_CURRENT_BINARY_DIR}/sandbox.css
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/pure-material.css
  ${CMAKE_CURRENT_BINARY_DIR}/pure-material.css
)

# Add the demos target.
include_directories(.)
file(GLOB demo_files "demos/*.cpp")
add_executable(demos demos.cpp ${demo_files})
target_compile_options(demos PRIVATE
    "SHELL:-s EXPORTED_FUNCTIONS='[\"_init_demo\"]'")
target_compile_options(demos PRIVATE
    "SHELL:-s EXTRA_EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\", \"UTF8ToString\"]'")
target_compile_options(demos PRIVATE
    "SHELL:-s MODULARIZE=1")
target_compile_options(demos PRIVATE
    "SHELL:-s 'EXPORT_NAME=\"AliaDemos\"'")
set_property(TARGET demos PROPERTY CXX_STANDARD 17)
target_link_libraries(demos PRIVATE asm-dom demolib)
