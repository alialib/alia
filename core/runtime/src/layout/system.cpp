#include <alia/layout/system.hpp>

namespace alia {

void
initialize(layout_system& system)
{
    alia_scratch_allocator allocator
        = make_lazy_commit_arena_allocator(&system.allocator);
    alia_scratch_construct(&system.node_arena, allocator);
    alia_scratch_construct(&system.placement_arena, allocator);
    alia_scratch_construct(&system.scratch_arena, allocator);
    system.root = layout_container{
        .base = {.vtable = nullptr, .next_sibling = nullptr},
        .flags = NO_FLAGS,
        .first_child = 0};
}

void
resolve_layout(layout_system& system, vec2 available_space)
{
    alia_scratch_reset(&system.scratch_arena);
    layout_node* root_node = system.root.first_child;
    vertical_requirements vertical;
    {
        measurement_context ctx{&system.scratch_arena};
        measure_horizontal(&ctx, root_node);
        alia_scratch_reset(&system.scratch_arena);
        vertical = measure_vertical(
            &ctx, ALIA_MAIN_AXIS_X, root_node, available_space.x);
    }
    {
        placement_context ctx{&system.scratch_arena, &system.placement_arena};
        alia_scratch_reset(&system.scratch_arena);
        assign_boxes(
            &ctx,
            ALIA_MAIN_AXIS_X,
            root_node,
            {vec2{0, 0}, available_space},
            vertical.ascent);
        alia_scratch_reset(&system.scratch_arena);
    }
}

} // namespace alia
