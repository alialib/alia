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

# Create an example executable.
add_executable(example examples/main.cpp)
target_link_libraries(example alia_html)

# Create symlinks for index.html and style.css file in the build directory.
# (Or copy them if we're on Travis so that deployment works.)
if(DEFINED ENV{TRAVIS})
    file(COPY
        examples/index.html
        DESTINATION .)
    file(COPY
        examples/style.css
        DESTINATION .)
else()
    file(CREATE_LINK
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/index.html
        ${CMAKE_CURRENT_BINARY_DIR}/index.html
        SYMBOLIC)
    file(CREATE_LINK
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/style.css
        ${CMAKE_CURRENT_BINARY_DIR}/style.css
        SYMBOLIC)
endif()
