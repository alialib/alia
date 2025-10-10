add_executable(test_kernel
    runner.cpp
    test_bit_packing.cpp
    test_scratch.cpp
    test_substrate.cpp
    animation/test_cubic_bezier.cpp
    animation/test_flares.cpp
    animation/test_transitions.cpp)
target_link_libraries(test_kernel PRIVATE alia_kernel)
target_include_directories(test_kernel PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/support)
