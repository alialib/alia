#include <alia/ui/layout/system.h>

#include <alia/abi/base/arena.h>
#include <alia/abi/ui/layout/utilities.h>
#include <alia/abi/ui/style.h>
#include <alia/impl/base/arena.hpp>

using namespace alia;

extern "C" {

void
alia_layout_system_init(alia_layout_system* system)
{
    initialize_lazy_commit_arena(&system->node_arena);
    initialize_lazy_commit_arena(&system->placement_arena);
    initialize_lazy_commit_arena(&system->scratch_arena);
    system->root = alia_layout_container{
        .base = {.vtable = nullptr, .next_sibling = nullptr},
        .flags = 0,
        .first_child = 0};
}

void
alia_layout_system_resolve(
    alia_layout_system* system, alia_vec2f available_space)
{
    alia_layout_node* root_node = system->root.first_child;
    alia_vertical_requirements vertical;
    {
        alia_measurement_context ctx;
        alia_bump_allocator_init(&ctx.scratch, &system->scratch_arena);
        alia_measure_horizontal(&ctx, root_node);
        alia_arena_reset(&ctx.scratch);
        vertical = alia_measure_vertical(
            &ctx, ALIA_MAIN_AXIS_X, root_node, available_space.x);
    }
    {
        alia_placement_context ctx;
        alia_bump_allocator_init(&ctx.scratch, &system->scratch_arena);
        alia_bump_allocator_init(&ctx.arena, &system->placement_arena);
        alia_assign_boxes(
            &ctx,
            ALIA_MAIN_AXIS_X,
            root_node,
            {.min = {0, 0}, .size = available_space},
            vertical.ascent);
    }
}

} // extern "C"
