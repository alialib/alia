cmake_minimum_required (VERSION 3.11)
project(asm-dom-fun)

# Apply some emscripten optimizations flags if we're in release mode.
include(cmake/release-mode.cmake)

# Add the asm-dom library.
include(cmake/asm-dom.cmake)

# Allow the use of Emscripten's Fetch API.
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s FETCH=1")

# Create a library of common demo functionality.
add_library(demolib STATIC color.cpp dom.cpp)
set_property(TARGET demolib PROPERTY CXX_STANDARD 17)
target_link_libraries(demolib PRIVATE asm-dom)

# Add the various demo targets...

include_directories(.)

function(add_demo_target cpp_file)
    get_filename_component(cpp_name ${cpp_file} NAME_WE)
    string(REPLACE "_" "-" demo_name ${cpp_name})
    add_executable(${demo_name} ${cpp_file})
    set_property(TARGET ${demo_name} PROPERTY CXX_STANDARD 17)
    target_link_libraries(${demo_name} PRIVATE asm-dom demolib)
endfunction()

file(GLOB demo_cpp_files "pages/*.cpp")
foreach(cpp_file ${demo_cpp_files})
    add_demo_target(${cpp_file})
    message(STATUS "X=${X}")
endforeach()
