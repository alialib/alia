# Copyright 2019 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.

message(STATUS "Checking for asm-dom: Please wait")
include(FetchContent)
FetchContent_Declare(asm-dom
  GIT_REPOSITORY https://github.com/mbasso/asm-dom
  GIT_TAG 49131a4ebe26383bafea213167561bffa2024f7c
)

FetchContent_GetProperties(asm-dom)
if(NOT asm-dom_POPULATED)
  FetchContent_Populate(asm-dom)
  add_library(asm-dom
    ${asm-dom_SOURCE_DIR}/cpp/asm-dom.cpp
    ${asm-dom_SOURCE_DIR}/cpp/asm-dom-server.cpp
    ${asm-dom_SOURCE_DIR}/cpp/asm-dom.hpp
    ${asm-dom_SOURCE_DIR}/cpp/asm-dom-server.hpp
  )
  set_property(TARGET asm-dom PROPERTY CXX_STANDARD 11)
  target_include_directories(asm-dom PUBLIC ${asm-dom_SOURCE_DIR}/cpp/)
  configure_file(
    ${asm-dom_SOURCE_DIR}/dist/cpp/asm-dom.js
    ${CMAKE_CURRENT_BINARY_DIR}/asm-dom.js
  )

  string(APPEND CMAKE_CXX_FLAGS " -s EXTRA_EXPORTED_RUNTIME_METHODS='UTF8ToString'")
  string(APPEND CMAKE_CXX_FLAGS " -s WASM=1 --bind")
endif()
message(STATUS "Checking for asm-dom: DONE")
