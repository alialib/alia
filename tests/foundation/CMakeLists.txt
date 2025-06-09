add_executable(test_foundation runner.cpp test_bit_packing.cpp test_arena.cpp)
target_link_libraries(test_foundation PRIVATE alia_foundation)
target_include_directories(test_foundation PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/support)
