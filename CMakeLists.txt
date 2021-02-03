cmake_minimum_required (VERSION 3.14)
project(asm-dom-fun)

set(CMAKE_CXX_STANDARD 17)

# Add the asm-dom library.
include(cmake/asm-dom.cmake)

# Add the nlohmann/json library.
include(cmake/json.cmake)

# Enable properties!
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdeclspec")

# Allow the use of Emscripten's Fetch API.
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s FETCH=1")
# Enable exceptions.
string(APPEND CMAKE_CXX_FLAGS " -s DISABLE_EXCEPTION_CATCHING=0")

# Add scnlib.
add_subdirectory(external/scnlib EXCLUDE_FROM_ALL)

# Set some Emscripten optimizations flags for release mode.
if (CMAKE_BUILD_TYPE STREQUAL "Release")
  string(APPEND CMAKE_CXX_FLAGS " -Oz")
  string(APPEND CMAKE_CXX_FLAGS " --js-opts 3")
  string(APPEND CMAKE_CXX_FLAGS " -flto")
  string(APPEND CMAKE_CXX_FLAGS " --llvm-opts 3")
  string(APPEND CMAKE_CXX_FLAGS " --closure 0")
endif()

# Download alia.hpp and set it up as a library.
# TODO: Improve this!
file(DOWNLOAD
    https://tmadden.github.io/alia/alia.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/alia.hpp)
file(DOWNLOAD
    https://tmadden.github.io/alia/alia.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/alia.cpp)
add_library(alia ${CMAKE_CURRENT_BINARY_DIR}/alia.cpp)
target_compile_definitions(alia PRIVATE -DALIA_IMPLEMENTATION)
target_include_directories(alia PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

# Set up the alia-html library.
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_library(alia_html STATIC ${SOURCES})
target_link_libraries(alia_html
    PUBLIC asm-dom scn::scn alia nlohmann_json::nlohmann_json)
target_include_directories(alia_html PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

# "Deploy" the given file to the given directory.
# In normal development, this creates a symlink of the file for more rapid
# iteration.
# In CI, it sets up a post-build step to copy the file.
function(deploy target file_path destination_directory)
    get_filename_component(file_name "${file_path}" NAME)
    if(DEFINED ENV{CI})
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${file_path}
                ${destination_directory}/${file_name})
    else()
        file(CREATE_LINK ${file_path}
               ${destination_directory}/${file_name}
               SYMBOLIC)
    endif()
endfunction()

# Set up the build for an example.
function(define_example name)
    file(GLOB source_files "${CMAKE_CURRENT_SOURCE_DIR}/examples/${name}/*")
    add_executable(${name}
        examples/${name}/main.cpp
        examples/demo-utilities/utilities.cpp)
    set_target_properties(${name} PROPERTIES OUTPUT_NAME ${name})
    target_link_libraries(${name} alia_html)
    set(deploy_dir "${CMAKE_CURRENT_BINARY_DIR}/deploy/${name}")
    file(MAKE_DIRECTORY ${deploy_dir})
    foreach(file_path ${source_files})
        deploy(${name} ${file_path} ${deploy_dir})
    endforeach()
    deploy(${name} ${CMAKE_CURRENT_BINARY_DIR}/${name}.js ${deploy_dir})
    deploy(${name} ${CMAKE_CURRENT_BINARY_DIR}/${name}.wasm ${deploy_dir})
    deploy(${name} ${CMAKE_CURRENT_BINARY_DIR}/asm-dom.js ${deploy_dir})
endfunction()

# Define the examples.
define_example(bootstrap-demo)
