#include <alia/ui/layout/resolution.hpp>

namespace alia {

LayoutPlacementNode*
resolve_layout(
    LayoutScratchArena& scratch,
    LayoutPlacementArena& arena,
    LayoutNode& root_node,
    Vec2 available_space)
{
    VerticalRequirements vertical;
    {
        MeasurementContext ctx{&scratch};
        scratch.reset();
        measure_horizontal(&ctx, &root_node);
        scratch.reset();
        vertical = measure_vertical(&ctx, &root_node, available_space.x);
    }
    LayoutPlacementNode* initial_placement = nullptr;
    {
        PlacementContext ctx{&scratch, &arena, &initial_placement};
        scratch.reset();
        assign_boxes(
            &ctx,
            &root_node,
            Box{Vec2{0, 0}, available_space},
            vertical.ascent);
        scratch.reset();
    }
    return initial_placement;
}

HorizontalRequirements
default_measure_wrapped_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    return measure_horizontal(ctx, node);
}

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
resolve_axis_assignment(
    LayoutAlignment alignment,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent)
{
    switch (alignment)
    {
        case LayoutAlignment::Center:
            return LayoutAxisPlacement{
                .offset = (assigned_size - required_size) / 2,
                .size = required_size};
        case LayoutAlignment::Start:
            return LayoutAxisPlacement{.offset = 0, .size = required_size};
        case LayoutAlignment::End:
            return LayoutAxisPlacement{
                .offset = assigned_size - required_size,
                .size = required_size};
        case LayoutAlignment::Baseline:
            return LayoutAxisPlacement{
                .offset = baseline - ascent, .size = required_size};
        case LayoutAlignment::Fill:
        default:
            return LayoutAxisPlacement{.offset = 0, .size = assigned_size};
    }
}

Box
resolve_assignment(
    LayoutProperties props,
    Vec2 assigned_size,
    float baseline,
    Vec2 required_size,
    float ascent)
{
    LayoutAxisPlacement x_placement = resolve_axis_assignment(
        props.x_alignment, assigned_size.x, 0, required_size.x, 0);
    LayoutAxisPlacement y_placement = resolve_axis_assignment(
        props.y_alignment, assigned_size.y, baseline, required_size.y, ascent);
    return Box{
        Vec2{x_placement.offset, y_placement.offset},
        Vec2{x_placement.size, y_placement.size}};
}

LayoutAxisPlacement
resolve_padded_axis_assignment(
    LayoutAlignment alignment,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent,
    float padding)
{
    auto placement = resolve_axis_assignment(
        alignment,
        assigned_size,
        baseline,
        required_size + padding * 2,
        ascent + padding);
    return LayoutAxisPlacement{
        .offset = placement.offset + padding,
        .size = placement.size - padding * 2};
}

Box
resolve_padded_assignment(
    LayoutProperties props,
    Vec2 assigned_size,
    float baseline,
    Vec2 required_size,
    float ascent,
    float padding)
{
    LayoutAxisPlacement x_placement = resolve_padded_axis_assignment(
        props.x_alignment, assigned_size.x, 0, required_size.x, 0, padding);
    LayoutAxisPlacement y_placement = resolve_padded_axis_assignment(
        props.y_alignment,
        assigned_size.y,
        baseline,
        required_size.y,
        ascent,
        padding);
    return Box{
        Vec2{x_placement.offset, y_placement.offset},
        Vec2{x_placement.size, y_placement.size}};
}

} // namespace alia
