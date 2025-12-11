#include <alia/layout/compositors/column.hpp>

#include <alia/layout/utilities.hpp>

namespace alia {

void
begin_column(
    context& ctx, layout_container_scope& scope, layout_flag_set flags)
{
    begin_container(ctx, scope, &column_vtable, flags);
}

void
end_column(context& ctx, layout_container_scope& scope)
{
    end_container(ctx, scope);
}

struct column_scratch
{
    std::uint32_t child_count = 0;
    float max_width = 0;
    float total_height = 0, total_growth = 0;
    float baseline = 0;
};

horizontal_requirements
column_measure_horizontal(measurement_context* ctx, layout_node* node)
{
    auto& column = *reinterpret_cast<column_layout_node*>(node);
    auto& scratch = claim_scratch<column_scratch>(*ctx->scratch);

    for (layout_node* child = column.first_child; child != nullptr;
         child = child->next_sibling)
    {
        ++scratch.child_count;
    }
    vertical_requirements* y_requirements
        = arena_array_alloc<vertical_requirements>(
            *ctx->scratch, scratch.child_count);

    for (layout_node* child = column.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = measure_horizontal(ctx, child);
        scratch.max_width = (std::max) (scratch.max_width, child_x.min_size);
    }
    return horizontal_requirements{
        .min_size = scratch.max_width,
        .growth_factor = resolve_growth_factor(column.flags)};
}

void
column_assign_widths(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& column = *reinterpret_cast<column_layout_node*>(node);
    auto& scratch = use_scratch<column_scratch>(*ctx->scratch);
    vertical_requirements* y_requirements
        = arena_array_alloc<vertical_requirements>(
            *ctx->scratch, scratch.child_count);
    auto const assignment = resolve_horizontal_assignment(
        adjust_flags_for_main_axis(column.flags, main_axis),
        assigned_width,
        scratch.max_width);
    for (layout_node* child = column.first_child; child != nullptr;
         child = child->next_sibling)
    {
        assign_widths(ctx, ALIA_MAIN_AXIS_Y, child, assignment.size);
    }
}

vertical_requirements
column_measure_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& column = *reinterpret_cast<column_layout_node*>(node);
    auto& scratch = use_scratch<column_scratch>(*ctx->scratch);
    vertical_requirements* y_requirements
        = arena_array_alloc<vertical_requirements>(
            *ctx->scratch, scratch.child_count);
    auto const assignment = resolve_horizontal_assignment(
        adjust_flags_for_main_axis(column.flags, main_axis),
        assigned_width,
        scratch.max_width);
    vertical_requirements* requirement_i = y_requirements;
    for (layout_node* child = column.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_y
            = measure_vertical(ctx, ALIA_MAIN_AXIS_Y, child, assignment.size);
        *requirement_i++ = child_y;
        scratch.total_height += child_y.min_size;
        scratch.total_growth += child_y.growth_factor;
    }
    scratch.baseline = (scratch.child_count > 0) ? y_requirements->ascent : 0;
    return vertical_requirements{
        .min_size = scratch.total_height,
        .growth_factor = resolve_growth_factor(column.flags),
        .ascent = scratch.baseline,
        .descent = scratch.total_height - scratch.baseline};
}

void
column_assign_boxes(
    placement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    box box,
    float baseline)
{
    auto& column = *reinterpret_cast<column_layout_node*>(node);
    auto& scratch = use_scratch<column_scratch>(*ctx->scratch);
    vertical_requirements* y_requirements
        = arena_array_alloc<vertical_requirements>(
            *ctx->scratch, scratch.child_count);
    auto const assignment = resolve_assignment(
        adjust_flags_for_main_axis(column.flags, main_axis),
        box.size,
        baseline,
        {scratch.max_width, scratch.total_height},
        scratch.baseline);
    float const total_extra_space
        = (std::max) (0.f, assignment.size.y - scratch.total_height);
    // TODO: Figure out how to handle 0 total growth.
    float const total_growth = (std::max) (0.00001f, scratch.total_growth);
    float current_y = box.pos.y + assignment.pos.y;
    for (layout_node* child = column.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_y = *y_requirements++;
        float const extra_space
            = total_extra_space * child_y.growth_factor / total_growth;
        assign_boxes(
            ctx,
            ALIA_MAIN_AXIS_Y,
            child,
            {.pos = vec2{box.pos.x + assignment.pos.x, current_y},
             .size = vec2{assignment.size.x, child_y.min_size + extra_space}},
            assign_baseline(
                column.flags,
                assignment.size.y,
                child_y.ascent,
                child_y.descent));
        current_y += child_y.min_size + extra_space;
    }
}

layout_node_vtable column_vtable
    = {column_measure_horizontal,
       column_assign_widths,
       column_measure_vertical,
       column_assign_boxes,
       column_measure_horizontal,
       default_measure_wrapped_vertical,
       nullptr};

} // namespace alia
