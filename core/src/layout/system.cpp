#include <alia/layout/system.hpp>

namespace alia {

void
initialize(layout_system& system)
{
    initialize_lazy_commit_arena(&system.node_arena);
    initialize_lazy_commit_arena(&system.placement_arena);
    initialize_lazy_commit_arena(&system.scratch_arena);
    system.root = layout_container{
        .base = {.vtable = nullptr, .next_sibling = nullptr},
        .flags = NO_FLAGS,
        .first_child = 0};
}

void
resolve_layout(layout_system& system, vec2f available_space)
{
    alia_arena_reset(alia_arena_get_view(&system.scratch_arena));
    layout_node* root_node = system.root.first_child;
    vertical_requirements vertical;
    {
        measurement_context ctx{alia_arena_get_view(&system.scratch_arena)};
        measure_horizontal(&ctx, root_node);
        alia_arena_reset(alia_arena_get_view(&system.scratch_arena));
        vertical = measure_vertical(
            &ctx, ALIA_MAIN_AXIS_X, root_node, available_space.x);
    }
    {
        placement_context ctx{
            alia_arena_get_view(&system.scratch_arena),
            alia_arena_get_view(&system.placement_arena)};
        alia_arena_reset(alia_arena_get_view(&system.scratch_arena));
        assign_boxes(
            &ctx,
            ALIA_MAIN_AXIS_X,
            root_node,
            {vec2f{0, 0}, available_space},
            vertical.ascent);
        alia_arena_reset(alia_arena_get_view(&system.scratch_arena));
    }
}

} // namespace alia
