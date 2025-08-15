add_library(alia_ui STATIC
    ${PROJECT_SOURCE_DIR}/core/ui/src/drawing.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/display_list.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/system.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/layout/compositors/column.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/layout/compositors/flow.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/layout/compositors/hyperflow.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/layout/compositors/row.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/layout/modifiers/alignment_override.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/layout/modifiers/growth_override.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/layout/modifiers/min_size.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/layout/modifiers/inset.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/layout/modifiers/placement_hook.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/layout/container.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/layout/leaf.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/layout/system.cpp
    ${PROJECT_SOURCE_DIR}/core/ui/src/layout/utilities.cpp)
target_include_directories(alia_ui PUBLIC
    ${PROJECT_SOURCE_DIR}/core/ui/include)
target_link_libraries(alia_ui PUBLIC alia_flow)
