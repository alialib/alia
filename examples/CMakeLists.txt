# Add the test example.
add_executable(test_example test.cpp runner.cpp)
target_link_libraries(test_example PRIVATE alia_sdl)
