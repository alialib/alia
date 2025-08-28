add_library(alia_flow STATIC
    src/infinite_arena.cpp
    src/scratch.cpp)
target_include_directories(alia_flow PUBLIC
    ${PROJECT_SOURCE_DIR}/core/kernel/abi/include
    include)
