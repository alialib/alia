project(qt-sandbox)
cmake_minimum_required(VERSION 3.1.0)

set(CMAKE_VERBOSE_MAKEFILE TRUE)

if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(KEEP_RPATHS)
endif()

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

add_executable(sandbox WIN32 sandbox.cpp adaptor.cpp)
qt5_use_modules(sandbox Widgets)
