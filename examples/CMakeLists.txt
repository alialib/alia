find_package(harfbuzz CONFIG REQUIRED)

# Add the test example.
add_executable(test_example test.cpp runner.cpp my_shaper.cpp)
target_link_libraries(test_example PRIVATE alia_glfw harfbuzz)
