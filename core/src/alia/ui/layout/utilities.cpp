#include <alia/layout/utilities.hpp>

#include <alia/base/arena.h>

extern "C" {

alia_layout_axis_placement
alia_resolve_container_x(
    alia_layout_flags_t flags, float assigned_size, float required_size)
{
    static float const offsets[8] = {0, 0.5, 0, 1, 0, 0, 0, 0};
    static float const sizes[8] = {1, 0, 0, 0, 1, 1, 1, 1};
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
    float padding)
{
    static float const offsets[8] = {0, 0.5, 0, 1, 0, 0, 0, 0};
    static float const sizes[8] = {0, 0, 0, 0, 1, 1, 1, 1};
    auto const index
        = (flags & ALIA_X_ALIGNMENT_MASK) >> ALIA_X_ALIGNMENT_BIT_OFFSET;
    float const extra_space = assigned_size - (required_size + padding * 2);
    return alia_layout_axis_placement{
        .offset = padding + offsets[index] * extra_space,
        .size = required_size + sizes[index] * extra_space};
}

alia_layout_axis_placement
alia_resolve_leaf_y(
    alia_layout_flags_t flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent,
    float padding)
{
    static float const offsets[8] = {0, 0.5, 0, 1, 0, 0, 0, 0};
    static float const baseline_offsets[8] = {0, 0, 0, 0, 0, 1, 0, 0};
    static float const sizes[8] = {0, 0, 0, 0, 1, 0, 1, 1};
    auto const index
        = (flags & ALIA_Y_ALIGNMENT_MASK) >> ALIA_Y_ALIGNMENT_BIT_OFFSET;
    float const extra_space = assigned_size - (required_size + padding * 2);
    return alia_layout_axis_placement{
        .offset = padding + offsets[index] * extra_space
                + baseline_offsets[index] * (baseline - (ascent + padding)),
        .size = required_size + sizes[index] * extra_space};
}

alia_box
alia_resolve_leaf_box(
    alia_layout_flags_t flags,
    alia_vec2f assigned_size,
    float baseline,
    alia_vec2f required_size,
    float ascent,
    alia_vec2f padding)
{
    alia_layout_axis_placement x_placement = alia_resolve_leaf_x(
        flags, assigned_size.x, required_size.x, padding.x);
    alia_layout_axis_placement y_placement = alia_resolve_leaf_y(
        flags, assigned_size.y, baseline, required_size.y, ascent, padding.y);
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
    auto const index = (flags & ALIA_BASELINE_GROUP_ALIGNMENT_MASK)
                    >> ALIA_BASELINE_GROUP_ALIGNMENT_BIT_OFFSET;
    return ascent + offsets[index] * (assigned_height - ascent - descent);
}

alia_wrapping_requirements
alia_default_measure_wrapped_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float current_x_offset,
    float line_width)
{
    auto const marker = alia_arena_mark(ctx->scratch);
    auto horizontal = alia_measure_horizontal(ctx, node);
    alia_arena_jump(ctx->scratch, marker);
    auto vertical
        = alia_measure_vertical(ctx, main_axis, node, horizontal.min_size);
    if (current_x_offset + horizontal.min_size > line_width)
    {
        return alia_wrapping_requirements{
            .first_line = {.height = 0, .ascent = 0, .descent = 0},
            .interior_height = 0,
            .last_line
            = {.height = vertical.min_size,
               .ascent = vertical.ascent,
               .descent = vertical.descent},
            .end_x = horizontal.min_size};
    }
    else
    {
        return alia_wrapping_requirements{
            .first_line
            = {.height = vertical.min_size,
               .ascent = vertical.ascent,
               .descent = vertical.descent},
            .interior_height = 0,
            .last_line = {.height = 0, .ascent = 0, .descent = 0},
            .end_x = current_x_offset + horizontal.min_size};
    };
}

} // extern "C"
