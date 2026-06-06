#include <alia/abi/ui/layout/utilities.h>
#include <alia/abi/ui/style.h>
#include <alia/context.h>
#include <alia/impl/base/arena.hpp>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>

using namespace alia::operators;

namespace alia {

struct inset_layout_node
{
    alia_layout_container container;
    alia_insets insets;
};

alia_horizontal_requirements
inset_measure_horizontal(alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);
    auto const child_x
        = alia_measure_horizontal(ctx, inset.container.first_child);
    return alia_horizontal_requirements{
        .min_size = child_x.min_size + inset.insets.left + inset.insets.right,
        .growth_factor = child_x.growth_factor};
}

void
inset_assign_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);
    alia_assign_widths(
        ctx,
        main_axis,
        inset.container.first_child,
        assigned_width - inset.insets.left - inset.insets.right);
}

alia_vertical_requirements
inset_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);
    auto const child_y = alia_measure_vertical(
        ctx,
        main_axis,
        inset.container.first_child,
        assigned_width - inset.insets.top - inset.insets.bottom);
    return alia_vertical_requirements{
        .min_size = child_y.min_size + inset.insets.top + inset.insets.bottom,
        .growth_factor = child_y.growth_factor,
        .ascent = child_y.ascent + inset.insets.top,
        .descent = child_y.descent + inset.insets.bottom};
}

void
inset_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);
    auto const child_box = alia_box{
        .min = box.min + alia_vec2f{inset.insets.left, inset.insets.top},
        .size = box.size
              - alia_vec2f{
                  inset.insets.left + inset.insets.right,
                  inset.insets.top + inset.insets.bottom}};
    alia_assign_boxes(
        ctx,
        main_axis,
        inset.container.first_child,
        child_box,
        baseline - inset.insets.top);
}

alia_flow_emission_counts
inset_count_flow_emissions(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);
    auto const child_counts
        = alia_count_flow_emissions(ctx, inset.container.first_child);
    return alia_flow_emission_counts{
        .fragment_count = child_counts.fragment_count,
        .run_count = child_counts.run_count + 1};
}

void
inset_emit_flow_fragments(
    alia_measurement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_emitter* emitter)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);
    alia_flow_run_index const saved_run_index = emitter->active_run_index;
    alia_flow_run_index const run_index = alia_flow_register_run(
        emitter, alia_flow_run_style{.padding = inset.insets});
    emitter->active_run_index = run_index;
    alia_emit_flow_fragments(ctx, inset.container.first_child, emitter);
    emitter->active_run_index = saved_run_index;
}

void
inset_read_fragment_placements(
    alia_placement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_reader* reader)
{
    auto& inset = *reinterpret_cast<inset_layout_node*>(node);
    alia_layout_read_fragment_placements(
        ctx, inset.container.first_child, reader);
}

alia_layout_node_vtable inset_vtable
    = {inset_measure_horizontal,
       inset_assign_widths,
       inset_measure_vertical,
       inset_assign_boxes,
       inset_count_flow_emissions,
       inset_emit_flow_fragments,
       inset_read_fragment_placements};

} // namespace alia

extern "C" {

using namespace alia;

struct alia_layout_inset_scope
{
    inset_layout_node* node;
};

void
alia_layout_inset_begin(
    alia_context* ctx, alia_insets insets, alia_layout_flags_t flags)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_push<alia_layout_inset_scope>(ctx);
        inset_layout_node* node
            = arena_alloc<inset_layout_node>(ctx->layout->emission.arena);
        *node = inset_layout_node{
            .container
            = {.base = {.vtable = &inset_vtable, .next_sibling = 0},
               .flags = flags,
               .first_child = 0},
            .insets = insets};
        scope.node = node;
        alia_layout_container_activate(ctx, &node->container);
    }
}

void
alia_layout_inset_end(alia_context* ctx)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_pop<alia_layout_inset_scope>(ctx);
        alia_layout_container_deactivate(ctx, &scope.node->container);
    }
}

} // extern "C"
