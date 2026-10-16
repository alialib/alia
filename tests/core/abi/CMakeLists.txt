add_executable(test_core_c
    runner.c
    base/test_stack.c
    base/test_arena.c
    base/test_color.c
    base/geometry/test_affine2.c
    base/geometry/test_box.c
    base/geometry/test_edge_offsets.c
    base/geometry/test_vec2.c
    kernel/test_ids.c
    kernel/test_effect.c
    kernel/test_substrate.c
    kernel/test_animation.c
    ui/layout/test_abi_headers.c
    ui/layout/test_placement.c
    ui/layout/test_line.c
    ui/layout/test_popup.c
    ui/layout/test_layout_detached.c)
target_link_libraries(test_core_c PRIVATE
    alia_core alia_layout_fixture alia_substrate_fixture)
target_include_directories(test_core_c PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/support)
set_property(TARGET test_core_c PROPERTY C_STANDARD 11)
set_property(TARGET test_core_c PROPERTY C_STANDARD_REQUIRED ON)
