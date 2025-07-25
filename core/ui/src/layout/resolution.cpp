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

} // namespace alia
