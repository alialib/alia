#include <alia/abi/ui/layout/placement.h>

#include <alia/abi/base/arena.h>
#include <alia/abi/ui/layout/utilities.h>
#include <alia/impl/base/arena.hpp>
#include <alia/impl/base/stack.hpp>
#include <alia/ui/layout/system.h>

#include <alia/abi/context.h>

using namespace alia;

namespace {

struct placement_consume_scope
{
    alia_offset old_offset;
};

} // namespace

extern "C" {

void
alia_layout_placement_clear(alia_layout_system* system)
{
    ALIA_ASSERT(system);
    ++system->placement.generation;
    system->placement.high_water = 0;
}

alia_layout_placement_generation
alia_layout_placement_current_generation(alia_layout_system const* system)
{
    ALIA_ASSERT(system);
    return system->placement.generation;
}

alia_layout_placement_region
alia_layout_placement_resolve(
    alia_layout_system* system,
    alia_layout_node* root,
    alia_vec2f available_space)
{
    ALIA_ASSERT(system);
    ALIA_ASSERT(root);

    alia_layout_placement_region const region
        = {.offset = (alia_offset) system->placement.high_water};

    alia_vertical_requirements vertical;
    {
        alia_measurement_context ctx{};
        alia_bump_allocator_init(&ctx.scratch, &system->scratch_arena);
        alia_measure_horizontal(&ctx, ALIA_MAIN_AXIS_X, root);
        alia_arena_reset(&ctx.scratch);
        vertical = alia_measure_vertical(
            &ctx, ALIA_MAIN_AXIS_X, root, available_space.x);
    }
    {
        alia_placement_context ctx;
        alia_bump_allocator_init(&ctx.scratch, &system->scratch_arena);
        alia_bump_allocator_init(&ctx.arena, &system->placement.arena);
        ctx.arena.offset = system->placement.high_water;
        alia_assign_boxes(
            &ctx,
            ALIA_MAIN_AXIS_X,
            root,
            {.min = {0, 0}, .size = available_space},
            vertical.ascent);
        system->placement.high_water = ctx.arena.offset;
        alia_bump_allocator_commit_peak(&ctx.arena);
    }

    return region;
}

void
alia_layout_placement_push(
    alia_context* ctx, alia_layout_placement_region region)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(ctx->layout);
    auto& scope = stack_push<placement_consume_scope>(ctx);
    scope.old_offset = ctx->layout->placement.offset;
    ctx->layout->placement.offset = region.offset;
}

void
alia_layout_placement_pop(alia_context* ctx)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(ctx->layout);
    auto& scope = stack_pop<placement_consume_scope>(ctx);
    ctx->layout->placement.offset = scope.old_offset;
}

} // extern "C"
