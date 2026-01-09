add_executable(test_core
    runner.cpp
    test_bit_packing.cpp
    test_color.cpp
    test_arena.cpp
    test_substrate.cpp
    animation/test_cubic_bezier.cpp
    animation/test_flares.cpp
    animation/test_transitions.cpp)
target_link_libraries(test_core PRIVATE alia_core)
target_include_directories(test_core PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/support)
