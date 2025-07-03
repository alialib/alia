#include <alia/ui/layout/resolution.hpp>

#include <alia/ui/layout/flow.hpp>
#include <alia/ui/layout/hbox.hpp>
#include <alia/ui/layout/vbox.hpp>
namespace alia {

LayoutPlacement*
resolve_layout(
    LayoutScratchArena& scratch,
    LayoutPlacementArena& arena,
    LayoutNode& root_node,
    Vec2 available_space)
{
    scratch.reset();
    gather_x_requirements(&scratch, &root_node);
    scratch.reset();
    assign_widths(&scratch, &root_node, available_space.x);
    scratch.reset();
    gather_y_requirements(&scratch, &root_node, available_space.x);
    scratch.reset();
    LayoutPlacement* initial_placement = nullptr;
    PlacementContext ctx{&scratch, &arena, &initial_placement};
    assign_boxes(&ctx, &root_node, Box{Vec2{0, 0}, available_space});
    scratch.reset();
    return initial_placement;
}

} // namespace alia
