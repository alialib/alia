add_library(alia_ui STATIC
    ${PROJECT_SOURCE_DIR}/core/ui/src/drawing.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/display_list.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/layout/api.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/layout/hbox.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/layout/vbox.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/layout/resolution.cpp)
target_include_directories(alia_ui PUBLIC
    ${PROJECT_SOURCE_DIR}/core/ui/include)
target_link_libraries(alia_ui PRIVATE alia_foundation)
