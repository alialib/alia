#include <alia/abi/ui/style.h>
#include <alia/impl/ui/layout.hpp>

namespace alia {

using row_layout_node = alia_layout_container;

struct row_scratch
{
    std::uint32_t child_count = 0;
    float total_width = 0, total_growth = 0;
    float height = 0, ascent = 0;
};

alia_horizontal_requirements
row_measure_horizontal(alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& row = *reinterpret_cast<row_layout_node*>(node);
    auto& scratch = claim_scratch<row_scratch>(ctx->scratch);
    for (alia_layout_node* child = row.first_child; child != nullptr;
         child = child->next_sibling)
    {
        ++scratch.child_count;
    }
    alia_horizontal_requirements* x_requirements
        = arena_alloc_array<alia_horizontal_requirements>(
            ctx->scratch, scratch.child_count);
    for (alia_layout_node* child = row.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = alia_measure_horizontal(ctx, child);
        *x_requirements++ = child_x;
        scratch.total_width += child_x.min_size;
        scratch.total_growth += child_x.growth_factor;
    }
    return alia_horizontal_requirements{
        .min_size = scratch.total_width,
        .growth_factor = alia_resolve_growth_factor(row.flags)};
}

void
row_assign_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& row = *reinterpret_cast<row_layout_node*>(node);
    auto& scratch = use_scratch<row_scratch>(ctx->scratch);
    alia_horizontal_requirements* x_requirements
        = arena_alloc_array<alia_horizontal_requirements>(
            ctx->scratch, scratch.child_count);
    auto const placement = alia_resolve_container_x(
        alia_fold_in_cross_axis_flags(row.flags, main_axis),
        assigned_width,
        scratch.total_width);
    float const total_extra_space
        = (std::max) (0.f, placement.size - scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max) (0.00001f, scratch.total_growth);
    for (alia_layout_node* child = row.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *x_requirements++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        alia_assign_widths(
            ctx, ALIA_MAIN_AXIS_X, child, child_x.min_size + extra_space);
    }
}

alia_vertical_requirements
row_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& row = *reinterpret_cast<row_layout_node*>(node);
    auto& scratch = use_scratch<row_scratch>(ctx->scratch);
    alia_horizontal_requirements* x_requirements
        = arena_alloc_array<alia_horizontal_requirements>(
            ctx->scratch, scratch.child_count);
    // TODO: Stop repeating this logic everywhere.
    auto const placement = alia_resolve_container_x(
        alia_fold_in_cross_axis_flags(row.flags, main_axis),
        assigned_width,
        scratch.total_width);
    float const total_extra_space
        = (std::max) (0.f, placement.size - scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max) (0.00001f, scratch.total_growth);
    float height = 0, ascent = 0, descent = 0;
    for (alia_layout_node* child = row.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *x_requirements++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        auto const child_y = alia_measure_vertical(
            ctx, ALIA_MAIN_AXIS_X, child, child_x.min_size + extra_space);
        height = (std::max) (height, child_y.min_size);
        ascent = (std::max) (ascent, child_y.ascent);
        descent = (std::max) (descent, child_y.descent);
    }
    scratch.height = height;
    scratch.ascent = ascent;
    return alia_vertical_requirements{
        .min_size = (std::max) (height, ascent + descent),
        .growth_factor = alia_resolve_growth_factor(row.flags),
        .ascent = ascent,
        .descent = descent};
}

void
row_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& row = *reinterpret_cast<row_layout_node*>(node);
    auto& scratch = use_scratch<row_scratch>(ctx->scratch);
    alia_horizontal_requirements* x_requirements
        = arena_alloc_array<alia_horizontal_requirements>(
            ctx->scratch, scratch.child_count);
    auto const placement = alia_resolve_container_box(
        alia_fold_in_cross_axis_flags(row.flags, main_axis),
        box.size,
        baseline,
        {scratch.total_width, scratch.height},
        scratch.ascent);
    float const total_extra_space
        = (std::max) (0.f, placement.size.x - scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max) (0.00001f, scratch.total_growth);
    float current_x = box.min.x + placement.min.x;
    for (alia_layout_node* child = row.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *x_requirements++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        alia_assign_boxes(
            ctx,
            ALIA_MAIN_AXIS_X,
            child,
            {.min = {current_x, box.min.y + placement.min.y},
             .size = {child_x.min_size + extra_space, box.size.y}},
            baseline);
        current_x += child_x.min_size + extra_space;
    }
}

alia_layout_node_vtable row_vtable
    = {row_measure_horizontal,
       row_assign_widths,
       row_measure_vertical,
       row_assign_boxes,
       row_measure_horizontal,
       alia_default_measure_wrapped_vertical,
       nullptr};

} // namespace alia

extern "C" {

void
alia_layout_row_begin(alia_context* ctx, alia_layout_flags_t flags)
{
    alia_layout_container_simple_begin(ctx, &alia::row_vtable, flags);
}

void
alia_layout_row_end(alia_context* ctx)
{
    alia_layout_container_simple_end(ctx);
}

} // extern "C"
