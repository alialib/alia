add_executable(test_flow
    runner.cpp test_arena.cpp test_bit_packing.cpp test_scratch.cpp)
target_link_libraries(test_flow PRIVATE alia_flow)
target_include_directories(test_flow PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/support)
