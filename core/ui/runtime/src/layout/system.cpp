#include <alia/ui/layout/system.hpp>

namespace alia {

void
initialize(layout_system& system)
{
    system.node_arena.initialize();
    system.placement_arena.initialize();
    system.scratch_arena.initialize();
    system.root = layout_container{
        .base = {.vtable = nullptr, .next_sibling = nullptr},
        .flags = NO_FLAGS,
        .first_child = 0};
}

void
resolve_layout(layout_system& system, vec2 available_space)
{
    system.scratch_arena.reset();
    layout_node* root_node = system.root.first_child;
    vertical_requirements vertical;
    {
        measurement_context ctx{&system.scratch_arena};
        measure_horizontal(&ctx, root_node);
        system.scratch_arena.reset();
        vertical = measure_vertical(
            &ctx, MAIN_AXIS_X, root_node, available_space.x);
    }
    {
        placement_context ctx{&system.scratch_arena, &system.placement_arena};
        system.scratch_arena.reset();
        assign_boxes(
            &ctx,
            MAIN_AXIS_X,
            root_node,
            {vec2{0, 0}, available_space},
            vertical.ascent);
        system.scratch_arena.reset();
    }
}

} // namespace alia
