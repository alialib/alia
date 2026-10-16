add_library(alia_substrate_fixture STATIC
    substrate_fixture.cpp)

target_include_directories(alia_substrate_fixture PUBLIC
    ${PROJECT_SOURCE_DIR}/tests/support)

target_include_directories(alia_substrate_fixture PRIVATE
    ${PROJECT_SOURCE_DIR}/core/src)

target_link_libraries(alia_substrate_fixture PUBLIC alia_core)
