#include <alia/ui/layout/utilities.hpp>

namespace alia {

WrappingRequirements
default_measure_wrapped_vertical(
    MeasurementContext* ctx,
    LayoutNode* node,
    float current_x_offset,
    float line_width)
{
    auto checkpoint = ctx->scratch->save_state();
    auto horizontal = measure_horizontal(ctx, node);
    ctx->scratch->restore_state(checkpoint);
    auto vertical = measure_vertical(ctx, node, horizontal.min_size);
    if (current_x_offset + horizontal.min_size > line_width)
    {
        return WrappingRequirements{
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
        return WrappingRequirements{
            .first_line
            = {.height = vertical.min_size,
               .ascent = vertical.ascent,
               .descent = vertical.descent},
            .interior_height = 0,
            .last_line = {.height = 0, .ascent = 0, .descent = 0},
            .end_x = current_x_offset + horizontal.min_size};
    };
}

LayoutAxisPlacement
resolve_horizontal_assignment(
    LayoutFlagSet flags, float assigned_size, float required_size)
{
    static float const offsets[8] = {0, 0.5, 0, 1, 0, 0, 0, 0};
    static float const sizes[8] = {1, 0, 0, 0, 1, 1, 1, 1};

    auto const index = raw_code(flags & X_ALIGNMENT_MASK);
    float const extra_space = assigned_size - required_size;
    return LayoutAxisPlacement{
        .offset = offsets[index] * extra_space,
        .size = required_size + sizes[index] * extra_space};
}

LayoutAxisPlacement
resolve_vertical_assignment(
    LayoutFlagSet flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent)
{
    static float const offsets[8] = {0, 0.5, 0, 1, 0, 0, 0, 0};
    static float const baseline_offsets[8] = {0, 0, 0, 0, 1, 0, 0, 0};
    static float const sizes[8] = {1, 0, 0, 0, 1, 0, 1, 1};

    auto const index
        = raw_code(flags & Y_ALIGNMENT_MASK) >> Y_ALIGNMENT_BIT_OFFSET;
    float const extra_space = assigned_size - required_size;
    return LayoutAxisPlacement{
        .offset = offsets[index] * extra_space
                + baseline_offsets[index] * (baseline - ascent),
        .size = required_size + sizes[index] * extra_space};
}

Box
resolve_assignment(
    LayoutFlagSet flags,
    Vec2 assigned_size,
    float baseline,
    Vec2 required_size,
    float ascent)
{
    LayoutAxisPlacement x_placement = resolve_horizontal_assignment(
        flags, assigned_size.x, required_size.x);
    LayoutAxisPlacement y_placement = resolve_vertical_assignment(
        flags, assigned_size.y, baseline, required_size.y, ascent);
    return Box{
        Vec2{x_placement.offset, y_placement.offset},
        Vec2{x_placement.size, y_placement.size}};
}

LayoutAxisPlacement
resolve_padded_horizontal_assignment(
    LayoutFlagSet flags,
    float assigned_size,
    float required_size,
    float padding)
{
    auto placement = resolve_horizontal_assignment(
        flags, assigned_size, required_size + padding * 2);
    return LayoutAxisPlacement{
        .offset = placement.offset + padding,
        .size = placement.size - padding * 2};
}

LayoutAxisPlacement
resolve_padded_vertical_assignment(
    LayoutFlagSet flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent,
    float padding)
{
    auto placement = resolve_vertical_assignment(
        flags, assigned_size, baseline, required_size + padding * 2, ascent);
    return LayoutAxisPlacement{
        .offset = placement.offset + padding,
        .size = placement.size - padding * 2};
}

Box
resolve_padded_assignment(
    LayoutFlagSet flags,
    Vec2 assigned_size,
    float baseline,
    Vec2 required_size,
    float ascent,
    float padding)
{
    LayoutAxisPlacement x_placement = resolve_padded_horizontal_assignment(
        flags, assigned_size.x, required_size.x, padding);
    LayoutAxisPlacement y_placement = resolve_padded_vertical_assignment(
        flags, assigned_size.y, baseline, required_size.y, ascent, padding);
    return Box{
        Vec2{x_placement.offset, y_placement.offset},
        Vec2{x_placement.size, y_placement.size}};
}

} // namespace alia
