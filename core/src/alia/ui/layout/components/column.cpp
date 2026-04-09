#include <alia/ui/layout/components/column.h>

#include <alia/abi/ui/style.h>
#include <alia/impl/ui/layout.hpp>

namespace alia {

struct column_scratch
{
    std::uint32_t child_count = 0;
    float max_width = 0;
    float total_height = 0, total_growth = 0;
    float baseline = 0;
};

alia_horizontal_requirements
column_measure_horizontal(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& column = *reinterpret_cast<column_layout_node*>(node);
    auto& scratch = claim_scratch<column_scratch>(ctx->scratch);

    for (alia_layout_node* child = column.first_child; child != nullptr;
         child = child->next_sibling)
    {
        ++scratch.child_count;
    }
    alia_vertical_requirements* y_requirements
        = arena_alloc_array<alia_vertical_requirements>(
            ctx->scratch, scratch.child_count);

    for (alia_layout_node* child = column.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = alia_measure_horizontal(ctx, child);
        scratch.max_width = (std::max) (scratch.max_width, child_x.min_size);
    }
    return alia_horizontal_requirements{
        .min_size = scratch.max_width,
        .growth_factor = alia_resolve_growth_factor(column.flags)};
}

void
column_assign_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& column = *reinterpret_cast<column_layout_node*>(node);
    auto& scratch = use_scratch<column_scratch>(ctx->scratch);
    alia_vertical_requirements* y_requirements
        = arena_alloc_array<alia_vertical_requirements>(
            ctx->scratch, scratch.child_count);
    auto const assignment = alia_resolve_container_x(
        alia_fold_in_cross_axis_flags(column.flags, main_axis),
        assigned_width,
        scratch.max_width);
    for (alia_layout_node* child = column.first_child; child != nullptr;
         child = child->next_sibling)
    {
        alia_assign_widths(ctx, ALIA_MAIN_AXIS_Y, child, assignment.size);
    }
}

alia_vertical_requirements
column_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& column = *reinterpret_cast<column_layout_node*>(node);
    auto& scratch = use_scratch<column_scratch>(ctx->scratch);
    alia_vertical_requirements* y_requirements
        = arena_alloc_array<alia_vertical_requirements>(
            ctx->scratch, scratch.child_count);
    auto const assignment = alia_resolve_container_x(
        alia_fold_in_cross_axis_flags(column.flags, main_axis),
        assigned_width,
        scratch.max_width);
    alia_vertical_requirements* requirement_i = y_requirements;
    for (alia_layout_node* child = column.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_y = alia_measure_vertical(
            ctx, ALIA_MAIN_AXIS_Y, child, assignment.size);
        *requirement_i++ = child_y;
        scratch.total_height += child_y.min_size;
        scratch.total_growth += child_y.growth_factor;
    }
    scratch.baseline = (scratch.child_count > 0) ? y_requirements->ascent : 0;
    return alia_vertical_requirements{
        .min_size = scratch.total_height,
        .growth_factor = alia_resolve_growth_factor(column.flags),
        .ascent = scratch.baseline,
        .descent = scratch.total_height - scratch.baseline};
}

void
column_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& column = *reinterpret_cast<column_layout_node*>(node);
    auto& scratch = use_scratch<column_scratch>(ctx->scratch);
    alia_vertical_requirements* y_requirements
        = arena_alloc_array<alia_vertical_requirements>(
            ctx->scratch, scratch.child_count);
    auto const assignment = alia_resolve_container_box(
        alia_fold_in_cross_axis_flags(column.flags, main_axis),
        box.size,
        baseline,
        {scratch.max_width, scratch.total_height},
        scratch.baseline);
    float const total_extra_space
        = (std::max) (0.f, assignment.size.y - scratch.total_height);
    // TODO: Figure out how to handle 0 total growth.
    float const total_growth = (std::max) (0.00001f, scratch.total_growth);
    float current_y = box.min.y + assignment.min.y;
    for (alia_layout_node* child = column.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_y = *y_requirements++;
        float const extra_space
            = total_extra_space * child_y.growth_factor / total_growth;
        float const assigned_height = child_y.min_size + extra_space;
        alia_assign_boxes(
            ctx,
            ALIA_MAIN_AXIS_Y,
            child,
            {.min = {box.min.x + assignment.min.x, current_y},
             .size = {assignment.size.x, assigned_height}},
            alia_resolve_baseline(
                column.flags,
                assigned_height,
                child_y.ascent,
                child_y.descent));
        current_y += assigned_height;
    }
}

alia_layout_node_vtable column_vtable
    = {column_measure_horizontal,
       column_assign_widths,
       column_measure_vertical,
       column_assign_boxes,
       column_measure_horizontal,
       alia_default_measure_wrapped_vertical,
       nullptr};

} // namespace alia

extern "C" {

void
alia_layout_column_begin(alia_context* ctx, alia_layout_flags_t flags)
{
    alia_layout_container_simple_begin(ctx, &alia::column_vtable, flags);
}

void
alia_layout_column_end(alia_context* ctx)
{
    alia_layout_container_simple_end(ctx);
}

} // extern "C"
