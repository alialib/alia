add_executable(test_core_c
    runner.c
    base/test_stack.c
    base/test_arena.c
    base/test_color.c
    base/geometry/test_affine2.c
    base/geometry/test_box.c
    base/geometry/test_insets.c
    base/geometry/test_vec2.c
    kernel/test_ids.c
    kernel/test_substrate.c
    kernel/substrate_harness.cpp)
target_link_libraries(test_core_c PRIVATE alia_core)
target_include_directories(test_core_c PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/support
    ${PROJECT_SOURCE_DIR}/core/src)
