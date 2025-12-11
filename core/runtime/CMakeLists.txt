add_library(alia_core STATIC
    src/arenas.cpp
    src/scratch.cpp
    src/substrate.cpp
    src/animation/cubic_bezier.cpp
    src/color.cpp
    src/drawing.cpp
    src/display_list.cpp
    src/flow/dispatch.cpp
    src/flow/traversal.cpp
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
    src/system/input_processing.cpp
    # src/system/object.cpp
    src/system.cpp
    src/theme.cpp)
target_include_directories(alia_core PUBLIC
    ${PROJECT_SOURCE_DIR}/core/abi/include
    include)
