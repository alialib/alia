add_executable(test_core_impl
    runner.cpp
    base/test_bit_packing.cpp
    base/test_operators.cpp
    kernel/test_timer.cpp
    kernel/animation/test_unit_cubic_bezier.cpp)
target_link_libraries(test_core_impl PRIVATE alia_core)
target_include_directories(test_core_impl PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/support
    ${PROJECT_SOURCE_DIR}/core/src)
target_compile_definitions(test_core_impl PRIVATE
    ALIA_IMPL_TESTS=1)
