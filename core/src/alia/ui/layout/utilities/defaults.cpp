#include <alia/abi/ui/layout/utilities/defaults.h>

#include <alia/abi/ui/layout/utilities/dispatch.h>
#include <alia/abi/ui/layout/utilities/flow.h>
#include <alia/impl/base/arena.hpp>

extern "C" {

alia_flow_emission_counts
alia_default_count_flow_emissions(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    (void) ctx;
    (void) node;
    return alia_flow_emission_counts{.fragment_count = 1};
}

void
alia_default_emit_flow_fragments(
    alia_measurement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_emitter* emitter)
{
    auto const marker = alia_arena_mark(&ctx->scratch);
    auto horizontal = alia_measure_horizontal(ctx, node);
    alia_arena_jump(&ctx->scratch, marker);
    auto vertical = alia_measure_vertical(
        ctx, ALIA_MAIN_AXIS_X, node, horizontal.min_size);
    alia_layout_emit_flow_fragment(
        emitter,
        alia_flow_fragment{
            .flags = 0,
            .content = alia_layout_content_metrics{
                .size = alia_vec2f_make(
                    horizontal.min_size, vertical.min_size),
                .ascent = vertical.ascent,
                .descent = vertical.descent}});
}

void
alia_default_read_fragment_placements(
    alia_placement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_reader* reader)
{
    auto const* fragment = alia_layout_read_fragment_spec(reader);
    auto const* placement = alia_layout_read_fragment_placement(reader);
    alia_layout_advance_fragment(reader);
    auto const* content = alia_flow_fragment_content(fragment);
    alia_assign_boxes(
        ctx,
        ALIA_MAIN_AXIS_X,
        node,
        alia_box{placement->position, content->size},
        placement->baseline);
}

} // extern "C"
