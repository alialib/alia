#include <alia/layout/compositors/row.hpp>

#include <alia/layout/container.hpp>
#include <alia/layout/utilities.hpp>

namespace alia {

void
begin_row(context& ctx, layout_container_scope& scope, layout_flag_set flags)
{
    begin_container(ctx, scope, &row_vtable, flags);
}

void
end_row(context& ctx, layout_container_scope& scope)
{
    end_container(ctx, scope);
}

struct row_scratch
{
    std::uint32_t child_count = 0;
    float total_width = 0, total_growth = 0;
    float height = 0, ascent = 0;
};

horizontal_requirements
row_measure_horizontal(measurement_context* ctx, layout_node* node)
{
    auto& row = *reinterpret_cast<row_layout_node*>(node);
    auto& scratch = claim_scratch<row_scratch>(*ctx->scratch);
    for (layout_node* child = row.first_child; child != nullptr;
         child = child->next_sibling)
    {
        ++scratch.child_count;
    }
    horizontal_requirements* x_requirements
        = arena_array_alloc<horizontal_requirements>(
            *ctx->scratch, scratch.child_count);
    for (layout_node* child = row.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = measure_horizontal(ctx, child);
        *x_requirements++ = child_x;
        scratch.total_width += child_x.min_size;
        scratch.total_growth += child_x.growth_factor;
    }
    return horizontal_requirements{
        .min_size = scratch.total_width,
        .growth_factor = resolve_growth_factor(row.flags)};
}

void
row_assign_widths(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& row = *reinterpret_cast<row_layout_node*>(node);
    auto& scratch = use_scratch<row_scratch>(*ctx->scratch);
    horizontal_requirements* x_requirements
        = arena_array_alloc<horizontal_requirements>(
            *ctx->scratch, scratch.child_count);
    auto const placement = resolve_horizontal_assignment(
        adjust_flags_for_main_axis(row.flags, main_axis),
        assigned_width,
        scratch.total_width);
    float const total_extra_space
        = (std::max) (0.f, placement.size - scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max) (0.00001f, scratch.total_growth);
    for (layout_node* child = row.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *x_requirements++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        assign_widths(
            ctx, ALIA_MAIN_AXIS_X, child, child_x.min_size + extra_space);
    }
}

vertical_requirements
row_measure_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& row = *reinterpret_cast<row_layout_node*>(node);
    auto& scratch = use_scratch<row_scratch>(*ctx->scratch);
    horizontal_requirements* x_requirements
        = arena_array_alloc<horizontal_requirements>(
            *ctx->scratch, scratch.child_count);
    // TODO: Stop repeating this logic everywhere.
    auto const placement = resolve_horizontal_assignment(
        adjust_flags_for_main_axis(row.flags, main_axis),
        assigned_width,
        scratch.total_width);
    float const total_extra_space
        = (std::max) (0.f, placement.size - scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max) (0.00001f, scratch.total_growth);
    float height = 0, ascent = 0, descent = 0;
    for (layout_node* child = row.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *x_requirements++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        auto const child_y = measure_vertical(
            ctx, ALIA_MAIN_AXIS_X, child, child_x.min_size + extra_space);
        height = (std::max) (height, child_y.min_size);
        ascent = (std::max) (ascent, child_y.ascent);
        descent = (std::max) (descent, child_y.descent);
    }
    scratch.height = height;
    scratch.ascent = ascent;
    return vertical_requirements{
        .min_size = (std::max) (height, ascent + descent),
        .growth_factor = resolve_growth_factor(row.flags),
        .ascent = ascent,
        .descent = descent};
}

void
row_assign_boxes(
    placement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    box box,
    float baseline)
{
    auto& row = *reinterpret_cast<row_layout_node*>(node);
    auto& scratch = use_scratch<row_scratch>(*ctx->scratch);
    horizontal_requirements* x_requirements
        = arena_array_alloc<horizontal_requirements>(
            *ctx->scratch, scratch.child_count);
    auto const placement = resolve_assignment(
        adjust_flags_for_main_axis(row.flags, main_axis),
        box.size,
        baseline,
        vec2{scratch.total_width, scratch.height},
        scratch.ascent);
    float const total_extra_space
        = (std::max) (0.f, placement.size.x - scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max) (0.00001f, scratch.total_growth);
    float current_x = box.pos.x + placement.pos.x;
    for (layout_node* child = row.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *x_requirements++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        assign_boxes(
            ctx,
            ALIA_MAIN_AXIS_X,
            child,
            {.pos = vec2{current_x, box.pos.y + placement.pos.y},
             .size = vec2{child_x.min_size + extra_space, box.size.y}},
            baseline);
        current_x += child_x.min_size + extra_space;
    }
}

layout_node_vtable row_vtable
    = {row_measure_horizontal,
       row_assign_widths,
       row_measure_vertical,
       row_assign_boxes,
       row_measure_horizontal,
       default_measure_wrapped_vertical,
       nullptr};

} // namespace alia
