cmake_minimum_required(VERSION 3.16)
project(alia)

add_subdirectory(core/foundation)

enable_testing()
add_subdirectory(tests/foundation)
add_test(NAME test_arena COMMAND test_arena)
