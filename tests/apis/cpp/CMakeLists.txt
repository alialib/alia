add_executable(test_apis_cpp
    runner.cpp
    ui/layout/layout_harness.cpp
    ui/layout/test_layout_components.cpp
    ui/layout/test_layout_wrappers.cpp
    ui/layout/test_layout_flow.cpp)
target_link_libraries(test_apis_cpp PRIVATE alia_cpp)
target_include_directories(test_apis_cpp PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/support
    ${PROJECT_SOURCE_DIR}/core/src)
