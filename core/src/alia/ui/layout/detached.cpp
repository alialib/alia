#include <alia/abi/ui/layout/detached.h>

#include <alia/abi/ui/layout/utilities.h>
#include <alia/impl/base/arena.hpp>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>
#include <alia/ui/layout/system.h>

using namespace alia;

namespace {

struct detached_emission_scope
{
    alia_layout_node** old_next_ptr;
};

struct detached_placement_scope
{
    alia_offset old_placement_offset;
};

alia_vec2f
measure_detached_size(alia_layout_system* system, alia_layout_node* root)
{
    alia_measurement_context ctx{};
    ctx.measure_preferred_size = true;
    alia_bump_allocator_init(&ctx.scratch, &system->scratch_arena);
    alia_horizontal_requirements const horizontal
        = alia_measure_horizontal(&ctx, ALIA_MAIN_AXIS_X, root);

    float const width = horizontal.min_size;

    alia_arena_reset(&ctx.scratch);
    alia_vertical_requirements const vertical
        = alia_measure_vertical(&ctx, ALIA_MAIN_AXIS_X, root, width);

    return {.x = width, .y = vertical.min_size};
}

} // namespace

extern "C" {

alia_vec2f
alia_layout_detached_measure(alia_context* ctx, alia_layout_detached* detached)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(detached);
    ALIA_ASSERT(ctx->layout);
    ALIA_ASSERT(ctx->layout->system);

    if (!detached->root)
        return {0.f, 0.f};

    return measure_detached_size(ctx->layout->system, detached->root);
}

void
alia_layout_detached_assign(
    alia_context* ctx, alia_layout_detached* detached, alia_vec2f size)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(detached);
    ALIA_ASSERT(ctx->layout);
    ALIA_ASSERT(ctx->layout->system);

    alia_layout_system* system = ctx->layout->system;
    if (detached->generation == system->placement.generation)
        return;

    if (!detached->root)
    {
        detached->region
            = {.offset = (alia_offset) system->placement.high_water};
        detached->size = {0.f, 0.f};
        detached->generation = system->placement.generation;
        return;
    }

    detached->region
        = alia_layout_placement_resolve(system, detached->root, size);
    detached->size = size;
    detached->generation = system->placement.generation;
}

void
alia_layout_detached_begin(alia_context* ctx, alia_layout_detached* detached)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(detached);
    ALIA_ASSERT(ctx->layout);

    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_push<detached_emission_scope>(ctx);
        scope.old_next_ptr = ctx->layout->emission.next_ptr;
        detached->root = nullptr;
        ctx->layout->emission.next_ptr = &detached->root;
    }
    else
    {
        ALIA_ASSERT(ctx->layout->system);
        ALIA_ASSERT(
            detached->generation == ctx->layout->system->placement.generation);
        auto& scope = stack_push<detached_placement_scope>(ctx);
        scope.old_placement_offset = ctx->layout->placement.offset;
        ctx->layout->placement.offset = detached->region.offset;
    }
}

void
alia_layout_detached_end(alia_context* ctx)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(ctx->layout);

    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_pop<detached_emission_scope>(ctx);
        *ctx->layout->emission.next_ptr = nullptr;
        ctx->layout->emission.next_ptr = scope.old_next_ptr;
    }
    else
    {
        auto& scope = stack_pop<detached_placement_scope>(ctx);
        ctx->layout->placement.offset = scope.old_placement_offset;
    }
}

} // extern "C"
