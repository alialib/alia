#include <alia/abi/ui/layout/api.h>
#include <alia/context.h>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>
#include <alia/impl/ui/layout.hpp>

#include <algorithm>

namespace alia {

struct min_axis_size_node
{
    alia_layout_container container;
    // minimum size as (length, breadth) in the parent's main/cross frame
    alia_vec2f min_lb;
};

// Map (length, breadth) to absolute (x, y) for `main_axis`.
static alia_vec2f
min_axis_size_absolute(alia_vec2f min_lb, alia_main_axis_index main_axis)
{
    if (main_axis == ALIA_MAIN_AXIS_X)
        return min_lb;
    return alia_vec2f_make(min_lb.y, min_lb.x);
}

alia_horizontal_requirements
min_axis_size_measure_horizontal(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* base_node)
{
    auto& node = *reinterpret_cast<min_axis_size_node*>(base_node);
    alia_vec2f const min_xy = min_axis_size_absolute(node.min_lb, main_axis);
    if (!node.container.first_child)
    {
        return alia_horizontal_requirements{
            .min_size = min_xy.x, .growth_factor = 0.f};
    }
    auto const child_x
        = alia_measure_horizontal(ctx, main_axis, node.container.first_child);
    return alia_horizontal_requirements{
        .min_size = (std::max)(min_xy.x, child_x.min_size),
        .growth_factor = child_x.growth_factor};
}

alia_vertical_requirements
min_axis_size_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* base_node,
    float assigned_width)
{
    auto& node = *reinterpret_cast<min_axis_size_node*>(base_node);
    alia_vec2f const min_xy = min_axis_size_absolute(node.min_lb, main_axis);
    if (!node.container.first_child)
    {
        float const h = min_xy.y;
        return alia_vertical_requirements{
            .min_size = h, .growth_factor = 0.f, .ascent = h, .descent = 0.f};
    }
    auto const child_y = alia_measure_vertical(
        ctx, main_axis, node.container.first_child, assigned_width);
    return alia_vertical_requirements{
        .min_size = (std::max)(min_xy.y, child_y.min_size),
        .growth_factor = child_y.growth_factor,
        .ascent = child_y.ascent,
        .descent = child_y.descent};
}

void
min_axis_size_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* base_node,
    alia_box box,
    float baseline)
{
    auto& node = *reinterpret_cast<min_axis_size_node*>(base_node);
    if (!node.container.first_child)
        return;
    alia_assign_boxes(
        ctx, main_axis, node.container.first_child, box, baseline);
}

alia_layout_node_vtable min_axis_size_vtable
    = {min_axis_size_measure_horizontal,
       min_axis_size_measure_vertical,
       min_axis_size_assign_boxes,
       alia_default_count_flow_emissions,
       alia_default_emit_flow_fragments,
       alia_default_read_fragment_placements};

} // namespace alia

using namespace alia;

extern "C" {

struct alia_layout_min_axis_size_scope
{
    min_axis_size_node* node;
};

void
alia_layout_min_axis_size_begin(alia_context* ctx, alia_vec2f min_lb)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_push<alia_layout_min_axis_size_scope>(ctx);
        auto* node
            = arena_alloc<min_axis_size_node>(ctx->layout->emission.arena);
        *node = min_axis_size_node{
            .container
            = {.base = {.vtable = &min_axis_size_vtable, .next_sibling = 0},
               .flags = 0,
               .first_child = 0},
            .min_lb = min_lb};
        scope.node = node;
        alia_layout_container_activate(ctx, &node->container);
    }
}

void
alia_layout_min_axis_size_end(alia_context* ctx)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_pop<alia_layout_min_axis_size_scope>(ctx);
        alia_layout_container_deactivate(ctx, &scope.node->container);
    }
}

} // extern "C"
