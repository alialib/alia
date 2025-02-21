# Add the test example.
if (ALIA_ENABLE_UI)
    add_executable(test_example WIN32 test.cpp runner.cpp)
    target_link_libraries(test_example PUBLIC alia_wx alia_ui unofficial::skia::skia)
endif()
