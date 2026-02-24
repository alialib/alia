add_library(alia_core STATIC
    src/alia/base/arena.cpp
    src/alia/base/color.cpp
    src/alia/base/stack.cpp
    src/alia/kernel/substrate.cpp
    src/alia/kernel/flow/dispatch.cpp
    src/alia/kernel/flow/traversal.cpp
    src/alia/ui/animation/unit_cubic_bezier.cpp
    src/alia/ui/drawing.cpp
    src/alia/ui/input/pointer.cpp
    src/alia/ui/input/regions.cpp
    src/alia/ui/input/elements.cpp
    src/alia/ui/input/keyboard.cpp
    src/alia/ui/layout/components/column.cpp
    src/alia/ui/layout/components/flow.cpp
    src/alia/ui/layout/components/grid.cpp
    src/alia/ui/layout/components/hyperflow.cpp
    src/alia/ui/layout/components/row.cpp
    src/alia/ui/layout/components/alignment_override.cpp
    src/alia/ui/layout/components/growth_override.cpp
    src/alia/ui/layout/components/min_size.cpp
    src/alia/ui/layout/components/inset.cpp
    src/alia/ui/layout/components/placement_hook.cpp
    src/alia/ui/layout/components/leaf.cpp
    src/alia/ui/layout/system.cpp
    src/alia/ui/layout/utilities.cpp
    src/alia/ui/system/api.cpp
    src/alia/ui/system/input_processing.cpp
    src/alia/ui/system/object.cpp
    src/alia/ui/system/interface.cpp
    src/alia/ui/system.cpp
    src/alia/ui/theme.cpp)
target_include_directories(alia_core PUBLIC
    include)
target_include_directories(alia_core PUBLIC # TODO: PRIVATE
    src)
