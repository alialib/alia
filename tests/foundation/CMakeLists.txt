add_executable(test_arena test_arena.cpp)
target_link_libraries(test_arena PRIVATE alia_foundation)
target_include_directories(test_arena PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/support)
