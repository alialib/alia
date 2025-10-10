add_library(alia_kernel STATIC
    src/infinite_arena.cpp
    src/scratch.cpp
    src/substrate.cpp
    src/animation/cubic_bezier.cpp)
target_include_directories(alia_kernel PUBLIC
    ${PROJECT_SOURCE_DIR}/core/kernel/abi/include
    include)
