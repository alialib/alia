add_library(alia_ui STATIC
    src/color.cpp
    src/drawing.cpp
    src/display_list.cpp
    src/system.cpp
    src/layout/compositors/column.cpp
    src/layout/compositors/flow.cpp
    src/layout/compositors/grid.cpp
    src/layout/compositors/hyperflow.cpp
    src/layout/compositors/row.cpp
    src/layout/modifiers/alignment_override.cpp
    src/layout/modifiers/growth_override.cpp
    src/layout/modifiers/min_size.cpp
    src/layout/modifiers/inset.cpp
    src/layout/modifiers/placement_hook.cpp
    src/layout/container.cpp
    src/layout/leaf.cpp
    src/layout/system.cpp
    src/layout/utilities.cpp
    src/theme.cpp)
target_include_directories(alia_ui PUBLIC
    ${PROJECT_SOURCE_DIR}/core/ui/abi/include
    include)
target_link_libraries(alia_ui PUBLIC alia_kernel)
