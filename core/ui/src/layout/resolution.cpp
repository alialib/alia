#include <alia/ui/layout/resolution.hpp>

namespace alia {

LayoutPlacementNode*
resolve_layout(
    LayoutScratchArena& scratch,
    LayoutPlacementArena& arena,
    LayoutNode& root_node,
    Vec2 available_space)
{
    scratch.reset();
    measure_horizontal(&scratch, &root_node);
    scratch.reset();
    measure_vertical(&scratch, &root_node, available_space.x);
    scratch.reset();
    LayoutPlacementNode* initial_placement = nullptr;
    PlacementContext ctx{&scratch, &arena, &initial_placement};
    assign_boxes(&ctx, &root_node, Box{Vec2{0, 0}, available_space});
    scratch.reset();
    return initial_placement;
}

HorizontalRequirements
default_measure_wrapped_horizontal(
    LayoutScratchArena* scratch, LayoutNode* node)
{
    return measure_horizontal(scratch, node);
}

WrappingRequirements
default_measure_wrapped_vertical(
    LayoutScratchArena* scratch,
    LayoutNode* node,
    float current_x_offset,
    float line_width)
{
    auto checkpoint = scratch->save_state();
    auto horizontal = measure_horizontal(scratch, node);
    scratch->restore_state(checkpoint);
    auto vertical = measure_vertical(scratch, node, horizontal.min_size);
    if (current_x_offset + horizontal.min_size > line_width)
    {
        return WrappingRequirements{
            .line_height = vertical.min_size,
            .ascent = vertical.ascent,
            .descent = vertical.descent,
            .wrap_count = 1,
            .wrapped_immediately = true,
            .new_x_offset = horizontal.min_size};
    }
    else
    {
        return WrappingRequirements{
            .line_height = vertical.min_size,
            .ascent = vertical.ascent,
            .descent = vertical.descent,
            .wrap_count = 0,
            .wrapped_immediately = false,
            .new_x_offset = current_x_offset + horizontal.min_size};
    }
}

} // namespace alia
