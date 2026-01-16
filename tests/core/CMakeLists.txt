add_executable(test_core_cpp
    runner.cpp
    test_bit_packing.cpp
    test_color.cpp
    test_arena.cpp
    test_substrate.cpp
    animation/test_cubic_bezier.cpp
    animation/test_flares.cpp
    animation/test_transitions.cpp)
target_link_libraries(test_core_cpp PRIVATE alia_core)
target_include_directories(test_core_cpp PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/support)

add_executable(test_core_c
    runner.c
    geometry/test_vec2_api.c
    geometry/test_box_api.c
    geometry/test_insets_api.c)
target_link_libraries(test_core_c PRIVATE alia_core)
target_include_directories(test_core_c PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/support)
