#include <alia/layout/utilities.hpp>

#include <alia/internals/arena.hpp>

namespace alia {

wrapping_requirements
default_measure_wrapped_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float current_x_offset,
    float line_width)
{
    auto const marker = alia_arena_mark(ctx->scratch);
    auto horizontal = measure_horizontal(ctx, node);
    alia_arena_jump(ctx->scratch, marker);
    auto vertical
        = measure_vertical(ctx, main_axis, node, horizontal.min_size);
    if (current_x_offset + horizontal.min_size > line_width)
    {
        return wrapping_requirements{
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
        return wrapping_requirements{
            .first_line
            = {.height = vertical.min_size,
               .ascent = vertical.ascent,
               .descent = vertical.descent},
            .interior_height = 0,
            .last_line = {.height = 0, .ascent = 0, .descent = 0},
            .end_x = current_x_offset + horizontal.min_size};
    };
}

layout_axis_placement
resolve_horizontal_assignment(
    layout_flag_set flags, float assigned_size, float required_size)
{
    static float const offsets[8] = {0, 0.5, 0, 1, 0, 0, 0, 0};
    static float const sizes[8] = {1, 0, 0, 0, 1, 1, 1, 1};

    auto const index = raw_code(flags & X_ALIGNMENT_MASK);
    float const extra_space = assigned_size - required_size;
    return layout_axis_placement{
        .offset = offsets[index] * extra_space,
        .size = required_size + sizes[index] * extra_space};
}

layout_axis_placement
resolve_vertical_assignment(
    layout_flag_set flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent)
{
    static float const offsets[8] = {0, 0.5, 0, 1, 0, 0, 0, 0};
    static float const baseline_offsets[8] = {0, 0, 0, 0, 0, 1, 0, 0};
    static float const sizes[8] = {1, 0, 0, 0, 1, 0, 1, 1};

    auto const index
        = raw_code(flags & Y_ALIGNMENT_MASK) >> Y_ALIGNMENT_BIT_OFFSET;
    float const extra_space = assigned_size - required_size;
    return layout_axis_placement{
        .offset = offsets[index] * extra_space
                + baseline_offsets[index] * (baseline - ascent),
        .size = required_size + sizes[index] * extra_space};
}

box
resolve_assignment(
    layout_flag_set flags,
    vec2 assigned_size,
    float baseline,
    vec2 required_size,
    float ascent)
{
    layout_axis_placement x_placement = resolve_horizontal_assignment(
        flags, assigned_size.x, required_size.x);
    layout_axis_placement y_placement = resolve_vertical_assignment(
        flags, assigned_size.y, baseline, required_size.y, ascent);
    return {
        vec2{x_placement.offset, y_placement.offset},
        vec2{x_placement.size, y_placement.size}};
}

layout_axis_placement
resolve_padded_horizontal_assignment(
    layout_flag_set flags,
    float assigned_size,
    float required_size,
    float padding)
{
    auto placement = resolve_horizontal_assignment(
        flags, assigned_size, required_size + padding * 2);
    return layout_axis_placement{
        .offset = placement.offset + padding,
        .size = placement.size - padding * 2};
}

layout_axis_placement
resolve_padded_vertical_assignment(
    layout_flag_set flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent,
    float padding)
{
    auto placement = resolve_vertical_assignment(
        flags,
        assigned_size,
        baseline,
        required_size + padding * 2,
        ascent + padding);
    return layout_axis_placement{
        .offset = placement.offset + padding,
        .size = placement.size - padding * 2};
}

box
resolve_padded_assignment(
    layout_flag_set flags,
    vec2 assigned_size,
    float baseline,
    vec2 required_size,
    float ascent,
    float padding)
{
    layout_axis_placement x_placement = resolve_padded_horizontal_assignment(
        flags, assigned_size.x, required_size.x, padding);
    layout_axis_placement y_placement = resolve_padded_vertical_assignment(
        flags, assigned_size.y, baseline, required_size.y, ascent, padding);
    return {
        vec2{x_placement.offset, y_placement.offset},
        vec2{x_placement.size, y_placement.size}};
}

float
assign_baseline(
    layout_flag_set flags, float assigned_height, float ascent, float descent)
{
    static float const offsets[4] = {0.5, 0, 1, 0.5};
    auto const index = raw_code(flags & BASELINE_GROUP_ALIGNMENT_MASK)
                    >> BASELINE_GROUP_ALIGNMENT_BIT_OFFSET;
    return ascent + offsets[index] * (assigned_height - ascent - descent);
}
} // namespace alia
