#include <alia/abi/ui/layout/detached.h>

#include <alia/abi/base/memory.h>
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
    alia_memory_system* memory = system->memory;
    alia_measurement_context ctx{};
    ctx.measure_preferred_size = true;
    ctx.scratch = alia_memory_open_bump(memory, system->scratch_pool);
    alia_horizontal_requirements const horizontal
        = alia_measure_horizontal(&ctx, ALIA_MAIN_AXIS_X, root);

    float const width = horizontal.min_size;

    ctx.scratch = alia_memory_open_bump(memory, system->scratch_pool);
    alia_vertical_requirements const vertical
        = alia_measure_vertical(&ctx, ALIA_MAIN_AXIS_X, root, width);
    alia_memory_commit_bump(memory, system->scratch_pool, &ctx.scratch);
    alia_memory_trim(memory, system->scratch_pool, ALIA_MEMORY_RETAIN_PEAK);

    return {.x = width, .y = vertical.min_size};
}

} // namespace

extern "C" {

alia_vec2f
alia_layout_detached_measure(
    alia_ui_context* ctx, alia_layout_detached* detached)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(detached);
    ALIA_ASSERT(alia_ctx_layout(ctx));
    ALIA_ASSERT(alia_ctx_layout(ctx)->system);

    if (!detached->root)
        return {0.f, 0.f};

    return measure_detached_size(alia_ctx_layout(ctx)->system, detached->root);
}

void
alia_layout_detached_assign(
    alia_ui_context* ctx, alia_layout_detached* detached, alia_vec2f size)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(detached);
    ALIA_ASSERT(alia_ctx_layout(ctx));
    ALIA_ASSERT(alia_ctx_layout(ctx)->system);

    alia_layout_system* system = alia_ctx_layout(ctx)->system;
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
alia_layout_detached_begin(
    alia_ui_context* ctx, alia_layout_detached* detached)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(detached);
    ALIA_ASSERT(alia_ctx_layout(ctx));

    if (is_refresh_event(*ctx))
    {
        auto& scope
            = stack_push<detached_emission_scope>(alia_ctx_kernel(ctx));
        scope.old_next_ptr = alia_ctx_layout(ctx)->emission.next_ptr;
        detached->root = nullptr;
        alia_ctx_layout(ctx)->emission.next_ptr = &detached->root;
    }
    else
    {
        ALIA_ASSERT(alia_ctx_layout(ctx)->system);
        ALIA_ASSERT(
            detached->generation
            == alia_ctx_layout(ctx)->system->placement.generation);
        auto& scope
            = stack_push<detached_placement_scope>(alia_ctx_kernel(ctx));
        scope.old_placement_offset = alia_ctx_layout(ctx)->placement.offset;
        alia_ctx_layout(ctx)->placement.offset = detached->region.offset;
    }
}

void
alia_layout_detached_end(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(alia_ctx_layout(ctx));

    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_pop<detached_emission_scope>(alia_ctx_kernel(ctx));
        *alia_ctx_layout(ctx)->emission.next_ptr = nullptr;
        alia_ctx_layout(ctx)->emission.next_ptr = scope.old_next_ptr;
    }
    else
    {
        auto& scope
            = stack_pop<detached_placement_scope>(alia_ctx_kernel(ctx));
        alia_ctx_layout(ctx)->placement.offset = scope.old_placement_offset;
    }
}

} // extern "C"
