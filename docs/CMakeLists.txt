cmake_minimum_required (VERSION 3.14)

include(cmake/utilities.cmake)

# Add the demos target.
include_directories(src)
file(GLOB demo_files "src/demos/*.cpp")
add_executable(demos src/entry.cpp src/color.cpp src/demo.cpp ${demo_files})
set_property(TARGET demos PROPERTY CXX_STANDARD 17)
target_link_libraries(demos PRIVATE alia_html)

# Set various other Emscripten options...
target_link_options(demos PRIVATE
    "-sEXPORTED_FUNCTIONS=[\"_init_demo\"]")
target_link_options(demos PRIVATE
    "-sEXPORTED_RUNTIME_METHODS=[\"ccall\", \"cwrap\", \"UTF8ToString\"]")
target_link_options(demos PRIVATE
    "-sMODULARIZE=1")
target_link_options(demos PRIVATE
    "-sEXPORT_NAME=\"AliaDemos\"")
