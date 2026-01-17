add_executable(test_core_cpp
    runner.cpp
    test_bit_packing.cpp
    test_color.cpp
    test_arena.cpp
    test_substrate.cpp
    test_operators.cpp
    animation/test_cubic_bezier.cpp
    animation/test_flares.cpp
    animation/test_transitions.cpp)
target_link_libraries(test_core_cpp PRIVATE alia_core)
target_include_directories(test_core_cpp PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/support)
