cmake_minimum_required (VERSION 3.11)
project(asm-dom-fun)

# Apply some emscripten optimizations flags if we're in release mode.
include(cmake/release-mode.cmake)

# Add the asm-dom library.
include(cmake/asm-dom.cmake)

# Allow the use of Emscripten's Fetch API.
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s FETCH=1")

# Add the various demo targets...

add_library(demolib STATIC color.cpp dom.cpp)
set_property(TARGET demolib PROPERTY CXX_STANDARD 17)
target_link_libraries(demolib PRIVATE asm-dom)

function(add_demo_target demo_name)
    add_executable(${demo_name} ${demo_name}.cpp)
    set_property(TARGET ${demo_name} PROPERTY CXX_STANDARD 17)
    target_link_libraries(${demo_name} PRIVATE asm-dom demolib)
endfunction()

add_demo_target(introduction)

# Copy the index.html file into the build directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/index.html
    ${CMAKE_CURRENT_BINARY_DIR}/index.html
)
# Copy the style.ss file into the build directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/style.css
    ${CMAKE_CURRENT_BINARY_DIR}/style.css
)
