# Add the test example.
if (ALIA_ENABLE_UI)
    add_executable(test_example test.cpp runner.cpp)
    target_link_libraries(test_example PUBLIC alia_glfw alia_ui unofficial::skia::skia)
endif()
