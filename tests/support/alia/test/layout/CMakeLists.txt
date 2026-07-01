add_library(alia_layout_fixture STATIC
    layout_fixture.cpp
    scenarios.c)

target_include_directories(alia_layout_fixture PUBLIC
    ${PROJECT_SOURCE_DIR}/tests/support)

target_include_directories(alia_layout_fixture PRIVATE
    ${PROJECT_SOURCE_DIR}/core/src)

target_link_libraries(alia_layout_fixture PUBLIC alia_core)
