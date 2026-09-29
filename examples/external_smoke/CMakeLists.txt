cmake_minimum_required(VERSION 3.16)

# Standalone smoke: nest Alia with examples/tests off and stock fonts on.
# Configure from this directory (see build.bat). Not part of Alia's example
# targets.

set(ALIA_ENABLE_EXAMPLES OFF CACHE BOOL "off for external nest" FORCE)
set(ALIA_ENABLE_TESTING OFF CACHE BOOL "off for external nest" FORCE)
set(ALIA_ENABLE_BENCHMARKS OFF CACHE BOOL "off for external nest" FORCE)
set(ALIA_ENABLE_GLFW OFF CACHE BOOL "off for external nest" FORCE)
set(ALIA_ENABLE_STOCK_FONTS ON CACHE BOOL "" FORCE)
set(ALIA_ENABLE_ASSET_PIPELINE ON CACHE BOOL "" FORCE)

project(alia_external_smoke LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    alia
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
FetchContent_MakeAvailable(alia)

# Add the stock UI icons/typography plus this demo's fonts.
alia_add_font_assets(smoke_fonts
    MANIFESTS
        "${ALIA_FONT_MANIFEST_UI_ICONS}"
        "${ALIA_FONT_MANIFEST_TYPOGRAPHY}"
        "${CMAKE_CURRENT_SOURCE_DIR}/fonts.yaml")

add_executable(alia_external_smoke main.cpp)
target_link_libraries(alia_external_smoke PRIVATE
    alia_cpp
    alia_shell_app
    smoke_fonts)
