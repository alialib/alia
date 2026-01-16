add_library(alia_cpp INTERFACE)
target_include_directories(alia_cpp INTERFACE
    ${PROJECT_SOURCE_DIR}/apis/cpp/include)
target_link_libraries(alia_cpp INTERFACE alia_core)
