add_executable(test_core_c
    runner.c
    geometry/test_affine2.c
    geometry/test_box.c
    geometry/test_insets.c
    geometry/test_vec2.c)
target_link_libraries(test_core_c PRIVATE alia_core)
target_include_directories(test_core_c PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/support)
