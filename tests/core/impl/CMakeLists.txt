add_executable(test_core_impl
    runner.cpp
    base/geometry/test_operators.cpp
    base/geometry/test_vec2.cpp
    base/test_bit_packing.cpp)
target_link_libraries(test_core_impl PRIVATE alia_core)
target_include_directories(test_core_impl PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/support
    ${PROJECT_SOURCE_DIR}/core/src)
target_compile_definitions(test_core_impl PRIVATE
    ALIA_IMPL_TESTS=1)
