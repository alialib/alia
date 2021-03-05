cmake_minimum_required (VERSION 3.14)
project(alia-html)

set(CMAKE_CXX_STANDARD 17)

# Determine if we're being built through FetchContent.
if (${CMAKE_SOURCE_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
    set(is_fetched FALSE)
else()
    set(is_fetched TRUE)
endif()

# Add the asm-dom library.
include(cmake/asm-dom.cmake)

# Add scnlib.
add_subdirectory(external/scnlib EXCLUDE_FROM_ALL)

# Allow the use of Emscripten's Fetch API.
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s FETCH=1")
# Enable exceptions.
string(APPEND CMAKE_CXX_FLAGS " -s DISABLE_EXCEPTION_CATCHING=0")

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
    https://alialib.github.io/alia/alia.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/alia.hpp)
file(DOWNLOAD
    https://alialib.github.io/alia/alia.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/alia.cpp)
add_library(alia ${CMAKE_CURRENT_BINARY_DIR}/alia.cpp)
target_compile_definitions(alia PRIVATE -DALIA_IMPLEMENTATION)
target_include_directories(alia PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

# Set up the alia-html library.
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_library(alia_html STATIC ${SOURCES})
target_link_libraries(alia_html
    PUBLIC asm-dom scn::scn alia)
target_include_directories(alia_html PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

# The rest of this is concerned with building/deploying the examples and demos,
# which we don't want to do if we're just building as a dependency.
if (is_fetched)
    return()
endif()

# Add the nlohmann/json library.
include(cmake/json.cmake)

# "Deploy" the given file to the given directory.
# In normal development, this creates a symlink of the file for more rapid
# iteration.
# In CI, it copies the file.
function(deploy file_path destination_directory)
    if(DEFINED ENV{CI})
        file(COPY
            ${file_path}
            DESTINATION ${destination_directory})
    else()
        get_filename_component(file_name "${file_path}" NAME)
        file(CREATE_LINK
            ${file_path}
            ${destination_directory}/${file_name}
            SYMBOLIC)
    endif()
endfunction()

# "Deploy" a built file to the given directory.
# In normal development, this creates a symlink of the file for more rapid
# iteration.
# In CI, it sets up a post-build step to copy the file.
function(post_build_deploy target file_path destination_directory)
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

# Create a subdirectory in the build dir that we'll use to accumulate all the
# files that need to be deployed to the web.
set(deploy_dir ${CMAKE_CURRENT_BINARY_DIR}/deploy)
file(MAKE_DIRECTORY ${deploy_dir})

# Deploy the root index.html file.
deploy(
    "${CMAKE_CURRENT_SOURCE_DIR}/demos/index.html"
    "${deploy_dir}")
# Extract and deploy the favicon.
file(ARCHIVE_EXTRACT
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/demos/favicon.zip"
    DESTINATION "${deploy_dir}")

# On CI, create the CNAME file for GitHub Pages.
if(DEFINED ENV{CI})
    file(WRITE ${deploy_dir}/CNAME "html.alia.dev")
endif()

# Set up the build for an example.
function(add_example dir)
    get_filename_component(name "${dir}" NAME)
    file(GLOB source_files "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/*")
    file(GLOB cpp_files "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/*.cpp")
    add_executable(${name} ${cpp_files})
    target_link_libraries(${name} alia_html)
    set(deploy_subdir "${deploy_dir}/${dir}")
    file(MAKE_DIRECTORY ${deploy_subdir})
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${deploy_subdir}
        OUTPUT_NAME app)
    foreach(file_path ${source_files})
        deploy(${file_path} ${deploy_subdir})
    endforeach()
    post_build_deploy(${name}
        ${CMAKE_CURRENT_BINARY_DIR}/asm-dom.js
        ${deploy_subdir})
endfunction()

# Set up the build for a demo.
function(add_demo dir)
    get_filename_component(name "${dir}" NAME)
    add_example(${dir})
    target_sources(${name} PRIVATE
        demos/demolib/utilities.cpp)
    target_link_libraries(${name} nlohmann_json::nlohmann_json)
    set(deploy_subdir "${deploy_dir}/${dir}")
    deploy(
        ${CMAKE_CURRENT_SOURCE_DIR}/demos/demolib/demo.css
        ${deploy_subdir})
    deploy(
        ${CMAKE_CURRENT_SOURCE_DIR}/demos/demolib/index.html
        ${deploy_subdir})
endfunction()

# Add the demos.
add_demo(demos/bootstrap)
add_demo(demos/routing)
add_demo(demos/io)
add_demo(demos/elements)

# Add the examples.
add_example(examples/todo-mvc)
target_include_directories(todo-mvc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/external/cereal/include)
