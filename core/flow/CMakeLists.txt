add_library(alia_flow STATIC
    ${PROJECT_SOURCE_DIR}/core/flow/src/arena.cpp
    ${PROJECT_SOURCE_DIR}/core/flow/src/infinite_arena.cpp)
target_include_directories(alia_flow PUBLIC
    ${PROJECT_SOURCE_DIR}/core/flow/include)
