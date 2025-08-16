add_library(alia_flow STATIC
    ${PROJECT_SOURCE_DIR}/core/base/src/arena.cpp
    ${PROJECT_SOURCE_DIR}/core/base/src/infinite_arena.cpp
    ${PROJECT_SOURCE_DIR}/core/base/src/timing/ticks.cpp)
target_include_directories(alia_flow PUBLIC
    ${PROJECT_SOURCE_DIR}/core/base/include)
