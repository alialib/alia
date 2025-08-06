#include <alia/ui/layout/system.hpp>

namespace alia {

void
initialize(LayoutSystem& system)
{
    system.node_arena.initialize();
    system.placement_arena.initialize();
    system.scratch_arena.initialize();
    system.root = LayoutContainer{
        .base = {.vtable = nullptr, .next_sibling = nullptr},
        .flags = NO_FLAGS,
        .first_child = 0};
}

void
resolve_layout(LayoutSystem& system, Vec2 available_space)
{
    system.scratch_arena.reset();
    LayoutNode* root_node = system.root.first_child;
    VerticalRequirements vertical;
    {
        MeasurementContext ctx{&system.scratch_arena};
        measure_horizontal(&ctx, root_node);
        system.scratch_arena.reset();
        vertical = measure_vertical(&ctx, root_node, available_space.x);
    }
    {
        PlacementContext ctx{&system.scratch_arena, &system.placement_arena};
        system.scratch_arena.reset();
        assign_boxes(
            &ctx,
            root_node,
            Box{Vec2{0, 0}, available_space},
            vertical.ascent);
        system.scratch_arena.reset();
    }
}

} // namespace alia
