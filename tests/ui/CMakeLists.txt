add_executable(test_ui
    runner.cpp test_color.cpp)
target_link_libraries(test_ui PRIVATE alia_ui)
target_include_directories(test_ui PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/support)
