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
    switch (raw_code(flags & X_ALIGNMENT_MASK))
    {
        case CENTER_X_CODE:
            return LayoutAxisPlacement{
                .offset = (assigned_size - required_size) / 2,
                .size = required_size};
        case LEFT_CODE:
            return LayoutAxisPlacement{.offset = 0, .size = required_size};
        case RIGHT_CODE:
            return LayoutAxisPlacement{
                .offset = assigned_size - required_size,
                .size = required_size};
        case FILL_X_CODE:
        default:
            return LayoutAxisPlacement{.offset = 0, .size = assigned_size};
    }
}

LayoutAxisPlacement
resolve_vertical_assignment(
    LayoutFlagSet flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent)
{
    switch (raw_code(flags & Y_ALIGNMENT_MASK))
    {
        case CENTER_Y_CODE:
            return LayoutAxisPlacement{
                .offset = (assigned_size - required_size) / 2,
                .size = required_size};
        case TOP_CODE:
            return LayoutAxisPlacement{.offset = 0, .size = required_size};
        case BOTTOM_CODE:
            return LayoutAxisPlacement{
                .offset = assigned_size - required_size,
                .size = required_size};
        case BASELINE_Y_CODE:
            return LayoutAxisPlacement{
                .offset = baseline - ascent, .size = required_size};
        case FILL_Y_CODE:
        default:
            return LayoutAxisPlacement{.offset = 0, .size = assigned_size};
    }
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
