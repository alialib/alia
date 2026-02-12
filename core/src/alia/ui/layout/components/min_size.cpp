#include <alia/abi/ui/style.h>
#include <alia/context.hpp>
#include <alia/events.hpp>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/ui/layout.hpp>

namespace alia {

struct min_size_node
{
    alia_layout_container container;
    alia_vec2f min_size;
};

alia_horizontal_requirements
min_size_measure_horizontal(
    alia_measurement_context* ctx, alia_layout_node* base_node)
{
    auto& node = *reinterpret_cast<min_size_node*>(base_node);
    auto const child_x
        = alia_measure_horizontal(ctx, node.container.first_child);
    return alia_horizontal_requirements{
        .min_size = std::max(node.min_size.x, child_x.min_size),
        .growth_factor = child_x.growth_factor};
}

void
min_size_assign_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* base_node,
    float assigned_width)
{
    auto& node = *reinterpret_cast<min_size_node*>(base_node);
    alia_assign_widths(
        ctx, main_axis, node.container.first_child, assigned_width);
}

alia_vertical_requirements
min_size_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* base_node,
    float assigned_width)
{
    auto& node = *reinterpret_cast<min_size_node*>(base_node);
    auto const child_y = alia_measure_vertical(
        ctx, main_axis, node.container.first_child, assigned_width);
    return alia_vertical_requirements{
        .min_size = std::max(node.min_size.y, child_y.min_size),
        .growth_factor = child_y.growth_factor,
        .ascent = child_y.ascent,
        .descent = child_y.descent};
}

void
min_size_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* base_node,
    alia_box box,
    float baseline)
{
    auto& node = *reinterpret_cast<min_size_node*>(base_node);
    alia_assign_boxes(
        ctx, main_axis, node.container.first_child, box, baseline);
}

alia_layout_node_vtable min_size_vtable
    = {min_size_measure_horizontal,
       min_size_assign_widths,
       min_size_measure_vertical,
       min_size_assign_boxes,
       min_size_measure_horizontal,
       alia_default_measure_wrapped_vertical,
       nullptr};

} // namespace alia

using namespace alia;

extern "C" {

struct alia_layout_min_size_scope
{
    min_size_node* node;
};

void
alia_layout_min_size_begin(alia_context* ctx, alia_vec2f min_size)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_push<alia_layout_min_size_scope>(ctx);
        auto& layout = as_refresh_event(*ctx).layout_emission;
        auto* node = arena_alloc<min_size_node>(*layout.arena);
        *node = min_size_node{
            .container
            = {.base = {.vtable = &min_size_vtable, .next_sibling = 0},
               .flags = 0,
               .first_child = 0},
            .min_size = min_size};
        scope.node = node;
        alia_layout_container_activate(ctx, &node->container);
    }
}

void
alia_layout_min_size_end(alia_context* ctx)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_pop<alia_layout_min_size_scope>(ctx);
        alia_layout_container_deactivate(ctx, &scope.node->container);
    }
}

} // extern "C"
