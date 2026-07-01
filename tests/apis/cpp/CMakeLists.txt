add_executable(test_apis_cpp
    runner.cpp
    ui/layout/test_layout_components.cpp
    ui/layout/test_layout_wrappers.cpp
    ui/layout/test_layout_flow.cpp
    ui/layout/test_layout_provide_box.cpp
    ui/layout/test_layout_baseline.cpp)
target_link_libraries(test_apis_cpp PRIVATE alia_cpp alia_layout_fixture)
target_include_directories(test_apis_cpp PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/support
    ${PROJECT_SOURCE_DIR}/core/src)
