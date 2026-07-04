if(NOT EMSCRIPTEN)
    return()
endif()

add_library(alia_web_platform STATIC
    src/host.cpp
    src/input_glue.cpp)
target_include_directories(alia_web_platform PUBLIC
    include)
target_link_libraries(alia_web_platform PUBLIC
    alia_core alia_platform_api)
