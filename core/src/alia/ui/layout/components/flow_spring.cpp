#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/layout/protocol.h>
#include <alia/abi/ui/layout/utilities/defaults.h>
#include <alia/abi/ui/layout/utilities/flow.h>

#include <alia/impl/events.hpp>
#include <alia/impl/ui/layout.hpp>

namespace alia {

struct layout_flow_spring_node
{
    alia_layout_node base;
    float min_width;
};

alia_horizontal_requirements
flow_spring_measure_horizontal(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    (void) ctx;
    auto& spring = *reinterpret_cast<layout_flow_spring_node*>(node);
    return alia_horizontal_requirements{
        .min_size = spring.min_width, .growth_factor = 0.f};
}

alia_vertical_requirements
flow_spring_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    (void) ctx;
    (void) main_axis;
    (void) node;
    (void) assigned_width;
    return alia_vertical_requirements{
        .min_size = 0.f, .growth_factor = 0.f, .ascent = 0.f, .descent = 0.f};
}

void
flow_spring_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    (void) main_axis;
    (void) baseline;
    auto& spring = *reinterpret_cast<layout_flow_spring_node*>(node);
    alia_box* placement = arena_alloc<alia_box>(ctx->arena);
    placement->min = box.min;
    placement->size = alia_vec2f_make(spring.min_width, 0.f);
}

alia_flow_emission_counts
flow_spring_count_flow_emissions(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    (void) ctx;
    (void) node;
    return alia_flow_emission_counts{.fragment_count = 1};
}

void
flow_spring_emit_flow_fragments(
    alia_measurement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_emitter* emitter)
{
    auto& spring = *reinterpret_cast<layout_flow_spring_node*>(node);
    auto const marker = alia_arena_mark(&ctx->scratch);
    auto vertical
        = alia_measure_vertical(ctx, ALIA_MAIN_AXIS_X, node, spring.min_width);
    alia_arena_jump(&ctx->scratch, marker);
    alia_layout_emit_flow_fragment(
        emitter,
        alia_flow_fragment{
            .flags = ALIA_FLOW_FRAGMENT_EXPANDABLE,
            .kind = ALIA_FLOW_FRAGMENT_KIND_CONTENT,
            .content = alia_layout_content_metrics{
                .size = alia_vec2f_make(spring.min_width, vertical.min_size),
                .ascent = vertical.ascent,
                .descent = vertical.descent}});
}

alia_layout_node_vtable flow_spring_vtable
    = {flow_spring_measure_horizontal,
       flow_spring_measure_vertical,
       flow_spring_assign_boxes,
       flow_spring_count_flow_emissions,
       flow_spring_emit_flow_fragments,
       alia_default_read_fragment_placements};

} // namespace alia

using namespace alia;

extern "C" {

void
alia_layout_flow_spring_emit(alia_context* ctx, float min_width)
{
    auto& emission = ctx->layout->emission;
    layout_flow_spring_node* new_node
        = arena_alloc<layout_flow_spring_node>(emission.arena);
    *emission.next_ptr = &new_node->base;
    emission.next_ptr = &new_node->base.next_sibling;
    *new_node = layout_flow_spring_node{
        .base = {.vtable = &flow_spring_vtable, .next_sibling = 0},
        .min_width = min_width};
}

} // extern "C"
