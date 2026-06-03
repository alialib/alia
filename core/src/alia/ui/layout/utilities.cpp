#include <alia/abi/ui/layout/utilities.h>

#include <alia/impl/base/arena.hpp>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>

using namespace alia;

extern "C" {

void
alia_layout_container_activate(
    alia_context* ctx, alia_layout_container* container)
{
    auto& emission = ctx->layout->emission;
    *emission.next_ptr = &container->base;
    emission.next_ptr = &container->first_child;
}

void
alia_layout_container_deactivate(
    alia_context* ctx, alia_layout_container* container)
{
    auto& emission = ctx->layout->emission;
    *emission.next_ptr = 0;
    emission.next_ptr = &container->base.next_sibling;
}

struct alia_layout_container_scope
{
    alia_layout_container* container;
};

void
alia_layout_container_simple_begin(
    alia_context* ctx,
    alia_layout_node_vtable* vtable,
    alia_layout_flags_t flags)
{
    if (alia::is_refresh_event(*ctx))
    {
        auto& scope = stack_push<alia_layout_container_scope>(ctx);
        auto& emission = ctx->layout->emission;
        auto* container = arena_alloc<alia_layout_container>(emission.arena);
        scope.container = container;
        *container = alia_layout_container{
            .base = {.vtable = vtable, .next_sibling = 0},
            .flags = flags,
            .first_child = 0};
        alia_layout_container_activate(ctx, container);
    }
}

void
alia_layout_container_simple_end(alia_context* ctx)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_pop<alia_layout_container_scope>(ctx);
        alia_layout_container_deactivate(ctx, scope.container);
    }
}

alia_layout_axis_placement
alia_resolve_container_x(
    alia_layout_flags_t flags, float assigned_size, float required_size)
{
    static float const offsets[8] = {0, 0.5, 0, 1, 0, 0, 0, 0};
    static float const sizes[8] = {1, 0, 0, 0, 1, 1, 1, 1};
    static_assert(ALIA_X_ALIGNMENT_MASK >> ALIA_X_ALIGNMENT_BIT_OFFSET == 7);
    auto const index
        = (flags & ALIA_X_ALIGNMENT_MASK) >> ALIA_X_ALIGNMENT_BIT_OFFSET;
    float const extra_space = assigned_size - required_size;
    return alia_layout_axis_placement{
        .offset = offsets[index] * extra_space,
        .size = required_size + sizes[index] * extra_space};
}

alia_layout_axis_placement
alia_resolve_container_y(
    alia_layout_flags_t flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent)
{
    static float const offsets[8] = {0, 0.5, 0, 1, 0, 0, 0, 0};
    static float const baseline_offsets[8] = {0, 0, 0, 0, 0, 1, 0, 0};
    static float const sizes[8] = {1, 0, 0, 0, 1, 0, 1, 1};
    static_assert(ALIA_Y_ALIGNMENT_MASK >> ALIA_Y_ALIGNMENT_BIT_OFFSET == 7);
    auto const index
        = (flags & ALIA_Y_ALIGNMENT_MASK) >> ALIA_Y_ALIGNMENT_BIT_OFFSET;
    float const extra_space = assigned_size - required_size;
    return alia_layout_axis_placement{
        .offset = offsets[index] * extra_space
                + baseline_offsets[index] * (baseline - ascent),
        .size = required_size + sizes[index] * extra_space};
}

alia_box
alia_resolve_container_box(
    alia_layout_flags_t flags,
    alia_vec2f assigned_size,
    float baseline,
    alia_vec2f required_size,
    float ascent)
{
    alia_layout_axis_placement x_placement
        = alia_resolve_container_x(flags, assigned_size.x, required_size.x);
    alia_layout_axis_placement y_placement = alia_resolve_container_y(
        flags, assigned_size.y, baseline, required_size.y, ascent);
    return {
        {x_placement.offset, y_placement.offset},
        {x_placement.size, y_placement.size}};
}

alia_layout_axis_placement
alia_resolve_leaf_x(
    alia_layout_flags_t flags,
    float assigned_size,
    float required_size,
    float spacing)
{
    static float const offsets[8] = {0, 0.5, 0, 1, 0, 0, 0, 0};
    static float const sizes[8] = {0, 0, 0, 0, 1, 1, 1, 1};
    static_assert(ALIA_X_ALIGNMENT_MASK >> ALIA_X_ALIGNMENT_BIT_OFFSET == 7);
    auto const index
        = (flags & ALIA_X_ALIGNMENT_MASK) >> ALIA_X_ALIGNMENT_BIT_OFFSET;
    float const extra_space = assigned_size - (required_size + spacing * 2);
    return alia_layout_axis_placement{
        .offset = spacing + offsets[index] * extra_space,
        .size = required_size + sizes[index] * extra_space};
}

alia_layout_axis_placement
alia_resolve_leaf_y(
    alia_layout_flags_t flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent,
    float spacing)
{
    static float const offsets[8] = {0, 0.5, 0, 1, 0, 0, 0, 0};
    static float const baseline_offsets[8] = {0, 0, 0, 0, 0, 1, 0, 0};
    static float const sizes[8] = {0, 0, 0, 0, 1, 0, 1, 1};
    static_assert(ALIA_Y_ALIGNMENT_MASK >> ALIA_Y_ALIGNMENT_BIT_OFFSET == 7);
    auto const index
        = (flags & ALIA_Y_ALIGNMENT_MASK) >> ALIA_Y_ALIGNMENT_BIT_OFFSET;
    float const extra_space = assigned_size - (required_size + spacing * 2);
    return alia_layout_axis_placement{
        .offset = spacing + offsets[index] * extra_space
                + baseline_offsets[index] * (baseline - (ascent + spacing)),
        .size = required_size + sizes[index] * extra_space};
}

alia_box
alia_resolve_leaf_box(
    alia_layout_flags_t flags,
    alia_vec2f assigned_size,
    float baseline,
    alia_vec2f required_size,
    float ascent,
    alia_vec2f spacing)
{
    alia_layout_axis_placement x_placement = alia_resolve_leaf_x(
        flags, assigned_size.x, required_size.x, spacing.x);
    alia_layout_axis_placement y_placement = alia_resolve_leaf_y(
        flags, assigned_size.y, baseline, required_size.y, ascent, spacing.y);
    return {
        {x_placement.offset, y_placement.offset},
        {x_placement.size, y_placement.size}};
}

float
alia_resolve_baseline(
    alia_layout_flags_t flags,
    float assigned_height,
    float ascent,
    float descent)
{
    static float const offsets[4] = {0.5, 0, 1, 0.5};
    static_assert(
        ALIA_BASELINE_GROUP_ALIGNMENT_MASK
            >> ALIA_BASELINE_GROUP_ALIGNMENT_BIT_OFFSET
        == 3);
    auto const index = (flags & ALIA_BASELINE_GROUP_ALIGNMENT_MASK)
                    >> ALIA_BASELINE_GROUP_ALIGNMENT_BIT_OFFSET;
    return ascent + offsets[index] * (assigned_height - ascent - descent);
}

int
alia_default_count_flow_fragments(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    return 1;
}

void
alia_default_emit_flow_fragments(
    alia_measurement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_emitter* emitter)
{
    auto const marker = alia_arena_mark(&ctx->scratch);
    auto horizontal = alia_measure_horizontal(ctx, node);
    alia_arena_jump(&ctx->scratch, marker);
    auto vertical = alia_measure_vertical(
        ctx, ALIA_MAIN_AXIS_X, node, horizontal.min_size);
    alia_layout_emit_flow_fragment(
        emitter,
        alia_flow_fragment{
            .width = horizontal.min_size,
            .height = vertical.min_size,
            .ascent = vertical.ascent,
            .descent = vertical.descent});
}

void
alia_default_read_fragment_placements(
    alia_placement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_reader* reader)
{
    auto const* fragment = alia_layout_read_fragment_spec(reader);
    auto const* placement = alia_layout_read_fragment_placement(reader);
    alia_layout_advance_fragment(reader);
    alia_assign_boxes(
        ctx,
        ALIA_MAIN_AXIS_X,
        node,
        alia_box{placement->position, {fragment->width, fragment->height}},
        placement->baseline);
}

alia_layout_line_spacing
alia_layout_justify_line(
    alia_layout_flags_t flags, float extra_space, int count)
{
    if (extra_space <= 0.f || count <= 0)
        return {0.f, 0.f};

    switch (flags & ALIA_JUSTIFY_MASK)
    {
        case ALIA_JUSTIFY_START:
        default:
            return {0.f, 0.f};
        case ALIA_JUSTIFY_END:
            return {extra_space, 0.f};
        case ALIA_JUSTIFY_CENTER:
            return {extra_space * 0.5f, 0.f};
        case ALIA_JUSTIFY_SPACE_BETWEEN:
            if (count >= 2)
            {
                return {0.f, extra_space / static_cast<float>(count - 1)};
            }
            else
            {
                return {0.f, 0.f};
            }
        case ALIA_JUSTIFY_SPACE_AROUND: {
            float const gap = extra_space / static_cast<float>(count);
            return {gap * 0.5f, gap};
        }
        case ALIA_JUSTIFY_SPACE_EVENLY: {
            float const gap = extra_space / static_cast<float>(count + 1);
            return {gap, gap};
        }
    }
}

} // extern "C"
