add_library(alia_foundation STATIC
    ${PROJECT_SOURCE_DIR}/core/foundation/src/arena.cpp)
target_include_directories(alia_foundation PUBLIC
    ${PROJECT_SOURCE_DIR}/core/foundation/include)
